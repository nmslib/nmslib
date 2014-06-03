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
  GHTree(const Space<dist_t>* space,
         const ObjectVector& data,
         const AnyParams& MethParams,
         bool use_random_center = true);
  ~GHTree();

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

 private:
  class GHNode {
   public:
    GHNode(const Space<dist_t>* space, ObjectVector& data,
           size_t bucket_size, bool chunk_bucket,
           const bool use_random_center, bool is_root);
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

  GHNode* root_;

  size_t                    BucketSize_;
  int                       MaxLeavesToVisit_;
  bool                      ChunkBucket_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(GHTree);
};

}   // namespace similarity

#endif     // _METRIC_GHTREE_H_

