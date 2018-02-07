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
#ifndef _PROJECTION_INDEX_INCREMENTAL_H_
#define _PROJECTION_INDEX_INCREMENTAL_H_

#include <vector>
#include <memory>

#include "index.h"
#include "projection.h"
#include "space/space_vector.h"

// If set, projected vectors are stored contiguously in memory
#define PROJ_CONTIGUOUS_STORAGE

#define METH_PROJECTION_INC_SORT   "proj_incsort"

namespace similarity {

/* 
 * The following filter-and-refine method is inspired by the paper of Chavez et al (see below). 
 * The main difference is that this method supports several transformations of the source objects into vectors.
 * In other words, we select dbScanFract vectors whose projection vector is close to the projection 
 * of the query. Sorting is done via the incremental quicksort. There is an additional parameter:
 * the maximum allowed distance in the projected space between the query and the data point projection
 * (which is not in the referenced paper).
 *
 * Edgar Chávez et al., Effective Proximity Retrieval by Ordering Permutations.
 *                      IEEE Trans. Pattern Anal. Mach. Intell. (2008)
 */

template <typename dist_t>
class ProjectionIndexIncremental : public Index<dist_t> {
 public:
  ProjectionIndexIncremental(bool PrintProgress,
                             const Space<dist_t>& space,
                             const ObjectVector& data);
  void CreateIndex(const AnyParams& IndexParams) override;
  ~ProjectionIndexIncremental();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;
  
  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;
 private:

  const Space<dist_t>&    space_;
  bool                    PrintProgress_;

  float                                                 max_proj_dist_;
  bool                                                  use_priority_queue_;
  size_t                                                K_;
  size_t                                                knn_amp_;
  float					                                        db_scan_frac_;
  size_t                                                proj_dim_;
  bool                                                  use_cosine_;
  string                                                proj_descr_;
  unique_ptr<Projection<dist_t> >                       proj_obj_;

#ifdef PROJ_CONTIGUOUS_STORAGE
  std::vector<float>            proj_vects_;
#else
  std::vector<vector<float>>    proj_vects_;
#endif
  
  size_t computeDbScan(size_t K) const {
    if (knn_amp_) { return min(K * knn_amp_, this->data_.size()); }
    return static_cast<size_t>(db_scan_frac_ * this->data_.size());
  }

  template <typename QueryType> void GenSearch(QueryType* query, size_t K) const;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(ProjectionIndexIncremental);
};

}  // namespace similarity

#endif

