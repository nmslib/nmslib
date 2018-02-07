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
* "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs" by Yu. A. Malkov, D. A. Yashunin
* This code was contributed by Yu. A. Malkov. It also was used in tests from the paper.
*
*
*/

#include "portable_simd.h"
#include "method/hnsw.h"
#include "knnquery.h"
#include "ported_boost_progress.h"
#include "rangequery.h"
#include "portable_intrinsics.h"
// This is only for _mm_prefetch
#include <mmintrin.h>
#include "space.h"

#include "sort_arr_bi.h"
#define MERGE_BUFFER_ALGO_SWITCH_THRESHOLD 100

#include <algorithm> // std::min
#include <limits>
#include <vector>

#ifdef __AVX__
#define USE_AVX
#endif

//#define DIST_CALC
namespace similarity {
    float
    L2SqrSIMD16Ext(const float *pVect1, const float *pVect2, size_t &qty, float *TmpRes)
    {
#ifdef USE_AVX
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
#endif
    };

    float
    L2SqrSIMDExt(const float *pVect1, const float *pVect2, size_t &qty, float *TmpRes)
    {
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

        return sqrt(res);
    };
    float
    ScalarProductSIMD(const float *__restrict pVect1, const float *__restrict pVect2, size_t qty, float *__restrict TmpRes)
    {
#ifdef USE_AVX
        size_t qty16 = qty / 16;
        size_t qty4 = qty / 4;

        const float *pEnd1 = pVect1 + 16 * qty16;
        const float *pEnd2 = pVect1 + 4 * qty4;

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
        ;
        return 1.0f - sum;
// return std::max(0.0f, 1 - std::max(float(-1), std::min(float(1), sum)));
#else
        size_t qty16 = qty / 16;
        size_t qty4 = qty / 4;

        const float *pEnd1 = pVect1 + 16 * qty16;
        const float *pEnd2 = pVect1 + 4 * qty4;

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

        return std::max(0.0f, 1 - std::max(float(-1), std::min(float(1), sum)));
#endif
    };

    float
    NormScalarProductSIMD(const float *pVect1, const float *pVect2, size_t &qty, float *TmpRes)
    {
        size_t qty16 = qty / 16;
        size_t qty4 = qty / 4;

        const float *pEnd1 = pVect1 + 16 * qty16;
        const float *pEnd2 = pVect1 + 4 * qty4;
        const float *pEnd3 = pVect1 + qty;

        __m128 v1, v2;
        __m128 sum_prod = _mm_set1_ps(0);
        __m128 sum_square1 = sum_prod;
        __m128 sum_square2 = sum_prod;

        while (pVect1 < pEnd1) {
            v1 = _mm_loadu_ps(pVect1);
            pVect1 += 4;
            v2 = _mm_loadu_ps(pVect2);
            pVect2 += 4;
            sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
            sum_square1 = _mm_add_ps(sum_square1, _mm_mul_ps(v1, v1));
            sum_square2 = _mm_add_ps(sum_square2, _mm_mul_ps(v2, v2));

            v1 = _mm_loadu_ps(pVect1);
            pVect1 += 4;
            v2 = _mm_loadu_ps(pVect2);
            pVect2 += 4;
            sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
            sum_square1 = _mm_add_ps(sum_square1, _mm_mul_ps(v1, v1));
            sum_square2 = _mm_add_ps(sum_square2, _mm_mul_ps(v2, v2));

            v1 = _mm_loadu_ps(pVect1);
            pVect1 += 4;
            v2 = _mm_loadu_ps(pVect2);
            pVect2 += 4;
            sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
            sum_square1 = _mm_add_ps(sum_square1, _mm_mul_ps(v1, v1));
            sum_square2 = _mm_add_ps(sum_square2, _mm_mul_ps(v2, v2));

            v1 = _mm_loadu_ps(pVect1);
            pVect1 += 4;
            v2 = _mm_loadu_ps(pVect2);
            pVect2 += 4;
            sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
            sum_square1 = _mm_add_ps(sum_square1, _mm_mul_ps(v1, v1));
            sum_square2 = _mm_add_ps(sum_square2, _mm_mul_ps(v2, v2));
        }

        while (pVect1 < pEnd2) {
            v1 = _mm_loadu_ps(pVect1);
            pVect1 += 4;
            v2 = _mm_loadu_ps(pVect2);
            pVect2 += 4;
            sum_prod = _mm_add_ps(sum_prod, _mm_mul_ps(v1, v2));
            sum_square1 = _mm_add_ps(sum_square1, _mm_mul_ps(v1, v1));
            sum_square2 = _mm_add_ps(sum_square2, _mm_mul_ps(v2, v2));
        }

        _mm_store_ps(TmpRes, sum_prod);
        float sum = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];
        _mm_store_ps(TmpRes, sum_square1);
        float norm1 = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];
        _mm_store_ps(TmpRes, sum_square2);
        float norm2 = TmpRes[0] + TmpRes[1] + TmpRes[2] + TmpRes[3];
        while (pVect1 < pEnd3) {
            sum += (*pVect1) * (*pVect2);
            norm1 += (*pVect1) * (*pVect1);
            norm2 += (*pVect2) * (*pVect2);

            ++pVect1;
            ++pVect2;
        }

        const float eps = numeric_limits<float>::min() * 2;

        if (norm1 < eps) { /*
                            * This shouldn't normally happen for this space, but
                            * if it does, we don't want to get NANs
                            */
            if (norm2 < eps)
                return 1;
            return 0;
        }
        return std::max(0.0f, 1 - std::max(float(-1), std::min(float(1), sum / sqrt(norm1 * norm2))));
    };

    /****************************************************************

    UNIVERSAL FUNCTION FOR CUSTOM DISTANCES

    ****************************************************************/
    template <typename dist_t>
    void
    Hnsw<dist_t>::SearchL2CustomOld(KNNQuery<dist_t> *query)
    {
        float *pVectq = (float *)((char *)query->QueryObject()->data());
        float PORTABLE_ALIGN32 TmpRes[8];
        size_t qty = query->QueryObject()->datalength() >> 2;

        VisitedList *vl = visitedlistpool->getFreeVisitedList();
        vl_type *massVisited = vl->mass;
        vl_type currentV = vl->curV;

        int maxlevel1 = maxlevel_;
        int curNodeNum = enterpointId_;
        dist_t curdist = (fstdistfunc_(
            pVectq, (float *)(data_level0_memory_ + enterpointId_ * memoryPerObject_ + offsetData_ + 16), qty, TmpRes));

        for (int i = maxlevel1; i > 0; i--) {
            bool changed = true;
            while (changed) {
                changed = false;
                int *data = (int *)(linkLists_[curNodeNum] + (maxM_ + 1) * (i - 1) * sizeof(int));
                int size = *data;
                for (int j = 1; j <= size; j++) {
                    _mm_prefetch(data_level0_memory_ + (*(data + j)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
                }
#ifdef DIST_CALC
                query->distance_computations_ += size;
#endif

                for (int j = 1; j <= size; j++) {
                    int tnum = *(data + j);

                    dist_t d = (fstdistfunc_(
                        pVectq, (float *)(data_level0_memory_ + tnum * memoryPerObject_ + offsetData_ + 16), qty, TmpRes));
                    if (d < curdist) {
                        curdist = d;
                        curNodeNum = tnum;
                        changed = true;
                    }
                }
            }
        }

        priority_queue<EvaluatedMSWNodeInt<dist_t>> candidateQueuei; // the set of elements which we can use to evaluate

        priority_queue<EvaluatedMSWNodeInt<dist_t>> closestDistQueuei; // The set of closest found elements
        // EvaluatedMSWNodeInt<dist_t> evi(curdist, curNodeNum);
        candidateQueuei.emplace(-curdist, curNodeNum);

        closestDistQueuei.emplace(curdist, curNodeNum);

        // query->CheckAndAddToResult(curdist, new Object(data_level0_memory_ + (curNodeNum)*memoryPerObject_ + offsetData_));
        query->CheckAndAddToResult(curdist, data_rearranged_[curNodeNum]);
        massVisited[curNodeNum] = currentV;

        while (!candidateQueuei.empty()) {
            EvaluatedMSWNodeInt<dist_t> currEv = candidateQueuei.top(); // This one was already compared to the query

            dist_t lowerBound = closestDistQueuei.top().getDistance();
            if ((-currEv.getDistance()) > lowerBound) {
                break;
            }

            candidateQueuei.pop();
            curNodeNum = currEv.element;
            int *data = (int *)(data_level0_memory_ + curNodeNum * memoryPerObject_ + offsetLevel0_);
            int size = *data;
            _mm_prefetch((char *)(massVisited + *(data + 1)), _MM_HINT_T0);
            _mm_prefetch((char *)(massVisited + *(data + 1) + 64), _MM_HINT_T0);
            _mm_prefetch(data_level0_memory_ + (*(data + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
            _mm_prefetch((char *)(data + 2), _MM_HINT_T0);

            for (int j = 1; j <= size; j++) {
                int tnum = *(data + j);
                _mm_prefetch((char *)(massVisited + *(data + j + 1)), _MM_HINT_T0);
                _mm_prefetch(data_level0_memory_ + (*(data + j + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
                if (!(massVisited[tnum] == currentV)) {
#ifdef DIST_CALC
                    query->distance_computations_++;
#endif
                    massVisited[tnum] = currentV;
                    char *currObj1 = (data_level0_memory_ + tnum * memoryPerObject_ + offsetData_);
                    dist_t d = (fstdistfunc_(pVectq, (float *)(currObj1 + 16), qty, TmpRes));
                    if (closestDistQueuei.top().getDistance() > d || closestDistQueuei.size() < ef_) {
                        candidateQueuei.emplace(-d, tnum);
                        _mm_prefetch(data_level0_memory_ + candidateQueuei.top().element * memoryPerObject_ + offsetLevel0_,
                                     _MM_HINT_T0);
                        // query->CheckAndAddToResult(d, new Object(currObj1));
                        query->CheckAndAddToResult(d, data_rearranged_[tnum]);
                        closestDistQueuei.emplace(d, tnum);

                        if (closestDistQueuei.size() > ef_) {
                            closestDistQueuei.pop();
                        }
                    }
                }
            }
        }
        visitedlistpool->releaseVisitedList(vl);
    }

    template <typename dist_t>
    void
    Hnsw<dist_t>::SearchL2CustomV1Merge(KNNQuery<dist_t> *query)
    {
        float *pVectq = (float *)((char *)query->QueryObject()->data());
        float PORTABLE_ALIGN32 TmpRes[8];
        size_t qty = query->QueryObject()->datalength() >> 2;

        VisitedList *vl = visitedlistpool->getFreeVisitedList();
        vl_type *massVisited = vl->mass;
        vl_type currentV = vl->curV;

        int maxlevel1 = maxlevel_;
        int curNodeNum = enterpointId_;
        dist_t curdist = (fstdistfunc_(
            pVectq, (float *)(data_level0_memory_ + enterpointId_ * memoryPerObject_ + offsetData_ + 16), qty, TmpRes));

        for (int i = maxlevel1; i > 0; i--) {
            bool changed = true;
            while (changed) {
                changed = false;
                int *data = (int *)(linkLists_[curNodeNum] + (maxM_ + 1) * (i - 1) * sizeof(int));
                int size = *data;
                for (int j = 1; j <= size; j++) {
                    _mm_prefetch(data_level0_memory_ + (*(data + j)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
                }
#ifdef DIST_CALC
                query->distance_computations_ += size;
#endif

                for (int j = 1; j <= size; j++) {
                    int tnum = *(data + j);

                    dist_t d = (fstdistfunc_(
                        pVectq, (float *)(data_level0_memory_ + tnum * memoryPerObject_ + offsetData_ + 16), qty, TmpRes));
                    if (d < curdist) {
                        curdist = d;
                        curNodeNum = tnum;
                        changed = true;
                    }
                }
            }
        }

        SortArrBI<dist_t, int> sortedArr(max<size_t>(ef_, query->GetK()));
        sortedArr.push_unsorted_grow(curdist, curNodeNum);

        int_fast32_t currElem = 0;

        typedef typename SortArrBI<dist_t, int>::Item QueueItem;
        vector<QueueItem> &queueData = sortedArr.get_data();
        vector<QueueItem> itemBuff(1 + max(maxM_, maxM0_));

        massVisited[curNodeNum] = currentV;

        while (currElem < min(sortedArr.size(), ef_)) {
            auto &e = queueData[currElem];
            CHECK(!e.used);
            e.used = true;
            curNodeNum = e.data;
            ++currElem;

            size_t itemQty = 0;
            dist_t topKey = sortedArr.top_key();

            int *data = (int *)(data_level0_memory_ + curNodeNum * memoryPerObject_ + offsetLevel0_);
            int size = *data;
            _mm_prefetch((char *)(massVisited + *(data + 1)), _MM_HINT_T0);
            _mm_prefetch((char *)(massVisited + *(data + 1) + 64), _MM_HINT_T0);
            _mm_prefetch(data_level0_memory_ + (*(data + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
            _mm_prefetch((char *)(data + 2), _MM_HINT_T0);

            for (int j = 1; j <= size; j++) {
                int tnum = *(data + j);
                _mm_prefetch((char *)(massVisited + *(data + j + 1)), _MM_HINT_T0);
                _mm_prefetch(data_level0_memory_ + (*(data + j + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
                if (!(massVisited[tnum] == currentV)) {
#ifdef DIST_CALC
                    query->distance_computations_++;
#endif
                    massVisited[tnum] = currentV;
                    char *currObj1 = (data_level0_memory_ + tnum * memoryPerObject_ + offsetData_);
                    dist_t d = (fstdistfunc_(pVectq, (float *)(currObj1 + 16), qty, TmpRes));

                    if (d < topKey || sortedArr.size() < ef_) {
                        CHECK_MSG(itemBuff.size() > itemQty,
                                  "Perhaps a bug: buffer size is not enough " + 
                                   ConvertToString(itemQty) + " >= " + ConvertToString(itemBuff.size()));
                        itemBuff[itemQty++] = QueueItem(d, tnum);
                    }
                }
            }
            if (itemQty) {
                _mm_prefetch(const_cast<const char *>(reinterpret_cast<char *>(&itemBuff[0])), _MM_HINT_T0);
                std::sort(itemBuff.begin(), itemBuff.begin() + itemQty);

                size_t insIndex = 0;
                if (itemQty > MERGE_BUFFER_ALGO_SWITCH_THRESHOLD) {
                    insIndex = sortedArr.merge_with_sorted_items(&itemBuff[0], itemQty);

                    if (insIndex < currElem) {
                        currElem = insIndex;
                    }
                } else {
                    for (size_t ii = 0; ii < itemQty; ++ii) {
                        size_t insIndex = sortedArr.push_or_replace_non_empty_exp(itemBuff[ii].key, itemBuff[ii].data);
                        if (insIndex < currElem) {
                            currElem = insIndex;
                        }
                    }
                }
                // because itemQty > 1, there would be at least item in sortedArr
                _mm_prefetch(data_level0_memory_ + sortedArr.top_item().data * memoryPerObject_ + offsetLevel0_, _MM_HINT_T0);
            }
            // To ensure that we either reach the end of the unexplored queue or currElem points to the first unused element
            while (currElem < sortedArr.size() && queueData[currElem].used == true)
                ++currElem;
        }

        for (int_fast32_t i = 0; i < query->GetK() && i < sortedArr.size(); ++i) {
            int tnum = queueData[i].data;
            // char *currObj = (data_level0_memory_ + tnum*memoryPerObject_ + offsetData_);
            // query->CheckAndAddToResult(queueData[i].key, new Object(currObj));
            query->CheckAndAddToResult(queueData[i].key, data_rearranged_[tnum]);
        }
        visitedlistpool->releaseVisitedList(vl);
    }

    /****************************************************************

    Search function for cosine

    ****************************************************************/
    template <typename dist_t>
    void
    Hnsw<dist_t>::SearchCosineNormalizedOld(KNNQuery<dist_t> *query)
    {
        float *pVectq = (float *)((char *)query->QueryObject()->data());
        float PORTABLE_ALIGN32 TmpRes[8];
        size_t qty = query->QueryObject()->datalength() >> 2;

        float *v = pVectq;
        float sum = 0;
        for (int i = 0; i < qty; i++) {
            sum += v[i] * v[i];
        }
        if (sum != 0.0) {
            sum = 1 / sqrt(sum);
            for (int i = 0; i < qty; i++) {
                v[i] *= sum;
            }
        }

        VisitedList *vl = visitedlistpool->getFreeVisitedList();
        vl_type *massVisited = vl->mass;
        vl_type currentV = vl->curV;

        int maxlevel1 = maxlevel_;
        int curNodeNum = enterpointId_;
        dist_t curdist = (ScalarProductSIMD(
            pVectq, (float *)(data_level0_memory_ + enterpointId_ * memoryPerObject_ + offsetData_ + 16), qty, TmpRes));

        for (int i = maxlevel1; i > 0; i--) {
            bool changed = true;
            while (changed) {
                changed = false;
                int *data = (int *)(linkLists_[curNodeNum] + (maxM_ + 1) * (i - 1) * sizeof(int));
                int size = *data;
                for (int j = 1; j <= size; j++) {
                    _mm_prefetch(data_level0_memory_ + (*(data + j)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
                }
#ifdef DIST_CALC
                query->distance_computations_ += size;
#endif

                for (int j = 1; j <= size; j++) {
                    int tnum = *(data + j);

                    dist_t d = (ScalarProductSIMD(
                        pVectq, (float *)(data_level0_memory_ + tnum * memoryPerObject_ + offsetData_ + 16), qty, TmpRes));
                    if (d < curdist) {
                        curdist = d;
                        curNodeNum = tnum;
                        changed = true;
                    }
                }
            }
        }

        priority_queue<EvaluatedMSWNodeInt<dist_t>> candidateQueuei; // the set of elements which we can use to evaluate

        priority_queue<EvaluatedMSWNodeInt<dist_t>> closestDistQueuei; // The set of closest found elements
        // EvaluatedMSWNodeInt<dist_t> evi(curdist, curNodeNum);
        candidateQueuei.emplace(-curdist, curNodeNum);

        closestDistQueuei.emplace(curdist, curNodeNum);

        // query->CheckAndAddToResult(curdist, new Object(data_level0_memory_ + (curNodeNum)*memoryPerObject_ + offsetData_));
        query->CheckAndAddToResult(curdist, data_rearranged_[curNodeNum]);
        massVisited[curNodeNum] = currentV;

        while (!candidateQueuei.empty()) {
            EvaluatedMSWNodeInt<dist_t> currEv = candidateQueuei.top(); // This one was already compared to the query

            dist_t lowerBound = closestDistQueuei.top().getDistance();
            if ((-currEv.getDistance()) > lowerBound) {
                break;
            }

            candidateQueuei.pop();
            curNodeNum = currEv.element;
            int *data = (int *)(data_level0_memory_ + curNodeNum * memoryPerObject_ + offsetLevel0_);
            int size = *data;
            _mm_prefetch((char *)(massVisited + *(data + 1)), _MM_HINT_T0);
            _mm_prefetch((char *)(massVisited + *(data + 1) + 64), _MM_HINT_T0);
            _mm_prefetch(data_level0_memory_ + (*(data + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
            _mm_prefetch((char *)(data + 2), _MM_HINT_T0);

            for (int j = 1; j <= size; j++) {
                int tnum = *(data + j);
                _mm_prefetch((char *)(massVisited + *(data + j + 1)), _MM_HINT_T0);
                _mm_prefetch(data_level0_memory_ + (*(data + j + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
                if (!(massVisited[tnum] == currentV)) {
#ifdef DIST_CALC
                    query->distance_computations_++;
#endif
                    massVisited[tnum] = currentV;
                    char *currObj1 = (data_level0_memory_ + tnum * memoryPerObject_ + offsetData_);
                    dist_t d = (ScalarProductSIMD(pVectq, (float *)(currObj1 + 16), qty, TmpRes));
                    if (closestDistQueuei.top().getDistance() > d || closestDistQueuei.size() < ef_) {
                        candidateQueuei.emplace(-d, tnum);
                        _mm_prefetch(data_level0_memory_ + candidateQueuei.top().element * memoryPerObject_ + offsetLevel0_,
                                     _MM_HINT_T0);
                        // query->CheckAndAddToResult(d, new Object(currObj1));
                        query->CheckAndAddToResult(d, data_rearranged_[tnum]);
                        closestDistQueuei.emplace(d, tnum);

                        if (closestDistQueuei.size() > ef_) {
                            closestDistQueuei.pop();
                        }
                    }
                }
            }
        }
        visitedlistpool->releaseVisitedList(vl);
    }

    template <typename dist_t>
    void
    Hnsw<dist_t>::SearchCosineNormalizedV1Merge(KNNQuery<dist_t> *query)
    {
        float *pVectq = (float *)((char *)query->QueryObject()->data());
        float PORTABLE_ALIGN32 TmpRes[8];
        size_t qty = query->QueryObject()->datalength() >> 2;

        float *v = pVectq;
        float sum = 0;
        for (int i = 0; i < qty; i++) {
            sum += v[i] * v[i];
        }
        if (sum != 0.0) {
            sum = 1 / sqrt(sum);
            for (int i = 0; i < qty; i++) {
                v[i] *= sum;
            }
        }

        VisitedList *vl = visitedlistpool->getFreeVisitedList();
        vl_type *massVisited = vl->mass;
        vl_type currentV = vl->curV;

        int maxlevel1 = maxlevel_;
        int curNodeNum = enterpointId_;
        dist_t curdist = (ScalarProductSIMD(
            pVectq, (float *)(data_level0_memory_ + enterpointId_ * memoryPerObject_ + offsetData_ + 16), qty, TmpRes));

        for (int i = maxlevel1; i > 0; i--) {
            bool changed = true;
            while (changed) {
                changed = false;
                int *data = (int *)(linkLists_[curNodeNum] + (maxM_ + 1) * (i - 1) * sizeof(int));
                int size = *data;
                for (int j = 1; j <= size; j++) {
                    _mm_prefetch(data_level0_memory_ + (*(data + j)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
                }
#ifdef DIST_CALC
                query->distance_computations_ += size;
#endif

                for (int j = 1; j <= size; j++) {
                    int tnum = *(data + j);

                    dist_t d = (ScalarProductSIMD(
                        pVectq, (float *)(data_level0_memory_ + tnum * memoryPerObject_ + offsetData_ + 16), qty, TmpRes));
                    if (d < curdist) {
                        curdist = d;
                        curNodeNum = tnum;
                        changed = true;
                    }
                }
            }
        }

        SortArrBI<dist_t, int> sortedArr(max<size_t>(ef_, query->GetK()));
        sortedArr.push_unsorted_grow(curdist, curNodeNum);

        int_fast32_t currElem = 0;

        typedef typename SortArrBI<dist_t, int>::Item QueueItem;
        vector<QueueItem> &queueData = sortedArr.get_data();
        vector<QueueItem> itemBuff(1 + max(maxM_, maxM0_));

        massVisited[curNodeNum] = currentV;

        while (currElem < min(sortedArr.size(), ef_)) {
            auto &e = queueData[currElem];
            CHECK(!e.used);
            e.used = true;
            curNodeNum = e.data;
            ++currElem;

            size_t itemQty = 0;
            dist_t topKey = sortedArr.top_key();

            int *data = (int *)(data_level0_memory_ + curNodeNum * memoryPerObject_ + offsetLevel0_);
            int size = *data;
            _mm_prefetch((char *)(massVisited + *(data + 1)), _MM_HINT_T0);
            _mm_prefetch((char *)(massVisited + *(data + 1) + 64), _MM_HINT_T0);
            _mm_prefetch(data_level0_memory_ + (*(data + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
            _mm_prefetch((char *)(data + 2), _MM_HINT_T0);

            for (int j = 1; j <= size; j++) {
                int tnum = *(data + j);
                _mm_prefetch((char *)(massVisited + *(data + j + 1)), _MM_HINT_T0);
                _mm_prefetch(data_level0_memory_ + (*(data + j + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
                if (!(massVisited[tnum] == currentV)) {
#ifdef DIST_CALC
                    query->distance_computations_++;
#endif
                    massVisited[tnum] = currentV;
                    char *currObj1 = (data_level0_memory_ + tnum * memoryPerObject_ + offsetData_);
                    dist_t d = (ScalarProductSIMD(pVectq, (float *)(currObj1 + 16), qty, TmpRes));

                    if (d < topKey || sortedArr.size() < ef_) {
                        CHECK_MSG(itemBuff.size() > itemQty,
                                  "Perhaps a bug: buffer size is not enough " + 
                                  ConvertToString(itemQty) + " >= " + ConvertToString(itemBuff.size()));
                        itemBuff[itemQty++] = QueueItem(d, tnum);
                    }
                }
            }

            if (itemQty) {
                _mm_prefetch(const_cast<const char *>(reinterpret_cast<char *>(&itemBuff[0])), _MM_HINT_T0);
                std::sort(itemBuff.begin(), itemBuff.begin() + itemQty);

                size_t insIndex = 0;
                if (itemQty > MERGE_BUFFER_ALGO_SWITCH_THRESHOLD) {
                    insIndex = sortedArr.merge_with_sorted_items(&itemBuff[0], itemQty);

                    if (insIndex < currElem) {
                        currElem = insIndex;
                    }
                } else {
                    for (size_t ii = 0; ii < itemQty; ++ii) {
                        size_t insIndex = sortedArr.push_or_replace_non_empty_exp(itemBuff[ii].key, itemBuff[ii].data);
                        if (insIndex < currElem) {
                            currElem = insIndex;
                        }
                    }
                }
                // because itemQty > 1, there would be at least item in sortedArr
                _mm_prefetch(data_level0_memory_ + sortedArr.top_item().data * memoryPerObject_ + offsetLevel0_, _MM_HINT_T0);
            }
            // To ensure that we either reach the end of the unexplored queue or currElem points to the first unused element
            while (currElem < sortedArr.size() && queueData[currElem].used == true)
                ++currElem;
        }

        for (int_fast32_t i = 0; i < query->GetK() && i < sortedArr.size(); ++i) {
            int tnum = queueData[i].data;
            // char *currObj = (data_level0_memory_ + tnum*memoryPerObject_ + offsetData_);
            // query->CheckAndAddToResult(queueData[i].key, new Object(currObj));
            query->CheckAndAddToResult(queueData[i].key, data_rearranged_[tnum]);
        }
        visitedlistpool->releaseVisitedList(vl);
    }

    template class Hnsw<float>;
    template class Hnsw<double>;
    template class Hnsw<int>;
}
