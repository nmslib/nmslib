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
#include <typeinfo>
#include <unordered_set>
#include <queue>

namespace similarity {

template <typename dist_t>
SmallWorldRand<dist_t>::SmallWorldRand(const Space<dist_t>* space,
                                                   const ObjectVector& data,
                                                   const AnyParams& MethParams) :
                                                   NN_(5),
                                                   initIndexAttempts_(2),
                                                   initSearchAttempts_(10),
                                                   size_(0)
{
  AnyParamManager pmgr(MethParams);

  pmgr.GetParamOptional("NN", NN_);
  pmgr.GetParamOptional("initIndexAttempts", initIndexAttempts_);
  pmgr.GetParamOptional("initSearchAttempts", initSearchAttempts_);

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
SmallWorldRand<dist_t>::~SmallWorldRand() {}

template <typename dist_t>
MSWNode* SmallWorldRand<dist_t>::getRandomEnterPoint()
{
  size_t size = ElList.size();
  if(!ElList.size()) {
    return NULL;
  } else {
    size_t num = rand()%size;
    return ElList[num];
  }
}

template <typename dist_t> dist_t SmallWorldRand<dist_t>::getKDistance(const set<EvaluatedMSWNode<dist_t>>& ElementSet, size_t k)
{
  if (k >= ElementSet.size()) {
    return (*ElementSet.rbegin()).getDistance();
  }
  size_t i=0;
  for (auto el = ElementSet.begin(); el != ElementSet.end(); ++el) {
    if (i>=k) return (*el).getDistance();
    i++;
  }
  return 0; // to shut up the compiler
}

template <typename dist_t>
set <EvaluatedMSWNode<dist_t>>
SmallWorldRand<dist_t>::kSearchElementsWithAttempts(const Space<dist_t>* space, MSWNode* query, size_t NN, size_t initIndexAttempts)
{
  set <EvaluatedMSWNode<dist_t>> resultSet; 
  unordered_set <MSWNode*>       visitedNodes;

  for (size_t i=0; i < initIndexAttempts; i++){

    /**
     * Search for the k most closest elements to the query.
     */
    MSWNode* provider = getRandomEnterPoint();

    priority_queue <dist_t>                     closestDistQueue;                      
    priority_queue <EvaluatedMSWNode<dist_t>>   candidateSet; 

    dist_t d = space->IndexTimeDistance(query->getData(), provider->getData());
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
          d = space->IndexTimeDistance(query->getData(), (*iter)->getData());
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

  return resultSet;
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

  set<EvaluatedMSWNode<dist_t>> viewed = kSearchElementsWithAttempts(space, newElement, NN_, initIndexAttempts_);

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
