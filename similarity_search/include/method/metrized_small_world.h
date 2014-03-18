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

#ifndef _METRIZED_SMALL_WORLD_H_
#define _METRIZED_SMALL_WORLD_H_

#include "index.h"
#include "params.h"
//#include <vector>
#include <set>
#include <limits>
#include <iostream>
#include <map>
#include <unordered_set>


#define METH_METRIZED_SMALL_WORLD                 "metrized_small_world"

namespace similarity {

using std::string;

template <typename dist_t>
class Space;

//----------------------------------
class MSWNode{
public:
	MSWNode(const Object *Obj){
		data_ = Obj;
	}
	~MSWNode(){};
	void removeAllFriends(){
		friends.clear();
	}
	void addFriend(MSWNode* element){
		friends.insert(element);
	}
	const Object* getData(){
		return data_;
	}
	const set<MSWNode*>& getAllFriends(){
		return friends;
	}

private:
	const Object* data_;
	set<MSWNode*> friends;
};
//----------------------------------
template <typename dist_t>
class EvaluatedMSWNode{
public:
	EvaluatedMSWNode(){
			distance = 0;
			element = NULL;
		}
	EvaluatedMSWNode(dist_t di, MSWNode* node){
		distance = di;
		element = node;
	}
	~EvaluatedMSWNode(){}
	dist_t getDistance() const {return distance;}
	MSWNode* getMSWNode() const {return element;}
    bool operator< (const EvaluatedMSWNode &obj1) const
	{
    	if (distance < obj1.getDistance())
		return true;
    	else
    	return false;
	}

private:
	dist_t distance;
	MSWNode* element;
};
//----------------------------------
template <typename dist_t>
class SearchResult{
public:
	SearchResult(set<EvaluatedMSWNode<dist_t>> viewedList, int steps){
		viewedList_ = viewedList;
		steps_ = steps;
	}
	~SearchResult(){}
	const set<EvaluatedMSWNode<dist_t>>& getViewedList(){
		return viewedList_;
	}
	int getSteps(){
		return steps_;
	}
private:
	set<EvaluatedMSWNode<dist_t>> viewedList_;
	int steps_;
};
//----------------------------------
template <typename dist_t>
class Metrized_small_world : public Index<dist_t> {
public:
 Metrized_small_world(const Space<dist_t>* space,
                      const ObjectVector& data,
                      const AnyParams& MethParams);
 ~Metrized_small_world();

 typedef std::vector<MSWNode*> ElementList;

 //void GenSearch(QueryType* query);
 const std::string ToString() const;
 void Search(RangeQuery<dist_t>* query);
 void Search(KNNQuery<dist_t>* query);
 MSWNode* getRandomEnterPoint();
 dist_t getKDistance(const set<EvaluatedMSWNode<dist_t>>& ElementSet, size_t k);
 SearchResult<dist_t> kSearchElementsWithAttempts(const Space<dist_t>* space, MSWNode* query, int NN, int initAttempts);
 void add(const Space<dist_t>* space, MSWNode *newElement);
 void link(MSWNode* first, MSWNode* second){
	 first->addFriend(second);
     second->addFriend(first);
 }
 template <typename QueryType> void GenSearch(QueryType* query);

private:
 int NN_;
 int initIndexAttempts_;
 int initSearchAttempts_;
 int size_;
 ElementList ElList;

protected:
 void incSize(){
	 size_ = size_ + 1;
 }
 int getSize(){
	 return size_;
 }

 DISABLE_COPY_AND_ASSIGN(Metrized_small_world);
};




}

#endif /* METRIZED_SMALL_WORLD_H_ */
