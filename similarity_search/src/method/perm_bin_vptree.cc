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
#include "space/space_bit_hamming.h"
#include "rangequery.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "method/perm_bin_vptree.h"
#include "utils.h"
#include "distcomp.h"
#include "logging.h"
#include "report_intr_dim.h"

namespace similarity {

using std::unique_ptr;

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
PermBinVPTree<dist_t, RankCorrelDistFunc>::PermBinVPTree(
    bool PrintProgress,
    Space<dist_t>& space,
    const ObjectVector& data) : 
      Index<dist_t>(data), space_(space), 
      PrintProgress_(PrintProgress),
      VPTreeSpace_(new SpaceBitHamming())
{}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
void PermBinVPTree<dist_t, RankCorrelDistFunc>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);

  size_t        NumPivot;

  pmgr.GetParamOptional("numPivot",   NumPivot, 16);
  pmgr.GetParamOptional("binThreshold", bin_threshold_, NumPivot / 2);

  bin_perm_word_qty_ = (NumPivot + 31)/32;


  LOG(LIB_INFO) << "NumPivot     = " << NumPivot;
  LOG(LIB_INFO) << "binThreshold = "  << bin_threshold_;
  LOG(LIB_INFO) << "# binary entry size (words) = "  << bin_perm_word_qty_;

  AnyParams RemainParams = pmgr.ExtractParametersExcept({ "numPivot", "binThreshold"});


  GetPermutationPivot(this->data_, space_, NumPivot, &pivots_);
  BinPermData_.resize(this->data_.size());

  for (size_t i = 0; i < this->data_.size(); ++i) {
    Permutation TmpPerm;
    GetPermutation(pivots_, space_, this->data_[i], &TmpPerm);
    vector<uint32_t>  binPivot;
    Binarize(TmpPerm, bin_threshold_, binPivot);
    CHECK(binPivot.size() == bin_perm_word_qty_);
    BinPermData_[i] = VPTreeSpace_->CreateObjFromVect(i, -1, binPivot);
  }

  vector<double> dist;
  ReportIntrinsicDimensionality("Set of permutations" , *VPTreeSpace_, BinPermData_, dist);
  VPTreeIndex_.reset(new VPTree<int, PolynomialPruner<int>>(
                                          PrintProgress_,
                                          *VPTreeSpace_,
                                          BinPermData_));

  VPTreeIndex_->CreateIndex(RemainParams);
  // Reset parameters only after the VP-tree index is created!
  this->ResetQueryTimeParams();
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
void PermBinVPTree<dist_t, RankCorrelDistFunc>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);

  AnyParams vptreeQueryParams = pmgr.ExtractParameters(VPTreeIndex_->getQueryTimeParams());
  VPTreeIndex_->SetQueryTimeParams(vptreeQueryParams);

  float dbScanFrac = 0;

  pmgr.GetParamOptional("dbScanFrac",   dbScanFrac, 0.05);

  if (dbScanFrac < 0.0 || dbScanFrac > 1.0) {
    PREPARE_RUNTIME_ERR(err) << METH_PERM_BIN_VPTREE << " requires that dbScanFrac is in the range [0,1]";
    THROW_RUNTIME_ERR(err);
  }


  LOG(LIB_INFO) << "Set query-time parameters for PermBinVPTree:";
  LOG(LIB_INFO) << "dbScanFrac=" << dbScanFrac;

  db_scan_qty_ = max(size_t(1), static_cast<size_t>(dbScanFrac * this->data_.size()));

  LOG(LIB_INFO) << "db_scan_qty_=" << db_scan_qty_;

  pmgr.CheckUnused();
}


template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
PermBinVPTree<dist_t, RankCorrelDistFunc>::~PermBinVPTree() {
  for (size_t i = 0; i < this->data_.size(); ++i) {
    delete BinPermData_[i];
  }
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
const std::string PermBinVPTree<dist_t, RankCorrelDistFunc>::StrDesc() const {
  std::stringstream str;
  str <<  "binarized permutation (vptree)";
  return str.str();
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
void PermBinVPTree<dist_t, RankCorrelDistFunc>::Search(RangeQuery<dist_t>* query, IdType) const {
  Permutation perm_q;
  GetPermutation(pivots_, query, &perm_q);

  vector<uint32_t>  binPivot;
  Binarize(perm_q, bin_threshold_, binPivot);
  CHECK(binPivot.size() == bin_perm_word_qty_);

  unique_ptr<Object>  QueryObject(VPTreeSpace_->CreateObjFromVect(0, -1, binPivot));

  unique_ptr<KNNQuery<int>> VPTreeQuery(new KNNQuery<int>(*VPTreeSpace_, QueryObject.get(), db_scan_qty_, 0.0));

  VPTreeIndex_->Search(VPTreeQuery.get(), -1);

  unique_ptr<KNNQueue<int>> ResQueue(VPTreeQuery->Result()->Clone());

  while (!ResQueue->Empty()) {
      size_t id = reinterpret_cast<const Object*>(ResQueue->TopObject())->id();
      query->CheckAndAddToResult(this->data_[id]);
      ResQueue->Pop();
  }
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
void PermBinVPTree<dist_t, RankCorrelDistFunc>::Search(KNNQuery<dist_t>* query, IdType) const {
  Permutation perm_q;
  GetPermutation(pivots_, query, &perm_q);

  vector<uint32_t>  binPivot;
  Binarize(perm_q, bin_threshold_, binPivot);
  CHECK(binPivot.size() == bin_perm_word_qty_);

  unique_ptr<Object>  QueryObject(VPTreeSpace_->CreateObjFromVect(0, -1, binPivot));

  unique_ptr<KNNQuery<int>> VPTreeQuery(new KNNQuery<int>(*VPTreeSpace_, QueryObject.get(), db_scan_qty_, 0.0));

  VPTreeIndex_->Search(VPTreeQuery.get(), -1);

  unique_ptr<KNNQueue<int>> ResQueue(VPTreeQuery->Result()->Clone());

  while (!ResQueue->Empty()) {
      size_t id = reinterpret_cast<const Object*>(ResQueue->TopObject())->id();
      query->CheckAndAddToResult(this->data_[id]);
      ResQueue->Pop();
  }
}

template class PermBinVPTree<float, SpearmanRho>;
template class PermBinVPTree<float, SpearmanRhoSIMD>;
template class PermBinVPTree<float, SpearmanFootrule>;
template class PermBinVPTree<double, SpearmanRho>;
template class PermBinVPTree<double, SpearmanRhoSIMD>;
template class PermBinVPTree<double, SpearmanFootrule>;
template class PermBinVPTree<int, SpearmanRho>;
template class PermBinVPTree<int, SpearmanRhoSIMD>;
template class PermBinVPTree<int, SpearmanFootrule>;

}  // namespace similarity

