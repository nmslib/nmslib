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
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

#include "index.h"
#include "params.h"
#include "ported_boost_progress.h"

#include "trigen/cSPModifier.h"
#include "trigen/cTriGen.h"

#define METH_VPTREE_TRIGEN     "vptree_trigen"

namespace similarity {

using std::string;
using std::vector;
using std::unique_ptr;
using std::pair;

// Vantage point tree

#define USE_UNSCALED_PROXY_DIST (1)

template <class dist_t>
struct DistWrapper : public cSpaceProxy {
  virtual double Compute(const Object* o1, const Object *o2) const override {
      dist_t distOrig = space_.IndexTimeDistance(o1,o2); 
      dist_t d = distOrig;
      if (!isSymmetrDist_) d = min(distOrig, space_.IndexTimeDistance(o2,o1));
      d = max((dist_t)0, d);
#if USE_UNSCALED_PROXY_DIST
      return d;
#else
      return min<dist_t>(1, d*maxInvCoeff_);
#endif
      //return d/(1+d); // To make it bounded
  }
  pair<dist_t,dist_t> ComputeWithQuery(const Query<dist_t>* q, const Object *o) const {
      dist_t distOrig = q->DistanceObjLeft(o);
      dist_t d = distOrig;
      if (!isSymmetrDist_) d = min(distOrig, q->DistanceObjRight(o));
      d = max((dist_t)0, d);
#if USE_UNSCALED_PROXY_DIST
      return make_pair(distOrig, d);
#else
      return make_pair(distOrig, min<dist_t>(1, d*maxInvCoeff_));
#endif
      //return d/(1+d); // To make it bounded
  }
  DistWrapper(const Space<dist_t>& space, const ObjectVector& data, bool isSymmetrDist, size_t sampleQty = 100000) : space_(space), isSymmetrDist_(isSymmetrDist) {
    dist_t maxDist = numeric_limits<dist_t>::lowest();
    for (size_t i = 0; i < sampleQty; ++i) {
      dist_t d = space.IndexTimeDistance(data[RandomInt() % data.size()], data[RandomInt() % data.size()]);
      maxDist = max(d, maxDist);
    }
    CHECK(maxDist > numeric_limits<dist_t>::min());
    maxInvCoeff_ = double(1)/maxDist;
    LOG(LIB_INFO) << "maxInvCoeff_=" << maxInvCoeff_;
  }
private:
  const Space<dist_t>& space_;
  bool  isSymmetrDist_ = true;
  double maxInvCoeff_ = 0;
};

template <typename dist_t> class Space;

template <typename dist_t, typename SearchOracle>
class VPTreeTrigen : public Index<dist_t> {
 public:
  VPTreeTrigen(bool PrintProgress,
         Space<dist_t>& space,
         const ObjectVector& data,
         bool use_random_center = true);

  void CreateIndex(const AnyParams& IndexParams) override;

  ~VPTreeTrigen();

  const std::string StrDesc() const override;

  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  const vector<string>& getQueryTimeParams() const { return QueryTimeParams_; }


  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override {
    AnyParamManager pmgr(QueryTimeParams);
    // Trigen must use the standard metric oracle, 
    // so we don't pass any parameters to the oracle (it will use default, i.e., metric ones).
    //oracle_.SetQueryTimeParams(pmgr); 
    pmgr.GetParamOptional("maxLeavesToVisit", MaxLeavesToVisit_, FAKE_MAX_LEAVES_TO_VISIT);
    LOG(LIB_INFO) << "Set VP-tree query-time parameters:";
    LOG(LIB_INFO) << "maxLeavesToVisit=" << MaxLeavesToVisit_;
    pmgr.CheckUnused();
  }

  virtual bool DuplicateData() const override { return ChunkBucket_; }
 private:
  void BuildTrigen();

  class VPNode {
   public:
    // We want trees to be balanced
    const size_t BalanceConst = 4; 

    VPNode(unsigned level,
           ProgressDisplay* progress_bar,
           const SearchOracle&  oracle,
           const Space<dist_t>& space, const ObjectVector& data,
           cSPModifier& resultModifier, DistWrapper<dist_t>& distWrapper,
           size_t max_pivot_select_attempts,
           size_t BucketSize, bool ChunkBucket,
           bool use_random_center);
    ~VPNode();

    template <typename QueryType>
    void GenericSearch(QueryType* query, 
                       double&    queryRadius,
                       cSPModifier& resultModifier, 
                       DistWrapper<dist_t>& distWrapper,
                       int& MaxLeavesToVisit) const;

   private:
    void CreateBucket(bool ChunkBucket, const ObjectVector& data, 
                      ProgressDisplay* progress_bar);
    const SearchOracle& oracle_; // The search oracle must be accessed by reference,
                                 // so that VP-tree may be able to change its parameters
    const Object* pivot_;
    /* 
     * Even if dist_t is double, or long double
     * storing the median as the single-precision number (i.e., float)
     * should be good enough.
     */
    float         mediandist_;
    VPNode*       left_child_;
    VPNode*       right_child_;
    ObjectVector* bucket_;
    char*         CacheOptimizedBucket_;

    friend class VPTreeTrigen;
  };

  Space<dist_t>&      space_;
  const ObjectVector& data_;
  bool                PrintProgress_;
  bool                use_random_center_;
  size_t              max_pivot_select_attempts_;

  SearchOracle        oracle_;
  unique_ptr<VPNode>  root_;
  size_t              BucketSize_;
  int                 MaxLeavesToVisit_;
  bool                ChunkBucket_;

  float               TrigenAcc_;
  unsigned            TrigenSampleQty_;
  unsigned            TrigenSampleTripletQty_;

  bool                isSymmetrDist_;

	vector<cSPModifier*>  AllModifiers_;
  unique_ptr<cTriGen>   trigen_;
  cSPModifier*          resultModifier_ = nullptr;

  unique_ptr<DistWrapper<dist_t>> distWrapper_;

  vector<string>  QueryTimeParams_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(VPTreeTrigen);
};


}   // namespace similarity

