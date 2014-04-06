/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
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
#include "small_world_rand.h"

#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <typeinfo>
#include <unordered_set>
#include <queue>

// Set to 1, if you want to use a thread pool
#define THREAD_POOL 1

namespace similarity {


template <typename dist_t>
struct PermSearchThread {
  void operator()(SearchThreadParams<dist_t>& prm) {
    while (true) {
      {
        unique_lock<mutex>  lck1(prm.mtx_);
        while (prm.status_ == kThreadWait) prm.cv1_.wait(lck1);
  
        if (prm.status_ == kThreadFinish) break;
        if (prm.status_ != kThreadSearch) {
          stringstream err;
          err << "Bug: should not see the status other than kSearch here, got: " << prm.status_;
          throw runtime_error(err.str());
        }
      }
      prm.index_.kSearchElementsWithAttemptsSingleThread(prm.space_, prm.queryObj_, prm.NN_, 1, prm.result_);

      {
        unique_lock<mutex> lck2(prm.mtx2_);
        prm.status_ = kThreadWait;
        prm.cv2_.notify_one();
      }
    }
    LOG(INFO) << "Indexing thread finished";
  }
};

template <typename dist_t>
struct TempSearchThread {
  void operator()(SearchThreadParams<dist_t>& prm) {
    prm.index_.kSearchElementsWithAttemptsSingleThread(prm.space_, prm.queryObj_, prm.NN_, 1, prm.result_);
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

  LOG(INFO) << "NN                  = " << NN_;
  LOG(INFO) << "initIndexAttempts   = " << initIndexAttempts_;
  LOG(INFO) << "initSearchAttempts  = " << initSearchAttempts_;
  LOG(INFO) << "indexThreadQty      = " << indexThreadQty_;

#if THREAD_POOL
  if (indexThreadQty_ > 0) {
    threadResultSet_.resize(indexThreadQty_);
    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threadParams_.push_back(shared_ptr<SearchThreadParams<dist_t>>(
                          new SearchThreadParams<dist_t>(space, *this, threadResultSet_[i], NULL, NN_)));
      threads_.push_back(thread(PermSearchThread<dist_t>(), ref(*threadParams_[i])));
    }
  }
#endif

  for (size_t i = 0; i != data.size(); ++i) {
    MSWNode* node = new MSWNode(data[i]);
    add(space, node);
  }
}

template <typename dist_t>
const std::string SmallWorldRand<dist_t>::ToString() const {
  return "small_world_rand";
}

template <typename dist_t>
SmallWorldRand<dist_t>::~SmallWorldRand() {
#if THREAD_POOL 
// Let's terminate threads
  for (size_t i = 0; i < threads_.size(); ++i) {
    SearchThreadParams<dist_t>&   prm = *threadParams_[i];
    unique_lock<mutex> lck(prm.mtx_);
    threadParams_[i]->status_ = kThreadFinish;
    prm.cv1_.notify_one();
  }
  for (size_t i = 0; i < threads_.size(); ++i) {
    threads_[i].join();
  }
#endif
}

template <typename dist_t>
MSWNode* SmallWorldRand<dist_t>::getRandomEnterPoint() const
{
  size_t size = ElList.size();
  if(!ElList.size()) {
    return NULL;
  } else {
    size_t num = rand()%size;
    return ElList[num];
  }
}

#if THREAD_POOL
template <typename dist_t>
void
SmallWorldRand<dist_t>::kSearchElementsWithAttemptsMultiThread(const Space<dist_t>* space, 
                                                              const Object* queryObj, 
                                                              size_t NN, size_t initIndexAttempts,
                                                              set<EvaluatedMSWNode<dist_t>>& resultSet) const
{
  // Thread params don't change we need to create them out only once
  for (size_t i = 0; i < indexThreadQty_; ++i) {
    SearchThreadParams<dist_t>&   prm = *threadParams_[i];
    prm.queryObj_ = queryObj;
  }


  for (size_t i = 0; i < (initIndexAttempts + indexThreadQty_ - 1)/indexThreadQty_; i++) {
    size_t qty = min((i + 1) * indexThreadQty_, initIndexAttempts) - i * indexThreadQty_;

    if (qty > indexThreadQty_) {
      throw runtime_error("Bug, qty > indexThreadQty_!");
    }

    for (size_t  n = 0; n < qty; ++n) {
      SearchThreadParams<dist_t>&   prm = *threadParams_[n];
      unique_lock<mutex> lck1(prm.mtx_);
      prm.status_ = kThreadSearch;
      prm.cv1_.notify_one();
    }

    for (size_t  n = 0; n < qty; ++n) {
      SearchThreadParams<dist_t>&   prm = *threadParams_[n];
      unique_lock<mutex> lck2(prm.mtx2_);
      while (prm.status_ != kThreadWait) prm.cv2_.wait(lck2);
    }

    for (auto& e: threadResultSet_) {
      resultSet.insert(e.begin(), e.end());
      e.clear();
    }
  }

}
#else
template <typename dist_t>
void
SmallWorldRand<dist_t>::kSearchElementsWithAttemptsMultiThread(const Space<dist_t>* space, 
                                                              const Object* queryObj, 
                                                              size_t NN, size_t initIndexAttempts,
                                                              set<EvaluatedMSWNode<dist_t>>& resultSet) const
{
  vector<set<EvaluatedMSWNode<dist_t>>>     threadResultSet(indexThreadQty_);
  vector<thread>                            threads(indexThreadQty_);
  vector<SearchThreadParams<dist_t>*>       threadParams; 
  AutoVectDel<SearchThreadParams<dist_t>>   delThreadParams(threadParams); 

  // Thread params don't change we need to create them out only once
  for (size_t i = 0; i < indexThreadQty_; ++i)
    threadParams.push_back(new SearchThreadParams<dist_t>(space, *this, threadResultSet[i], queryObj, NN));

  for (size_t i = 0; i < (initIndexAttempts + indexThreadQty_ - 1)/indexThreadQty_; i++) {
    size_t qty = min((i + 1) * indexThreadQty_, initIndexAttempts) - i * indexThreadQty_;

    if (qty > indexThreadQty_) {
      throw runtime_error("Bug, qty > indexThreadQty_!");
    }

    for (size_t  n = 0; n < qty; ++n) {
      threads[n] = thread(TempSearchThread<dist_t>(), ref(*threadParams[n]));
    }
    for (size_t  n = 0; n < qty; ++n) {
      threads[n].join();
    }

    for (auto& e: threadResultSet) {
      resultSet.insert(e.begin(), e.end());
      e.clear();
    }
  }

}
#endif

template <typename dist_t>
void 
SmallWorldRand<dist_t>::kSearchElementsWithAttemptsSingleThread(const Space<dist_t>* space, 
                                                    const Object* queryObj, 
                                                    size_t NN, 
                                                    size_t initIndexAttempts,
                                                    set <EvaluatedMSWNode<dist_t>>& resultSet) const
{
  unordered_set <MSWNode*>       visitedNodes;

  for (size_t i=0; i < initIndexAttempts; i++){

    /**
     * Search for the k most closest elements to the query.
     */
    MSWNode* provider = getRandomEnterPoint();

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

      const vector<MSWNode*>& neighbor = (currEv.getMSWNode())->getAllFriends();

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
  MSWNode* enterPoint = getRandomEnterPoint();
  newElement->removeAllFriends(); 

  if(enterPoint == NULL){
    ElList.push_back(newElement);
    incSize();
    return;
  }

  set<EvaluatedMSWNode<dist_t>> viewed;

  if (!indexThreadQty_) {
    kSearchElementsWithAttemptsSingleThread(space, newElement->getData(), NN_, initIndexAttempts_, viewed);
  } else {
    kSearchElementsWithAttemptsMultiThread(space, newElement->getData(), NN_, initIndexAttempts_, viewed);
  }

  size_t i = 0;
  
  for (auto ee = viewed.rbegin(); ee != viewed.rend(); ++ee) {
    if (i >= NN_) break;
    i++;

    // link will check for duplicates
    link(((*ee).getMSWNode()), newElement);
  }

  ElList.push_back(newElement);
  incSize();
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
    MSWNode* provider = getRandomEnterPoint();

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
