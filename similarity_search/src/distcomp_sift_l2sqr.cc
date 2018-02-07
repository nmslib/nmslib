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
#include "distcomp.h"
#include "logging.h"
#include "utils.h"
#include "pow.h"
#include "portable_intrinsics.h"

#include <cstdlib>
#include <limits>
#include <algorithm>
#include <cmath>

namespace similarity {

using namespace std;

DistTypeSIFT l2SqrSIFTNaive(const uint8_t* pVect1,
                            const uint8_t* pVect2) {
  DistTypeSIFT res = 0;
  for (uint_fast32_t i = 0; i < SIFT_DIM; ++i) {
    DistTypeSIFT d = DistTypeSIFT(pVect1[i]) - DistTypeSIFT(pVect1[i]);
    res += d*d;
  }

  return res;
}

DistTypeSIFT l2SqrSIFTPrecomp(const uint8_t* pVect1,
                              const uint8_t* pVect2) {
  DistTypeSIFT sumProd = 0;
  for (uint_fast32_t i = 0; i < SIFT_DIM; ++i) {
    sumProd +=  DistTypeSIFT(pVect1[i]) * DistTypeSIFT(pVect2[i]);
  }

  return *reinterpret_cast<const DistTypeSIFT*>(pVect1 + SIFT_DIM) +
         *reinterpret_cast<const DistTypeSIFT*>(pVect2 + SIFT_DIM) - 2 * sumProd;
}

DistTypeSIFT l2SqrSIFTPrecompAVX(const uint8_t* pVect1,
                                 const uint8_t* pVect2) {
#ifndef PORTABLE_AVX
#pragma message WARN("l2_sqrt_sift_precomp_avx: AVX is not available, defaulting to pure C++ implementation!")
  return l2SqrSIFTPrecomp(pVect1, pVect2);
#else
  const unsigned dim = SIFT_DIM;

  DistTypeSIFT sumProd = 0;

  size_t sse_offset = (dim / 32) * 32;

  const __m256i* pStart1 = reinterpret_cast<const __m256i*>(pVect1);
  const __m256i* pStart2 = reinterpret_cast<const __m256i*>(pVect2);
  const __m256i* pEnd1   = reinterpret_cast<const __m256i*>(pVect1 + sse_offset);

  __m256i zero, x1, y1;
  zero = _mm256_xor_si256(zero,zero);
  __m256i sum = zero;

  int32_t PORTABLE_ALIGN32 unpack[8];

  while (pStart1 < pEnd1) {
    const __m256i x = _mm256_loadu_si256(pStart1++);
    const __m256i y = _mm256_loadu_si256(pStart2++);
    x1 = _mm256_unpackhi_epi8(x,zero);
    y1 = _mm256_unpackhi_epi8(y,zero);
    sum = _mm256_add_epi32(sum, _mm256_madd_epi16(x1, y1));
    x1 = _mm256_unpacklo_epi8(x,zero);
    y1 = _mm256_unpacklo_epi8(y,zero);
    sum = _mm256_add_epi32(sum, _mm256_madd_epi16(x1, y1));
  }
  _mm256_store_si256((__m256i *)unpack, sum);
  sumProd += unpack[0] + unpack[1] + unpack[2] + unpack[3] +
         unpack[4] + unpack[5] + unpack[6] + unpack[7];

  if (dim & 32) {
    for (uint_fast32_t i = sse_offset; i < dim; ++i) {
      sumProd += DistTypeSIFT(pVect1[i]) * DistTypeSIFT(pVect2[i]);
    }
  }

  return
      *reinterpret_cast<const DistTypeSIFT*>(pVect1+dim) +
      *reinterpret_cast<const DistTypeSIFT*>(pVect2+dim) - 2*sumProd;
#endif
}

}
