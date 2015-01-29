/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <cmath>
#include <random>
#include <vector>

#include "randproj_util.h"
#include "distcomp.h"
#include "logging.h"

namespace similarity {

using namespace std;

template <class dist_t> void initRandProj(size_t nVect, size_t nElem,
                                         bool bDoOrth,
                                         vector<vector<dist_t>>& projMatr) {
  // Static is thread-safe in C++-11
  static  std::random_device          rd;
  static  std::mt19937                engine(rd());
  static  std::normal_distribution<>  normGen(0.0f, 1.0f);

  // 1. Create normally distributed vectors
  projMatr.resize(nVect);
  for (size_t i = 0; i < nVect; ++i) {
    projMatr[i].resize(nElem);
    for (size_t j = 0; j < nElem; ++j)
      projMatr[i][j] = normGen(engine);
  }
  /* 
   * 2. If bDoOrth == true, normalize the basis using the Gram–Schmidt process.
   *    Otherwise, just divide each vector by its norm. 
   */
  for (size_t i = 0; i < nVect; ++i) {
    if (bDoOrth) {
      for (size_t k = 0; k < i; ++k) {
        dist_t coeff = ScalarProductSIMD(&projMatr[i][0], &projMatr[k][0], nElem);
      /* 
       * Invariant the all previously processed vectors have been normalized.
       * Therefore, we we subtract the projection to a previous vector u,
       * we don't divide elements by the norm of the vector u
       */
        for (size_t n = 0; n < nElem; ++n) projMatr[i][n] -= coeff * projMatr[k][n];
      }
    }
    // Normalize the outcome (in particular, to ensure the above mentioned invariant is true)
    dist_t normCoeff = ScalarProductSIMD(&projMatr[i][0], &projMatr[i][0], nElem);
    for (size_t n = 0; n < nElem; ++n) projMatr[i][n] /= normCoeff;
  }
}

template void initRandProj<float>(size_t nVect, size_t nElem,
                                  bool bDoOrth,
                                  vector<vector<float>>& projMatr);
template void initRandProj<double>(size_t nVect, size_t nElem,
                                  bool bDoOrth,
                                  vector<vector<double>>& projMatr);

template <class dist_t> void compProj(const vector<vector<dist_t>>& projMatr, 
                                      const dist_t* pSrcVect, size_t nSrcQty,
                                      dist_t* pDstVect, size_t nDstQty) {
  if (projMatr.empty()) LOG(LIB_FATAL) << "Bug: empty projection matrix";
  if (projMatr.size() != nDstQty) 
    LOG(LIB_FATAL) << "Bug: the # of rows in the projection matrix (" << projMatr.size() << ")"
               << " isn't equal to the number of vector elements in the target space "
               << "(" << nDstQty << ")";

  for (size_t i = 0; i < nDstQty; ++i) {
    if (projMatr[i].size() != nSrcQty) {
      LOG(LIB_FATAL) << "Bug: row index " << i << " the number of columns "
                 << "(" << projMatr[i].size() << ")"
                 << " isn't equal to the number of vector elements in the source space "
                 << "(" << nSrcQty << ")";
    }
    pDstVect[i] = ScalarProductSIMD(&projMatr[i][0], pSrcVect, nSrcQty);
  }
}

template void compProj<float>(const vector<vector<float>>& projMatr,
                              const float* pSrcVect, size_t nSrcQty,
                              float* pDstVect, size_t nDstQty);
template void compProj<double>(const vector<vector<double>>& projMatr,
                              const double* pSrcVect, size_t nSrcQty,
                              double* pDstVect, size_t nDstQty);
}
