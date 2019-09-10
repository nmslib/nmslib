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
#include <algorithm>
#include <sstream>
#include <thread>
#include <memory>
#include <fstream>
#include <thread>
#include <unordered_map>

#include "portable_simd.h"
#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "incremental_quick_select.h"
#include "method/napp_optim.h"
#include "utils.h"
#include "thread_pool.h"
#include "fastscancount.h"
#include "fastscancount_avx2.h"

#include "falconn_heap_mod.h"

// Don't enable without re-writing it will try to read from deleted mem
//#define  PRINT_PIVOT_OCCURR_STAT

namespace similarity {

using std::vector;
using std::pair;
using std::mutex;

template <typename dist_t>
NappOptim<dist_t>::NappOptim(
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
void NappOptim<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);


  pmgr.GetParamOptional("numPivot", num_pivot_, 512);

  if (pmgr.hasParam("numPivotIndex") && pmgr.hasParam("numPrefix")) {
    throw runtime_error("One shouldn't specify both parameters numPrefix and numPivotIndex, b/c they are synonyms!");
  }
  pmgr.GetParamOptional("numPivotIndex", num_prefix_, 32);
  pmgr.GetParamOptional("numPrefix",     num_prefix_, num_prefix_);

  pmgr.GetParamOptional("chunkIndexSize", chunk_index_size_, 16384);

  pmgr.GetParamOptional("indexThreadQty", index_thread_qty_,  thread::hardware_concurrency());
  pmgr.GetParamOptional("recreatePoints", recreate_points_,  false);
  pmgr.GetParamOptional("disablePivotIndex", disable_pivot_index_, false);
  pmgr.GetParamOptional("hashTrickDim", hash_trick_dim_, 0);

  if (num_prefix_ > num_pivot_) {
    PREPARE_RUNTIME_ERR(err) << METH_NAPP_OPTIM << " requires that numPrefix (" << num_prefix_ << ") "
                             << "should be <= numPivot (" << num_pivot_ << ")";
    THROW_RUNTIME_ERR(err);
  }

  CHECK(num_prefix_ <= num_pivot_);

  pmgr.GetParamOptional("pivotFile", pivot_file_, "");
  
  index_qty_ = (this->data_.size() + chunk_index_size_ - 1) / chunk_index_size_;

  pmgr.CheckUnused();
  this->ResetQueryTimeParams();

  LOG(LIB_INFO) << "# of entries in an index chunk  = " << chunk_index_size_;
  LOG(LIB_INFO) << "# of index chunks             = " << index_qty_;
  LOG(LIB_INFO) << "# of indexing thread          = " << index_thread_qty_;
  LOG(LIB_INFO) << "# pivotFile                   = " << pivot_file_;
  LOG(LIB_INFO) << "# pivots                      = " << num_pivot_;
  LOG(LIB_INFO) << "# pivots to index (numPrefix) = " << num_prefix_;
  LOG(LIB_INFO) << "# hash trick dimensionionality= " << hash_trick_dim_;
  LOG(LIB_INFO) << "Do we recreate points during indexing when computing distances to pivots?  = " << recreate_points_;

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

  posting_lists_tmp_.resize(index_qty_);

  /*
   * After we allocated a pointer to each index chunks' vector,
   * it is thread-safe to index each chunk separately.
   */
  for (size_t chunkId = 0; chunkId < index_qty_; ++chunkId) {
    posting_lists_tmp_[chunkId] = shared_ptr<vector<PostingListInt>>(new vector<PostingListInt>());
  }

  // Don't need more thread than you have chunks
  index_thread_qty_ = min(index_thread_qty_, index_qty_);

  {
    // First index individual chunks
    mutex                                               progressBarMutex;

    unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                                new ProgressDisplay(this->data_.size(), cerr)
                                :NULL);

    ParallelFor(0, index_qty_, index_thread_qty_, 
                [&](int chunkId, int threadId) { IndexChunk(chunkId, progress_bar.get(), progressBarMutex); });

    if (progress_bar) {
      (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
    }
  }

  {
    // Then merge temporary chunks into contiguous arrays
    mutex                                               progressBarMutex;

    posting_lists_.resize(num_pivot_);
    // Let's init lists, before using threads, then each thread will have its own
    // separate list pointer initialized in advance (and stored in the pointer array posting_lists_). 
    // If we update pointer array from multiple threads, things can get inconsistent
    for (size_t pivotId = 0; pivotId < num_pivot_; ++pivotId) {
      size_t totPostQty = 0;
      for (size_t chunkId = 0; chunkId < index_qty_; ++chunkId) { 
        const auto & chunkPostLists = *posting_lists_tmp_[chunkId];
        totPostQty += chunkPostLists[pivotId].size();
      }
      posting_lists_[pivotId] = shared_ptr<PostingListInt>(new PostingListInt(totPostQty));
    }

    unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                                new ProgressDisplay(num_pivot_, cerr)
                                :NULL);

    ParallelFor(0, num_pivot_, index_thread_qty_, 
                [&](int pivotId, int threadId) { 
      auto & contigPostList = *posting_lists_[pivotId];
      size_t pos = 0;
      for (size_t chunkId = 0; chunkId < index_qty_; ++chunkId) { 
        size_t minId = chunkId * chunk_index_size_;
        const auto & chunkPostLists = *posting_lists_tmp_[chunkId];
        for (auto currId : chunkPostLists[pivotId]) {
          contigPostList[pos++] = currId + minId;
        }
        PostingListInt empty;
        (*posting_lists_tmp_[chunkId])[pivotId].swap(empty); // free mem
      }
      {
        unique_lock<mutex> lock(progressBarMutex);
        if (progress_bar) (*progress_bar)+=1;
      }
    });

    for (size_t chunkId = 0; chunkId < index_qty_; ++chunkId) { 
      posting_lists_tmp_[chunkId].reset(); // free mem
    }

    if (progress_bar) {
      (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
    }
  }

  // Let's collect pivot occurrence statistics
#ifdef PRINT_PIVOT_OCCURR_STAT
  vector<size_t> pivotOcurrQty(num_pivot_);

  for (size_t chunkId = 0; chunkId < index_qty_; ++chunkId) {
    auto & chunkPostLists = *posting_lists_tmp_[chunkId];
    for (size_t i = 0; i < num_pivot_; ++i)
      pivotOcurrQty[i] += chunkPostLists[i].size();
  }
  sort(pivotOcurrQty.begin(), pivotOcurrQty.end());
  stringstream str;
  for (size_t i = 0; i < num_pivot_; ++i) {
    str << pivotOcurrQty[i] << " , ";
  }
  LOG(LIB_INFO) << "Pivot occurrences stat sorted.:" << str.str();
#endif
}

template <typename dist_t>
void NappOptim<dist_t>::GetPermutationPPIndexEfficiently(const Object* pObj, Permutation& p) const {
  vector<dist_t> vDst;

  pivot_index_->ComputePivotDistancesIndexTime(pObj, vDst);
  GetPermutationPPIndexEfficiently(p, vDst);
}

template <typename dist_t>
void NappOptim<dist_t>::GetPermutationPPIndexEfficiently(const Query<dist_t>* pQuery, Permutation& p) const {
  vector<dist_t> vDst;

  pivot_index_->ComputePivotDistancesQueryTime(pQuery, vDst);
  GetPermutationPPIndexEfficiently(p, vDst);
}

template <typename dist_t>
void NappOptim<dist_t>::GetPermutationPPIndexEfficiently(Permutation &p, const vector <dist_t> &vDst) const {
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
NappOptim<dist_t>::IndexChunk(size_t chunkId, ProgressDisplay* progress_bar, mutex& display_mutex) {
  size_t minId = chunkId * chunk_index_size_;
  size_t maxId = min(this->data_.size(), minId + chunk_index_size_);


  auto & chunkPostLists = *posting_lists_tmp_[chunkId];
  chunkPostLists.resize(num_pivot_);
  string externId;

  size_t prevId = 0;
  for (size_t id = 0; id < maxId - minId; ++id) {
    Permutation perm;
    const Object* pObj = this->data_[minId + id];

    unique_ptr<Object> extObj;
    if (recreate_points_) {
      extObj=space_.CreateObjFromStr(-1, -1, space_.CreateStrFromObj(pObj, externId), NULL);
      pObj=extObj.get();
    }

    GetPermutationPPIndexEfficiently(pObj, perm);
    for (size_t j = 0; j < num_prefix_; ++j) {
      chunkPostLists[perm[j]].push_back(id);
    }
        
    if (id >= prevId + 1000) {
      unique_lock<mutex> lock(display_mutex);
      if (progress_bar) (*progress_bar) += id - prevId;
      prevId = id;
    }
  }

  // Sorting is essential for merging algos
  for (auto & p:chunkPostLists) {
    sort(p.begin(), p.end());
  }
}
    

    
    
template <typename dist_t>
void 
NappOptim<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);
  
  pmgr.GetParamOptional("skipChecking", skip_checking_, false);

  if (pmgr.hasParam("minTimes") && pmgr.hasParam("numPivotSearch")) {
    throw runtime_error("One shouldn't specify both parameters minTimes and numPivotSearch, b/c they are synonyms!");
  }

  pmgr.GetParamOptional("minTimes",        min_times_, 2);
  pmgr.GetParamOptional("numPivotSearch",  min_times_, min_times_);

  CHECK_MSG(min_times_ > 0, "The parameter minTimes (aliased to numPivotSearch) should be > 0");

  pmgr.GetParamOptional("numPrefixSearch",num_prefix_search_, num_prefix_);
  if (num_prefix_search_ > num_pivot_) {
    PREPARE_RUNTIME_ERR(err) << METH_NAPP_OPTIM << " requires that numPrefixSearch (" << num_prefix_search_ << ") "
                             << "should be <= numPivot (" << num_pivot_ << ")";
    THROW_RUNTIME_ERR(err);
  }
  
  if (pmgr.hasParam("dbScanFrac") && pmgr.hasParam("knnAmp")) {
    throw runtime_error("One shouldn't specify both parameters dbScanFrac and knnAmp");
  }

  pmgr.CheckUnused();
  
  LOG(LIB_INFO) << "Set query-time parameters:";
  LOG(LIB_INFO) << "# pivot overlap (minTimes)    = " << min_times_;
  LOG(LIB_INFO) << "# pivots to query (numPrefixSearch) = " << num_prefix_search_;
  LOG(LIB_INFO) << "# skipChecking                = " << skip_checking_;
}

template <typename dist_t>
NappOptim<dist_t>::~NappOptim() {
  for (const Object* o: genPivot_) delete o;
}

template <typename dist_t>
const string NappOptim<dist_t>::StrDesc() const {
  stringstream str;
  str <<  "permutation (inverted index over neighboring pivots)";
  return str.str();
}

template <typename dist_t>
void NappOptim<dist_t>::SaveIndex(const string &location) {
#if 0
  ofstream outFile(location);
  CHECK_MSG(outFile, "Cannot open file '" + location + "' for writing");
  outFile.exceptions(std::ios::badbit);

  size_t lineNum = 0;
  // Save main parameters
  WriteField(outFile, METHOD_DESC, StrDesc()); lineNum++;
  WriteField(outFile, "numPivot", num_pivot_); lineNum++;
  WriteField(outFile, "numPivotIndex", num_prefix_); lineNum++;
  WriteField(outFile, "chunkIndexSize", chunk_index_size_); lineNum++;
  WriteField(outFile, "indexQty", posting_lists_.size()); lineNum++;
  WriteField(outFile, "pivotFile", pivot_file_); lineNum++;
  WriteField(outFile, "disablePivotIndex", disable_pivot_index_); lineNum++;
  WriteField(outFile, "hashTrickDim", hash_trick_dim_); lineNum++;

  if (pivot_file_.empty()) {
    // Save pivots positions
    outFile << MergeIntoStr(pivot_pos_, ' ') << endl;
    lineNum++;
    vector<IdType> oIDs;
    for (const Object *pObj: pivot_)
      oIDs.push_back(pObj->id());
    // Save pivot IDs
    outFile << MergeIntoStr(oIDs, ' ') << endl;
    lineNum++;
  }

  for(size_t i = 0; i < posting_lists_.size(); ++i) {
    WriteField(outFile, "chunkId", i); lineNum++;
    CHECK(posting_lists_[i]->size() == num_pivot_);
    for (size_t pivotId = 0; pivotId < num_pivot_; ++pivotId) {
      outFile << MergeIntoStr((*posting_lists_[i])[pivotId], ' ') << endl; lineNum++;
    }
  }

  WriteField(outFile, LINE_QTY, lineNum + 1 /* including this line */);
  outFile.close();
#endif
}

template <typename dist_t>
void NappOptim<dist_t>::LoadIndex(const string &location) {
#if 0
  ifstream inFile(location);
  CHECK_MSG(inFile, "Cannot open file '" + location + "' for reading");
  inFile.exceptions(std::ios::badbit);

  size_t lineNum = 1;
  string methDesc;
  ReadField(inFile, METHOD_DESC, methDesc); lineNum++;
  CHECK_MSG(methDesc == StrDesc(),
            "Looks like you try to use an index created by a different method: " + methDesc);
  ReadField(inFile, "numPivot", num_pivot_); lineNum++;
  ReadField(inFile, "numPivotIndex", num_prefix_); lineNum++;
  ReadField(inFile, "chunkIndexSize", chunk_index_size_); lineNum++;
  size_t indexQty;
  ReadField(inFile, "indexQty", indexQty);  lineNum++;
  ReadField(inFile, "pivotFile", pivot_file_); lineNum++;
  ReadField(inFile, "disablePivotIndex", disable_pivot_index_); lineNum++;
  ReadField(inFile, "hashTrickDim", hash_trick_dim_); lineNum++;

  string line;
  if (pivot_file_.empty()) {
    // Read pivot positions
    CHECK_MSG(getline(inFile, line),
              "Failed to read line #" + ConvertToString(lineNum) + " from " + location);
    pivot_pos_.clear();
    CHECK_MSG(SplitStr(line, pivot_pos_, ' '),
              "Failed to extract pivot indices from line #" + ConvertToString(lineNum) + " from " + location);
    CHECK_MSG(pivot_pos_.size() == num_pivot_,
              "# of extracted pivots indices from line #" + ConvertToString(lineNum) + " (" +
              ConvertToString(pivot_pos_.size()) + ")"
                  " doesn't match the number of pivots (" + ConvertToString(num_pivot_) +
              " from the header (location  " + location + ")");
    pivot_.resize(num_pivot_);
    for (size_t i = 0; i < pivot_pos_.size(); ++i) {
      CHECK_MSG(pivot_pos_[i] < this->data_.size(),
                DATA_MUTATION_ERROR_MSG + " (detected an object index >= #of data points");
      pivot_[i] = this->data_[pivot_pos_[i]];
    }
    ++lineNum;
    // Read pivot object IDs
    vector<IdType> oIDs;
    CHECK_MSG(getline(inFile, line),
              "Failed to read line #" + ConvertToString(lineNum) + " from " + location);
    CHECK_MSG(SplitStr(line, oIDs, ' '),
              "Failed to extract pivot IDs from line #" + ConvertToString(lineNum) + " from " + location);
    CHECK_MSG(oIDs.size() == num_pivot_,
              "# of extracted pivots IDs from line #" + ConvertToString(lineNum) + " (" +
              ConvertToString(pivot_pos_.size()) + ")"
                  " doesn't match the number of pivots (" + ConvertToString(num_pivot_) +
              " from the header (location  " + location + ")");
    /*
     * Now let's make a quick sanity-check to see if the pivot IDs match what was saved previously.
     * If the user used a different data set, or a different test split (and a different gold-standard file),
     * we cannot re-use the index
     */
    for (size_t i = 0; i < num_pivot_; ++i) {
      if (oIDs[i] != pivot_[i]->id()) {
        PREPARE_RUNTIME_ERR(err) << DATA_MUTATION_ERROR_MSG <<
                                 " (different pivot IDs detected, old: " << oIDs[i] << " new: " << pivot_[i]->id() <<
                                 " pivot index: " << i << ")";
        THROW_RUNTIME_ERR(err);
      }
    }
    ++lineNum;
  } else {
    vector<string> vExternIds;
    space_.ReadDataset(pivot_, vExternIds, pivot_file_, num_pivot_);
    if (pivot_.size() < num_pivot_) {
      throw runtime_error("Not enough pivots in the file '" + pivot_file_+ "'");
    }
    genPivot_ = pivot_;
  }
  // Attempt to create an efficient pivot index, after pivots are loaded
  initPivotIndex();

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
  size_t ExpLineNum;
  ReadField(inFile, LINE_QTY, ExpLineNum);
  CHECK_MSG(lineNum == ExpLineNum,
            DATA_MUTATION_ERROR_MSG + " (expected number of lines " + ConvertToString(ExpLineNum) +
            " read so far doesn't match the number of read lines: " + ConvertToString(lineNum));
  inFile.close();
#endif
}

template <typename dist_t>
template <typename QueryType>
void NappOptim<dist_t>::GenSearch(QueryType* query, size_t K) const {

  Permutation perm_q;
  GetPermutationPPIndexEfficiently(query, perm_q);


#define ALGO_SELECTOR (1)

#if ALGO_SELECTOR == 0
  vector<const PostingListInt*> postPtrs;
  vector<uint32_t>              out;
  vector<uint8_t>               counters(this->data_.size());
  //#error "Here"

  for (size_t i = 0; i < num_prefix_search_; ++i) {
    postPtrs.push_back(posting_lists_[perm_q[i]].get());
  }

  fastscancount::fastscancount_avx2(counters, postPtrs, out, min_times_ - 1);

  if (!skip_checking_) {
    for (uint32_t id : out) {
      const Object* obj = this->data_[id];
      query->CheckAndAddToResult(obj);
    }
  }
#elif ALGO_SELECTOR == 1
  vector<const PostingListInt*> postPtrs;
  vector<uint32_t>              out;
  //#error "Here"

  for (size_t i = 0; i < num_prefix_search_; ++i) {
    postPtrs.push_back(posting_lists_[perm_q[i]].get());
  }

  fastscancount::fastscancount(postPtrs, out, min_times_ - 1);

  if (!skip_checking_) {
    for (uint32_t id : out) {
      const Object* obj = this->data_[id];
      query->CheckAndAddToResult(obj);
    }
  }
#elif ALGO_SELECTOR == 2
  vector<uint32_t>          counter(chunk_index_size_);
  vector<PostingListInt::const_iterator> iters;
  vector<const Object*>     tmp_cand;
  tmp_cand.reserve(chunk_index_size_);

  for (size_t i = 0; i < num_prefix_search_; ++i) {
    iters.push_back(posting_lists_[perm_q[i]]->begin());
  }

  for (size_t chunkId = 0; chunkId < index_qty_; ++chunkId) { 

    if (chunkId) {
      memset(&counter[0], 0, sizeof(counter[0])*counter.size());
    }
    size_t minId = chunkId * chunk_index_size_;
    // maxId is exclusive
    size_t maxId = min(this->data_.size(), minId + chunk_index_size_);
    size_t chunkQty = maxId - minId;

    for (size_t i = 0; i < num_prefix_search_; ++i) {

      const auto iterEnd = posting_lists_[perm_q[i]]->end();
      auto& iter = iters[i];

      for (;iter != iterEnd && (*iter) < maxId; ++iter) {
        counter[*iter - minId]++;
      }
    }
    const auto data_start = &this->data_[0] + minId;

    for (size_t i = 0; i < chunkQty; ++i) {
      if (counter[i] >= min_times_) {
        tmp_cand.push_back(data_start[i]);
      }
    }

  }

  if (!skip_checking_) {
    for (const Object* obj: tmp_cand) {
      query->CheckAndAddToResult(obj);
    }
  }
#else
  vector<const Object*>     tmp_cand;
  tmp_cand.reserve(chunk_index_size_);
  vector<uint32_t>          counter(chunk_index_size_);

  for (size_t chunkId = 0; chunkId < posting_lists_.size(); ++chunkId) {
    const auto & chunkPostLists = *posting_lists_[chunkId];
    size_t minId = chunkId * chunk_index_size_;
    size_t maxId = min(this->data_.size(), minId + chunk_index_size_);
    size_t chunkQty = maxId - minId;

    const auto data_start = &this->data_[0] + minId;

    if (chunkId) {
      memset(&counter[0], 0, sizeof(counter[0])*counter.size());
    }

    for (size_t i = 0; i < num_prefix_search_; ++i) {
      for (auto p : chunkPostLists[perm_q[i]]) {
        counter[p]++;
      }
    }

    for (size_t i = 0; i < chunkQty; ++i) {
      if (counter[i] >= min_times_) {
        tmp_cand.push_back(data_start[i]);
      }
    }
  }

  if (!skip_checking_) {
    for (const Object* obj: tmp_cand) {
      query->CheckAndAddToResult(obj);
    }
  }
#endif
}

template <typename dist_t>
void NappOptim<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  GenSearch(query, 0);
}

template <typename dist_t>
void NappOptim<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  GenSearch(query, query->GetK());
}

template class NappOptim<float>;
template class NappOptim<double>;
template class NappOptim<int>;

}  // namespace similarity
