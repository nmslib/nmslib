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
#include "bunit.h"
#include "distcomp.h"
#include "randproj_util.h"
#include "genrand_vect.h"

namespace similarity {

using namespace std;

template <class dist_t>
bool TestRandProj(size_t srcDim, size_t dstDim, size_t rep, dist_t eps) {
  vector<vector<dist_t>>  projMatr;

  for (size_t r = 0; r < rep; ++r) {
    // 1. Generate the random matrix
    initRandProj(srcDim, dstDim, 
                 true, // yes, do create an orthonormal basis
                 projMatr);  
    // 2. Check if we got an orthonormal basis
    for (int i = 0; i < dstDim; ++i) {
      dist_t v = ScalarProductSIMD(&projMatr[i][0], &projMatr[i][0], srcDim);
      if (fabs(v - static_cast<dist_t>(1)) >  eps) {
        LOG(LIB_ERROR) << "Expected norm 1, got: " << v 
                       << " srcDim = " << srcDim << " dstDim = " << dstDim
                       << " type: " << typeid(dist_t).name();
        return false;
      }
      for (int k = i + 1; k < dstDim; ++k) {
        v = ScalarProductSIMD(&projMatr[i][0], &projMatr[k][0], srcDim);
        if (fabs(v - static_cast<dist_t>(0)) >  eps) {
          LOG(LIB_ERROR) << "Expected product 0, got: " << v 
                         << " srcDim = " << srcDim << " dstDim = " << dstDim
                       << " type: " << typeid(dist_t).name();
          return false;
        }
      }
    }
  } 
  return true;
}

TEST(TestRandProj) {
    int nTest  = 0;
    int nFail = 0;

    for (int srcDim = 1; srcDim <= 128; ) {
      for (int dstDim = 1; dstDim < srcDim; ) {
        ++nTest;
        nFail += !TestRandProj<float>(srcDim, dstDim, 2, 1e-4f);
        ++nTest;
        nFail += !TestRandProj<double>(srcDim, dstDim, 2, 1e-8);
        dstDim += (srcDim < 32 ? 1:8);
      }
      srcDim += (srcDim < 32 ? 1:8);
    }

    LOG(LIB_INFO) << nTest << " (sub) tests performed " << nFail << " failed";

    EXPECT_EQ(0, nFail);
}

}
