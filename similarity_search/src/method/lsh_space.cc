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
#include "distcomp.h"
#include "method/lsh_space.h"

namespace similarity {

/*
 * TODO(@leo/bileg) 
 * Do we compute the number of times we invoke these functions during retrieval?
 * Perhaps, through query?
 */ 

// l1 & l2 distances for lsh
float LSHLp(const float* x, const float* y, const int dim, const int p) {
  if (p == 1) {
    return L1NormSIMD(x, y, dim);
  } else if (p == 2) {
    return L2NormSIMD(x, y, dim);
  } else {
    throw runtime_error("LSH can work only with l_1 and l_2");
  }
  return -1;
}

// l2sqr (squared euclidean distance) for multi probe lsh
float LSHMultiProbeLp(const float* x, const float* y, const int dim) {
#if 1
  return L2SqrSIMD(x, y, dim);
#else

  float r = 0, diff;
  for (int i = 0; i < dim; ++i) {
    diff = x[i] - y[i];
    r += diff * diff;
  }
  return r;
#endif
}

}   // namespace similarity
