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
#include "perm_index_incr_bin.h"
#include "utils.h"

namespace similarity {

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
PermutationIndexIncrementalBin<dist_t, perm_func>::PermutationIndexIncrementalBin(
    const Space<dist_t>* space,
    const ObjectVector& data,
    const size_t num_pivot,
    const size_t bin_threshold,
    const double db_scan_fraction,
    const size_t max_hamming_dist,
    const bool use_sort,
    const bool skip_checking)
    : data_(data),   // reference
      bin_threshold_(bin_threshold),
      db_scan_(max(size_t(1), static_cast<size_t>(db_scan_fraction * data.size()))),
      bin_perm_word_qty_((num_pivot + 31)/32),
      use_sort_(use_sort),
      max_hamming_dist_(max_hamming_dist),
      skip_checking_(skip_checking) {
  CHECK(db_scan_fraction > 0.0);
  CHECK(db_scan_fraction <= 1.0);

  GetPermutationPivot(data, space, num_pivot, &pivot_);

  permtable_.resize(data.size() * bin_perm_word_qty_);

  for (size_t i = 0, start = 0; i < data.size(); ++i, start += bin_perm_word_qty_) {
    Permutation TmpPerm;
    GetPermutation(pivot_, space, data[i], &TmpPerm);
    CHECK(TmpPerm.size() == num_pivot);
    vector<uint32_t>  binPivot;
    Binarize(TmpPerm, bin_threshold_, binPivot);
    CHECK(binPivot.size() == bin_perm_word_qty_);
    memcpy(&permtable_[start], &binPivot[0], bin_perm_word_qty_ * sizeof(binPivot[0]));
  }

  LOG(INFO) << "# pivots                  = " << num_pivot;
  LOG(INFO) << "# binarization threshold = "  << bin_threshold_;
  LOG(INFO) << "# binary entry size (words) = "  << bin_perm_word_qty_;
  LOG(INFO) << "use sort = " << use_sort_;
  if (use_sort_) {
    LOG(INFO) << "db scan fraction = " << db_scan_fraction;
  } else {
    LOG(INFO) << "max hamming distance = " << max_hamming_dist_;
  }
  LOG(INFO) << "skip checking = " << skip_checking_;
  //SavePermTable(permtable_, "permtab");
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
PermutationIndexIncrementalBin<dist_t, perm_func>::~PermutationIndexIncrementalBin() {
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
const std::string PermutationIndexIncrementalBin<dist_t, perm_func>::ToString() const {
  std::stringstream str;
  str <<  "permutation binarized (incr. sorting)";
  return str.str();
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)> 
template <typename QueryType>
void PermutationIndexIncrementalBin<dist_t, perm_func>::GenSearch(QueryType* query) {
  Permutation perm_q;
  GetPermutation(pivot_, query, &perm_q);
  vector<uint32_t>  binPivot;
  Binarize(perm_q, bin_threshold_, binPivot);

  std::vector<IntInt> perm_dists;
  perm_dists.reserve(data_.size());

  if (use_sort_) {
    for (size_t i = 0, start = 0; i < data_.size(); ++i, start += bin_perm_word_qty_) {
      perm_dists.push_back(std::make_pair(BitHamming(&permtable_[start], &binPivot[0], bin_perm_word_qty_), i));
    }

    IncrementalQuickSelect<IntInt> quick_select(perm_dists);
    for (size_t i = 0; i < db_scan_; ++i) {
      const size_t idx = quick_select.GetNext().second;
      quick_select.Next();
      if (!skip_checking_) query->CheckAndAddToResult(data_[idx]);
    }
  } else {
    for (size_t i = 0, start = 0; i < data_.size(); ++i, start += bin_perm_word_qty_) {
      if (BitHamming(&permtable_[start], &binPivot[0], bin_perm_word_qty_) < max_hamming_dist_) {
        if (!skip_checking_) query->CheckAndAddToResult(data_[i]);
      }
    }
  }
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
void PermutationIndexIncrementalBin<dist_t, perm_func>::Search(
    RangeQuery<dist_t>* query) {
  GenSearch(query);
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
void PermutationIndexIncrementalBin<dist_t, perm_func>::Search(
    KNNQuery<dist_t>* query) {
  GenSearch(query);
}


template class PermutationIndexIncrementalBin<float,SpearmanRho>;
template class PermutationIndexIncrementalBin<double,SpearmanRho>;
template class PermutationIndexIncrementalBin<int,SpearmanRho>;
template class PermutationIndexIncrementalBin<float,SpearmanFootrule>;
template class PermutationIndexIncrementalBin<double,SpearmanFootrule>;
template class PermutationIndexIncrementalBin<int,SpearmanFootrule>;
template class PermutationIndexIncrementalBin<float,SpearmanRhoSIMD>;
template class PermutationIndexIncrementalBin<double,SpearmanRhoSIMD>;
template class PermutationIndexIncrementalBin<int,SpearmanRhoSIMD>;
template class PermutationIndexIncrementalBin<float,SpearmanFootruleSIMD>;
template class PermutationIndexIncrementalBin<double,SpearmanFootruleSIMD>;
template class PermutationIndexIncrementalBin<int,SpearmanFootruleSIMD>;

}  // namespace similarity

