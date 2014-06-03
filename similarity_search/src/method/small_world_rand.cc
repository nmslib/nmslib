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
#include <cmath>
#include <memory>
#include <iostream>

#include "space.h"
#include "knnquery.h"
#include "rangequery.h"
#include "method/small_world_rand.h"

#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <typeinfo>
#include <unordered_set>
#include <queue>

namespace similarity {

template <typename dist_t>
struct IndexThreadParamsSW {
  const Space<dist_t>*                        space_;
  SmallWorldRand<dist_t>&                     index_;
  const ObjectVector&                         data_;
  size_t                                      index_every_;
  size_t                                      out_of_;
  
  IndexThreadParamsSW(
                     const Space<dist_t>*             space,
                     SmallWorldRand<dist_t>&          index, 
                     const ObjectVector&              data,
                     size_t                           index_every,
                     size_t                           out_of
                      ) : 
                     space_(space),
                     index_(index), 
                     data_(data),
                     index_every_(index_every),
                     out_of_(out_of) 
                     {
  }
};

template <typename dist_t>
struct IndexThreadSW {
  void operator()(IndexThreadParamsSW<dist_t>& prm) {
    /* 
     * Skip the first element, it was added already
     */
    for (size_t i = 1; i < prm.data_.size(); ++i) {
      if (prm.index_every_ == i % prm.out_of_) {
        MSWNode* node = new MSWNode(prm.data_[i]);
        prm.index_.add(prm.space_, node);
      }
    }
  }
};

template <typename dist_t>
SmallWorldRand<dist_t>::SmallWorldRand(const Space<dist_t>* space,
                                                   const ObjectVector& data,
                                                   const AnyParams& MethParams) :
                                                   NN_(5),
                                                   initIndexAttempts_(2),
                                                   initSearchAttempts_(10),
                                                   size_(0),
                                                   indexThreadQty_(0)
{
  AnyParamManager pmgr(MethParams);

  pmgr.GetParamOptional("NN", NN_);
  pmgr.GetParamOptional("initIndexAttempts",  initIndexAttempts_);
  pmgr.GetParamOptional("initSearchAttempts", initSearchAttempts_);
  pmgr.GetParamOptional("indexThreadQty",     indexThreadQty_);

  LOG(LIB_INFO) << "NN                  = " << NN_;
  LOG(LIB_INFO) << "initIndexAttempts   = " << initIndexAttempts_;
  LOG(LIB_INFO) << "initSearchAttempts  = " << initSearchAttempts_;
  LOG(LIB_INFO) << "indexThreadQty      = " << indexThreadQty_;

  if (data.empty()) return;

  ElList_.push_back(new MSWNode(data[0]));

  if (indexThreadQty_ <= 1) {
    // Skip the first element, one element is already added
    for (size_t i = 1; i != data.size(); ++i) {
      MSWNode* node = new MSWNode(data[i]);
      add(space, node);
    }
  } else {
    vector<thread>                                    threads(indexThreadQty_);
    vector<shared_ptr<IndexThreadParamsSW<dist_t>>>   threadParams; 

    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threadParams.push_back(shared_ptr<IndexThreadParamsSW<dist_t>>(
                              new IndexThreadParamsSW<dist_t>(space, *this, data, i, indexThreadQty_)));
    }
    for (size_t i = 0; i < indexThreadQty_; ++i) {
      LOG(LIB_INFO) << "Creating indexing thread: " << (i+1) << " out of " << indexThreadQty_;
      threads[i] = thread(IndexThreadSW<dist_t>(), ref(*threadParams[i]));
    }
    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threads[i].join();
    }
  }
}

template <typename dist_t>
void 
SmallWorldRand<dist_t>::SetQueryTimeParamsInternal(AnyParamManager& pmgr) {
  pmgr.GetParamOptional("initSearchAttempts", initSearchAttempts_);
}

template <typename dist_t>
vector<string>
SmallWorldRand<dist_t>::GetQueryTimeParamNames() const {
  vector<string> names;
  names.push_back("initSearchAttempts");
  return names;
}

template <typename dist_t>
const std::string SmallWorldRand<dist_t>::ToString() const {
  return "small_world_rand";
}

template <typename dist_t>
SmallWorldRand<dist_t>::~SmallWorldRand() {
}

template <typename dist_t>
MSWNode* SmallWorldRand<dist_t>::getRandomEntryPointLocked() const
{
  unique_lock<mutex> lock(ElListGuard_);
  MSWNode* res = getRandomEntryPoint();
  return res;
}

template <typename dist_t>
MSWNode* SmallWorldRand<dist_t>::getRandomEntryPoint() const {
  size_t size = ElList_.size();

  if(!ElList_.size()) {
    return NULL;
  } else {
    size_t num = rand()%size;
    return ElList_[num];
  }
}


template <typename dist_t>
void 
SmallWorldRand<dist_t>::kSearchElementsWithAttempts(const Space<dist_t>* space, 
                                                    const Object* queryObj, 
                                                    size_t NN, 
                                                    size_t initIndexAttempts,
                                                    set <EvaluatedMSWNode<dist_t>>& resultSet) const
{
  resultSet.clear();
  unordered_set <MSWNode*>       visitedNodes;

  for (size_t i=0; i < initIndexAttempts; i++){

    /**
     * Search for the k most closest elements to the query.
     */
    MSWNode* provider = getRandomEntryPointLocked();

    priority_queue <dist_t>                     closestDistQueue;                      
    priority_queue <EvaluatedMSWNode<dist_t>>   candidateSet; 

    dist_t d = space->IndexTimeDistance(queryObj, provider->getData());
    EvaluatedMSWNode<dist_t> ev(d, provider);

    candidateSet.push(ev);
    closestDistQueue.push(d);
    visitedNodes.insert(provider);
    resultSet.insert(ev);

    while (!candidateSet.empty()) {
      const EvaluatedMSWNode<dist_t>& currEv = candidateSet.top();
      dist_t lowerBound = closestDistQueue.top();

      /*
       * Check if we reached a local minimum.
       */
      if (currEv.getDistance() > lowerBound) {
        break;
      }
      MSWNode* currNode = currEv.getMSWNode();

      /*
       * This lock protects currNode from being modified
       * while we are accessing elements of currNode.
       */
      unique_lock<mutex>  lock(currNode->accessGuard_);

      const vector<MSWNode*>& neighbor = currNode->getAllFriends();

      // Can't access curEv anymore! The reference would become invalid
      candidateSet.pop();

      // calculate distance to each neighbor
      for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter){
        if (visitedNodes.find(*iter) == visitedNodes.end()){
          d = space->IndexTimeDistance(queryObj, (*iter)->getData());
          EvaluatedMSWNode<dist_t> evE1(d, *iter);
          visitedNodes.insert(*iter);
          closestDistQueue.push(d);
          if (closestDistQueue.size() > NN) {
            closestDistQueue.pop();
          }
          candidateSet.push(evE1);
          resultSet.insert(evE1);
        }
      }
    }
  }
}

template <typename dist_t>
void SmallWorldRand<dist_t>::add(const Space<dist_t>* space, MSWNode *newElement){
  newElement->removeAllFriends(); 

  if(ElList_.empty()){
    throw runtime_error("Bug: create at least one element, before calling the function add!");
    return;
  }

  set<EvaluatedMSWNode<dist_t>> viewed;

  kSearchElementsWithAttempts(space, newElement->getData(), NN_, initIndexAttempts_, viewed);

  size_t i = 0;
  
  for (auto ee = viewed.rbegin(); ee != viewed.rend(); ++ee) {
    if (i >= NN_) break;
    i++;

    /* 
     * link will 
     * 1) check for duplicates
     * 2) update each node in a node-specific critical section (supported via mutex)
     */
    link(((*ee).getMSWNode()), newElement);
  }

  unique_lock<mutex> lock(ElListGuard_);

  ElList_.push_back(newElement);
}

template <typename dist_t>
void SmallWorldRand<dist_t>::Search(RangeQuery<dist_t>* query) {
  throw runtime_error("Range search is not supported!");
}

template <typename dist_t>
void SmallWorldRand<dist_t>::Search(KNNQuery<dist_t>* query) {
  size_t k = query->GetK();
  set <EvaluatedMSWNode<dist_t>>    resultSet;
  unordered_set <MSWNode*>          visitedNodes;

  for (size_t i=0; i < initSearchAttempts_; i++) {
  /**
   * Search of most k-closest elements to the query.
   */
    MSWNode* provider = getRandomEntryPoint();

    priority_queue <dist_t>                   closestDistQueue; //The set of all elements which distance was calculated
    priority_queue <EvaluatedMSWNode<dist_t>> candidateSet; //the set of elements which we can use to evaluate

    dist_t d = query->DistanceObjLeft(provider->getData());
    EvaluatedMSWNode<dist_t> ev(d, provider);

    candidateSet.push(ev);
    closestDistQueue.push(d);
    visitedNodes.insert(provider);
    resultSet.insert(ev);

    while(!candidateSet.empty()){

    auto iter = candidateSet.top();
    const EvaluatedMSWNode<dist_t>& currEv = iter;
    dist_t lowerBound = closestDistQueue.top();

      // Did we reach a local minimum?
      if (currEv.getDistance() > lowerBound) {
        break;
      }

      const vector<MSWNode*>& neighbor = (currEv.getMSWNode())->getAllFriends();

      // Can't access curEv anymore! The reference would become invalid
      candidateSet.pop();

      //calculate distance to each neighbor
      for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter){
        if (visitedNodes.find(*iter) == visitedNodes.end()){
            d = query->DistanceObjLeft((*iter)->getData());
            EvaluatedMSWNode<dist_t> evE1(d, *iter);
            visitedNodes.insert(*iter);
            closestDistQueue.push(d);
            if (closestDistQueue.size() > NN_) { 
              closestDistQueue.pop(); 
            }
            candidateSet.push(evE1);
            resultSet.insert(evE1);
          }
      }
    }
  }

  auto iter = resultSet.rbegin();

  while(k && iter != resultSet.rend()) {
    query->CheckAndAddToResult(iter->getDistance(), iter->getMSWNode()->getData());
    iter++;
    k--;
  }
}



template class SmallWorldRand<float>;
template class SmallWorldRand<double>;
template class SmallWorldRand<int>;

}
