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
#ifndef _PROJ_VPTREE_H_
#define _PROJ_VPTREE_H_

#include <vector>

#include "index.h"
#include "space/space_lp.h"
#include "space/space_vector.h"
#include "method/vptree.h"
#include "params.h"
#include "projection.h"
#include "searchoracle.h"

#define METH_PROJ_VPTREE     "proj_vptree"

namespace similarity {

/*
 * The VP-tree build over projections.
 */
template <typename dist_t>
class ProjectionVPTree : public Index<dist_t> {
 public:
  ProjectionVPTree(bool PrintProgress, 
                   Space<dist_t>& space,
                   const ObjectVector& data);
  void CreateIndex(const AnyParams& IndexParams) override;

  ~ProjectionVPTree();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType ) const override;
  void Search(KNNQuery<dist_t>* query, IdType ) const override;

  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;
 private:


  Space<dist_t>&                    space_;
  bool                              PrintProgress_;

  size_t                            K_;
  size_t                            knn_amp_;
  float					                    db_scan_frac_;

  size_t computeDbScan(size_t K) const {
    if (knn_amp_) { return min(K * knn_amp_, this->data_.size()); }
    return static_cast<size_t>(db_scan_frac_ * this->data_.size());
  }

  unique_ptr<Projection<dist_t> >   projObj_;
  ObjectVector                      projData_;
  size_t                            projDim_;

  Object*                           ProjectOneVect(size_t targSpaceId,
                                                   const Query<dist_t>* pQuery,
                                                   const Object* pSrcObj) const;

  unique_ptr<VPTree<float, PolynomialPruner<float>>>  VPTreeIndex_;
  unique_ptr<VectorSpaceSimpleStorage<float>>         VPTreeSpace_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(ProjectionVPTree);
};

}  // namespace similarity

#endif     // _PROJ_VPTREE_H_

