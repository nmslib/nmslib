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

#include <algorithm>
#include <sstream>
#include <memory>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "method/proj_vptree.h"
#include "utils.h"
#include "distcomp.h"

namespace similarity {

using std::unique_ptr;

template <typename dist_t>
Object* 
ProjectionVPTree<dist_t>::ProjectOneVect(size_t id, const Object* sparseVect) const {
  vector<float> denseElem;

  for (unsigned i = 0; i < randProjPivots_.size(); ++i) {
    denseElem.push_back(space_->ScalarProduct(sparseVect, randProjPivots_[i]));
  }

  return VPTreeSpace_->CreateObjFromVect(id, -1, denseElem);
};

template <typename dist_t>
ProjectionVPTree<dist_t>::ProjectionVPTree(
    const Space<dist_t>* space,
    const ObjectVector& data,
    const AnyParams& AllParams) : 
      space_(dynamic_cast<const SpaceSparseVector<dist_t>*>(space)), 
      data_(data),   // reference
      VPTreeSpace_(new SpaceLp<float>(1))
{
  if (!space_) {
    LOG(LIB_FATAL) << METH_PROJ_VPTREE << " can work only with sparse vectors!";
  }
  AnyParamManager pmgr(AllParams);

  double    DbScanFrac = 0.05;
  pmgr.GetParamOptional("dbScanFrac", DbScanFrac);

  if (DbScanFrac < 0.0 || DbScanFrac > 1.0) {
    LOG(LIB_FATAL) << METH_PROJ_VPTREE << " requires that dbScanFrac is in the range [0,1]";
  }

  size_t        ProjPivotQty  = 128;
  size_t        ProjMaxElem = 8192;

  pmgr.GetParamOptional("projPivotQty", ProjPivotQty);
  pmgr.GetParamOptional("projMaxElem", ProjMaxElem);

  space_->GenRandProjPivots(randProjPivots_, ProjPivotQty, ProjMaxElem);

  AnyParams RemainParams;

  double AlphaLeft = 1.0, AlphaRight = 1.0;

  pmgr.GetParamOptional("alphaLeft",  AlphaLeft);
  pmgr.GetParamOptional("alphaRight", AlphaRight);

  RemainParams = pmgr.ExtractParametersExcept(
                        {"dbScanFrac",

                         "projPivotQty",
                         "projMaxElem",

                         "alphaLeft", 
                         "alphaRight"
                        });

  // db_can_qty_ should always be > 0
  db_scan_qty_ = max(size_t(1), static_cast<size_t>(DbScanFrac * data.size())),
  projData_.resize(data.size());

  for (size_t id = 0; id < data.size(); ++id) {
    projData_[id] = ProjectOneVect(id, data[id]);
  }

  ReportIntrinsicDimensionality("Set of projections" , *VPTreeSpace_, projData_);

  TriangIneqCreator<float> OracleCreator(AlphaLeft, AlphaRight);

  VPTreeIndex_ = new VPTree<float, TriangIneq<float>,
                            TriangIneqCreator<float> >(
                                          true,
                                          OracleCreator,
                                          VPTreeSpace_,
                                          projData_,
                                          RemainParams
                                    );
}

template <typename dist_t>
ProjectionVPTree<dist_t>::~ProjectionVPTree() {
  for (size_t i = 0; i < data_.size(); ++i) {
    delete projData_[i];
  }
  for (size_t i = 0; i < randProjPivots_.size(); ++i) {
    delete randProjPivots_[i];
  }
  delete VPTreeIndex_;
  delete VPTreeSpace_;
}

template <typename dist_t>
const std::string ProjectionVPTree<dist_t>::ToString() const {
  std::stringstream str;
  str <<  "projection (vptree)";
  return str.str();
}

template <typename dist_t>
void ProjectionVPTree<dist_t>::Search(RangeQuery<dist_t>* query) {
  unique_ptr<Object>            QueryObject(ProjectOneVect(0, query->QueryObject()));
  unique_ptr<KNNQuery<float>>   VPTreeQuery(new KNNQuery<float>(VPTreeSpace_, QueryObject.get(), db_scan_qty_, 0.0));

  VPTreeIndex_->Search(VPTreeQuery.get());

  unique_ptr<KNNQueue<float>> ResQueue(VPTreeQuery->Result()->Clone());

  while (!ResQueue->Empty()) {
      size_t id = reinterpret_cast<const Object*>(ResQueue->TopObject())->id();
      query->CheckAndAddToResult(data_[id]);
      ResQueue->Pop();
  }
}

template <typename dist_t>
void ProjectionVPTree<dist_t>::Search(KNNQuery<dist_t>* query) {
  unique_ptr<Object>            QueryObject(ProjectOneVect(0, query->QueryObject()));
  unique_ptr<KNNQuery<float>>   VPTreeQuery(new KNNQuery<float>(VPTreeSpace_, QueryObject.get(), db_scan_qty_, 0.0));

  VPTreeIndex_->Search(VPTreeQuery.get());

  unique_ptr<KNNQueue<float>> ResQueue(VPTreeQuery->Result()->Clone());

  while (!ResQueue->Empty()) {
      size_t id = reinterpret_cast<const Object*>(ResQueue->TopObject())->id();
      query->CheckAndAddToResult(data_[id]);
      ResQueue->Pop();
  }
}

template class ProjectionVPTree<float>;
template class ProjectionVPTree<double>;

}  // namespace similarity

