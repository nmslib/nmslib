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

#ifndef _PIVOT_NEIGHBORHOOD_INVINDEX_QA1_H
#define _PIVOT_NEIGHBORHOOD_INVINDEX_QA1_H

#include <vector>
#include <mutex>

#include "space/qa/space_qa1.h"
#include "index.h"
#include "permutation_utils.h"
#include "ported_boost_progress.h"

#define METH_PIVOT_NEIGHB_INVINDEX_QA1  "napp_qa1"

#include <method/pivot_neighb_common.h>

namespace similarity {

using std::vector;
using std::mutex;

/*
 * Neighborhood-APProximation Index (NAPP). An experimental modification that 
 * uses inverted indexes to quickly compute all the distances to the pivots. This
 * modification works only for SpaceQA1
 *
 * The main idea of the method (indexing K most closest pivots using an inverted file)
 * was taken from the paper:
 *
 * Eric Sadit Tellez, Edgar Chavez and Gonzalo Navarro,
 * Succinct Nearest Neighbor Search, SISAP 2011
 *
 * In this implementation, we introduce several modifications:
 * 1) The inverted file is split into small parts. In doing so, we aim to
 *    achieve better caching properties of the counter array used in ScanCount.
 * 2) The index is not compressed (though it could be)
 * 3) Instead of the adaptive union algorithm, we use a well-known ScanCount algorithm (by default). 
 *    The overall time spent on processing of the inverted file is 20-30% of the overall
 *    search time. Thus, the retrieval time cannot be substantially improved by
 *    replacing ScanCount with even better approach (should one exist).
 * 4) We also implemented several other simple algorithms for posting processing, to compare
 *    against ScanCount. For instance, the merge-sort union algorithms is about 2-3 times as slow.
 *
 *  For an example of using ScanCount see, e.g.:
 *
 *        Li, Chen, Jiaheng Lu, and Yiming Lu. 
 *       "Efficient merging and filtering algorithms for approximate string searches." 
 *        In Data Engineering, 2008. ICDE 2008. 
 *        IEEE 24th International Conference on, pp. 257-266. IEEE, 2008.
 */

template <typename dist_t>
class PivotNeighbInvertedIndexQA1 : public Index<dist_t> {
 public:
  PivotNeighbInvertedIndexQA1(bool PrintProgress,
                           const Space<dist_t>& space,
                           const ObjectVector& data);

  virtual void CreateIndex(const AnyParams& IndexParams) override;
  virtual void SaveIndex(const string &location) override;
  virtual void LoadIndex(const string &location) override;

  ~PivotNeighbInvertedIndexQA1();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;
  
  void IndexChunk(size_t chunkId, ProgressDisplay*, mutex&);
  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;
 private:

  const   ObjectVector&   data_;
  const   SpaceQA1*       pSpace_;
  bool    PrintProgress_;

  size_t  chunk_index_size_;
  size_t  K_;
  size_t  knn_amp_;
  float   db_scan_frac_;
  size_t  num_prefix_;       // K in the original paper
  size_t  min_times_;        // t in the original paper
  bool    use_sort_;
  bool    skip_checking_;
  size_t  index_thread_qty_;
  size_t  num_pivot_;
  string  pivot_file_;

  mutable std::mutex   stat_lock_;

  mutable size_t  search_time_ = 0;
  mutable size_t  dist_comp_time_ = 0;
  mutable size_t  dist_pivot_comp_time_ = 0;
  mutable size_t  proc_query_qty_ = 0;

  unique_ptr<PivotInvIndexHolder> pivotIndx_;

  void GetPermutationPPIndexEfficiently(const Object* object, Permutation& p) const;
  /*
   *  It is essential to also call this function after loading a previously save index!
   */
  void CreatePivotIndices() {
    pivotIndx_.reset(
        new PivotInvIndexHolder(pSpace_->computeCosinePivotIndex(pivot_), pSpace_->computeBM25PivotIndex(pivot_), pSpace_->computeModel1PivotIndex(pivot_), pivot_.size())
    );
  }

  enum eAlgProctype {
    kScan,
    kMap,
    kMerge
  } inv_proc_alg_;

  string toString(eAlgProctype type) {
    if (type == kScan)   return PERM_PROC_FAST_SCAN;
    if (type == kMap)    return PERM_PROC_MAP;
    if (type == kMerge)  return PERM_PROC_MERGE;
    return "unknown";
  }

  ObjectVector    pivot_;
  vector<IdType>  pivot_pos_;

  ObjectVector    genPivot_; // generated pivots

  size_t computeDbScan(size_t K, size_t chunkQty) const {
    size_t totalDbScan = static_cast<size_t>(db_scan_frac_ * data_.size());
    if (knn_amp_) { 
      totalDbScan = K * knn_amp_;
    }
    totalDbScan = min(totalDbScan, data_.size());
    CHECK_MSG(chunkQty, "Bug or inconsistent parameters: the number of index chunks cannot be zero!");
    return (totalDbScan + chunkQty - 1) / chunkQty;
  }
  
  vector<shared_ptr<vector<PostingListInt>>> posting_lists_;

  template <typename QueryType> void GenSearch(QueryType* query, size_t K) const;

  void PrintStat() const;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PivotNeighbInvertedIndexQA1);
};

}  // namespace similarity

#endif     // _PERMUTATION_SUCCINCT_INDEX_H_

