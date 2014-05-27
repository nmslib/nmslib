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
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <algorithm>
#include <sstream>
#include <memory>

#include "space.h"
#include "space/space_rank_correl.h"
#include "rangequery.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "method/permutation_vptree.h"
#include "utils.h"
#include "distcomp.h"

namespace similarity {

using std::unique_ptr;

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
PermutationVPTree<dist_t, RankCorrelDistFunc>::PermutationVPTree(
    const Space<dist_t>* space,
    const ObjectVector& data,
    const AnyParams& AllParams) : 
      space_(space), data_(data),   // reference
#ifdef USE_VPTREE_SAMPLE
      VPTreeSpace_(new RankCorrelVectorSpace<RankCorrelDistFunc>())
#else
      VPTreeSpace_(new SpaceLp<float>(2))
#endif
{
  AnyParamManager pmgr(AllParams);

  db_scan_frac_        = 0.05;
  size_t    NumPivot   = 16;

  pmgr.GetParamOptional("dbScanFrac", db_scan_frac_);
  pmgr.GetParamOptional("numPivot", NumPivot);

  if (db_scan_frac_< 0.0 || db_scan_frac_> 1.0) {
    LOG(LIB_FATAL) << METH_PERMUTATION_VPTREE << " requires that dbScanFrac is in the range [0,1]";
  }

  AnyParams RemainParams;

#ifdef USE_VPTREE_SAMPLE
    bool      DoRandSample                  = true;
    unsigned  MaxK                          = 100;
    float     QuantileStepPivot             = 0.005;
    float     QuantileStepPseudoQuery       = 0.001;
    size_t    NumOfPseudoQueriesInQuantile  = 5;
    float     DistLearnThreshold            = 0.05;

    pmgr.GetParamOptional("doRandSample",            DoRandSample);
    pmgr.GetParamOptional("maxK",                    MaxK);
    pmgr.GetParamOptional("quantileStepPivot",       QuantileStepPivot);
    pmgr.GetParamOptional("quantileStepPseudoQuery", QuantileStepPseudoQuery);
    pmgr.GetParamOptional("numOfPseudoQueriesInQuantile", NumOfPseudoQueriesInQuantile);
    pmgr.GetParamOptional("distLearnThresh",         DistLearnThreshold);

    RemainParams = pmgr.ExtractParametersExcept(
                        {"dbScanFrac",
                         "numPivot",

                         "doRandSample", 
                         "maxK",
                         "quantileStepPivot",
                         "quantileStepPseudoQuery",
                         "quantileStepPseudoQuery",
                         "distLearnThresh",
                        });
#else
  double AlphaLeft = 1.0, AlphaRight = 1.0;

  pmgr.GetParamOptional("alphaLeft",  AlphaLeft);
  pmgr.GetParamOptional("alphaRight", AlphaRight);

  RemainParams = pmgr.ExtractParametersExcept(
                        { "dbScanFrac",
                         "numPivot",

                         "alphaLeft", 
                         "alphaRight",
                        });
#endif

  ComputeDbScanQty(db_scan_frac_);
  
  GetPermutationPivot(data, space, NumPivot, &pivots_);
  PermData_.resize(data.size());
#ifdef USE_VPTREE_SAMPLE
  for (size_t i = 0; i < data.size(); ++i) {
    Permutation OnePerm;
    GetPermutation(pivots_, space_, data[i], &OnePerm);
    PermData_[i] = VPTreeSpace_->CreateObjFromVect(i, OnePerm);
  }

  ReportIntrinsicDimensionality("Set of permutations" , *VPTreeSpace_, PermData_);
  
  SamplingOracleCreator<PivotIdType> OracleCreator(VPTreeSpace_,
                                                  PermData_,
                                                  DoRandSample,
                                                  MaxK,
                                                  QuantileStepPivot,
                                                  QuantileStepPseudoQuery,
                                                  NumOfPseudoQueriesInQuantile,
                                                  DistLearnThreshold);

  VPTreeIndex_ = new VPTree<PivotIdType, SamplingOracle<PivotIdType>,
                            SamplingOracleCreator<PivotIdType> >(
                                          true,
                                          OracleCreator,
                                          VPTreeSpace_,
                                          PermData_,
                                          RemainParams
                                    );
#else
  for (size_t i = 0; i < data.size(); ++i) {
    Permutation OnePerm;
    GetPermutation(pivots_, space_, data[i], &OnePerm);
    vector<float> OnePermFloat(OnePerm.size());
    for (size_t j = 0;j < OnePerm.size(); ++j) {
      OnePermFloat[j] = OnePerm[j];
    }
    PermData_[i] = VPTreeSpace_->CreateObjFromVect(i, OnePermFloat);
  }
  TriangIneqCreator<float> OracleCreator(AlphaLeft, AlphaRight);

  ReportIntrinsicDimensionality("Set of permutations" , *VPTreeSpace_, PermData_);


  VPTreeIndex_ = new VPTree<float, TriangIneq<float>,
                            TriangIneqCreator<float> >(
                                          true,
                                          OracleCreator,
                                          VPTreeSpace_,
                                          PermData_,
                                          RemainParams
                                    );
#endif
}
    
template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
void 
PermutationVPTree<dist_t, RankCorrelDistFunc>::SetQueryTimeParams(AnyParamManager& pmgr) {
  pmgr.GetParamOptional("dbScanFrac", db_scan_frac_);  
  ComputeDbScanQty(db_scan_frac_);
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
vector<string>
PermutationVPTree<dist_t, RankCorrelDistFunc>::GetQueryTimeParamNames() const {
  vector<string> names;
  names.push_back("dbScanFrac");
  return names;
}    

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
PermutationVPTree<dist_t, RankCorrelDistFunc>::~PermutationVPTree() {
  for (size_t i = 0; i < data_.size(); ++i) {
    delete PermData_[i];
  }
  delete VPTreeIndex_;
  delete VPTreeSpace_;
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
const std::string PermutationVPTree<dist_t, RankCorrelDistFunc>::ToString() const {
  std::stringstream str;
  str <<  "permutation (vptree)";
  return str.str();
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
void PermutationVPTree<dist_t, RankCorrelDistFunc>::Search(RangeQuery<dist_t>* query) {
  Permutation perm_q;
  GetPermutation(pivots_, query, &perm_q);
#ifdef USE_VPTREE_SAMPLE
  unique_ptr<Object>  QueryObject(VPTreeSpace_->CreateObjFromVect(0, perm_q));

  unique_ptr<KNNQuery<PivotIdType>> VPTreeQuery(new KNNQuery<PivotIdType>(VPTreeSpace_, QueryObject.get(), db_scan_qty_, 0.0));
#else
  vector<float> perm_qf(perm_q.size());
  for (size_t j = 0;j < perm_q.size(); ++j) {
    perm_qf[j] = perm_q[j];
  }

  unique_ptr<Object>  QueryObject(VPTreeSpace_->CreateObjFromVect(0, perm_qf));

  unique_ptr<KNNQuery<float>> VPTreeQuery(new KNNQuery<float>(VPTreeSpace_, QueryObject.get(), db_scan_qty_, 0.0));
#endif
  VPTreeIndex_->Search(VPTreeQuery.get());

#ifdef USE_VPTREE_SAMPLE
  unique_ptr<KNNQueue<PivotIdType>> ResQueue(VPTreeQuery->Result()->Clone());
#else
  unique_ptr<KNNQueue<float>> ResQueue(VPTreeQuery->Result()->Clone());
#endif

  while (!ResQueue->Empty()) {
      size_t id = reinterpret_cast<const Object*>(ResQueue->TopObject())->id();
      query->CheckAndAddToResult(data_[id]);
      ResQueue->Pop();
  }
}

template <typename dist_t, PivotIdType (*RankCorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
void PermutationVPTree<dist_t, RankCorrelDistFunc>::Search(KNNQuery<dist_t>* query) {
  Permutation perm_q;
  GetPermutation(pivots_, query, &perm_q);
#ifdef USE_VPTREE_SAMPLE
  unique_ptr<Object>  QueryObject(VPTreeSpace_->CreateObjFromVect(0, perm_q));

  unique_ptr<KNNQuery<PivotIdType>> VPTreeQuery(new KNNQuery<PivotIdType>(VPTreeSpace_, QueryObject.get(), db_scan_qty_, query->GetEPS()));
#else
  vector<float> perm_qf(perm_q.size());
  for (size_t j = 0;j < perm_q.size(); ++j) {
    perm_qf[j] = perm_q[j];
  }

  unique_ptr<Object>  QueryObject(VPTreeSpace_->CreateObjFromVect(0, perm_qf));

  unique_ptr<KNNQuery<float>> VPTreeQuery(new KNNQuery<float>(VPTreeSpace_, QueryObject.get(), db_scan_qty_, 0.0));
#endif
  VPTreeIndex_->Search(VPTreeQuery.get());

#ifdef USE_VPTREE_SAMPLE
  unique_ptr<KNNQueue<PivotIdType>> ResQueue(VPTreeQuery->Result()->Clone());
#else
  unique_ptr<KNNQueue<float>> ResQueue(VPTreeQuery->Result()->Clone());
#endif

  while (!ResQueue->Empty()) {
      size_t id = reinterpret_cast<const Object*>(ResQueue->TopObject())->id();
      query->CheckAndAddToResult(data_[id]);
      ResQueue->Pop();
  }
}

template class PermutationVPTree<float, SpearmanRho>;
template class PermutationVPTree<float, SpearmanRhoSIMD>;
template class PermutationVPTree<float, SpearmanFootrule>;
template class PermutationVPTree<float, SpearmanFootruleSIMD>;
template class PermutationVPTree<double, SpearmanRho>;
template class PermutationVPTree<double, SpearmanRhoSIMD>;
template class PermutationVPTree<double, SpearmanFootrule>;
template class PermutationVPTree<double, SpearmanFootruleSIMD>;
template class PermutationVPTree<int, SpearmanRho>;
template class PermutationVPTree<int, SpearmanRhoSIMD>;
template class PermutationVPTree<int, SpearmanFootrule>;
template class PermutationVPTree<int, SpearmanFootruleSIMD>;

}  // namespace similarity

