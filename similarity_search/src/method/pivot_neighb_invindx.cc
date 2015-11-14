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
    const Space<dist_t>& space,
    const ObjectVector& data) 
      : data_(data),  
        space_(space), 
        PrintProgress_(PrintProgress) {
}


template <typename dist_t>
void PivotNeighbInvertedIndex<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);


  pmgr.GetParamOptional("numPivot", num_pivot_, 512);

  if (pmgr.hasParam("numPivotIndex") && pmgr.hasParam("numPrefix")) {
    throw runtime_error("One shouldn't specify both parameters numPrefix and numPivotIndex, b/c they are synonyms!");
  }
  pmgr.GetParamOptional("numPivotIndex", num_prefix_, 32);
  pmgr.GetParamOptional("numPrefix",     num_prefix_, num_prefix_);

  pmgr.GetParamOptional("chunkIndexSize", chunk_index_size_, 65536);

  pmgr.GetParamOptional("indexThreadQty", index_thread_qty_,  0);

  if (num_prefix_ > num_pivot_) {
    PREPARE_RUNTIME_ERR(err) << METH_PIVOT_NEIGHB_INVINDEX << " requires that numPrefix (" << num_prefix_ << ") "
                             << "should be less than or equal to numPivot (" << num_pivot_ << ")";
    THROW_RUNTIME_ERR(err);
  }

  CHECK(num_prefix_ <= num_pivot_);
  
  size_t indexQty = (data_.size() + chunk_index_size_ - 1) / chunk_index_size_;

  pmgr.CheckUnused();
  this->ResetQueryTimeParams();

  LOG(LIB_INFO) << "# of entries in an index chunk  = " << chunk_index_size_;
  LOG(LIB_INFO) << "# of index chunks             = " << indexQty;
  LOG(LIB_INFO) << "# of indexing thread          = " << index_thread_qty_;
  LOG(LIB_INFO) << "# pivots                      = " << num_pivot_;
  LOG(LIB_INFO) << "# pivots to index (numPrefix) = " << num_prefix_;

  GetPermutationPivot(data_, space_, num_pivot_, &pivot_, &pivot_pos_);

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
    unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                                new ProgressDisplay(data_.size(), cerr)
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

    unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                                new ProgressDisplay(data_.size(), cerr)
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
PivotNeighbInvertedIndex<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);
  string inv_proc_alg;
  
  pmgr.GetParamOptional("skipChecking", skip_checking_, false);
  pmgr.GetParamOptional("useSort",      use_sort_,      false);
  pmgr.GetParamOptional("invProcAlg",   inv_proc_alg,   PERM_PROC_FAST_SCAN);

  if (pmgr.hasParam("minTimes") && pmgr.hasParam("numPivotSearch")) {
    throw runtime_error("One shouldn't specify both parameters minTimes and numPivotSearch, b/c they are synonyms!");
  }

  pmgr.GetParamOptional("minTimes",        min_times_, 2);
  pmgr.GetParamOptional("numPivotSearch",  min_times_, 2);
  
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
  
  pmgr.GetParamOptional("dbScanFrac",   db_scan_frac_,  0);
  pmgr.GetParamOptional("knnAmp",       knn_amp_,       0);

  pmgr.CheckUnused();
  
  LOG(LIB_INFO) << "Set query-time parameters for PivotNeighbInvertedIndex:";
  LOG(LIB_INFO) << "# pivots to search (minTimes) = " << min_times_;
  LOG(LIB_INFO) << "# dbScanFrac                  = " << db_scan_frac_;
  LOG(LIB_INFO) << "# knnAmp                      = " << knn_amp_;
  LOG(LIB_INFO) << "# useSort                     = " << use_sort_;
  LOG(LIB_INFO) << "invProcAlg (code)             = " << inv_proc_alg_ << "(" << toString(inv_proc_alg_) << ")";
  LOG(LIB_INFO) << "# skipChecking                = " << skip_checking_;
}

template <typename dist_t>
PivotNeighbInvertedIndex<dist_t>::~PivotNeighbInvertedIndex() {
}

template <typename dist_t>
const string PivotNeighbInvertedIndex<dist_t>::StrDesc() const {
  stringstream str;
  str <<  "permutation (inverted index over neighboring pivots)";
  return str.str();
}

template <typename dist_t>
void PivotNeighbInvertedIndex<dist_t>::SaveIndex(const string &location) {
  ofstream outFile(location);
  CHECK_MSG(outFile, "Cannot open file '" + location + "' for writing");

  // Save main parameters

  WriteField(outFile, "numPivot", num_pivot_);
  WriteField(outFile, "numPivotIndex", num_prefix_);
  WriteField(outFile, "chunkIndexSize", chunk_index_size_);
  WriteField(outFile, "indexQty", posting_lists_.size());

  // Save pivots positions
  outFile << MergeIntoStr(pivot_pos_, ' ') << endl;
  vector<IdType> oIDs;
  for (const Object* pObj: pivot_)
    oIDs.push_back(pObj->id());
  // Save pivot IDs
  outFile << MergeIntoStr(oIDs, ' ') << endl;

  for(size_t i = 0; i < posting_lists_.size(); ++i) {
    WriteField(outFile, "chunkId", i);
    CHECK(posting_lists_[i]->size() == num_pivot_);
    for (size_t pivotId = 0; pivotId < num_pivot_; ++pivotId) {
      outFile << MergeIntoStr((*posting_lists_[i])[pivotId], ' ') << endl;
    }
  }

  outFile.close();
}

template <typename dist_t>
void PivotNeighbInvertedIndex<dist_t>::LoadIndex(const string &location) {
  ifstream inFile(location);

  CHECK_MSG(inFile, "Cannot open file '" + location + "' for reading");

  size_t lineNum = 1;
  ReadField(inFile, "numPivot", num_pivot_); lineNum++;
  ReadField(inFile, "numPivotIndex", num_prefix_); lineNum++;
  ReadField(inFile, "chunkIndexSize", chunk_index_size_); lineNum++;
  size_t indexQty;
  ReadField(inFile, "indexQty", indexQty);  lineNum++;

  string line;
  // Read pivot positions
  CHECK_MSG(getline(inFile, line),
            "Failed to read line #" + ConvertToString(lineNum) + " from " + location);
  pivot_pos_.clear();
  CHECK_MSG(SplitStr(line, pivot_pos_, ' '),
            "Failed to extract pivot indices from line #" + ConvertToString(lineNum) + " from " + location);
  CHECK_MSG(pivot_pos_.size() == num_pivot_,
            "# of extracted pivots indices from line #" + ConvertToString(lineNum) + " (" + ConvertToString(pivot_pos_.size()) + ")"
            " doesn't match the number of pivots (" + ConvertToString(num_pivot_) + " from the header (location  " + location + ")");
  pivot_.resize(num_pivot_);
  for (size_t i = 0; i < pivot_pos_.size(); ++i) {
    CHECK_MSG(pivot_pos_[i] < data_.size(), DATA_MUTATION_ERROR_MSG +" (detected an object index >= #of data points");
    pivot_[i] = data_[pivot_pos_[i]];
  }
  ++lineNum;
  // Read pivot object IDs
  vector<IdType> oIDs;
  CHECK_MSG(getline(inFile, line),
            "Failed to read line #" + ConvertToString(lineNum) + " from " + location);
  CHECK_MSG(SplitStr(line, oIDs, ' '),
            "Failed to extract pivot IDs from line #" + ConvertToString(lineNum) + " from " + location);
  CHECK_MSG(oIDs.size() == num_pivot_,
            "# of extracted pivots IDs from line #" + ConvertToString(lineNum) + " (" + ConvertToString(pivot_pos_.size()) + ")"
                " doesn't match the number of pivots (" + ConvertToString(num_pivot_) + " from the header (location  " + location + ")");
  for (size_t i = 0; i < num_pivot_; ++i) {
    if (oIDs[i] != pivot_[i]->id()) {
      PREPARE_RUNTIME_ERR(err) << DATA_MUTATION_ERROR_MSG <<
          " (different pivot IDs detected, old: " << oIDs[i] << " new: " << pivot_[i]->id() << " pivot index: " << i << ")";
      THROW_RUNTIME_ERR(err);
    }
  }
  ++lineNum;
  /*
   * Now let's make a quick sanity-check to see if the pivot IDs match what was saved previously.
   * If the user used a different data set, or a different test split (and a different gold-standard file),
   * we cannot re-use the index
   */

  posting_lists_.resize(indexQty);

  for (size_t chunkId = 0; chunkId < indexQty; ++chunkId) {
    size_t tmp;
    ReadField(inFile, "chunkId", tmp);
    CHECK_MSG(tmp == chunkId, "The chunkId (" + ConvertToString(tmp) + " read from line " + ConvertToString(lineNum) +
              " doesn't match the expected chunk ID " + ConvertToString(chunkId));
    ++lineNum;
    posting_lists_[chunkId] = shared_ptr<vector<PostingListInt>>(new vector<PostingListInt>());
    (*posting_lists_[chunkId]).resize(num_pivot_);
    for (size_t pivotId = 0; pivotId < num_pivot_; ++pivotId) {
      CHECK_MSG(getline(inFile, line),
                "Failed to read line #" + ConvertToString(lineNum) + " from " + location);
      CHECK_MSG(SplitStr(line, (*posting_lists_[chunkId])[pivotId], ' '),
                "Failed to extract object IDs from line #" + ConvertToString(lineNum) +
                " chunkId " + ConvertToString(chunkId) + " location: " + location);
      ++lineNum;
    }
  }

  inFile.close();
}

template <typename dist_t>
template <typename QueryType>
void PivotNeighbInvertedIndex<dist_t>::GenSearch(QueryType* query, size_t K) const {
  // Let's make this check here. Otherwise, if you misspell dbScanFrac, you will get 
  // a strange error message that says: dbScanFrac should be in the range [0,1].
  if (!knn_amp_) {
    if (db_scan_frac_ < 0.0 || db_scan_frac_ > 1.0) {
      stringstream err;
      err << METH_PIVOT_NEIGHB_INVINDEX << " requires that dbScanFrac is in the range [0,1]";
      throw runtime_error(err.str());
    }
  }

  size_t db_scan = computeDbScan(K, posting_lists_.size());


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
        stringstream err;
        err << "One should specify a proper value for either dbScanFrac or knnAmp" <<
               " currently, dbScanFrac=" << db_scan_frac_ << " knnAmp=" << knn_amp_;
        throw runtime_error(err.str());
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
        PREPARE_RUNTIME_ERR(err) << "Bug, unknown inv_proc_alg_: " << inv_proc_alg_;
        THROW_RUNTIME_ERR(err);
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
        PREPARE_RUNTIME_ERR(err) << "Bug, unknown inv_proc_alg_: " << inv_proc_alg_;
        THROW_RUNTIME_ERR(err);
      }
    }
  }
}

template <typename dist_t>
void PivotNeighbInvertedIndex<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  GenSearch(query, 0);
}

template <typename dist_t>
void PivotNeighbInvertedIndex<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  GenSearch(query, query->GetK());
}

template class PivotNeighbInvertedIndex<float>;
template class PivotNeighbInvertedIndex<double>;
template class PivotNeighbInvertedIndex<int>;

}  // namespace similarity
