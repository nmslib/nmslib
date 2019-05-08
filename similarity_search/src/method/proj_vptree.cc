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
#include "projection.h"
#include "spacefactory.h"
#include "report_intr_dim.h"
#include "portable_simd.h"

namespace similarity {

using std::unique_ptr;

template <typename dist_t>
Object* ProjectionVPTree<dist_t>::ProjectOneVect(size_t targSpaceId,
                                         const Query<dist_t>* pQuery,
                                         const Object* pSrcObj) const {
  vector<float> targVect(projDim_);

  projObj_->compProj(pQuery, pSrcObj, &targVect[0]);

  return VPTreeSpace_->CreateObjFromVect(targSpaceId, -1, targVect);
}

template <typename dist_t>
void ProjectionVPTree<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  CHECK_MSG(VPTreeIndex_ != NULL, "Expecting non-null pointer for the VP-tree index in SetQueryTimeParams");
  AnyParamManager pmgr(QueryTimeParams);

  AnyParams vptreeQueryParams = pmgr.ExtractParameters(VPTreeIndex_->getQueryTimeParams());
  VPTreeIndex_->SetQueryTimeParams(vptreeQueryParams);

  if (pmgr.hasParam("dbScanFrac") && pmgr.hasParam("knnAmp")) {
    throw runtime_error("One shouldn't specify both parameters dbScanFrac and knnAmp");
  }
  
  pmgr.GetParamOptional("dbScanFrac",   db_scan_frac_, 0);
  pmgr.GetParamOptional("knnAmp",       knn_amp_,      0);

  LOG(LIB_INFO) << "Set query-time parameters for ProjectionVPTree:";
  LOG(LIB_INFO) << "dbScanFrac=" << db_scan_frac_;
  LOG(LIB_INFO) << "knnAmp="     << knn_amp_;
}

template <typename dist_t>
ProjectionVPTree<dist_t>::ProjectionVPTree(
    bool PrintProgress,
    Space<dist_t>& space,
    const ObjectVector& data) :
      Index<dist_t>(data),
      space_(space),
      PrintProgress_(PrintProgress),
      K_(0),
      knn_amp_(0),
      db_scan_frac_(0)
{
}


template <typename dist_t>
void ProjectionVPTree<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);
  string          projSpaceType = "l2";

  size_t        intermDim = 0;

  size_t        binThreshold = 0;
  string        projType;

  pmgr.GetParamOptional("intermDim", intermDim, 0);
  pmgr.GetParamRequired("projDim", projDim_);
  pmgr.GetParamRequired("projType", projType);
  pmgr.GetParamOptional("binThreshold", binThreshold, 0);
  pmgr.GetParamOptional("projSpaceType", projSpaceType, "l2");

  AnyParams RemainParams;

  RemainParams = pmgr.ExtractParametersExcept(
                        {
                         "intermDim",
                         "projDim",
                         "projType",
                         "binThreshold",
                         "projSpaceType"
                        });

  LOG(LIB_INFO) << "projType     = " << projType;
  LOG(LIB_INFO) << "projSpaceType= " << projSpaceType;
  LOG(LIB_INFO) << "projDim      = " << projDim_;
  LOG(LIB_INFO) << "intermDim    = " << intermDim;
  LOG(LIB_INFO) << "binThreshold = " << binThreshold;


  /*
   * Let's extract all parameters before doing
   * any heavy lifting. If an exception fires for some reason,
   * e.g., because the user specified a wrong projection
   * type, the destructor of the AnyParams will check
   * for unclaimed parameters. Currently, this destructor
   * terminates the application and prints and apparently unrelated
   * error message (e.g. a wrong value for the parameter projType)
   */

  projObj_.reset(Projection<dist_t>::createProjection(
                    space_,
                    this->data_,
                    projType,
                    intermDim,
                    projDim_,
                    binThreshold));

  const string   projDescStr = projSpaceType;
  vector<string> projSpaceDesc;

  ParseSpaceArg(projDescStr, projSpaceType, projSpaceDesc);
  unique_ptr<AnyParams> projSpaceParams =
            unique_ptr<AnyParams>(new AnyParams(projSpaceDesc));

  unique_ptr<Space<dist_t>> tmpSpace(SpaceFactoryRegistry<dist_t>::
                     Instance().CreateSpace(projSpaceType, *projSpaceParams));

  if (NULL == tmpSpace.get()) {
    stringstream err;
    err << "Cannot create the projection space: '" << projSpaceType
                   << "' (desc: '" << projDescStr << "')";
    throw runtime_error(err.str());
  }

  VectorSpaceSimpleStorage<float>*  ps =
      dynamic_cast<VectorSpaceSimpleStorage<float>*>(tmpSpace.get());

  if (NULL == ps) {
    stringstream err;
    err << "The target projection space: '" << projDescStr << "' "
                   << " should be a simple-storage dense vector space, e.g., l2";
    throw runtime_error(err.str());
  }
  VPTreeSpace_.reset(ps);
  tmpSpace.release();



  projData_.resize(this->data_.size());

  for (size_t id = 0; id < this->data_.size(); ++id) {
    projData_[id] = ProjectOneVect(id, NULL, this->data_[id]);
  }

  vector<double> dist;
  ReportIntrinsicDimensionality("Set of projections" , *VPTreeSpace_, projData_, dist);

  VPTreeIndex_.reset(new VPTree<float, PolynomialPruner<float>>(
                                          PrintProgress_,
                                          *VPTreeSpace_,
                                          projData_, true /* use random centers */));
  VPTreeIndex_->CreateIndex(RemainParams);

  // Reset parameters only after the VP-tree index is created!
  this->ResetQueryTimeParams();
}

template <typename dist_t>
ProjectionVPTree<dist_t>::~ProjectionVPTree() {
  for (size_t i = 0; i < this->data_.size(); ++i) {
    delete projData_[i];
  }
}

template <typename dist_t>
const std::string ProjectionVPTree<dist_t>::StrDesc() const {
  std::stringstream str;
  str <<  "projection (vptree)";
  return str.str();
}

template <typename dist_t>
void ProjectionVPTree<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  if (db_scan_frac_ < 0.0 || db_scan_frac_ > 1.0) {
    stringstream err;
    err << METH_PROJ_VPTREE << " requires that dbScanFrac is in the range [0,1]";
    throw runtime_error(err.str());
  }
  size_t db_scan_qty = computeDbScan(0);
  if (!db_scan_qty) {
    throw runtime_error("For the range search you need to specify a sufficiently large dbScanFrac!");
  }
  unique_ptr<Object>            QueryObject(ProjectOneVect(0, query, query->QueryObject()));
  unique_ptr<KNNQuery<float>>   VPTreeQuery(new KNNQuery<float>(*VPTreeSpace_,
                                                                QueryObject.get(),
                                                                db_scan_qty, 0.0));

  VPTreeIndex_->Search(VPTreeQuery.get(), -1);

  unique_ptr<KNNQueue<float>> ResQueue(VPTreeQuery->Result()->Clone());

  while (!ResQueue->Empty()) {
      size_t id = reinterpret_cast<const Object*>(ResQueue->TopObject())->id();
      query->CheckAndAddToResult(this->data_[id]);
      ResQueue->Pop();
  }
}

template <typename dist_t>
void ProjectionVPTree<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  size_t db_scan_qty = computeDbScan(query->GetK());
  if (!db_scan_qty) {
    throw runtime_error("You need to specify knnAmp > 0 or a sufficiently large dbScanFrac!");
  }

  unique_ptr<Object>            QueryObject(ProjectOneVect(0, query, query->QueryObject()));
  unique_ptr<KNNQuery<float>>   VPTreeQuery(new KNNQuery<float>(*VPTreeSpace_,
                                                                QueryObject.get(),
                                                                db_scan_qty, 0.0));

  VPTreeIndex_->Search(VPTreeQuery.get(), -1);

  unique_ptr<KNNQueue<float>> ResQueue(VPTreeQuery->Result()->Clone());

  while (!ResQueue->Empty()) {
      size_t id = reinterpret_cast<const Object*>(ResQueue->TopObject())->id();
      query->CheckAndAddToResult(this->data_[id]);
      ResQueue->Pop();
  }
}

template class ProjectionVPTree<float>;
template class ProjectionVPTree<double>;
template class ProjectionVPTree<int>;

}  // namespace similarity

