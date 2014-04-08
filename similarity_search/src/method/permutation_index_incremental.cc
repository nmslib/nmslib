/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <algorithm>
#include <sstream>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "incremental_quick_select.h"
#include "permutation_index_incremental.h"
#include "utils.h"

namespace similarity {

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
PermutationIndexIncremental<dist_t, perm_func>::PermutationIndexIncremental(
    const Space<dist_t>* space,
    const ObjectVector& data,
    const size_t num_pivot,
    const double db_scan_fraction)
    : data_(data)   // reference {
       {
  CHECK(db_scan_fraction > 0.0);
  CHECK(db_scan_fraction <= 1.0);
  
  ComputeDbScan(db_scan_fraction);
  
  GetPermutationPivot(data, space, num_pivot, &pivot_);
  
#ifdef CONTIGUOUS_STORAGE
  permtable_.resize(data.size() * num_pivot);

  for (size_t i = 0, start = 0; i < data.size(); ++i, start += num_pivot) {
    Permutation TmpPerm;
    GetPermutation(pivot_, space, data[i], &TmpPerm);
    CHECK(TmpPerm.size() == num_pivot);
    memcpy(&permtable_[start], &TmpPerm[0], sizeof(permtable_[start])*num_pivot); 
  }
#else
  permtable_.resize(data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    GetPermutation(pivot_, space, data[i], &permtable_[i]);
  }
#endif
  LOG(INFO) << "# pivots         = " << num_pivot;
  LOG(INFO) << "db scan fraction = " << db_scan_fraction;
}
    
template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
void 
PermutationIndexIncremental<dist_t, perm_func>::SetQueryTimeParams(AnyParamManager& pmgr) {
  float           dbScanFrac;
  pmgr.GetParamOptional("dbScanFrac", dbScanFrac);
  
  ComputeDbScan(dbScanFrac);
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
vector<string>
PermutationIndexIncremental<dist_t, perm_func>::GetQueryTimeParamNames() const {
  vector<string> names;
  names.push_back("dbScanFrac");
  return names;
}    
    

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
PermutationIndexIncremental<dist_t, perm_func>::~PermutationIndexIncremental() {
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
const std::string PermutationIndexIncremental<dist_t, perm_func>::ToString() const {
  std::stringstream str;
  str <<  "permutation (incr. sorting)";
  return str.str();
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)> 
template <typename QueryType>
void PermutationIndexIncremental<dist_t, perm_func>::GenSearch(QueryType* query) {
  Permutation perm_q;
  GetPermutation(pivot_, query, &perm_q);

  std::vector<IntInt> perm_dists;
  perm_dists.reserve(data_.size());

#ifdef CONTIGUOUS_STORAGE
  size_t num_pivot = pivot_.size();
  for (size_t i = 0, start = 0; i < data_.size(); ++i, start += num_pivot) {
    perm_dists.push_back(std::make_pair((perm_func)(&permtable_[start], &perm_q[0], perm_q.size()), i));
  }
#else
  for (size_t i = 0; i < permtable_.size(); ++i) {
    perm_dists.push_back(std::make_pair((perm_func)(&permtable_[i][0], &perm_q[0], perm_q.size()), i));
  }
#endif
  IncrementalQuickSelect<IntInt> quick_select(perm_dists);
  for (size_t i = 0; i < db_scan_; ++i) {
    const size_t idx = quick_select.GetNext().second;
    quick_select.Next();
    query->CheckAndAddToResult(data_[idx]);
  }
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
void PermutationIndexIncremental<dist_t, perm_func>::Search(
    RangeQuery<dist_t>* query) {
  GenSearch(query);
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
void PermutationIndexIncremental<dist_t, perm_func>::Search(
    KNNQuery<dist_t>* query) {
  GenSearch(query);
}


template class PermutationIndexIncremental<float,SpearmanRho>;
template class PermutationIndexIncremental<double,SpearmanRho>;
template class PermutationIndexIncremental<int,SpearmanRho>;
template class PermutationIndexIncremental<float,SpearmanFootrule>;
template class PermutationIndexIncremental<double,SpearmanFootrule>;
template class PermutationIndexIncremental<int,SpearmanFootrule>;
template class PermutationIndexIncremental<float,SpearmanRhoSIMD>;
template class PermutationIndexIncremental<double,SpearmanRhoSIMD>;
template class PermutationIndexIncremental<int,SpearmanRhoSIMD>;
template class PermutationIndexIncremental<float,SpearmanFootruleSIMD>;
template class PermutationIndexIncremental<double,SpearmanFootruleSIMD>;
template class PermutationIndexIncremental<int,SpearmanFootruleSIMD>;

}  // namespace similarity

