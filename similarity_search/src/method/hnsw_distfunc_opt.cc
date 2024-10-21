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
#define EMAX7
#ifdef EMAX7
#define NULL ((void *)0)
#define IMAX_KERNEL_COL_SIZE 60
#define NCHIP 1
                int imax_emb = qty % IMAX_KERNEL_COL_SIZE ? ((qty/IMAX_KERNEL_COL_SIZE) + 1)*IMAX_KERNEL_COL_SIZE : qty;
                int imax_size = size % 8 ? ((size/8) + 1)*8 : size;
                float imax_key_array[imax_size*imax_emb];
                float imax_query_array[imax_emb];
                float imax_result_array[imax_size];
                for (int j = 1; j <= imax_size; j++) {
                    int tnum = *(data + j);
                    for (int k = 0; k < imax_emb; k++) {
                        if (k < qty) {
                            imax_key_array[(j-1)*imax_size + k] = *(data_level0_memory_ + tnum * memoryPerObject_ + offsetData_ + 16 + k);
                            imax_query_array[k] = pVectq[k];
                        } else {
                            imax_key_array[(j-1)*imax_size + k] = 0;
                            imax_query_array[k] = 0;
                        }
                    }
                }

                for (int emb_blk_idx = 0; emb_blk_idx < imax_emb / IMAX_KERNEL_COL_SIZE; emb_blk_idx++) {
                    Ull qaddr = (Ull)&imax_query_array[emb_blk_idx*IMAX_KERNEL_COL_SIZE];
                    Ull kaddr[IMAX_KERNEL_COL_SIZE * 4];
                    for (int j = 0; j < IMAX_KERNEL_COL_SIZE; j++) {
                        for (int k = 0; k < 4; k++) {
                            kaddr[(j*4) + k] = (Ull)&imax_key_array[(emb_blk_idx*IMAX_KERNEL_COL_SIZE*imax_size) + (j*imax_size) + k*2];
                        }
                    }

                    Ull raddr[4];
                    for (int j = 0; j < 4; j++) {
                        raddr[j] = (Ull)&imax_result_array[j*2];
                    }

                    Ull CHIP, LOLP, INIT0, INIT1, LOOP0, LOOP1;
                    Ull cofs, rofs;
                    Ull fetch_size = imax_size * 4;
                    Ull rofs_init = 0<<32|(0-4*8LL)&0xFFFFFFFF;
                    Ull BR[64][4][4], AR[64][4];

#define mv1_core(r, rm1, k0, k1, k2, k3, q) \
                            mop(OP_LDR, 3, &BR[rm1][0][1], (Ull)kaddr[k0], (Ull)rofs, MSK_W0, (Ull)kaddr[k0], fetch_size, 0, 0, (Ull)NULL, 0); \
                            mop(OP_LDR, 3, &BR[rm1][0][0], (Ull)kaddr[k1], (Ull)rofs, MSK_W0, (Ull)kaddr[k0], fetch_size, 0, 0, (Ull)NULL, 0); \
                            mop(OP_LDR, 3, &BR[rm1][1][1], (Ull)kaddr[k2], (Ull)rofs, MSK_W0, (Ull)kaddr[k0], fetch_size, 0, 0, (Ull)NULL, 0); \
                            mop(OP_LDR, 3, &BR[rm1][1][0], (Ull)kaddr[k3], (Ull)rofs, MSK_W0, (Ull)kaddr[k0], fetch_size, 0, 0, (Ull)NULL, 0); \
                            mop(OP_LDR, 3, &BR[rm1][2][1], (Ull)qaddr, q*8LL, MSK_W0, (Ull)qaddr, imax_emb, 0, 0, (Ull)NULL, 0); \
                            exe(OP_FMA, &AR[r][3], BR[rm1][0][1], EXP_H3210, BR[rm1][2][1], EXP_H3210, AR[rm1][3], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                            exe(OP_FMA, &AR[r][2], BR[rm1][0][0], EXP_H3210, BR[rm1][2][1], EXP_H3210, AR[rm1][2], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                            exe(OP_FMA, &AR[r][1], BR[rm1][1][1], EXP_H3210, BR[rm1][2][1], EXP_H3210, AR[rm1][1], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                            exe(OP_FMA, &AR[r][0], BR[rm1][1][0], EXP_H3210, BR[rm1][2][1], EXP_H3210, AR[rm1][0], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL)

#define mv1_store(r, rm1) \
                            mop(OP_LDR, 3, &BR[rm1][0][1], (Ull)raddr[0], (Ull)rofs, MSK_W0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, 0); \
                            mop(OP_LDR, 3, &BR[rm1][0][0], (Ull)raddr[1], (Ull)rofs, MSK_W0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, 0); \
                            mop(OP_LDR, 3, &BR[rm1][1][1], (Ull)raddr[2], (Ull)rofs, MSK_W0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, 0); \
                            mop(OP_LDR, 3, &BR[rm1][1][0], (Ull)raddr[3], (Ull)rofs, MSK_W0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, 0); \
                            exe(OP_FMA, &AR[r][3], BR[rm1][0][1], EXP_H3210, AR[rm1][3], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                            exe(OP_FMA, &AR[r][2], BR[rm1][0][0], EXP_H3210, AR[rm1][2], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                            exe(OP_FMA, &AR[r][1], BR[rm1][1][1], EXP_H3210, AR[rm1][1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                            exe(OP_FMA, &AR[r][0], BR[rm1][1][0], EXP_H3210, AR[rm1][0], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                            mop(OP_STR, 3, &AR[r][3], (Ull)raddr[0], (Ull)rofs, MSK_D0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, 0); \
                            mop(OP_STR, 3, &AR[r][2], (Ull)raddr[1], (Ull)rofs, MSK_D0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, 0); \
                            mop(OP_STR, 3, &AR[r][1], (Ull)raddr[2], (Ull)rofs, MSK_D0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, 0); \
                            mop(OP_STR, 3, &AR[r][0], (Ull)raddr[3], (Ull)rofs, MSK_D0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, 0)
//EMAX5A begin mv1 mapdist=0
                    for (CHIP=0;CHIP<NCHIP;CHIP++) {
                        for (INIT1=1,LOOP1=1,cofs=0;LOOP1--;INIT1=0) {
                            for (INIT0=1,LOOP0=imax_size/8,rofs=rofs_init;LOOP0--;INIT0=0) {
                                exe(OP_ADD, &rofs, INIT0?rofs:rofs, EXP_H3210, 0<<32|(4*8LL)&0xffffffff, EXP_H3210, 0LL, EXP_H3210, OP_AND, 0xffffffffffffffffLL, OP_NOP, 0LL);

                                mop(OP_LDR, 3, &BR[1][0][1], (Ull)kaddr[0], (Ull)rofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size);
                                mop(OP_LDR, 3, &BR[1][0][0], (Ull)kaddr[1], (Ull)rofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size);
                                mop(OP_LDR, 3, &BR[1][1][1], (Ull)kaddr[2], (Ull)rofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size);
                                mop(OP_LDR, 3, &BR[1][1][0], (Ull)kaddr[3], (Ull)rofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size);
                                mop(OP_LDR, 3, &BR[1][2][1], (Ull)qaddr,  (Ull)0LL,  MSK_W0, (Ull)qaddr,  imax_emb,   0, 0, (Ull)NULL, imax_emb);

                                exe(OP_FML, &AR[2][3], BR[1][0][1], EXP_H3210, BR[1][2][1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                                exe(OP_FML, &AR[2][2], BR[1][0][0], EXP_H3210, BR[1][2][1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                                exe(OP_FML, &AR[2][1], BR[1][0][1], EXP_H3210, BR[1][2][1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                                exe(OP_FML, &AR[2][0], BR[1][0][0], EXP_H3210, BR[1][2][1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);

                                mv1_core( 3,  2,  4,  5,  6,  7,  1);mv1_core( 4,  3,  8,  9, 11, 12,  2);
                                mv1_core( 5,  4, 13, 14, 15, 16,  3);mv1_core( 6,  5,  4, 17, 18, 19,  4);
                                mv1_core( 7,  6, 20, 21, 22, 23,  5);mv1_core( 8,  7, 24, 25, 26, 27,  6);
                                mv1_core( 9,  8, 28, 29, 30, 31,  7);mv1_core(10,  9, 32, 33, 34, 35,  8);
                                mv1_core(11, 10, 36, 37, 38, 39,  9);mv1_core(12, 11, 40, 41, 42, 43, 10);
                                mv1_core(13, 12, 44, 45, 46, 47, 11);mv1_core(14, 13, 48, 49, 50, 51, 12);
                                mv1_core(15, 14, 52, 53, 54, 55, 13);mv1_core(16, 15, 56, 57, 58, 59, 14);
                                mv1_core(17, 16, 60, 61, 62, 63, 15);mv1_core(18, 17, 64, 65, 66, 67, 16);
                                mv1_core(19, 18, 68, 69, 70, 71, 17);mv1_core(20, 19, 72, 73, 74, 75, 18);
                                mv1_core(21, 20, 76, 77, 78, 79, 19);mv1_core(22, 21, 80, 81, 82, 83, 20);
                                mv1_core(23, 22, 84, 85, 86, 87, 21);mv1_core(24, 23, 88, 89, 90, 91, 22);
                                mv1_core(25, 24, 92, 93, 94, 95, 23);mv1_core(26, 25, 96, 97, 98, 99, 24);
                                mv1_core(27, 26,100,101,102,103, 25);mv1_core(28, 27,104,105,106,107, 26);
                                mv1_core(29, 28,108,109,110,111, 27);mv1_core(30, 29,112,113,114,115, 28);
                                mv1_core(31, 30,116,117,118,119, 29);mv1_core(32, 31,120,121,122,123, 30);
                                mv1_core(33, 32,124,125,126,127, 31);mv1_core(34, 33,128,129,130,131, 32);
                                mv1_core(35, 34,132,133,134,135, 33);mv1_core(36, 35,136,137,138,139, 34);
                                mv1_core(37, 36,140,141,142,143, 35);mv1_core(38, 37,144,145,146,147, 36);
                                mv1_core(39, 38,148,149,150,151, 37);mv1_core(40, 39,152,153,154,155, 38);
                                mv1_core(41, 40,156,157,158,159, 39);mv1_core(42, 41,160,161,162,163, 40);
                                mv1_core(43, 42,164,165,166,167, 41);mv1_core(44, 43,168,169,170,171, 42);
                                mv1_core(45, 44,172,173,174,175, 43);mv1_core(46, 45,176,177,178,179, 44);
                                mv1_core(47, 46,180,181,182,183, 45);mv1_core(48, 47,184,185,186,187, 46);
                                mv1_core(49, 48,188,189,190,191, 47);mv1_core(50, 49,192,193,194,195, 48);
                                mv1_core(51, 50,196,197,198,199, 49);mv1_core(52, 51,200,201,202,203, 50);
                                mv1_core(53, 52,204,205,206,207, 51);mv1_core(54, 53,208,209,210,211, 52);
                                mv1_core(55, 54,212,213,214,215, 53);mv1_core(56, 55,216,217,218,219, 54);
                                mv1_core(57, 56,220,221,222,223, 55);mv1_core(58, 57,224,225,226,227, 56);
                                mv1_core(59, 58,228,229,230,231, 57);mv1_core(60, 59,232,233,234,235, 58);
                                mv1_core(61, 60,236,237,238,239, 59);

                                mv1_store(62, 61);
                            }
                        }
                    }
//EMAX5A end

                }
//EMAX5A drain_dirty_lmm

                float minDist = std::numeric_limits<float>::max();
                for (int j = 1; j <= size; j++) {
                    float result = -*(float*)&imax_result_array[j-1];
                    if (result < minDist) {
                        minDist = result;
                    }
                    if (minDist < curdist) {
                        curdist = minDist;
                        curNodeNum = *(data + j);
                        changed = true;
                    }
                }
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