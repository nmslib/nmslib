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
#ifndef _VPTREE_H_
#define _VPTREE_H_

#include <string>
#include <vector>
#include <memory>

#include "index.h"
#include "params.h"
#include "ported_boost_progress.h"

#define METH_VPTREE          "vptree"

namespace similarity {

using std::string;
using std::vector;
using std::unique_ptr;

// Vantage point tree

template <typename dist_t> class Space;

template <typename dist_t, typename SearchOracle>
class VPTree : public Index<dist_t> {
 public:
  VPTree(bool  PrintProgress,
         const Space<dist_t>* space,
         const ObjectVector& data,
         const AnyParams& MethParams,
         bool use_random_center = true);
  ~VPTree();

  const std::string ToString() const;

  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

  vector<string> GetQueryTimeParamNames() const { return oracle_.GetParams(); }

 private:
  void SetQueryTimeParamsInternal(AnyParamManager& pmgr) { oracle_.SetParams(pmgr); }

  class VPNode {
   public:
    // We want trees to be balanced
    const size_t BalanceConst = 4; 

    VPNode(unsigned level,
           ProgressDisplay* progress_bar,
           const SearchOracle&  oracle,
           const Space<dist_t>* space, const ObjectVector& data,
           size_t BucketSize, bool ChunkBucket,
           bool use_random_center, bool is_root);
    ~VPNode();

    template <typename QueryType>
    void GenericSearch(QueryType* query, int& MaxLeavesToVisit);

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

    friend class VPTreeOld;
  };

  SearchOracle    oracle_;
  VPNode*         root_;
  size_t          BucketSize_;
  int             MaxLeavesToVisit_;
  bool            ChunkBucket_;
  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(VPTree);
};


}   // namespace similarity

#endif
