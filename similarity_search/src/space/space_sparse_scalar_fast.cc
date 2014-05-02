/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
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

#include <cmath>
#include <fstream>
#include <string>
#include <sstream>

#include "space_sparse_scalar_fast.h"
#include "logging.h"
#include "experimentconf.h"


namespace similarity {

#ifdef __SSE4_2__
#include <immintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>

const static __m128i shuffle_mask16[16] = {
  _mm_set_epi8(-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127),
  _mm_set_epi8(-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,3,2,1,0),
  _mm_set_epi8(-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,7,6,5,4),
  _mm_set_epi8(-127,-127,-127,-127,-127,-127,-127,-127,7,6,5,4,3,2,1,0),
  _mm_set_epi8(-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,11,10,9,8),
  _mm_set_epi8(-127,-127,-127,-127,-127,-127,-127,-127,11,10,9,8,3,2,1,0),
  _mm_set_epi8(-127,-127,-127,-127,-127,-127,-127,-127,11,10,9,8,7,6,5,4),
  _mm_set_epi8(-127,-127,-127,-127,11,10,9,8,7,6,5,4,3,2,1,0),
  _mm_set_epi8(-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,-127,15,14,13,12),
  _mm_set_epi8(-127,-127,-127,-127,-127,-127,-127,-127,15,14,13,12,3,2,1,0),
  _mm_set_epi8(-127,-127,-127,-127,-127,-127,-127,-127,15,14,13,12,7,6,5,4),
  _mm_set_epi8(-127,-127,-127,-127,15,14,13,12,7,6,5,4,3,2,1,0),
  _mm_set_epi8(-127,-127,-127,-127,-127,-127,-127,-127,15,14,13,12,11,10,9,8),
  _mm_set_epi8(-127,-127,-127,-127,15,14,13,12,11,10,9,8,3,2,1,0),
  _mm_set_epi8(-127,-127,-127,-127,15,14,13,12,11,10,9,8,7,6,5,4),
  _mm_set_epi8(15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0),
};

#endif


/*
 * The efficient SIMD intersection is based on the code of Daniel Lemire (lemire.me). 
 *
 * Lemire's code implemented an algorithm similar to one described in:
 *
 * Schlegel, Benjamin, Thomas Willhalm, and Wolfgang Lehner. 
 * "Fast sorted-set intersection using simd instructions." ADMS Workshop, Seattle, WA, USA. 2011.
 *
 * The code was adapted, simplified, and modified by Leo as follows:
 *
 * 1) The original algorithm was used to obtain only IDs.
 *    to extract both IDs and respective floating-point values,
 *    we need to call _mm_cmpistrm twice. 
 * 2) During partitioning we scale ids so that 
 *    none of them is a multiple of 65536.
 *    The latter trick allows us to employ a slightly faster
 *    version of the _mm_cmpistrm.
 *
 */
float ScalarProjectFast(const char* pData1, size_t len1,
                        const char* pData2, size_t len2) {
  float           norm1 = 0, norm2 = 0;
  size_t          blockQty1 = 0, blockQty2 = 0;
  const size_t*   pBlockQtys1 = NULL; 
  const size_t*   pBlockQtys2 = NULL; 
  const size_t*   pBlockOffs1 = NULL; 
  const size_t*   pBlockOffs2 = NULL; 
  const char*     pBlockBeg1 = NULL; 
  const char*     pBlockBeg2 = NULL;

  ParseSparseElementHeader(pData1, blockQty1, norm1, pBlockQtys1, pBlockOffs1, pBlockBeg1);
  ParseSparseElementHeader(pData2, blockQty2, norm2, pBlockQtys2, pBlockOffs2, pBlockBeg2);

  float sum = 0;

  size_t bid1 = 0, bid2 = 0; // block ids

  size_t elemSize = 2 + sizeof(float);

  while (bid1 < blockQty1 && bid2 < blockQty2) {
    if (pBlockOffs1[bid1] == pBlockOffs2[bid2]) {
      size_t qty1    = pBlockQtys1[bid1];
      const uint16_t* pBlockIds1 = reinterpret_cast<const uint16_t*>(pBlockBeg1);
      const float*    pBlockVals1 = reinterpret_cast<const float*>(pBlockIds1 + qty1);

      size_t qty2    = pBlockQtys2[bid2];
      const uint16_t* pBlockIds2 = reinterpret_cast<const uint16_t*>(pBlockBeg2);
      const float*    pBlockVals2 = reinterpret_cast<const float*>(pBlockIds2 + qty2);

      size_t mx = max(qty1, qty2);

      float val1[mx];
      float val2[mx];
  
      float* pVal1 = val1;
      float* pVal2 = val2;

      size_t i1 = 0, i2 = 0;
      size_t iEnd1 = qty1 / 8 * 8; 
      size_t iEnd2 = qty2 / 8 * 8; 

      if (i1 < iEnd1 && i2 < iEnd2) {
        while (pBlockIds1[i1 + 7] < pBlockIds2[i2]) {
          i1 += 8;
          if (i1 >= iEnd1) goto scalar_inter; 
        }
        while (pBlockIds2[i2 + 7] < pBlockIds1[i1]) {
          i2 += 8;
          if (i2 >= iEnd2) goto scalar_inter; 
        }
        __m128i id1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&pBlockIds1[i1]));
        __m128i id2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&pBlockIds2[i2]));

        while (true) {
          __m128i cmpRes = _mm_cmpistrm(id2, id1,
                                        _SIDD_UWORD_OPS | 
                                        _SIDD_CMP_EQUAL_ANY | 
                                        _SIDD_BIT_MASK);

          int r = _mm_extract_epi32(cmpRes, 0);

          if (r) {

            int r1 = r & 15;
            __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&pBlockVals1[i1]));
            __m128  vs = (__m128)_mm_shuffle_epi8(v, shuffle_mask16[r1]);
            _mm_storeu_ps(pVal1, vs);
            pVal1 += _mm_popcnt_u32(r1);

            int r2 = (r >> 4) & 15;
            v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&pBlockVals1[i1+4]));
            vs = (__m128)_mm_shuffle_epi8(v, shuffle_mask16[r2]);
            _mm_storeu_ps(pVal1, vs);
            pVal1 += _mm_popcnt_u32(r2);

            cmpRes = _mm_cmpistrm(id1, id2,
                                _SIDD_UWORD_OPS | 
                                _SIDD_CMP_EQUAL_ANY | 
                                _SIDD_BIT_MASK);
            r = _mm_extract_epi32(cmpRes, 0);

            r1 = r & 15;

            v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&pBlockVals2[i2]));
            vs = (__m128)_mm_shuffle_epi8(v, shuffle_mask16[r1]);
            _mm_storeu_ps(pVal2, vs);
            pVal2 += _mm_popcnt_u32(r1);

            r2 = (r >> 4) & 15;
            v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&pBlockVals2[i2+4]));
            vs = (__m128)_mm_shuffle_epi8(v, shuffle_mask16[r2]);
            _mm_storeu_ps(pVal2, vs);
            pVal2 += _mm_popcnt_u32(r2);
          }

          const uint16_t id1max = pBlockIds1[i1 + 7];
          if (id1max <= pBlockIds2[i2 + 7]) {
            i1 += 8;
            if (i1 >= iEnd1) goto scalar_inter; 
            id1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&pBlockIds1[i1]));
          }
          if (id1max >= pBlockIds2[i2 + 7]) {
            i2 += 8;
            if (i2 >= iEnd2) goto scalar_inter; 
            id2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(&pBlockIds2[i2]));
          }
        }
      }

    scalar_inter:
      while (i1 < qty1 && i2 < qty2) {
        if (pBlockIds1[i1] == pBlockIds2[i2]) {
          *pVal1++ = pBlockVals1[i1]; 
          *pVal2++ = pBlockVals2[i2];
          ++i1;
          ++i2;
        } else if (pBlockIds1[i1] < pBlockIds2[i2]) {
          ++i1;
        } else {
          ++i2;
        } 
      }

      pBlockBeg1 += elemSize * pBlockQtys1[bid1++];
      pBlockBeg2 += elemSize * pBlockQtys2[bid2++];



      ssize_t resQty = pVal1 - val1;

      CHECK(resQty == pVal2 - val2);

      ssize_t resQty4 = resQty /4 * 4; 

      if (resQty4) {
        __m128 sum128 = _mm_set1_ps(0);

        for (ssize_t k = 0; k < resQty4; k += 4) {
          sum128 = _mm_add_ps(sum128,
                              _mm_mul_ps(_mm_loadu_ps(val1 + k),
                                         _mm_loadu_ps(val2 + k)));
        }

        sum += _mm_extract_ps(sum128, 0);
        sum += _mm_extract_ps(sum128, 1);
        sum += _mm_extract_ps(sum128, 2);
        sum += _mm_extract_ps(sum128, 3);
      }

      for (ssize_t k = resQty4 ; k < resQty; ++k)
        sum += val1[k] * val2[k];
    } else if (pBlockOffs1[bid1] < pBlockOffs2[bid2]) {
      pBlockBeg1 += elemSize * pBlockQtys1[bid1++];
    } else {
      pBlockBeg2 += elemSize * pBlockQtys2[bid2++];
    }
  }

  // These two loops are necessary to carry out size checks below
  while (bid1 < blockQty1) {
    pBlockBeg1 += elemSize * pBlockQtys1[bid1++];
  }

  while (bid2 < blockQty2) {
    pBlockBeg2 += elemSize * pBlockQtys2[bid2++];
  }

  CHECK(pBlockBeg1 - pData1 == (ssize_t)len1);
  CHECK(pBlockBeg2 - pData2 == (ssize_t)len2);

 /* 
  * Sometimes due to rounding errors, we get values > 1 or < -1.
  * This throws off other functions that use scalar product, e.g., acos
  */

  return max(float(-1), min(float(1), sum / sqrt(norm1) / sqrt(norm2)));
}

float 
SpaceSparseCosineSimilarityFast::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj2->datalength() > 0);

  float val = 1 - ScalarProjectFast(obj1->data(), obj1->datalength(),
                                    obj2->data(), obj2->datalength());

  return max(val, float(0));
}

float 
SpaceSparseAngularDistanceFast::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj2->datalength() > 0);

  return acos(ScalarProjectFast(obj1->data(), obj1->datalength(),
                                obj2->data(), obj2->datalength()));
}


}  // namespace similarity
