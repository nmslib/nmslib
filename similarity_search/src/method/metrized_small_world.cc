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
#include "metrized_small_world.h"
#include <vector>
#include <set>
#include <map>
#include <typeinfo>
#include <unordered_set>

namespace similarity {

template <typename dist_t>
Metrized_small_world<dist_t>::Metrized_small_world(const Space<dist_t>* space,
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
const std::string Metrized_small_world<dist_t>::ToString() const {
  return "metrized_small_world";
}

template <typename dist_t>
Metrized_small_world<dist_t>::~Metrized_small_world() {}

template <typename dist_t>
MSWNode* Metrized_small_world<dist_t>::getRandomEnterPoint()
{
	int size = ElList.size();
	if(!ElList.size())
	{
		return NULL;
	}
	else
	{
		int num = rand()%size;
		return ElList[num];
	}
}

template <typename dist_t> dist_t Metrized_small_world<dist_t>::getKDistance(const set<EvaluatedMSWNode<dist_t>>& ElementSet, size_t k)
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
SearchResult<dist_t>
Metrized_small_world<dist_t>::kSearchElementsWithAttempts(const Space<dist_t>* space, MSWNode* query, int NN, int initIndexAttempts)
{
	int steps = 0;
	set <EvaluatedMSWNode<dist_t>> globalViewedSet;

	for (int i=0; i < initIndexAttempts; i++){

		/**
		 * Search of most k-closest elements to the query.
		 */
		MSWNode* provider = getRandomEnterPoint();

		unordered_set<MSWNode*> visitedSet; //The set of elements which has used to extend our view represented by viewedSet
		set<EvaluatedMSWNode<dist_t>> viewedSet; //The set of all elements which distance was calculated
		map <MSWNode*, dist_t> viewedMap;
		set<EvaluatedMSWNode<dist_t>> candidateSet; //the set of elements which we can use to evaluate
		EvaluatedMSWNode<dist_t> evE1;

		dist_t d = space->IndexTimeDistance(query->getData(), provider->getData());
		EvaluatedMSWNode<dist_t> ev(d, provider);

		candidateSet.insert(ev);
		viewedSet.insert(ev);
		viewedMap.insert(std::pair<MSWNode*, dist_t>(ev.getMSWNode(), ev.getDistance()));

		while(!candidateSet.empty()){
			auto iter = candidateSet.begin();
			EvaluatedMSWNode<dist_t> currEv = *iter;
			candidateSet.erase(iter);
			dist_t lowerBound = getKDistance(viewedSet, NN);
			//Check condition for lower bound
			if (currEv.getDistance() > lowerBound) {
				break;
			}

			visitedSet.insert(currEv.getMSWNode());
			const set<MSWNode*>& neighbor = (currEv.getMSWNode())->getAllFriends();

			//synchronized
			//calculate distance for each element from neighbor
			for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter){
				if (viewedMap.find(*iter) == viewedMap.end()){
					d = space->IndexTimeDistance(query->getData(), (*iter)->getData());
					EvaluatedMSWNode<dist_t> evE1(d, *iter);
					viewedMap.insert(std::pair<MSWNode*, dist_t>((*iter), evE1.getDistance()));
					viewedSet.insert(evE1);
					candidateSet.insert(evE1);
				}
			}

		}

		globalViewedSet.insert(viewedSet.begin(), viewedSet.end());
		steps = steps + visitedSet.size();
	}

  SearchResult<dist_t> Result(globalViewedSet, steps);
  return Result;
}

template <typename dist_t>
void Metrized_small_world<dist_t>::add(const Space<dist_t>* space, MSWNode *newElement){

	int linkCount = 0;
	MSWNode* enterPoint = getRandomEnterPoint();
	newElement->removeAllFriends(); 		//отсебятина, в исходном коде не было.

	if(enterPoint == NULL){
		ElList.push_back(newElement);
		incSize();
		return;
	}

	SearchResult<dist_t> sr = kSearchElementsWithAttempts(space, newElement, NN_, initIndexAttempts_);

	int i = 0;
	const set <EvaluatedMSWNode<dist_t>>& viewed = sr.getViewedList();
	for (auto ee = viewed.begin(); ee != viewed.end(); ++ee) {
		if (i >= NN_) break;
		i ++;
		MSWNode* el = (*ee).getMSWNode();
		const set<MSWNode*>& vec = newElement->getAllFriends();
		if (vec.find(el) == vec.end())
		{
			link(((*ee).getMSWNode()), newElement);
			linkCount++;
		}
	}

	ElList.push_back(newElement);
	incSize();
}

template <typename dist_t>
void Metrized_small_world<dist_t>::Search(RangeQuery<dist_t>* query) {
  //GenSearch(query);

}

template <typename dist_t>
void Metrized_small_world<dist_t>::Search(KNNQuery<dist_t>* query) {
    GenSearch(query);

}

template <typename dist_t>
template <typename QueryType>
void Metrized_small_world<dist_t>::GenSearch(QueryType* query){
/*
	MSWNode *Node;
	dist_t d1 =0 ,d2 = 100;
	int i =0;

	for (auto iter = ElList.begin(); iter != ElList.end(); iter++)
	{
		d1 = query->DistanceObjLeft((*iter)->getData());

		if (d1 < d2 && d1 > 0)
		{
			d2 = d1;
			Node = *iter;
		}

	}
*/

//	query->CheckAndAddToResult(Node->getData());

	int steps = 0;
	int k = query->GetK();
	set <EvaluatedMSWNode<dist_t>> globalViewedSet;

	for (int i=0; i < initSearchAttempts_; i++){

	/**
	 * Search of most k-closest elements to the query.
	 */
	    MSWNode* provider = getRandomEnterPoint();

			unordered_set<MSWNode*> visitedSet; //The set of elements which has used to extend our view represented by viewedSet
			set<EvaluatedMSWNode<dist_t>> viewedSet; //The set of all elements which distance was calculated
			map <MSWNode*, dist_t> viewedMap;
			set<EvaluatedMSWNode<dist_t>> candidateSet; //the set of elements which we can use to evaluate
			EvaluatedMSWNode<dist_t> evE1;

			dist_t d = query->DistanceObjLeft(provider->getData());
			EvaluatedMSWNode<dist_t> ev(d, provider);

			candidateSet.insert(ev);
			viewedSet.insert(ev);
			viewedMap.insert(std::pair<MSWNode*, dist_t>(ev.getMSWNode(), ev.getDistance()));

			while(!candidateSet.empty()){
				auto iter = candidateSet.begin();
				EvaluatedMSWNode<dist_t> currEv = *iter;
				candidateSet.erase(iter);
				dist_t lowerBound = getKDistance(viewedSet, k);
				//Check condition for lower bound
				if (currEv.getDistance() > lowerBound) {
					break;
				}

				visitedSet.insert(currEv.getMSWNode());
				const set<MSWNode*>& neighbor = (currEv.getMSWNode())->getAllFriends();

				//synchronized
				//calculate distance for each element from neighbor
				for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter){
					if (viewedMap.find(*iter) == viewedMap.end()){
						d = query->DistanceObjLeft((*iter)->getData());
						EvaluatedMSWNode<dist_t> evE1(d, *iter);
						viewedMap.insert(std::pair<MSWNode*, dist_t>((*iter), evE1.getDistance()));
						viewedSet.insert(evE1);
						candidateSet.insert(evE1);
					}
				}

			}

			globalViewedSet.insert(viewedSet.begin(), viewedSet.end());
			steps = steps + visitedSet.size();
		}

		auto iter = globalViewedSet.begin();
		//MSWNode* v =  (*iter).getMSWNode();
		//dist_t d = query->DistanceObjLeft(((*iter).getMSWNode())->getData());

		while(k && iter != globalViewedSet.end()){
			query->CheckAndAddToResult(iter->getDistance(), iter->getMSWNode()->getData());
			iter++;
			k--;
		}
	}



template class Metrized_small_world<float>;
template class Metrized_small_world<double>;
template class Metrized_small_world<int>;

}
