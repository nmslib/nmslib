/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
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

#include <algorithm>
#include <sstream>
#include <thread>
#include <memory>
#include <fstream>
#include <thread>
#include <limits>
#include <unordered_map>

// This is only for _mm_prefetch
#include <mmintrin.h>

#include "portable_simd.h"
#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "incremental_quick_select.h"
#include "method/pivot_neighb_horder_closepiv_invindx.h"
#include "utils.h"
#include "ztimer.h"

#include "falconn_heap_mod.h"

#define SCALE_MIN_TIMES true

// This include is used for store-and-sort merging method only
#include <boost/sort/spreadsort/integer_sort.hpp>

namespace similarity {

using std::vector;
using std::pair;
using std::mutex;
using std::unique_lock;


template <typename dist_t>
struct IndexThreadParamsHorderClosePivPNII {
  PivotNeighbHorderClosePivInvIndex<dist_t>&           index_;
  size_t                                      chunk_qty_;
  size_t                                      index_every_;
  size_t                                      out_of_;
  ProgressDisplay*                            progress_bar_;
  mutex&                                      display_mutex_;

  IndexThreadParamsHorderClosePivPNII(
                     PivotNeighbHorderClosePivInvIndex<dist_t>&  index,
                     size_t                             chunk_qty,
                     size_t                             index_every,
                     size_t                             out_of,
                     ProgressDisplay*                   progress_bar,
                     mutex&                             display_mutex
                      ) :
                     index_(index),
                     chunk_qty_(chunk_qty),
                     index_every_(index_every),
                     out_of_(out_of),
                     progress_bar_(progress_bar),
                     display_mutex_(display_mutex)
                     { }
};

template <typename dist_t>
struct IndexThreadHOrderHorderClosePivPNII {
  void operator()(IndexThreadParamsHorderClosePivPNII<dist_t>& prm) {
#ifdef UINT16_IDS
    CHECK(prm.chunk_qty_ <= UINT16_ID_MAX);
#endif
    for (size_t i = 0; i < prm.chunk_qty_; ++i) {
      if (prm.index_every_ == i % prm.out_of_) {
        prm.index_.IndexChunk(i, prm.progress_bar_, prm.display_mutex_);
      }
    }
  }
};

template <typename dist_t>
PivotNeighbHorderClosePivInvIndex<dist_t>::PivotNeighbHorderClosePivInvIndex(
    bool  PrintProgress,
    const Space<dist_t>& space,
    const ObjectVector& data) 
      : Index<dist_t>(data),  
        space_(space), 
        PrintProgress_(PrintProgress),
        recreate_points_(false),
        disable_pivot_index_(false) {
}


template <typename dist_t>
void PivotNeighbHorderClosePivInvIndex<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);


  pmgr.GetParamOptional("numPivot", num_pivot_, 512);

  if (pmgr.hasParam("numPivotIndex") && pmgr.hasParam("numPrefix")) {
    throw runtime_error("One shouldn't specify both parameters numPrefix and numPivotIndex, b/c they are synonyms!");
  }
  pmgr.GetParamOptional("numPivotIndex", num_prefix_, 32);
  pmgr.GetParamOptional("numPrefix",     num_prefix_, num_prefix_);

  pmgr.GetParamOptional("chunkIndexSize", chunk_index_size_, 65536);
#ifdef UINT16_IDS
  CHECK_MSG(chunk_index_size_ <= UINT16_ID_MAX, "For this build/mode, the chunkIndexSize should be <= " + ConvertToString(UINT16_ID_MAX));
#endif

  pmgr.GetParamOptional("indexThreadQty", index_thread_qty_,  thread::hardware_concurrency());
  pmgr.GetParamOptional("recreatePoints", recreate_points_,  false);
  pmgr.GetParamOptional("disablePivotIndex", disable_pivot_index_, false);
  pmgr.GetParamOptional("hashTrickDim", hash_trick_dim_, 0);

  if (num_prefix_ > num_pivot_) {
    PREPARE_RUNTIME_ERR(err) << METH_PIVOT_NEIGHB_HORDER_CLOSEPIV_INVINDEX << " requires that numPrefix (" << num_prefix_ << ") "
                             << "should be <= numPivot (" << num_pivot_ << ")";
    THROW_RUNTIME_ERR(err);
  }

  CHECK(num_prefix_ <= num_pivot_);

  pmgr.GetParamOptional("pivotFile", pivot_file_, "");
  pmgr.GetParamOptional("pivotFracInv",  pivotFracInv_, 1);
  pmgr.GetParamOptional("pivotCombQty", pivot_comb_qty_, 2); // we use pairs by default
  pmgr.GetParamOptional("printPivotStat", print_pivot_stat_, 0);

  CHECK_MSG(pivot_comb_qty_ == 2,
            "Illegal number of pivots in the combinations " + ConvertToString(pivot_comb_qty_) + " must be 2");
  
  size_t indexQty = (this->data_.size() + chunk_index_size_ - 1) / chunk_index_size_;

  pmgr.CheckUnused();
  this->ResetQueryTimeParams();

  LOG(LIB_INFO) << "# of entries in an index chunk  = " << chunk_index_size_;
  LOG(LIB_INFO) << "# of index chunks             = " << indexQty;
  LOG(LIB_INFO) << "# of indexing thread          = " << index_thread_qty_;
  LOG(LIB_INFO) << "# pivotFile                   = " << pivot_file_;
  LOG(LIB_INFO) << "# pivots                      = " << num_pivot_;
  LOG(LIB_INFO) << "# pivots to index (numPrefix) = " << num_prefix_;
  LOG(LIB_INFO) << "# hash trick dimensionionality= " << hash_trick_dim_;
  LOG(LIB_INFO) << "Do we recreate points during indexing when computing distances to pivots?  = " << recreate_points_;
  LOG(LIB_INFO) << "# of pivots to combine        = " << pivot_comb_qty_;
  LOG(LIB_INFO) << "# pivotFracInv                = " << pivotFracInv_;
  LOG(LIB_INFO) << "Do we print pivot stat?       = " << print_pivot_stat_;

  if (pivot_file_.empty())
    GetPermutationPivot(this->data_, space_, num_pivot_, &pivot_, &pivot_pos_);
  else {
    vector<string> vExternIds;
    space_.ReadDataset(pivot_, vExternIds, pivot_file_, num_pivot_);
    if (pivot_.size() < num_pivot_) {
      throw runtime_error("Not enough pivots in the file '" + pivot_file_ + "'");
    }
    genPivot_ = pivot_;
  }
  // Attempt to create an efficient pivot index, after pivots are loaded/created
  initPivotIndex();

  /*
   * After we allocated each chunks posting lists hash,
   * it is thread-safe to index each chunk separately.
   */
  posting_lists_.resize(indexQty);

  for (size_t chunkId = 0; chunkId < indexQty; ++chunkId) {
    posting_lists_[chunkId].reset(new unordered_map<IdType,PostingListHorderType>());
  }

  // Don't need more thread than you have chunks
  index_thread_qty_ = min(index_thread_qty_, indexQty);

  mutex                                               progressBarMutex;

  if (index_thread_qty_ <= 1) {
    unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                                new ProgressDisplay(this->data_.size(), cerr)
                                :NULL);
    for (size_t chunkId = 0; chunkId < indexQty; ++chunkId) {
      IndexChunk(chunkId, progress_bar.get(), progressBarMutex);
    }

    if (progress_bar) {
      (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
    }
  } else {
    vector<thread>                                      threads(index_thread_qty_);
    vector<shared_ptr<IndexThreadParamsHorderClosePivPNII<dist_t>>>   threadParams;

    LOG(LIB_INFO) << "Will create " << index_thread_qty_ << " indexing threads";;

    unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                                new ProgressDisplay(this->data_.size(), cerr)
                                :NULL);

    for (size_t i = 0; i < index_thread_qty_; ++i) {
      threadParams.push_back(shared_ptr<IndexThreadParamsHorderClosePivPNII<dist_t>>(
                              new IndexThreadParamsHorderClosePivPNII<dist_t>(*this, indexQty, i, index_thread_qty_,
                                                               progress_bar.get(), progressBarMutex)));
    }

    for (size_t i = 0; i < index_thread_qty_; ++i) {
      threads[i] = thread(IndexThreadHOrderHorderClosePivPNII<dist_t>(), ref(*threadParams[i]));
    }

    for (size_t i = 0; i < index_thread_qty_; ++i) {
      threads[i].join();
    }

    if (progress_bar) {
      (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
    }
  }
}

template <typename dist_t>
void PivotNeighbHorderClosePivInvIndex<dist_t>::GetPermutationPPIndexEfficiently(const Object* pObj, Permutation& p) const {
  vector<dist_t> vDst;

  pivot_index_->ComputePivotDistancesIndexTime(pObj, vDst);
  GetPermutationPPIndexEfficiently(p, vDst);
}

template <typename dist_t>
void PivotNeighbHorderClosePivInvIndex<dist_t>::GetPermutationPPIndexEfficiently(const Query<dist_t>* pQuery, Permutation& p) const {
  vector<dist_t> vDst;

  pivot_index_->ComputePivotDistancesQueryTime(pQuery, vDst);
  GetPermutationPPIndexEfficiently(p, vDst);
}

template <typename dist_t>
void PivotNeighbHorderClosePivInvIndex<dist_t>::GetPermutationPPIndexEfficiently(Permutation &p, const vector <dist_t> &vDst) const {
  vector<DistInt<dist_t>> dists;
  p.clear();

  for (size_t i = 0; i < pivot_.size(); ++i) {
    dists.push_back(std::make_pair(vDst[i], static_cast<PivotIdType>(i)));
  }
  sort(dists.begin(), dists.end());
  // dists.second = pivot id    i.e.  \Pi_o(i)

  for (size_t i = 0; i < pivot_.size(); ++i) {
    p.push_back(dists[i].second);
  }
}

template <typename dist_t>
void 
PivotNeighbHorderClosePivInvIndex<dist_t>::IndexChunk(size_t chunkId, ProgressDisplay* progress_bar, mutex& display_mutex) {
  size_t minId = chunkId * chunk_index_size_;
  size_t maxId = min(this->data_.size(), minId + chunk_index_size_);


  unordered_map<IdType, PostingListHorderType> & chunkPostLists = *posting_lists_[chunkId];

  vector<uint32_t > combIds;
  combIds.reserve((size_t)1 + num_prefix_ * (num_prefix_ - 1) / 2 / pivotFracInv_);

  //LOG(LIB_INFO) << "Chunk id: " << chunkId << " total # of posting lists: " << maxPostQty;
 
  string externId;

  CHECK(num_prefix_ > 0);

  for (IdTypeUnsign objId = 0; objId < maxId - minId; ++objId) {
    Permutation perm;
    const Object* pObj = this->data_[minId + objId];

    unique_ptr<Object> extObj;
    if (recreate_points_) {
      extObj=space_.CreateObjFromStr(-1, -1, space_.CreateStrFromObj(pObj, externId), NULL);
      pObj=extObj.get();
    }

#ifdef UINT16_IDS
    CHECK(id < UINT16_ID_MAX);
#endif

    GetPermutationPPIndexEfficiently(pObj, perm);

    size_t qty = genPivotCombIds(combIds, perm, num_prefix_);

    for (size_t i = 0; i < qty; ++i) {
      uint32_t id = combIds[i];
      PostingListHorderType& post = getPostingList(chunkPostLists, id);
      post.push_back(objId);
    }
        
    if (objId % 1000) {
      unique_lock<mutex> lock(display_mutex);
      if (progress_bar) ++(*progress_bar);
    }
  }

  // Sorting is essential for merging algos
  for (auto & p:chunkPostLists) {
    sort(p.second.begin(), p.second.end());
  }
}
    

    
    
template <typename dist_t>
void 
PivotNeighbHorderClosePivInvIndex<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);
  string inv_proc_alg;
  
  pmgr.GetParamOptional("skipChecking", skip_checking_, false);
  pmgr.GetParamOptional("invProcAlg",   inv_proc_alg,   PERM_PROC_FAST_SCAN);
  //pmgr.GetParamOptional("invProcAlg",   inv_proc_alg,   PERM_PROC_STORE_SORT);

  if (pmgr.hasParam("minTimes") && pmgr.hasParam("numPivotSearch")) {
    throw runtime_error("One shouldn't specify both parameters minTimes and numPivotSearch, b/c they are synonyms!");
  }

  pmgr.GetParamOptional("minTimes",        min_times_, 2);
  pmgr.GetParamOptional("numPivotSearch",  min_times_, 2);


  pmgr.GetParamOptional("numPrefixSearch",num_prefix_search_, num_prefix_);
  if (num_prefix_search_ > num_pivot_) {
    PREPARE_RUNTIME_ERR(err) << METH_PIVOT_NEIGHB_HORDER_CLOSEPIV_INVINDEX << " requires that numPrefixSearch (" << num_prefix_search_ << ") "
                             << "should be <= numPivot (" << num_pivot_ << ")";
    THROW_RUNTIME_ERR(err);
  }
  
  if (inv_proc_alg == PERM_PROC_FAST_SCAN) {
    inv_proc_alg_ = kScan; 
  } else if (inv_proc_alg == PERM_PROC_STORE_SORT) {
    inv_proc_alg_ = kStoreSort; 
  } else if (inv_proc_alg == PERM_PROC_MERGE) {
    inv_proc_alg_ = kMerge; 
  } else if (inv_proc_alg == PERM_PROC_PRIOR_QUEUE) {
    inv_proc_alg_ = kPriorQueue;
  } else {
    stringstream err;
    err << "Unknown value of parameter for the inverted file processing algorithm: " << inv_proc_alg_;
    throw runtime_error(err.str());
  } 
    
  pmgr.CheckUnused();
  
  LOG(LIB_INFO) << "Set query-time parameters for PivotNeighbHorderClosePivInvIndex:";
  LOG(LIB_INFO) << "# pivot overlap (minTimes)    = " << min_times_;
  LOG(LIB_INFO) << "# pivots to query (numPrefixSearch) = " << num_prefix_search_;
  LOG(LIB_INFO) << "invProcAlg (code)             = " << inv_proc_alg_ << "(" << toString(inv_proc_alg_) << ")";
  LOG(LIB_INFO) << "# skipChecking                = " << skip_checking_;

}

template <typename dist_t>
PivotNeighbHorderClosePivInvIndex<dist_t>::~PivotNeighbHorderClosePivInvIndex() {
  LOG(LIB_INFO) << "Query qty: " << proc_query_qty_ << " postings per query: " << float(post_qty_) / proc_query_qty_;
  LOG(LIB_INFO) << "Search time: " << search_time_ / proc_query_qty_;
  LOG(LIB_INFO) << "Posting IDS generation time: " <<  ids_gen_time_ / proc_query_qty_;
  LOG(LIB_INFO) << "Pivot-dist comp. time: " <<  dist_pivot_comp_time_ / proc_query_qty_;
  LOG(LIB_INFO) << "Sorting time: " <<  sort_comp_time_ / proc_query_qty_;
  LOG(LIB_INFO) << "Scanning sorted time: " <<  scan_sorted_time_ / proc_query_qty_;
  LOG(LIB_INFO) << "Distance comp. time: " <<  dist_comp_time_ / proc_query_qty_;
  for (const Object* o: genPivot_) delete o;
}

template <typename dist_t>
const string PivotNeighbHorderClosePivInvIndex<dist_t>::StrDesc() const {
  return string(METH_PIVOT_NEIGHB_HORDER_CLOSEPIV_INVINDEX);
}

template <typename dist_t>
void PivotNeighbHorderClosePivInvIndex<dist_t>::SaveIndex(const string &location) {
}

template <typename dist_t>
void PivotNeighbHorderClosePivInvIndex<dist_t>::LoadIndex(const string &location) {
}

/*
 * This function focuses only on getting combinations of closest pivots
 */
template <typename dist_t>
size_t PivotNeighbHorderClosePivInvIndex<dist_t>::genPivotCombIds(std::vector<uint32_t >& ids,
                                                          const Permutation& perm,
                                                          unsigned permPrefixSize
) const {
  CHECK_MSG(pivot_comb_qty_ == 2,
            "Illegal number of pivots in the combinations " + ConvertToString(pivot_comb_qty_) + " must be ==2");
  size_t resSize = 0;
  size_t maxResSize = size_t(permPrefixSize * (permPrefixSize - 1)/ (2*pivotFracInv_));

  for (size_t j = 1; j < permPrefixSize; ++j) {
    for (size_t k = 0; k < j; ++k) {
      size_t   index = PostingListIndex(perm[j], perm[k]);

      if (resSize + 1 >= ids.size()) ids.resize(2*(resSize + 1));

      ids[resSize++] = index;
      if (resSize >= maxResSize) 
      goto end;
    }
  }
end:

  return resSize;
}

template <typename dist_t>
template <typename QueryType>
void PivotNeighbHorderClosePivInvIndex<dist_t>::GenSearch(QueryType* query, size_t K) const {
  size_t dist_comp_time = 0;
  size_t dist_pivot_comp_time = 0;
  size_t sort_comp_time = 0;
  size_t scan_sorted_time = 0;
  size_t ids_gen_time = 0;

  WallClockTimer z_search_time, z_dist_pivot_comp_time, z_dist_comp_time, 
                 z_sort_comp_time, z_scan_sorted_time, z_ids_gen_time;
  z_search_time.reset();

  z_dist_pivot_comp_time.reset();

  Permutation perm_q;
  GetPermutationPPIndexEfficiently(query, perm_q);

  dist_pivot_comp_time = z_dist_pivot_comp_time.split();

  vector<unsigned>          counter;
  if (inv_proc_alg_ == kScan)
    counter.resize(chunk_index_size_);
  PostingListHorderType     tmpRes;
  if (inv_proc_alg_ == kStoreSort)
    tmpRes.resize(chunk_index_size_);

  ObjectVector              cands(chunk_index_size_);
  size_t                    candQty = 0;

  z_ids_gen_time.reset();

  vector<uint32_t>          combIds;

  combIds.reserve((size_t)1 + num_prefix_search_ * (num_prefix_search_ - 1) / 2 / pivotFracInv_);

  size_t qty = genPivotCombIds(combIds, perm_q, num_prefix_search_);

  combIds.resize(qty);

  ids_gen_time += z_ids_gen_time.split();

  for (size_t chunkId = 0; chunkId < posting_lists_.size(); ++chunkId) {
    const auto &chunkPostLists = *posting_lists_[chunkId];
    size_t minId = chunkId * chunk_index_size_;

    const auto &data_start = &this->data_[0] + minId;

    size_t thresh = min_times_;
#if SCALE_MIN_TIMES
    if (pivot_comb_qty_ == 3) 
      thresh = min_times_ * (num_prefix_ - 1) * (num_prefix_ - 2) / 6;
    if (pivot_comb_qty_ == 2) 
      thresh = min_times_ * (num_prefix_ - 1) / 2;
#endif

    if (inv_proc_alg_ == kPriorQueue) {
      // sorted list (priority queue) of pairs (doc_id, its_position_in_the_posting_list)
      //   the doc_ids are negative to keep the queue ordered the way we need
      FalconnHeapMod1<IdType, int32_t> postListQueue;
      // state information for each query-term posting list
      vector<PostListQueryState> queryStates;

      CHECK(num_prefix_search_ >= 1);

      for (size_t id: combIds) {
        const PostingListHorderType& post = getPostingList(chunkPostLists, id);

        if (!post.empty()) {

          unsigned qsi = queryStates.size();
          queryStates.emplace_back(PostListQueryState(post));

          // initialize the postListQueue to the first position - insert pair (-doc_id, query_term_index)
          postListQueue.push(-IdType(post[0]), qsi);
          post_qty_++;
        }
      }

      float accum = 0;

      candQty = 0;

      while (!postListQueue.empty()) {
        // index of the posting list with the current SMALLEST doc_id
        IdType minDocIdNeg = postListQueue.top_key();

        // this while accumulates values for one document (DAAT), specifically for the one with   doc_id = -minDocIdNeg
        while (!postListQueue.empty() && postListQueue.top_key() == minDocIdNeg) {
          unsigned qsi = postListQueue.top_data();
          PostListQueryState &queryState = queryStates[qsi];
          const PostingListHorderType &pl = queryState.post_;

          accum += pivotFracInv_;

          // move to next position in the posting list
          queryState.post_pos_++;
          post_qty_++;

          /*
           * If we didn't reach the end of the posting list, we retrieve the next document id.
           * Then, we push this update element down the priority queue.
           *
           * On reaching the end of the posting list, we evict the entry from the priority queue.
           */
          if (queryState.post_pos_ < pl.size()) {
            /*
             * Leo thinks it may be beneficial to access the posting list entry only once.
             * This access is used for two things
             * 1) obtain the next doc id
             * 2) compute the contribution of the current document to the overall dot product (val_ * qval_)
             */
            postListQueue.replace_top_key(-IdType(pl[queryState.post_pos_]));
          } else postListQueue.pop();

        }

        if (accum >= thresh) {
          cands[candQty++] = data_start[-minDocIdNeg];
        }

        accum = 0;
      }
    }

    if (inv_proc_alg_ == kScan) {
      size_t maxId = min(this->data_.size(), minId + chunk_index_size_);
      size_t chunkQty = maxId - minId;

      if (chunkId) {
        memset(&counter[0], 0, sizeof(counter[0]) * counter.size());
      }

      CHECK(num_prefix_search_ >= 1);

      for (size_t id : combIds) {
        const PostingListHorderType& post = getPostingList(chunkPostLists, id);

        post_qty_ += post.size();
        for (IdType p : post) {
          //counter[p]++;
          counter[p] += pivotFracInv_;
        }
      }

      candQty = 0;
      for (size_t i = 0; i < chunkQty; ++i) {
        if (counter[i] >= thresh) {
          cands[candQty++] = data_start[i];
        }
      }
    }

    if (inv_proc_alg_ == kMerge) {
      VectIdCount tmpRes[2];
      unsigned prevRes = 0;

      CHECK(num_prefix_search_ >= 1);

      for (size_t id : combIds) {
        const PostingListHorderType& post = getPostingList(chunkPostLists, id);

        postListUnion(tmpRes[prevRes], post, tmpRes[1 - prevRes], pivotFracInv_);
        prevRes = 1 - prevRes;

        post_qty_ += post.size();
      }

      candQty = 0;
      for (const auto &it: tmpRes[1 - prevRes]) {
        if (it.qty >= thresh) {
          cands[candQty++] = data_start[it.id];
        }
      }
    }

    if (inv_proc_alg_ == kStoreSort) {
      unsigned tmpResSize = 0;

      CHECK(num_prefix_search_ >= 1);

      for (size_t id : combIds) {
        const PostingListHorderType& post = getPostingList(chunkPostLists, id);

        if (post.size() + tmpResSize > tmpRes.size())
          tmpRes.resize(2 * tmpResSize + post.size());
        memcpy(&tmpRes[tmpResSize], &post[0], post.size() * sizeof(post[0]));
        tmpResSize += post.size();

        post_qty_ += post.size();
      }

      z_sort_comp_time.reset();

      boost::sort::spreadsort::integer_sort(tmpRes.begin(), tmpRes.begin() + tmpResSize);

      sort_comp_time += z_sort_comp_time.split();

      candQty = 0;

      #if 1
      z_scan_sorted_time.reset();
      unsigned start = 0;
      while (start < tmpResSize) {
        IdType prevId = tmpRes[start];
        unsigned next = start + 1;
        for (; next < tmpResSize && tmpRes[next] == prevId; ++next);
        if (pivotFracInv_ * (next - start) >= thresh) {
          cands[candQty++] = data_start[prevId];
        }
        start = next;
      }
      scan_sorted_time += z_scan_sorted_time.split();
      #else
      IdType prevId = -1;
      unsigned prevQty = 0;
      for (unsigned i = 0; i < tmpResSize; ++i) {
        IdType id = tmpRes[i];
        if (id == prevId) prevQty += pivotFracInv_;
        else {
          if (prevQty >= thresh) {
            cands[candQty++]=data_start[prevId];
          }
          prevId = id;
          prevQty = pivotFracInv_;
        }
      }
      if (prevId >= 0 && prevQty >= thresh) {
        cands[candQty++]=data_start[prevId];
      }
      #endif
    }


    z_dist_comp_time.reset();

    if (!skip_checking_) {
      for (size_t i = 0; i < candQty; ++i)
        query->CheckAndAddToResult(cands[i]);
    }

    dist_comp_time += z_dist_comp_time.split();

  }

  {
    unique_lock<mutex> lock(stat_mutex_);

    search_time_ += z_search_time.split();
    dist_comp_time_ += dist_comp_time;
    dist_pivot_comp_time_ += dist_pivot_comp_time;
    sort_comp_time_ += sort_comp_time;
    scan_sorted_time_ += scan_sorted_time;
    ids_gen_time_ += ids_gen_time;
    proc_query_qty_++;
  }

}

template <typename dist_t>
void PivotNeighbHorderClosePivInvIndex<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  GenSearch(query, 0);
}

template <typename dist_t>
void PivotNeighbHorderClosePivInvIndex<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  GenSearch(query, query->GetK());
}

template class PivotNeighbHorderClosePivInvIndex<float>;
template class PivotNeighbHorderClosePivInvIndex<double>;
template class PivotNeighbHorderClosePivInvIndex<int>;

}  // namespace similarity
