#ifndef __HYPERPLANE_HASH_H__
#define __HYPERPLANE_HASH_H__

#include <cstdint>
#include <ctime>
#include <iterator>
#include <memory>
#include <random>
#include <vector>

#include "../eigen_wrapper.h"

#include "data_storage.h"
#include "heap.h"
#include "lsh_function_helpers.h"

namespace falconn {
namespace core {

// Base class for both the dense and spare hyperplane hash classes.
// The derived classes only have to define how to multiply an input vector
// with the hyperplanes. Everything else (next step in the hash computation,
// probing, etc.) is done in the base class.
template <typename Derived, typename VectorT, typename CoordinateType = float,
          typename HashT = uint32_t>
class HyperplaneHashBase {
 private:
  class MultiProbeLookup;

 public:
  typedef VectorT VectorType;
  typedef HashT HashType;
  typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>
      TransformedVectorType;
  typedef void* TransformationTmpData;
  typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic, Eigen::Dynamic,
                        Eigen::ColMajor>
      MatrixType;

  typedef HashObjectQuery<Derived> Query;

  class HashTransformation {
   public:
    HashTransformation(const Derived& parent) : parent_(parent) {}

    void apply(const VectorT& v, TransformedVectorType* result) const {
      parent_.get_multiplied_vector_all_tables(v, result);
    }

   private:
    const Derived& parent_;
  };

  // TODO: specialize template for faster batch hyperplane setup (if the batch
  // vector type is also an Eigen matrix, we can just do a single matrix-matrix
  // multiplication.)
  template <typename BatchVectorType>
  class BatchHash {
   public:
    BatchHash(const Derived& parent)
        : parent_(parent), tmp_vector_(parent.get_k()) {}

    void batch_hash_single_table(const BatchVectorType& points, int_fast32_t l,
                                 std::vector<HashType>* res) {
      int_fast64_t nn = points.size();
      if (static_cast<int_fast64_t>(res->size()) != nn) {
        res->resize(nn);
      }

      typename BatchVectorType::FullSequenceIterator iter =
          points.get_full_sequence();
      for (int_fast64_t ii = 0; ii < nn; ++ii) {
        parent_.get_multiplied_vector_single_table(iter.get_point(), l,
                                                   &tmp_vector_);
        (*res)[ii] = compute_hash_single_table(tmp_vector_);
        ++iter;
      }
    }

   private:
    const Derived& parent_;
    TransformedVectorType tmp_vector_;
  };

  void reserve_transformed_vector_memory(TransformedVectorType* tv) const {
    tv->resize(k_ * l_);
  }

  int_fast32_t get_l() const { return l_; }

  int_fast32_t get_k() const { return k_; }

  const MatrixType& get_hyperplanes() const { return hyperplanes_; }

  static HashType compute_hash_single_table(const TransformedVectorType& v) {
    HashType res = 0;
    for (int_fast32_t jj = 0; jj < v.size(); ++jj) {
      res = res << 1;
      res = res | (v[jj] >= 0.0);
    }
    return res;
  }

  void hash(const VectorType& point, std::vector<HashType>* result,
            TransformedVectorType* tmp_hash_vector = nullptr) const {
    bool allocated = false;
    if (tmp_hash_vector == nullptr) {
      allocated = true;
      tmp_hash_vector = new TransformedVectorType(k_ * l_);
    }

    static_cast<const Derived*>(this)->get_multiplied_vector_all_tables(
        point, tmp_hash_vector);

    std::vector<HashType>& res = *result;
    if (res.size() != static_cast<size_t>(l_)) {
      res.resize(l_);
    }
    for (int_fast32_t ii = 0; ii < l_; ++ii) {
      res[ii] = 0;
      for (int_fast32_t jj = 0; jj < k_; ++jj) {
        res[ii] = res[ii] << 1;
        res[ii] = res[ii] | ((*tmp_hash_vector)[ii * k_ + jj] >= 0.0);
      }
    }

    if (allocated) {
      delete tmp_hash_vector;
    }
  }

 protected:
  HyperplaneHashBase(int dim, int_fast32_t k, int_fast32_t l,
                     uint_fast64_t seed)
      : dim_(dim), k_(k), l_(l), seed_(seed) {
    if (dim_ < 1) {
      throw LSHFunctionError("Dimension must be at least 1.");
    }

    if (k_ < 1) {
      throw LSHFunctionError(
          "Number of hash functions must be"
          "at least 1.");
    }

    if (k_ > 8 * static_cast<int_fast32_t>(sizeof(HashType))) {
      throw LSHFunctionError(
          "More hash functions than supported by the "
          "hash type.");
    }

    if (l_ < 1) {
      throw LSHFunctionError("Number of hash tables must be at least 1.");
    }

    // use the STL Mersenne Twister for random numbers
    std::mt19937_64 gen(seed_);
    std::normal_distribution<CoordinateType> gauss(0.0, 1.0);

    hyperplanes_.resize(k_ * l_, dim_);

    std::vector<CoordinateType> row_norms(k_ * l_, 0.0);

    for (int ii = 0; ii < dim_; ++ii) {
      for (int jj = 0; jj < k_ * l_; ++jj) {
        hyperplanes_(jj, ii) = gauss(gen);
        row_norms[jj] += hyperplanes_(jj, ii) * hyperplanes_(jj, ii);
      }
    }

    for (int plane = 0; plane < k_ * l_; ++plane) {
      row_norms[plane] = std::sqrt(row_norms[plane]);
    }

    // Normalize the hyperplanes
    for (int plane = 0; plane < k_ * l_; ++plane) {
      for (int ii = 0; ii < dim_; ++ii) {
        hyperplanes_(plane, ii) /= row_norms[plane];
      }
    }
  }

  int dim_;
  int_fast32_t k_;
  int_fast32_t l_;
  uint_fast64_t seed_;
  Eigen::Matrix<CoordinateType, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>
      hyperplanes_;

 private:
  friend Query;
  // Helper class for multiprobe LSH
  class MultiProbeLookup {
   public:
    MultiProbeLookup(const Derived& parent)
        : k_(parent.k_),
          l_(parent.l_),
          num_probes_(0),
          cur_probe_counter_(0),
          sorted_hyperplane_indices_(parent.l_),
          main_table_probe_(parent.l_) {
      for (int_fast32_t ii = 0; ii < l_; ++ii) {
        sorted_hyperplane_indices_[ii].resize(k_);
        for (int_fast32_t jj = 0; jj < k_; ++jj) {
          sorted_hyperplane_indices_[ii][jj] = jj;
        }
      }
    }

    void setup_probing(const TransformedVectorType& hash_vector,
                       int_fast64_t num_probes) {
      hash_vector_ = &hash_vector;
      num_probes_ = num_probes;
      cur_probe_counter_ = -1;

      for (int_fast32_t ii = 0; ii < l_; ++ii) {
        main_table_probe_[ii] = 0;
        for (int_fast32_t jj = 0; jj < k_; ++jj) {
          main_table_probe_[ii] = main_table_probe_[ii] << 1;
          main_table_probe_[ii] =
              main_table_probe_[ii] | (hash_vector[ii * k_ + jj] >= 0.0);
        }
      }

      if (num_probes_ >= 0 && num_probes_ <= l_) {
        return;
      }

      for (int_fast32_t ii = 0; ii < l_; ++ii) {
        HyperplaneComparator comp(hash_vector, ii * k_);
        std::sort(sorted_hyperplane_indices_[ii].begin(),
                  sorted_hyperplane_indices_[ii].end(), comp);
      }

      if (num_probes_ >= 0) {
        heap_.resize(2 * num_probes_);
      }
      heap_.reset();
      for (int_fast32_t ii = 0; ii < l_; ++ii) {
        int_fast32_t best_index = sorted_hyperplane_indices_[ii][0];
        CoordinateType score = hash_vector[ii * k_ + best_index];
        score = score * score;
        HashType hash_mask = 1;
        hash_mask = hash_mask << (k_ - best_index - 1);
        heap_.insert_unsorted(score, ProbeCandidate(ii, hash_mask, 0));
      }
      heap_.heapify();
    }

    bool get_next_probe(HashType* cur_probe, int_fast32_t* cur_table) {
      cur_probe_counter_ += 1;

      if (num_probes_ >= 0 && cur_probe_counter_ >= num_probes_) {
        // printf("out of probes\n");
        return false;
      }

      if (cur_probe_counter_ < l_) {
        // printf("initial probes %lld\n", cur_probe_counter_);
        *cur_probe = main_table_probe_[cur_probe_counter_];
        *cur_table = cur_probe_counter_;
        return true;
      }

      if (heap_.empty()) {
        return false;
      }

      const TransformedVectorType& hash_vector = *hash_vector_;
      CoordinateType cur_score;
      ProbeCandidate cur_candidate;
      heap_.extract_min(&cur_score, &cur_candidate);
      *cur_table = cur_candidate.table_;
      int_fast32_t cur_index =
          sorted_hyperplane_indices_[*cur_table][cur_candidate.last_index_];
      *cur_probe = main_table_probe_[*cur_table] ^ cur_candidate.hash_mask_;

      if (cur_candidate.last_index_ != k_ - 1) {
        // swapping out the last flipped index
        int_fast32_t next_index =
            sorted_hyperplane_indices_[*cur_table]
                                      [cur_candidate.last_index_ + 1];
        // printf("cur_mask: %d  cur_index: %d\n", cur_candidate.hash_mask_,
        //       cur_index);
        HashType next_mask = cur_candidate.hash_mask_ ^
                             (HashType(1) << (k_ - cur_index - 1))  // xor out
                             ^
                             (HashType(1) << (k_ - next_index - 1));  // xor in
        CoordinateType next_score =
            cur_score - (hash_vector[*cur_table * k_ + cur_index] *
                         hash_vector[*cur_table * k_ + cur_index]);
        next_score += (hash_vector[*cur_table * k_ + next_index] *
                       hash_vector[*cur_table * k_ + next_index]);
        // printf("next_mask 1: %d\n", next_mask);
        // printf("cur_candidate.last_index_: %d\n", cur_candidate.last_index_);
        heap_.insert(next_score, ProbeCandidate(*cur_table, next_mask,
                                                cur_candidate.last_index_ + 1));

        // adding a new flipped index
        next_mask =
            cur_candidate.hash_mask_ ^ (HashType(1) << (k_ - next_index - 1));
        next_score = cur_score + (hash_vector[*cur_table * k_ + next_index] *
                                  hash_vector[*cur_table * k_ + next_index]);
        // printf("next_mask 2: %d\n", next_mask);
        heap_.insert(next_score, ProbeCandidate(*cur_table, next_mask,
                                                cur_candidate.last_index_ + 1));
      }

      return true;
    }

   private:
    class ProbeCandidate {
     public:
      ProbeCandidate(int_fast32_t table = 0, HashType hash_mask = 0,
                     int_fast32_t last_index = 0)
          : table_(table), hash_mask_(hash_mask), last_index_(last_index) {}

      int_fast32_t table_;
      HashType hash_mask_;
      int_fast32_t last_index_;
    };

    class HyperplaneComparator {
     public:
      HyperplaneComparator(const TransformedVectorType& values,
                           int_fast32_t offset)
          : values_(values), offset_(offset) {}

      bool operator()(int_fast32_t ii, int_fast32_t jj) const {
        return std::abs(values_[offset_ + ii]) <
               std::abs(values_[offset_ + jj]);
      }

     private:
      const TransformedVectorType& values_;
      int_fast32_t offset_;
    };

    int_fast32_t k_;
    int_fast32_t l_;
    int_fast64_t num_probes_;
    int_fast64_t cur_probe_counter_;
    std::vector<std::vector<int_fast32_t>> sorted_hyperplane_indices_;
    std::vector<HashType> main_table_probe_;
    SimpleHeap<CoordinateType, ProbeCandidate> heap_;
    const TransformedVectorType* hash_vector_;
  };
};

// Hash function implementation for the dense hyperplane hash.
// The only functionality that has to be implemented here is the mapping from
// an input point / vector to the result of multiplying with the hyperplanes
// (i.e., generating the "tansformed vector").
template <typename CoordinateType = float, typename HashType = uint32_t>
class HyperplaneHashDense
    : public HyperplaneHashBase<
          HyperplaneHashDense<CoordinateType, HashType>,
          Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>,
          CoordinateType, HashType> {
 public:
  typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>
      DerivedVectorT;

  HyperplaneHashDense(int dim, int_fast32_t k, int_fast32_t l,
                      uint_fast64_t seed)
      : HyperplaneHashBase<HyperplaneHashDense<CoordinateType, HashType>,
                           DerivedVectorT, CoordinateType, HashType>(dim, k, l,
                                                                     seed) {}

  void get_multiplied_vector_all_tables(const DerivedVectorT& point,
                                        DerivedVectorT* res) const {
    *res = this->hyperplanes_ * point;
  }

  void get_multiplied_vector_single_table(const DerivedVectorT& point,
                                          int_fast32_t l,
                                          DerivedVectorT* res) const {
    // TODO: check whether middleRows is as fast as building a memory map
    // manually.
    *res = this->hyperplanes_.middleRows(l * this->k_, this->k_) * point;
  }
};

// Hash function implementation for the sparse hyperplane hash.
// The only functionality that has to be implemented here is the mapping from
// an input point / vector to the result of multiplying with the hyperplanes
// (i.e., generating the "tansformed vector").
template <typename CoordinateType = float, typename HashType = uint32_t,
          typename IndexType = int32_t>
class HyperplaneHashSparse
    : public HyperplaneHashBase<
          HyperplaneHashSparse<CoordinateType, HashType, IndexType>,
          std::vector<std::pair<IndexType, CoordinateType>>, CoordinateType,
          HashType> {
 public:
  typedef std::vector<std::pair<IndexType, CoordinateType>> DerivedVectorT;
  typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>
      DerivedTransformedVectorT;

  HyperplaneHashSparse(IndexType dim, int_fast32_t k, int_fast32_t l,
                       uint_fast64_t seed)
      : HyperplaneHashBase<
            HyperplaneHashSparse<CoordinateType, HashType, IndexType>,
            DerivedVectorT, CoordinateType, HashType>(dim, k, l, seed) {}

  void get_multiplied_vector_all_tables(const DerivedVectorT& point,
                                        DerivedTransformedVectorT* res) const {
    // TODO: would row-major be a better storage order for sparse vectors?
    res->setZero();
    for (IndexType ii = 0; ii < static_cast<IndexType>(point.size()); ++ii) {
      *res += point[ii].second * this->hyperplanes_.col(point[ii].first);
    }
  }

  void get_multiplied_vector_single_table(
      const DerivedVectorT& point, int_fast32_t l,
      DerivedTransformedVectorT* res) const {
    res->setZero();
    for (IndexType ii = 0; ii < static_cast<IndexType>(point.size()); ++ii) {
      *res += point[ii].second *
              this->hyperplanes_.col(point[ii].first)
                  .segment(l * this->k_, this->k_);
    }
  }
};

}  // namespace core
}  // namespace falconn

#endif
