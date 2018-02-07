/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2013-2018
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef _LSH_SPACE_H_
#define _LSH_SPACE_H_

#include <cmath>
#include "knnquery.h"

namespace similarity {

float LSHLp(const float* x, const float* y, const int dim, const int p);

template <typename dist_t>
class LSHLpSpace {
 public:
  LSHLpSpace(unsigned dim, unsigned p, KNNQuery<dist_t>* query)
      : dim_(dim), p_(p), query_(query) {
  }

  ~LSHLpSpace() {}

  float operator()(const float* x, const float* y) const {
    query_->AddDistanceComputations(1);
    return LSHLp(x, y, dim_, p_);
  }

 private:
  unsigned dim_;
  unsigned p_;
  KNNQuery<dist_t>* query_;
};

float LSHMultiProbeLp(const float* x, const float* y, const int dim);

template <typename dist_t>
class LSHMultiProbeLpSpace {
 public:
  LSHMultiProbeLpSpace(unsigned dim, KNNQuery<dist_t>* query)
      : dim_(dim), query_(query) {
  }

  ~LSHMultiProbeLpSpace() {}

  float operator()(const float* x, const float* y) const {
    query_->AddDistanceComputations(1);
    return LSHMultiProbeLp(x, y, dim_);
  }

 private:
  unsigned dim_;
  KNNQuery<dist_t>* query_;
};


}   // namespace similarity

#endif     // _LSH_SPACE_H_
