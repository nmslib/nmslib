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

template <typename dist_t, typename SearchOracle, typename SearchOracleCreator>
VPTree<dist_t, SearchOracle, SearchOracleCreator>::VPTree(
                       bool  PrintProgress,
                       const SearchOracleCreator& OracleCreator,
                       const Space<dist_t>* space,
                       const ObjectVector& data,
                       const AnyParams& MethParams,
                       bool use_random_center) : 
                              BucketSize_(50),
                              MaxLeavesToVisit_(FAKE_MAX_LEAVES_TO_VISIT),
                              ChunkBucket_(true),
                              SaveHistFileName_("")
                       {
  AnyParamManager pmgr(MethParams);

  pmgr.GetParamOptional("bucketSize", BucketSize_);
  pmgr.GetParamOptional("chunkBucket", ChunkBucket_);
  pmgr.GetParamOptional("maxLeavesToVisit", MaxLeavesToVisit_);
  pmgr.GetParamOptional("saveHistFileName", SaveHistFileName_);

  unique_ptr<ProgressDisplay>   progress_bar(PrintProgress ? 
                                              new ProgressDisplay(data.size(), cout):
                                              NULL);

  root_ = new VPNode(0,
                     progress_bar.get(), 
                     OracleCreator, space,
                     const_cast<ObjectVector&>(data),
                     BucketSize_, ChunkBucket_,
                     SaveHistFileName_,
                     use_random_center, true);

  if (progress_bar) { // make it 100%
    (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
  }
}

template <typename dist_t, typename SearchOracle, typename SearchOracleCreator>
VPTree<dist_t, SearchOracle, SearchOracleCreator>::~VPTree() {
  delete root_;
}

template <typename dist_t, typename SearchOracle, typename SearchOracleCreator>
const std::string VPTree<dist_t, SearchOracle, SearchOracleCreator>::ToString() const {
  return "vptree: " + SearchOracle::GetName();
}

template <typename dist_t, typename SearchOracle, typename SearchOracleCreator>
void VPTree<dist_t, SearchOracle, SearchOracleCreator>::Search(RangeQuery<dist_t>* query) {
  int mx = MaxLeavesToVisit_;
  root_->GenericSearch(query, mx);
}

template <typename dist_t, typename SearchOracle, typename SearchOracleCreator>
void VPTree<dist_t, SearchOracle, SearchOracleCreator>::Search(KNNQuery<dist_t>* query) {
  int mx = MaxLeavesToVisit_;
  root_->GenericSearch(query, mx);
}

template <typename dist_t, typename SearchOracle, typename SearchOracleCreator>
void VPTree<dist_t, SearchOracle, SearchOracleCreator>::VPNode::CreateBucket(bool ChunkBucket, 
                                                                             const ObjectVector& data, 
                                                                             ProgressDisplay* progress_bar
                                                                             ) {
    if (ChunkBucket) {
      CreateCacheOptimizedBucket(data, CacheOptimizedBucket_, bucket_);
    } else {
      bucket_ = new ObjectVector(data);
    }
    if (progress_bar) (*progress_bar) += data.size();
}

template <typename dist_t, typename SearchOracle, typename SearchOracleCreator>
VPTree<dist_t, SearchOracle, SearchOracleCreator>::VPNode::VPNode(
                               unsigned level,
                               ProgressDisplay* progress_bar,
                               const SearchOracleCreator& OracleCreator,
                               const Space<dist_t>* space, const ObjectVector& data,
                               size_t BucketSize, bool ChunkBucket,
                               const string& SaveHistFileName,
                               bool use_random_center, bool is_root)
    : pivot_(NULL), mediandist_(0),
      left_child_(NULL), right_child_(NULL), oracle_(NULL),
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
      dp.push_back(std::make_pair(space->IndexTimeDistance(pivot_, data[i]), data[i]));
    }

    std::sort(dp.begin(), dp.end(), DistObjectPairAscComparator<dist_t>());
    DistObjectPair<dist_t>  medianDistObj = GetMedian(dp);
    mediandist_ = medianDistObj.first; 

    oracle_ = OracleCreator.Create(level, pivot_, dp);

    if (0 == level && !SaveHistFileName.empty()) {
      stringstream str;
      str <<  oracle_->Dump() <<  mediandist_ << endl ;

      std::ofstream  of(SaveHistFileName.c_str(), std::ios::trunc | std::ios::out); 

      if (!of) {
        LOG(LIB_FATAL) << "Cannot open: " << SaveHistFileName << " for writing";
      }
      of << str.str();
    }

    ObjectVector left;
    ObjectVector right;

#if 1
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
#else
    /*
     * TODO: This code creates overlapping partitions.
     *       If we ever uncomment it, you will have to take care of duplicates.
     *       You will also need to modify the code that reports progress in index creation.
     *       See std::cout << ... in VPNode
     */
    size_t LeftQty = dp.size() / 2;
    size_t RightQty = dp.size() / 2;
    if (level < 8) {
        RightQty = (size_t)round(0.4 * dp.size());
        LeftQty = (size_t)round(0.6 * dp.size());
    }

    for (size_t i = 0; i < dp.size(); ++i) {
      const Object* v = dp[i].second;

      if (i <= LeftQty) {
        left.push_back(v);
      }
      if (i >= RightQty)  {
        right.push_back(v);
      }
    }
#endif
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
      left_child_ = new VPNode(level + 1, progress_bar, OracleCreator, space, left, BucketSize, ChunkBucket, "", use_random_center, false);
    }

    if (!right.empty()) {
      right_child_ = new VPNode(level + 1, progress_bar, OracleCreator, space, right, BucketSize, ChunkBucket, "", use_random_center, false);
    }
  }
}

template <typename dist_t, typename SearchOracle, typename SearchOracleCreator>
VPTree<dist_t, SearchOracle, SearchOracleCreator>::VPNode::~VPNode() {
  delete left_child_;
  delete right_child_;
  delete oracle_;
  ClearBucket(CacheOptimizedBucket_, bucket_);
}

template <typename dist_t, typename SearchOracle, typename SearchOracleCreator>
template <typename QueryType>
void VPTree<dist_t, SearchOracle, SearchOracleCreator>::VPNode::GenericSearch(QueryType* query,
                                                                          int& MaxLeavesToVisit) {
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

  // Distance can be asymmetric, the pivot is always on the left side (see the function that create the node)!
  dist_t distQC = query->DistanceObjLeft(pivot_);
  query->CheckAndAddToResult(distQC, pivot_);

  if (distQC < mediandist_) {      // the query is inside
    // then first check inside
    if (left_child_ != NULL && oracle_->Classify(distQC, query->Radius(), mediandist_) != kVisitRight)
       left_child_->GenericSearch(query, MaxLeavesToVisit);

    // after that outside
    if (right_child_ != NULL && oracle_->Classify(distQC, query->Radius(), mediandist_) != kVisitLeft)
       right_child_->GenericSearch(query, MaxLeavesToVisit);
  } else {                         // the query is outside
    // then first check outside
    if (right_child_ != NULL && oracle_->Classify(distQC, query->Radius(), mediandist_) != kVisitLeft)
       right_child_->GenericSearch(query, MaxLeavesToVisit);

    // after that inside
    if (left_child_ != NULL && oracle_->Classify(distQC, query->Radius(), mediandist_) != kVisitRight)
      left_child_->GenericSearch(query, MaxLeavesToVisit);
  }
}

template class VPTree<float, TriangIneq<float>, TriangIneqCreator<float> >;
template class VPTree<double, TriangIneq<double>, TriangIneqCreator<double> >;
template class VPTree<int, TriangIneq<int>, TriangIneqCreator<int> >;

template class VPTree<float, SamplingOracle<float>, SamplingOracleCreator<float> >;
template class VPTree<double, SamplingOracle<double>, SamplingOracleCreator<double> >;
template class VPTree<int, SamplingOracle<int>, SamplingOracleCreator<int> >;

}   // namespace similarity

