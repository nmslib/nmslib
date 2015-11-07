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
#include "method/vptree.h"
#include "method/vptree_utils.h"
#include "methodfactory.h"

namespace similarity {

using std::string;
using std::stringstream;
using std::endl;
using std::cout;
using std::cerr;
    
template <typename dist_t, typename SearchOracle>
VPTree<dist_t, SearchOracle>::VPTree(
                       bool  PrintProgress,
                       Space<dist_t>& space,
                       const ObjectVector& data,
                       bool use_random_center) : 
                              space_(space),
                              data_(data),
                              PrintProgress_(PrintProgress),
                              use_random_center_(use_random_center),
                              oracle_(space, data, PrintProgress) { }

template <typename dist_t, typename SearchOracle>
void VPTree<dist_t, SearchOracle>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("bucketSize", BucketSize_, 50);
  pmgr.GetParamOptional("chunkBucket", ChunkBucket_, true);

  LOG(LIB_INFO) << "bucketSize  = " << BucketSize_;
  LOG(LIB_INFO) << "chunkBucket = " << ChunkBucket_;

  // Call this function *ONLY AFTER* the bucket size is obtained!
  oracle_.SetIndexTimeParams(pmgr);
  oracle_.LogParams();

  pmgr.CheckUnused();

  this->ResetQueryTimeParams(); // reset query-time parameters

  unique_ptr<ProgressDisplay>   progress_bar(PrintProgress_ ? 
                                              new ProgressDisplay(data_.size(), cerr):
                                              NULL);

  root_.reset(new VPNode(0,
                     progress_bar.get(), 
                     oracle_, 
                     space_, data_,
                     BucketSize_, ChunkBucket_,
                     use_random_center_ /* use random center */));

  if (progress_bar) { // make it 100%
    (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
  }
}

template <typename dist_t,typename SearchOracle>
VPTree<dist_t, SearchOracle>::~VPTree() {
}

template <typename dist_t,typename SearchOracle>
const std::string VPTree<dist_t, SearchOracle>::ToString() const {
  return "vptree: " + SearchOracle::GetName();
}

template <typename dist_t, typename SearchOracle>
void VPTree<dist_t, SearchOracle>::Search(RangeQuery<dist_t>* query, IdType) const {
  int mx = MaxLeavesToVisit_;
  root_->GenericSearch(query, mx);
}

template <typename dist_t, typename SearchOracle>
void VPTree<dist_t, SearchOracle>::Search(KNNQuery<dist_t>* query, IdType) const {
  int mx = MaxLeavesToVisit_;
  root_->GenericSearch(query, mx);
}

template <typename dist_t, typename SearchOracle>
void VPTree<dist_t, SearchOracle>::VPNode::CreateBucket(bool ChunkBucket, 
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
VPTree<dist_t, SearchOracle>::VPNode::VPNode(
                               unsigned level,
                               ProgressDisplay* progress_bar,
                               const SearchOracle& oracle,
                               const Space<dist_t>& space, const ObjectVector& data,
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

  const size_t index = SelectVantagePoint(data, use_random_center);
  pivot_ = data[index];

  if (data.size() >= 2) {
    DistObjectPairVector<dist_t> dp;
    for (size_t i = 0; i < data.size(); ++i) {
      if (i == index) {
        continue;
      }
      // Distance can be asymmetric, the pivot is always on the left side!
      dp.push_back(std::make_pair(space.IndexTimeDistance(pivot_, data[i]), data[i]));
    }

    std::sort(dp.begin(), dp.end(), DistObjectPairAscComparator<dist_t>());
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
      left_child_ = new VPNode(level + 1, progress_bar, oracle_, space, left, BucketSize, ChunkBucket, use_random_center);
    }

    if (!right.empty()) {
      right_child_ = new VPNode(level + 1, progress_bar, oracle_, space, right, BucketSize, ChunkBucket, use_random_center);
    }
  }
}

template <typename dist_t, typename SearchOracle>
VPTree<dist_t, SearchOracle>::VPNode::~VPNode() {
  delete left_child_;
  delete right_child_;
  ClearBucket(CacheOptimizedBucket_, bucket_);
}

template <typename dist_t, typename SearchOracle>
template <typename QueryType>
void VPTree<dist_t, SearchOracle>::VPNode::GenericSearch(QueryType* query,
                                                         int& MaxLeavesToVisit) const {
  if (MaxLeavesToVisit <= 0) return; // early termination
  if (bucket_) {
    --MaxLeavesToVisit;

    for (unsigned i = 0; i < bucket_->size(); ++i) {
      const Object* Obj = (*bucket_)[i];
      dist_t distQC = query->DistanceObjLeft(Obj);
      query->CheckAndAddToResult(distQC, Obj);
    }
    return;
  }

  // Distance can be asymmetric, the pivot is always the left argument (see the function that creates the node)!
  dist_t distQC = query->DistanceObjLeft(pivot_);
  query->CheckAndAddToResult(distQC, pivot_);

  if (distQC < mediandist_) {      // the query is inside
    // then first check inside
    if (left_child_ != NULL && oracle_.Classify(distQC, query->Radius(), mediandist_) != kVisitRight)
       left_child_->GenericSearch(query, MaxLeavesToVisit);

    /* 
     * After potentially visiting the left child, we need to reclassify the node,
     * because the query radius might have decreased.
     */


    // after that outside
    if (right_child_ != NULL && oracle_.Classify(distQC, query->Radius(), mediandist_) != kVisitLeft)
       right_child_->GenericSearch(query, MaxLeavesToVisit);
  } else {                         // the query is outside
    // then first check outside
    if (right_child_ != NULL && oracle_.Classify(distQC, query->Radius(), mediandist_) != kVisitLeft)
       right_child_->GenericSearch(query, MaxLeavesToVisit);

    /* 
     * After potentially visiting the left child, we need to reclassify the node,
     * because the query radius might have decreased.
     */

    // after that inside
    if (left_child_ != NULL && oracle_.Classify(distQC, query->Radius(), mediandist_) != kVisitRight)
      left_child_->GenericSearch(query, MaxLeavesToVisit);
  }
}

template class VPTree<float, PolynomialPruner<float> >;
template class VPTree<double, PolynomialPruner<double> >;
template class VPTree<int, PolynomialPruner<int> >;

}   // namespace similarity

