/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/nmslib/nmslib
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

#include "method/hnsw.h"
#include "method/hnsw_distfunc_opt_impl_inline.h"
#include "knnquery.h"
#include "ported_boost_progress.h"
#include "rangequery.h"

#include "portable_prefetch.h"
#include "space.h"

#include "sort_arr_bi.h"
#define MERGE_BUFFER_ALGO_SWITCH_THRESHOLD 100

#include <algorithm> // std::min
#include <limits>
#include <vector>

int LANE = 1;
#include "conv-c2d/emax7.h"
#include "conv-c2d/emax7lib.h"

//#define DIST_CALC
namespace similarity {


    template <typename dist_t>
    void
    Hnsw<dist_t>::SearchOld(KNNQuery<dist_t> *query, bool normalize)
    {
        float *pVectq = (float *)((char *)query->QueryObject()->data());
        TMP_RES_ARRAY(TmpRes);
        size_t qty = query->QueryObject()->datalength() >> 2;

        if (normalize) {
            NormalizeVect(pVectq, qty);
        }

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
                    PREFETCH(data_level0_memory_ + (*(data + j)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
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
            PREFETCH((char *)(massVisited + *(data + 1)), _MM_HINT_T0);
            PREFETCH((char *)(massVisited + *(data + 1) + 64), _MM_HINT_T0);
            PREFETCH(data_level0_memory_ + (*(data + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
            PREFETCH((char *)(data + 2), _MM_HINT_T0);

            for (int j = 1; j <= size; j++) {
                int tnum = *(data + j);
                PREFETCH((char *)(massVisited + *(data + j + 1)), _MM_HINT_T0);
                PREFETCH(data_level0_memory_ + (*(data + j + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
                if (!(massVisited[tnum] == currentV)) {
#ifdef DIST_CALC
                    query->distance_computations_++;
#endif
                    massVisited[tnum] = currentV;
                    char *currObj1 = (data_level0_memory_ + tnum * memoryPerObject_ + offsetData_);
                    dist_t d = (fstdistfunc_(pVectq, (float *)(currObj1 + 16), qty, TmpRes));
                    if (closestDistQueuei.top().getDistance() > d || closestDistQueuei.size() < ef_) {
                        candidateQueuei.emplace(-d, tnum);
                        PREFETCH(data_level0_memory_ + candidateQueuei.top().element * memoryPerObject_ + offsetLevel0_,
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
    Hnsw<dist_t>::SearchV1Merge(KNNQuery<dist_t> *query, bool normalize)
    {
        float *pVectq = (float *)((char *)query->QueryObject()->data());
        TMP_RES_ARRAY(TmpRes);
        size_t qty = query->QueryObject()->datalength() >> 2;

        if (normalize) {
            NormalizeVect(pVectq, qty);
        }

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
#define IMAX3
#ifdef IMAX3
#define IMAX_KERNEL_ROW_SIZE 60
#define IMAX_KERNEL_COL_SIZE 60
#define NCHIP 1
                int imax_emb = qty % IMAX_KERNEL_COL_SIZE ? ((qty/IMAX_KERNEL_COL_SIZE) + 1)*IMAX_KERNEL_COL_SIZE : qty;
                int imax_size = size % 2 ? ((size/2) + 1)*2 : size;
                Uint imax_key_array[size*imax_emb];
                Ull imax_query_array[imax_emb];
                Uint imax_result_array[imax_size];
                for (int j = 1; j <= imax_size; j++) {
                    int tnum = *(data + j);
                    for (int k = 0; k < imax_emb; k++) {
                        if (k < qty) {
                            imax_key_array[(j-1)*imax_size + k] = *(data_level0_memory_ + tnum * memoryPerObject_ + offsetData_ + 16 + k);
                        } else {
                            imax_key_array[(j-1)*imax_size + k] = 0;
                        }
                    }
                }
                for (int j = 0; j < imax_emb; j++) {
                    if (j < qty) {
                        ((Uint*)imax_query_array)[j] = pVectq[j];
                    } else {
                        ((Uint*)imax_query_array)[j] = 0;
                    }
                }

                Ull imax_key_array_addr[imax_emb * 4];
                for (int j = 0; j < imax_emb; j++) {
                    for (int k = 0; k < 4; k++) {
                        imax_key_array_addr[j] = (Ull)imax_key_array + (4 * imax_emb * j) + (k * 8);
                    }
                }

                Ull imax_result_array_addr[imax_size * 4];
                for (int j = 0; j < imax_size; j++) {
                    for (int k = 0; k < 4; k++) {
                        imax_result_array_addr[j] = (Ull)imax_result_array + (4 * imax_size * j) + (k * 8);
                    }
                }

                for (int j = 0; j < imax_size; j++) {
                    for (int k = 0; k < 4; k++) {
                        imax_result_array[j] = (Ull)imax_result_array + (4 * imax_size * j) + (k * 8);
                    }
                }
                Ull CHIP, LOLP, INIT0, INIT1, LOOP0, LOOP1;
                Ull cofs, rofs;
                Ull fetch_size = qty * 4;
                Ull rofs_init = (0-1*8LL)<<32|(0-1*4LL)&0xFFFFFFFF;
                Ull BR[64][4][4], AR[64][4];

#define mv1_core(r, rm1, b) \
                        mop(OP_LDR, 3, &BR[rm1][1][1], (Ull)imax_key_array_addr[b*4 + 0], (Ull)rofs, MSK_W1, (Ull)imax_key_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0); \
                        mop(OP_LDR, 3, &BR[rm1][1][0], (Ull)imax_key_array_addr[b*4 + 1], (Ull)rofs, MSK_W1, (Ull)imax_key_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0); \
                        mop(OP_LDR, 3, &BR[rm1][0][1], (Ull)imax_key_array_addr[b*4 + 2], (Ull)rofs, MSK_W1, (Ull)imax_key_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0); \
                        mop(OP_LDR, 3, &BR[rm1][0][0], (Ull)imax_key_array_addr[b*4 + 3], (Ull)rofs, MSK_W1, (Ull)imax_key_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0); \
                        exe(OP_FMA, &AR[r][3], BR[1][1][1], EXP_H3210, imax_query_array[b*4 + 0], EXP_H3210, AR[rm1][0], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                        exe(OP_FMA, &AR[r][2], BR[1][1][0], EXP_H3210, imax_query_array[b*4 + 1], EXP_H3210, AR[rm1][0], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                        exe(OP_FMA, &AR[r][1], BR[1][0][1], EXP_H3210, imax_query_array[b*4 + 2], EXP_H3210, AR[rm1][0], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                        exe(OP_FMA, &AR[r][0], BR[1][0][0], EXP_H3210, imax_query_array[b*4 + 3], EXP_H3210, AR[rm1][0], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL)

#define mv1_store(r, rm1) \
                        mop(OP_LDR, 3, &BR[rm1][1][1], (Ull)imax_result_array_addr[0], (Ull)rofs, MSK_W1, (Ull)imax_result_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0); \
                        mop(OP_LDR, 3, &BR[rm1][1][0], (Ull)imax_result_array_addr[1], (Ull)rofs, MSK_W1, (Ull)imax_result_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0); \
                        mop(OP_LDR, 3, &BR[rm1][0][1], (Ull)imax_result_array_addr[2], (Ull)rofs, MSK_W1, (Ull)imax_result_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0); \
                        mop(OP_LDR, 3, &BR[rm1][0][0], (Ull)imax_result_array_addr[3], (Ull)rofs, MSK_W1, (Ull)imax_result_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0); \
                        exe(OP_FMA, &AR[r][3], BR[rm1][1][1], EXP_H3210, AR[rm1][0], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                        exe(OP_FMA, &AR[r][2], BR[rm1][1][0], EXP_H3210, AR[rm1][1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                        exe(OP_FMA, &AR[r][1], BR[rm1][0][1], EXP_H3210, AR[rm1][2], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                        exe(OP_FMA, &AR[r][0], BR[rm1][0][0], EXP_H3210, AR[rm1][3], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                        mop(OP_STR, 3, &AR[r][0], (Ull)imax_result_array_addr[0], (Ull)rofs, MSK_D0, (Ull)imax_result_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0); \
                        mop(OP_STR, 3, &AR[r][1], (Ull)imax_result_array_addr[1], (Ull)rofs, MSK_D0, (Ull)imax_result_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0); \
                        mop(OP_STR, 3, &AR[r][2], (Ull)imax_result_array_addr[2], (Ull)rofs, MSK_D0, (Ull)imax_result_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0); \
                        mop(OP_STR, 3, &AR[r][3], (Ull)imax_result_array_addr[3], (Ull)rofs, MSK_D0, (Ull)imax_result_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0)

//EMAX%A begin mv1 mapdist=0
                for (CHIP=0;CHIP<NCHIP;CHIP++) {
                    for (INIT0=1,LOOP0=IMAX_KERNEL_ROW_SIZE/2,rofs=rofs_init;LOOP0--;INIT0=0) {
                        exe(OP_ADD, &rofs, rofs, EXP_H3210, (0-1*8LL)<<32|(0-1*4LL)&0xFFFFFFFF, EXP_H3210, 0LL, EXP_H3210, OP_AND, 0xFFFFFFFFFFFFFFFFLL, OP_NOP, 0LL);

                        mop(OP_LDR, 3, &BR[1][1][1], (Ull)imax_key_array_addr[0], (Ull)rofs, MSK_W1, (Ull)imax_key_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0);
                        mop(OP_LDR, 3, &BR[1][1][0], (Ull)imax_key_array_addr[1], (Ull)rofs, MSK_W1, (Ull)imax_key_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0);
                        mop(OP_LDR, 3, &BR[1][0][1], (Ull)imax_key_array_addr[2], (Ull)rofs, MSK_W1, (Ull)imax_key_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0);
                        mop(OP_LDR, 3, &BR[1][0][0], (Ull)imax_key_array_addr[3], (Ull)rofs, MSK_W1, (Ull)imax_key_array_addr[0], fetch_size, 0, 0, (Ull)NULL, 0);

                        exe(OP_FML, &AR[2][3], BR[1][1][1], EXP_H3210, imax_query_array[0], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                        exe(OP_FML, &AR[2][2], BR[1][1][0], EXP_H3210, imax_query_array[1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                        exe(OP_FML, &AR[2][1], BR[1][0][1], EXP_H3210, imax_query_array[2], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                        exe(OP_FML, &AR[2][0], BR[1][0][0], EXP_H3210, imax_query_array[3], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);

                        mv1_core( 3,  2,  1);mv1_core( 4,  3,  2);
                        mv1_core( 5,  4,  3);mv1_core( 6,  5,  4);mv1_core( 7,  6,  5);mv1_core( 8,  7,  6);mv1_core( 9,  8,  7);
                        mv1_core(10,  9,  8);mv1_core(11, 10,  9);mv1_core(12, 11, 10);mv1_core(13, 12, 11);mv1_core(14, 13, 12);
                        mv1_core(15, 14, 13);mv1_core(16, 15, 14);mv1_core(17, 16, 15);mv1_core(18, 17, 16);mv1_core(19, 18, 17);
                        mv1_core(20, 19, 18);mv1_core(21, 20, 19);mv1_core(22, 21, 20);mv1_core(23, 22, 21);mv1_core(24, 23, 22);
                        mv1_core(25, 24, 23);mv1_core(26, 25, 24);mv1_core(27, 26, 25);mv1_core(28, 27, 26);mv1_core(29, 28, 27);
                        mv1_core(30, 29, 28);mv1_core(31, 30, 29);mv1_core(32, 31, 30);mv1_core(33, 32, 31);mv1_core(34, 33, 32);
                        mv1_core(35, 34, 33);mv1_core(36, 35, 34);mv1_core(37, 36, 35);mv1_core(38, 37, 36);mv1_core(39, 38, 37);
                        mv1_core(40, 39, 38);mv1_core(41, 40, 39);mv1_core(42, 41, 40);mv1_core(43, 42, 41);mv1_core(44, 43, 42);
                        mv1_core(45, 44, 43);mv1_core(46, 45, 44);mv1_core(47, 46, 45);mv1_core(48, 47, 46);mv1_core(49, 48, 47);
                        mv1_core(50, 49, 48);mv1_core(51, 50, 49);mv1_core(52, 51, 50);mv1_core(53, 52, 51);mv1_core(54, 53, 52);
                        mv1_core(55, 54, 53);mv1_core(56, 55, 54);mv1_core(57, 56, 55);mv1_core(58, 57, 56);mv1_core(59, 58, 57);
                        mv1_core(60, 59, 58);mv1_core(61, 60, 59);

                        mv1_store(62, 61);
                    }
                }
//EMAX5A end
#else
                for (int j = 1; j <= size; j++) {
                    PREFETCH(data_level0_memory_ + (*(data + j)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
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
#endif
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
            PREFETCH((char *)(massVisited + *(data + 1)), _MM_HINT_T0);
            PREFETCH((char *)(massVisited + *(data + 1) + 64), _MM_HINT_T0);
            PREFETCH(data_level0_memory_ + (*(data + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
            PREFETCH((char *)(data + 2), _MM_HINT_T0);

            for (int j = 1; j <= size; j++) {
                int tnum = *(data + j);
                PREFETCH((char *)(massVisited + *(data + j + 1)), _MM_HINT_T0);
                PREFETCH(data_level0_memory_ + (*(data + j + 1)) * memoryPerObject_ + offsetData_, _MM_HINT_T0);
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
                PREFETCH(const_cast<const char *>(reinterpret_cast<char *>(&itemBuff[0])), _MM_HINT_T0);
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
                PREFETCH(data_level0_memory_ + sortedArr.top_item().data * memoryPerObject_ + offsetLevel0_, _MM_HINT_T0);
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
    template class Hnsw<int>;
}