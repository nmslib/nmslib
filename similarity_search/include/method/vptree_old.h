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

#ifndef WITHOUT_VPTREE_OLD

#ifndef _VPTREE_OLD_H_
#define _VPTREE_OLD_H_

#include <string>
#include <memory>

#include "index.h"
#include "params.h"
#include "ported_boost_progress.h"

#define METH_VPTREE_OLD          "vptree_old"
#define METH_VPTREE_OLD_SAMPLE   "vptree_old_sample"

namespace similarity {

using std::string;
using std::unique_ptr;

// Vantage point tree

template <typename dist_t> class Space;

template <typename dist_t, typename SearchOracle, typename SearchOracleCreator>
class VPTreeOld : public Index<dist_t> {
 public:
  VPTreeOld(bool  PrintProgress,
         const SearchOracleCreator& OracleCreator,
         const Space<dist_t>* space,
         const ObjectVector& data,
         const AnyParams& MethParams,
         bool use_random_center = true);
  ~VPTreeOld();

  const std::string ToString() const;

  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

 private:
  class VPNode {
   public:
    // We want trees to be balanced
    const size_t BalanceConst = 4; 

    VPNode(unsigned level,
           ProgressDisplay* progress_bar,
           const SearchOracleCreator& OracleCreator,
           const Space<dist_t>* space, const ObjectVector& data,
           size_t BucketSize, bool ChunkBucket,
           const string& SaveHistFileName,
           bool use_random_center, bool is_root);
    ~VPNode();

    template <typename QueryType>
    void GenericSearch(QueryType* query, int& MaxLeavesToVisit);

   private:
    void CreateBucket(bool ChunkBucket, const ObjectVector& data, 
                      ProgressDisplay* progress_bar);
    const Object* pivot_;
    /* 
     * Even if dist_t is double, or long double
     * storing the median as the single-precision number (i.e., float)
     * should be good enough.
     */
    float         mediandist_;
    VPNode*       left_child_;
    VPNode*       right_child_;
    SearchOracle* oracle_;
    ObjectVector* bucket_;
    char*         CacheOptimizedBucket_;

    friend class VPTreeOld;
  };

  VPNode* root_;
  size_t  BucketSize_;
  int     MaxLeavesToVisit_;
  bool    ChunkBucket_;
  string  SaveHistFileName_;
  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(VPTreeOld);
};


}   // namespace similarity

#endif

#endif    // WITHOUT_VPTREE_OLD
