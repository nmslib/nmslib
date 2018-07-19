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
#include "method/pivot_neighb_horder_invindx.h"
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

/**
 * A structure that keeps information about current state of search within one posting list.
 */
struct PostListQueryState {
  // pointer to the posting list (fixed from the beginning)
  const PostingListType&  post_;
  // actual position in the list
  unsigned                post_pos_;

  PostListQueryState(const PostingListType& pl) : 
                     post_(pl), post_pos_(0) {}
};

template <typename dist_t>
struct IndexThreadParamsHorderPNII {
  PivotNeighbHorderInvIndex<dist_t>&           index_;
  size_t                                      chunk_qty_;
  size_t                                      index_every_;
  size_t                                      out_of_;
  ProgressDisplay*                            progress_bar_;
  mutex&                                      display_mutex_;

  IndexThreadParamsHorderPNII(
                     PivotNeighbHorderInvIndex<dist_t>&  index,
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
struct IndexThreadHOrderHorderPNII {
  void operator()(IndexThreadParamsHorderPNII<dist_t>& prm) {
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
PivotNeighbHorderInvIndex<dist_t>::PivotNeighbHorderInvIndex(
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
void PivotNeighbHorderInvIndex<dist_t>::CreateIndex(const AnyParams& IndexParams) {
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
    PREPARE_RUNTIME_ERR(err) << METH_PIVOT_NEIGHB_HORDER_INVINDEX << " requires that numPrefix (" << num_prefix_ << ") "
                             << "should be <= numPivot (" << num_pivot_ << ")";
    THROW_RUNTIME_ERR(err);
  }

  CHECK(num_prefix_ <= num_pivot_);

  pmgr.GetParamOptional("pivotFile", pivot_file_, "");
  pmgr.GetParamOptional("skipVal",  skip_val_, 1);
  pmgr.GetParamOptional("pivotCombQty", pivot_comb_qty_, 1);
  pmgr.GetParamOptional("printPivotStat", print_pivot_stat_, 0);

  CHECK_MSG(pivot_comb_qty_ && pivot_comb_qty_ <= 3,
            "Illegal number of pivots in the combinations " + ConvertToString(pivot_comb_qty_) + " must be >0 and <=3");
  
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
  LOG(LIB_INFO) << "# skipVal                     = " << skip_val_;
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
    posting_lists_[chunkId].reset(new vector<PostingListType>());
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
    vector<shared_ptr<IndexThreadParamsHorderPNII<dist_t>>>   threadParams;

    LOG(LIB_INFO) << "Will create " << index_thread_qty_ << " indexing threads";;

    unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                                new ProgressDisplay(this->data_.size(), cerr)
                                :NULL);

    for (size_t i = 0; i < index_thread_qty_; ++i) {
      threadParams.push_back(shared_ptr<IndexThreadParamsHorderPNII<dist_t>>(
                              new IndexThreadParamsHorderPNII<dist_t>(*this, indexQty, i, index_thread_qty_,
                                                               progress_bar.get(), progressBarMutex)));
    }

    for (size_t i = 0; i < index_thread_qty_; ++i) {
      threads[i] = thread(IndexThreadHOrderHorderPNII<dist_t>(), ref(*threadParams[i]));
    }

    for (size_t i = 0; i < index_thread_qty_; ++i) {
      threads[i].join();
    }

    if (progress_bar) {
      (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
    }
  }

  // Let's collect/privt pivot occurrence statistics
  if (print_pivot_stat_) {
    size_t total_qty = 0;

    size_t maxPostQty = getPostQtys(pivot_comb_qty_, skip_val_);

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
    //LOG(LIB_INFO) << MergeIntoStr(pivotOcurrQty, ' ');
  }
}

template <typename dist_t>
void PivotNeighbHorderInvIndex<dist_t>::GetPermutationPPIndexEfficiently(const Object* pObj, Permutation& p) const {
  vector<dist_t> vDst;

  pivot_index_->ComputePivotDistancesIndexTime(pObj, vDst);
  GetPermutationPPIndexEfficiently(p, vDst);
}

template <typename dist_t>
void PivotNeighbHorderInvIndex<dist_t>::GetPermutationPPIndexEfficiently(const Query<dist_t>* pQuery, Permutation& p) const {
  vector<dist_t> vDst;

  pivot_index_->ComputePivotDistancesQueryTime(pQuery, vDst);
  GetPermutationPPIndexEfficiently(p, vDst);
}

template <typename dist_t>
void PivotNeighbHorderInvIndex<dist_t>::GetPermutationPPIndexEfficiently(Permutation &p, const vector <dist_t> &vDst) const {
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
PivotNeighbHorderInvIndex<dist_t>::IndexChunk(size_t chunkId, ProgressDisplay* progress_bar, mutex& display_mutex) {
  size_t minId = chunkId * chunk_index_size_;
  size_t maxId = min(this->data_.size(), minId + chunk_index_size_);


  vector<PostingListType> & chunkPostLists = *posting_lists_[chunkId];
  size_t maxPostQty = getPostQtys(pivot_comb_qty_, skip_val_);

  vector<uint32_t > combIds;
  combIds.reserve(maxPostQty);

  chunkPostLists.resize(maxPostQty);

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
      uint32_t idiv = combIds[i];
      CHECK_MSG(idiv < chunkPostLists.size(), ConvertToString(idiv) + " vs " + ConvertToString(chunkPostLists.size()));
      PostingListType& post = chunkPostLists[idiv];
      post.push_back(objId);
    }
        
    if (objId % 1000) {
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
PivotNeighbHorderInvIndex<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);
  string inv_proc_alg;
  
  pmgr.GetParamOptional("skipChecking", skip_checking_, false);
  pmgr.GetParamOptional("invProcAlg",   inv_proc_alg,   PERM_PROC_STORE_SORT);

  if (pmgr.hasParam("minTimes") && pmgr.hasParam("numPivotSearch")) {
    throw runtime_error("One shouldn't specify both parameters minTimes and numPivotSearch, b/c they are synonyms!");
  }

  pmgr.GetParamOptional("minTimes",        min_times_, 2);
  pmgr.GetParamOptional("numPivotSearch",  min_times_, 2);


  pmgr.GetParamOptional("numPrefixSearch",num_prefix_search_, num_prefix_);
  if (num_prefix_search_ > num_pivot_) {
    PREPARE_RUNTIME_ERR(err) << METH_PIVOT_NEIGHB_HORDER_INVINDEX << " requires that numPrefixSearch (" << num_prefix_search_ << ") "
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
  
  LOG(LIB_INFO) << "Set query-time parameters for PivotNeighbHorderInvIndex:";
  LOG(LIB_INFO) << "# pivot overlap (minTimes)    = " << min_times_;
  LOG(LIB_INFO) << "# pivots to query (numPrefixSearch) = " << num_prefix_search_;
  LOG(LIB_INFO) << "invProcAlg (code)             = " << inv_proc_alg_ << "(" << toString(inv_proc_alg_) << ")";
  LOG(LIB_INFO) << "# skipChecking                = " << skip_checking_;

}

template <typename dist_t>
PivotNeighbHorderInvIndex<dist_t>::~PivotNeighbHorderInvIndex() {
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
const string PivotNeighbHorderInvIndex<dist_t>::StrDesc() const {
  return string(METH_PIVOT_NEIGHB_HORDER_INVINDEX);
}

template <typename dist_t>
void PivotNeighbHorderInvIndex<dist_t>::SaveIndex(const string &location) {
  ofstream outFile(location);
  CHECK_MSG(outFile, "Cannot open file '" + location + "' for writing");
  outFile.exceptions(std::ios::badbit);

  size_t lineNum = 0;
  // Save main parameters
  WriteField(outFile, METHOD_DESC, StrDesc()); lineNum++;
  WriteField(outFile, "numPivot", num_pivot_); lineNum++;
  WriteField(outFile, "numPivotIndex", num_prefix_); lineNum++;
  WriteField(outFile, "skipVal", skip_val_); lineNum++;
  WriteField(outFile, "pivotCombQty", pivot_comb_qty_); lineNum++;
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

  size_t maxPostQty = getPostQtys(pivot_comb_qty_, skip_val_);

  for(size_t i = 0; i < posting_lists_.size(); ++i) {
    WriteField(outFile, "chunkId", i); lineNum++;
    CHECK(posting_lists_[i]->size() == maxPostQty);
    WriteField(outFile, "postQty", (*posting_lists_[i]).size());
    for (const auto& e: (*posting_lists_[i])) {
      outFile << MergeIntoStr(e, ' ') << endl; lineNum++;
    }
  }

  WriteField(outFile, LINE_QTY, lineNum + 1 // including this line
  );
  outFile.close();
}

template <typename dist_t>
void PivotNeighbHorderInvIndex<dist_t>::LoadIndex(const string &location) {
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
  ReadField(inFile, "skipVal", skip_val_); lineNum++;
  ReadField(inFile, "pivotCombQty", pivot_comb_qty_); lineNum++;
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
    
     // Now let's make a quick sanity-check to see if the pivot IDs match what was saved previously.
     // If the user used a different data set, or a different test split (and a different gold-standard file),
     // we cannot re-use the index
     //
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
    posting_lists_[chunkId] = unique_ptr<vector<PostingListType>>(new vector<PostingListType>());

    size_t postQty;
    ReadField(inFile, "postQty", postQty);
    (*posting_lists_[chunkId]).resize(postQty);
    for (size_t postId = 0; postId < postQty; ++postId) {
      CHECK_MSG(getline(inFile, line),
                "Failed to read line #" + ConvertToString(lineNum) + " from " + location);
      CHECK_MSG(SplitStr(line, (*posting_lists_[chunkId])[postId], ' '),
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
}

template <typename dist_t>
size_t PivotNeighbHorderInvIndex<dist_t>::genPivotCombIds(std::vector<uint32_t >& ids,
                                                          const Permutation& perm,
                                                          unsigned permPrefixSize
) const {
  CHECK_MSG(pivot_comb_qty_ && pivot_comb_qty_ <= 3,
            "Illegal number of pivots in the combinations " + ConvertToString(pivot_comb_qty_) + " must be >0 and <=3");

  if (pivot_comb_qty_ == 1) {
    size_t resSize = 0;
    CHECK(permPrefixSize < perm.size());

    for (unsigned i = 0; i < permPrefixSize; ++i) {
      size_t   index = perm[i];

      uint32_t idiv = index / skip_val_;
      size_t imod   = index % skip_val_;

      if (imod == 0) {
        if (resSize + 1 >= ids.size())
          ids.resize(2*(resSize + 1));
        ids[resSize++] = idiv;
      }
    }

    return resSize;

  } else if (pivot_comb_qty_ == 2) {
    size_t resSize = 0;

    for (size_t j = 1; j < permPrefixSize; ++j) {
      for (size_t k = 0; k < j; ++k) {
        size_t   index = PostingListIndex(perm[j], perm[k]);

        uint32_t idiv = index / skip_val_;
        size_t imod   = index % skip_val_;

        if (imod == 0) {
          if (resSize + 1 >= ids.size())
            ids.resize(2*(resSize + 1));
          ids[resSize++] = idiv;
        }
      }
    }

    return resSize;
  } else {
    CHECK(pivot_comb_qty_ == 3);
    size_t resSize = 0;

    for (size_t j = 2; j < permPrefixSize; ++j) {
      for (size_t k = 1; k < j; ++k) {
        for (size_t l = 0; l < k; ++l) {
          IdTypeUnsign index = PostingListIndex(perm[j], perm[k], perm[l]);
          uint32_t idiv = index / skip_val_;
          size_t imod = index - idiv * skip_val_;
          if (imod == 0) {
            if (resSize + 1 >= ids.size())
              ids.resize(2*(resSize + 1));
            ids[resSize++] = idiv;
          }
        }
      }
    }

    return resSize;
  }
}

template <typename dist_t>
template <typename QueryType>
void PivotNeighbHorderInvIndex<dist_t>::GenSearch(QueryType* query, size_t K) const {
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
  PostingListType           tmpRes;
  if (inv_proc_alg_ == kStoreSort)
    tmpRes.resize(chunk_index_size_);

  ObjectVector              cands(chunk_index_size_);
  size_t                    candQty = 0;

  z_ids_gen_time.reset();

  vector<uint32_t>          combIds;

  combIds.reserve(getPostQtys(pivot_comb_qty_, skip_val_));

  size_t qty = genPivotCombIds(combIds, perm_q, num_prefix_search_);

  combIds.resize(qty);

  ids_gen_time += z_ids_gen_time.split();

  for (size_t chunkId = 0; chunkId < posting_lists_.size(); ++chunkId) {
    const auto &chunkPostLists = *posting_lists_[chunkId];
    size_t minId = chunkId * chunk_index_size_;

    const auto &data_start = &this->data_[0] + minId;

    // This threshold will also be divided by skip_val_ (see the code below)
    // So we essentially we scale min_times_ by the ratio between the
    // number pivots pairs or triples indexed by the number of regular
    // pivots that would have been indexed by the classic NAPP
    
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

      for (size_t idiv : combIds) {
        CHECK_MSG(idiv < chunkPostLists.size(),
                  ConvertToString(idiv) + " vs " + ConvertToString(chunkPostLists.size()));
        const PostingListType &post = chunkPostLists[idiv];

        if (!post.empty()) {

          unsigned qsi = queryStates.size();
          queryStates.emplace_back(PostListQueryState(post));

          // initialize the postListQueue to the first position - insert pair (-doc_id, query_term_index)
          postListQueue.push(-IdType(post[0]), qsi);
          post_qty_++;
        }
      }

      unsigned accum = 0;

      candQty = 0;

      while (!postListQueue.empty()) {
        // index of the posting list with the current SMALLEST doc_id
        IdType minDocIdNeg = postListQueue.top_key();

        // this while accumulates values for one document (DAAT), specifically for the one with   doc_id = -minDocIdNeg
        while (!postListQueue.empty() && postListQueue.top_key() == minDocIdNeg) {
          unsigned qsi = postListQueue.top_data();
          PostListQueryState &queryState = queryStates[qsi];
          const PostingListType &pl = queryState.post_;

          accum += skip_val_;

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

      for (size_t idiv : combIds) {
        CHECK_MSG(idiv < chunkPostLists.size(),
                  ConvertToString(idiv) + " vs " + ConvertToString(chunkPostLists.size()));
        const PostingListType &post = chunkPostLists[idiv];

        post_qty_ += post.size();
        for (IdType p : post) {
          //counter[p]++;
          counter[p] += skip_val_;
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

      for (size_t idiv : combIds) {
        CHECK_MSG(idiv < chunkPostLists.size(),
                  ConvertToString(idiv) + " vs " + ConvertToString(chunkPostLists.size()));
        const PostingListType &post = chunkPostLists[idiv];

        postListUnion(tmpRes[prevRes], post, tmpRes[1 - prevRes], skip_val_);
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

      for (size_t idiv : combIds) {
        CHECK_MSG(idiv < chunkPostLists.size(),
                  ConvertToString(idiv) + " vs " + ConvertToString(chunkPostLists.size()));
        const PostingListType &post = chunkPostLists[idiv];

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
        if (skip_val_ * (next - start) >= thresh) {
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
        if (id == prevId) prevQty += skip_val_;
        else {
          if (prevQty >= thresh) {
            cands[candQty++]=data_start[prevId];
          }
          prevId = id;
          prevQty = skip_val_;
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
void PivotNeighbHorderInvIndex<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  GenSearch(query, 0);
}

template <typename dist_t>
void PivotNeighbHorderInvIndex<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  GenSearch(query, query->GetK());
}

template class PivotNeighbHorderInvIndex<float>;
template class PivotNeighbHorderInvIndex<double>;
template class PivotNeighbHorderInvIndex<int>;

}  // namespace similarity
