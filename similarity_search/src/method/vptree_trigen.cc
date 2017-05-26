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
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "searchoracle.h"
#include "method/vptree_trigen.h"
#include "method/vptree_utils.h"
#include "methodfactory.h"

#include "simdutils.h"

#include "trigen/cTriGen.h"
#include "trigen/cRBQModifier.h"
#include "trigen/cApproximatedModifier.h"

#define MIN_PIVOT_SELECT_DATA_QTY 10
#define MAX_PIVOT_SELECT_ATTEMPTS 5


namespace similarity {

using std::string;
using std::stringstream;
using std::endl;
using std::cout;
using std::cerr;
    
template <typename dist_t, typename SearchOracle>
VPTreeTrigen<dist_t, SearchOracle>::VPTreeTrigen(
                       bool  PrintProgress,
                       Space<dist_t>& space,
                       const ObjectVector& data,
                       bool use_random_center) : 
                              space_(space),
                              data_(data),
                              PrintProgress_(PrintProgress),
                              use_random_center_(use_random_center),
                              max_pivot_select_attempts_(MAX_PIVOT_SELECT_ATTEMPTS),
                              oracle_(space, data, PrintProgress),
                              QueryTimeParams_(oracle_.GetQueryTimeParamNames()) { 
                                QueryTimeParams_.push_back("maxLeavesToVisit");
                              }

template <typename dist_t, typename SearchOracle>
void VPTreeTrigen<dist_t, SearchOracle>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("bucketSize", BucketSize_, 50);
  pmgr.GetParamOptional("chunkBucket", ChunkBucket_, true);
  pmgr.GetParamOptional("selectPivotAttempts", max_pivot_select_attempts_, MAX_PIVOT_SELECT_ATTEMPTS);
  pmgr.GetParamRequired("trigenAcc", TrigenAcc_);
  pmgr.GetParamOptional("trigenSampleQty", TrigenSampleQty_, 5000);
  pmgr.GetParamOptional("trigenSampleTripletQty", TrigenSampleTripletQty_, 1000000);
  pmgr.GetParamOptional("isSymmetrDist", isSymmetrDist_, true);

  CHECK_MSG(max_pivot_select_attempts_ >= 1, "selectPivotAttempts should be >=1");

  LOG(LIB_INFO) << "bucketSize          = " << BucketSize_;
  LOG(LIB_INFO) << "chunkBucket         = " << ChunkBucket_;
  LOG(LIB_INFO) << "selectPivotAttempts = " << max_pivot_select_attempts_;
  LOG(LIB_INFO) << "trigenAcc           = " << TrigenAcc_;
  LOG(LIB_INFO) << "trigenSampleQty     = " << TrigenSampleQty_;
  LOG(LIB_INFO) << "trigenSampleTripletQty= " << TrigenSampleTripletQty_;
  LOG(LIB_INFO) << "isSymmetrDist=         " << isSymmetrDist_;

  // Trigen must use the standard metric oracle, so we don't pass any
  // parameters to the oracle (it will use default, i.e., metric ones).
  // Call this function *ONLY AFTER* the bucket size is obtained!
  //oracle_.SetIndexTimeParams(pmgr);
  //oracle_.LogParams();

  pmgr.CheckUnused();

  this->ResetQueryTimeParams(); // reset query-time parameters

  BuildTrigen();

  unique_ptr<ProgressDisplay>   progress_bar(PrintProgress_ ? 
                                              new ProgressDisplay(data_.size(), cerr):
                                              NULL);

  root_.reset(new VPNode(0,
                     progress_bar.get(), 
                     oracle_, 
                     space_, data_,
                     *resultModifier_, *distWrapper_,
                     max_pivot_select_attempts_,
                     BucketSize_, ChunkBucket_,
                     use_random_center_ /* use random center */));

  if (progress_bar) { // make it 100%
    (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
  }
}

template <typename dist_t,typename SearchOracle>
void VPTreeTrigen<dist_t, SearchOracle>::BuildTrigen() {
  // This code is modelled after makeTrigen.h from Tomas Skopal TriGenLite
	AllModifiers_.push_back(new cFractionalPowerModifier(0));	
	double stepA = 0.0025;
	double stepB = 0.05;

/*
  // Create a list of modifiers
	for(double a = 0; a < 1; a += stepA) {
		for(double b = a + stepB; b < 1; b += stepB) {
			AllModifiers_.push_back(new cRBQModifier(a,b));
		}
  }
*/

  distWrapper_.reset(new DistWrapper<dist_t>(space_, data_, isSymmetrDist_));

  trigen_.reset(new cTriGen(*distWrapper_, data_, TrigenSampleQty_, AllModifiers_));

	ObjectVector trigenSampledObjects;
	trigen_->GetSampledItems(trigenSampledObjects);

  double idim;
  unsigned int funcOrder;
  double error = 1-TrigenAcc_;
  resultModifier_ = trigen_->Run(error, funcOrder, idim, TrigenSampleTripletQty_);

  CHECK_MSG(resultModifier_ != nullptr, "Failed to find a trigen modifier with the given accuracy!");

  if (funcOrder==0)
    LOG(LIB_INFO) << "Result: fractional Power Modifier";
  else
    LOG(LIB_INFO) << "Result: RBQ Modifier, a = " <<  ((cRBQModifier*)resultModifier_)->GetA() << 
                     ", b = ", ((cRBQModifier*)resultModifier_)->GetB();
  LOG(LIB_INFO) << "Triangular Error: " << error << 
                    "  CW: " <<  resultModifier_->GetConcavityWeight() << 
                    " IDim: " << idim;

}

template <typename dist_t,typename SearchOracle>
VPTreeTrigen<dist_t, SearchOracle>::~VPTreeTrigen() {
  for (auto e : AllModifiers_) delete e;
}

template <typename dist_t,typename SearchOracle>
const std::string VPTreeTrigen<dist_t, SearchOracle>::StrDesc() const {
  return "vptree: " + SearchOracle::GetName();
}

template <typename dist_t, typename SearchOracle>
void VPTreeTrigen<dist_t, SearchOracle>::Search(RangeQuery<dist_t>* query, IdType) const {
  int mx = MaxLeavesToVisit_;
  double queryRadius=numeric_limits<dist_t>::max();
  root_->GenericSearch(query, queryRadius, *resultModifier_, *distWrapper_, mx);
}

template <typename dist_t, typename SearchOracle>
void VPTreeTrigen<dist_t, SearchOracle>::Search(KNNQuery<dist_t>* query, IdType) const {
  int mx = MaxLeavesToVisit_;
  double queryRadius=numeric_limits<dist_t>::max();
  root_->GenericSearch(query, queryRadius, *resultModifier_, *distWrapper_, mx);
}

template <typename dist_t, typename SearchOracle>
void VPTreeTrigen<dist_t, SearchOracle>::VPNode::CreateBucket(bool ChunkBucket, 
                                                        const ObjectVector& data, 
                                                        ProgressDisplay* progress_bar) {
    if (ChunkBucket) {
      CreateCacheOptimizedBucket(data, CacheOptimizedBucket_, bucket_);
    } else {
      bucket_ = new ObjectVector(data);
    }
    if (progress_bar) (*progress_bar) += data.size();
}

template <typename dist_t, typename SearchOracle>
VPTreeTrigen<dist_t, SearchOracle>::VPNode::VPNode(
                               unsigned level,
                               ProgressDisplay* progress_bar,
                               const SearchOracle& oracle,
                               const Space<dist_t>& space, const ObjectVector& data,
                               cSPModifier& resultModifier, DistWrapper<dist_t>& distWrapper,
                               size_t max_pivot_select_attempts,
                               size_t BucketSize, bool ChunkBucket,
                               bool use_random_center)
    : oracle_(oracle),
      pivot_(NULL), mediandist_(0),
      left_child_(NULL), right_child_(NULL),
      bucket_(NULL), CacheOptimizedBucket_(NULL)
{
  CHECK(!data.empty());

  if (!data.empty() && data.size() <= BucketSize) {
    CreateBucket(ChunkBucket, data, progress_bar);
    return;
  }


  if (data.size() >= 2) {
    unsigned bestDP = 0;
    float    largestSIGMA = 0;
    vector<DistObjectPairVector<dist_t>> dpARR(max_pivot_select_attempts);

    // To compute StdDev we need at least 2 points not counting the pivot
    size_t maxAtt = data.size() >= max(3, MIN_PIVOT_SELECT_DATA_QTY) ? max_pivot_select_attempts : 1;

    vector<double> dists(data.size());
    for (size_t att = 0; att < maxAtt; ++att) {
      dpARR[att].reserve(data.size());
      const size_t  currPivotIndex = SelectVantagePoint(data, use_random_center);
      const Object* pCurrPivot = data[currPivotIndex];
      for (size_t i = 0; i < data.size(); ++i) {
        if (i == currPivotIndex) {
          continue;
        }
        // Distance can be asymmetric, the pivot is always on the left side!
        dist_t d = resultModifier.ComputeModification(distWrapper.Compute(pCurrPivot, data[i]));
        dists[i] = d;
        dpARR[att].emplace_back(d, data[i]);
      }

      double sigma = StdDev(&dists[0], dists.size());
      if (att == 0 || sigma > largestSIGMA) {
        //LOG(LIB_INFO) << " ### " << largestSIGMA << " -> "  << sigma << " att=" << att << " data.size()=" << data.size();
        largestSIGMA = sigma;
        bestDP = att;
        pivot_ = pCurrPivot;
        std::sort(dpARR[att].begin(), dpARR[att].end(), DistObjectPairAscComparator<dist_t>());
      }
    }


    DistObjectPairVector<dist_t>& dp = dpARR[bestDP];
    DistObjectPair<dist_t>  medianDistObj = GetMedian(dp);
    mediandist_ = medianDistObj.first; 

    ObjectVector left;
    ObjectVector right;

    for (auto it = dp.begin(); it != dp.end(); ++it) {
      const Object* v = it->second;

      /* 
       * Note that here we compare a pair (distance, pointer)
       * If distances are equal, pointers are compared.
       * Thus, we would get a balanced split, even if the median
       * occurs many times in the array dp[].
       */
      if (*it < medianDistObj) {
        left.push_back(v);
      } else {
        right.push_back(v);
      }
    }

    /*
     * Sometimes, e.g.., for integer-valued distances,
     * mediandist_ will be non-discriminative. In this case
     * it is more efficient to put everything into a single bucket.
     */
    size_t LeastSize = dp.size() / BalanceConst;

    if (left.size() < LeastSize || right.size() < LeastSize) {
        CreateBucket(ChunkBucket, data, progress_bar);
        return;
    }

    if (!left.empty()) {
      left_child_ = new VPNode(level + 1, progress_bar, oracle_, space, left, 
                               resultModifier, distWrapper, 
                               max_pivot_select_attempts, BucketSize, ChunkBucket, use_random_center);
    }

    if (!right.empty()) {
      right_child_ = new VPNode(level + 1, progress_bar, oracle_, space, right, 
                               resultModifier, distWrapper, 
                               max_pivot_select_attempts, BucketSize, ChunkBucket, use_random_center);
    }
  } else {
    CHECK_MSG(data.size() == 1, "Bug: expect the subset to contain exactly one element!");
    pivot_ = data[0];
  }
}

template <typename dist_t, typename SearchOracle>
VPTreeTrigen<dist_t, SearchOracle>::VPNode::~VPNode() {
  delete left_child_;
  delete right_child_;
  ClearBucket(CacheOptimizedBucket_, bucket_);
}

template <typename dist_t, typename SearchOracle>
template <typename QueryType>
void VPTreeTrigen<dist_t, SearchOracle>::VPNode::GenericSearch(QueryType* query,
                                                         double&    queryRadius,
                                                         cSPModifier& resultModifier, 
                                                         DistWrapper<dist_t>& distWrapper,
                                                         int& MaxLeavesToVisit) const {
  if (MaxLeavesToVisit <= 0) return; // early termination
  if (bucket_) {
    --MaxLeavesToVisit;

    if (CacheOptimizedBucket_) {
      _mm_prefetch(CacheOptimizedBucket_, _MM_HINT_T0);
    }

    for (unsigned i = 0; i < bucket_->size(); ++i) {
      const Object* Obj = (*bucket_)[i];
      //dist_t distQC = query->DistanceObjLeft(Obj);
      //query->CheckAndAddToResult(distQC, Obj);
      auto res = distWrapper.ComputeWithQuery(query, Obj);//dist_t distQC = query->DistanceObjLeft(Obj);
      query->CheckAndAddToResult(res.first, Obj);
      queryRadius = min(queryRadius, resultModifier.ComputeModification(res.second));
    }
    return;
  }

/*
  // Distance can be asymmetric, the pivot is always the left argument (see the function that creates the node)!
  dist_t distQC = query->DistanceObjLeft(pivot_);
  query->CheckAndAddToResult(distQC, pivot_);
*/
  auto res = distWrapper.ComputeWithQuery(query, pivot_);
  dist_t distQC = resultModifier.ComputeModification(res.second);
  query->CheckAndAddToResult(res.first, pivot_);

  if (distQC < mediandist_) {      // the query is inside
    // then first check inside
    if (left_child_ != NULL && 
        oracle_.Classify(distQC, static_cast<dist_t>(queryRadius), mediandist_) != kVisitRight)
       left_child_->GenericSearch(query, queryRadius, resultModifier, distWrapper, MaxLeavesToVisit);

    /* 
     * After potentially visiting the left child, we need to reclassify the node,
     * because the query radius might have decreased.
     */


    // after that outside
    if (right_child_ != NULL && 
        oracle_.Classify(distQC, static_cast<dist_t>(queryRadius), mediandist_) != kVisitLeft)
       right_child_->GenericSearch(query, queryRadius, resultModifier, distWrapper, MaxLeavesToVisit);
  } else {                         // the query is outside
    // then first check outside
    if (right_child_ != NULL && 
        oracle_.Classify(distQC, static_cast<dist_t>(queryRadius), mediandist_) != kVisitLeft)
       right_child_->GenericSearch(query, queryRadius, resultModifier, distWrapper, MaxLeavesToVisit);

    /* 
     * After potentially visiting the left child, we need to reclassify the node,
     * because the query radius might have decreased.
     */

    // after that inside
    if (left_child_ != NULL && 
        oracle_.Classify(distQC, static_cast<dist_t>(queryRadius), mediandist_) != kVisitRight)
      left_child_->GenericSearch(query, queryRadius, resultModifier, distWrapper, MaxLeavesToVisit);
  }
}

template class VPTreeTrigen<float, PolynomialPruner<float> >;
template class VPTreeTrigen<double, PolynomialPruner<double> >;
template class VPTreeTrigen<int, PolynomialPruner<int> >;

template class DistWrapper<float >;
template class DistWrapper<double >;
template class DistWrapper<int >;

}   // namespace similarity

