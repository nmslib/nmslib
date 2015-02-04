/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * Patent ALERT: even though the code is released under the liberal Apache 2 license,
 * the underlying search method is patented. Therefore, it's free to use in research,
 * but may be problematic in a production setting.
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <unordered_map>
#include <algorithm>
#include <cmath>

#include "space.h"
#include "permutation_utils.h"
#include "rangequery.h"
#include "knnquery.h"
#include "method/omedrank.h"

namespace similarity {

template <typename dist_t>
OMedRank<dist_t>::OMedRank(
    const Space<dist_t>* space,
    const ObjectVector& data,
    AnyParamManager &pmgr) 
	: data_(data),  /* reference */
    space_(space), /* pointer */
    num_pivot_(8),
    chunk_index_size_(16536),
    index_qty_(0), // If ComputeDbScan is called before index_qty_ is computed, it will see this zero
    skip_check_(false),
    proj_dim_(0)
 {

  pmgr.GetParamOptional("projType", proj_type_);
  if (proj_type_.empty()) proj_type_ = PROJ_TYPE_RAND;
  pmgr.GetParamOptional("projDim", proj_dim_);
  pmgr.GetParamOptional("numPivot", num_pivot_);
  pmgr.GetParamOptional("chunkIndexSize", chunk_index_size_);

  projection_.reset(Projection<dist_t>::createProjection(
                                                      space_,
                                                      data_,
                                                      proj_type_,
                                                      proj_dim_,
                                                      num_pivot_));

  if (projection_.get() == NULL) {
    throw runtime_error("Cannot create projection class '" + proj_type_ + "'" +
                        " for the space: '" + space_->ToString() +"' " +
                        " distance value type: '" + DistTypeName<dist_t>() + "'");
  }

  index_qty_ = (data_.size() + chunk_index_size_ - 1) / chunk_index_size_;
  // Call this function AFTER the index size is computed!
  SetQueryTimeParamsInternal(pmgr);

  LOG(LIB_INFO) << "# of entries in an index chunk  = " << chunk_index_size_;
  LOG(LIB_INFO) << "# of index chunks  = " << index_qty_;
  LOG(LIB_INFO) << "projection type:   " << proj_type_;
  LOG(LIB_INFO) << "projection dim:    " << proj_dim_;
  LOG(LIB_INFO) << "# pivots         = " << num_pivot_;
  LOG(LIB_INFO) << "db scan fraction = " << db_scan_frac_;
  LOG(LIB_INFO) << "min freq = "         << min_freq_;

  posting_lists_.resize(index_qty_);

  for (size_t chunkId = 0; chunkId < index_qty_; ++chunkId) {
    posting_lists_[chunkId] = shared_ptr<vector<PostingList>>(new vector<PostingList>());
  }

  for (size_t chunkId = 0; chunkId < index_qty_; ++chunkId) {
    IndexChunk(chunkId);
  }
}

template <typename dist_t> 
template <typename QueryType> 
void OMedRank<dist_t>::GenSearch(QueryType* query) {
  vector<ssize_t> lowIndx(num_pivot_);
  vector<ssize_t> highIndx(num_pivot_);

  ObjectInvEntry  e(IdType(0), 0); 

  vector<unsigned>  counter(chunk_index_size_);
  vector<float>     projDists(num_pivot_);

  projection_->compProj(query, NULL, &projDists[0]);

  for (size_t chunkId = 0; chunkId < posting_lists_.size(); ++chunkId) {
    const auto & chunkPostLists = *posting_lists_[chunkId];

    if (chunkId != 0)
      memset(&counter[0], 0, sizeof(counter[0])*counter.size());

    for (size_t i = 0 ; i < num_pivot_; ++i) {
      // Again, pivot is the left argument, see the comment in the constructor
      e.pivot_dist_ = projDists[i];
      lowIndx[i] = (lower_bound(chunkPostLists[i].begin(), chunkPostLists[i].end(), e) - chunkPostLists[i].begin());;
      --lowIndx[i]; // Can become less than zero
      highIndx[i] = lowIndx[i] + 1;
      CHECK(chunkPostLists[i].size() > 0);
      CHECK(lowIndx[i] < 0 || static_cast<size_t>(lowIndx[i]) < chunkPostLists[i].size());
      CHECK(highIndx[i] >= 0);
    }

    bool      eof = false;
    size_t    totOp = 0;
    size_t    scannedQty = 0;
    size_t    minMatchPivotQty = max(size_t(1), static_cast<size_t>(round(min_freq_ * num_pivot_)));

    size_t minId = chunkId * chunk_index_size_;
    size_t maxId = min(data_.size(), minId + chunk_index_size_);
    size_t chunkQty = (maxId - minId);

    CHECK(chunkQty <= chunk_index_size_);

    while (scannedQty < min(db_scan_, chunkQty) && !eof) {
      eof = true;
      for (size_t i = 0 ; i < num_pivot_; ++i) {
        IdType    indx[2];
        unsigned  iQty = 0;

        if (lowIndx[i] >= 0) {
          indx[iQty++]=lowIndx[i];
          --lowIndx[i];
        }
        if (static_cast<size_t>(highIndx[i]) < chunkPostLists[i].size()) {
          indx[iQty++]=highIndx[i];
          ++highIndx[i];
        }

        for (unsigned k = 0; k < iQty; ++k) {
          ++totOp;
          IdType objIdDiff = chunkPostLists[i][indx[k]].id_;
          unsigned freq = counter[objIdDiff]++;

          if (freq == minMatchPivotQty) { // Add only the first time when we exceeded the threshold!
            ++scannedQty;
            if (!skip_check_) query->CheckAndAddToResult(data_[objIdDiff + minId]);
          } 
          eof = false;
        }
      }
    }
  }
}
 

template <typename dist_t>
void OMedRank<dist_t>::Search(RangeQuery<dist_t>* query) {
  GenSearch(query);
}

template <typename dist_t>
void OMedRank<dist_t>::Search(KNNQuery<dist_t>* query) {
  GenSearch(query);
}

template <typename dist_t>
void OMedRank<dist_t>::SetQueryTimeParamsInternal(AnyParamManager& pmgr) {
  pmgr.GetParamOptional("skipChecking", skip_check_);
  pmgr.GetParamOptional("dbScanFrac", db_scan_frac_);
  CHECK(db_scan_frac_ > 0.0);
  CHECK(db_scan_frac_ <= 1.0);
  ComputeDbScan(db_scan_frac_, index_qty_);
  pmgr.GetParamOptional("minFreq", min_freq_);
  CHECK(min_freq_ > 0.0);
  CHECK(min_freq_ <= 1.0);
}

template <typename dist_t>
vector<string>
OMedRank<dist_t>::GetQueryTimeParamNames() const {
  vector<string> names;
  names.push_back("dbScanFrac");
  names.push_back("minFreq");
  return names;
}

template <typename dist_t>
void OMedRank<dist_t>::IndexChunk(size_t chunkId) {
  size_t minId = chunkId * chunk_index_size_;
  size_t maxId = min(data_.size(), minId + chunk_index_size_);

  auto & chunkPostLists = *posting_lists_[chunkId];
  chunkPostLists.resize(num_pivot_);

  vector<float>     projDists(num_pivot_);


  for (size_t i = 0; i < maxId - minId; ++i) {
    IdType id = minId + i;

    projection_->compProj(NULL, data_[id], &projDists[0]);

    for (size_t j = 0; j < num_pivot_; ++j) {
      /* 
       * Object (in this case pivot) is the left argument.
       * At search time, the right argument of the distance will be the query point
       * and pivot again will be the left argument.
       */
      dist_t leftObjDst = projDists[j];
      chunkPostLists[j].push_back(ObjectInvEntry(id - minId, leftObjDst));
    }
  }
  for (size_t j = 0; j < num_pivot_; ++j) {
    sort(chunkPostLists[j].begin(), chunkPostLists[j].end());
  }
}

template class OMedRank<float>;
template class OMedRank<double>;
template class OMedRank<int>;

}
