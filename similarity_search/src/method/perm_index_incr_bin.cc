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
#include <memory>

#include "space.h"
#include "ported_boost_progress.h"
#include "rangequery.h"
#include "knnquery.h"
#include "incremental_quick_select.h"
#include "method/perm_index_incr_bin.h"
#include "utils.h"

namespace similarity {

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
PermutationIndexIncrementalBin<dist_t, perm_func>::PermutationIndexIncrementalBin(
    bool PrintProgress,
    const Space<dist_t>& space,
    const ObjectVector& data) 
    : Index<dist_t>(data), space_(space), PrintProgress_(PrintProgress) {}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
void PermutationIndexIncrementalBin<dist_t, perm_func>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("numPivot",     num_pivot_, 16);
  pmgr.GetParamOptional("binThreshold", bin_threshold_, num_pivot_ / 2);

  bin_perm_word_qty_ = (num_pivot_ + 31)/32,

  pmgr.CheckUnused();
  this->ResetQueryTimeParams();

  LOG(LIB_INFO) << "# pivots                  = " << num_pivot_;
  LOG(LIB_INFO) << "# binarization threshold = "  << bin_threshold_;
  LOG(LIB_INFO) << "# binary entry size (words) = "  << bin_perm_word_qty_;

  GetPermutationPivot(this->data_, space_, num_pivot_, &pivot_);

  permtable_.resize(this->data_.size() * bin_perm_word_qty_);

  unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                                new ProgressDisplay(this->data_.size(), cerr)
                                :NULL);

  for (size_t i = 0, start = 0; i < this->data_.size(); ++i, start += bin_perm_word_qty_) {
    Permutation TmpPerm;
    GetPermutation(pivot_, space_, this->data_[i], &TmpPerm);
    CHECK(TmpPerm.size() == num_pivot_);
    vector<uint32_t>  binPivot;
    Binarize(TmpPerm, bin_threshold_, binPivot);
    CHECK(binPivot.size() == bin_perm_word_qty_);
    memcpy(&permtable_[start], &binPivot[0], bin_perm_word_qty_ * sizeof(binPivot[0]));
    if (progress_bar) ++(*progress_bar);
  }
  //SavePermTable(permtable_, "permtab");
}
    
template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
void PermutationIndexIncrementalBin<dist_t, perm_func>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);

  pmgr.GetParamOptional("skipChecking", skip_checking_,     false);
  pmgr.GetParamOptional("useSort",      use_sort_,          true);
  pmgr.GetParamOptional("maxHammingDist", max_hamming_dist_, num_pivot_);

    
  if (pmgr.hasParam("dbScanFrac") && pmgr.hasParam("knnAmp")) {
    throw runtime_error("One shouldn't specify both parameters dbScanFrac and knnAmp");
  }
  
  pmgr.GetParamOptional("dbScanFrac",   db_scan_frac_,  0.05);
  pmgr.GetParamOptional("knnAmp",       knn_amp_,       0);

  pmgr.CheckUnused();

  LOG(LIB_INFO) << "Set query-time parameters fro PermutationIndexIncrementalBin:";
  LOG(LIB_INFO) << "use sort = " << use_sort_;
  if (use_sort_) {
    LOG(LIB_INFO) << "db scan fraction = " << db_scan_frac_;
  } else {
    LOG(LIB_INFO) << "max hamming distance = " << max_hamming_dist_;
  }
  LOG(LIB_INFO) << "skip checking = " << skip_checking_;
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
PermutationIndexIncrementalBin<dist_t, perm_func>::~PermutationIndexIncrementalBin() {
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
const std::string PermutationIndexIncrementalBin<dist_t, perm_func>::StrDesc() const {
  std::stringstream str;
  str <<  "permutation binarized (incr. sorting)";
  return str.str();
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)> 
template <typename QueryType>
void PermutationIndexIncrementalBin<dist_t, perm_func>::GenSearch(QueryType* query, size_t K) const {
  // Let's make this check here. Otherwise, if you misspell dbScanFrac, you will get 
  // a strange error message that says: dbScanFrac should be in the range [0,1].
  if (!knn_amp_) {
    if (db_scan_frac_ < 0.0 || db_scan_frac_ > 1.0) {
      stringstream err;
      err << METH_PERMUTATION_INC_SORT_BIN << " requires that dbScanFrac is in the range [0,1]";
      throw runtime_error(err.str());
    }
  }

  size_t db_scan = computeDbScan(K);


  Permutation perm_q;
  GetPermutation(pivot_, query, &perm_q);
  vector<uint32_t>  binPivot;
  Binarize(perm_q, bin_threshold_, binPivot);

  std::vector<IntInt> perm_dists;
  perm_dists.reserve(this->data_.size());

  if (use_sort_) {
    for (size_t i = 0, start = 0; i < this->data_.size(); ++i, start += bin_perm_word_qty_) {
      perm_dists.push_back(std::make_pair(BitHamming(&permtable_[start], &binPivot[0], bin_perm_word_qty_), i));
    }

    IncrementalQuickSelect<IntInt> quick_select(perm_dists);
    for (size_t i = 0; i < db_scan; ++i) {
      const size_t idx = quick_select.GetNext().second;
      quick_select.Next();
      if (!skip_checking_) query->CheckAndAddToResult(this->data_[idx]);
    }
  } else {
    for (size_t i = 0, start = 0; i < this->data_.size(); ++i, start += bin_perm_word_qty_) {
      if (BitHamming(&permtable_[start], &binPivot[0], bin_perm_word_qty_) < max_hamming_dist_) {
        if (!skip_checking_) query->CheckAndAddToResult(this->data_[i]);
      }
    }
  }
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
void PermutationIndexIncrementalBin<dist_t, perm_func>::Search(RangeQuery<dist_t>* query, IdType) const {
  GenSearch(query, 0);
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
void PermutationIndexIncrementalBin<dist_t, perm_func>::Search(KNNQuery<dist_t>* query, IdType) const {
  GenSearch(query, query->GetK());
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

