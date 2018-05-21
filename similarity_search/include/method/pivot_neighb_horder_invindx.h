/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
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

#ifndef PIVOT_NEIGHBORHOOD_HORDER_INVINDEX_H
#define PIVOT_NEIGHBORHOOD_HORDER_INVINDEX_H

#include <vector>
#include <mutex>
#include <memory>

#include "index.h"
#include "permutation_utils.h"
#include "ported_boost_progress.h"

#define METH_PIVOT_NEIGHB_HORDER_INVINDEX      "napp_horder"

#include <method/pivot_neighb_common.h>

#define UINT16_IDS

#ifdef UINT16_IDS
typedef uint16_t PostingListElemType; 
const size_t UINT16_ID_MAX=65536;
#else
typedef uint32_t PostingListElemType; 
#endif

typedef std::vector<PostingListElemType> PostingListType;

namespace similarity {

using std::vector;
using std::mutex;
using std::unique_ptr;

/*
 * A modified variant of the Neighborhood-APProximation Index (NAPP):
 * more details to follow.
 *
 */

template <typename dist_t>
class PivotNeighbHorderInvIndex : public Index<dist_t> {
 public:
  PivotNeighbHorderInvIndex(bool PrintProgress,
                           const Space<dist_t>& space,
                           const ObjectVector& data);

  virtual void CreateIndex(const AnyParams& IndexParams) override;
  virtual void SaveIndex(const string &location) override;
  virtual void LoadIndex(const string &location) override;

  ~PivotNeighbHorderInvIndex();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;
  
  void IndexChunk(size_t chunkId, ProgressDisplay*, mutex&);
  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;
 private:

  const   Space<dist_t>&  space_;
  bool    PrintProgress_;
  bool    recreate_points_;

  size_t  chunk_index_size_;
  size_t  K_;
  size_t  knn_amp_;
  float   db_scan_frac_;
  size_t  num_prefix_;       // K in the original paper
  size_t  num_prefix_search_;// K used during search (our modification can use a different K)
  size_t  min_times_;        // t in the original paper
  bool    use_sort_;
  bool    skip_checking_;
  size_t  index_thread_qty_;
  size_t  num_pivot_;
  string  pivot_file_;
  bool    disable_pivot_index_;
  size_t hash_trick_dim_;

  unique_ptr<PivotIndex<dist_t>> pivot_index_;

  enum eAlgProctype {
    kScan,
    kMap,
    kMerge,
    kPriorQueue,
    kWAND
  } inv_proc_alg_;

  string toString(eAlgProctype type) const {
    if (type == kScan)   return PERM_PROC_FAST_SCAN;
    if (type == kMap)    return PERM_PROC_MAP;
    if (type == kMerge)  return PERM_PROC_MERGE;
    if (type == kPriorQueue) return PERM_PROC_PRIOR_QUEUE;
    if (type == kWAND) return PERM_PROC_WAND;
    return "unknown";
  }

  ObjectVector    pivot_;
  vector<IdType>  pivot_pos_;

  ObjectVector    genPivot_; // generated pivots

  size_t computeDbScan(size_t K, size_t chunkQty) const {
    size_t totalDbScan = static_cast<size_t>(db_scan_frac_ * this->data_.size());
    if (knn_amp_) { 
      totalDbScan = K * knn_amp_;
    }
    totalDbScan = min(totalDbScan, this->data_.size());
    CHECK_MSG(chunkQty, "Bug or inconsistent parameters: the number of index chunks cannot be zero!");
    return (totalDbScan + chunkQty - 1) / chunkQty;
  }

  void initPivotIndex() {
    if (disable_pivot_index_) {
      pivot_index_.reset(new DummyPivotIndex<dist_t>(space_, pivot_));
      LOG(LIB_INFO) << "Created a dummy pivot index";
    } else {
      pivot_index_.reset(space_.CreatePivotIndex(pivot_, hash_trick_dim_));
      LOG(LIB_INFO) << "Attempted to create an efficient pivot index (however only few spaces support such index)";
    }
  }

  #define ADD_CHECKS

  inline IdTypeUnsign PostingListIndex(IdTypeUnsign pivotId1,
                                      IdTypeUnsign pivotId2) const {
   if (pivotId1 > pivotId2) {
     swap(pivotId1, pivotId2);
   }
#ifdef ADD_CHECKS
   CHECK(pivotId1 != pivotId2 );
   CHECK(pivotId2 < num_pivot_ );
#endif

   IdTypeUnsign res = pivotId1 + pivotId2*(pivotId2 - 1) / 2 ;

#ifdef ADD_CHECKS
   static IdTypeUnsign resUB = num_pivot_ * (num_pivot_ - 1) / 2;

   CHECK(res < resUB); 
#endif
      
   return res;
  }

  inline IdTypeUnsign PostingListIndex(IdTypeUnsign pivotId1,
                                      IdTypeUnsign pivotId2,
                                      IdTypeUnsign pivotId3) const {
  IdTypeUnsign pivots[3] = {pivotId1, pivotId2, pivotId3};
  sort(pivots, pivots + 3);

  pivotId1 = pivots[0];
  pivotId2 = pivots[1];
  pivotId3 = pivots[2];

#ifdef ADD_CHECKS
   CHECK(pivotId1 < pivotId2 && pivotId2 < pivotId3 && pivotId3 < num_pivot_);
#endif

   IdTypeUnsign res = pivotId1 + 
                      pivotId2 * (pivotId2 - 1) / 2 + 
                      pivotId3 * (pivotId3-1) * (pivotId3-2) / 6;

#ifdef ADD_CHECKS
   static IdTypeUnsign resUB = num_pivot_ * (num_pivot_ - 1) * (num_pivot_ - 2) / 6;

   CHECK(res < resUB); 
#endif
      
   return res;
  }
  
    
  
  vector<unique_ptr<vector<PostingListType>>> posting_lists_;

  size_t getPostQtysTwoPivots(size_t skipVal) const {
    return (skipVal - 1 + size_t(num_pivot_) * (num_pivot_ - 1)/ 2) / skipVal;
  }

  size_t getPostQtysThreePivots(size_t skipVal) const {
    CHECK(num_pivot_ >= 2);
    return (skipVal - 1 + size_t(num_pivot_) * size_t(num_pivot_ - 1) * size_t(num_pivot_ - 2)/ 6) / skipVal;
  }

  template <typename QueryType> void GenSearch(QueryType* query, size_t K) const;

  void GetPermutationPPIndexEfficiently(const Object* object, Permutation& p) const;
  void GetPermutationPPIndexEfficiently(const Query<dist_t>* query, Permutation& p) const;
  void GetPermutationPPIndexEfficiently(Permutation &p, const vector <dist_t> &vDst) const;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PivotNeighbHorderInvIndex);

  void GetPermutationPPIndexEfficiently(Permutation &p, const vector<bool> &vDst) const;

  mutable size_t  post_qty_ = 0;
  mutable size_t  search_time_ = 0;
  mutable size_t  dist_comp_time_ = 0;
  mutable size_t  dist_pivot_comp_time_ = 0;
  mutable size_t  sort_comp_time_ = 0;
  mutable size_t  scan_sorted_time_ = 0;
  mutable size_t  ids_gen_time_ = 0;
  mutable size_t  proc_query_qty_ = 0;

  mutable mutex   stat_mutex_;

  size_t  skip_val_ = 1;
};

}  // namespace similarity

#endif     // _PERMUTATION_SUCCINCT_INDEX_H_

