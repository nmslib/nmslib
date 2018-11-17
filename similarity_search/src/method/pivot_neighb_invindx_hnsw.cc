/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2010--2018
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
#include "knnqueue.h"
#include "method/pivot_neighb_invindx_hnsw.h"
#include "utils.h"
#include "ztimer.h"
#include "method/hnsw.h"
#include "thread_pool.h"
#include "logging.h"

#include "falconn_heap_mod.h"

// This include is used for store-and-sort merging method only
#include <boost/sort/spreadsort/integer_sort.hpp>

namespace similarity {

using std::vector;
using std::pair;
using std::mutex;
using std::unique_lock;

template <typename dist_t>
PivotNeighbInvertedIndexHNSW<dist_t>::PivotNeighbInvertedIndexHNSW(
  bool  PrintProgress,
  const Space<dist_t>& space,
  const ObjectVector& data)
  : Index<dist_t>(data),
    space_(space),
    PrintProgress_(PrintProgress) {
}


template <typename dist_t>
void PivotNeighbInvertedIndexHNSW<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);


  pmgr.GetParamOptional("numPivot", num_pivot_, min(size_t(100000), this->data_.size()/10));

  if (pmgr.hasParam("numPivotIndex") && pmgr.hasParam("numPrefix")) {
    throw runtime_error("One shouldn't specify both parameters numPrefix and numPivotIndex, b/c they are synonyms!");
  }
  pmgr.GetParamOptional("numPivotIndex", num_prefix_, 32);
  pmgr.GetParamOptional("numPrefix",     num_prefix_, num_prefix_);

  pmgr.GetParamOptional("indexThreadQty", index_thread_qty_,  thread::hardware_concurrency());
  pmgr.GetParamOptional("hashTrickDim", hash_trick_dim_, 0);

  if (num_prefix_ > num_pivot_) {
    PREPARE_RUNTIME_ERR(err) << METH_PIVOT_NEIGHB_INVINDEX_HNSW << " requires that numPrefix (" << num_prefix_ << ") "
                             << "should be <= numPivot (" << num_pivot_ << ")";
    THROW_RUNTIME_ERR(err);
  }


  CHECK(num_prefix_ <= num_pivot_);

  pmgr.GetParamOptional("pivotFile", pivot_file_, "");
  pmgr.GetParamOptional("printPivotStat", print_pivot_stat_, 0);

  pmgr.GetParamOptional("M", M_, 16);
  pmgr.GetParamOptional("delaunay_type", delaunay_type_, 2);
  int post_;
  pmgr.GetParamOptional("post", post_, 1);
  pmgr.GetParamOptional("efConstruction", efConstruction_, 200);
  pmgr.GetParamOptional("efPivotSearchIndex", efPivotSearchIndex_, 50);


  pmgr.GetParamOptional("numPrefixSearch", num_prefix_search_, num_prefix_);
  pmgr.GetParamOptional("efPivotSearchQuery", efPivotSearchQuery_, efPivotSearchIndex_);

  pmgr.CheckUnused();
  this->ResetQueryTimeParams();

  LOG(LIB_INFO) << "# of indexing thread          = " << index_thread_qty_;
  LOG(LIB_INFO) << "# pivotFile                   = " << pivot_file_;
  LOG(LIB_INFO) << "# pivots                      = " << num_pivot_;
  LOG(LIB_INFO) << "# pivots to index (numPrefix) = " << num_prefix_;
  LOG(LIB_INFO) << "# hash trick dimensionionality= " << hash_trick_dim_;
  LOG(LIB_INFO) << "Do we print pivot stat?       = " << print_pivot_stat_;

  LOG(LIB_INFO) << "M                   = " << M_;
  LOG(LIB_INFO) << "efConstruction      = " << efConstruction_;
  LOG(LIB_INFO) << "delaunay_type       = " << delaunay_type_;
  LOG(LIB_INFO) << "efPivotSearchIndex  = " << efPivotSearchIndex_;

  ObjectVector pivot;
  if (pivot_file_.empty()) {

    GetPermutationPivot(this->data_, space_, num_pivot_, &pivot);
    CHECK(pivot.size() == num_pivot_);
    genPivot_.resize(num_pivot_);
    // IDs must be a sequence from 0 to num_pivot_ - 1
    for (unsigned i = 0; i < num_pivot_; ++i) {
      genPivot_[i] = new Object(i, 0, pivot[i]->datalength(), pivot[i]->data());
    }
  } else {
    vector<string> vExternIds;
    space_.ReadDataset(pivot, vExternIds, pivot_file_, num_pivot_);
    if (pivot.size() < num_pivot_) {
      throw runtime_error("Not enough pivots in the file '" + pivot_file_ + "'");
    }
    genPivot_ = pivot; // Objects will have to be deleted
  }
  for (unsigned i = 0; i < num_pivot_; ++i) {
    CHECK(genPivot_[i]->id() == i); // IDs must be a sequence from 0 to num_pivot_ - 1
  }

  /* Now pivots are loaded or sampled, we can create an HNSW index */
  AnyParams hnswParamsIndex;
  hnswParamsIndex.AddChangeParam("M", M_);
  hnswParamsIndex.AddChangeParam("delaunay_type", delaunay_type_);
  hnswParamsIndex.AddChangeParam("post", post_);
  hnswParamsIndex.AddChangeParam("efConstruction", efConstruction_);

  pivot_index_.reset(new Hnsw<dist_t>(PrintProgress_, space_, genPivot_));
  pivot_index_->CreateIndex(hnswParamsIndex);

  exp_avg_post_size_ = this->data_.size() * num_prefix_ / num_pivot_;
  expCandQtyUB_ = size_t(exp_avg_post_size_ * num_prefix_search_ * 2);

  // Create a new progress bar only after the HNSW index is created
  if (PrintProgress_)
    progress_bar_.reset(new ProgressDisplay(this->data_.size(), cerr));

  posting_lists_.resize(num_pivot_);
  post_list_mutexes_.resize(num_pivot_);
  for (size_t i = 0; i < num_pivot_; ++i) {
    posting_lists_[i].reset(new PostingListInt());
    posting_lists_[i]->reserve(size_t(exp_avg_post_size_ * 1.2));
    post_list_mutexes_[i].reset(new mutex());
  }

  // Do it after ResetQueryTimeParams, but before we create pivot index
  // with the help of HNSW
  AnyParams hnswParamsSearch;
  hnswParamsSearch.AddChangeParam("ef", efPivotSearchIndex_);
  pivot_index_->SetQueryTimeParams(hnswParamsSearch);

  // The search for close pivots can be run in parallel,
    // but modification of the inverted files doesn't have to be (b/c it's super fast)
  ParallelFor(0, this->data_.size(), index_thread_qty_, [&](unsigned id, unsigned threadId) {
    vector<IdType> pivotIds;
    const Object* pObj = this->data_[id];

    GetClosePivotIds(pObj, num_prefix_, pivotIds);

    // We need to sort pivotIds or else we may get a deadlock!
    boost::sort::spreadsort::integer_sort(pivotIds.begin(), pivotIds.end());

    for (IdType pivId: pivotIds) {
      CHECK(pivId < num_pivot_);
      {
        unique_lock<mutex> lock(*post_list_mutexes_[pivId]);
        posting_lists_[pivId]->push_back(id);
      }
    }

    if (id % 1000 && PrintProgress_) {
      unique_lock<mutex> lock(progress_bar_mutex_);
      if (progress_bar_)
        ++(*progress_bar_);
    }
  });

  tmp_res_pool_.reset(new VectorPool<IdType>(index_thread_qty_, expCandQtyUB_));
  counter_pool_.reset(new VectorPool<unsigned>(index_thread_qty_, this->data_.size()));
  cand_pool_.reset(new VectorPool<const Object*>(index_thread_qty_, expCandQtyUB_));

  if (PrintProgress_) {
    if (progress_bar_) {
      (*progress_bar_) += (progress_bar_->expected_count() - progress_bar_->count());
    }
  }


  // Let's collect/privt pivot occurrence statistics
  if (print_pivot_stat_) {
    size_t total_qty = 0;

    size_t maxPostQty = num_pivot_;

    vector<size_t> pivotOcurrQty(maxPostQty);

    for (size_t index = 0; index < posting_lists_.size(); index++) {
      pivotOcurrQty[index ] += posting_lists_[index]->size();
      total_qty += posting_lists_[index]->size();
    }

    LOG(LIB_INFO) << "Pivot occurrences stat" <<
                     " mean: " << Mean(&pivotOcurrQty[0], pivotOcurrQty.size()) <<
                     " std: " << StdDev(&pivotOcurrQty[0], pivotOcurrQty.size()) <<
                     " Expected mean postings size: " << exp_avg_post_size_;
    LOG(LIB_INFO) << "Number of postings per document: " << total_qty / this->data_.size() <<
                    " alternative version for the mean # of pivots per posting: " << total_qty / num_pivot_;
    //sort(pivotOcurrQty.begin(), pivotOcurrQty.end());
    //LOG(LIB_INFO) << MergeIntoStr(pivotOcurrQty, ' ');
  }
}

template <typename dist_t>
void
PivotNeighbInvertedIndexHNSW<dist_t>::GetClosePivotIds(const Object* queryObj, size_t K, vector<IdType>& pivotIds) const {
  KNNQuery<dist_t> query(space_, queryObj, K);

  pivot_index_->Search(&query, 0);

  const KNNQueue<dist_t>* constRes = query.Result();

  unique_ptr<KNNQueue<dist_t>> queue(constRes->Clone());
  pivotIds.resize(queue->Size());
  size_t resPos = queue->Size() - 1;
  while(!queue->Empty()) {
    const Object *pObj = reinterpret_cast<const Object *>(queue->TopObject());
    pivotIds[resPos--] = pObj->id();
    queue->Pop();
  }
}

    
template <typename dist_t>
void
PivotNeighbInvertedIndexHNSW<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);
  string inv_proc_alg;
  
  pmgr.GetParamOptional("skipChecking", skip_checking_, false);
  pmgr.GetParamOptional("invProcAlg",   inv_proc_alg,   PERM_PROC_STORE_SORT);

  if (pmgr.hasParam("minTimes") && pmgr.hasParam("numPivotSearch")) {
    throw runtime_error("One shouldn't specify both parameters minTimes and numPivotSearch, b/c they are synonyms!");
  }

  pmgr.GetParamOptional("numPivotSearch",  min_times_, 2);
  pmgr.GetParamOptional("minTimes",  min_times_, min_times_);

  if (inv_proc_alg == PERM_PROC_FAST_SCAN) {
    inv_proc_alg_ = kScan;
  } else if (inv_proc_alg == PERM_PROC_STORE_SORT) {
    inv_proc_alg_ = kStoreSort;
  } else {
    stringstream err;
    err << "Unknown value of parameter for the inverted file processing algorithm: " << inv_proc_alg;
    throw runtime_error(err.str());
  }

  // This default will be inherited from the index params
  pmgr.GetParamOptional("numPrefixSearch", num_prefix_search_, num_prefix_search_);
  if (num_prefix_search_ > num_pivot_) {
    PREPARE_RUNTIME_ERR(err) << METH_PIVOT_NEIGHB_INVINDEX_HNSW << " requires that numPrefixSearch (" << num_prefix_search_ << ") "
                             << "should be <= numPivot (" << num_pivot_ << ")";
    THROW_RUNTIME_ERR(err);
  }

  // This default will be inherited from the index params
  pmgr.GetParamOptional("efPivotSearchQuery", efPivotSearchQuery_, efPivotSearchQuery_);

  if (pivot_index_) {
    // This function might be called before the pivot index is created,
    // so we don't set its parameters if the pointer is null
    AnyParams hnswParams;
    hnswParams.AddChangeParam("ef", efPivotSearchQuery_);
    pivot_index_->SetQueryTimeParams(hnswParams);
  }

  pmgr.CheckUnused();
  
  LOG(LIB_INFO) << "Set query-time parameters for PivotNeighbHorderHashPivInvIndex:";
  LOG(LIB_INFO) << "numPrefixSearch                     = " << num_prefix_search_;
   LOG(LIB_INFO)<< "# pivot overlap threshold (minTimes)= " << min_times_;
  LOG(LIB_INFO) << "# pivots to query (numPrefixSearch) = " << num_prefix_search_;
  LOG(LIB_INFO) << "# skipChecking                      = " << skip_checking_;
  LOG(LIB_INFO) << "invProcAlg code                     = " << inv_proc_alg_;
  LOG(LIB_INFO) << "efPivotSearchQuery                  = " << efPivotSearchQuery_;
}

template <typename dist_t>
PivotNeighbInvertedIndexHNSW<dist_t>::~PivotNeighbInvertedIndexHNSW() {
  size_t proc_query_qty = proc_query_qty_scan_ + proc_query_qty_store_sort_;
  LOG(LIB_INFO) << "Query qty: " << proc_query_qty;
  LOG(LIB_INFO) << PERM_PROC_FAST_SCAN << ": postings per query: " << float(post_qty_scan_) / proc_query_qty_scan_;
  LOG(LIB_INFO) << PERM_PROC_STORE_SORT << ": postings per query: " << float(post_qty_store_sort_) / proc_query_qty_store_sort_;
  LOG(LIB_INFO) << "Search time: " << search_time_ / proc_query_qty;
  LOG(LIB_INFO) << "Pivot-dist comp. time: " <<  pivot_search_time_ / proc_query_qty;
  LOG(LIB_INFO) << "Result copy time (for storeSort): " <<  copy_post_time_ / proc_query_qty;
  LOG(LIB_INFO) << "Sorting time (for storeSort): " <<  sort_comp_time_ / proc_query_qty;
  LOG(LIB_INFO) << "Scanning sorted time (for storeSort): " <<  scan_sorted_time_ / proc_query_qty;
  LOG(LIB_INFO) << "Distance comp. time: " <<  dist_comp_time_ / proc_query_qty;
  for (const Object* o: genPivot_) delete o;
}

template <typename dist_t>
const string PivotNeighbInvertedIndexHNSW<dist_t>::StrDesc() const {
  return string(METH_PIVOT_NEIGHB_INVINDEX_HNSW);
}

template <typename dist_t>
void PivotNeighbInvertedIndexHNSW<dist_t>::SaveIndex(const string &location) {
  throw runtime_error("Not implemented!");
}

template <typename dist_t>
void PivotNeighbInvertedIndexHNSW<dist_t>::LoadIndex(const string &location) {
  throw runtime_error("Not implemented!");
}


template <typename dist_t>
template <typename QueryType>
void PivotNeighbInvertedIndexHNSW<dist_t>::GenSearch(QueryType* query, size_t K) const {
  size_t dist_comp_time = 0;
  size_t pivot_search_time = 0;
  size_t sort_comp_time = 0;
  size_t scan_sorted_time = 0;
  size_t copy_post_time = 0;
  size_t post_qty = 0;

  WallClockTimer z_search_time, z_pivot_search_time, z_dist_comp_time,
                 z_copy_post, z_sort_comp_time, z_scan_sorted_time;
  z_search_time.reset();


  vector<IdType> pivotIds;


  vector<const Object*>*          candVectorPtr = cand_pool_->loan();
  vector<const Object*>&          cands = *candVectorPtr;

  vector<unsigned>*               counter = nullptr;
  unsigned*                       counterBeg = nullptr;


  size_t candSize = 0;
  size_t tmpResSize = 0;

  CHECK(candVectorPtr);

  if (inv_proc_alg_ == kScan) {
    CHECK(counter_pool_);
    counter = counter_pool_->loan();

    if (counter->size() < this->data_.size()) {
      counter->resize(this->data_.size());
    }
    // Set the pointer only after potentially resizing the vector!
    counterBeg = &(*counter)[0];
    memset(counterBeg, 0, sizeof(*counterBeg) * counter->size());
  }

  vector<IdType>*           tmpRes = nullptr;
  if (inv_proc_alg_ == kStoreSort) {
    tmpRes = tmp_res_pool_->loan();
  }

  z_pivot_search_time.reset();

  GetClosePivotIds(query->QueryObject(), num_prefix_search_, pivotIds);

  pivot_search_time += z_pivot_search_time.split();

  if (inv_proc_alg_ == kScan) {
    CHECK(counter && counter->size() >= this->data_.size());
    CHECK(num_prefix_search_ >= 1);
    counterBeg = &(*counter)[0];
    
    for (IdTypeUnsign pivId : pivotIds) {
      const PostingListInt &post = *(posting_lists_[pivId]);

      post_qty += post.size();
      for (IdType p : post) {
        counterBeg[p]++;
      }

    }

    for (size_t i = 0; i < this->data_.size(); ++i) {
      if (counterBeg[i] >= min_times_) {

        if (candSize >= cands.size())
          cands.resize(2 * candSize + 1);
        cands[candSize++] = this->data_[i];

      }
    }
  }

  if (inv_proc_alg_ == kStoreSort) {
    CHECK(tmpRes);
    CHECK(num_prefix_search_ >= 1);

    z_copy_post.reset();
    for (IdTypeUnsign pivId : pivotIds) {
      const PostingListInt &post = *(posting_lists_[pivId]);

      if (post.size() + tmpResSize > tmpRes->size())
        tmpRes->resize(2 * tmpResSize + post.size());
      memcpy(&(*tmpRes)[tmpResSize], &post[0], post.size() * sizeof(post[0]));
      tmpResSize += post.size();

      post_qty += post.size();

    }
    copy_post_time += z_copy_post.split();

    z_sort_comp_time.reset();

    boost::sort::spreadsort::integer_sort(tmpRes->begin(), tmpRes->begin() + tmpResSize);

    sort_comp_time += z_sort_comp_time.split();

    z_scan_sorted_time.reset();
    unsigned start = 0;
    const IdType *tmpResBeg = &(*tmpRes)[0];
    while (start < tmpResSize) {
      IdType prevId = tmpResBeg[start];
      unsigned next = start + 1;
      for (; next < tmpResSize && tmpResBeg[next] == prevId; ++next);
      if ((next - start) >= min_times_) {

        if (candSize >= cands.size())
          cands.resize(2 * candSize + 1);
        cands[candSize++] = this->data_[prevId];

      }
      start = next;
    }
    scan_sorted_time += z_scan_sorted_time.split();
  }


  z_dist_comp_time.reset();

  if (!skip_checking_) {
    for (size_t i = 0; i < candSize; ++i) {
      query->CheckAndAddToResult(cands[i]);
    }
  }

  if (tmpRes) {
    CHECK(tmp_res_pool_.get());
    tmp_res_pool_->release(tmpRes);
  }
  CHECK(cand_pool_.get());
  cand_pool_->release(candVectorPtr);

  if (counter) {
    CHECK(counter_pool_.get());
    counter_pool_->release(counter);
  }

  dist_comp_time += z_dist_comp_time.split();

  {
    unique_lock<mutex> lock(stat_mutex_);

    search_time_ += z_search_time.split();
    dist_comp_time_ += dist_comp_time;
    pivot_search_time_ += pivot_search_time;
    sort_comp_time_ += sort_comp_time;
    copy_post_time_ += copy_post_time;
    scan_sorted_time_ += scan_sorted_time;
    if (inv_proc_alg_ == kScan) {
      proc_query_qty_scan_++;
      post_qty_scan_ += post_qty;
    }
    if (inv_proc_alg_ == kStoreSort) {
      proc_query_qty_store_sort_++;
      post_qty_store_sort_ += post_qty;
    }
  }

}

template <typename dist_t>
void PivotNeighbInvertedIndexHNSW<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  GenSearch(query, 0);
}

template <typename dist_t>
void PivotNeighbInvertedIndexHNSW<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  GenSearch(query, query->GetK());
}

template class PivotNeighbInvertedIndexHNSW<float>;
template class PivotNeighbInvertedIndexHNSW<double>;
template class PivotNeighbInvertedIndexHNSW<int>;

}  // namespace similarity
