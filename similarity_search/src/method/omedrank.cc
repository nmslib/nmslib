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
    const size_t num_pivot,
    AnyParamManager &pmgr) 
	: data_(data)  /* reference */ {
  SetQueryTimeParamsInternal(pmgr);
  LOG(LIB_INFO) << "# pivots         = " << num_pivot;
  LOG(LIB_INFO) << "db scan fraction = " << db_scan_frac_;
  LOG(LIB_INFO) << "min freq = "         << min_freq_;
  CHECK(data_.size() > num_pivot);

  GetPermutationPivot(data, space, num_pivot, &pivot_);

  posting_lists_.resize(num_pivot);

  for (size_t id = 0; id < data.size(); ++id) {
    for (size_t j = 0; j < num_pivot; ++j) {
      /* 
       * Object (in this case pivot) is the left argument.
       * At search time, the right argument of the distance will be the query point
       * and pivot again will be the left argument.
       */
      dist_t leftObjDst = space->IndexTimeDistance(pivot_[j], data[id]);
      posting_lists_[j].push_back(ObjectInvEntry(id, leftObjDst));
    }
  }
  for (size_t j = 0; j < posting_lists_.size(); ++j) {
    sort(posting_lists_[j].begin(), posting_lists_[j].end());
  }
}

template <typename dist_t> 
template <typename QueryType> 
void OMedRank<dist_t>::GenSearch(QueryType* query) {
  size_t num_pivot = pivot_.size();
  vector<ssize_t> lowIndx(num_pivot);
  vector<ssize_t> highIndx(num_pivot);

  ObjectInvEntry  e(IdType(0), 0); 

  unordered_map<IdType, unsigned> visited;


  for (size_t i = 0 ; i < num_pivot; ++i) {
    // Again, pivot is the left argument, see the comment in the constructor
    e.pivot_dist_ = query->DistanceObjLeft(pivot_[i]); 
    lowIndx[i] = (lower_bound(posting_lists_[i].begin(), posting_lists_[i].end(), e) - posting_lists_[i].begin());;
    --lowIndx[i]; // Can become less than zero
    highIndx[i] = lowIndx[i] + 1;
    CHECK(posting_lists_[i].size() > 0);
    CHECK(lowIndx[i] < posting_lists_[i].size());
    CHECK(highIndx[i] >= 0);
  }


  bool      eof = false;
  size_t    totOp = 0;
  size_t    scannedQty = 0;
  size_t    minMatchPivotQty = max(size_t(1), static_cast<size_t>(min_freq_ * num_pivot));

  while (scannedQty < db_scan_ && !eof) {
    eof = true;
    for (size_t i = 0 ; i < num_pivot; ++i) {
      IdType    indx[2];
      unsigned  iQty = 0;

      if (lowIndx[i] >= 0) {
        indx[iQty++]=lowIndx[i];
        --lowIndx[i];
      }
      if (highIndx[i] < posting_lists_[i].size()) {
        indx[iQty++]=highIndx[i];
        ++highIndx[i];
      }

      for (int k = 0; k < iQty; ++k) {
        ++totOp;
        IdType objId = posting_lists_[i][indx[k]].id_;
        unsigned freq = 1;
        auto it = visited.find(objId);
        if (it != visited.end()) {
          freq = it -> second + 1;
        }
        visited[objId] = freq;
        if (freq >= minMatchPivotQty && freq < minMatchPivotQty + 1) { // Add only the first time when we exceeded the threshold!
          ++scannedQty;
          query->CheckAndAddToResult(data_[objId]);
        } 
        eof = false;
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
  pmgr.GetParamOptional("dbScanFrac", db_scan_frac_);
  CHECK(db_scan_frac_ > 0.0);
  CHECK(db_scan_frac_ <= 1.0);
  ComputeDbScan(db_scan_frac_);
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

template class OMedRank<float>;
template class OMedRank<double>;
template class OMedRank<int>;

}
