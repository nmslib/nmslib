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
#include <cmath>
#include <random>
#include <vector>
#include <sstream>
#include <stdexcept>

#include "randproj_util.h"
#include "distcomp.h"
#include "logging.h"
#include "utils.h"

namespace similarity {

using namespace std;

template <class dist_t> void initRandProj(size_t nSrcDim, size_t nDstDim,
                                         bool bDoOrth,
                                         vector<vector<dist_t>>& projMatr) {
  // Static is thread-safe in C++-11
  static thread_local auto&           randGen(getThreadLocalRandomGenerator());
  static  std::normal_distribution<>  normGen(0.0f, 1.0f);

  // 1. Create normally distributed vectors
  projMatr.resize(nDstDim);
  for (size_t i = 0; i < nDstDim; ++i) {
    projMatr[i].resize(nSrcDim);
    for (size_t j = 0; j < nSrcDim; ++j)
      projMatr[i][j] = normGen(randGen);
  }
  /* 
   * 2. If bDoOrth == true, normalize the basis using the numerically stable
        variant of the Gram–Schmidt process (see Wikipedia for details 
        http://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process#Algorithm).
   *    Otherwise, just divide each vector by its norm. 
   */
  size_t maxNormDim = min(nDstDim, nSrcDim);

  for (size_t i = 0; i < nDstDim; ++i) {
    if (bDoOrth) {
      // Normalize the outcome (in particular, to ensure the above mentioned invariant is true)
      dist_t normCoeff = sqrt(ScalarProductSIMD(&projMatr[i][0], &projMatr[i][0], nSrcDim));
      for (size_t n = 0; n < nSrcDim; ++n) projMatr[i][n] /= normCoeff;

      for (size_t k = i + 1; k < maxNormDim; ++k) {
        dist_t coeff = ScalarProductSIMD(&projMatr[i][0], &projMatr[k][0], nSrcDim);
      /* 
       * Invariant the all previously processed vectors have been normalized.
       * Therefore, we we subtract the projection to a previous vector u,
       * we don't divide elements by the norm of the vector u
       */
        for (size_t n = 0; n < nSrcDim; ++n) projMatr[k][n] -= coeff * projMatr[i][n];
      }
    }
  }
}

template <> void  initRandProj<int>(size_t nSrcDim, size_t nDstDim,
                                bool bDoOrth,
                                vector<vector<int>>& projMatr) {
  throw runtime_error("random projections are not supported for integer-valued distances!");
}

template void initRandProj<float>(size_t nSrcDim, size_t nDstDim,
                                  bool bDoOrth,
                                  vector<vector<float>>& projMatr);
template void initRandProj<double>(size_t nSrcDim, size_t nDstDim,
                                  bool bDoOrth,
                                  vector<vector<double>>& projMatr);

template <class dist_t> void compRandProj(const vector<vector<dist_t>>& projMatr, 
                                      const dist_t* pSrcVect, size_t nSrcDim,
                                      dist_t* pDstVect, size_t nDstDim) {
  if (projMatr.empty()) throw runtime_error("Bug: empty projection matrix");
  if (projMatr.size() != nDstDim) { 
    stringstream err;
    err << "Bug: the # of rows in the projection matrix (" << projMatr.size() << ")"
               << " isn't equal to the number of vector elements in the target space "
               << "(" << nDstDim << ")";
    throw runtime_error(err.str());
  }

  for (size_t i = 0; i < nDstDim; ++i) {
    if (projMatr[i].size() != nSrcDim) {
      stringstream err;
      err << "Bug: row index " << i << " the number of columns "
          << "(" << projMatr[i].size() << ")"
          << " isn't equal to the number of vector elements in the source space "
          << "(" << nSrcDim << ")";
      throw runtime_error(err.str());
    }
    pDstVect[i] = ScalarProductSIMD(&projMatr[i][0], pSrcVect, nSrcDim);
  }
}

template <> void compRandProj<int>(const vector<vector<int>>& projMatr,
                              const int* pSrcVect, size_t nSrcDim,
                              int* pDstVect, size_t nDstQty) {
  throw runtime_error("random projections are not supported for integer-valued distances!");
}

template void compRandProj<float>(const vector<vector<float>>& projMatr,
                              const float* pSrcVect, size_t nSrcDim,
                              float* pDstVect, size_t nDstQty);
template void compRandProj<double>(const vector<vector<double>>& projMatr,
                              const double* pSrcVect, size_t nSrcDim,
                              double* pDstVect, size_t nDstQty);
}
