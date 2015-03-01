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
#include <map>
#include <string>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <queue>

#include "distcomp.h"
#include "space.h"
#include "rangequery.h"
#include "ported_boost_progress.h"
#include "knnquery.h"
#include "incremental_quick_select.h"
#include "method/projection_index_incremental.h"
#include "utils.h"

#define METH_PROJ_INDEX_INCREMENTAL   "proj_incr"

using namespace std;

typedef pair<float,int> FloatInt; 

namespace similarity {

template <typename dist_t>
ProjectionIndexIncremental<dist_t>::ProjectionIndexIncremental(
    bool  PrintProgress,
    const Space<dist_t>*  space,
    const ObjectVector&   data,
    const AnyParams&      AllParams)
	: space_(space), data_(data) /* reference */,
	  use_priority_queue_(false) ,
    K_(0),
    knn_amp_(0),
    db_scan_frac_(0),
    use_cosine_(false)
{

  AnyParamManager pmgr(AllParams);

  max_proj_dist_ = numeric_limits<float>::max();

  size_t        intermDim = 0;

  size_t        binThreshold = 0;
  string        projType;
  string        projSpaceType;

  pmgr.GetParamOptional("intermDim",      intermDim);
  pmgr.GetParamRequired("projDim",        proj_dim_);
  pmgr.GetParamRequired("projType",       proj_descr_);
  pmgr.GetParamOptional("binThreshold",   binThreshold);

  SetQueryTimeParamsInternal(pmgr);

  pmgr.CheckUnused();


  LOG(LIB_INFO) << "projType     = " << proj_descr_;
  LOG(LIB_INFO) << "projDim      = " << proj_dim_;
  LOG(LIB_INFO) << "intermDim    = " << intermDim;
  LOG(LIB_INFO) << "binThreshold = " << binThreshold;
  LOG(LIB_INFO) << "dbDscanFrac  = " << db_scan_frac_;
  LOG(LIB_INFO) << "knnAmp       = " << knn_amp_;
  LOG(LIB_INFO) << "maxProjDist  = " << max_proj_dist_;
  LOG(LIB_INFO) << "useQueue     = " << use_priority_queue_;
  LOG(LIB_INFO) << "useCosine    = " << use_cosine_;


  /*
   * Let's extract all parameters before doing
   * any heavy lifting. If an exception fires for some reason,
   * e.g., because the user specified a wrong projection
   * type, the destructor of the AnyParams will check
   * for unclaimed parameters. Currently, this destructor
   * terminates the application and prints and apparently unrelated
   * error message (e.g. a wrong parameter name)
   */

  proj_obj_.reset(Projection<dist_t>::createProjection(
                    space,
                    data,
                    proj_descr_,
                    intermDim,
                    proj_dim_,
                    binThreshold));

  const string   projDescStr = proj_descr_;
  vector<string> projSpaceDesc;

  ParseSpaceArg(projDescStr, projSpaceType, projSpaceDesc);
  unique_ptr<AnyParams> projSpaceParams =
            unique_ptr<AnyParams>(new AnyParams(projSpaceDesc));

  unique_ptr<ProgressDisplay> progress_bar(PrintProgress ?
                                new ProgressDisplay(data.size(), cerr)
                                :NULL);

#ifdef PROJ_CONTIGUOUS_STORAGE
  proj_vects_.resize(data.size() * proj_dim_);
  vector<float> TmpVect(proj_dim_);

  for (size_t i = 0, start = 0; i < data.size(); ++i, start += proj_dim_) {
    proj_obj_->compProj(NULL, data_[i], &TmpVect[0]);
    memcpy(&proj_vects_[start], &TmpVect[0], sizeof(proj_vects_[0])*proj_dim_); 
    if (progress_bar) ++(*progress_bar);
  }
#else
  proj_vects_.resize(data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    proj_vects_[i].resize(proj_dim_);
    proj_obj_->compProj(NULL, data_[i], &proj_vects_[i][0]);
    if (progress_bar) ++(*progress_bar);
  }
#endif
}
    
template <typename dist_t>
void 
ProjectionIndexIncremental<dist_t>::SetQueryTimeParamsInternal(AnyParamManager& pmgr) {
  pmgr.GetParamOptional("useQueue", use_priority_queue_);
  pmgr.GetParamOptional("maxProjDist", max_proj_dist_);
  pmgr.GetParamOptional("useCosine", use_cosine_);
    
  if (pmgr.hasParam("dbScanFrac") && pmgr.hasParam("knnAmp")) {
    throw runtime_error("One shouldn't specify both parameters dbScanFrac and knnAmp");
  }
  if (pmgr.hasParam("knnAmp")) {
    db_scan_frac_ = 0;
  } else {
    knn_amp_ = 0;
  }
  if (!pmgr.hasParam("dbScanFrac") && !pmgr.hasParam("knnAmp")) {
    db_scan_frac_ = 0;
    knn_amp_ = 0;
  }
  pmgr.GetParamOptional("dbScanFrac",   db_scan_frac_);
  pmgr.GetParamOptional("knnAmp",  knn_amp_);
}

template <typename dist_t>
vector<string>
ProjectionIndexIncremental<dist_t>::GetQueryTimeParamNames() const {
  vector<string> names;
  names.push_back("dbScanFrac");
  names.push_back("knnAmp");
  names.push_back("useCosine");
  names.push_back("useQueue");
  names.push_back("maxProjDist");
  return names;
}    
    

template <typename dist_t>
ProjectionIndexIncremental<dist_t>::~ProjectionIndexIncremental() {
}

template <typename dist_t>
const std::string ProjectionIndexIncremental<dist_t>::ToString() const {
  std::stringstream str;
  str <<  "projection (" << proj_descr_ << ") incr. sorting";
  return str.str();
}

template <typename dist_t> 
template <typename QueryType>
void ProjectionIndexIncremental<dist_t>::GenSearch(QueryType* query, size_t K) {
  // Let's make this check here. Otherwise, if you misspell dbScanFrac, you will get 
  // a strange error message that says: dbScanFrac should be in the range [0,1].
  if (!knn_amp_) {
    if (db_scan_frac_ < 0.0 || db_scan_frac_ > 1.0) {
      stringstream err;
      err << METH_PROJECTION_INC_SORT << " requires that dbScanFrac is in the range [0,1]";
      throw runtime_error(err.str());
    }
  }

  size_t db_scan = computeDbScan(K);


  vector<float>     QueryVect(proj_dim_);
  proj_obj_->compProj(query, query->QueryObject(), &QueryVect[0]);

  if (!use_priority_queue_) {
    std::vector<FloatInt> proj_dists;
    proj_dists.reserve(data_.size());

#ifdef PROJ_CONTIGUOUS_STORAGE
    for (size_t i = 0, start = 0; i < data_.size(); ++i, start += proj_dim_) {
      float projDist = use_cosine_ ? 
                    CosineSimilarity(&proj_vects_[start], &QueryVect[0], proj_dim_)
                    :
                    L2NormSIMD(&proj_vects_[start], &QueryVect[0], proj_dim_)
                    ;
#else
    for (size_t i = 0; i < proj_vects_.size(); ++i) {
      float projDist = use_cosine_ ? 
                    CosineSimilarity(&proj_vects_[i][0], &QueryVect[0], proj_dim_)
                    :
                    L2SqrSIMD(&proj_vects_[i][0], &QueryVect[0], proj_dim_);
#endif
      if (projDist <= max_proj_dist_)
        proj_dists.push_back(std::make_pair(projDist, i));
    }

    IncrementalQuickSelect<FloatInt> quick_select(proj_dists);

    size_t scan_qty = min(db_scan, proj_dists.size());

    for (size_t i = 0; i < scan_qty; ++i) {
      const size_t idx = quick_select.GetNext().second;
      quick_select.Next();
      query->CheckAndAddToResult(data_[idx]);
    }
  } else {
    priority_queue<FloatInt> filterQueue;

#ifdef PROJ_CONTIGUOUS_STORAGE
    for (size_t i = 0, start = 0; i < data_.size(); ++i, start += proj_dim_) {
      float projDist = L2NormSIMD(&proj_vects_[start], &QueryVect[0], proj_dim_);
#else
    for (size_t i = 0; i < proj_vects_.size(); ++i) {
      float projDist = L2NormSIMD(&proj_vects_[i][0], &QueryVect[0], proj_dim_);
#endif
      if (projDist <= max_proj_dist_) {
        filterQueue.push(std::make_pair(projDist, i));
        if (filterQueue.size() > db_scan) filterQueue.pop();
      }
    }

    while (filterQueue.size() > db_scan) filterQueue.pop();
    while (!filterQueue.empty()) {
      const size_t idx = filterQueue.top().second;
      query->CheckAndAddToResult(data_[idx]);
      filterQueue.pop();
    }
  }
}

template <typename dist_t>
void ProjectionIndexIncremental<dist_t>::Search(
    RangeQuery<dist_t>* query) {
  GenSearch(query, 0);
}

template <typename dist_t>
void ProjectionIndexIncremental<dist_t>::Search(
    KNNQuery<dist_t>* query) {
  GenSearch(query, query->GetK());
}


template class ProjectionIndexIncremental<float>;
template class ProjectionIndexIncremental<double>;
template class ProjectionIndexIncremental<int>;

}  // namespace similarity

