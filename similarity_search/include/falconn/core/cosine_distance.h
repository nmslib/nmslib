#ifndef __COSINE_DISTANCE_H__
#define __COSINE_DISTANCE_H__

#include <cstdint>
#include <vector>

#include "../eigen_wrapper.h"

namespace falconn {
namespace core {

// TODO: rename to negative inner product distance?
// TODO: make a single CosineDistance class with different template
// specializations?

// The Sparse functions assume that the data points are stored as a
// std::vector of (index, coefficient) pairs. The indices must be sorted.

template <typename CoordinateType = float, typename IndexType = int32_t>
struct CosineDistanceSparse {
  typedef std::vector<std::pair<IndexType, CoordinateType>> VectorType;

  CoordinateType operator()(const VectorType& p1, const VectorType& p2) {
    CoordinateType res = 0.0;
    IndexType ii1 = 0, ii2 = 0;
    IndexType p1size = p1.size();
    IndexType p2size = p2.size();

    while (ii1 < p1size && ii2 < p2size) {
      while (ii1 < p1size && p1[ii1].first < p2[ii2].first) {
        ii1 += 1;
      }

      if (ii1 == p1size) {
        return res;
      }

      while (ii2 < p2size && p2[ii2].first < p1[ii1].first) {
        ii2 += 1;
      }

      if (ii2 == p2size) {
        return res;
      }

      if (p1[ii1].first == p2[ii2].first) {
        res += p1[ii1].second * p2[ii2].second;
        ii1 += 1;
        ii2 += 1;
      }
    }

    // negate the result because LSHTable assumes that smaller distances
    // are better
    return -res;
  }
};

// The Dense functions assume that the data points are stored as dense
// Eigen column vectors.

template <typename CoordinateType = float>
struct CosineDistanceDense {
  typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic, 1, Eigen::ColMajor>
      VectorType;

  template <typename Derived1, typename Derived2>
  CoordinateType operator()(const Eigen::MatrixBase<Derived1>& p1,
                            const Eigen::MatrixBase<Derived2>& p2) {
    // negate the result because LSHTable assumes that smaller distances
    // are better
    return -p1.dot(p2);
  }
};

}  // namespace core
}  // namespace falconn

#endif
