#ifndef __NN_QUERY_H__
#define __NN_QUERY_H__

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../falconn_global.h"
#include "heap.h"

namespace falconn {
namespace core {

class NearestNeighborQueryError : public FalconnError {
 public:
  NearestNeighborQueryError(const char* msg) : FalconnError(msg) {}
};

template <typename LSHTableQuery, typename LSHTablePointType,
          typename LSHTableKeyType, typename ComparisonPointType,
          typename DistanceType, typename DistanceFunction,
          typename DataStorage>
class NearestNeighborQuery {
 public:
  NearestNeighborQuery(LSHTableQuery* table_query,
                       const DataStorage& data_storage)
      : table_query_(table_query), data_storage_(data_storage) {}

  LSHTableKeyType find_nearest_neighbor(const LSHTablePointType& q,
                                        const ComparisonPointType& q_comp,
                                        int_fast64_t num_probes,
                                        int_fast64_t max_num_candidates) {
    auto start_time = std::chrono::high_resolution_clock::now();
    stats_num_queries_ += 1;

    table_query_->get_unique_candidates(q, num_probes, max_num_candidates,
                                        &candidates_);
    auto distance_start_time = std::chrono::high_resolution_clock::now();

    // TODO: use nullptr for pointer types
    LSHTableKeyType best_key = -1;

    if (candidates_.size() > 0) {
      typename DataStorage::SubsequenceIterator iter =
          data_storage_.get_subsequence(candidates_);

      best_key = candidates_[0];
      DistanceType best_distance = dst_(q_comp, iter.get_point());
      ++iter;

      // printf("%d %f\n", candidates_[0], best_distance);

      while (iter.is_valid()) {
        DistanceType cur_distance = dst_(q_comp, iter.get_point());
        // printf("%d %f\n", iter.get_key(), cur_distance);
        if (cur_distance < best_distance) {
          best_distance = cur_distance;
          best_key = iter.get_key();
          // printf("  is new best\n");
        }
        ++iter;
      }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_distance =
        std::chrono::duration_cast<std::chrono::duration<double>>(
            end_time - distance_start_time);
    auto elapsed_total =
        std::chrono::duration_cast<std::chrono::duration<double>>(end_time -
                                                                  start_time);
    stats_.average_distance_time += elapsed_distance.count();
    stats_.average_total_query_time += elapsed_total.count();

    return best_key;
  }

  void find_k_nearest_neighbors(const LSHTablePointType& q,
                                const ComparisonPointType& q_comp,
                                int_fast64_t k, int_fast64_t num_probes,
                                int_fast64_t max_num_candidates,
                                std::vector<LSHTableKeyType>* result) {
    if (result == nullptr) {
      throw NearestNeighborQueryError("Results vector pointer is nullptr.");
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    stats_num_queries_ += 1;

    std::vector<LSHTableKeyType>& res = *result;
    res.clear();

    table_query_->get_unique_candidates(q, num_probes, max_num_candidates,
                                        &candidates_);

    heap_.reset();
    heap_.resize(k);

    auto distance_start_time = std::chrono::high_resolution_clock::now();

    typename DataStorage::SubsequenceIterator iter =
        data_storage_.get_subsequence(candidates_);

    int_fast64_t initially_inserted = 0;
    for (; initially_inserted < k; ++initially_inserted) {
      if (iter.is_valid()) {
        heap_.insert_unsorted(-dst_(q_comp, iter.get_point()), iter.get_key());
        ++iter;
      } else {
        break;
      }
    }

    if (initially_inserted >= k) {
      heap_.heapify();
      while (iter.is_valid()) {
        DistanceType cur_distance = dst_(q_comp, iter.get_point());
        if (cur_distance < -heap_.min_key()) {
          heap_.replace_top(-cur_distance, iter.get_key());
        }
        ++iter;
      }
    }

    res.resize(initially_inserted);
    std::sort(heap_.get_data().begin(),
              heap_.get_data().begin() + initially_inserted);
    for (int_fast64_t ii = 0; ii < initially_inserted; ++ii) {
      res[ii] = heap_.get_data()[initially_inserted - ii - 1].data;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_distance =
        std::chrono::duration_cast<std::chrono::duration<double>>(
            end_time - distance_start_time);
    auto elapsed_total =
        std::chrono::duration_cast<std::chrono::duration<double>>(end_time -
                                                                  start_time);
    stats_.average_distance_time += elapsed_distance.count();
    stats_.average_total_query_time += elapsed_total.count();
  }

  void find_near_neighbors(const LSHTablePointType& q,
                           const ComparisonPointType& q_comp,
                           DistanceType threshold, int_fast64_t num_probes,
                           int_fast64_t max_num_candidates,
                           std::vector<LSHTableKeyType>* result) {
    if (result == nullptr) {
      throw NearestNeighborQueryError("Results vector pointer is nullptr.");
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    stats_num_queries_ += 1;

    std::vector<LSHTableKeyType>& res = *result;
    res.clear();

    table_query_->get_unique_candidates(q, num_probes, max_num_candidates,
                                        &candidates_);
    auto distance_start_time = std::chrono::high_resolution_clock::now();

    typename DataStorage::SubsequenceIterator iter =
        data_storage_.get_subsequence(candidates_);
    while (iter.is_valid()) {
      DistanceType cur_distance = dst_(q_comp, iter.get_point());
      if (cur_distance < threshold) {
        res.push_back(iter.get_key());
      }
      ++iter;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed_distance =
        std::chrono::duration_cast<std::chrono::duration<double>>(
            end_time - distance_start_time);
    auto elapsed_total =
        std::chrono::duration_cast<std::chrono::duration<double>>(end_time -
                                                                  start_time);
    stats_.average_distance_time += elapsed_distance.count();
    stats_.average_total_query_time += elapsed_total.count();
  }

  void reset_query_statistics() {
    table_query_->reset_query_statistics();
    stats_num_queries_ = 0;
    stats_.average_total_query_time = 0.0;
    stats_.average_lsh_time = 0.0;
    stats_.average_hash_table_time = 0.0;
    stats_.average_distance_time = 0.0;
    stats_.average_num_candidates = 0.0;
    stats_.average_num_unique_candidates = 0.0;
  }

  QueryStatistics get_query_statistics() {
    QueryStatistics res = table_query_->get_query_statistics();
    res.average_total_query_time = stats_.average_total_query_time;
    res.average_distance_time = stats_.average_distance_time;

    if (stats_num_queries_ > 0) {
      res.average_total_query_time /= stats_num_queries_;
      res.average_distance_time /= stats_num_queries_;
    }
    return res;
  }

 private:
  LSHTableQuery* table_query_;
  const DataStorage& data_storage_;
  std::vector<LSHTableKeyType> candidates_;
  DistanceFunction dst_;
  SimpleHeap<DistanceType, LSHTableKeyType> heap_;

  QueryStatistics stats_;
  int_fast64_t stats_num_queries_ = 0;
};

}  // namespace core
}  // namespace falconn

#endif
