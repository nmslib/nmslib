/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
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

namespace similarity {

#ifndef __SSE2__
// Of course, this would work only for Intel
#error "Need SSE2 to compile! Did you add -msse2 to the compiler options?"
#endif

#include <immintrin.h>

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
 * NOTE 2: if the number is <=0, its log is computed as the minimum possible numbers.
 */
template <class T> void PrecompLogarithms(T* pVect, size_t qty) {
    for (size_t i = 0; i < qty; i++) { 
        pVect[i + qty] = (pVect[i] > 0) ? log(pVect[i]): std::numeric_limits<T>::lowest();
    }
}

/*
 * Jensen-Shannon divergence 
 */
template <class T> T JSStandard(const T *pVect1, const T *pVect2, size_t qty);
// Precomputed logs
template <class T> T JSPrecomp(const T *pVect1, const T *pVect2, size_t qty);
// Precomputed logs, one log is approximate
template <class T> T JSPrecompApproxLog(const T *pVect1, const T *pVect2, size_t qty);

// Precomputed logs & inverse values
template <class T> T JSPrecompDivApproxLog(const T *pVect1, const T *pVect2, size_t qty);

template <class T> T JSPrecompSIMDApproxLog(const T* pVect1, const T* pVect2, size_t qty);



template <class T> 
void GenRandVect(T* pVect, size_t qty, T MinElem = T(0), bool DoNormalize = false) {
    T sum = 0;
    for (size_t i = 0; i < qty; ++i) {
        pVect[i] = std::max(static_cast<T>(drand48()), MinElem);
        sum += pVect[i];
    }
    if (DoNormalize && sum != 0) {
      for (size_t i = 0; i < qty; ++i) {
        pVect[i] /= sum;
      }
    }
}

template <class T> 
void SetRandZeros(T* pVect, size_t qty, double pZero) {
    for (size_t j = 0; j < qty; ++j) if (drand48() < pZero) pVect[j] = T(0);
}

/*
 * A slow version of LP-distance
 */
template <typename T>
T LPGenericDistance(const T* x, const T* y, const int length, const int p) {
  T result = 0;
  T temp;
  if (p == 0) {    // Infinity distance
    for (int i = 0; i < length; ++i) {
      temp = Abs(x[i] - y[i]);
      result = std::max(result, temp);
    }
  } else {
    for (int i = 0; i < length; ++i) {
      temp = Abs(x[i] - y[i]);
      result += pow(temp, p);
    }
    result = static_cast<T>(pow(static_cast<double>(result), 1.0 / p));
  }
  return result;
}

}


#endif
