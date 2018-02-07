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
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <memory>

#include "space.h"
#include "knnqueue.h"
#include "knnquery.h"
#include "rangequery.h"
#include "methodfactory.h"
#include "method/multi_index.h"

namespace similarity {

using std::unique_ptr;

template <typename dist_t>
MultiIndex<dist_t>::MultiIndex(
         bool PrintProgress,
         const string& SpaceType,
         Space<dist_t>& space, 
         const ObjectVector& data) : Index<dist_t>(data), space_(space), SpaceType_(SpaceType), PrintProgress_(PrintProgress) {}


template <typename dist_t>
void MultiIndex<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamRequired("indexQty", IndexQty_);
  pmgr.GetParamRequired("methodName", MethodName_);

  AnyParams RemainParams = pmgr.ExtractParametersExcept( {"indexQty", "methodName"} );

  for (size_t i = 0; i < IndexQty_; ++i) {
    LOG(LIB_INFO) << "Method: " << MethodName_ << " index # " << (i+1) << " out of " << IndexQty_;
    indices_.push_back(MethodFactoryRegistry<dist_t>::Instance().CreateMethod(PrintProgress_, 
                                                                 MethodName_,
                                                                 SpaceType_,
                                                                 space_,
                                                                 this->data_));

    indices_.back()->CreateIndex(RemainParams);
  }

  this->ResetQueryTimeParams(); // reset query time parameters
}

template <typename dist_t>
void MultiIndex<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  for (size_t i = 0; i < indices_.size(); ++i) {
    AnyParams ParamCopy(QueryTimeParams);
    indices_[i]->SetQueryTimeParams(ParamCopy);
  }
}

template <typename dist_t>
MultiIndex<dist_t>::~MultiIndex() {
  for (size_t i = 0; i < indices_.size(); ++i) 
    delete indices_[i];
}

template <typename dist_t>
const std::string MultiIndex<dist_t>::StrDesc() const {
  std::stringstream str;
  str << "" << indices_.size() << " copies of " << MethodName_;
  return str.str();
}

template <typename dist_t>
void MultiIndex<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  /* 
   * There may be duplicates: the same object coming from 
   * different indices. The set found is used to filter them out.
   */
  std::unordered_set<const Object*> found;

  for (size_t i = 0; i < indices_.size(); ++i) {
    RangeQuery<dist_t>  TmpRes(space_, query->QueryObject(), query->Radius());
    indices_[i]->Search(&TmpRes);
    const ObjectVector& res          = *TmpRes.Result();
    const std::vector<dist_t>& dists = *TmpRes.ResultDists();

    query->AddDistanceComputations(TmpRes.DistanceComputations());
    for (size_t i = 0; i < res.size(); ++i) {
      const Object* obj = res[i];
      if (!found.count(obj)) {
        query->CheckAndAddToResult(dists[i], obj);
        found.insert(obj);
      }
    }
  }
}

template <typename dist_t>
void MultiIndex<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  /* 
   * There may be duplicates: the same object coming from 
   * different indices. The set found is used to filter them out.
   */
  std::unordered_set<IdType> found;

  for (size_t i = 0; i < indices_.size(); ++i) {
    KNNQuery<dist_t> TmpRes(space_, query->QueryObject(), query->GetK(), query->GetEPS());

    indices_[i]->Search(&TmpRes);
    unique_ptr<KNNQueue<dist_t>> ResQ(TmpRes.Result()->Clone());

    query->AddDistanceComputations(TmpRes.DistanceComputations());
    while(!ResQ->Empty()) {
      const Object* obj = reinterpret_cast<const Object*>(ResQ->TopObject());

      /*  
       * It's essential to check for previously added entries.
       * If we don't do this, the same close entry may be added multiple
       * times. At the same time, other relevant entries will be removed!
       */
      if (!found.count(obj->id())) {
        query->CheckAndAddToResult(ResQ->TopDistance(), obj);
        found.insert(obj->id());
      }
      ResQ->Pop();
    }
  }
}

template class MultiIndex<float>;
template class MultiIndex<double>;
template class MultiIndex<int>;

}   // namespace similarity
