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

#ifndef _SMALL_WORLD_RAND_H_
#define _SMALL_WORLD_RAND_H_

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


#define METH_SMALL_WORLD_RAND                 "small_world_rand"

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
 * a straightforward randomized algorithm to insert an element. 
 * This work is described in a paper:
 * 
 * Malkov, Yury, Alexander Ponomarenko, Andrey Logvinov, and Vladimir Krylov. 
 * "Approximate nearest neighbor algorithm based on navigable small world graphs." Information Systems (2013).
 *
 */

//----------------------------------
class MSWNode{
public:
  MSWNode(const Object *Obj, size_t id) {
    data_ = Obj;
    id_ = id;
  }
  ~MSWNode(){};
  void removeAllFriends(){
    friends.clear();
  }
  /* 
   * 1. The list of friend pointers is sorted.
   * 2. addFriend checks for duplicates using binary searching
   */
  void addFriend(MSWNode* element) {
    unique_lock<mutex> lock(accessGuard_);

    auto it = lower_bound(friends.begin(), friends.end(), element);
    if (it == friends.end() || (*it) != element) {
      friends.insert(it, element);
    }
  }
  const Object* getData() const {
    return data_;
  }
  size_t getId() const { return id_; }
  /* 
   * THIS NOTE APPLIES ONLY TO THE INDEXING PHASE:
   *
   * Before getting access to the friends,
   * one needs to lock the mutex accessGuard_
   * The mutex can be released ONLY when
   * we exit the scope that has access to
   * the reference returned by getAllFriends()
   */
  const vector<MSWNode*>& getAllFriends() const {
    return friends;
  }

  mutex accessGuard_;

private:
  const Object*       data_;
  size_t              id_;
  vector<MSWNode*>    friends;
};
//----------------------------------
template <typename dist_t>
class EvaluatedMSWNodeReverse{
public:
  EvaluatedMSWNodeReverse() {
      distance = 0;
      element = NULL;
  }
  EvaluatedMSWNodeReverse(dist_t di, MSWNode* node) {
    distance = di;
    element = node;
  }
  ~EvaluatedMSWNodeReverse(){}
  dist_t getDistance() const {return distance;}
  MSWNode* getMSWNode() const {return element;}
  bool operator< (const EvaluatedMSWNodeReverse &obj1) const {
    return (distance > obj1.getDistance());
  }

private:
  dist_t distance;
  MSWNode* element;
};

template <typename dist_t>
class EvaluatedMSWNodeDirect{
public:
  EvaluatedMSWNodeDirect() {
      distance = 0;
      element = NULL;
  }
  EvaluatedMSWNodeDirect(dist_t di, MSWNode* node) {
    distance = di;
    element = node;
  }
  ~EvaluatedMSWNodeDirect(){}
  dist_t getDistance() const {return distance;}
  MSWNode* getMSWNode() const {return element;}
  bool operator< (const EvaluatedMSWNodeDirect &obj1) const {
    return (distance < obj1.getDistance());
  }

private:
  dist_t distance;
  MSWNode* element;
};

//----------------------------------
template <typename dist_t>
class SmallWorldRand : public Index<dist_t> {
public:
  SmallWorldRand(bool PrintProgress,
                 const Space<dist_t>* space,
                 const ObjectVector& data,
                 const AnyParams& MethParams);
  ~SmallWorldRand();

  typedef std::vector<MSWNode*> ElementList;

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);
  MSWNode* getRandomEntryPoint() const;
  MSWNode* getRandomEntryPointLocked() const;
  size_t getEntryQtyLocked() const;
   
  void kSearchElementsWithAttempts(const Space<dist_t>* space, 
                                   const Object* queryObj, size_t NN, 
                                   size_t initAttempts, 
                                   std::priority_queue<EvaluatedMSWNodeDirect<dist_t>>& resultSet) const;
  void add(const Space<dist_t>* space, MSWNode *newElement);
  void addCriticalSection(MSWNode *newElement);
  void link(MSWNode* first, MSWNode* second){
    // addFriend checks for duplicates
    first->addFriend(second);
    second->addFriend(first);
  }

  virtual vector<string> GetQueryTimeParamNames() const;

private:
  virtual void SetQueryTimeParamsInternal(AnyParamManager& );

  size_t                NN_;
  size_t                initIndexAttempts_;
  size_t                initSearchAttempts_;
  size_t                indexThreadQty_;
  const ObjectVector&   data_;

  mutable mutex   ElListGuard_;
  ElementList     ElList_;

protected:

  DISABLE_COPY_AND_ASSIGN(SmallWorldRand);
};




}

#endif
