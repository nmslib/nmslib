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
#include "ported_boost_progress.h"
#include "method/small_world_rand_old.h"

#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <typeinfo>
#include <unordered_set>
#include <queue>

namespace similarity {

template <typename dist_t>
struct IndexThreadParamsSWOld {
  const Space<dist_t>*                        space_;
  SmallWorldRandOld<dist_t>&                     index_;
  const ObjectVector&                         data_;
  size_t                                      index_every_;
  size_t                                      out_of_;
  ProgressDisplay*                            progress_bar_;
  mutex&                                      display_mutex_;
  size_t                                      progress_update_qty_;
  
  IndexThreadParamsSWOld(
                     const Space<dist_t>*             space,
                     SmallWorldRandOld<dist_t>&          index, 
                     const ObjectVector&              data,
                     size_t                           index_every,
                     size_t                           out_of,
                     ProgressDisplay*                 progress_bar,
                     mutex&                           display_mutex,
                     size_t                           progress_update_qty
                      ) : 
                     space_(space),
                     index_(index), 
                     data_(data),
                     index_every_(index_every),
                     out_of_(out_of),
                     progress_bar_(progress_bar),
                     display_mutex_(display_mutex),
                     progress_update_qty_(progress_update_qty)
                     { }
};

template <typename dist_t>
struct IndexThreadSW {
  void operator()(IndexThreadParamsSWOld<dist_t>& prm) {
    ProgressDisplay*  progress_bar = prm.progress_bar_;
    mutex&            display_mutex(prm.display_mutex_); 
    /* 
     * Skip the first element, it was added already
     */
    size_t nextQty = prm.progress_update_qty_;
    for (size_t i = 1; i < prm.data_.size(); ++i) {
      if (prm.index_every_ == i % prm.out_of_) {
        MSWNodeOld* node = new MSWNodeOld(prm.data_[i]);
        prm.index_.add(prm.space_, node);
      
        if ((i + 1 >= min(prm.data_.size(), nextQty)) && progress_bar) {
          unique_lock<mutex> lock(display_mutex);
          (*progress_bar) += (nextQty - progress_bar->count());
          nextQty += prm.progress_update_qty_;
        }
      }
    }
    if (progress_bar) {
      unique_lock<mutex> lock(display_mutex);
      (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
    }
  }
};

template <typename dist_t>
SmallWorldRandOld<dist_t>::SmallWorldRandOld(bool PrintProgress,
                                       const Space<dist_t>* space,
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

  ElList_.push_back(new MSWNodeOld(data[0]));

  unique_ptr<ProgressDisplay> progress_bar(PrintProgress ?
                                new ProgressDisplay(data.size(), cerr)
                                :NULL);

  if (indexThreadQty_ <= 1) {
    // Skip the first element, one element is already added
    if (progress_bar) ++(*progress_bar);
    for (size_t i = 1; i != data.size(); ++i) {
      MSWNodeOld* node = new MSWNodeOld(data[i]);
      add(space, node);
      if (progress_bar) ++(*progress_bar);
    }
  } else {
    vector<thread>                                    threads(indexThreadQty_);
    vector<shared_ptr<IndexThreadParamsSWOld<dist_t>>>   threadParams; 
    mutex                                             progressBarMutex;

    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threadParams.push_back(shared_ptr<IndexThreadParamsSWOld<dist_t>>(
                              new IndexThreadParamsSWOld<dist_t>(space, *this, data, i, indexThreadQty_,
                                                              progress_bar.get(), progressBarMutex, 200)));
    }
    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threads[i] = thread(IndexThreadSW<dist_t>(), ref(*threadParams[i]));
    }
    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threads[i].join();
    }
    LOG(LIB_INFO) << indexThreadQty_ << " indexing threads have finished";
  }
}

template <typename dist_t>
void 
SmallWorldRandOld<dist_t>::SetQueryTimeParamsInternal(AnyParamManager& pmgr) {
  pmgr.GetParamOptional("initSearchAttempts", initSearchAttempts_);
}

template <typename dist_t>
vector<string>
SmallWorldRandOld<dist_t>::GetQueryTimeParamNames() const {
  vector<string> names;
  names.push_back("initSearchAttempts");
  return names;
}

template <typename dist_t>
const std::string SmallWorldRandOld<dist_t>::ToString() const {
  return "small_world_rand_old";
}

template <typename dist_t>
SmallWorldRandOld<dist_t>::~SmallWorldRandOld() {
}

template <typename dist_t>
MSWNodeOld* SmallWorldRandOld<dist_t>::getRandomEntryPointLocked() const
{
  unique_lock<mutex> lock(ElListGuard_);
  MSWNodeOld* res = getRandomEntryPoint();
  return res;
}

template <typename dist_t>
MSWNodeOld* SmallWorldRandOld<dist_t>::getRandomEntryPoint() const {
  size_t size = ElList_.size();

  if(!ElList_.size()) {
    return NULL;
  } else {
    size_t num = RandomInt()%size;
    return ElList_[num];
  }
}


template <typename dist_t>
void 
SmallWorldRandOld<dist_t>::kSearchElementsWithAttempts(const Space<dist_t>* space, 
                                                    const Object* queryObj, 
                                                    size_t NN, 
                                                    size_t initIndexAttempts,
                                                    set <EvaluatedMSWNodeOld<dist_t>>& resultSet) const
{
  resultSet.clear();
  unordered_set <MSWNodeOld*>       visitedNodes;

  for (size_t i=0; i < initIndexAttempts; i++){

    /**
     * Search for the k most closest elements to the query.
     */
    MSWNodeOld* provider = getRandomEntryPointLocked();

    priority_queue <dist_t>                     closestDistQueue;                      
    priority_queue <EvaluatedMSWNodeOld<dist_t>>   candidateSet; 

    dist_t d = space->IndexTimeDistance(queryObj, provider->getData());
    EvaluatedMSWNodeOld<dist_t> ev(d, provider);

    candidateSet.push(ev);
    closestDistQueue.push(d);
    visitedNodes.insert(provider);
    resultSet.insert(ev);

    while (!candidateSet.empty()) {
      const EvaluatedMSWNodeOld<dist_t>& currEv = candidateSet.top();
      dist_t lowerBound = closestDistQueue.top();

      /*
       * Check if we reached a local minimum.
       */
      if (currEv.getDistance() > lowerBound) {
        break;
      }
      MSWNodeOld* currNode = currEv.getMSWNodeOld();

      /*
       * This lock protects currNode from being modified
       * while we are accessing elements of currNode.
       */
      unique_lock<mutex>  lock(currNode->accessGuard_);

      const vector<MSWNodeOld*>& neighbor = currNode->getAllFriends();

      // Can't access curEv anymore! The reference would become invalid
      candidateSet.pop();

      // calculate distance to each neighbor
      for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter){
        if (visitedNodes.find(*iter) == visitedNodes.end()){
          d = space->IndexTimeDistance(queryObj, (*iter)->getData());
          EvaluatedMSWNodeOld<dist_t> evE1(d, *iter);
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
void SmallWorldRandOld<dist_t>::add(const Space<dist_t>* space, MSWNodeOld *newElement){
  newElement->removeAllFriends(); 

  if(ElList_.empty()){
    throw runtime_error("Bug: create at least one element, before calling the function add!");
    return;
  }

  set<EvaluatedMSWNodeOld<dist_t>> viewed;

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
    link(((*ee).getMSWNodeOld()), newElement);
  }

  unique_lock<mutex> lock(ElListGuard_);

  ElList_.push_back(newElement);
}

template <typename dist_t>
void SmallWorldRandOld<dist_t>::Search(RangeQuery<dist_t>* query) {
  throw runtime_error("Range search is not supported!");
}

template <typename dist_t>
void SmallWorldRandOld<dist_t>::Search(KNNQuery<dist_t>* query) {
  size_t k = query->GetK();
  set <EvaluatedMSWNodeOld<dist_t>>    resultSet;
  unordered_set <MSWNodeOld*>          visitedNodes;

  for (size_t i=0; i < initSearchAttempts_; i++) {
  /**
   * Search of most k-closest elements to the query.
   */
    MSWNodeOld* provider = getRandomEntryPoint();

    priority_queue <dist_t>                   closestDistQueue; //The set of all elements which distance was calculated
    priority_queue <EvaluatedMSWNodeOld<dist_t>> candidateSet; //the set of elements which we can use to evaluate

    dist_t d = query->DistanceObjLeft(provider->getData());
    EvaluatedMSWNodeOld<dist_t> ev(d, provider);

    candidateSet.push(ev);
    closestDistQueue.push(d);
    visitedNodes.insert(provider);
    resultSet.insert(ev);

    while(!candidateSet.empty()){

    auto iter = candidateSet.top();
    const EvaluatedMSWNodeOld<dist_t>& currEv = iter;
    dist_t lowerBound = closestDistQueue.top();

      // Did we reach a local minimum?
      if (currEv.getDistance() > lowerBound) {
        break;
      }

      const vector<MSWNodeOld*>& neighbor = (currEv.getMSWNodeOld())->getAllFriends();

      // Can't access curEv anymore! The reference would become invalid
      candidateSet.pop();

      //calculate distance to each neighbor
      for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter){
        if (visitedNodes.find(*iter) == visitedNodes.end()){
            d = query->DistanceObjLeft((*iter)->getData());
            EvaluatedMSWNodeOld<dist_t> evE1(d, *iter);
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
    query->CheckAndAddToResult(iter->getDistance(), iter->getMSWNodeOld()->getData());
    iter++;
    k--;
  }
}



template class SmallWorldRandOld<float>;
template class SmallWorldRandOld<double>;
template class SmallWorldRandOld<int>;

}
