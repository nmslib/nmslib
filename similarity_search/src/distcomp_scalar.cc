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
#include "portable_intrinsics.h"
#include "distcomp.h"
#include "string.h"

#include <cstdlib>
#include <limits>
#include <algorithm>


namespace similarity {

using namespace std;
/*
 * Scalar-product (divided by Euclidean vector norms)
 */

template <class T>
T NormScalarProduct(const T *p1, const T *p2, size_t qty) 
{ 
    const T eps = numeric_limits<T>::min() * 2;

    T sum = 0;
    T norm1 = 0;
    T norm2 = 0;

    for (size_t i = 0; i < qty; i++) {
      norm1 += p1[i] * p1[i];
      norm2 += p2[i] * p2[i];
      sum += p1[i] * p2[i];
    }

    if (norm1 < eps) { /* 
                        * This shouldn't normally happen for this space, but 
                        * if it does, we don't want to get NANs 
                        */
      if (norm2 < eps) return 1;
      return 0;
    } 
    /* 
     * Sometimes due to rounding errors, we get values > 1 or < -1.
     * This throws off other functions that use scalar product, e.g., acos
     */
    return max(T(-1), min(T(1), sum / sqrt(norm1) / sqrt(norm2)));
}

template float  NormScalarProduct<float>(const float* pVect1, const float* pVect2, size_t qty);
template double NormScalarProduct<double>(const double* pVect1, const double* pVect2, size_t qty);

// Query is the second argument (by convention we use only left queries, where a data point is the left argument)
template <class T>
T QueryNormScalarProduct(const T *p1, const T *p2, size_t qty)
{
    const T eps = numeric_limits<T>::min() * 2;

    T sum = 0;
    T norm2 = 0;

    for (size_t i = 0; i < qty; i++) {
        norm2 += p2[i] * p2[i];
        sum += p1[i] * p2[i];
    }

    norm2 = max(norm2, eps);

    return sum / sqrt(norm2);
}

template float  QueryNormScalarProduct<float>(const float* pVect1, const float* pVect2, size_t qty);
template double QueryNormScalarProduct<double>(const double* pVect1, const double* pVect2, size_t qty);

template <> 
float NormScalarProductSIMD(const float* pVect1, const float* pVect2, size_t qty) {
#ifndef PORTABLE_SSE2
#pragma message WARN("ScalarProductSIMD<float>: SSE2 is not available, defaulting to pure C++ implementation!")
    return NormScalarProduct(pVect1, pVect2, qty);
#else
    size_t qty16  = qty/16;
    size_t qty4  = qty/4;

    const float* pEnd1 = pVect1 + 16  * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    __m128  v1, v2; 
    __m128  sum_prod = _mm_set1_ps(0); 
    __m128  sum_square1 = sum_prod;
    __m128  sum_square2 = sum_prod;

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        sum_prod  = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
        sum_square1  = _mm_add_ps(sum_square1, _mm_mul_ps(v1, v1));
        sum_square2  = _mm_add_ps(sum_square2, _mm_mul_ps(v2, v2));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        sum_prod  = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
        sum_square1  = _mm_add_ps(sum_square1, _mm_mul_ps(v1, v1));
        sum_square2  = _mm_add_ps(sum_square2, _mm_mul_ps(v2, v2));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        sum_prod  = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
        sum_square1  = _mm_add_ps(sum_square1, _mm_mul_ps(v1, v1));
        sum_square2  = _mm_add_ps(sum_square2, _mm_mul_ps(v2, v2));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        sum_prod  = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
        sum_square1  = _mm_add_ps(sum_square1, _mm_mul_ps(v1, v1));
        sum_square2  = _mm_add_ps(sum_square2, _mm_mul_ps(v2, v2));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        sum_prod  = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
        sum_square1  = _mm_add_ps(sum_square1, _mm_mul_ps(v1, v1));
        sum_square2  = _mm_add_ps(sum_square2, _mm_mul_ps(v2, v2));
    }

    float PORTABLE_ALIGN16 TmpResProd[4];
    float PORTABLE_ALIGN16 TmpResSquare1[4];
    float PORTABLE_ALIGN16 TmpResSquare2[4];

    _mm_store_ps(TmpResProd, sum_prod);
    float sum = TmpResProd[0] + TmpResProd[1] + TmpResProd[2] + TmpResProd[3];
    _mm_store_ps(TmpResSquare1, sum_square1);
    float norm1 = TmpResSquare1[0] + TmpResSquare1[1] + TmpResSquare1[2] + TmpResSquare1[3];
    _mm_store_ps(TmpResSquare2, sum_square2);
    float norm2 = TmpResSquare2[0] + TmpResSquare2[1] + TmpResSquare2[2] + TmpResSquare2[3];

    while (pVect1 < pEnd3) {
        sum += (*pVect1) * (*pVect2); 
        norm1 += (*pVect1) * (*pVect1); 
        norm2 += (*pVect2) * (*pVect2); 

        ++pVect1; ++pVect2;
    }

    const float eps = numeric_limits<float>::min() * 2;

    if (norm1 < eps) { /* 
                        * This shouldn't normally happen for this space, but 
                        * if it does, we don't want to get NANs 
                        */
      if (norm2 < eps) return 1;
      return 0;
    } 
    /* 
     * Sometimes due to rounding errors, we get values > 1 or < -1.
     * This throws off other functions that use scalar product, e.g., acos
     */
    return max(float(-1), min(float(1), sum / sqrt(norm1) / sqrt(norm2)));
#endif
}

template <> 
double NormScalarProductSIMD(const double* pVect1, const double* pVect2, size_t qty) {
#ifndef PORTABLE_SSE2
#pragma message WARN("NormScalarProductSIMD<double>: SSE2 is not available, defaulting to pure C++ implementation!")
    return NormScalarProduct(pVect1, pVect2, qty);
#else
    size_t qty8 = qty/8;
    size_t qty2 = qty/2;

    const double* pEnd1 = pVect1 + 8 * qty8;
    const double* pEnd2 = pVect1 + 2 * qty2;
    const double* pEnd3 = pVect1 + qty;

    __m128d  v1, v2; 
    __m128d  sum_prod = _mm_set1_pd(0); 
    __m128d  sum_square1 = sum_prod;
    __m128d  sum_square2 = sum_prod;

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        sum_prod  = _mm_add_pd(sum_prod, _mm_mul_pd(v1, v2));
        sum_square1 = _mm_add_pd(sum_square1, _mm_mul_pd(v1, v1));
        sum_square2 = _mm_add_pd(sum_square2, _mm_mul_pd(v2, v2));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        sum_prod  = _mm_add_pd(sum_prod, _mm_mul_pd(v1, v2));
        sum_square1 = _mm_add_pd(sum_square1, _mm_mul_pd(v1, v1));
        sum_square2 = _mm_add_pd(sum_square2, _mm_mul_pd(v2, v2));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        sum_prod  = _mm_add_pd(sum_prod, _mm_mul_pd(v1, v2));
        sum_square1 = _mm_add_pd(sum_square1, _mm_mul_pd(v1, v1));
        sum_square2 = _mm_add_pd(sum_square2, _mm_mul_pd(v2, v2));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        sum_prod  = _mm_add_pd(sum_prod, _mm_mul_pd(v1, v2));
        sum_square1 = _mm_add_pd(sum_square1, _mm_mul_pd(v1, v1));
        sum_square2 = _mm_add_pd(sum_square2, _mm_mul_pd(v2, v2));

    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        sum_prod  = _mm_add_pd(sum_prod, _mm_mul_pd(v1, v2));
        sum_square1 = _mm_add_pd(sum_square1, _mm_mul_pd(v1, v1));
        sum_square2 = _mm_add_pd(sum_square2, _mm_mul_pd(v2, v2));

    }

    double PORTABLE_ALIGN16 TmpResProd[2];
    double PORTABLE_ALIGN16 TmpResSquare1[2];
    double PORTABLE_ALIGN16 TmpResSquare2[2];

    _mm_store_pd(TmpResProd, sum_prod);
    double sum = TmpResProd[0] + TmpResProd[1];

    _mm_store_pd(TmpResSquare1, sum_square1);
    double norm1 = TmpResSquare1[0] + TmpResSquare1[1];

    _mm_store_pd(TmpResSquare2, sum_square2);
    double norm2 = TmpResSquare2[0] + TmpResSquare2[1];

    while (pVect1 < pEnd3) {
        sum += (*pVect1) * (*pVect2); 
        norm1 += (*pVect1) * (*pVect1); 
        norm2 += (*pVect2) * (*pVect2); 

        ++pVect1; ++pVect2;
    }

    const double eps = numeric_limits<double>::min() * 2;

    if (norm1 < eps) { /* 
                        * This shouldn't normally happen for this space, but 
                        * if it does, we don't want to get NANs 
                        */
      if (norm2 < eps) return 1;
      return 0;
    } 
    /* 
     * Sometimes due to rounding errors, we get values > 1 or < -1.
     * This throws off other functions that use scalar product, e.g., acos
     */
    return max(double(-1), min(double(1), sum / sqrt(norm1) / sqrt(norm2)));
#endif
}

template float   NormScalarProductSIMD<float>(const float* pVect1, const float* pVect2, size_t qty);
template double  NormScalarProductSIMD<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * Scalar products that are not normalized.
 */

template <class T>
T ScalarProduct(const T *p1, const T *p2, size_t qty) { 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += p1[i] * p2[i];
    }
    return sum;
}

template float  ScalarProduct<float>(const float* pVect1, const float* pVect2, size_t qty);
template double ScalarProduct<double>(const double* pVect1, const double* pVect2, size_t qty);

/* 
 * On new architectures unaligned loads are almost as fast as aligned ones. 
 * Ensuring that both pVect1 and pVect2 are similarly aligned could be hard.
 */

template <> 
float ScalarProductSIMD(const float* pVect1, const float* pVect2, size_t qty) {
#ifndef PORTABLE_SSE2
#pragma message WARN("ScalarProductSIMD<float>: SSE2 is not available, defaulting to pure C++ implementation!")
    return ScalarProduct(pVect1, pVect2, qty);
#else
    size_t qty16  = qty/16;
    size_t qty4  = qty/4;

    const float* pEnd1 = pVect1 + 16  * qty16;
    const float* pEnd2 = pVect1 + 4  * qty4;
    const float* pEnd3 = pVect1 + qty;

    __m128  v1, v2; 
    __m128  sum = _mm_set1_ps(0); 

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        sum  = _mm_add_ps(sum, _mm_mul_ps(v1, v2));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        sum  = _mm_add_ps(sum, _mm_mul_ps(v1, v2));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        sum  = _mm_add_ps(sum, _mm_mul_ps(v1, v2));

        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        sum  = _mm_add_ps(sum, _mm_mul_ps(v1, v2));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_ps(pVect1); pVect1 += 4;
        v2   = _mm_loadu_ps(pVect2); pVect2 += 4;
        sum  = _mm_add_ps(sum, _mm_mul_ps(v1, v2));
    }

    float PORTABLE_ALIGN16 TmpRes[4];

    _mm_store_ps(TmpRes, sum);
    float res = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    while (pVect1 < pEnd3) {
        res += (*pVect1) * (*pVect2); 
        ++pVect1; ++pVect2;
    }

    return res;
#endif
}

template <> 
double ScalarProductSIMD(const double* pVect1, const double* pVect2, size_t qty) {
#ifndef PORTABLE_SSE2
#pragma message WARN("ScalarProductSIMD<double>: SSE2 is not available, defaulting to pure C++ implementation!")
    return ScalarProduct(pVect1, pVect2, qty);
#else
    size_t qty8 = qty/8;
    size_t qty2 = qty/2;

    const double* pEnd1 = pVect1 + 8 * qty8;
    const double* pEnd2 = pVect1 + 2 * qty2;
    const double* pEnd3 = pVect1 + qty;

    __m128d  v1, v2; 
    __m128d  sum = _mm_set1_pd(0); 

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        sum  = _mm_add_pd(sum, _mm_mul_pd(v1, v2));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        sum  = _mm_add_pd(sum, _mm_mul_pd(v1, v2));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        sum  = _mm_add_pd(sum, _mm_mul_pd(v1, v2));

        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        sum  = _mm_add_pd(sum, _mm_mul_pd(v1, v2));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_pd(pVect1); pVect1 += 2;
        v2   = _mm_loadu_pd(pVect2); pVect2 += 2;
        sum  = _mm_add_pd(sum, _mm_mul_pd(v1, v2));
    }

    double PORTABLE_ALIGN16 TmpRes[2];

    _mm_store_pd(TmpRes, sum);
    double res= TmpRes[0] + TmpRes[1];

    while (pVect1 < pEnd3) {
        res += (*pVect1) * (*pVect2); 
        ++pVect1; ++pVect2;
    }

    return res;
#endif
}

template float   ScalarProductSIMD<float>(const float* pVect1, const float* pVect2, size_t qty);
template double  ScalarProductSIMD<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * Angular distance (a proper metric)
 *
 */

template <class T>
T AngularDistance(const T *p1, const T *p2, size_t qty) 
{ 
    return acos(NormScalarProductSIMD(p1, p2, qty));
}

template float  AngularDistance<float>(const float* pVect1, const float* pVect2, size_t qty);
template double AngularDistance<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * Cosine similarity (not exactly a metric)
 *
 */

template <class T>
T CosineSimilarity(const T *p1, const T *p2, size_t qty) 
{ 
    return std::max(T(0), 1 - NormScalarProductSIMD(p1, p2, qty));
}

template float  CosineSimilarity<float>(const float* pVect1, const float* pVect2, size_t qty);
template double CosineSimilarity<double>(const double* pVect1, const double* pVect2, size_t qty);

}
