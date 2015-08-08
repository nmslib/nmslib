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

#ifndef _SMALL_WORLD_RAND_OLD_H_
#define _SMALL_WORLD_RAND_OLD_H_

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


#define METH_SMALL_WORLD_RAND_OLD                 "small_world_rand_old"

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
 * The main publication is as follows, but the basic algorithm was also presented as SISAP'12: 
 *    Malkov, Y., Ponomarenko, A., Logvinov, A., & Krylov, V., 2014. 
 *    Approximate nearest neighbor algorithm based on navigable small world graphs. Information Systems, 45, 61-68. 
 *
 */

//----------------------------------
class MSWNodeOld{
public:
  MSWNodeOld(const Object *Obj){
    data_ = Obj;
  }
  ~MSWNodeOld(){};
  void removeAllFriends(){
    friends.clear();
  }
  /* 
   * 1. The list of friend pointers is sorted.
   * 2. addFriend checks for duplicates using binary searching
   */
  void addFriend(MSWNodeOld* element) {
    unique_lock<mutex> lock(accessGuard_);

    auto it = lower_bound(friends.begin(), friends.end(), element);
    if (it == friends.end() || (*it) != element) {
      friends.insert(it, element);
    }
  }
  const Object* getData() const {
    return data_;
  }
  /* 
   * THIS NOTE APPLIES ONLY TO THE INDEXING PHASE:
   *
   * Before getting access to the friends,
   * one needs to lock the mutex accessGuard_
   * The mutex can be released ONLY when
   * we exit the scope that has access to
   * the reference returned by getAllFriends()
   */
  const vector<MSWNodeOld*>& getAllFriends() const {
    return friends;
  }

  mutex accessGuard_;

private:
  const Object*       data_;
  vector<MSWNodeOld*>    friends;
};
//----------------------------------
template <typename dist_t>
class EvaluatedMSWNodeOld{
public:
  EvaluatedMSWNodeOld() {
      distance = 0;
      element = NULL;
  }
  EvaluatedMSWNodeOld(dist_t di, MSWNodeOld* node) {
    distance = di;
    element = node;
  }
  ~EvaluatedMSWNodeOld(){}
  dist_t getDistance() const {return distance;}
  MSWNodeOld* getMSWNodeOld() const {return element;}
  bool operator< (const EvaluatedMSWNodeOld &obj1) const {
    return (distance > obj1.getDistance());
  }

private:
  dist_t distance;
  MSWNodeOld* element;
};

//----------------------------------
template <typename dist_t>
class SmallWorldRandOld : public Index<dist_t> {
public:
  SmallWorldRandOld(bool PrintProgress,
                 const Space<dist_t>* space,
                 const ObjectVector& data,
                 const AnyParams& MethParams);
  ~SmallWorldRandOld();

  typedef std::vector<MSWNodeOld*> ElementList;

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);
  MSWNodeOld* getRandomEntryPoint() const;
  MSWNodeOld* getRandomEntryPointLocked() const;
   
  void kSearchElementsWithAttempts(const Space<dist_t>* space, 
                                   const Object* queryObj, size_t NN, 
                                   size_t initAttempts, 
                                   set<EvaluatedMSWNodeOld<dist_t>>& resultSet) const;
  void add(const Space<dist_t>* space, MSWNodeOld *newElement);
  void link(MSWNodeOld* first, MSWNodeOld* second){
    // addFriend checks for duplicates
    first->addFriend(second);
    second->addFriend(first);
  }

  virtual vector<string> GetQueryTimeParamNames() const;

private:
  virtual void SetQueryTimeParamsInternal(AnyParamManager& );

  size_t NN_;
  size_t initIndexAttempts_;
  size_t initSearchAttempts_;
  size_t size_;
  size_t indexThreadQty_;

  mutable mutex   ElListGuard_;
  ElementList     ElList_;

protected:

  DISABLE_COPY_AND_ASSIGN(SmallWorldRandOld);
};




}

#endif
