#ifndef __POLYTOPE_HASH_H__
#define __POLYTOPE_HASH_H__

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <random>
#include <vector>

#include "../eigen_wrapper.h"

#include "../ffht/fht_header_only.h"
#include "data_storage.h"
#include "heap.h"
#include "incremental_sorter.h"
#include "lsh_function_helpers.h"
#include "math_helpers.h"

namespace falconn {
namespace core {

// TODO: move this namespace to a separate file
namespace cp_hash_helpers {

static inline void compute_k_parameters_for_bits(
    int_fast32_t rotation_dim, int_fast32_t number_of_hash_bits,
    int_fast32_t* k, int_fast32_t* last_cp_dim) {
  int_fast32_t bits_per_cp = log2ceil(rotation_dim) + 1;
  *k = number_of_hash_bits / bits_per_cp;
  if (*k * bits_per_cp < number_of_hash_bits) {
    int_fast32_t remaining_bits = number_of_hash_bits - *k * bits_per_cp;
    *k += 1;
    *last_cp_dim = 1 << (remaining_bits - 1);
  } else {
    *last_cp_dim = rotation_dim;
  }
}

static inline int_fast32_t compute_number_of_hash_bits(
    int_fast32_t rotation_dim, int_fast32_t last_cp_dim, int_fast32_t k) {
  int_fast32_t log_rotation_dim = log2ceil(rotation_dim);
  int_fast32_t last_cp_log_dim = log2ceil(last_cp_dim);
  return (k - 1) * (log_rotation_dim + 1) + last_cp_log_dim + 1;
}

template <typename ScalarType>
struct FHTFunction {
  static void apply(ScalarType*, int_fast32_t) {
    static_assert(FalseStruct<ScalarType>::value, "ScalarType not supported.");
  };
  template <typename T>
  struct FalseStruct : std::false_type {};
};

template <>
struct FHTFunction<float> {
  static void apply(float* data, int_fast32_t dim) {
    if (FHTFloat(data, dim, std::max(dim, static_cast<int_fast32_t>(8))) != 0) {
      throw LSHFunctionError("FHTFloat returned nonzero value.");
    }
  }
};

template <>
struct FHTFunction<double> {
  static void apply(double* data, int_fast32_t dim) {
    if (FHTDouble(data, dim, std::max(dim, static_cast<int_fast32_t>(8))) !=
        0) {
      throw LSHFunctionError("FHTDouble returned nonzero value.");
    }
  }
};

template <typename ScalarType>
class FHTHelper {
 public:
  FHTHelper(int_fast32_t dim) : dim_(dim) {}

  int_fast32_t get_dim() { return dim_; }

  void apply(ScalarType* data) {
#ifdef __AVX__
    if (reinterpret_cast<int_fast64_t>(data) % 32 == 0) {
      FHTFunction<ScalarType>::apply(data, dim_);
    } else {
      if (aligned_data_ == nullptr) {
        if (posix_memalign(
                (reinterpret_cast<void**>(&aligned_data_)), 32,
                std::max(static_cast<int_fast32_t>(dim_ * sizeof(ScalarType)),
                         static_cast<int_fast32_t>(32))) != 0) {
          throw LSHFunctionError("Error when allocating temporary FHT array.");
        }
      }
      std::memcpy(aligned_data_, data, dim_ * sizeof(ScalarType));
      FHTFunction<ScalarType>::apply(aligned_data_, dim_);
      std::memcpy(data, aligned_data_, dim_ * sizeof(ScalarType));
    }
#else
    FHTFunction<ScalarType>::apply(data, dim_);
#endif
  }

#ifdef __AVX__
  ~FHTHelper() {
    if (aligned_data_ != nullptr) {
      free(aligned_data_);
    }
  }
#endif

 private:
  int_fast64_t dim_;
#ifdef __AVX__
  ScalarType* aligned_data_ = nullptr;
#endif
};

}  // namespace cp_hash_helpers

// TODO: replace CoordinateType with a type trait of VectorT?
template <typename Derived, typename VectorT, typename CoordinateType = float,
          typename HashT = uint32_t>
class CrossPolytopeHashBase {
 private:
  class MultiProbeLookup;

 public:
  typedef VectorT VectorType;
  typedef HashT HashType;
  typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>
      RotatedVectorType;
  typedef std::vector<RotatedVectorType> TransformedVectorType;

  typedef HashObjectQuery<Derived> Query;

  class HashTransformation {
   public:
    HashTransformation(const Derived& parent)
        : parent_(parent), fht_helper_(parent.rotation_dim_) {}

    void apply(const VectorT& v, TransformedVectorType* result) {
      parent_.compute_rotated_vectors(v, result, &fht_helper_);
    }

   private:
    const Derived& parent_;
    cp_hash_helpers::FHTHelper<CoordinateType> fht_helper_;
  };

  // TODO: Can the FHT rotations be faster by doing 8 rotations (AVX float
  // register size) in parallel? If so, the table setup time could be made
  // faster by using a batch FHT below.
  template <typename BatchVectorType, typename PointType>
  class BatchHash {
   public:
    BatchHash(const Derived& parent, const typename PointTypeConverter<PointType>::DensePointType* pCenter)
        : parent_(parent),
          tmp_vector_(parent.rotation_dim_),
          fht_helper_(parent.rotation_dim_),
          center_(pCenter) {}

    void batch_hash_single_table(const BatchVectorType& points, int_fast32_t l,
                                 std::vector<HashType>* res) {
      int_fast64_t nn = points.size();
      if (static_cast<int_fast64_t>(res->size()) != nn) {
        res->resize(nn);
      }

      typename PointTypeConverter<PointType>::DensePointType interm_dense;
      PointType                                              interm_orig;

      typename BatchVectorType::FullSequenceIterator iter =
          points.get_full_sequence();
      for (int_fast64_t ii = 0; ii < nn; ++ii) {
        (*res)[ii] = 0;
        int_fast32_t pattern = l * parent_.k_ * parent_.num_rotations_;

        const auto& point = center_ == nullptr ? iter.get_point() : interm_orig;
        if (center_ != nullptr) {
          toDenseVector(iter.get_point(), interm_dense, center_->rows());
          interm_dense -= *center_;
          fromDenseVector(interm_dense, interm_orig);
        }

        for (int_fast32_t jj = 0; jj < parent_.k_ - 1; ++jj) {
          parent_.embed(point, l, jj, &tmp_vector_);

          for (int_fast32_t rot = 0; rot < parent_.num_rotations_; ++rot) {
            tmp_vector_ =
                tmp_vector_.cwiseProduct(parent_.random_signs_[pattern]);
            ++pattern;
            fht_helper_.apply(tmp_vector_.data());
          }

          (*res)[ii] = (*res)[ii] << (parent_.log_rotation_dim_ + 1);
          (*res)[ii] =
              (*res)[ii] | decodeCP(tmp_vector_, parent_.rotation_dim_);
        }

        parent_.embed(point, l, parent_.k_ - 1, &tmp_vector_);
        for (int_fast32_t rot = 0; rot < parent_.num_rotations_; ++rot) {
          tmp_vector_ =
              tmp_vector_.cwiseProduct(parent_.random_signs_[pattern]);
          ++pattern;
          fht_helper_.apply(tmp_vector_.data());
        }

        (*res)[ii] = (*res)[ii] << (parent_.last_cp_log_dim_ + 1);
        (*res)[ii] = (*res)[ii] | decodeCP(tmp_vector_, parent_.last_cp_dim_);

        ++iter;
      }
    }

   private:
    const Derived& parent_;
    RotatedVectorType tmp_vector_;
    cp_hash_helpers::FHTHelper<CoordinateType> fht_helper_;
    const typename PointTypeConverter<PointType>::DensePointType* center_;
  };

  int_fast32_t get_l() const { return l_; }

  void reserve_transformed_vector_memory(TransformedVectorType* tv) const {
    tv->resize(k_ * l_);
    for (int_fast32_t ii = 0; ii < k_ * l_; ++ii) {
      (*tv)[ii].resize(rotation_dim_);
    }
  }

  void hash(
      const VectorType& point, std::vector<HashType>* result,
      TransformedVectorType* tmp_transformed_vector = nullptr,
      cp_hash_helpers::FHTHelper<CoordinateType>* fht_helper = nullptr) const {
    bool allocated_tmpvec = false;
    if (tmp_transformed_vector == nullptr) {
      allocated_tmpvec = true;
      tmp_transformed_vector = new TransformedVectorType();
      reserve_transformed_vector_memory(tmp_transformed_vector);
    }
    bool allocated_fht_helper = false;
    if (fht_helper == nullptr) {
      allocated_fht_helper = true;
      fht_helper =
          new cp_hash_helpers::FHTHelper<CoordinateType>(rotation_dim_);
    }

    if (fht_helper->get_dim() != rotation_dim_) {
      throw LSHFunctionError("FHT helper has incorrect dimension.");
    }

    // TODO: might be able to interleave this better (for each cross polytope,
    // rotate and also decode before going to the next cross polytope)
    compute_rotated_vectors(point, tmp_transformed_vector, fht_helper);
    compute_cp_hashes(*tmp_transformed_vector, k_, l_, rotation_dim_,
                      log_rotation_dim_, last_cp_dim_, last_cp_log_dim_,
                      result);

    if (allocated_tmpvec) {
      delete tmp_transformed_vector;
    }
    if (allocated_fht_helper) {
      delete fht_helper;
    }
  }

  static HashType decodeCP(const RotatedVectorType& data, int_fast64_t dim) {
    HashType res = 0;
    CoordinateType best = data[0];
    if (-data[0] > best) {
      best = -data[0];
      res = dim;
    }
    for (int_fast64_t ii = 1; ii < dim; ++ii) {
      if (data[ii] > best) {
        best = data[ii];
        res = ii;
      } else if (-data[ii] > best) {
        best = -data[ii];
        res = ii + dim;
      }
    }
    return res;
  }

 protected:
  CrossPolytopeHashBase(int_fast32_t rotation_dim, int_fast32_t k,
                        int_fast32_t l, int_fast32_t num_rotations,
                        int_fast32_t last_cp_dim, uint_fast64_t seed)
      : rotation_dim_(rotation_dim),
        log_rotation_dim_(log2ceil(rotation_dim)),
        k_(k),
        l_(l),
        num_rotations_(num_rotations),
        last_cp_dim_(last_cp_dim),
        last_cp_log_dim_(log2ceil(last_cp_dim)),
        seed_(seed) {
    if (rotation_dim_ < 1) {
      throw LSHFunctionError("Rotation dimension must be at least 1.");
    }

    if (last_cp_dim_ < 1) {
      throw LSHFunctionError("Dimension of last CP must be at least 1.");
    }

    if (last_cp_dim_ > rotation_dim_) {
      throw LSHFunctionError(
          "Dimension of last CP must be at most the "
          "rotation dimension.");
    }

    if (k_ < 1) {
      throw LSHFunctionError(
          "Number of hash functions must be"
          "at least 1.");
    }

    if (l_ < 1) {
      throw LSHFunctionError("Number of hash tables must be at least 1.");
    }

    if (num_rotations_ < 0) {
      throw LSHFunctionError("Number of rotations must be at least 0.");
    }

    // http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
    bool rotation_dim_power_of_two = !(rotation_dim & (rotation_dim - 1));
    if (!rotation_dim_power_of_two) {
      throw LSHFunctionError("Rotation dimension must be a power of two.");
    }
    /*bool last_CP_dim_power_of_two = !(last_cp_dim & (last_cp_dim - 1));
    if (!last_CP_dim_power_of_two) {
      throw LSHFunctionError("Last CP dimension must be a power of two.");
    }*/

    if ((k_ - 1) * (log_rotation_dim_ + 1) + last_cp_log_dim_ + 1 >
        8 * static_cast<int_fast32_t>(sizeof(HashType))) {
      throw LSHFunctionError(
          "More hash functions than supported by the "
          "hash type.");
    }

    // use the STL Mersenne Twister for random numbers
    std::mt19937_64 gen(seed_);
    std::uniform_int_distribution<int_fast32_t> bernoulli(0, 1);

    for (int_fast32_t ii = 0; ii < k_ * l_ * num_rotations_; ++ii) {
      RotatedVectorType tmp_vec(rotation_dim_);
      for (int_fast32_t jj = 0; jj < rotation_dim_; ++jj) {
        tmp_vec(jj) = 2 * bernoulli(gen) - 1;
      }
      random_signs_.push_back(tmp_vec);
    }
  }

  static void compute_cp_hashes(const TransformedVectorType& rotated_vectors,
                                int_fast32_t k, int_fast32_t l,
                                int_fast32_t dim, int_fast32_t log_dim,
                                int_fast32_t last_dim,
                                int_fast32_t last_log_dim,
                                std::vector<HashType>* hashes) {
    std::vector<HashType>& res = *hashes;
    if (res.size() != static_cast<size_t>(l)) {
      res.resize(l);
    }

    for (int_fast32_t ii = 0; ii < l; ++ii) {
      res[ii] = 0;

      for (int_fast32_t jj = 0; jj < k - 1; ++jj) {
        res[ii] = res[ii] << (log_dim + 1);
        res[ii] = res[ii] | decodeCP(rotated_vectors[ii * k + jj], dim);
      }

      res[ii] = res[ii] << (last_log_dim + 1);
      res[ii] = res[ii] | decodeCP(rotated_vectors[ii * k + k - 1], last_dim);
    }
  }

  const int_fast32_t rotation_dim_;  // dimension of the vectors to be rotated
  const int_fast32_t log_rotation_dim_;  // binary log of rotation_dim
  const int_fast32_t k_;
  const int_fast32_t l_;
  const int_fast32_t num_rotations_;
  const int_fast32_t last_cp_dim_;
  const int_fast32_t last_cp_log_dim_;
  const uint_fast64_t seed_;
  std::vector<RotatedVectorType> random_signs_;

 private:
  friend Query;

  void compute_rotated_vectors(
      const VectorT& v, TransformedVectorType* result,
      cp_hash_helpers::FHTHelper<CoordinateType>* fht) const {
    int_fast32_t pattern = 0;
    for (int_fast32_t ii = 0; ii < l_; ++ii) {
      for (int_fast32_t jj = 0; jj < k_; ++jj) {
        RotatedVectorType& cur_vec = (*result)[ii * k_ + jj];
        static_cast<const Derived*>(this)->embed(v, ii, jj, &cur_vec);

        for (int_fast32_t rot = 0; rot < num_rotations_; ++rot) {
          cur_vec = cur_vec.cwiseProduct(random_signs_[pattern]);
          ++pattern;
          fht->apply(cur_vec.data());
        }
      }
    }
  }

  // friend BatchHash;
  // Helper class for multiprobe LSH
  class MultiProbeLookup {
   public:
    typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>
        VectorType;

    MultiProbeLookup(const Derived& parent)
        : k_(parent.k_),
          l_(parent.l_),
          dim_(parent.rotation_dim_),
          log_dim_(parent.log_rotation_dim_),
          last_cp_dim_(parent.last_cp_dim_),
          last_cp_log_dim_(parent.last_cp_log_dim_),
          num_probes_(0),
          cur_probe_counter_(0),
          sorted_coordinate_indices_(parent.k_ * parent.l_),
          inc_sorted_coordinate_indices_(parent.k_ * parent.l_),
          main_table_probes_(parent.l_) {
      for (int_fast32_t ii = 0; ii < l_; ++ii) {
        for (int_fast32_t jj = 0; jj < k_; ++jj) {
          int_fast32_t cur_cp_dim = (jj == k_ - 1 ? last_cp_dim_ : dim_);
          sorted_coordinate_indices_[ii * k_ + jj].resize(2 * cur_cp_dim);
        }
      }
    }

    void setup_probing(const TransformedVectorType& transformed_vector,
                       int_fast64_t num_probes) {
      transformed_vector_ = &transformed_vector;
      num_probes_ = num_probes;
      cur_probe_counter_ = -1;

      if (num_probes_ >= 0 && num_probes_ <= l_) {
        // If we don't want extra probes, the top probes are going to be the
        // standard hashes for each table.
        compute_cp_hashes(transformed_vector, k_, l_, dim_, log_dim_,
                          last_cp_dim_, last_cp_log_dim_, &main_table_probes_);
        return;
      }

      int sorting_block_size = 0;
      if (num_probes_ >= 0) {
        double b = 1.0;
        double sqrt2 = std::sqrt(2.0);
        while (std::pow(b, k_ - 1) *
                   std::max(1.0, b * last_cp_dim_ / static_cast<double>(dim_)) <
               static_cast<double>(num_probes_) / l_) {
          b *= sqrt2;
        }
        sorting_block_size =
            std::min(1, static_cast<int>(std::round(b * sqrt2)));
      } else {
        sorting_block_size = 8;
      }
      // printf("sorting block size: %d\n", sorting_block_size_);

      // For each CP, we now sort the potential hash values (2 * dim) by their
      // distance to the largest absolute value in the respective CP
      for (int_fast32_t ii = 0; ii < l_; ++ii) {
        for (int_fast32_t jj = 0; jj < k_; ++jj) {
          int_fast32_t cur_cp_dim = (jj == k_ - 1 ? last_cp_dim_ : dim_);
          const VectorType& cur_vec = transformed_vector[ii * k_ + jj];
          std::vector<std::pair<CoordinateType, int_fast32_t>>& cur_indices =
              sorted_coordinate_indices_[ii * k_ + jj];

          // TODO: use eigen for abs and max here
          CoordinateType max_abs_coord = std::abs(cur_vec[0]);
          for (int_fast32_t mm = 1; mm < cur_cp_dim; ++mm) {
            max_abs_coord = std::max(max_abs_coord, std::abs(cur_vec[mm]));
          }

          for (int_fast32_t mm = 0; mm < cur_cp_dim; ++mm) {
            cur_indices[mm] = std::make_pair(max_abs_coord - cur_vec[mm], mm);
            cur_indices[mm + cur_cp_dim] =
                std::make_pair(max_abs_coord + cur_vec[mm], mm + cur_cp_dim);
          }

          inc_sorted_coordinate_indices_[ii * k_ + jj].reset(
              &(sorted_coordinate_indices_[ii * k_ + jj]), sorting_block_size);
        }
      }

      // We use a heap to construct the probing sequence.
      if (num_probes_ >= 0) {
        heap_.resize(2 * k_ * l_ * num_probes_ + l_);
      }
      heap_.reset();
      for (int_fast32_t ii = 0; ii < l_; ++ii) {
        heap_.insert_unsorted(0.0, ProbeCandidate(ii, 0, 0, 0));
      }
      heap_.heapify();
    }

    bool get_next_probe(HashType* result_probe, int_fast32_t* result_table) {
      cur_probe_counter_ += 1;

      if (num_probes_ >= 0 && cur_probe_counter_ >= num_probes_) {
        return false;
      }

      if (num_probes_ <= l_) {
        *result_table = cur_probe_counter_;
        *result_probe = main_table_probes_[*result_table];
        return true;
      }

      if (heap_.empty()) {
        return false;
      }

      while (true) {
        CoordinateType cur_score;
        ProbeCandidate cur_candidate;
        heap_.extract_min(&cur_score, &cur_candidate);
        int_fast32_t cur_table = cur_candidate.table_;
        int_fast32_t cur_cp = cur_candidate.cur_cp_;
        int_fast32_t cur_sorted_coord_index =
            cur_candidate.cur_sorted_coord_index_;

        /*printf("Current probe: score = %f  hash = %d  table = %d"
            " cp = %d  sorted_index = %d\n", cur_score,
            cur_candidate.prev_cps_hash_, cur_table, cur_cp,
            cur_sorted_coord_index);*/

        if (cur_cp == k_) {
          // have complete probe candidate
          // printf("  Current probe is complete probe.\n");
          *result_probe = cur_candidate.prev_cps_hash_;
          *result_table = cur_table;
          return true;
        } else {
          // If the current probe candidate is not complete, there are two ways
          // to continue the current probe candidate:
          // - go to the next worse index / hash value in the current CP
          // - go to the next CP (starting again at sorted index 0 in the
          //   next CP)

          int_fast32_t cur_cp_dim = dim_;
          int_fast32_t cur_cp_log_dim = log_dim_;
          if (cur_cp == k_ - 1) {
            cur_cp_dim = last_cp_dim_;
            cur_cp_log_dim = last_cp_log_dim_;
          }

          CoordinateType cur_coord_score =
              inc_sorted_coordinate_indices_[cur_table * k_ + cur_cp]
                  .get(cur_sorted_coord_index)
                  .first;
          cur_coord_score = cur_coord_score * cur_coord_score;

          // first case: same CP, but next higher index
          if (cur_sorted_coord_index < 2 * cur_cp_dim - 1) {
            CoordinateType next_score = cur_score - cur_coord_score;
            CoordinateType next_coord_score =
                inc_sorted_coordinate_indices_[cur_table * k_ + cur_cp]
                    .get(cur_sorted_coord_index + 1)
                    .first;
            next_score += next_coord_score * next_coord_score;

            heap_.insert(next_score,
                         ProbeCandidate(cur_table, cur_candidate.prev_cps_hash_,
                                        cur_cp, cur_sorted_coord_index + 1));
          }

          // second case: next CP
          HashType cur_index =
              inc_sorted_coordinate_indices_[cur_table * k_ + cur_cp]
                  .get(cur_sorted_coord_index)
                  .second;
          HashType next_hash = cur_candidate.prev_cps_hash_
                               << (cur_cp_log_dim + 1);
          next_hash = next_hash | cur_index;
          heap_.insert_guaranteed_top(
              cur_score, ProbeCandidate(cur_table, next_hash, cur_cp + 1, 0));
        }
      }
    }

   private:
    class ProbeCandidate {
     public:
      ProbeCandidate(int_fast32_t table = 0, HashType prev_cps_hash = 0,
                     int_fast32_t cur_cp = 0,
                     int_fast32_t cur_sorted_coord_index = 0)
          : table_(table),
            prev_cps_hash_(prev_cps_hash),
            cur_cp_(cur_cp),
            cur_sorted_coord_index_(cur_sorted_coord_index) {}

      int_fast32_t table_;
      HashType prev_cps_hash_;
      int_fast32_t cur_cp_;
      int_fast32_t cur_sorted_coord_index_;
    };

    const int_fast32_t k_;
    const int_fast32_t l_;
    const int_fast32_t dim_;
    const int_fast32_t log_dim_;
    const int_fast32_t last_cp_dim_;
    const int_fast32_t last_cp_log_dim_;
    int_fast64_t num_probes_;
    int_fast64_t cur_probe_counter_;
    std::vector<std::vector<std::pair<CoordinateType, int_fast32_t>>>
        sorted_coordinate_indices_;
    std::vector<IncrementalSorter<std::pair<CoordinateType, int_fast32_t>>>
        inc_sorted_coordinate_indices_;
    std::vector<HashType> main_table_probes_;
    AugmentedHeap<CoordinateType, ProbeCandidate> heap_;
    const TransformedVectorType* transformed_vector_;
  };
};

template <typename CoordinateType = float, typename HashType = uint32_t,
          typename IndexType = int32_t>
class CrossPolytopeHashSparse
    : public CrossPolytopeHashBase<
          CrossPolytopeHashSparse<CoordinateType, HashType>,
          std::vector<std::pair<IndexType, CoordinateType>>, CoordinateType,
          HashType> {
 public:
  typedef std::vector<std::pair<IndexType, CoordinateType>> DerivedVectorT;
  typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>
      HashedVectorT;

  CrossPolytopeHashSparse(int_fast32_t vector_dim, int_fast32_t k,
                          int_fast32_t l, int_fast32_t num_rotations,
                          int_fast32_t feature_hashing_dim,
                          int_fast32_t last_cp_dim, uint_fast64_t seed)
      : CrossPolytopeHashBase<CrossPolytopeHashSparse<CoordinateType, HashType>,
                              std::vector<std::pair<IndexType, CoordinateType>>,
                              CoordinateType, HashType>(
            feature_hashing_dim, k, l, num_rotations, last_cp_dim, seed),
        vector_dim_(vector_dim) {
    if (vector_dim_ < 1) {
      throw LSHFunctionError("Vector dimension must be at least 1.");
    }

    // use the STL Mersenne Twister for random numbers
    // xor'ing in a random number to avoid seeding with the same seed as in
    // the base class.
    std::mt19937_64 gen(this->seed_ ^ 846980723);
    std::uniform_int_distribution<int_fast32_t> bernoulli(0, 1);
    std::uniform_int_distribution<int_fast32_t> feature_hashing_distribution(
        0, this->rotation_dim_ - 1);

    // feature hashing randomness
    int_fast64_t num_feature_hashing_indices =
        (static_cast<int_fast64_t>(this->k_) * this->l_) * vector_dim_;
    feature_hashing_index_.resize(num_feature_hashing_indices);
    feature_hashing_coeff_.resize(num_feature_hashing_indices);
    for (int_fast64_t ii = 0; ii < num_feature_hashing_indices; ++ii) {
      feature_hashing_index_[ii] = feature_hashing_distribution(gen);
      feature_hashing_coeff_[ii] = 2 * bernoulli(gen) - 1;
    }
  }

  void embed(const DerivedVectorT& v, int_fast32_t l, int_fast32_t k,
             HashedVectorT* res) const {
    res->setZero();
    IndexType offset =
        ((static_cast<int_fast64_t>(l) * this->k_) + k) * vector_dim_;

    for (IndexType ii = 0; ii < static_cast<IndexType>(v.size()); ++ii) {
      IndexType cur_ind = v[ii].first;
      CoordinateType cur_val = v[ii].second;

      (*res)[feature_hashing_index_[offset + cur_ind]] +=
          feature_hashing_coeff_[offset + cur_ind] * cur_val;
    }
  }

 private:
  const int_fast32_t vector_dim_;  // actual dimension of the vectors
  std::vector<int> feature_hashing_index_;
  std::vector<CoordinateType> feature_hashing_coeff_;
};

template <typename CoordinateType = float, typename HashType = uint32_t>
class CrossPolytopeHashDense
    : public CrossPolytopeHashBase<
          CrossPolytopeHashDense<CoordinateType, HashType>,
          Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>,
          CoordinateType, HashType> {
 public:
  typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>
      DerivedVectorT;

  CrossPolytopeHashDense(int_fast32_t vector_dim, int_fast32_t k,
                         int_fast32_t l, int_fast32_t num_rotations,
                         int_fast32_t last_cp_dim, uint_fast64_t seed)
      : CrossPolytopeHashBase<
            CrossPolytopeHashDense<CoordinateType, HashType>,
            Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>,
            CoordinateType, HashType>(find_next_power_of_two(vector_dim), k, l,
                                      num_rotations, last_cp_dim, seed),
        vector_dim_(vector_dim) {
    if (vector_dim_ < 1) {
      throw LSHFunctionError("Vector dimension must be at least 1.");
    }
  }

  void embed(const DerivedVectorT& v, int, int, DerivedVectorT* result) const {
    // TODO: use something more low-level here?
    for (int_fast32_t ii = 0; ii < vector_dim_; ++ii) {
      (*result)[ii] = v[ii];
    }
    for (int_fast32_t ii = vector_dim_; ii < this->rotation_dim_; ++ii) {
      (*result)[ii] = 0.0;
    }
  }

 private:
  const int_fast32_t vector_dim_;  // actual dimension of the vectors
};

}  // namespace core
}  // namespace falconn

#endif
