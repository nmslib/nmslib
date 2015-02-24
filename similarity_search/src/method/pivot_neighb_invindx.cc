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
#include <unordered_map>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "incremental_quick_select.h"
#include "method/pivot_neighb_invindx.h"
#include "utils.h"

namespace similarity {

using std::vector;
using std::pair;
using std::mutex;

struct IdCount {
  size_t id;
  size_t qty;
  IdCount(size_t id=0, int    qty=0) : id(id), qty(qty) {}
};

typedef vector<IdCount> VectIdCount;

template <typename dist_t>
struct IndexThreadParamsPNII {
  PivotNeighbInvertedIndex<dist_t>&           index_;
  size_t                                      chunk_qty_;
  size_t                                      index_every_;
  size_t                                      out_of_;
  ProgressDisplay*                            progress_bar_;
  mutex&                                      display_mutex_;

  IndexThreadParamsPNII(
                     PivotNeighbInvertedIndex<dist_t>&  index,
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
struct IndexThreadPNII {
  void operator()(IndexThreadParamsPNII<dist_t>& prm) {
    for (size_t i = 0; i < prm.chunk_qty_; ++i) {
      if (prm.index_every_ == i % prm.out_of_) {
        prm.index_.IndexChunk(i, prm.progress_bar_, prm.display_mutex_);
      }
    }
  }
};

void postListUnion(const VectIdCount& lst1, const PostingListInt lst2, VectIdCount& res) {
  res.clear();
  res.reserve((lst1.size() + lst2.size())/2);
  auto i1 = lst1.begin();
  auto i2 = lst2.begin();

  while (i1 != lst1.end() && i2 != lst2.end()) {
    size_t id2 = static_cast<size_t>(*i2);
    if (i1->id < id2) {
      res.push_back(*i1);
      ++i1;
    } else if (i1->id > id2) {
      res.push_back(IdCount(id2, 1));
      ++i2;
    } else {
      res.push_back(IdCount(i1->id, i1->qty + 1));
      ++i1;
      ++i2;
    }
  }
  while (i1 != lst1.end()) {
      res.push_back(*i1);
    ++i1;
  }
  while (i2 != lst2.end()) {
      res.push_back(IdCount(*i2, 1));
      ++i2;
  }
}

template <typename dist_t>
PivotNeighbInvertedIndex<dist_t>::PivotNeighbInvertedIndex(
    bool  PrintProgress,
    const Space<dist_t>* space,
    const ObjectVector& data,
    const AnyParams& AllParams) 
: data_(data),   // reference
  space_(space), // pointer
  chunk_index_size_(65536),
  K_(0),
  knn_amp_(0),
  db_scan_frac_(0),
  num_prefix_(32),
  min_times_(2),
  use_sort_(false),
  skip_checking_(false),
  index_thread_qty_(0),
  num_pivot_(512),
  inv_proc_alg_ (kScan) {
  AnyParamManager pmgr(AllParams);

  string inv_proc_alg = PERM_PROC_FAST_SCAN;

  pmgr.GetParamOptional("numPivot", num_pivot_);

  if (pmgr.hasParam("numPivotIndex") && pmgr.hasParam("numPrefix")) {
    throw runtime_error("One shouldn't specify both parameters numPrefix and numPivotIndex, b/c they are synonoyms!");
  }
  pmgr.GetParamOptional("numPivotIndex", num_prefix_);
  pmgr.GetParamOptional("numPrefix", num_prefix_);

  pmgr.GetParamOptional("chunkIndexSize", chunk_index_size_);

  pmgr.GetParamOptional("indexThreadQty", index_thread_qty_);

  if (num_prefix_ > num_pivot_) {
    LOG(LIB_FATAL) << METH_PIVOT_NEIGHB_INVINDEX << " requires that numPrefix "
               << "should be less than or equal to numPivot";
  }

  CHECK(num_prefix_ <= num_pivot_);
  
  size_t indexQty = (data_.size() + chunk_index_size_ - 1) / chunk_index_size_;

  SetQueryTimeParamsInternal(pmgr);

  pmgr.CheckUnused();

  LOG(LIB_INFO) << "# of entries in an index chunk  = " << chunk_index_size_;
  LOG(LIB_INFO) << "# of index chunks         = " << indexQty;
  LOG(LIB_INFO) << "# of indexing thread      = " << index_thread_qty_;
  LOG(LIB_INFO) << "# pivots                  = " << num_pivot_;
  LOG(LIB_INFO) << "# pivots to index         = " << num_prefix_;
  LOG(LIB_INFO) << "# dbScanFrac              = " << db_scan_frac_;
  LOG(LIB_INFO) << "# knnAmp                  = " << knn_amp_;
  

  GetPermutationPivot(data_, space_, num_pivot_, &pivot_);

  posting_lists_.resize(indexQty);

  /*
   * After we allocated a pointer to each index chunks' vector,
   * it is thread-safe to index each chunk separately.
   */
  for (size_t chunkId = 0; chunkId < indexQty; ++chunkId) {
    posting_lists_[chunkId] = shared_ptr<vector<PostingListInt>>(new vector<PostingListInt>());
  }

  // Don't need more thread than you have chunks
  index_thread_qty_ = min(index_thread_qty_, indexQty);

  mutex                                               progressBarMutex;

  if (index_thread_qty_ <= 1) {
    unique_ptr<ProgressDisplay> progress_bar(PrintProgress ?
                                new ProgressDisplay(data.size(), cerr)
                                :NULL);
    for (size_t chunkId = 0; chunkId < indexQty; ++chunkId) {
      IndexChunk(chunkId, progress_bar.get(), progressBarMutex);
    }

    if (progress_bar) {
      (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
    }
  } else {
    vector<thread>                                      threads(index_thread_qty_);
    vector<shared_ptr<IndexThreadParamsPNII<dist_t>>>   threadParams;

    LOG(LIB_INFO) << "Will create " << index_thread_qty_ << " indexing threads";;

    unique_ptr<ProgressDisplay> progress_bar(PrintProgress ?
                                new ProgressDisplay(data.size(), cerr)
                                :NULL);

    for (size_t i = 0; i < index_thread_qty_; ++i) {
      threadParams.push_back(shared_ptr<IndexThreadParamsPNII<dist_t>>(
                              new IndexThreadParamsPNII<dist_t>(*this, indexQty, i, index_thread_qty_,
                                                               progress_bar.get(), progressBarMutex)));
    }

    for (size_t i = 0; i < index_thread_qty_; ++i) {
      threads[i] = thread(IndexThreadPNII<dist_t>(), ref(*threadParams[i]));
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
void 
PivotNeighbInvertedIndex<dist_t>::IndexChunk(size_t chunkId, ProgressDisplay* progress_bar, mutex& display_mutex) {
  size_t minId = chunkId * chunk_index_size_;
  size_t maxId = min(data_.size(), minId + chunk_index_size_);


  auto & chunkPostLists = *posting_lists_[chunkId];
  chunkPostLists.resize(num_pivot_);

  for (size_t id = 0; id < maxId - minId; ++id) {
    Permutation perm;
    GetPermutationPPIndex(pivot_, space_, data_[minId + id], &perm);
    for (size_t j = 0; j < num_prefix_; ++j) {
      chunkPostLists[perm[j]].push_back(id);
    }
        
    if (id % 1000) {
      unique_lock<mutex> lock(display_mutex);
      if (progress_bar) ++(*progress_bar);
    }
  }

  // Sorting is essential for merging algos
  for (auto & p:chunkPostLists) {
    sort(p.begin(), p.end());
  }
}
    

    
    
template <typename dist_t>
void 
PivotNeighbInvertedIndex<dist_t>::SetQueryTimeParamsInternal(AnyParamManager& pmgr) {
  string inv_proc_alg = PERM_PROC_FAST_SCAN;
  
  pmgr.GetParamOptional("skipChecking", skip_checking_);
  pmgr.GetParamOptional("useSort",      use_sort_);
  pmgr.GetParamOptional("invProcAlg",   inv_proc_alg);
  pmgr.GetParamOptional("minTimes",     min_times_);
  
  if (inv_proc_alg == PERM_PROC_FAST_SCAN) {
    inv_proc_alg_ = kScan; 
  } else if (inv_proc_alg == PERM_PROC_MAP) {
    inv_proc_alg_ = kMap; 
  } else if (inv_proc_alg == PERM_PROC_MERGE) {
    inv_proc_alg_ = kMerge; 
  } else {
    stringstream err;
    err << "Unknown value of parameter for the inverted file processing algorithm: " << inv_proc_alg_;
    throw runtime_error(err.str());
  } 
    
  if (pmgr.hasParam("dbScanFrac") && pmgr.hasParam("knnAmp")) {
    throw runtime_error("One shouldn't specify both parameters dbScanFrac and knnAmp");
  }
  if (pmgr.hasParam("knnAmp")) {
    db_scan_frac_ = 0;
  } else {
    knn_amp_ = 0;
  }
  if (!pmgr.hasParam("dbScanFrac") && !pmgr.hasParam("knnAmp")) {
    db_scan_frac_ = 0;
    knn_amp_ = 0;
  }
  pmgr.GetParamOptional("dbScanFrac",   db_scan_frac_);
  pmgr.GetParamOptional("knnAmp",  knn_amp_);
}

template <typename dist_t>
vector<string>
PivotNeighbInvertedIndex<dist_t>::GetQueryTimeParamNames() const {
  vector<string> names;
  
  names.push_back("dbScanFrac");
  names.push_back("knnAmp");
  names.push_back("useSort");
  names.push_back("skipChecking");
  names.push_back("invProcAlg");
  names.push_back("minTimes");
    
  return names;
}
    
    

template <typename dist_t>
PivotNeighbInvertedIndex<dist_t>::~PivotNeighbInvertedIndex() {
}

template <typename dist_t>
const string PivotNeighbInvertedIndex<dist_t>::ToString() const {
  stringstream str;
  str <<  "permutation (inverted index over neighboring pivots)";
  return str.str();
}

template <typename dist_t>
template <typename QueryType>
void PivotNeighbInvertedIndex<dist_t>::GenSearch(QueryType* query, size_t K) {
  // Let's make this check here. Otherwise, if you misspell dbScanFrac, you will get 
  // a strange error message that says: dbScanFrac should be in the range [0,1].
  if (!knn_amp_) {
    if (db_scan_frac_ < 0.0 || db_scan_frac_ > 1.0) {
      stringstream err;
      err << METH_PIVOT_NEIGHB_INVINDEX << " requires that dbScanFrac is in the range [0,1]";
      throw runtime_error(err.str());
    }
  }

  size_t db_scan = computeDbScan(K);


  Permutation perm_q;
  GetPermutationPPIndex(pivot_, query, &perm_q);

  vector<unsigned>          counter(chunk_index_size_);



  for (size_t chunkId = 0; chunkId < posting_lists_.size(); ++chunkId) {
    const auto & chunkPostLists = *posting_lists_[chunkId];
    size_t minId = chunkId * chunk_index_size_;
    size_t maxId = min(data_.size(), minId + chunk_index_size_);
    size_t chunkQty = maxId - minId;

    const auto data_start = &data_[0] + minId;

    if (use_sort_) {
      if (!db_scan) {
        throw runtime_error("One should specify a proper value for either dbScanFrac or knnAmp");
      }
      vector<pair<int, size_t>> candidates;

      if (inv_proc_alg_ == kMap) {
        std::unordered_map<uint32_t, uint32_t> map_counter;
        for (size_t i = 0; i < num_prefix_; ++i) {
          for (auto& p : chunkPostLists[perm_q[i]]) {
            map_counter[p]++;
          }
        }

        candidates.reserve(db_scan);

        for (auto& it : map_counter) {
          if (it.second >= min_times_) {
            candidates.push_back(std::make_pair(-static_cast<int>(it.second), it.first));
          }
        }
      } else if (inv_proc_alg_ == kScan) {
        candidates.resize(chunkQty);

        for (size_t i = 0; i < candidates.size(); ++i) {
          candidates[i].second = i;
        }
        for (size_t i = 0; i < num_prefix_; ++i) {
          for (auto& p : chunkPostLists[perm_q[i]]) {
            candidates[p].first--;
          }
        }
      } else if (inv_proc_alg_ == kMerge) {
        VectIdCount   tmpRes[2];
        unsigned      prevRes = 0;

        for (size_t i = 0; i < num_prefix_; ++i) {
          postListUnion(tmpRes[prevRes], chunkPostLists[perm_q[i]], tmpRes[1-prevRes]);
          prevRes = 1 - prevRes;
        }

        candidates.reserve(db_scan);

        for (const auto& it: tmpRes[1-prevRes]) {
          if (it.qty >= min_times_) {
            candidates.push_back(std::make_pair(-static_cast<int>(it.qty), it.id));
          }
        }
      } else {
        LOG(LIB_FATAL) << "Bug, unknown inv_proc_alg_: " << inv_proc_alg_;
      }

      IncrementalQuickSelect<IntInt> quick_select(candidates);

      size_t scan_qty = min(db_scan, candidates.size());

      for (size_t i = 0; i < scan_qty; ++i) {
        auto z = quick_select.GetNext();
        if (static_cast<size_t>(-z.first) >= min_times_) {
          const size_t idx = z.second;
          quick_select.Next();
          if (!skip_checking_) query->CheckAndAddToResult(data_start[idx]);
        } else {
          break;
        }
      }
    } else {
      if (inv_proc_alg_ == kMap) {
        std::unordered_map<uint32_t, uint32_t> map_counter;
        for (size_t i = 0; i < num_prefix_; ++i) {
          for (auto& p : chunkPostLists[perm_q[i]]) {
            map_counter[p]++;
          }
        }
        for (auto& it : map_counter) {
          if (it.second >= min_times_) {
            const size_t idx = it.first;
            if (!skip_checking_) query->CheckAndAddToResult(data_start[idx]);
          }
        }
      } else if (inv_proc_alg_ == kScan) {
        if (chunkId) {
          memset(&counter[0], 0, sizeof(counter[0])*counter.size());
        }
        for (size_t i = 0; i < num_prefix_; ++i) {
          for (auto& p : chunkPostLists[perm_q[i]]) {
            counter[p]++;
          }
        }
        for (size_t i = 0; i < chunkQty; ++i) {
          if (counter[i] >= min_times_) {
            if (!skip_checking_) query->CheckAndAddToResult(data_start[i]);
          }
        }
      } else if (inv_proc_alg_ == kMerge) {
        VectIdCount   tmpRes[2];
        unsigned      prevRes = 0;
        for (size_t i = 0; i < num_prefix_; ++i) {
          postListUnion(tmpRes[prevRes], chunkPostLists[perm_q[i]], tmpRes[1-prevRes]);
          prevRes = 1 - prevRes;
        }

        for (const auto& it: tmpRes[1-prevRes]) {
          if (it.qty >= min_times_) {
            if (!skip_checking_) query->CheckAndAddToResult(data_start[it.id]);
          }
        }
      } else {
        LOG(LIB_FATAL) << "Bug, unknown inv_proc_alg_: " << inv_proc_alg_;
      }
    }
  }
}

template <typename dist_t>
void PivotNeighbInvertedIndex<dist_t>::Search(
    RangeQuery<dist_t>* query) {
  GenSearch(query, 0);
}

template <typename dist_t>
void PivotNeighbInvertedIndex<dist_t>::Search(
    KNNQuery<dist_t>* query) {
  GenSearch(query, query->GetK());
}

template class PivotNeighbInvertedIndex<float>;
template class PivotNeighbInvertedIndex<double>;
template class PivotNeighbInvertedIndex<int>;

}  // namespace similarity
