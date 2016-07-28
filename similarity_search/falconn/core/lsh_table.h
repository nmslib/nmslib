#ifndef __LSH_TABLE_H__
#define __LSH_TABLE_H__

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <future>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "../falconn_global.h"
#include "data_storage.h"

namespace falconn {
namespace core {

class LSHTableError : public FalconnError {
 public:
  LSHTableError(const char* msg) : FalconnError(msg) {}
};

// An LSH implementation for a single set of LSH functions.
// The actual LSH (and low-level hashing) implementations can be added via
// template parameters.

template <typename LSH,        // the LSH family
          typename HashTable,  // the low-level hash tables
          typename Derived>
class BasicLSHTable {
 public:
  BasicLSHTable(LSH* lsh, HashTable* hash_table)
      : lsh_(lsh), hash_table_(hash_table) {
    if (lsh_ == nullptr) {
      throw LSHTableError("The LSH object cannot be a nullptr.");
    }
    if (hash_table_ == nullptr) {
      throw LSHTableError("The low-level hash table cannot be a nullptr.");
    }
    if (lsh_->get_l() != hash_table_->get_l()) {
      throw LSHTableError(
          "Number of tables in LSH and low level hash table"
          " objects does not match.");
    }
  }

  LSH* LSH_object() { return lsh_; }

  HashTable* low_level_hash_table() { return hash_table_; }

 protected:
  LSH* lsh_;
  HashTable* hash_table_;
};

template <typename PointType,  // the type of the data points to be stored
          typename KeyType,    // must be integral for a static table
          typename LSH,        // the LSH family
          typename HashType,   // type returned by a set of k LSH functions
          typename HashTable,  // the low-level hash tables
          typename DataStorageType = ArrayDataStorage<PointType, KeyType>>
class StaticLSHTable
    : public BasicLSHTable<LSH, HashTable,
                           StaticLSHTable<PointType, KeyType, LSH, HashType,
                                          HashTable, DataStorageType>> {
 public:
  StaticLSHTable(LSH* lsh, HashTable* hash_table, const DataStorageType& points,
                 int_fast32_t num_setup_threads)
      : BasicLSHTable<LSH, HashTable,
                      StaticLSHTable<PointType, KeyType, LSH, HashType,
                                     HashTable, DataStorageType>>(lsh,
                                                                  hash_table),
        n_(points.size()) {
    if (num_setup_threads < 0) {
      throw LSHTableError("Number of setup threads cannot be negative.");
    }
    if (num_setup_threads == 0) {
      num_setup_threads = std::max(1u, std::thread::hardware_concurrency());
    }
    int_fast32_t l = this->lsh_->get_l();

    num_setup_threads = std::min(l, num_setup_threads);
    int_fast32_t num_tables_per_thread = l / num_setup_threads;
    int_fast32_t num_leftover_tables = l % num_setup_threads;

    std::vector<std::future<void>> thread_results;
    int_fast32_t next_table_range_start = 0;

    for (int_fast32_t ii = 0; ii < num_setup_threads; ++ii) {
      int_fast32_t next_table_range_end =
          next_table_range_start + num_tables_per_thread - 1;
      if (ii < num_leftover_tables) {
        next_table_range_end += 1;
      }
      thread_results.push_back(std::async(
          std::launch::async, &StaticLSHTable::setup_table_range, this,
          next_table_range_start, next_table_range_end, points));
      next_table_range_start = next_table_range_end + 1;
    }

    for (int_fast32_t ii = 0; ii < num_setup_threads; ++ii) {
      thread_results[ii].get();
    }
  }

  // TODO: add query statistics back in
  class Query {
   public:
    Query(const StaticLSHTable& parent)
        : parent_(parent),
          is_candidate_(parent.n_),
          lsh_query_(*(parent.lsh_)) {}

    void get_candidates_with_duplicates(const PointType& p,
                                        int_fast64_t num_probes,
                                        int_fast64_t max_num_candidates,
                                        std::vector<KeyType>* result) {
      auto start_time = std::chrono::high_resolution_clock::now();
      stats_num_queries_ += 1;

      lsh_query_.get_probes_by_table(p, &tmp_probes_by_table_, num_probes);

      auto lsh_end_time = std::chrono::high_resolution_clock::now();
      auto elapsed_lsh =
          std::chrono::duration_cast<std::chrono::duration<double>>(
              lsh_end_time - start_time);
      stats_.average_lsh_time += elapsed_lsh.count();

      hash_table_iterators_ =
          parent_.hash_table_->retrieve_bulk(tmp_probes_by_table_);

      int_fast64_t num_candidates = 0;
      result->clear();
      if (max_num_candidates < 0) {
        max_num_candidates = std::numeric_limits<int_fast64_t>::max();
      }
      while (num_candidates < max_num_candidates &&
             hash_table_iterators_.first != hash_table_iterators_.second) {
        num_candidates += 1;
        result->push_back(*(hash_table_iterators_.first));
        ++hash_table_iterators_.first;
      }

      auto hashing_end_time = std::chrono::high_resolution_clock::now();
      auto elapsed_hashing =
          std::chrono::duration_cast<std::chrono::duration<double>>(
              hashing_end_time - lsh_end_time);
      stats_.average_hash_table_time += elapsed_hashing.count();

      stats_.average_num_candidates += num_candidates;

      auto end_time = std::chrono::high_resolution_clock::now();
      auto elapsed_total =
          std::chrono::duration_cast<std::chrono::duration<double>>(end_time -
                                                                    start_time);
      stats_.average_total_query_time += elapsed_total.count();
    }

    void get_unique_candidates(const PointType& p, int_fast64_t num_probes,
                               int_fast64_t max_num_candidates,
                               std::vector<KeyType>* result) {
      auto start_time = std::chrono::high_resolution_clock::now();
      stats_num_queries_ += 1;

      get_unique_candidates_internal(p, num_probes, max_num_candidates, result);

      auto end_time = std::chrono::high_resolution_clock::now();
      auto elapsed_total =
          std::chrono::duration_cast<std::chrono::duration<double>>(end_time -
                                                                    start_time);
      stats_.average_total_query_time += elapsed_total.count();
    }

    /*void get_unique_sorted_candidates(const PointType& p,
                                      int_fast64_t num_probes,
                                      int_fast64_t max_num_candidates,
                                      std::vector<KeyType>* result) {
      auto start_time = std::chrono::high_resolution_clock::now();
      stats_num_queries_ += 1;

      get_unique_candidates_internal(p, num_probes, max_num_candidates, result);
      std::sort(result->begin(), result->end());

      auto end_time = std::chrono::high_resolution_clock::now();
      auto elapsed_total = std::chrono::duration_cast<
          std::chrono::duration<double>>(end_time - start_time);
      stats_.average_total_query_time += elapsed_total.count();
    }*/

    void reset_query_statistics() {
      stats_num_queries_ = 0;
      stats_.average_total_query_time = 0.0;
      stats_.average_lsh_time = 0.0;
      stats_.average_hash_table_time = 0.0;
      stats_.average_distance_time = 0.0;
      stats_.average_num_candidates = 0.0;
      stats_.average_num_unique_candidates = 0.0;
    }

    QueryStatistics get_query_statistics() {
      QueryStatistics res = stats_;
      if (stats_num_queries_ > 0) {
        res.average_total_query_time /= stats_num_queries_;
        res.average_lsh_time /= stats_num_queries_;
        res.average_hash_table_time /= stats_num_queries_;
        res.average_distance_time /= stats_num_queries_;
        res.average_num_candidates /= stats_num_queries_;
        res.average_num_unique_candidates /= stats_num_queries_;
      }
      return res;
    }

    // TODO: add void get_candidate_sequence(const PointType& p)
    // TODO: add void get_unique_candidate_sequence(const PointType& p)

   private:
    const StaticLSHTable& parent_;
    int_fast32_t query_counter_ = 0;
    std::vector<int32_t> is_candidate_;
    typename LSH::Query lsh_query_;
    std::vector<std::vector<HashType>> tmp_probes_by_table_;
    std::pair<typename HashTable::Iterator, typename HashTable::Iterator>
        hash_table_iterators_;

    QueryStatistics stats_;
    int_fast64_t stats_num_queries_ = 0;

    void get_unique_candidates_internal(const PointType& p,
                                        int_fast64_t num_probes,
                                        int_fast64_t max_num_candidates,
                                        std::vector<KeyType>* result) {
      auto start_time = std::chrono::high_resolution_clock::now();

      lsh_query_.get_probes_by_table(p, &tmp_probes_by_table_, num_probes);

      auto lsh_end_time = std::chrono::high_resolution_clock::now();
      auto elapsed_lsh =
          std::chrono::duration_cast<std::chrono::duration<double>>(
              lsh_end_time - start_time);
      stats_.average_lsh_time += elapsed_lsh.count();

      hash_table_iterators_ =
          parent_.hash_table_->retrieve_bulk(tmp_probes_by_table_);
      query_counter_ += 1;

      int_fast64_t num_candidates = 0;
      result->clear();
      if (max_num_candidates < 0) {
        max_num_candidates = std::numeric_limits<int_fast64_t>::max();
      }
      while (num_candidates < max_num_candidates &&
             hash_table_iterators_.first != hash_table_iterators_.second) {
        num_candidates += 1;
        int_fast64_t cur = *(hash_table_iterators_.first);
        if (is_candidate_[cur] != query_counter_) {
          is_candidate_[cur] = query_counter_;
          result->push_back(cur);
        }

        ++hash_table_iterators_.first;
      }

      auto hashing_end_time = std::chrono::high_resolution_clock::now();
      auto elapsed_hashing =
          std::chrono::duration_cast<std::chrono::duration<double>>(
              hashing_end_time - lsh_end_time);
      stats_.average_hash_table_time += elapsed_hashing.count();

      stats_.average_num_candidates += num_candidates;
      stats_.average_num_unique_candidates += result->size();
    }
  };

 private:
  int_fast64_t n_;

  void setup_table_range(int_fast32_t from, int_fast32_t to,
                         const DataStorageType& points) {
    typename LSH::template BatchHash<DataStorageType> bh(*(this->lsh_));
    std::vector<HashType> table_hashes;
    for (int_fast32_t ii = from; ii <= to; ++ii) {
      bh.batch_hash_single_table(points, ii, &table_hashes);
      this->hash_table_->add_entries_for_table(table_hashes, ii);
    }
  }
};

}  // namespace core
}  // namespace falconn

#endif
