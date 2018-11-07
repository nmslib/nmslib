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

  pmgr.GetParamOptional("chunkIndexSize", chunk_index_size_, 65536);
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

  size_t indexQty = (this->data_.size() + chunk_index_size_ - 1) / chunk_index_size_;


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


  LOG(LIB_INFO) << "# of entries in an index chunk  = " << chunk_index_size_;
  LOG(LIB_INFO) << "# of index chunks             = " << indexQty;
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

  if (PrintProgress_)
    progress_bar_.reset(new ProgressDisplay(this->data_.size(), cerr));

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

  /*
   * After we allocated each chunks posting lists hash,
   * it is thread-safe to index each chunk separately.
   */
  posting_lists_.resize(indexQty);

  for (size_t chunkId = 0; chunkId < indexQty; ++chunkId) {
    posting_lists_[chunkId].reset(new vector<PostingListInt>(num_pivot_));
  }

  // Do it after ResetQueryTimeParams, but before we create pivot index
  // with the help of HNSW
  AnyParams hnswParamsSearch;
  hnswParamsSearch.AddChangeParam("ef", efPivotSearchIndex_);
  pivot_index_->SetQueryTimeParams(hnswParamsSearch);

  // Don't need more thread than you have chunks
  index_thread_qty_ = min(index_thread_qty_, indexQty);

  ParallelFor(0, indexQty, index_thread_qty_, [&](unsigned chunkId, unsigned threadId) {
    this->IndexChunk(chunkId);
  });

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

    for (size_t chunkId = 0; chunkId < indexQty; ++chunkId) {
      auto & chunkPostLists = *posting_lists_[chunkId];
      for (size_t index = 0; index < chunkPostLists.size(); index++) {
        pivotOcurrQty[index ] += chunkPostLists[index].size();
        total_qty += chunkPostLists[index].size();
      }
    }
    LOG(LIB_INFO) << "Pivot occurrences stat" <<
                     " mean: " << Mean(&pivotOcurrQty[0], pivotOcurrQty.size()) <<
                     " std: " << StdDev(&pivotOcurrQty[0], pivotOcurrQty.size());
    LOG(LIB_INFO) << "Number of postings per document: " << total_qty / this->data_.size();
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
PivotNeighbInvertedIndexHNSW<dist_t>::IndexChunk(size_t chunkId) {
  size_t minId = chunkId * chunk_index_size_;
  size_t maxId = min(this->data_.size(), minId + chunk_index_size_);


  auto & chunkPostLists = *posting_lists_[chunkId];
  chunkPostLists.resize(num_pivot_);
  string externId;

  for (size_t id = 0; id < maxId - minId; ++id) {
    vector<IdType> pivotIds;

    const Object* pObj = this->data_[minId + id];

    unique_ptr<Object> extObj;

    GetClosePivotIds(pObj, num_prefix_, pivotIds);

    for (IdType pivId : pivotIds) {
      chunkPostLists[pivId].push_back(id);
    }

    if (id % 1000 && PrintProgress_) {
      unique_lock<mutex> lock(progress_bar_mutex_);
      if (progress_bar_)
        ++(*progress_bar_);
    }
  }

  // Sorting is essential for merging algos
  for (auto & p:chunkPostLists) {
    sort(p.begin(), p.end());
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
  LOG(LIB_INFO) << "Query qty: " << proc_query_qty_ << " postings per query: " << float(post_qty_) / proc_query_qty_;
  LOG(LIB_INFO) << "Search time: " << search_time_ / proc_query_qty_;
  LOG(LIB_INFO) << "Pivot-dist comp. time: " <<  pivot_search_time_ / proc_query_qty_;
  LOG(LIB_INFO) << "Result copy time (for storeSort): " <<  copy_post_time_ / proc_query_qty_;
  LOG(LIB_INFO) << "Sorting time (for storeSort): " <<  sort_comp_time_ / proc_query_qty_;
  LOG(LIB_INFO) << "Scanning sorted time (for storeSort): " <<  scan_sorted_time_ / proc_query_qty_;
  LOG(LIB_INFO) << "Distance comp. time: " <<  dist_comp_time_ / proc_query_qty_;
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

  WallClockTimer z_search_time, z_pivot_search_time, z_dist_comp_time,
                 z_copy_post, z_sort_comp_time, z_scan_sorted_time;
  z_search_time.reset();


  vector<IdType> pivotIds;

  vector<unsigned>          counter;
  if (inv_proc_alg_ == kScan)
    counter.resize(chunk_index_size_);
  vector<IdType>           tmpRes;
  if (inv_proc_alg_ == kStoreSort)
    tmpRes.resize(chunk_index_size_);

  ObjectVector              cands(chunk_index_size_);
  size_t                    candQty = 0;

  z_pivot_search_time.reset();

  GetClosePivotIds(query->QueryObject(), num_prefix_search_, pivotIds);

  pivot_search_time += z_pivot_search_time.split();

  for (size_t chunkId = 0; chunkId < posting_lists_.size(); ++chunkId) {
    const auto &chunkPostLists = *posting_lists_[chunkId];
    size_t minId = chunkId * chunk_index_size_;

    const auto &data_start = &this->data_[0] + minId;

    if (inv_proc_alg_ == kScan) {
      size_t maxId = min(this->data_.size(), minId + chunk_index_size_);
      size_t chunkQty = maxId - minId;

      if (chunkId) {
        memset(&counter[0], 0, sizeof(counter[0]) * counter.size());
      }

      CHECK(num_prefix_search_ >= 1);

      for (IdTypeUnsign pivId : pivotIds) {
        const PostingListInt &post = chunkPostLists[pivId];

        post_qty_ += post.size();
        for (IdType p : post) {
          counter[p]++;
        }
      }

      candQty = 0;
      for (size_t i = 0; i < chunkQty; ++i) {
        if (counter[i] >= min_times_) {
          cands[candQty++] = data_start[i];
        }
      }
    }

    if (inv_proc_alg_ == kStoreSort) {
      unsigned tmpResSize = 0;

      CHECK(num_prefix_search_ >= 1);

      z_copy_post.reset();
      for (IdTypeUnsign pivId : pivotIds) {
        const PostingListInt &post = chunkPostLists[pivId];

        if (post.size() + tmpResSize > tmpRes.size())
          tmpRes.resize(2 * tmpResSize + post.size());
        memcpy(&tmpRes[tmpResSize], &post[0], post.size() * sizeof(post[0]));
        tmpResSize += post.size();

        post_qty_ += post.size();
      }
      copy_post_time += z_copy_post.split();

      z_sort_comp_time.reset();

      boost::sort::spreadsort::integer_sort(tmpRes.begin(), tmpRes.begin() + tmpResSize);

      sort_comp_time += z_sort_comp_time.split();

      candQty = 0;

      z_scan_sorted_time.reset();
      unsigned start = 0;
      while (start < tmpResSize) {
        IdType prevId = tmpRes[start];
        unsigned next = start + 1;
        for (; next < tmpResSize && tmpRes[next] == prevId; ++next);
        if ((next - start) >= min_times_) {
          cands[candQty++] = data_start[prevId];
        }
        start = next;
      }
      scan_sorted_time += z_scan_sorted_time.split();
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
    pivot_search_time_ += pivot_search_time;
    sort_comp_time_ += sort_comp_time;
    copy_post_time_ += copy_post_time;
    scan_sorted_time_ += scan_sorted_time;
    proc_query_qty_++;
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
