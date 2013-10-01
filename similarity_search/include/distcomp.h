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

/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */
template <class T> T LInfNormSIMD(const T* pVect1, const T* pVect2, size_t qty);

template <> 
inline float LInfNormSIMD(const float* pVect1, const float* pVect2, size_t qty) {
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    __m128  diff, v1, v2; 
    __m128  MAX = _mm_set1_ps(0); 

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), diff), diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), diff), diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), diff), diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), diff), diff));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), diff), diff));
    }

    float __attribute__((aligned(16))) TmpRes[4];

    _mm_store_ps(TmpRes, MAX);
    float res= max(max(TmpRes[0], TmpRes[1]), max(TmpRes[2], TmpRes[3]));

    while (pVect1 < pEnd3) {
        res = max(res, fabsf(*pVect1++ - *pVect2++));
    }

    return res;
}

template <> 
inline double LInfNormSIMD(const double* pVect1, const double* pVect2, size_t qty) {
    size_t qty8 = qty/8;

    const double* pEnd1 = pVect1 + 8 * qty8;
    const double* pEnd2 = pVect1 + qty;

    __m128d  diff, v1, v2; 
    __m128d  MAX = _mm_set1_pd(0); 


    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        MAX  = _mm_max_pd(MAX, _mm_max_pd(_mm_sub_pd(_mm_setzero_pd(), diff), diff));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        MAX  = _mm_max_pd(MAX, _mm_max_pd(_mm_sub_pd(_mm_setzero_pd(), diff), diff));

    }

    double __attribute__((aligned(16))) TmpRes[2];

    _mm_storeu_pd(TmpRes, MAX);
    double res= max(TmpRes[0], TmpRes[1]);

    while (pVect1 < pEnd2) {
        // Leonid (@TODO) sometimes seg-faults in the unit test if float is replaced with double
        float diff = fabs(*pVect1++ - *pVect2++);
        res = max(res, (double)diff);
    }

    return res;
}

/*
 * L1 norm
 */

template <class T> T L1NormStandard(const T *p1, const T *p2, size_t qty);
template <class T> T L1Norm(const T *p1, const T *p2, size_t qty) ;

/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */
template <class T> T L1NormSIMD(const T* pVect1, const T* pVect2, size_t qty);

template <> 
inline float L1NormSIMD(const float* pVect1, const float* pVect2, size_t qty) {
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    __m128  diff, v1, v2; 
    __m128  sum = _mm_set1_ps(0); 

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), diff), diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), diff), diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), diff), diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), diff), diff));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), diff), diff));
    }

    float __attribute__((aligned(16))) TmpRes[4];

    _mm_store_ps(TmpRes, sum);
    float res= TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    while (pVect1 < pEnd3) {
        res += fabs(*pVect1++ - *pVect2++);
    }

    return res;
}

template <> 
inline double L1NormSIMD(const double* pVect1, const double* pVect2, size_t qty) {
    size_t qty8 = qty/8;

    const double* pEnd1 = pVect1 + 8 * qty8;
    const double* pEnd2 = pVect1 + qty;

    __m128d  diff, v1, v2; 
    __m128d  sum = _mm_set1_pd(0); 


    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        sum  = _mm_add_pd(sum, _mm_max_pd(_mm_sub_pd(_mm_setzero_pd(), diff), diff));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        sum  = _mm_add_pd(sum, _mm_max_pd(_mm_sub_pd(_mm_setzero_pd(), diff), diff));

    }

    double __attribute__((aligned(16))) TmpRes[2];

    _mm_store_pd(TmpRes, sum);
    double res= TmpRes[0] + TmpRes[1];

    while (pVect1 < pEnd2) {
        // Leonid (@TODO) sometimes seg-faults in the unit test if float is replaced with double
        float diff = *pVect1++ - *pVect2++;
        res += fabs(diff);
    }

    return res;
}

/*
 * L2 (Eucledian) norm
 */

template <class T> T L2NormStandard(const T *p1, const T *p2, size_t qty);
template <class T> T L2Norm(const T* pVect1, const T* pVect2, size_t qty);

/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */
template <class T> T L2NormSIMD(const T* pVect1, const T* pVect2, size_t qty);

inline float L2SqrSIMD(const float* pVect1, const float* pVect2, size_t qty) {
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    __m128  diff, v1, v2; 
    __m128  sum = _mm_set1_ps(0); 

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
    }

    float __attribute__((aligned(16))) TmpRes[4];

    _mm_store_ps(TmpRes, sum);
    float res= TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    while (pVect1 < pEnd3) {
        float diff = *pVect1++ - *pVect2++; 
        res += diff * diff;
    }

    return res;
}

template <> inline float L2NormSIMD(const float* pVect1, const float* pVect2, size_t qty) {
    float res = L2SqrSIMD(pVect1, pVect2, qty);
    return sqrt(res);
}

template <> 
inline double L2NormSIMD(const double* pVect1, const double* pVect2, size_t qty) {
    size_t qty8 = qty/8;

    const double* pEnd1 = pVect1 + 8 * qty8;
    const double* pEnd2 = pVect1 + qty;

    __m128d  diff, v1, v2; 
    __m128d  sum = _mm_set1_pd(0); 

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        sum  = _mm_add_pd(sum, _mm_mul_pd(diff, diff));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        diff = _mm_sub_pd(v1, v2);
        sum  = _mm_add_pd(sum, _mm_mul_pd(diff, diff));
    }

    double __attribute__((aligned(16))) TmpRes[2];

    _mm_store_pd(TmpRes, sum);
    double res= TmpRes[0] + TmpRes[1];

    while (pVect1 < pEnd2) {
        // Leonid (@TODO) sometimes seg-faults in the unit test if float is replaced with double
        float diff = *pVect1++ - *pVect2++; 
        res += diff * diff;
    }

    return sqrt(res);
}

/*
 *  Itakura-Saito distance
 */
template <class T> 
inline T ItakuraSaito(const T* pVect1, const T* pVect2, size_t qty) {
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += pVect1[i]/pVect2[i]  - log(pVect1[i]/pVect2[i]) - 1;
    }

    return sum;
}
/* 
 * Precomp means that qty logarithms are precomputed 
 * and stored right after qty original vector values.
 * That's the layout is:
 * x1 ... x1_qty log(x1) log(x2) ... log(x_qty)
 */ 
template <class T> 
inline T ItakuraSaitoPrecomp(const T* pVect1, const T* pVect2, size_t qty) {
    T sum = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    const T* pVectLog1 = pVect1 + qty;
    const T* pVectLog2 = pVect2 + qty;

    while (pVect1 < pEnd1) {
        sum += (*pVect1)/(*pVect2) - ((*pVectLog1) - (*pVectLog2)); ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
        sum += (*pVect1)/(*pVect2) - ((*pVectLog1) - (*pVectLog2)); ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
        sum += (*pVect1)/(*pVect2) - ((*pVectLog1) - (*pVectLog2)); ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
        sum += (*pVect1)/(*pVect2) - ((*pVectLog1) - (*pVectLog2)); ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
    }

    while (pVect1 < pEnd2) {
        sum += (*pVect1)/(*pVect2) - ((*pVectLog1) - (*pVectLog2)); ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
    }

    return sum - qty;
}

/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */
template <class T> T ItakuraSaitoPrecompSIMD(const T* pVect1, const T* pVect2, size_t qty);

template <>
inline float ItakuraSaitoPrecompSIMD(const float* pVect1, const float* pVect2, size_t qty)
{
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    const float* pVectLog1 = pVect1 + qty;
    const float* pVectLog2 = pVect2 + qty;

    __m128  v1, v2, vLog1, vLog2;
    __m128  sum = _mm_set1_ps(0); 

    while (pVect1 < pEnd1) {
        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        v2      = _mm_loadu_ps(pVect2);     pVect2 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(sum, _mm_sub_ps(_mm_div_ps(v1, v2), _mm_sub_ps(vLog1, vLog2)));

        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        v2      = _mm_loadu_ps(pVect2);     pVect2 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(sum, _mm_sub_ps(_mm_div_ps(v1, v2), _mm_sub_ps(vLog1, vLog2)));

        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        v2      = _mm_loadu_ps(pVect2);     pVect2 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(sum, _mm_sub_ps(_mm_div_ps(v1, v2), _mm_sub_ps(vLog1, vLog2)));

        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        v2      = _mm_loadu_ps(pVect2);     pVect2 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(sum, _mm_sub_ps(_mm_div_ps(v1, v2), _mm_sub_ps(vLog1, vLog2)));

    }

    while (pVect1 < pEnd2) {
        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        v2      = _mm_loadu_ps(pVect2);     pVect2 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(sum, _mm_sub_ps(_mm_div_ps(v1, v2), _mm_sub_ps(vLog1, vLog2)));

    }

    float __attribute__((aligned(16))) TmpRes[4];

    _mm_store_ps(TmpRes, sum);
    float res= TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    while (pVect1 < pEnd3) {
        res += (*pVect1)/(*pVect2) - ((*pVectLog1) - (*pVectLog2)); ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
    }

    return res - qty;
}

template <>
inline double ItakuraSaitoPrecompSIMD(const double* pVect1, const double* pVect2, size_t qty)
{
    size_t qty8 = qty/8;

    const double* pEnd1 = pVect1 + 8 * qty8;
    const double* pEnd2 = pVect1 + qty;

    const double* pVectLog1 = pVect1 + qty;
    const double* pVectLog2 = pVect2 + qty;

    __m128d  v1, v2, vLog1, vLog2;
    __m128d  sum = _mm_set1_pd(0); 

    while (pVect1 < pEnd1) {
        v1      = _mm_loadu_pd(pVect1);     pVect1 += 2;
        v2      = _mm_loadu_pd(pVect2);     pVect2 += 2;
        vLog1   = _mm_loadu_pd(pVectLog1);  pVectLog1 += 2;
        vLog2   = _mm_loadu_pd(pVectLog2);  pVectLog2 += 2;
        sum  = _mm_add_pd(sum, _mm_sub_pd(_mm_div_pd(v1, v2),  _mm_sub_pd(vLog1, vLog2)));
    
        v1      = _mm_loadu_pd(pVect1);     pVect1 += 2;
        v2      = _mm_loadu_pd(pVect2);     pVect2 += 2;
        vLog1   = _mm_loadu_pd(pVectLog1);  pVectLog1 += 2;
        vLog2   = _mm_loadu_pd(pVectLog2);  pVectLog2 += 2;
        sum  = _mm_add_pd(sum, _mm_sub_pd(_mm_div_pd(v1, v2),  _mm_sub_pd(vLog1, vLog2)));
    
    }

    double __attribute__((aligned(16))) TmpRes[2];

    _mm_store_pd(TmpRes, sum);
    double res= TmpRes[0] + TmpRes[1];

    while (pVect1 < pEnd2) {
        res += (*pVect1)/(*pVect2) - ((*pVectLog1) - (*pVectLog2)); ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
    }

    return res - qty;
}

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
/* 
 * Precomp means that qty logarithms are precomputed 
 * and stored right after qty original vector values.
 * That's the layout is:
 * x1 ... x1_qty log(x1) log(x2) ... log(x_qty)
 */ 
template <class T> T KLPrecomp(const T* pVect1, const T* pVect2, size_t qty);
// Same as KLPrecomp, but uses SIMD to make computation faster.

/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */
template <class T> T KLPrecompSIMD(const T* pVect1, const T* pVect2, size_t qty);

template <>
inline float KLPrecompSIMD(const float* pVect1, const float* pVect2, size_t qty)
{
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    const float* pVectLog1 = pVect1 + qty;
    const float* pVectLog2 = pVect2 + qty;

    __m128  v1, vLog1, vLog2;
    __m128  sum = _mm_set1_ps(0); 

    while (pVect1 < pEnd1) {
        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(sum, _mm_mul_ps(v1, _mm_sub_ps(vLog1, vLog2)));

        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(sum, _mm_mul_ps(v1, _mm_sub_ps(vLog1, vLog2)));

        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(sum, _mm_mul_ps(v1, _mm_sub_ps(vLog1, vLog2)));

        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(sum, _mm_mul_ps(v1, _mm_sub_ps(vLog1, vLog2)));
    }

    while (pVect1 < pEnd2) {
        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(sum, _mm_mul_ps(v1, _mm_sub_ps(vLog1, vLog2)));
    }

    float __attribute__((aligned(16))) TmpRes[4];

    _mm_store_ps(TmpRes, sum);
    float res= TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    while (pVect1 < pEnd3) {
        res += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
    }

    return res;
}

template <>
inline double KLPrecompSIMD(const double* pVect1, const double* pVect2, size_t qty)
{
    size_t qty8 = qty/8;

    const double* pEnd1 = pVect1 + 8 * qty8;
    const double* pEnd2 = pVect1 + qty;

    const double* pVectLog1 = pVect1 + qty;
    const double* pVectLog2 = pVect2 + qty;

    __m128d  v1, vLog1, vLog2;
    __m128d  sum = _mm_set1_pd(0); 

    while (pVect1 < pEnd1) {
        v1      = _mm_loadu_pd(pVect1);     pVect1 += 2;
        vLog1   = _mm_loadu_pd(pVectLog1);  pVectLog1 += 2;
        vLog2   = _mm_loadu_pd(pVectLog2);  pVectLog2 += 2;
        sum  = _mm_add_pd(sum, _mm_mul_pd(v1, _mm_sub_pd(vLog1, vLog2)));
    
        v1      = _mm_loadu_pd(pVect1);     pVect1 += 2;
        vLog1   = _mm_loadu_pd(pVectLog1);  pVectLog1 += 2;
        vLog2   = _mm_loadu_pd(pVectLog2);  pVectLog2 += 2;
        sum  = _mm_add_pd(sum, _mm_mul_pd(v1, _mm_sub_pd(vLog1, vLog2)));
    }

    double __attribute__((aligned(16))) TmpRes[2];

    _mm_store_pd(TmpRes, sum);
    double res= TmpRes[0] + TmpRes[1];

    while (pVect1 < pEnd2) {
        res += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
    }

    return res;
}

/*
 * Generalized KL-divergence
 * Should be applied ONLY to vectors such that:
 * 1) ALL elements are positive
 * 2) the sum of elements is ONE
 */
template <class T> T KLGeneralStandard(const T* pVect1, const T* pVect2, size_t qty);

/* 
 * Precomp means that qty logarithms are precomputed 
 * and stored right after qty original vector values.
 * That's the layout is:
 * x1 ... x1_qty log(x1) log(x2) ... log(x_qty)
 */ 
template <class T> T KLGeneralPrecomp(const T* pVect1, const T* pVect2, size_t qty);
// Same as KLPrecomp, but uses SIMD to make computation faster.

/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */
template <class T> T KLGeneralPrecompSIMD(const T* pVect1, const T* pVect2, size_t qty);

template <>
inline float KLGeneralPrecompSIMD(const float* pVect1, const float* pVect2, size_t qty)
{
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    const float* pVectLog1 = pVect1 + qty;
    const float* pVectLog2 = pVect2 + qty;

    __m128  v1, v2, vLog1, vLog2;
    __m128  sum = _mm_set1_ps(0); 

    while (pVect1 < pEnd1) {
        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        v2      = _mm_loadu_ps(pVect2);     pVect2 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(_mm_add_ps(sum, _mm_mul_ps(v1, _mm_sub_ps(vLog1, vLog2))), _mm_sub_ps(v2, v1));

        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        v2      = _mm_loadu_ps(pVect2);     pVect2 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(_mm_add_ps(sum, _mm_mul_ps(v1, _mm_sub_ps(vLog1, vLog2))), _mm_sub_ps(v2, v1));

        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        v2      = _mm_loadu_ps(pVect2);     pVect2 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(_mm_add_ps(sum, _mm_mul_ps(v1, _mm_sub_ps(vLog1, vLog2))), _mm_sub_ps(v2, v1));

        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        v2      = _mm_loadu_ps(pVect2);     pVect2 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(_mm_add_ps(sum, _mm_mul_ps(v1, _mm_sub_ps(vLog1, vLog2))), _mm_sub_ps(v2, v1));

    }

    while (pVect1 < pEnd2) {
        v1      = _mm_loadu_ps(pVect1);     pVect1 += 4;
        v2      = _mm_loadu_ps(pVect2);     pVect2 += 4;
        vLog1   = _mm_loadu_ps(pVectLog1);  pVectLog1 += 4;
        vLog2   = _mm_loadu_ps(pVectLog2);  pVectLog2 += 4;
        sum  = _mm_add_ps(_mm_add_ps(sum, _mm_mul_ps(v1, _mm_sub_ps(vLog1, vLog2))), _mm_sub_ps(v2, v1));

    }

    float __attribute__((aligned(16))) TmpRes[4];

    _mm_store_ps(TmpRes, sum);
    float res= TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    while (pVect1 < pEnd3) {
        res += (*pVect1) * ((*pVectLog1++) - (*pVectLog2++)) + (*pVect2) - (*pVect1);
        ++pVect1; ++pVect2;
    }

    return res;
}

template <>
inline double KLGeneralPrecompSIMD(const double* pVect1, const double* pVect2, size_t qty)
{
    size_t qty8 = qty/8;

    const double* pEnd1 = pVect1 + 8 * qty8;
    const double* pEnd2 = pVect1 + qty;

    const double* pVectLog1 = pVect1 + qty;
    const double* pVectLog2 = pVect2 + qty;

    __m128d  v1, v2, vLog1, vLog2;
    __m128d  sum = _mm_set1_pd(0); 

    while (pVect1 < pEnd1) {
        v1      = _mm_loadu_pd(pVect1);     pVect1 += 2;
        v2      = _mm_loadu_pd(pVect2);     pVect2 += 2;
        vLog1   = _mm_loadu_pd(pVectLog1);  pVectLog1 += 2;
        vLog2   = _mm_loadu_pd(pVectLog2);  pVectLog2 += 2;
        sum  = _mm_add_pd(_mm_add_pd(sum, _mm_mul_pd(v1, _mm_sub_pd(vLog1, vLog2))), _mm_sub_pd(v2, v1));
    
        v1      = _mm_loadu_pd(pVect1);     pVect1 += 2;
        v2      = _mm_loadu_pd(pVect2);     pVect2 += 2;
        vLog1   = _mm_loadu_pd(pVectLog1);  pVectLog1 += 2;
        vLog2   = _mm_loadu_pd(pVectLog2);  pVectLog2 += 2;
        sum  = _mm_add_pd(_mm_add_pd(sum, _mm_mul_pd(v1, _mm_sub_pd(vLog1, vLog2))), _mm_sub_pd(v2, v1));
    
    }

    double __attribute__((aligned(16))) TmpRes[2];

    _mm_store_pd(TmpRes, sum);
    double res= TmpRes[0] + TmpRes[1];

    while (pVect1 < pEnd2) {
        res += (*pVect1) * ((*pVectLog1++) - (*pVectLog2++)) + (*pVect2) - (*pVect1);
        ++pVect1; ++pVect2;
    }

    return res;
}

/*
 * Computes logarithms and stores them after qty values of pVect.
 * NOTE: pVect should have 2 * qty elements!!! but only
 *       the first qty elements are used to compute the logarithms.
 *       After this function completes, the would look like:
 *       x1 ... x1_qty log(x1) log(x2) ... log(x_qty)
 */
template <class T> void PrecompLogarithms(T* pVect, size_t qty) {
    for (size_t i = 0; i < qty; i++) { 
        pVect[i + qty] = log(pVect[i]);
    }
}

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

unsigned ED1(const char* s1, const int len1, const char* s2, const int len2);

unsigned ED2(const char* s1, const int len1, const char* s2, const int len2);

}


#endif
