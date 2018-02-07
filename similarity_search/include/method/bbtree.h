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

/*
 * This code is based on the BBTREE implementation written by Lawrence Cayton
 * Copyright (c) 2013-2018
 *
 * The algorithms were originally published in the papers:
 * 1) L. Cayton. Fast nearest neighbor retrieval for bregman divergences. 
 *    Twenty-Fifth International Conference on Machine Learning (ICML), 2008. 
 *
 * 2) L. Cayton. Efficient bregman range search.  
 * Advances in Neural Information Processing Systems 22 (NIPS), 2009. 
 *
 * See https://github.com/lcayton/bbtree and http://lcayton.com/code.html
 *
 * Because the original code is released under the terms of the GNU General Public License,
 * we had to release this file under the GNU license as well.
*/

#ifndef _BBTREE_H_
#define _BBTREE_H_

#include "index.h"
#include "params.h"

#define METH_BBTREE                 "bbtree"

namespace similarity {

template <typename dist_t>
class Space;

template <typename dist_t>
class BregmanDiv;

template <typename dist_t>
class BBTree : public Index<dist_t> {
 public:
  BBTree(const Space<dist_t>& space,
         const ObjectVector& data);

  void CreateIndex(const AnyParams& IndexParams) override;
  void SetQueryTimeParams(const AnyParams& params) override {
    AnyParamManager pmgr(params);
    pmgr.GetParamOptional("maxLeavesToVisit", MaxLeavesToVisit_, FAKE_MAX_LEAVES_TO_VISIT);

    LOG(LIB_INFO) << "Set bbtree query-time parameters:";
    LOG(LIB_INFO) << "maxLeavesToVisit" << MaxLeavesToVisit_;
    pmgr.CheckUnused();
  }

  ~BBTree();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  virtual bool DuplicateData() const override { return ChunkBucket_; }
 private:
  class BBNode {
   public:
    BBNode(const BregmanDiv<dist_t>* div,
           const ObjectVector& data, size_t bucket_size, bool use_optim);
    ~BBNode();

    inline bool IsLeaf() const;

    template <typename QueryType>
    bool RecBinSearch(const BregmanDiv<dist_t>* div,
                      Object* query_gradient, 
                      QueryType* query, dist_t mindist_est,
                      dist_t l=0.0, dist_t r=1.0, int depth=0) const;

    template <typename QueryType>
    bool NeedToSearch(const BregmanDiv<dist_t>* div,
                      Object* query_gradient, 
                      QueryType* query, dist_t mindist_est,
                      dist_t div_query_to_center) const;

    template <typename QueryType>
    void LeftSearch(const BregmanDiv<dist_t>* div,
                    Object* query_gradient, QueryType* query,
                    int& MaxLeavesToVisit_) const;

    void SelectCenters(const ObjectVector& data, ObjectVector& centers);

    void FindSplitKMeans(const BregmanDiv<dist_t>* div, 
                         const ObjectVector& data,
                         ObjectVector& bucket_left, 
                         ObjectVector& bucket_right);

   private:
    enum { kMaxRetry = 10 };

    Object*       center_;
    Object*       center_gradf_;
    dist_t        covering_radius_;
    bool          is_leaf_;
    ObjectVector* bucket_;
    char*         CacheOptimizedBucket_;
    BBNode*       left_child_;
    BBNode*       right_child_;
    DISABLE_COPY_AND_ASSIGN(BBNode);
  };

  unique_ptr<BBNode>        root_node_;
  size_t                    BucketSize_;
  int                       MaxLeavesToVisit_;
  bool                      ChunkBucket_;
  const BregmanDiv<dist_t>* BregmanDivSpace_;
  DISABLE_COPY_AND_ASSIGN(BBTree);
};

}  // namespace similarity

#endif       // _BBTREE_H_
