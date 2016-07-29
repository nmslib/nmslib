#ifndef __PREFETCHERS_H__
#define __PREFETCHERS_H__

#include <cstdint>
#include <utility>
#include <vector>

#include "../eigen_wrapper.h"

namespace falconn {
namespace core {

template <typename PointType>
class StdVectorPrefetcher {
 public:
  // Parameters are points and prefetch_index.
  void prefetch(const std::vector<PointType>&, int_fast64_t) {
    static_assert(FalseStruct<PointType>::value,
                  "No prefetcher implemented for this point type.");
  }

 private:
  template <typename PT>
  struct FalseStruct : std::false_type {};
};

template <typename CoordinateType>
class StdVectorPrefetcher<
    Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>> {
 public:
  typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>
      PointType;

  void prefetch(const std::vector<PointType>& points,
                int_fast64_t prefetch_index) {
    __builtin_prefetch((points[prefetch_index]).data(), 0, 1);
  }
};

template <typename CoordinateType, typename IndexType>
class StdVectorPrefetcher<std::vector<std::pair<IndexType, CoordinateType>>> {
 public:
  typedef std::vector<std::pair<IndexType, CoordinateType>> PointType;
  /*DefaultPrefetcher() {
    printf("In sparse prefetcher.\n");
  }*/

  void prefetch(const std::vector<PointType>& points,
                int_fast64_t prefetch_index) {
    __builtin_prefetch((points[prefetch_index]).data(), 0, 1);
  }
};

template <typename T>
class PlainArrayPrefetcher {
 public:
  void prefetch(const T* p) { __builtin_prefetch(p, 0, 1); }
};

}  // namespace core
}  // namespace falconn

#endif
