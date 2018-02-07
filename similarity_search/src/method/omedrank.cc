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
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <queue>

#include "space.h"
#include "permutation_utils.h"
#include "rangequery.h"
#include "knnquery.h"
#include "method/omedrank.h"

namespace similarity {

using namespace std;

template <typename dist_t>
OMedRank<dist_t>::OMedRank(
    bool                 PrintProgress,
    const Space<dist_t>& space,
    const ObjectVector& data) :
        Index<dist_t>(data), space_(space), PrintProgress_(PrintProgress),
        index_qty_(0) // If ComputeDbScan is called before index_qty_ is computed, it will see this zero
{ }

template <typename dist_t>
void OMedRank<dist_t>::CreateIndex(const AnyParams &IndexParams) {
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("projType",  proj_type_,    PROJ_TYPE_RAND);
  pmgr.GetParamOptional("intermDim", interm_dim_,   0);
  pmgr.GetParamOptional("numPivot",  num_pivot_,    8);

  pmgr.GetParamOptional("chunkIndexSize", chunk_index_size_, 65536);

  ToLower(proj_type_);
  if (PROJ_TYPE_PERM_BIN == proj_type_)
    throw runtime_error("This method cannot be used with binarized permutations!");

  projection_.reset(Projection<dist_t>::createProjection(
                                                      space_,
                                                      this->data_,
                                                      proj_type_,
                                                      interm_dim_,
                                                      num_pivot_,
                                                      0 /* can't be used with binarized permutations */));

  if (projection_.get() == NULL) {
    throw runtime_error("Cannot create projection class '" + proj_type_ + "'" +
                        " for the space: '" + space_.StrDesc() +"' " +
                        " distance value type: '" + DistTypeName<dist_t>() + "'");
  }

  index_qty_ = (this->data_.size() + chunk_index_size_ - 1) / chunk_index_size_;
  
  pmgr.CheckUnused();

  LOG(LIB_INFO) << "# of entries in an index chunk  = " << chunk_index_size_;
  LOG(LIB_INFO) << "# of index chunks  = " << index_qty_;
  LOG(LIB_INFO) << "projection type:     " << proj_type_;
  LOG(LIB_INFO) << "intermediate dim:    " << interm_dim_;
  LOG(LIB_INFO) << "# pivots/target dim  " << num_pivot_;

  posting_lists_.resize(index_qty_);

  for (size_t chunkId = 0; chunkId < index_qty_; ++chunkId) {
    posting_lists_[chunkId] = shared_ptr<vector<PostingList>>(new vector<PostingList>());
  }

  unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                              new ProgressDisplay(this->data_.size(), cerr)
                              :NULL);

  for (size_t chunkId = 0; chunkId < index_qty_; ++chunkId) {
    IndexChunk(chunkId, progress_bar.get());
  }
}

template <typename dist_t> 
template <typename QueryType> 
void OMedRank<dist_t>::GenSearch(QueryType* query, size_t K) const {
  // Let's make this check here. Otherwise, if you misspell dbScanFrac, you will get 
  // a strange error message that says: dbScanFrac should be in the range [0,1].
  if (!knn_amp_) {
    if (db_scan_frac_ < 0.0 || db_scan_frac_ > 1.0) {
      stringstream err;
      err << METH_OMEDRANK << " requires that dbScanFrac is in the range [0,1]";
      throw runtime_error(err.str());
    }
  }

  size_t db_scan = computeDbScan(K);

  /* 
   * Let these guys have num_pivot_ elements despite
   * only num_pivot_search_ (potentially << num_pivot_) are used.
   * This simplifies the code: element arrays can be addressed
   * using the pivotId that can be > num_pivot_search_
   */
  vector<ssize_t> lowIndx(num_pivot_);
  vector<ssize_t> highIndx(num_pivot_);

  ObjectInvEntry  e(IdType(0), 0); 

  vector<unsigned>  counter(chunk_index_size_);
  vector<float>     projDists(num_pivot_);

  projection_->compProj(query, NULL, &projDists[0]);

  typedef pair<dist_t, size_t>  DIPair;

  // Let's use the closest pivots
  priority_queue<DIPair>    closePivots;  

  for (size_t i = 0 ; i < num_pivot_; ++i) {
    dist_t  posDist = projDists[i] > 0 ? projDists[i] : - projDists[i];  
    /*
     * minus means that the closest pivots are selected. This works
     * better for random projections. However, it appears to work
     * worse for type = rand (at least for CoPhIr)
     */
    closePivots.push(make_pair(-posDist, i));
    if (closePivots.size() > num_pivot_search_) closePivots.pop();
  }

  if (closePivots.size() != num_pivot_search_) {
    stringstream err;
    err << "Bug: the number of close pivots '" << closePivots.size() << "' isn't equal to " << num_pivot_search_;
    throw runtime_error(err.str());
  }

  vector<size_t>  closePivotIds(num_pivot_search_);

  // Note we should have exactly num_pivot_search_ elements in the queue
  for (size_t kk = 0; kk < num_pivot_search_; ++kk) {
    closePivotIds[kk] = closePivots.top().second;  
    closePivots.pop();
  } 

  for (size_t chunkId = 0; chunkId < posting_lists_.size(); ++chunkId) {
    const auto & chunkPostLists = *posting_lists_[chunkId];

    if (chunkId != 0)
      memset(&counter[0], 0, sizeof(counter[0])*counter.size());

    //for (size_t i = 0 ; i < num_pivot_; ++i) {
    for (size_t kk = 0; kk < num_pivot_search_; ++kk) {
      size_t pivotId = closePivotIds[kk];
      // Again, pivot is the left argument, see the comment in the constructor
      e.pivot_dist_ = projDists[pivotId];
      lowIndx[pivotId] = (lower_bound(chunkPostLists[pivotId].begin(), chunkPostLists[pivotId].end(), e) - chunkPostLists[pivotId].begin());;
      --lowIndx[pivotId]; // Can become less than zero
      highIndx[pivotId] = lowIndx[pivotId] + 1;
      CHECK(chunkPostLists[pivotId].size() > 0);
      CHECK(lowIndx[pivotId] < 0 || static_cast<size_t>(lowIndx[pivotId]) < chunkPostLists[pivotId].size());
      CHECK(highIndx[pivotId] >= 0);
    }

    bool      eof = false;
    size_t    totOp = 0;
    size_t    scannedQty = 0;
    size_t    minMatchPivotQty = max(size_t(1), static_cast<size_t>(round(min_freq_ * num_pivot_search_)));

    size_t minId = chunkId * chunk_index_size_;
    size_t maxId = min(this->data_.size(), minId + chunk_index_size_);
    size_t chunkQty = (maxId - minId);

    CHECK(chunkQty <= chunk_index_size_);

    while (scannedQty < min(db_scan, chunkQty) && !eof) {
      eof = true;
      //for (size_t i = 0 ; i < num_pivot_; ++i) {
      for (size_t kk = 0; kk < num_pivot_search_; ++kk) {
        size_t pivotId = closePivotIds[kk];
        IdType    indx[2];
        unsigned  iQty = 0;

        if (lowIndx[pivotId] >= 0) {
          indx[iQty++]=lowIndx[pivotId];
          --lowIndx[pivotId];
        }
        if (static_cast<size_t>(highIndx[pivotId]) < chunkPostLists[pivotId].size()) {
          indx[iQty++]=highIndx[pivotId];
          ++highIndx[pivotId];
        }

        for (unsigned k = 0; k < iQty; ++k) {
          ++totOp;
          IdType objIdDiff = chunkPostLists[pivotId][indx[k]].id_;
          unsigned freq = counter[objIdDiff]++;

          if (freq == minMatchPivotQty) { // Add only the first time when we exceeded the threshold!
            ++scannedQty;
            if (!skip_check_) query->CheckAndAddToResult(this->data_[objIdDiff + minId]);
          } 
          eof = false;
        }
      }
    }
  }
}
 

template <typename dist_t>
void OMedRank<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  GenSearch(query, 0);
}

template <typename dist_t>
void OMedRank<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  GenSearch(query, query->GetK());
}

template <typename dist_t>
void OMedRank<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);

  pmgr.GetParamOptional("skipChecking",   skip_check_, false);

  pmgr.GetParamOptional("minFreq",        min_freq_, 0.5);
  pmgr.GetParamOptional("numPivotSearch", num_pivot_search_, num_pivot_);

  if (num_pivot_search_ > num_pivot_) 
    throw runtime_error("numPivotSearch can't be > numPivot");

  if (pmgr.hasParam("dbScanFrac") && pmgr.hasParam("knnAmp")) {
    throw runtime_error("One shouldn't specify both parameters dbScanFrac and knnAmp");
  }
  
  pmgr.GetParamOptional("dbScanFrac",   db_scan_frac_, 0.05);
  pmgr.GetParamOptional("knnAmp",       knn_amp_,      0);

  pmgr.CheckUnused();
  
  LOG(LIB_INFO) << "Set query-time parameters for OMedRank:";
  LOG(LIB_INFO) << "# dbScanFrac                  = " << db_scan_frac_;
  LOG(LIB_INFO) << "# knnAmp                      = " << knn_amp_;
  LOG(LIB_INFO) << "# minFreq                     = " << min_freq_;
  LOG(LIB_INFO) << "# numPivotSearch              = " << num_pivot_search_;
  LOG(LIB_INFO) << "# skipChecking                = " << skip_check_;
}

template <typename dist_t>
void OMedRank<dist_t>::IndexChunk(size_t chunkId, ProgressDisplay* displayBar) {
  size_t minId = chunkId * chunk_index_size_;
  size_t maxId = min(this->data_.size(), minId + chunk_index_size_);

  auto & chunkPostLists = *posting_lists_[chunkId];
  chunkPostLists.resize(num_pivot_);

  vector<float>     projDists(num_pivot_);


  for (size_t i = 0; i < maxId - minId; ++i) {
    IdType id = minId + i;

    projection_->compProj(NULL, this->data_[id], &projDists[0]);

    for (size_t j = 0; j < num_pivot_; ++j) {
      /* 
       * Object (in this case pivot) is the left argument.
       * At search time, the right argument of the distance will be the query point
       * and pivot again will be the left argument.
       */
      dist_t leftObjDst = projDists[j];
      chunkPostLists[j].push_back(ObjectInvEntry(id - minId, leftObjDst));
    }
    if (displayBar) ++(*displayBar);
  }
  for (size_t j = 0; j < num_pivot_; ++j) {
    sort(chunkPostLists[j].begin(), chunkPostLists[j].end());
  }
}

template class OMedRank<float>;
template class OMedRank<double>;
template class OMedRank<int>;

}
