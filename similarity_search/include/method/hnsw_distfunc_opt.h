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
/*
*
* A Hierarchical Navigable Small World (HNSW) approach.
*
* The main publication is (available on arxiv: http://arxiv.org/abs/1603.09320):
* "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs"
* Yu. A. Malkov, D. A. Yashunin
* This code was contributed by Yu. A. Malkov. It also was used in tests from the paper.
*
*
*/
#pragma once


#include "portable_simd.h"
#include "portable_intrinsics.h"
#include "distcomp.h"

namespace similarity {

float L2SqrExtSSE(const float *pVect1, const float *pVect2, size_t &qty, float * __restrict TmpRes);
float L2Sqr16ExtAVX(const float *pVect1, const float *pVect2, size_t &qty, float * __restrict TmpRes);


/* These prototypes are exposed only b/c we need them for unit testing */
float ScalarProductAVX(const float *__restrict pVect1, const float *__restrict pVect2, size_t qty, float *__restrict TmpRes);
float ScalarProductSSE(const float *__restrict pVect1, const float *__restrict pVect2, size_t qty, float *__restrict TmpRes);

float L2Sqr16ExtSSE(const float *pVect1, const float *pVect2, size_t &qty, float * __restrict TmpRes);
float L2Sqr16ExtAVX(const float *pVect1, const float *pVect2, size_t &qty, float * __restrict TmpRes);

float L2SqrExtSSE(const float *pVect1, const float *pVect2, size_t &qty, float * __restrict TmpRes);


}
