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

#ifndef _PERMUTATION_UTILS_H_
#define _PERMUTATION_UTILS_H_

#include <algorithm>
#include <iostream>
#include <unordered_set>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "perm_type.h"

#ifndef __SSE2__
// Of course, this would work only for Intel
#error "Need SSE2 to compile! Did you add -msse2 to the compiler options?"
#endif
#ifndef __SSE4_1__
#error "Need SSE4.1 to compile! Did you add -msse4.1 to the compiler options?"
#endif
#ifndef __SSSE3__
#error "Need SSSE3 to compile! Did you add -mssse3 to the compiler options?"
#endif

#include <immintrin.h>


/* 
 * SpearmanFootrule and SpearmanRho require advanced SIMD extensions.
 */

#include <smmintrin.h>
#include <tmmintrin.h>



namespace similarity {

using std::max;

template <typename dist_t>
using DistInt = std::pair<dist_t, PivotIdType>;     // <dist,      pivot_id>

typedef std::pair<PivotIdType, size_t> IntInt;      // <perm-dist, object_id>

template <typename dist_t>
void GetPermutationPivot(const ObjectVector& data,
                         const Space<dist_t>* space,
                         const size_t num_pivot,
                         ObjectVector* pivot) {
  CHECK(num_pivot < data.size());
  std::unordered_set<int> pivot_idx;
  for (size_t i = 0; i < num_pivot; ++i) {
    int p = RandomInt() % data.size();
    while (pivot_idx.count(p) != 0) {
      p = RandomInt() % data.size();
    }
    pivot_idx.insert(p);
    pivot->push_back(data[p]);
  }
}

template <typename dist_t>
void GetPermutation(const ObjectVector& pivot, const Space<dist_t>* space,
                    const Object* object, Permutation* p) {
  // get pivot id
  std::vector<DistInt<dist_t>> dists;
  for (size_t i = 0; i < pivot.size(); ++i) {
    dists.push_back(std::make_pair(space->IndexTimeDistance(pivot[i], object), static_cast<PivotIdType>(i)));
  }
  std::sort(dists.begin(), dists.end());
  // dists.second = pivot id    i.e.  \Pi_o(i)

  // get pivot id's pos         i.e. position in \Pi_o(i) = \Pi^{-1}(i)
  std::vector<IntInt> pivot_idx;
  for (size_t i = 0; i < pivot.size(); ++i) {
    pivot_idx.push_back(std::make_pair(dists[i].second, static_cast<PivotIdType>(i)));
  }
  std::sort(pivot_idx.begin(), pivot_idx.end());
  // pivot_idx.second = pos of pivot  (which is needed for computing the Rho func)
  for (size_t i = 0; i < pivot.size(); ++i) {
    p->push_back(pivot_idx[i].second);
  }
}

template <template<typename> class QueryType, typename dist_t>
void GenPermutation(const ObjectVector& pivot,
                    QueryType<dist_t>* query,
                    Permutation* p) {
  std::vector<DistInt<dist_t>> dists;
  for (size_t i = 0; i < pivot.size(); ++i) {
    /* Distance can be asymmetric, pivot should be always on the left side */
    dists.push_back(std::make_pair(query->DistanceObjLeft(pivot[i]),
                                   static_cast<PivotIdType>(i)));
  }
  std::sort(dists.begin(), dists.end());
  std::vector<IntInt> pivot_idx;
  for (size_t i = 0; i < pivot.size(); ++i) {
    pivot_idx.push_back(std::make_pair(dists[i].second,
                                       static_cast<PivotIdType>(i)));
  }
  std::sort(pivot_idx.begin(), pivot_idx.end());
  for (size_t i = 0; i < pivot.size(); ++i) {
    p->push_back(pivot_idx[i].second);
  }
}

template <typename dist_t>
void GetPermutation(
    const ObjectVector& pivot, RangeQuery<dist_t>* query, Permutation* p) {
  GenPermutation(pivot, query, p);
}

template <typename dist_t>
void GetPermutation(
    const ObjectVector& pivot, KNNQuery<dist_t>* query, Permutation* p) {
  GenPermutation(pivot, query, p);
}

#ifdef __SSE4_1__
#ifdef __SSSE3__

/* 
 * SpearmanFootrule and SpearmanRho require advanced SIMD extensions.
 */

inline PivotIdType SpearmanFootruleSIMD(const int32_t* pVect1, const int32_t* pVect2, size_t qty) {
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const int32_t* pEnd1 = pVect1 + 16 * qty16;
    const int32_t* pEnd2 = pVect1 + 4  * qty4;
    const int32_t* pEnd3 = pVect1 + qty;

    __m128i  diff, v1, v2;
    __m128i  sum = _mm_set1_epi32(0);

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect1)); pVect1 += 4;
        v2   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect2)); pVect2 += 4;
        diff = _mm_sub_epi32(v1, v2);
        sum  = _mm_add_epi32(sum, _mm_max_epi32(_mm_sub_epi32(_mm_setzero_si128(), diff), diff));

        v1   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect1)); pVect1 += 4;
        v2   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect2)); pVect2 += 4;
        diff = _mm_sub_epi32(v1, v2);
        sum  = _mm_add_epi32(sum, _mm_max_epi32(_mm_sub_epi32(_mm_setzero_si128(), diff), diff));

        v1   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect1)); pVect1 += 4;
        v2   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect2)); pVect2 += 4;
        diff = _mm_sub_epi32(v1, v2);
        sum  = _mm_add_epi32(sum, _mm_max_epi32(_mm_sub_epi32(_mm_setzero_si128(), diff), diff));

        v1   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect1)); pVect1 += 4;
        v2   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect2)); pVect2 += 4;
        diff = _mm_sub_epi32(v1, v2);
        sum  = _mm_add_epi32(sum, _mm_max_epi32(_mm_sub_epi32(_mm_setzero_si128(), diff), diff));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect1)); pVect1 += 4;
        v2   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect2)); pVect2 += 4;
        diff = _mm_sub_epi32(v1, v2);
        sum  = _mm_add_epi32(sum, _mm_max_epi32(_mm_sub_epi32(_mm_setzero_si128(), diff), diff));

    }

    int32_t __attribute__((aligned(16))) TmpRes[4];

    _mm_store_si128(reinterpret_cast<__m128i*>(TmpRes), sum);
    int res= TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    while (pVect1 < pEnd3) {
        res += abs(*pVect1++ - *pVect2++);
    }

    return res;
}

inline int32_t SpearmanRhoSIMD(const int32_t* pVect1, const int32_t* pVect2, size_t qty) {
    size_t qty4  = qty/4;
    size_t qty16 = qty/16;

    const int32_t* pEnd1 = pVect1 + 16 * qty16;
    const int32_t* pEnd2 = pVect1 + 4  * qty4;
    const int32_t* pEnd3 = pVect1 + qty;

    __m128i  diff, v1, v2;
    __m128i  sum = _mm_set1_epi16(0);

    while (pVect1 < pEnd1) {
        v1   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect1)); pVect1 += 4;
        v2   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect2)); pVect2 += 4;
        diff = _mm_sub_epi32(v1, v2);
        sum  = _mm_add_epi32(sum, _mm_mullo_epi32(diff, diff));

        v1   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect1)); pVect1 += 4;
        v2   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect2)); pVect2 += 4;
        diff = _mm_sub_epi32(v1, v2);
        sum  = _mm_add_epi32(sum, _mm_mullo_epi32(diff, diff));

        v1   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect1)); pVect1 += 4;
        v2   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect2)); pVect2 += 4;
        diff = _mm_sub_epi32(v1, v2);
        sum  = _mm_add_epi32(sum, _mm_mullo_epi32(diff, diff));

        v1   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect1)); pVect1 += 4;
        v2   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect2)); pVect2 += 4;
        diff = _mm_sub_epi32(v1, v2);
        sum  = _mm_add_epi32(sum, _mm_mullo_epi32(diff, diff));
    }

    while (pVect1 < pEnd2) {
        v1   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect1)); pVect1 += 4;
        v2   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pVect2)); pVect2 += 4;
        diff = _mm_sub_epi32(v1, v2);
        sum  = _mm_add_epi32(sum, _mm_mullo_epi32(diff, diff));
    }

    int32_t __attribute__((aligned(16))) TmpRes[8];

    _mm_store_si128(reinterpret_cast<__m128i*>(TmpRes), sum);
    int32_t res= TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];

    while (pVect1 < pEnd3) {
        res += *pVect1++ * *pVect2++;
    }

    return res;
}

#endif
#endif

inline PivotIdType SpearmanFootrule(const PivotIdType* x, const PivotIdType* y, size_t qty) {
  int res = 0;
  int diff;
  for (size_t i = 0; i < qty; ++i) {
    diff = x[i] >= y[i] ? x[i] - y[i] : y[i] - x[i];
    res += diff;
  }
  return res;
}


inline PivotIdType SpearmanRho(const PivotIdType* x, const PivotIdType* y, size_t qty) {
  PivotIdType res = 0;
  PivotIdType diff;
  size_t i = 0;
  for (; i < qty/4*4; ) {
    diff = y[i] - x[i]; res += diff * diff;++i;
    diff = y[i] - x[i]; res += diff * diff;++i;
    diff = y[i] - x[i]; res += diff * diff;++i;
    diff = y[i] - x[i]; res += diff * diff;++i;
  }
  for (; i < qty; ++i) {
    //diff = x[i] >= y[i] ? x[i] - y[i] : y[i] - x[i];
    diff = y[i] - x[i];
    res += diff * diff;
  }
  return res;
}

// Permutation Prefix Index

template <typename dist_t>
void GetPermutationPPIndex(const ObjectVector& pivot,
                           const Space<dist_t>* space,
                           const Object* object,
                           Permutation* p) {
  // get pivot id
  std::vector<DistInt<dist_t>> dists;
  for (size_t i = 0; i < pivot.size(); ++i) {
    dists.push_back(std::make_pair(space->IndexTimeDistance(pivot[i], object),
                                   static_cast<PivotIdType>(i)));
  }
  std::sort(dists.begin(), dists.end());
  // dists.second = pivot id    i.e.  \Pi_o(i)

  for (size_t i = 0; i < pivot.size(); ++i) {
    p->push_back(dists[i].second);
  }
}

template <template<typename> class QueryType, typename dist_t>
void GetPermutationPPIndex(const ObjectVector& pivot,
                           QueryType<dist_t>* query,
                           Permutation* p) {
  std::vector<DistInt<dist_t>> dists;
  for (size_t i = 0; i < pivot.size(); ++i) {
    /* Distance can be asymmetric, pivot should be always on the left side */
    dists.push_back(std::make_pair(query->DistanceObjLeft(pivot[i]),
                                   static_cast<PivotIdType>(i)));
  }
  std::sort(dists.begin(), dists.end());
  for (size_t i = 0; i < pivot.size(); ++i) {
    p->push_back(dists[i].second);
  }
}

}  // namespace similarity

#endif     // _PERMUTATION_UTILS_H_

