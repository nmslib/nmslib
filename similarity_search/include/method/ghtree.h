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
#ifndef _METRIC_GHTREE_H_
#define _METRIC_GHTREE_H_

#include "index.h"
#include "params.h"

#define METH_GHTREE                 "ghtree"

namespace similarity {

/* 
 * J. K. Uhlmann. Satisfying general proximity/similarity queries with metric trees, 1991
 */

template <typename dist_t>
class Space;

template <typename dist_t>
class GHTree : public Index<dist_t> {
 public:
  GHTree(const Space<dist_t>& space,
         const ObjectVector& data,
         bool use_random_center = true);

  void CreateIndex(const AnyParams& IndexParams) override;

  ~GHTree();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType ) const override;
  void Search(KNNQuery<dist_t>* query, IdType ) const override;

  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override {
    AnyParamManager pmgr(QueryTimeParams);
    pmgr.GetParamOptional("maxLeavesToVisit", MaxLeavesToVisit_, FAKE_MAX_LEAVES_TO_VISIT);
    LOG(LIB_INFO) << "Set GH-tree query-time parameters:";
    LOG(LIB_INFO) << "maxLeavesToVisit=" << MaxLeavesToVisit_;
    pmgr.CheckUnused();
  }

 private:
  class GHNode {
   public:
    GHNode(const Space<dist_t>& space, const ObjectVector& data,
           size_t bucket_size, bool chunk_bucket,
           const bool use_random_center);
    ~GHNode();

    template <typename QueryType>
    void GenericSearch(QueryType* query, int& MaxLeavesToVisit);

   private:

    const Object* pivot1_;
    const Object* pivot2_;
    GHNode* left_child_;
    GHNode* right_child_;
    ObjectVector* bucket_;
    char* CacheOptimizedBucket_;
    friend class GHTree;
  };

  const Space<dist_t>& space_;
  bool                 use_random_center_;
  unique_ptr<GHNode>   root_;

  size_t                    BucketSize_;
  int                       MaxLeavesToVisit_;
  bool                      ChunkBucket_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(GHTree);
};

}   // namespace similarity

#endif     // _METRIC_GHTREE_H_

