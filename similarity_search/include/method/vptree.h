/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef _VPTREE_H_
#define _VPTREE_H_

#include <string>

#include "index.h"
#include "params.h"

#define METH_VPTREE          "vptree"
#define METH_VPTREE_SAMPLE   "vptree_sample"

namespace similarity {

using std::string;

// Vantage point tree

template <typename dist_t> class Space;

template <typename dist_t, typename SearchOracle, typename SearchOracleCreator>
class VPTree : public Index<dist_t> {
 public:
  VPTree(bool  PrintProgress,
         const SearchOracleCreator& OracleCreator,
         const Space<dist_t>* space,
         const ObjectVector& data,
         const AnyParams& MethParams,
         bool use_random_center = true);
  ~VPTree();

  const std::string ToString() const;

  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

 private:

  class VPNode {
   public:
    // We want trees to be balanced
    const size_t BalanceConst = 4; 

    VPNode(bool     PrintProgress,
           unsigned level,
           size_t   TotalQty,
           size_t&  IndexedQty,
           const SearchOracleCreator& OracleCreator,
           const Space<dist_t>* space, ObjectVector& data,
           size_t BucketSize, bool ChunkBucket,
           const string& SaveHistFileName,
           bool use_random_center, bool is_root);
    ~VPNode();

    template <typename QueryType>
    void GenericSearch(QueryType* query, int& MaxLeavesToVisit);

   private:
    void CreateBucket(bool ChunkBucket, ObjectVector& data, 
                      bool PrintProgress,
                      size_t&  IndexedQty, size_t   TotalQty);
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

    friend class VPTree;
  };

  VPNode* root_;
  size_t  BucketSize_;
  int     MaxLeavesToVisit_;
  bool    ChunkBucket_;
  string  SaveHistFileName_;
  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(VPTree);
};


}   // namespace similarity

#endif     // _VPTREE_H_
