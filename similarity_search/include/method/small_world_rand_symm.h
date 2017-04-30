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

#ifndef _SMALL_WORLD_RAND_SYMM_H_
#define _SMALL_WORLD_RAND_SYMM_H_

#include "index.h"
#include "params.h"
#include <set>
#include <limits>
#include <iostream>
#include <map>
#include <unordered_set>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>

#include <method/small_world_rand.h>
#include <method/sym_seqsearch.h>

#define METH_SMALL_WORLD_RAND_SYMM         "sw-graph-symm"

namespace similarity {

using std::string;
using std::vector;
using std::thread;
using std::mutex;
using std::unique_lock;
using std::condition_variable;
using std::ref;

template <typename dist_t>
class Space;

/*
 *
 * A small world approach. It builds the knn-graph incrementally and relies on 
 * a straightforward randomized algorithm to insert an element. This modification 
 * is different in that it explicitly supports index- and query-time 
 * distance symmetrization.
 *
 */

//----------------------------------
template <typename dist_t>
class SmallWorldRandSymm : public Index<dist_t> {
public:
  virtual void SaveIndex(const string &location) override;

  virtual void LoadIndex(const string &location) override;

  SmallWorldRandSymm(bool PrintProgress,
                 const Space<dist_t>& space,
                 const ObjectVector& data);
  void CreateIndex(const AnyParams& IndexParams) override;

  ~SmallWorldRandSymm();

  typedef std::vector<MSWNode*> ElementList;

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;
  MSWNode* getRandomEntryPoint() const;
  MSWNode* getRandomEntryPointLocked() const;
  size_t getEntryQtyLocked() const;
   
  void searchForIndexing(const Object *queryObj,
                         std::priority_queue<EvaluatedMSWNodeDirect<dist_t>> &resultSet) const;
  void add(MSWNode *newElement);
  void addCriticalSection(MSWNode *newElement);
  void link(MSWNode* first, MSWNode* second){
    // addFriend checks for duplicates if the second argument is true
    first->addFriend(second, true);
    second->addFriend(first, true);
  }

  void SetQueryTimeParams(const AnyParams& ) override;
private:

  size_t                NN_;
  size_t                efConstruction_;
  size_t                efSearch_;
  size_t                initIndexAttempts_;
  size_t                initSearchAttempts_;
  size_t                indexThreadQty_;
  string                pivotFile_;
  ObjectVector          pivots_;

  const Space<dist_t>&  space_;
  ObjectVector          data_; // We copy all the data
  bool                  PrintProgress_;

  bool                  use_proxy_dist_;
  SymmType              indexSymm_ = kSymmNone;
  SymmType              querySymm_ = kSymmNone;
  size_t                symmCandK_ = 0;

  mutable mutex   ElListGuard_;
  ElementList     ElList_;

  void SearchInternal(const KNNQuery<dist_t>& query, KNNQueue<dist_t>& resQueue) const;

  dist_t IndexTimeSymmDistance(const Object* queryObj, const Object* dataObj) const {
    return use_proxy_dist_ ?  space_.ProxyDistance(queryObj, dataObj) : 
                              SymmDistance(space_, queryObj, dataObj, indexSymm_);
  }

  dist_t QueryTimeSymmDistance(const KNNQuery<dist_t>& query, const Object* dataObj) const {
    return SymmDistance(query, dataObj, indexSymm_);
  }

protected:

  DISABLE_COPY_AND_ASSIGN(SmallWorldRandSymm);
};




}

#endif
