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
#include "method/permutation_index.h"
#include "utils.h"

namespace similarity {

template <typename dist_t>
PermutationIndex<dist_t>::PermutationIndex(
    const Space<dist_t>* space,
    const ObjectVector& data,
    const size_t num_pivot,
    const double db_scan_fraction,
    const IntDistFuncPtr permfunc)
    : data_(data),   // reference
      db_scan_(max(size_t(1), static_cast<size_t>(db_scan_fraction * data.size()))),
      permfunc_(permfunc) {
  CHECK(db_scan_fraction > 0.0);
  CHECK(db_scan_fraction <= 1.0);
  CHECK(permfunc != NULL);
  GetPermutationPivot(data, space, num_pivot, &pivot_);
  permtable_.resize(data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    GetPermutation(pivot_, space, data[i], &permtable_[i]);
  }
  LOG(INFO) << "# pivots         = " << num_pivot;
  LOG(INFO) << "db scan fraction = " << db_scan_fraction;
}

template <typename dist_t>
PermutationIndex<dist_t>::~PermutationIndex() {
}

template <typename dist_t>
const std::string PermutationIndex<dist_t>::ToString() const {
  std::stringstream str;
  str <<  "permutation";
  return str.str();
}

template <typename dist_t> 
template <typename QueryType>
void PermutationIndex<dist_t>::GenSearch(QueryType* query) {
    Permutation perm_q;
    GetPermutation(pivot_, query, &perm_q);
    std::vector<IntInt> perm_dists;
    for (size_t i = 0; i < permtable_.size(); ++i) {
      perm_dists.push_back(std::make_pair((*permfunc_)(&permtable_[i][0], &perm_q[0], perm_q.size()), i)); 
    }
    std::sort(perm_dists.begin(), perm_dists.end());
    for (size_t i = 0; i < db_scan_; ++i) {
      const size_t idx = perm_dists[i].second;
      query->CheckAndAddToResult(data_[idx]);
    }
}

template <typename dist_t>
void PermutationIndex<dist_t>::Search(RangeQuery<dist_t>* query) {
  GenSearch(query);
}

template <typename dist_t>
void PermutationIndex<dist_t>::Search(KNNQuery<dist_t>* query) {
  GenSearch(query);
}

template class PermutationIndex<float>;
template class PermutationIndex<double>;
template class PermutationIndex<int>;

}  // namespace similarity

