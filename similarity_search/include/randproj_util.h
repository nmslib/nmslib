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
#ifndef _RANDPROJ_UTILS_H_
#define _RANDPROJ_UTILS_H_

#include <vector>

#include "distcomp.h"

namespace similarity {

using std::vector;

/*
 * These are the classic random projections that rely on random normal numbers.
 * Vectors are subsequently normalized using the Gram-Schmidt algorithm (therefore,
 * nDstDim should be <= nSrcDim). 
 * For a more detailed discussion see, e.g.:
 *
 * Dasgupta, Sanjoy. "Experiments with random projection." 
 * Proceedings of the Sixteenth conference on Uncertainty in artificial intelligence. 2000.
 *
 */ 

template <class dist_t> void initRandProj(size_t nSrcDim, size_t nDstDim,
                                          bool bDoOrth, vector<vector<dist_t>>& projMatr);
template <class dist_t> void compRandProj(const vector<vector<dist_t>>& projMatr,
                                      const dist_t* pSrcVect, size_t nSrcDim,
                                      dist_t* pDstVect, size_t nDstDim);


}


#endif
