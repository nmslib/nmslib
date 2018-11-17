/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2010--2018
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef PIVOT_NEIGHBORHOOD_HNSW_H
#define PIVOT_NEIGHBORHOOD_HNSW_H

#include <vector>
#include <mutex>
#include <memory>

#include "index.h"
#include "permutation_utils.h"
#include "ported_boost_progress.h"
#include "vector_pool.h"

#define METH_PIVOT_NEIGHB_INVINDEX_HNSW      "napp_hnsw"

#include <method/pivot_neighb_common.h>

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
class PivotNeighbInvertedIndexHNSW : public Index<dist_t> {
 public:
  PivotNeighbInvertedIndexHNSW(bool PrintProgress,
                              const Space<dist_t>& space,
                              const ObjectVector& data);

  virtual void CreateIndex(const AnyParams& IndexParams) override;
  virtual void SaveIndex(const string &location) override;
  virtual void LoadIndex(const string &location) override;

  ~PivotNeighbInvertedIndexHNSW();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;
 private:

  const   Space<dist_t>&  space_;
  bool    PrintProgress_;

  size_t  exp_avg_post_size_;
  size_t  K_;
  size_t  num_prefix_;       // K in the original paper, also numPivotIndex
  size_t  num_prefix_search_;// K used during search (our modification can use a different K)
  size_t  min_times_; // numPivotSearch
  bool    skip_checking_;
  size_t  index_thread_qty_;
  size_t  num_pivot_;
  string  pivot_file_;
  int     print_pivot_stat_;
  size_t  hash_trick_dim_;

  unsigned  efConstruction_;
  unsigned  efPivotSearchIndex_;
  unsigned  efPivotSearchQuery_;
  unsigned  delaunay_type_;
  unsigned  M_;
  unsigned  post_;

  unique_ptr<Index<dist_t>>     pivot_index_;
  unique_ptr<ProgressDisplay>   progress_bar_;
  mutex                         progress_bar_mutex_;

  ObjectVector    genPivot_; // generated pivots: need to remember to delete them

  enum eAlgProctype {
    kScan,
    kStoreSort
  } inv_proc_alg_;

  vector<unique_ptr<PostingListInt>> posting_lists_;
  vector<unique_ptr<mutex>>          post_list_mutexes_;

  unique_ptr<VectorPool<IdType>>  tmp_res_pool_;
  unique_ptr<VectorPool<const Object*>>  cand_pool_;
  unique_ptr<VectorPool<unsigned>> counter_pool_;

  size_t expCandQtyUB_ = 0;

  template <typename QueryType> void GenSearch(QueryType* query, size_t K) const;
  void GetClosePivotIds(const Object* queryObj, size_t K, vector<IdType>& pivotIds) const;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PivotNeighbInvertedIndexHNSW);

  mutable size_t  post_qty_scan_ = 0;
  mutable size_t  post_qty_store_sort_ = 0;
  mutable size_t  search_time_ = 0;
  mutable size_t  dist_comp_time_ = 0;
  mutable size_t  pivot_search_time_ = 0;
  mutable size_t  sort_comp_time_ = 0;
  mutable size_t  copy_post_time_ = 0;
  mutable size_t  scan_sorted_time_ = 0;
  mutable size_t  proc_query_qty_scan_ = 0;
  mutable size_t  proc_query_qty_store_sort_ = 0;

  mutable mutex   stat_mutex_;

};

}  // namespace similarity

#endif     

