#ifndef __CPP_WRAPPER_IMPL_H__
#define __CPP_WRAPPER_IMPL_H__

#include <atomic>
#include <random>
#include <thread>
#include <type_traits>

#include "../core/bit_packed_flat_hash_table.h"
#include "../core/composite_hash_table.h"
#include "../core/cosine_distance.h"
#include "../core/data_storage.h"
#include "../core/euclidean_distance.h"
#include "../core/flat_hash_table.h"
#include "../core/hyperplane_hash.h"
#include "../core/lsh_table.h"
#include "../core/nn_query.h"
#include "../core/polytope_hash.h"
#include "../core/probing_hash_table.h"
#include "../core/stl_hash_table.h"

namespace falconn {
namespace wrapper {

template <typename PointType>
struct PointTypeTraitsInternal {};

// TODO: get rid of these type trait classes once CosineDistance and the LSH
// classes are specialized on PointType (if we want to specialize on point
// type).
template <typename CoordinateType>
class PointTypeTraitsInternal<DenseVector<CoordinateType>> {
 public:
  typedef core::CosineDistanceDense<CoordinateType> CosineDistance;
  typedef core::EuclideanDistanceDense<CoordinateType> EuclideanDistance;
  template <typename HashType>
  using HPHash = core::HyperplaneHashDense<CoordinateType, HashType>;
  template <typename HashType>
  using CPHash = core::CrossPolytopeHashDense<CoordinateType, HashType>;

  template <typename HashType>
  static std::unique_ptr<CPHash<HashType>> construct_cp_hash(
      const LSHConstructionParameters& params) {
    std::unique_ptr<CPHash<HashType>> res(new CPHash<HashType>(
        params.dimension, params.k, params.l, params.num_rotations,
        params.last_cp_dimension, params.seed ^ 93384688));
    return std::move(res);
  }
};

template <typename CoordinateType, typename IndexType>
class PointTypeTraitsInternal<SparseVector<CoordinateType, IndexType>> {
 public:
  typedef core::CosineDistanceSparse<CoordinateType, IndexType> CosineDistance;
  typedef core::EuclideanDistanceSparse<CoordinateType, IndexType>
      EuclideanDistance;
  template <typename HashType>
  using HPHash =
      core::HyperplaneHashSparse<CoordinateType, HashType, IndexType>;
  template <typename HashType>
  using CPHash =
      core::CrossPolytopeHashSparse<CoordinateType, HashType, IndexType>;

  template <typename HashType>
  static std::unique_ptr<CPHash<HashType>> construct_cp_hash(
      const LSHConstructionParameters& params) {
    std::unique_ptr<CPHash<HashType>> res(new CPHash<HashType>(
        params.dimension, params.k, params.l, params.num_rotations,
        params.feature_hashing_dimension, params.last_cp_dimension,
        params.seed ^ 93384688));
    return std::move(res);
  }
};

template <typename PointSet>
class DataStorageAdapter {
 public:
  DataStorageAdapter() {
    static_assert(FalseStruct<PointSet>::value,
                  "Point set type not supported.");
  }

  template <typename PS>
  struct FalseStruct : std::false_type {};
};

template <typename PointType>
class DataStorageAdapter<std::vector<PointType>> {
 public:
  template <typename KeyType>
  using DataStorage = core::ArrayDataStorage<PointType, KeyType>;

  template <typename KeyType>
  static std::unique_ptr<DataStorage<KeyType>> construct_data_storage(
      const std::vector<PointType>& points) {
    std::unique_ptr<DataStorage<KeyType>> res(new DataStorage<KeyType>(points));
    return std::move(res);
  }
};

template <typename CoordinateType>
class DataStorageAdapter<PlainArrayPointSet<CoordinateType>> {
 public:
  template <typename KeyType>
  using DataStorage =
      core::PlainArrayDataStorage<DenseVector<CoordinateType>, KeyType>;

  template <typename KeyType>
  static std::unique_ptr<DataStorage<KeyType>> construct_data_storage(
      const PlainArrayPointSet<CoordinateType>& points) {
    std::unique_ptr<DataStorage<KeyType>> res(new DataStorage<KeyType>(
        points.data, points.num_points, points.dimension));
    return std::move(res);
  }
};

template <typename PointType>
struct ComputeNumberOfHashFunctions {
  static void compute(int_fast32_t, LSHConstructionParameters*) {
    static_assert(FalseStruct<PointType>::value, "Point type not supported.");
  }
  template <typename T>
  struct FalseStruct : std::false_type {};
};

template <typename CoordinateType>
struct ComputeNumberOfHashFunctions<DenseVector<CoordinateType>> {
  static void compute(int_fast32_t number_of_hash_bits,
                      LSHConstructionParameters* params) {
    if (params->lsh_family == LSHFamily::Hyperplane) {
      params->k = number_of_hash_bits;
    } else if (params->lsh_family == LSHFamily::CrossPolytope) {
      if (params->dimension <= 0) {
        throw LSHNNTableSetupError(
            "Vector dimension must be set to determine "
            "the number of dense cross polytope hash functions.");
      }
      int_fast32_t rotation_dim =
          core::find_next_power_of_two(params->dimension);
      core::cp_hash_helpers::compute_k_parameters_for_bits(
          rotation_dim, number_of_hash_bits, &(params->k),
          &(params->last_cp_dimension));
    } else {
      throw LSHNNTableSetupError(
          "Cannot set paramters for unknown hash "
          "family.");
    }
  }
};

template <typename CoordinateType, typename IndexType>
struct ComputeNumberOfHashFunctions<SparseVector<CoordinateType, IndexType>> {
  static void compute(int_fast32_t number_of_hash_bits,
                      LSHConstructionParameters* params) {
    if (params->lsh_family == LSHFamily::Hyperplane) {
      params->k = number_of_hash_bits;
    } else if (params->lsh_family == LSHFamily::CrossPolytope) {
      if (params->feature_hashing_dimension <= 0) {
        throw LSHNNTableSetupError(
            "Feature hashing dimension must be set to "
            "determine  the number of sparse cross polytope hash functions.");
      }
      // TODO: add check here for power-of-two feature hashing dimension
      // (or allow non-power-of-two feature hashing dimension in the CP hash)
      int_fast32_t rotation_dim =
          core::find_next_power_of_two(params->feature_hashing_dimension);
      core::cp_hash_helpers::compute_k_parameters_for_bits(
          rotation_dim, number_of_hash_bits, &(params->k),
          &(params->last_cp_dimension));
    } else {
      throw LSHNNTableSetupError(
          "Cannot set paramters for unknown hash "
          "family.");
    }
  }
};

template <typename PointType>
struct ComputeNumberOfHashBits {
  static int_fast32_t compute(const LSHConstructionParameters&) {
    static_assert(FalseStruct<PointType>::value, "Point type not supported.");
    return 0;
  }
  template <typename T>
  struct FalseStruct : std::false_type {};
};

template <typename CoordinateType>
struct ComputeNumberOfHashBits<DenseVector<CoordinateType>> {
  static int_fast32_t compute(const LSHConstructionParameters& params) {
    if (params.k <= 0) {
      throw LSHNNTableSetupError(
          "Number of hash functions k must be at least "
          "1 to determine the number of hash bits.");
    }
    if (params.lsh_family == LSHFamily::Hyperplane) {
      return params.k;
    } else if (params.lsh_family == LSHFamily::CrossPolytope) {
      if (params.dimension <= 0) {
        throw LSHNNTableSetupError(
            "Vector dimension must be set to determine "
            "the number of dense cross polytope hash bits.");
      }
      if (params.last_cp_dimension <= 0) {
        throw LSHNNTableSetupError(
            "Last cross-polytope dimension must be set "
            "to determine the number of dense cross polytope hash bits.");
      }
      return core::cp_hash_helpers::compute_number_of_hash_bits(
          params.dimension, params.last_cp_dimension, params.k);
    } else {
      throw LSHNNTableSetupError(
          "Cannot compute number of hash bits for "
          "unknown hash family.");
    }
  }
};

template <typename CoordinateType, typename IndexType>
struct ComputeNumberOfHashBits<SparseVector<CoordinateType, IndexType>> {
  static int_fast32_t compute(const LSHConstructionParameters& params) {
    if (params.k <= 0) {
      throw LSHNNTableSetupError(
          "Number of hash functions k must be at least "
          "1 to determine the number of hash bits.");
    }
    if (params.lsh_family == LSHFamily::Hyperplane) {
      return params.k;
    } else if (params.lsh_family == LSHFamily::CrossPolytope) {
      if (params.feature_hashing_dimension <= 0) {
        throw LSHNNTableSetupError(
            "Feature hashing dimension must be set to "
            "determine the number of dense cross polytope hash bits.");
      }
      if (params.last_cp_dimension <= 0) {
        throw LSHNNTableSetupError(
            "Last cross-polytope dimension must be set "
            "to determine the number of dense cross polytope hash bits.");
      }
      return core::cp_hash_helpers::compute_number_of_hash_bits(
          params.feature_hashing_dimension, params.last_cp_dimension, params.k);
    } else {
      throw LSHNNTableSetupError(
          "Cannot compute number of hash bits for "
          "unknown hash family.");
    }
  }
};

template <typename PointType>
struct GetDefaultParameters {
  static LSHConstructionParameters get(int_fast64_t, int_fast32_t,
                                       DistanceFunction, bool) {
    static_assert(FalseStruct<PointType>::value, "Point type not supported.");
    LSHConstructionParameters tmp;
    return tmp;
  }
  template <typename T>
  struct FalseStruct : std::false_type {};
};

template <typename CoordinateType>
struct GetDefaultParameters<DenseVector<CoordinateType>> {
  static LSHConstructionParameters get(int_fast64_t dataset_size,
                                       int_fast32_t dimension,
                                       DistanceFunction distance_function,
                                       bool is_sufficiently_dense) {
    LSHConstructionParameters result;
    result.dimension = dimension;
    result.distance_function = distance_function;
    result.lsh_family = LSHFamily::CrossPolytope;

    result.num_rotations = 2;
    if (is_sufficiently_dense) {
      result.num_rotations = 1;
    }

    result.l = 10;
    result.storage_hash_table = StorageHashTable::BitPackedFlatHashTable;
    result.num_setup_threads = 0;

    int_fast32_t number_of_hash_bits = 1;
    while ((1 << (number_of_hash_bits + 2)) <= dataset_size) {
      ++number_of_hash_bits;
    }
    compute_number_of_hash_functions<DenseVector<CoordinateType>>(
        number_of_hash_bits, &result);

    return result;
  }
};

template <typename CoordinateType>
struct GetDefaultParameters<SparseVector<CoordinateType>> {
  static LSHConstructionParameters get(int_fast64_t dataset_size,
                                       int_fast32_t dimension,
                                       DistanceFunction distance_function,
                                       bool) {
    LSHConstructionParameters result;
    result.dimension = dimension;
    result.distance_function = distance_function;
    result.lsh_family = LSHFamily::CrossPolytope;
    result.feature_hashing_dimension = 1024;
    result.num_rotations = 2;

    result.l = 10;
    result.storage_hash_table = StorageHashTable::BitPackedFlatHashTable;
    result.num_setup_threads = 0;

    int_fast32_t number_of_hash_bits = 1;
    while ((1 << (number_of_hash_bits + 2)) <= dataset_size) {
      ++number_of_hash_bits;
    }
    compute_number_of_hash_functions<SparseVector<CoordinateType>>(
        number_of_hash_bits, &result);

    return result;
  }
};

template <typename PointType, typename KeyType, typename DistanceType,
          typename LSHTable, typename ScalarType, typename DistanceFunction,
          typename DataStorage>
class LSHNNQueryWrapper : public LSHNearestNeighborQuery<PointType, KeyType> {
  typedef core::NearestNeighborQuery<typename LSHTable::Query, PointType,
                                     KeyType, PointType, ScalarType,
                                     DistanceFunction, DataStorage>
      NNQueryType;

 public:
  LSHNNQueryWrapper(const LSHTable& parent, int_fast64_t num_probes,
                    int_fast64_t max_num_candidates,
                    const DataStorage& data_storage)
      : num_probes_(num_probes), max_num_candidates_(max_num_candidates) {
    if (num_probes <= 0) {
      throw LSHNearestNeighborTableError(
          "Number of probes must be at least 1.");
    }
    internal_query_.reset(new typename LSHTable::Query(parent));
    internal_nn_query_.reset(
        new NNQueryType(internal_query_.get(), data_storage));
  }

  KeyType find_nearest_neighbor(const PointType& q) {
    return internal_nn_query_->find_nearest_neighbor(q, q, num_probes_,
                                                     max_num_candidates_);
  }

  void find_k_nearest_neighbors(const PointType& q, int_fast64_t k,
                                std::vector<KeyType>* result) {
    internal_nn_query_->find_k_nearest_neighbors(q, q, k, num_probes_,
                                                 max_num_candidates_, result);
  }

  void find_near_neighbors(const PointType& q, DistanceType threshold,
                           std::vector<KeyType>* result) {
    internal_nn_query_->find_near_neighbors(q, q, threshold, num_probes_,
                                            max_num_candidates_, result);
  }

  void get_candidates_with_duplicates(const PointType& q,
                                      std::vector<KeyType>* result) {
    internal_nn_query_->get_candidates_with_duplicates(
        q, num_probes_, max_num_candidates_, result);
  }

  void get_unique_candidates(const PointType& q, std::vector<KeyType>* result) {
    internal_nn_query_->get_unique_candidates(q, num_probes_,
                                              max_num_candidates_, result);
  }

  int_fast64_t get_num_probes() { return num_probes_; }

  void set_num_probes(int_fast64_t new_num_probes) {
    if (new_num_probes <= 0) {
      throw LSHNearestNeighborTableError(
          "Number of probes must be at least 1.");
    }
    num_probes_ = new_num_probes;
  }

  int_fast64_t get_max_num_candidates() { return max_num_candidates_; }

  void set_max_num_candidates(int_fast64_t new_max_num_candidates) {
    max_num_candidates_ = new_max_num_candidates;
  }

  void reset_query_statistics() {
    internal_nn_query_->reset_query_statistics();
  }

  QueryStatistics get_query_statistics() {
    return internal_nn_query_->get_query_statistics();
  }

  virtual ~LSHNNQueryWrapper() {}

 protected:
  std::unique_ptr<typename LSHTable::Query> internal_query_;
  std::unique_ptr<NNQueryType> internal_nn_query_;
  int_fast64_t num_probes_;
  int_fast64_t max_num_candidates_;
};

template <typename PointType, typename KeyType, typename DistanceType,
          typename LSHTable, typename ScalarType, typename DistanceFunction,
          typename DataStorage>
class LSHNNQueryPool : public LSHNearestNeighborQueryPool<PointType, KeyType> {
  typedef core::NearestNeighborQuery<typename LSHTable::Query, PointType,
                                     KeyType, PointType, ScalarType,
                                     DistanceFunction, DataStorage>
      NNQueryType;

 public:
  LSHNNQueryPool(const LSHTable& parent, int_fast64_t num_probes,
                 int_fast64_t max_num_candidates,
                 const DataStorage& data_storage,
                 int_fast64_t num_query_objects)
      : locks_(num_query_objects),
        num_probes_(num_probes),
        max_num_candidates_(max_num_candidates) {
    if (num_probes <= 0) {
      throw LSHNearestNeighborTableError(
          "Number of probes must be at least 1.");
    }
    if (num_query_objects <= 0) {
      throw LSHNearestNeighborTableError(
          "Number of query objects in the pool must be at least 1.");
    }
    for (int ii = 0; ii < num_query_objects; ++ii) {
      std::unique_ptr<typename LSHTable::Query> cur_query(
          new typename LSHTable::Query(parent));
      std::unique_ptr<NNQueryType> cur_nn_query(
          new NNQueryType(cur_query.get(), data_storage));
      internal_queries_.push_back(std::move(cur_query));
      internal_nn_queries_.push_back(std::move(cur_nn_query));
      locks_[ii].clear(std::memory_order_release);
    }
  }

  KeyType find_nearest_neighbor(const PointType& q) {
    int_fast32_t query_index = get_query_index_and_lock();
    KeyType res = internal_nn_queries_[query_index]->find_nearest_neighbor(
        q, q, num_probes_, max_num_candidates_);
    unlock_query(query_index);
    return res;
  }

  void find_k_nearest_neighbors(const PointType& q, int_fast64_t k,
                                std::vector<KeyType>* result) {
    int_fast32_t query_index = get_query_index_and_lock();
    internal_nn_queries_[query_index]->find_k_nearest_neighbors(
        q, q, k, num_probes_, max_num_candidates_, result);
    unlock_query(query_index);
  }

  void find_near_neighbors(const PointType& q, DistanceType threshold,
                           std::vector<KeyType>* result) {
    int_fast32_t query_index = get_query_index_and_lock();
    internal_nn_queries_[query_index]->find_near_neighbors(
        q, q, threshold, num_probes_, max_num_candidates_, result);
    unlock_query(query_index);
  }

  void get_candidates_with_duplicates(const PointType& q,
                                      std::vector<KeyType>* result) {
    int_fast32_t query_index = get_query_index_and_lock();
    internal_nn_queries_[query_index]->get_candidates_with_duplicates(
        q, num_probes_, max_num_candidates_, result);
    unlock_query(query_index);
  }

  void get_unique_candidates(const PointType& q, std::vector<KeyType>* result) {
    int_fast32_t query_index = get_query_index_and_lock();
    internal_nn_queries_[query_index]->get_unique_candidates(
        q, num_probes_, max_num_candidates_, result);
    unlock_query(query_index);
  }

  int_fast64_t get_num_probes() { return num_probes_; }

  void set_num_probes(int_fast64_t new_num_probes) {
    if (new_num_probes <= 0) {
      throw LSHNearestNeighborTableError(
          "Number of probes must be at least 1.");
    }
    num_probes_ = new_num_probes;
  }

  int_fast64_t get_max_num_candidates() { return max_num_candidates_; }

  void set_max_num_candidates(int_fast64_t new_max_num_candidates) {
    max_num_candidates_ = new_max_num_candidates;
  }

  void reset_query_statistics() {
    for (int_fast64_t ii = 0;
         ii < static_cast<int_fast64_t>(internal_nn_queries_.size()); ++ii) {
      while (locks_[ii].test_and_set(std::memory_order_acquire))
        ;
      internal_nn_queries_[ii]->reset_query_statistics();
      locks_[ii].clear(std::memory_order_release);
    }
  }

  QueryStatistics get_query_statistics() {
    QueryStatistics res;
    for (int_fast64_t ii = 0;
         ii < static_cast<int_fast64_t>(internal_nn_queries_.size()); ++ii) {
      while (locks_[ii].test_and_set(std::memory_order_acquire))
        ;
      QueryStatistics cur_stats =
          internal_nn_queries_[ii]->get_query_statistics();
      cur_stats.convert_to_totals();
      res.add_totals(cur_stats);
      locks_[ii].clear(std::memory_order_release);
    }
    res.compute_averages();
    return res;
  }

  virtual ~LSHNNQueryPool() {}

 protected:
  int_fast32_t get_query_index_and_lock() {
    static thread_local std::minstd_rand gen((std::random_device())());
    std::uniform_int_distribution<int_fast32_t> dist(0, locks_.size() - 1);
    int_fast32_t cur_index = dist(gen);
    while (true) {
      if (!locks_[cur_index].test_and_set(std::memory_order_acquire)) {
        return cur_index;
      }
      if (cur_index == static_cast<int_fast32_t>(locks_.size()) - 1) {
        cur_index = 0;
      } else {
        cur_index += 1;
      }
    }
  }

  void unlock_query(int_fast32_t index) {
    locks_[index].clear(std::memory_order_release);
  }

  std::vector<std::unique_ptr<typename LSHTable::Query>> internal_queries_;
  std::vector<std::unique_ptr<NNQueryType>> internal_nn_queries_;
  std::vector<std::atomic_flag> locks_;
  int_fast64_t num_probes_;
  int_fast64_t max_num_candidates_;
};

template <typename PointType, typename KeyType, typename DistanceType,
          typename DistanceFunction, typename LSHTable, typename LSHFunction,
          typename HashTableFactory, typename CompositeHashTable,
          typename DataStorage>
class LSHNNTableWrapper : public LSHNearestNeighborTable<PointType, KeyType> {
 public:
  LSHNNTableWrapper(std::unique_ptr<LSHFunction> lsh,
                    std::unique_ptr<LSHTable> lsh_table,
                    std::unique_ptr<HashTableFactory> hash_table_factory,
                    std::unique_ptr<CompositeHashTable> composite_hash_table,
                    std::unique_ptr<DataStorage> data_storage)
      : lsh_(std::move(lsh)),
        lsh_table_(std::move(lsh_table)),
        hash_table_factory_(std::move(hash_table_factory)),
        composite_hash_table_(std::move(composite_hash_table)),
        data_storage_(std::move(data_storage)) {}

  std::unique_ptr<LSHNearestNeighborQuery<PointType, KeyType>>
  construct_query_object(int_fast64_t num_probes = -1,
                         int_fast64_t max_num_candidates = -1) const {
    if (num_probes <= 0) {
      num_probes = lsh_->get_l();
    }
    typedef typename PointTypeTraits<PointType>::ScalarType ScalarType;
    std::unique_ptr<
        LSHNNQueryWrapper<PointType, KeyType, DistanceType, LSHTable,
                          ScalarType, DistanceFunction, DataStorage>>
        nn_query(
            new LSHNNQueryWrapper<PointType, KeyType, DistanceType, LSHTable,
                                  ScalarType, DistanceFunction, DataStorage>(
                *lsh_table_, num_probes, max_num_candidates, *data_storage_));
    return std::move(nn_query);
  }

  std::unique_ptr<LSHNearestNeighborQueryPool<PointType, KeyType>>
  construct_query_pool(int_fast64_t num_probes = -1,
                       int_fast64_t max_num_candidates = -1,
                       int_fast64_t num_query_objects = -1) const {
    if (num_probes <= 0) {
      num_probes = lsh_->get_l();
    }
    if (num_query_objects <= 0) {
      num_query_objects = std::max(1u, 2 * std::thread::hardware_concurrency());
    }
    typedef typename PointTypeTraits<PointType>::ScalarType ScalarType;
    std::unique_ptr<LSHNNQueryPool<PointType, KeyType, DistanceType, LSHTable,
                                   ScalarType, DistanceFunction, DataStorage>>
        nn_query_pool(
            new LSHNNQueryPool<PointType, KeyType, DistanceType, LSHTable,
                               ScalarType, DistanceFunction, DataStorage>(
                *lsh_table_, num_probes, max_num_candidates, *data_storage_,
                num_query_objects));
    return std::move(nn_query_pool);
  }

  ~LSHNNTableWrapper() {}

 protected:
  std::unique_ptr<LSHFunction> lsh_;
  std::unique_ptr<LSHTable> lsh_table_;
  std::unique_ptr<HashTableFactory> hash_table_factory_;
  std::unique_ptr<CompositeHashTable> composite_hash_table_;
  std::unique_ptr<DataStorage> data_storage_;
};

template <typename PointType, typename KeyType, typename PointSet>
class StaticTableFactory {
 public:
  typedef typename PointTypeTraits<PointType>::ScalarType ScalarType;

  typedef typename DataStorageAdapter<PointSet>::template DataStorage<KeyType>
      DataStorageType;

  StaticTableFactory(const PointSet& points,
                     const LSHConstructionParameters& params)
      : points_(points), params_(params) {}

  std::unique_ptr<LSHNearestNeighborTable<PointType, KeyType>> setup() {
    if (params_.dimension < 1) {
      throw LSHNNTableSetupError(
          "Point dimension must be at least 1. Maybe "
          "you forgot to set the point dimension in the parameter struct?");
    }
    if (params_.k < 1) {
      throw LSHNNTableSetupError(
          "The number of hash functions k must be at "
          "least 1. Maybe you forgot to set k in the parameter struct?");
    }
    if (params_.l < 1) {
      throw LSHNNTableSetupError(
          "The number of hash tables l must be at "
          "least 1. Maybe you forgot to set l in the parameter struct?");
    }
    if (params_.num_setup_threads < 0) {
      throw LSHNNTableSetupError(
          "The number of setup threads cannot be "
          "negative. Maybe you forgot to set num_setup_threads in the "
          "parameter struct? A value of 0 indicates that FALCONN should use "
          "the maximum number of available hardware threads.");
    }
    if (params_.lsh_family == LSHFamily::Unknown) {
      throw LSHNNTableSetupError("The hash family is not specified.");
    }
    if (params_.distance_function == DistanceFunction::Unknown) {
      throw LSHNNTableSetupError("The distance function is not specified.");
    }
    if (params_.storage_hash_table == StorageHashTable::Unknown) {
      throw LSHNNTableSetupError("The storage type is not specified.");
    }
    if (params_.lsh_family == LSHFamily::CrossPolytope) {
      if (params_.last_cp_dimension < 1) {
        throw LSHNNTableSetupError(
            "Forgot to set last_cp_dimension in the parameter struct.");
      }
      if (params_.num_rotations < 1) {
        throw LSHNNTableSetupError(
            "Forgot to set num_rotations in the parameter struct.");
      }
      if (params_.feature_hashing_dimension < -1) {
        throw LSHNNTableSetupError(
            "Invalid value for the feature hashing dimension.");
      }
    }

    data_storage_ = std::move(
        DataStorageAdapter<PointSet>::template construct_data_storage<KeyType>(
            points_));

    ComputeNumberOfHashBits<PointType> helper;
    num_bits_ = helper.compute(params_);

    n_ = data_storage_->size();

    setup0();

    return std::move(table_);
  }

 private:
  void setup0() {
    if (num_bits_ <= 32) {
      typedef uint32_t HashType;
      HashType tmp;
      setup1(std::make_tuple(tmp));
    } else if (num_bits_ <= 64) {
      typedef uint64_t HashType;
      HashType tmp;
      setup1(std::make_tuple(tmp));
    } else {
      throw LSHNNTableSetupError(
          "More than 64 hash bits are currently not "
          "supported.");
    }
  }

  template <typename V>
  void setup1(V vals) {
    typedef typename std::tuple_element<kHashTypeIndex, V>::type HashType;

    if (params_.lsh_family == LSHFamily::Hyperplane) {
      typedef typename wrapper::PointTypeTraitsInternal<
          PointType>::template HPHash<HashType>
          LSH;
      std::unique_ptr<LSH> lsh(new LSH(params_.dimension, params_.k, params_.l,
                                       params_.seed ^ 93384688));
      setup2(std::tuple_cat(vals, std::make_tuple(std::move(lsh))));
    } else if (params_.lsh_family == LSHFamily::CrossPolytope) {
      if (params_.num_rotations < 0) {
        throw LSHNNTableSetupError(
            "The number of pseudo-random rotations for "
            "the cross polytope hash must be non-negative. Maybe you forgot to "
            "set num_rotations in the parameter struct?");
      }
      if (params_.last_cp_dimension <= 0) {
        throw LSHNNTableSetupError(
            "The last cross polytope dimension for "
            "the cross polytope hash must be at least 1. Maybe you forgot to "
            "set last_cp_dimension in the parameter struct?");
      }

      // TODO: for sparse vectors, also check feature_hashing_dimension here (it
      // is checked in the CP hash class, but the error message is less
      // verbose).

      typedef typename wrapper::PointTypeTraitsInternal<
          PointType>::template CPHash<HashType>
          LSH;
      std::unique_ptr<LSH> lsh(
          std::move(wrapper::PointTypeTraitsInternal<
                    PointType>::template construct_cp_hash<HashType>(params_)));
      setup2(std::tuple_cat(vals, std::make_tuple(std::move(lsh))));
    } else {
      throw LSHNNTableSetupError(
          "Unknown hash family. Maybe you forgot to set "
          "the hash family in the parameter struct?");
    }
  }

  template <typename V>
  void setup2(V vals) {
    if (params_.distance_function == DistanceFunction::NegativeInnerProduct) {
      typedef
          typename wrapper::PointTypeTraitsInternal<PointType>::CosineDistance
              DistanceFunc;
      DistanceFunc tmp;
      setup3(std::tuple_cat(std::move(vals), std::make_tuple(tmp)));
    } else if (params_.distance_function ==
               DistanceFunction::EuclideanSquared) {
      typedef typename wrapper::PointTypeTraitsInternal<
          PointType>::EuclideanDistance DistanceFunc;
      DistanceFunc tmp;
      setup3(std::tuple_cat(std::move(vals), std::make_tuple(tmp)));
    } else {
      throw LSHNNTableSetupError(
          "Unknown distance function. Maybe you forgot "
          "to set the hash family in the parameter struct?");
    }
  }

  template <typename V>
  void setup3(V vals) {
    typedef typename std::tuple_element<kHashTypeIndex, V>::type HashType;

    if (params_.storage_hash_table == StorageHashTable::FlatHashTable) {
      typedef core::FlatHashTable<HashType> HashTable;
      std::unique_ptr<typename HashTable::Factory> factory(
          new typename HashTable::Factory(1 << num_bits_));

      typedef core::StaticCompositeHashTable<HashType, KeyType, HashTable>
          CompositeTable;
      std::unique_ptr<CompositeTable> composite_table(
          new CompositeTable(params_.l, factory.get()));
      setup4(std::tuple_cat(std::move(vals),
                            std::make_tuple(std::move(factory)),
                            std::make_tuple(std::move(composite_table))));
    } else if (params_.storage_hash_table ==
               StorageHashTable::BitPackedFlatHashTable) {
      typedef core::BitPackedFlatHashTable<HashType> HashTable;
      std::unique_ptr<typename HashTable::Factory> factory(
          new typename HashTable::Factory(1 << num_bits_, n_));

      typedef core::StaticCompositeHashTable<HashType, KeyType, HashTable>
          CompositeTable;
      std::unique_ptr<CompositeTable> composite_table(
          new CompositeTable(params_.l, factory.get()));
      setup4(std::tuple_cat(std::move(vals),
                            std::make_tuple(std::move(factory)),
                            std::make_tuple(std::move(composite_table))));
    } else if (params_.storage_hash_table == StorageHashTable::STLHashTable) {
      typedef core::STLHashTable<HashType> HashTable;
      std::unique_ptr<typename HashTable::Factory> factory(
          new typename HashTable::Factory());

      typedef core::StaticCompositeHashTable<HashType, KeyType, HashTable>
          CompositeTable;
      std::unique_ptr<CompositeTable> composite_table(
          new CompositeTable(params_.l, factory.get()));
      setup4(std::tuple_cat(std::move(vals),
                            std::make_tuple(std::move(factory)),
                            std::make_tuple(std::move(composite_table))));
    } else if (params_.storage_hash_table ==
               StorageHashTable::LinearProbingHashTable) {
      typedef core::StaticLinearProbingHashTable<HashType, KeyType> HashTable;
      std::unique_ptr<typename HashTable::Factory> factory(
          new typename HashTable::Factory(2 * n_));

      typedef core::StaticCompositeHashTable<HashType, KeyType, HashTable>
          CompositeTable;
      std::unique_ptr<CompositeTable> composite_table(
          new CompositeTable(params_.l, factory.get()));
      setup4(std::tuple_cat(std::move(vals),
                            std::make_tuple(std::move(factory)),
                            std::make_tuple(std::move(composite_table))));
    } else {
      throw LSHNNTableSetupError(
          "Unknown storage hash table type. Maybe you "
          "forgot to set the hash table type in the parameter struct?");
    }
  }

  template <typename V>
  void setup4(V vals) {
    setup_final(std::move(vals));
  }

  template <typename V>
  void setup_final(V vals) {
    typedef typename std::tuple_element<kHashTypeIndex, V>::type HashType;

    typedef
        typename std::tuple_element<kLSHFamilyIndex, V>::type LSHPointerType;
    typedef typename LSHPointerType::element_type LSHType;

    typedef typename std::tuple_element<kDistanceFunctionIndex, V>::type
        DistanceFunctionType;

    typedef typename std::tuple_element<kHashTableFactoryIndex, V>::type
        HashTableFactoryPointerType;
    typedef
        typename HashTableFactoryPointerType::element_type HashTableFactoryType;

    typedef typename std::tuple_element<kCompositeHashTableIndex, V>::type
        CompositeHashTablePointerType;
    typedef typename CompositeHashTablePointerType::element_type
        CompositeHashTableType;

    std::unique_ptr<LSHType>& lsh = std::get<kLSHFamilyIndex>(vals);
    std::unique_ptr<HashTableFactoryType>& factory =
        std::get<kHashTableFactoryIndex>(vals);
    std::unique_ptr<CompositeHashTableType>& composite_table =
        std::get<kCompositeHashTableIndex>(vals);

    typedef core::StaticLSHTable<PointType, KeyType, LSHType, HashType,
                                 CompositeHashTableType, DataStorageType>
        LSHTableType;
    std::unique_ptr<LSHTableType> lsh_table(
        new LSHTableType(lsh.get(), composite_table.get(), *data_storage_,
                         params_.num_setup_threads));

    table_.reset(new LSHNNTableWrapper<PointType, KeyType, ScalarType,
                                       DistanceFunctionType, LSHTableType,
                                       LSHType, HashTableFactoryType,
                                       CompositeHashTableType, DataStorageType>(
        std::move(lsh), std::move(lsh_table), std::move(factory),
        std::move(composite_table), std::move(data_storage_)));
  }

  const static int_fast32_t kHashTypeIndex = 0;
  const static int_fast32_t kLSHFamilyIndex = 1;
  const static int_fast32_t kDistanceFunctionIndex = 2;
  const static int_fast32_t kHashTableFactoryIndex = 3;
  const static int_fast32_t kCompositeHashTableIndex = 4;

  const PointSet& points_;
  const LSHConstructionParameters& params_;
  std::unique_ptr<DataStorageType> data_storage_;
  int_fast32_t num_bits_;
  int_fast64_t n_;
  std::unique_ptr<LSHNearestNeighborTable<PointType, KeyType>> table_ = nullptr;
};

}  // namespace wrapper
}  // namespace falconn

namespace falconn {

template <typename PointType>
void compute_number_of_hash_functions(int_fast32_t number_of_hash_bits,
                                      LSHConstructionParameters* params) {
  wrapper::ComputeNumberOfHashFunctions<PointType>::compute(number_of_hash_bits,
                                                            params);
}

template <typename PointType>
LSHConstructionParameters get_default_parameters(
    int_fast64_t dataset_size, int_fast32_t dimension,
    DistanceFunction distance_function, bool is_sufficiently_dense) {
  return wrapper::GetDefaultParameters<PointType>::get(
      dataset_size, dimension, distance_function, is_sufficiently_dense);
}

template <typename PointType, typename KeyType, typename PointSet>
std::unique_ptr<LSHNearestNeighborTable<PointType, KeyType>> construct_table(
    const PointSet& points, const LSHConstructionParameters& params) {
  wrapper::StaticTableFactory<PointType, KeyType, PointSet> factory(points,
                                                                    params);
  return std::move(factory.setup());
}

}  // namespace falconn

#endif
