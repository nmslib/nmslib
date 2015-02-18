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
                             const Space<dist_t>* space,
                             const ObjectVector& data,
                             const AnyParams& AllParams);
  ~ProjectionIndexIncremental();

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);
  
  virtual vector<string> GetQueryTimeParamNames() const;

 private:
  virtual void SetQueryTimeParamsInternal(AnyParamManager& );

  const Space<dist_t>*                                  space_;
  const ObjectVector&                                   data_;
  float                                                 max_proj_dist_;
  float					                                        db_scan_frac_;
  size_t                                                db_scan_;
  size_t                                                proj_dim_;
  string                                                proj_descr_;
  unique_ptr<Projection<dist_t> >                       proj_obj_;
  bool                                                  use_priority_queue_;

#ifdef PROJ_CONTIGUOUS_STORAGE
  std::vector<float>            proj_vects_;
#else
  std::vector<vector<float>>    proj_vects_;
#endif
  
  void ComputeDbScan(float db_scan_fraction) {
    db_scan_ = max(size_t(1), static_cast<size_t>(db_scan_fraction * data_.size()));
  }

  template <typename QueryType> void GenSearch(QueryType* query);

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(ProjectionIndexIncremental);
};

}  // namespace similarity

#endif

