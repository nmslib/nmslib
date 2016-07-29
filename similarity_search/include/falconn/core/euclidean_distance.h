#ifndef __EUCLIDEAN_DISTANCE_H__
#define __EUCLIDEAN_DISTANCE_H__

#include <cstdint>
#include <vector>

#include "../eigen_wrapper.h"

namespace falconn {
namespace core {

// Computes *SQUARED* Euclidean distance between sparse or dense vectors.

template <typename CoordinateType = float, typename IndexType = int32_t>
struct EuclideanDistanceSparse {
  typedef std::vector<std::pair<IndexType, CoordinateType>> VectorType;

  CoordinateType operator()(const VectorType& p1, const VectorType& p2) {
    CoordinateType res = 0.0;
    IndexType ii1 = 0, ii2 = 0;
    IndexType p1size = p1.size();
    IndexType p2size = p2.size();
    CoordinateType x;

    while (ii1 < p1size || ii2 < p2size) {
      if (ii2 == p2size) {
        x = p1[ii1].second;
        res += x * x;
        ++ii1;
        continue;
      }
      if (ii1 == p1size) {
        x = p2[ii2].second;
        res += x * x;
        ++ii2;
        continue;
      }
      if (p1[ii1].first < p2[ii2].first) {
        x = p1[ii1].second;
        res += x * x;
        ++ii1;
        continue;
      }
      if (p2[ii2].first < p1[ii1].first) {
        x = p2[ii2].second;
        res += x * x;
        ++ii2;
        continue;
      }
      x = p1[ii1].second;
      x -= p2[ii2].second;
      res += x * x;
      ++ii1;
      ++ii2;
    }

    return res;
  }
};

// The Dense functions assume that the data points are stored as dense
// Eigen column vectors.

template <typename CoordinateType = float>
struct EuclideanDistanceDense {
  typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>
      VectorType;

  template <typename Derived1, typename Derived2>
  CoordinateType operator()(const Eigen::MatrixBase<Derived1>& p1,
                            const Eigen::MatrixBase<Derived2>& p2) {
    return (p1 - p2).squaredNorm();
  }
};

}  // namespace core
}  // namespace falconn

#endif
