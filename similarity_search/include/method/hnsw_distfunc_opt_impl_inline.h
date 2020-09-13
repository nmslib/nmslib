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

inline float
L2Sqr16ExtSSE(const float *pVect1, const float *pVect2, size_t &qty, float * __restrict TmpRes) {
#ifdef PORTABLE_SSE2
  // size_t qty4 = qty >> 2;
  size_t qty16 = qty >> 4;

  const float *pEnd1 = pVect1 + (qty16 << 4);
  // const float* pEnd2 = pVect1 + (qty4 << 2);
  // const float* pEnd3 = pVect1 + qty;

  __m128 diff, v1, v2;
  __m128 sum = _mm_set1_ps(0);

  while (pVect1 < pEnd1) {
    //_mm_prefetch((char*)(pVect2 + 16), _MM_HINT_T0);
    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    diff = _mm_sub_ps(v1, v2);
    sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    diff = _mm_sub_ps(v1, v2);
    sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    diff = _mm_sub_ps(v1, v2);
    sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    diff = _mm_sub_ps(v1, v2);
    sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
  }

  _mm_store_ps(TmpRes, sum);
  float res = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

  return (res);
#else
  #pragma message WARN("L2Sqr16ExtSSE: SSE2 is not available")
  v = L2NormStandard(pVect1, pVect2, qty);
  return v * v;
#endif
}

inline float
L2Sqr16ExtAVX(const float *pVect1, const float *pVect2, size_t &qty, float * __restrict TmpRes) {
#ifdef PORTABLE_AVX
  size_t qty16 = qty >> 4;

  const float *pEnd1 = pVect1 + (qty16 << 4);

  __m256 diff, v1, v2;
  __m256 sum = _mm256_set1_ps(0);

  while (pVect1 < pEnd1) {
    //_mm_prefetch((char*)(pVect2 + 16), _MM_HINT_T0);
    v1 = _mm256_loadu_ps(pVect1);
    pVect1 += 8;
    v2 = _mm256_loadu_ps(pVect2);
    pVect2 += 8;
    diff = _mm256_sub_ps(v1, v2);
    sum = _mm256_add_ps(sum, _mm256_mul_ps(diff, diff));

    v1 = _mm256_loadu_ps(pVect1);
    pVect1 += 8;
    v2 = _mm256_loadu_ps(pVect2);
    pVect2 += 8;
    diff = _mm256_sub_ps(v1, v2);
    sum = _mm256_add_ps(sum, _mm256_mul_ps(diff, diff));
  }

  _mm256_store_ps(TmpRes, sum);
  float res = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3] + TmpRes[4] + TmpRes[5] + TmpRes[6] + TmpRes[7];

  return (res);
#else
  #pragma message WARN("L2Sqr16Ext: AVX is not available")
  return L2Sqr16ExtSSE(pVect1, pVect2, qty, TmpRes);
#endif
};


inline float
L2SqrExtSSE(const float *pVect1, const float *pVect2, size_t &qty, float * __restrict TmpRes) {
#ifdef PORTABLE_SSE2
  size_t qty4 = qty >> 2;
  size_t qty16 = qty >> 4;

  const float *pEnd1 = pVect1 + (qty16 << 4);
  const float *pEnd2 = pVect1 + (qty4 << 2);
  const float *pEnd3 = pVect1 + qty;

  __m128 diff, v1, v2;
  __m128 sum = _mm_set1_ps(0);

  while (pVect1 < pEnd1) {
    //_mm_prefetch((char*)(pVect2 + 16), _MM_HINT_T0);
    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    diff = _mm_sub_ps(v1, v2);
    sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    diff = _mm_sub_ps(v1, v2);
    sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    diff = _mm_sub_ps(v1, v2);
    sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));

    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    diff = _mm_sub_ps(v1, v2);
    sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
  }

  while (pVect1 < pEnd2) {
    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    diff = _mm_sub_ps(v1, v2);
    sum = _mm_add_ps(sum, _mm_mul_ps(diff, diff));
  }

  _mm_store_ps(TmpRes, sum);
  float res = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

  while (pVect1 < pEnd3) {
    float diff = *pVect1++ - *pVect2++;
    res += diff * diff;
  }

  return res;
#else
  #pragma message WARN("L2SqrExt: SSE2 is not available")
  v = L2NormStandard(pVect1, pVect2, qty);
  return v * v;
#endif
};

inline float
ScalarProductSSE(const float *__restrict pVect1, const float *__restrict pVect2, size_t qty,
                  float *__restrict TmpRes) {
#ifdef PORTABLE_SSE2
  size_t qty16 = qty / 16;
  size_t qty4 = qty / 4;

  const float *pEnd1 = pVect1 + 16 * qty16;
  const float *pEnd2 = pVect1 + 4 * qty4;
  const float *pEnd3 = pVect1 + qty;

  __m128 v1, v2;
  __m128 sum_prod = _mm_set1_ps(0);

  while (pVect1 < pEnd1) {
    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));

    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));

    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));

    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
  }

  while (pVect1 < pEnd2) {
    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
  }

  _mm_store_ps(TmpRes, sum_prod);
  float sum = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];
  while (pVect1 < pEnd3) {
    sum += (*pVect1) * (*pVect2);
    ++pVect1;
    ++pVect2;
  }
  return sum;
#else
  #pragma message WARN("ScalarProductSSE: SSE2 is not available")
  return ScalarProduct(pVect1, pVect2, qty);
#endif
}

inline float
ScalarProductAVX(const float *__restrict pVect1, const float *__restrict pVect2, size_t qty,
                  float *__restrict TmpRes) {
#ifdef PORTABLE_AVX
  size_t qty16 = qty / 16;
  size_t qty4 = qty / 4;

  const float *pEnd1 = pVect1 + 16 * qty16;
  const float *pEnd2 = pVect1 + 4 * qty4;
  const float *pEnd3 = pVect1 + qty;

  __m256 sum256 = _mm256_set1_ps(0);

  while (pVect1 < pEnd1) {
    //_mm_prefetch((char*)(pVect2 + 16), _MM_HINT_T0);

    __m256 v1 = _mm256_loadu_ps(pVect1);
    pVect1 += 8;
    __m256 v2 = _mm256_loadu_ps(pVect2);
    pVect2 += 8;
    sum256 = _mm256_add_ps(sum256, _mm256_mul_ps(v1, v2));

    v1 = _mm256_loadu_ps(pVect1);
    pVect1 += 8;
    v2 = _mm256_loadu_ps(pVect2);
    pVect2 += 8;
    sum256 = _mm256_add_ps(sum256, _mm256_mul_ps(v1, v2));
  }

  __m128 v1, v2;
  __m128 sum_prod = _mm_add_ps(_mm256_extractf128_ps(sum256, 0), _mm256_extractf128_ps(sum256, 1));

  while (pVect1 < pEnd2) {
    v1 = _mm_loadu_ps(pVect1);
    pVect1 += 4;
    v2 = _mm_loadu_ps(pVect2);
    pVect2 += 4;
    sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
  }

  _mm_store_ps(TmpRes, sum_prod);
  float sum = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];
  while (pVect1 < pEnd3) {
    sum += (*pVect1) * (*pVect2);
    ++pVect1;
    ++pVect2;
  }
  return sum;
#else
  return ScalarProductSSE(pVect1, pVect2, qty, TmpRes);
#endif
}


}
