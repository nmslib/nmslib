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
#include "distcomp.h"
#include "string.h"

#include <cstdlib>
#include <limits>

namespace similarity {

using namespace std;

/*
 * LInf-norm.
 */

template <class T>
T LInfNormStandard(const T *p1, const T *p2, size_t qty) 
{ 
    T res = 0;

    for (size_t i = 0; i < qty; i++) { 
      res = max(res, fabs(p1[i]-p2[i]));
    }

    return res;
}

template float LInfNormStandard<float>(const float *p1, const float *p2, size_t qty);
template double LInfNormStandard<double>(const double *p1, const double *p2, size_t qty);


template <class T>
T LInfNorm(const T* pVect1, const T* pVect2, size_t qty) {
    T res = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    while (pVect1 < pEnd1) {
        res = max(res, fabs(*pVect1++ - *pVect2++));
        res = max(res, fabs(*pVect1++ - *pVect2++));
        res = max(res, fabs(*pVect1++ - *pVect2++));
        res = max(res, fabs(*pVect1++ - *pVect2++));
    }

    while (pVect1 < pEnd2) {
        res = max(res, fabs(*pVect1++ - *pVect2++));
    }

    return res;
}

template float LInfNorm<float>(const float* pVect1, const float* pVect2, size_t qty);
template double LInfNorm<double>(const double* pVect1, const double* pVect2, size_t qty);
/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */

template <> 
float LInfNormSIMD(const float* pVect1, const float* pVect2, size_t qty) {
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    __m128  diff, v1, v2; 

    /* 
     * A hack to quickly unset the sign flag.
     */
    __m128 mask_sign = reinterpret_cast<__m128>(_mm_set1_epi32(0x7fffffffu));
    __m128 MAX = _mm_setzero_ps();

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_and_ps(diff, mask_sign ));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_and_ps(diff, mask_sign ));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_and_ps(diff, mask_sign ));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_and_ps(diff, mask_sign ));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        MAX  = _mm_max_ps(MAX, _mm_and_ps(diff, mask_sign ));
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
double LInfNormSIMD(const double* pVect1, const double* pVect2, size_t qty) {
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

template float LInfNormSIMD<float>(const float* pVect1, const float* pVect2, size_t qty);
template double LInfNormSIMD<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * L1-norm.
 */

template <class T>
T L1NormStandard(const T *p1, const T *p2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += fabs(p1[i]-p2[i]);
    }

    return sum;
}

template float L1NormStandard<float>(const float *p1, const float *p2, size_t qty);
template double L1NormStandard<double>(const double *p1, const double *p2, size_t qty);


template <class T>
T L1Norm(const T* pVect1, const T* pVect2, size_t qty) {
    T res = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    while (pVect1 < pEnd1) {
        res += fabs(*pVect1++ - *pVect2++);
        res += fabs(*pVect1++ - *pVect2++);
        res += fabs(*pVect1++ - *pVect2++);
        res += fabs(*pVect1++ - *pVect2++);
    }

    while (pVect1 < pEnd2) {
        res += fabs(*pVect1++ - *pVect2++);
    }

    return res;
}

template float L1Norm<float>(const float* pVect1, const float* pVect2, size_t qty);
template double L1Norm<double>(const double* pVect1, const double* pVect2, size_t qty);
/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */

template <> 
float L1NormSIMD(const float* pVect1, const float* pVect2, size_t qty) {
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const float* pEnd1 = pVect1 + 16 * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    __m128  diff, v1, v2; 
    __m128  sum = _mm_setzero_ps();

    /* 
     * A hack to quickly unset the sign flag.
     */
    __m128 mask_sign = reinterpret_cast<__m128>(_mm_set1_epi32(0x7fffffffu));


    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_and_ps(diff, mask_sign));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_and_ps(diff, mask_sign));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_and_ps(diff, mask_sign));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_and_ps(diff, mask_sign));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        diff = _mm_sub_ps(v1, v2);
        sum  = _mm_add_ps(sum, _mm_and_ps(diff, mask_sign));
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
double L1NormSIMD(const double* pVect1, const double* pVect2, size_t qty) {
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

template float L1NormSIMD<float>(const float* pVect1, const float* pVect2, size_t qty);
template double L1NormSIMD<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * L2-norm.
 */

template <class T>
T L2NormStandard(const T *p1, const T *p2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      T diff = (p1[i]-p2[i]);
      sum += diff * diff;
    }

    return sqrt(sum);
}

template  float L2NormStandard<float>(const float *p1, const float *p2, size_t qty);
template  double L2NormStandard<double>(const double *p1, const double *p2, size_t qty);


template <class T>
T L2Norm(const T* pVect1, const T* pVect2, size_t qty) {
    T res = 0, diff = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    while (pVect1 < pEnd1) {
        diff = *pVect1++ - *pVect2++; res += diff * diff;
        diff = *pVect1++ - *pVect2++; res += diff * diff;
        diff = *pVect1++ - *pVect2++; res += diff * diff;
        diff = *pVect1++ - *pVect2++; res += diff * diff;
    }

    while (pVect1 < pEnd2) {
        diff = *pVect1++ - *pVect2++; res += diff * diff;
    }

    return sqrt(res);
}

template float  L2Norm<float>(const float* pVect1, const float* pVect2, size_t qty);
template double L2Norm<double>(const double* pVect1, const double* pVect2, size_t qty);

/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */

float L2SqrSIMD(const float* pVect1, const float* pVect2, size_t qty) {
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

template <> 
float L2NormSIMD(const float* pVect1, const float* pVect2, size_t qty) {
    float res = L2SqrSIMD(pVect1, pVect2, qty);
    return sqrt(res);
}

template <> 
double L2NormSIMD(const double* pVect1, const double* pVect2, size_t qty) {
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

template float  L2NormSIMD<float>(const float* pVect1, const float* pVect2, size_t qty);
template double L2NormSIMD<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 *  Itakura-Saito distance
 */

template <class T> 
T ItakuraSaito(const T* pVect1, const T* pVect2, size_t qty) {
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += pVect1[i]/pVect2[i]  - log(pVect1[i]/pVect2[i]) - 1;
    }

    return sum;
}

template float  ItakuraSaito<float>(const float* pVect1, const float* pVect2, size_t qty);
template double ItakuraSaito<double>(const double* pVect1, const double* pVect2, size_t qty);

template <class T> 
T ItakuraSaitoPrecomp(const T* pVect1, const T* pVect2, size_t qty) {
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

template float  ItakuraSaitoPrecomp<float>(const float* pVect1, const float* pVect2, size_t qty);
template double ItakuraSaitoPrecomp<double>(const double* pVect1, const double* pVect2, size_t qty);


/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */
template <>
float ItakuraSaitoPrecompSIMD(const float* pVect1, const float* pVect2, size_t qty)
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
double ItakuraSaitoPrecompSIMD(const double* pVect1, const double* pVect2, size_t qty)
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

template float  ItakuraSaitoPrecompSIMD<float>(const float* pVect1, const float* pVect2, size_t qty);
template double ItakuraSaitoPrecompSIMD<double>(const double* pVect1, const double* pVect2, size_t qty);


/*
 * KL-divergence
 */

template <class T> T KLStandard(const T *pVect1, const T *pVect2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += pVect1[i] * log(pVect1[i]/pVect2[i]);
    }

    return sum;
}

template float KLStandard<float>(const float *pVect1, const float *pVect2, size_t qty); 
template double KLStandard<double>(const double *pVect1, const double *pVect2, size_t qty);

template <class T> T KLStandardLogDiff(const T *p1, const T *p2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += p1[i] * (log(p1[i])-log(p2[i]));
    }

    return sum;
}

template float KLStandardLogDiff<float>(const float *p1, const float *p2, size_t qty);
template double KLStandardLogDiff<double>(const double *p1, const double *p2, size_t qty);


template <class T> 
T KLPrecomp(const T* pVect1, const T* pVect2, size_t qty) {
    T sum = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    const T* pVectLog1 = pVect1 + qty;
    const T* pVectLog2 = pVect2 + qty;

    while (pVect1 < pEnd1) {
        sum += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
        sum += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
        sum += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
        sum += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
    }

    while (pVect1 < pEnd2) {
        sum += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
    }

    return sum;
}

template float KLPrecomp<float>(const float* pVect1, const float* pVect2, size_t qty);
template double KLPrecomp<double>(const double* pVect1, const double* pVect2, size_t qty);

/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */
template <>
float KLPrecompSIMD(const float* pVect1, const float* pVect2, size_t qty)
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
double KLPrecompSIMD(const double* pVect1, const double* pVect2, size_t qty)
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

template float KLPrecompSIMD<float>(const float* pVect1, const float* pVect2, size_t qty);
template double KLPrecompSIMD<double>(const double* pVect1, const double* pVect2, size_t qty);

template <class T> T KLGeneralStandard(const T *pVect1, const T *pVect2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += pVect1[i] * log(pVect1[i]/pVect2[i]) + pVect2[i] - pVect1[i];
    }

    return sum;
}

/*
 * Generalized KL-divergence
 */

template float KLGeneralStandard<float>(const float *pVect1, const float *pVect2, size_t qty); 
template double KLGeneralStandard<double>(const double *pVect1, const double *pVect2, size_t qty);

template <class T> 
T KLGeneralPrecomp(const T* pVect1, const T* pVect2, size_t qty) {
    T sum = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    const T* pVectLog1 = pVect1 + qty;
    const T* pVectLog2 = pVect2 + qty;

    while (pVect1 < pEnd1) {
        sum += (*pVect1) * (*pVectLog1 - *pVectLog2) + *pVect2 - *pVect1; ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
        sum += (*pVect1) * (*pVectLog1 - *pVectLog2) + *pVect2 - *pVect1; ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
        sum += (*pVect1) * (*pVectLog1 - *pVectLog2) + *pVect2 - *pVect1; ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
        sum += (*pVect1) * (*pVectLog1 - *pVectLog2) + *pVect2 - *pVect1; ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
    }

    while (pVect1 < pEnd2) {
        sum += (*pVect1) * (*pVectLog1 - *pVectLog2) + *pVect2 - *pVect1; ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
    }

    return sum;
}

template float KLGeneralPrecomp<float>(const float* pVect1, const float* pVect2, size_t qty);
template double KLGeneralPrecomp<double>(const double* pVect1, const double* pVect2, size_t qty);

/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */
template <>
float KLGeneralPrecompSIMD(const float* pVect1, const float* pVect2, size_t qty)
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
double KLGeneralPrecompSIMD(const double* pVect1, const double* pVect2, size_t qty)
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

template float KLGeneralPrecompSIMD<float>(const float* pVect1, const float* pVect2, size_t qty);
template double KLGeneralPrecompSIMD<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * Jensen-Shannon divergence 
 */
template <class T> T JSStandard(const T *pVect1, const T *pVect2, size_t qty)
{
    T sum1 = 0, sum2 = 0;

    for (size_t i = 0; i < qty; i++) {
      T m = (pVect1[i] + pVect2[i])*0.5;
      if (m >= numeric_limits<T>::min()) {
        sum1 += pVect1[i] * log(pVect1[i]) + pVect2[i] * log(pVect2[i]);
        sum2 +=  m * log(m);
      }
    }

    return 0.5*sum1 - sum2;
}

template float JSStandard<float>(const float* pVect1, const float* pVect2, size_t qty);
template double JSStandard<double>(const double* pVect1, const double* pVect2, size_t qty);

template <class T>
T JSPrecomp(const T* pVect1, const T* pVect2, size_t qty) {
    T sum1 = 0, sum2 = 0;

    const T* pEnd2 = pVect1 + qty;

    const T* pVectLog1 = pVect1 + qty;
    const T* pVectLog2 = pVect2 + qty;

    while (pVect1 < pEnd2) {
        T m = 0.5*(*pVect1 + *pVect2);
        if (m >= numeric_limits<T>::min()) {
          sum1 += (*pVect1) * (*pVectLog1) + (*pVect2)*(*pVectLog2);
          sum2 += m * log(m);
        }
        pVect1++; pVect2++; pVectLog1++; pVectLog2++;
    }

    return 0.5*sum1 - sum2;
}

template float JSPrecomp<float>(const float* pVect1, const float* pVect2, size_t qty);
template double JSPrecomp<double>(const double* pVect1, const double* pVect2, size_t qty);

constexpr unsigned LogQty = 65536;

template <class T>
struct ApproxLogs {
  ApproxLogs() {
    for (unsigned i = 0; i <= LogQty; ++i) {
      T v = i * T(1.0) / LogQty;
      unsigned k = lapprox(v);
      LogTable[k] = log(1 + v);
    }
  }
  inline size_t lapprox(float f) { return floor(LogQty*f); }
  T LogTable[LogQty + 2];
};

ApproxLogs<float> ApproxLogsFloat;

constexpr float clog2() { return log(2.0f); }



}
