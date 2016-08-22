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
#ifndef DISTCOMP_HPP
#define DISTCOMP_HPP

#include <iostream>

#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <limits>

#include "permutation_type.h"

namespace similarity {

using std::max;

/*
 * LInf norm
 */

template <class T> T LInfNormStandard(const T *p1, const T *p2, size_t qty);
template <class T> T LInfNorm(const T *p1, const T *p2, size_t qty) ;
template <class T> T LInfNormSIMD(const T* pVect1, const T* pVect2, size_t qty);


/*
 * L1 norm
 */

template <class T> T L1NormStandard(const T *p1, const T *p2, size_t qty);
template <class T> T L1Norm(const T *p1, const T *p2, size_t qty) ;
template <class T> T L1NormSIMD(const T* pVect1, const T* pVect2, size_t qty);


/*
 * L2 (Eucledian) norm
 */

template <class T> T L2NormStandard(const T *p1, const T *p2, size_t qty);
template <class T> T L2Norm(const T* pVect1, const T* pVect2, size_t qty);
template <class T> T L2NormSIMD(const T* pVect1, const T* pVect2, size_t qty);

float L2SqrSIMD(const float* pVect1, const float* pVect2, size_t qty);

/*
 * Scalar product related distances 
 */
template <class T> T AngularDistance(const T *p1, const T *p2, size_t qty);
template <class T> T CosineSimilarity(const T *p1, const T *p2, size_t qty);
// Scalar product divided by vector Euclidean norms
template <class T> T NormScalarProduct(const T *p1, const T *p2, size_t qty);
// Sclar product divided by query vector Euclidean norm
// Query is the second argument (by convention we use only left queries, where a data point is the left argument)
// We don't have a SIMD version for QueryNormScalarProduct function yet.
template <class T> T QueryNormScalarProduct(const T *p1, const T *p2, size_t qty);
template <class T> T NormScalarProductSIMD(const T *p1, const T *p2, size_t qty);

// Scalar product that is not normalized 
template <class T> T ScalarProduct(const T *p1, const T *p2, size_t qty);
template <class T> T ScalarProductSIMD(const T *p1, const T *p2, size_t qty);

// Fast normalized-scalar product between sparse vectors (using SIMD)
float NormSparseScalarProductFast(const char* pData1, size_t len1, const char* pData2, size_t len2);
/*
 * Fast query-side normalized-scalar product between sparse vectors (using SIMD).
 * By our standard convention of using left queries, the query is the right argument.
 */
float QueryNormSparseScalarProductFast(const char* pData, size_t lenData, const char* pQuery, size_t lenQuery);
// Fast scalar product between sparse vectors without normalization (using SIMD)
float SparseScalarProductFast(const char* pData1, size_t len1, const char* pData2, size_t len2);

/*
 * Sometimes due to rounding errors, we get values > 1 or < -1.
 * This throws off other functions that use scalar product, e.g., acos
 */

/*
 *  Itakura-Saito distance
 */
template <class T> T ItakuraSaito(const T* pVect1, const T* pVect2, size_t qty);

/* 
 * Precomp means that qty logarithms are precomputed 
 * and stored right after qty original vector values.
 * That's the layout is:
 * x1 ... x1_qty log(x1) log(x2) ... log(x_qty)
 */ 

template <class T> T ItakuraSaitoPrecomp(const T* pVect1, const T* pVect2, size_t qty);
template <class T> T ItakuraSaitoPrecompSIMD(const T* pVect1, const T* pVect2, size_t qty);

/*
 * KL-divergence
 * Should be applied ONLY to vectors such that:
 * 1) ALL elements are positive
 * 2) the sum of elements is ONE
 */
template <class T> T KLStandard(const T* pVect1, const T* pVect2, size_t qty);
/*
 * This is a less efficient version of KLStandard that computes the difference
 * of logarithms as opposed to log of the ratio. Yet, it has better accuracy.
 */
template <class T> T KLStandardLogDiff(const T* pVect1, const T* pVect2, size_t qty);
// Precomputed logs
template <class T> T KLPrecomp(const T* pVect1, const T* pVect2, size_t qty);
// Same as KLPrecomp, but uses SIMD to make computation faster.
template <class T> T KLPrecompSIMD(const T* pVect1, const T* pVect2, size_t qty);

/*
 * Generalized KL-divergence
 * Should be applied ONLY to vectors such that:
 * 1) ALL elements are positive
 * 2) Yet, the sum of elements IS NOT NECESSARILY one.
 *
 */
template <class T> T KLGeneralStandard(const T* pVect1, const T* pVect2, size_t qty);
// Precomputed logs
template <class T> T KLGeneralPrecomp(const T* pVect1, const T* pVect2, size_t qty);
// Same as KLPrecomp, but uses SIMD to make computation faster.
template <class T> T KLGeneralPrecompSIMD(const T* pVect1, const T* pVect2, size_t qty);

/*
 * Computes logarithms and stores them after qty values of pVect.
 * NOTE 1: pVect should have 2 * qty elements!!! but only
 *       the first qty elements are used to compute the logarithms.
 *       After this function completes, the would look like:
 *       x1 ... x1_qty log(x1) log(x2) ... log(x_qty)
 *
 * NOTE 2: if the number is <=0, its log is computed as the minimum possible numbers,
 *         rather than minus infinity.
 */
template <class T> void PrecompLogarithms(T* pVect, size_t qty) {
    for (size_t i = 0; i < qty; i++) { 
        //pVect[i + qty] = (pVect[i] > 0) ? log(pVect[i]): std::numeric_limits<T>::lowest();
        pVect[i + qty] = (pVect[i] > 0) ? log(pVect[i]): T(-1e5f);
    }
}

/*
 * Jensen-Shannon divergence 
 *
 * The squared root of JS-divergence is a metric.
 * 
 * Österreicher, Ferdinand, and Igor Vajda. 
 * "A new class of metric divergences on probability spaces and its applicability in statistics." 
 * Annals of the Institute of Statistical Mathematics 55.3 (2003): 639-653.
 *
 *
 * Endres, Dominik Maria, and Johannes E. Schindelin. 
 * "A new metric for probability distributions." 
 * Information Theory, IEEE Transactions on 49.7 (2003): 1858-1860.
 *
 */
template <class T> T JSStandard(const T *pVect1, const T *pVect2, size_t qty);
// Precomputed logs
template <class T> T JSPrecomp(const T *pVect1, const T *pVect2, size_t qty);
// Precomputed logs, one log is approximate
template <class T> T JSPrecompApproxLog(const T *pVect1, const T *pVect2, size_t qty);

// Precomputed logs & inverse values
template <class T> T JSPrecompDivApproxLog(const T *pVect1, const T *pVect2, size_t qty);

template <class T> T JSPrecompSIMDApproxLog(const T* pVect1, const T* pVect2, size_t qty);

/*
 * Slower versions of LP-distance
 */
template <typename T> T LPGenericDistance(const T* x, const T* y, const int length, const T p);

template <typename T> T LPGenericDistanceOptim(const T* x, const T* y, const int length, const T p);


/*
 * Rank correlations
 */

typedef int (*IntDistFuncPtr)(const PivotIdType* x, const PivotIdType* y, size_t qty);

int SpearmanFootrule(const PivotIdType* x, const PivotIdType* y, size_t qty);
int SpearmanRho(const PivotIdType* x, const PivotIdType* y, size_t qty);
int SpearmanFootruleSIMD(const PivotIdType* x, const PivotIdType* y, size_t qty);
int SpearmanRhoSIMD(const PivotIdType* x, const PivotIdType* y, size_t qty);

//unsigned BitHamming(const uint32_t* a, const uint32_t* b, size_t qty);

#include "simdutils.h"


unsigned inline BitHamming(const uint32_t* a, const uint32_t* b, size_t qty) {
  unsigned res = 0;

  for (size_t i=0; i < qty; ++i) {
    //  __builtin_popcount quickly computes the number on 1s
    res +=  __builtin_popcount(a[i] ^ b[i]);
  }

  return res;
}

}

/*
 * Edit distances
 */
#include "distcomp_edist.h"


#endif
