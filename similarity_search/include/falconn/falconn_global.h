#ifndef __FALCONN_GLOBAL_H__
#define __FALCONN_GLOBAL_H__

#include <stdexcept>
#include <utility>
#include <vector>

#include <Eigen/Dense>

namespace falconn {

///
/// Common exception base class
///
class FalconnError : public std::logic_error {
 public:
  FalconnError(const char* msg) : logic_error(msg) {}
};

///
/// General traits class for point types. Only the template specializations
/// below correspond to valid point types.
///
template <typename PointType>
struct PointTypeTraits {
  PointTypeTraits() {
    static_assert(FalseStruct<PointType>::value, "Point type not supported.");
  }

  template <typename PT>
  struct FalseStruct : std::false_type {};
};

///
/// Type for dense points / vectors. The coordinate type can be either float
/// or double (i.e., use DenseVector<float> or DenseVector<double>). In most
/// cases, float (single precision) should be sufficient.
///
template <typename CoordinateType>
using DenseVector =
    Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>;

///
/// Traits class for accessing the corresponding scalar type.
///
template <typename CoordinateType>
struct PointTypeTraits<DenseVector<CoordinateType>> {
  typedef CoordinateType ScalarType;
};

///
/// Type for sparse points / vectors. The coordinate type can be either float
/// or double (i.e., use SparseVector<float> or SparseVector<double>). In most
/// cases, float (single precision) should be sufficient.
///
/// The elements of the vector must be sorted by the index (the first
/// component of the pair).
///
/// Optionally, you can also change the type of the coordinate indices. This
/// might be useful if you have indices that fit into an int16_t and you want
/// to save memory.
///
template <typename CoordinateType, typename IndexType = int32_t>
using SparseVector = std::vector<std::pair<IndexType, CoordinateType>>;

///
/// Traits class for accessing the corresponding scalar type.
///
template <typename CoordinateType, typename IndexT>
struct PointTypeTraits<SparseVector<CoordinateType, IndexT>> {
  typedef CoordinateType ScalarType;
  typedef IndexT IndexType;
};

///
/// Data structure for point query statistics
///
struct QueryStatistics {
  ///
  /// Average total query time
  ///
  double average_total_query_time = 0.0;
  ///
  /// Average hashing time
  ///
  double average_lsh_time = 0.0;
  ///
  /// Average hash table retrieval time
  ///
  double average_hash_table_time = 0.0;
  ///
  /// Average time for computing distances
  ///
  double average_distance_time = 0.0;
  ///
  /// Average number of candidates
  ///
  double average_num_candidates = 0;
  ///
  /// Average number of *unique* candidates
  ///
  double average_num_unique_candidates = 0;
  ///
  /// Number of queries the statistics were computed over
  ///
  int_fast64_t num_queries = 0;

  // TODO: move these to internal helper functions?
  void convert_to_totals() {
    average_total_query_time *= num_queries;
    average_lsh_time *= num_queries;
    average_hash_table_time *= num_queries;
    average_distance_time *= num_queries;
    average_num_candidates *= num_queries;
    average_num_unique_candidates *= num_queries;
  }

  void compute_averages() {
    if (num_queries > 0) {
      average_total_query_time /= num_queries;
      average_lsh_time /= num_queries;
      average_hash_table_time /= num_queries;
      average_distance_time /= num_queries;
      average_num_candidates /= num_queries;
      average_num_unique_candidates /= num_queries;
    }
  }

  void add_totals(const QueryStatistics& other) {
    average_total_query_time += other.average_total_query_time;
    average_lsh_time += other.average_lsh_time;
    average_hash_table_time += other.average_hash_table_time;
    average_distance_time += other.average_distance_time;
    average_num_candidates += other.average_num_candidates;
    average_num_unique_candidates += other.average_num_unique_candidates;
    num_queries += other.num_queries;
  }

  void reset() {
    average_total_query_time = 0.0;
    average_lsh_time = 0.0;
    average_hash_table_time = 0.0;
    average_distance_time = 0.0;
    average_num_candidates = 0.0;
    average_num_unique_candidates = 0.0;
    num_queries = 0;
  }
};

}  // namespace falconn

#endif
