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

/*
*
* A Hierarchical Navigable Small World (HNSW) approach.
*
* The main publication is (available on arxiv: http://arxiv.org/abs/1603.09320):
* "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs" by Yu. A. Malkov, D. A. Yashunin
* This code was contributed by Yu. A. Malkov. It also was used in tests from the paper.
*
*
*/

#include <cmath>
#include <memory>
#include <iostream>
#include <mmintrin.h>

#include "space.h"
#include "knnquery.h"
#include "rangequery.h"
#include "ported_boost_progress.h"
#include "method/hnsw.h"
#include "space/space_lp.h"

#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <typeinfo>

#ifdef _OPENMP
#include <omp.h>
#endif

#define USE_BITSET_FOR_INDEXING 1
#define EXTEND_USE_EXTENDED_NEIGHB_AT_CONSTR (0) // 0 is faster build, 1 is faster search on clustered data

#if defined(__GNUC__)
#define PORTABLE_ALIGN16 __attribute__((aligned(16)))
#else
#define PORTABLE_ALIGN16 __declspec(align(16))
#endif


namespace similarity {

    using namespace std;
    /*Functions from hnsw_distfunc_opt.cc:*/
    float L2SqrSIMDExt(const float* pVect1, const float* pVect2, size_t &qty, float *TmpRes);
    float L2SqrSIMD16Ext(const float* pVect1, const float* pVect2, size_t &qty, float *TmpRes);
    float NormScalarProductSIMD(const float* pVect1, const float* pVect2, size_t &qty, float *TmpRes);

    template <typename dist_t>    Hnsw<dist_t>::Hnsw(bool PrintProgress, const Space<dist_t>& space,   const ObjectVector& data) :
        space_(space), data_(data), PrintProgress_(PrintProgress) {}

    template <typename dist_t> void Hnsw<dist_t>::CreateIndex(const AnyParams& IndexParams)
    {

        AnyParamManager pmgr(IndexParams);

        generator = new std::default_random_engine(100);

        pmgr.GetParamOptional("M", M_, 16);

#ifdef _OPENMP
        indexThreadQty_ = omp_get_max_threads();
#endif
        pmgr.GetParamOptional("indexThreadQty", indexThreadQty_, indexThreadQty_);
        pmgr.GetParamOptional("efConstruction", efConstruction_, 200);
        // Let's use a generic algorithm by default!
        pmgr.GetParamOptional("searchMethod", searchMethod_, 0);
        pmgr.GetParamOptional("maxM", maxM_, M_);
        pmgr.GetParamOptional("maxM0", maxM0_, M_ * 2);
        pmgr.GetParamOptional("mult", mult_, 1 / log(1.0*M_));
        pmgr.GetParamOptional("delaunay_type", delaunay_type_, 1);
        int skip_optimized_index = 0;
        pmgr.GetParamOptional("skip_optimized_index", skip_optimized_index, 0);
        
        LOG(LIB_INFO) << "M                  = " << M_;
        LOG(LIB_INFO) << "indexThreadQty      = " << indexThreadQty_;
        LOG(LIB_INFO) << "efConstruction      = " << efConstruction_;
        LOG(LIB_INFO) << "maxM			      = " << maxM_;
        LOG(LIB_INFO) << "maxM0			      = " << maxM0_;

		    SetQueryTimeParams(getEmptyParams());
        
        if (data_.empty()) return;

        // One entry should be added before all the threads are started, or else add() will not work properly
        HnswNode *first = new HnswNode(data_[0], 0 /* id == 0 */);        
        first->init(getRandomLevel(mult_), maxM_, maxM0_);        
        maxlevel_ = first->level;
        enterpoint_ = first;
        addToElementListSynchronized(first);

        visitedlistpool = new VisitedListPool(indexThreadQty_, data_.size());

        unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ? new ProgressDisplay(data_.size(), cerr) : NULL);

#pragma omp parallel for schedule(dynamic,128) num_threads(indexThreadQty_)
        for (int id = 1; id < data_.size(); ++id) {
            HnswNode* node = new HnswNode(data_[id], id);
            add(&space_, node); 

            if (progress_bar) ++(*progress_bar);

        }


        data_level0_memory_ = NULL;
        linkLists_ = NULL;
        if (skip_optimized_index)
            return;

        int friendsSectionSize = (maxM0_ + 1)*sizeof(int);
        //Checking for maximum size of the datasection:
        int dataSectionSize = 1;
        for (int i = 0; i < ElList_.size(); i++) {
            ElList_[i]->id_ = i;
            if (ElList_[i]->getData()->bufferlength()>dataSectionSize)
                dataSectionSize = ElList_[i]->getData()->bufferlength();
        }

        // Selecting custom made functions 
        if (space_.StrDesc().compare("SpaceLp: p = 2 do we have a special implementation for this p? : 1") == 0 && sizeof(dist_t) == 4)
        {
            LOG(LIB_INFO) << "\nThe vectorspace is Euclidian";
            vectorlength_ = ((dataSectionSize - 16) >> 2);
            LOG(LIB_INFO) << "Vector length=" << vectorlength_;
            if (vectorlength_ % 16 == 0) {
                LOG(LIB_INFO) << "Thus using an optimised function for base 16";
                fstdistfunc_ = L2SqrSIMD16Ext;
                dist_func_type_ = 1;
                searchMethod_ = 3;
            }
            else {
                LOG(LIB_INFO) << "Thus using function with any base";
                fstdistfunc_ = L2SqrSIMDExt;
                dist_func_type_ = 2;
                searchMethod_ = 3;
            }
        }
        else if (space_.StrDesc().compare("CosineSimilarity") == 0 && sizeof(dist_t) == 4)
        {
            LOG(LIB_INFO) << "\nThe vectorspace is Cosine Similarity";
            vectorlength_ = ((dataSectionSize - 16) >> 2);
            LOG(LIB_INFO) << "Vector length=" << vectorlength_;
            iscosine_ = true;
            if (vectorlength_ % 4 == 0) {
                LOG(LIB_INFO) << "Thus using an optimised function for base 4";
                fstdistfunc_ = NormScalarProductSIMD;
                dist_func_type_ = 3;
                searchMethod_ = 4;
            }
            else {
                LOG(LIB_INFO) << "Thus using function with any base";
                LOG(LIB_INFO) << "Search method 4 is not allowed in this case";
                fstdistfunc_ = NormScalarProductSIMD;
                dist_func_type_ = 3;
                searchMethod_ = 3;
            }
        }
        else {
            LOG(LIB_INFO) << "No appropriate custom distance function for " << space_.StrDesc();
            //if (searchMethod_ != 0 && searchMethod_ != 1)
                searchMethod_ = 0;
            return; // No optimized index
        }
        LOG(LIB_INFO) << "searchMethod       =" << searchMethod_;
        memoryPerObject_ = dataSectionSize + friendsSectionSize;

        int total_memory_allocated = (memoryPerObject_*ElList_.size());
        data_level0_memory_ = (char*)malloc(memoryPerObject_*ElList_.size());        

        offsetLevel0_ = dataSectionSize;
        offsetData_ = 0;
       

        memset(data_level0_memory_, 1, memoryPerObject_*ElList_.size());
        LOG(LIB_INFO) << "Making optimized index";
        for (long i = 0; i < ElList_.size(); i++) {            
            ElList_[i]->copyDataAndLevel0LinksToOptIndex(data_level0_memory_ + (size_t)i*memoryPerObject_, offsetLevel0_, offsetData_);
        };
        ////////////////////////////////////////////////////////////////////////
        //
        // The next step is needed only fos cosine similarity space
        // All vectors are normalized, so we don't have to normalize them later
        //
        ////////////////////////////////////////////////////////////////////////
        if (iscosine_)
        {
            for (long i = 0; i < ElList_.size(); i++) {
                float *v = (float *)(data_level0_memory_ + (size_t)i*memoryPerObject_ + offsetData_ + 16);
                float sum = 0;
                for (int i = 0; i < vectorlength_; i++) {
                    sum += v[i] * v[i];
                }                
                if (sum != 0.0) {
                    sum = 1 / sqrt(sum);
                    for (int i = 0; i < vectorlength_; i++) {
                        v[i] *= sum;
                    }
                }
            };
        }      
        
        /////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////
        linkLists_ = (char**)malloc(sizeof(void*)*ElList_.size());
        for (long i = 0; i < ElList_.size(); i++) {
            if (ElList_[i]->level < 1) {
                linkLists_[i] = nullptr;
                continue;
            }
            int sizemass = ((ElList_[i]->level)*(maxM_ + 1))*sizeof(int);
            total_memory_allocated += sizemass;
            char *linkList = (char*)malloc(sizemass);
            linkLists_[i] = linkList;
            ElList_[i]->copyHigherLevelLinksToOptIndex(linkList, 0);
        };
        enterpointId_ = enterpoint_->getId();
        LOG(LIB_INFO) << "Finished making optimized index";
        LOG(LIB_INFO) << "Maximum level = " << enterpoint_->level;
        LOG(LIB_INFO) << "Total memory allocated for optimized index+data: " << (total_memory_allocated >> 20) << " Mb";

    }

    template <typename dist_t>
    void Hnsw<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
        AnyParamManager pmgr(QueryTimeParams);

        if (pmgr.hasParam("ef") && pmgr.hasParam("efSearch")) {
          throw new runtime_error("The user shouldn't specify parameters ef and efSearch at the same time (they are synonyms)");
        }

        // ef and efSearch are going to be parameter-synonyms with the default value 20
        pmgr.GetParamOptional("ef", ef_, 20);
        pmgr.GetParamOptional("efSearch", ef_, ef_);

        pmgr.CheckUnused();
        LOG(LIB_INFO) << "Set HNSW query-time parameters:";
        LOG(LIB_INFO) << "ef(Search)         =" << ef_;
    }

    template <typename dist_t>
    const std::string Hnsw<dist_t>::StrDesc() const {
        return METH_HNSW;
    }

    template <typename dist_t>
    Hnsw<dist_t>::~Hnsw() {
        
        delete visitedlistpool;
        if (data_level0_memory_)
            free(data_level0_memory_);
        if (linkLists_) {
            for (int i = 0; i < ElList_.size(); i++) {
                if (linkLists_[i])
                    free(linkLists_[i]);
            }
            free(linkLists_);
        }
        for (int i = 0; i < ElList_.size(); i++)
            delete ElList_[i];
    }
    
    template <typename dist_t>
     void Hnsw<dist_t>::add(const Space<dist_t>* space, HnswNode *NewElement) {
        int curlevel = getRandomLevel(mult_);
        unique_lock<mutex> *lock = nullptr;
        if (curlevel > maxlevel_)
            lock = new unique_lock<mutex>(MaxLevelGuard_);

        NewElement->init(curlevel, maxM_, maxM0_);       

        


        int maxlevelcopy = maxlevel_;
        HnswNode *ep = enterpoint_;
        if (curlevel < maxlevelcopy) {
            const Object* currObj = ep->getData();

            dist_t d = space->IndexTimeDistance(NewElement->getData(), currObj);
            dist_t curdist = d;
            HnswNode *curNode = ep;
            for (int level = maxlevelcopy; level > curlevel; level--) {
                bool changed = true;
                while (changed) {
                    changed = false;
                    unique_lock<mutex>  lock(curNode->accessGuard_);


                    const vector<HnswNode*>& neighbor = curNode->getAllFriends(level);
                    int size = neighbor.size();
                    for (int i = 0; i < size; i++) {
                        HnswNode *node = neighbor[i];
                        _mm_prefetch((char *)(node)->getData(), _MM_HINT_T0);
                    }
                    for (int i = 0; i < size; i++)
                    {
                        currObj = (neighbor[i])->getData();
                        d = space->IndexTimeDistance(NewElement->getData(), currObj);
                        if (d < curdist) {
                            curdist = d;
                            curNode = neighbor[i];
                            changed = true;
                        }
                    }

                }

            }
            ep = curNode;
        }

        
        for (int level = min(curlevel, maxlevelcopy); level >= 0; level--) {
			priority_queue<HnswNodeDistCloser<dist_t>> resultSet;
            kSearchElementsWithAttemptsLevel(space, NewElement->getData(), efConstruction_, resultSet, ep, level);//DOTO: make level
           
            switch (delaunay_type_) {
            case 0:
                while (resultSet.size() > M_)
                    resultSet.pop();
                break;
            case 1:
                NewElement->getNeighborsByDistanceHeuristic(resultSet, M_, space);
                break;
            case 2:
                NewElement->getNeighborsByMiniGreedySearches(resultSet, M_, space, level);
                break;
            }
            while (!resultSet.empty()) {
                link(resultSet.top().getMSWNodeHier(), NewElement, level, space, delaunay_type_);
                resultSet.pop();
            }

        }
        if (curlevel > enterpoint_->level)
        {
            enterpoint_ = NewElement;
            maxlevel_ = curlevel;
        }
        if (lock != nullptr)
            delete lock;
        addToElementListSynchronized(NewElement);
    }




    template <typename dist_t>
    void
        Hnsw<dist_t>::kSearchElementsWithAttemptsLevel(const Space<dist_t>* space,
            const Object* queryObj, size_t efConstruction, priority_queue<HnswNodeDistCloser<dist_t>>& resultSet, HnswNode* ep, int level) const
    {

        
#if EXTEND_USE_EXTENDED_NEIGHB_AT_CONSTR!=0
        priority_queue<HnswNodeDistCloser<dist_t>> fullResultSet;
#endif
        
#if USE_BITSET_FOR_INDEXING
        VisitedList * vl = visitedlistpool->getFreeVisitedList();
        unsigned int *mass = vl->mass;
        unsigned int curV = vl->curV;
#else
        unordered_set<HnswNode*>             visited;
#endif
        HnswNode* provider = ep;
        priority_queue <HnswNodeDistFarther<dist_t>>   candidateSet;
        dist_t d = space->IndexTimeDistance(queryObj, provider->getData());
        HnswNodeDistFarther<dist_t> ev(d, provider);

        candidateSet.push(ev);
        resultSet.emplace(d, provider);
        
#if EXTEND_USE_EXTENDED_NEIGHB_AT_CONSTR!=0
        fullResultSet.emplace(d, provider);
#endif
        
#if USE_BITSET_FOR_INDEXING
        size_t nodeId = provider->getId();
        mass[nodeId] = curV;
#else
        visited.insert(provider);
#endif


        while (!candidateSet.empty()) {
            const HnswNodeDistFarther<dist_t>& currEv = candidateSet.top();
            dist_t lowerBound = resultSet.top().getDistance();

            /*
            * Check if we reached a local minimum.
            */
            if (currEv.getDistance() > lowerBound) {
                break;
            }
            HnswNode* currNode = currEv.getMSWNodeHier();

            /*
            * This lock protects currNode from being modified
            * while we are accessing elements of currNode.
            */
            unique_lock<mutex>  lock(currNode->accessGuard_);
            const vector<HnswNode*>& neighbor = currNode->getAllFriends(level);

            // Can't access curEv anymore! The reference would become invalid
            candidateSet.pop();

            // calculate distance to each neighbor
            for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {
                _mm_prefetch((char *)(*iter)->getData(), _MM_HINT_T0);
            }

            for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {
#if USE_BITSET_FOR_INDEXING
                size_t nodeId = (*iter)->getId();
                if (mass[nodeId] != curV) {
                    mass[nodeId] = curV;
#else
                if (visited.find((*iter)) == visited.end()) {
                    visited.insert(*iter);
#endif
                    d = space->IndexTimeDistance(queryObj, (*iter)->getData());
                    HnswNodeDistFarther<dist_t> evE1(d, *iter);
        
#if EXTEND_USE_EXTENDED_NEIGHB_AT_CONSTR!=0
                    fullResultSet.emplace(d, *iter);
#endif
        
                    if (resultSet.size() < efConstruction || resultSet.top().getDistance() > d) {
                        resultSet.emplace(d, *iter);
                        candidateSet.push(evE1);
                        if (resultSet.size() > efConstruction) {
                            resultSet.pop();
                        }
                    }
                }
            }
        }
        
#if EXTEND_USE_EXTENDED_NEIGHB_AT_CONSTR!=0
        resultSet.swap(fullResultSet);
#endif
        
#if USE_BITSET_FOR_INDEXING
        visitedlistpool->releaseVisitedList(vl);
#endif
    }

    template <typename dist_t>
    void Hnsw<dist_t>::addToElementListSynchronized(HnswNode *HierElement) {
        unique_lock<mutex> lock(ElListGuard_);
        ElList_.push_back(HierElement);
    }
    template <typename dist_t>
    void Hnsw<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
        throw runtime_error("Range search is not supported!");
    }


    template <typename dist_t>
    void Hnsw<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const  {
        switch (searchMethod_) {
            		case 0:
                    default:
                        /// Basic search using Nmslib data structure:
                        const_cast<Hnsw*>(this)->baseSearchAlgorithm(query);
            			break;
            		case 1:
                        /// Experimental search using Nmslib data structure (should not be used):
                        const_cast<Hnsw*>(this)->listPassingModifiedAlgorithm(query);
            			break;
            		case 3:
                        /// Basic search using optimized index(cosine+L2)
                        const_cast<Hnsw*>(this)->SearchL2Custom(query);
            			break;
            		case 4:
                        /// Basic search using optimized index with one-time normalized cosine similarity
                        /// Only for cosine similarity!
                        const_cast<Hnsw*>(this)->SearchCosineNormalized(query);
            			break;
            		};
    }

    template <typename dist_t>
    void Hnsw<dist_t>::SaveIndex(const string &location) {
        if (!data_level0_memory_)
            throw runtime_error("Storing non-optimized index is not supported yet!");

        std::ofstream output(location, std::ios::binary);
        streampos position;
        totalElementsStored_ = ElList_.size();        
        output.write((char*)&totalElementsStored_, sizeof(size_t));
        output.write((char*)&memoryPerObject_, sizeof(size_t));
        output.write((char*)&offsetLevel0_, sizeof(size_t));
        output.write((char*)&offsetData_, sizeof(size_t));
        output.write((char*)&maxlevel_, sizeof(size_t));
        output.write((char*)&enterpointId_, sizeof(size_t));
        output.write((char*)&maxM_, sizeof(size_t));
        output.write((char*)&maxM0_, sizeof(size_t));
        output.write((char*)&dist_func_type_, sizeof(size_t));       

            
            
        size_t data_plus_links0_size = memoryPerObject_*totalElementsStored_;
        LOG(LIB_INFO) << "writing " << data_plus_links0_size << " bytes";
        output.write(data_level0_memory_, data_plus_links0_size);
        
        //output.write(data_level0_memory_, memoryPerObject_*totalElementsStored_);
        

        size_t total_memory_allocated = 0;
        for (size_t i = 0; i < totalElementsStored_; i++) {            
            unsigned int sizemass = ((ElList_[i]->level)*(maxM_ + 1))*sizeof(int);       
            output.write((char*)&sizemass, sizeof(unsigned int));
            if((sizemass))
                output.write(linkLists_[i], sizemass);
        };        
        output.close();


    }

    template <typename dist_t>
    void Hnsw<dist_t>::LoadIndex(const string &location) {
        LOG(LIB_INFO) << "Loading index from "<<location;
        std::ifstream input(location, std::ios::binary);
        streampos position;
        
        //input.seekg(0, std::ios::beg);

        input.read((char*)(&totalElementsStored_), sizeof(size_t)); 
        input.read((char*)(&memoryPerObject_), sizeof(size_t));
        input.read((char*)&offsetLevel0_, sizeof(size_t));
        input.read((char*)&offsetData_, sizeof(size_t));
        input.read((char*)&maxlevel_, sizeof(size_t));
        input.read((char*)&enterpointId_, sizeof(size_t));
        input.read((char*)&maxM_, sizeof(size_t));
        input.read((char*)&maxM0_, sizeof(size_t));
        input.read((char*)&dist_func_type_, sizeof(size_t));
        
        if (dist_func_type_ == 1)
             fstdistfunc_ = L2SqrSIMD16Ext;
        else if (dist_func_type_ == 2)
            fstdistfunc_ = L2SqrSIMDExt;
        else if (dist_func_type_ == 3)
            fstdistfunc_ = NormScalarProductSIMD;


  
//        LOG(LIB_INFO) << input.tellg();
        LOG(LIB_INFO) << "Total: " << totalElementsStored_<< ", Memory per object: " << memoryPerObject_;
        size_t data_plus_links0_size = memoryPerObject_*totalElementsStored_;
        data_level0_memory_ = (char*)malloc(data_plus_links0_size);        
        input.read(data_level0_memory_, data_plus_links0_size);            
        linkLists_ = (char**)malloc(sizeof(void*)*totalElementsStored_);

        for (size_t i = 0; i < totalElementsStored_; i++) {
            unsigned int linkListSize;
            input.read((char*)&linkListSize, sizeof(unsigned int));
            position = input.tellg();
            if (linkListSize == 0) {
                linkLists_[i] = nullptr;
            }
            else {
                linkLists_[i] = (char *) malloc(linkListSize);
                input.read(linkLists_[i], linkListSize);
            }

        }
        LOG(LIB_INFO) << "Finished loading index";
        visitedlistpool = new VisitedListPool(1, totalElementsStored_);
      
        input.close();

    }


    	template <typename dist_t>
    	void Hnsw<dist_t>::baseSearchAlgorithm(KNNQuery<dist_t>* query) {
    		VisitedList * vl = visitedlistpool->getFreeVisitedList();
    		unsigned int *massVisited = vl->mass;
    		unsigned int currentV = vl->curV;

    		HnswNode* provider;
    		int maxlevel1 = enterpoint_->level;
    		provider = enterpoint_;
    
    		const Object* currObj = provider->getData();
    
    		dist_t d = query->DistanceObjLeft(currObj);
    		dist_t curdist = d;
    		HnswNode *curNode = provider;
    		for (int i = maxlevel1; i > 0; i--) {
    			bool changed = true;
    			while (changed) {
    				changed = false;
    
    				const vector<HnswNode*>& neighbor = curNode->getAllFriends(i);
    				for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {
    					_mm_prefetch((char *)(*iter)->getData(), _MM_HINT_T0);
    				}
    				for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {
    					currObj = (*iter)->getData();
    					d = query->DistanceObjLeft(currObj);
    					if (d < curdist) {
    						curdist = d;
    						curNode = *iter;
    						changed = true;
    					}
    				}
    			}
    		}
    	
    
    		priority_queue <HnswNodeDistFarther<dist_t>> candidateQueue; //the set of elements which we can use to evaluate    																			 
    		priority_queue <HnswNodeDistCloser<dist_t>> closestDistQueue1; //The set of closest found elements 
    
    		HnswNodeDistFarther<dist_t> ev(curdist, curNode);
    		candidateQueue.emplace(curdist, curNode);
    		closestDistQueue1.emplace(curdist, curNode);
    
    		query->CheckAndAddToResult(curdist, curNode->getData());
    		massVisited[curNode->getId()] = currentV;
    		//visitedQueue.insert(curNode->getId());
    
    		////////////////////////////////////////////////////////////////////////////////
    		// PHASE TWO OF THE SEARCH
    		// Extraction of the neighborhood to find k nearest neighbors.
    		////////////////////////////////////////////////////////////////////////////////
    		
    		while (!candidateQueue.empty()) {
    			
    			auto iter = candidateQueue.top(); // This one was already compared to the query
    			const HnswNodeDistFarther<dist_t>& currEv = iter;
    			//Check condtion to end the search
    			dist_t lowerBound = closestDistQueue1.top().getDistance();
    			if (currEv.getDistance() > lowerBound) {
    				break;
    			}
    
    			HnswNode *initNode = currEv.getMSWNodeHier();
    			candidateQueue.pop();
    
    			const vector<HnswNode*>& neighbor = (initNode)->getAllFriends(0);
    
    			size_t curId;
    
    			for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {
    				_mm_prefetch((char *)(*iter)->getData(), _MM_HINT_T0);
    				_mm_prefetch((char *)(massVisited + (*iter)->getId()), _MM_HINT_T0);
    			}
    			//calculate distance to each neighbor
    			for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {
    
    				curId = (*iter)->getId();
    				
    				if (!(massVisited[curId] == currentV))
    				{
    					massVisited[curId] = currentV;
    					currObj = (*iter)->getData();
    					d = query->DistanceObjLeft(currObj);
    					if (closestDistQueue1.top().getDistance() > d || closestDistQueue1.size() < ef_) {
    						{
    							query->CheckAndAddToResult(d, currObj);    
    							candidateQueue.emplace(d, *iter);
    							closestDistQueue1.emplace(d, *iter);
    							if (closestDistQueue1.size() > ef_) {
    								closestDistQueue1.pop();
    							}
    						}
    					}
    				}
    			}
    
    		}
    		visitedlistpool->releaseVisitedList(vl);
    
    	}
        // Experimental search algorithm
    	template <typename dist_t>
    	void Hnsw<dist_t>::listPassingModifiedAlgorithm(KNNQuery<dist_t>* query) {
    		int efSearchL = 4; // This parameters defines the confidence of searches at level higher than zero 
                               // for zero level it is set to ef
            //Getting the visitedlist
    		VisitedList * vl = visitedlistpool->getFreeVisitedList();
    		unsigned int *massVisited = vl->mass;
    		unsigned int currentV = vl->curV;
    
    		int maxlevel1 = enterpoint_->level;    
    
    		const Object* currObj = enterpoint_->getData();
    
    		dist_t d = query->DistanceObjLeft(currObj);
    		dist_t curdist = d;
    		HnswNode *curNode = enterpoint_;
    
    
    		priority_queue <HnswNodeDistFarther<dist_t>> candidateQueue; //the set of elements which we can use to evaluate    														
    		priority_queue <HnswNodeDistCloser<dist_t>> closestDistQueue= priority_queue <HnswNodeDistCloser<dist_t>>(); //The set of closest found elements 
    		priority_queue <HnswNodeDistCloser<dist_t>> closestDistQueueCpy = priority_queue <HnswNodeDistCloser<dist_t>>();

    		HnswNodeDistFarther<dist_t> ev(curdist, curNode);
    		candidateQueue.emplace(curdist, curNode);
    		closestDistQueue.emplace(curdist, curNode);

    		massVisited[curNode->getId()] = currentV;

    		for (int i = maxlevel1; i > 0; i--) {
    			
    			while (!candidateQueue.empty()) {
    	
    				auto iter = candidateQueue.top(); 
    				const HnswNodeDistFarther<dist_t>& currEv = iter;
    				//Check condtion to end the search
    				dist_t lowerBound = closestDistQueue.top().getDistance();
    				if (currEv.getDistance() > lowerBound) {
    					break;
    				}
    
    				HnswNode *initNode = currEv.getMSWNodeHier();
    				candidateQueue.pop();
    
    				const vector<HnswNode*>& neighbor = (initNode)->getAllFriends(i);
    
    				size_t curId;
    
    				for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {
    					_mm_prefetch((char *)(*iter)->getData(), _MM_HINT_T0);
    					_mm_prefetch((char *)(massVisited + (*iter)->getId()), _MM_HINT_T0);
    				}
    				//calculate distance to each neighbor
    				for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {    
    					curId = (*iter)->getId();
    					if (!(massVisited[curId] == currentV))
    					{
    						massVisited[curId] = currentV;
    						currObj = (*iter)->getData();
    						d = query->DistanceObjLeft(currObj);
                            if (closestDistQueue.top().getDistance() > d || closestDistQueue.size() < efSearchL) {
                                candidateQueue.emplace(d, *iter);
                                closestDistQueue.emplace(d, *iter);
                                if (closestDistQueue.size() > efSearchL) {
                                    closestDistQueue.pop();
                                }
                            }
    					}
    				}
    
    			}
                //Updating the bitset key:
                currentV++;
                vl->curV++;// not to forget updating in the pool
    			if (currentV == 0) {
    				memset(massVisited, 0, ElList_.size()*sizeof(int));
    				currentV++;
                    vl->curV++;// not to forget updating in the pool
    			}
                candidateQueue = priority_queue <HnswNodeDistFarther<dist_t>>();
                closestDistQueueCpy = priority_queue <HnswNodeDistCloser<dist_t>>(closestDistQueue);
    			if (i > 1) {    // Passing the closest neighbors to layers higher than zero:
                    while (closestDistQueueCpy.size() > 0) {
                        massVisited[closestDistQueueCpy.top().getMSWNodeHier()->getId()] = currentV;
                        candidateQueue.emplace(closestDistQueueCpy.top().getDistance(), closestDistQueueCpy.top().getMSWNodeHier());                        
                        closestDistQueueCpy.pop();
                    }    				
    			}
                else {     // Passing the closest neighbors to the 0 zero layer(one has to add also to query):                    
                    while (closestDistQueueCpy.size() > 0) {
                        massVisited[closestDistQueueCpy.top().getMSWNodeHier()->getId()] = currentV;
                        candidateQueue.emplace(closestDistQueueCpy.top().getDistance(), closestDistQueueCpy.top().getMSWNodeHier());
                        query->CheckAndAddToResult(closestDistQueueCpy.top().getDistance(), closestDistQueueCpy.top().getMSWNodeHier()->getData());
                        closestDistQueueCpy.pop();
                    }
                }
    		}
    		
    		////////////////////////////////////////////////////////////////////////////////
    		// PHASE TWO OF THE SEARCH
    		// Extraction of the neighborhood to find k nearest neighbors.
    		////////////////////////////////////////////////////////////////////////////////
    
    
    		while (!candidateQueue.empty()) {
    
    			auto iter = candidateQueue.top();
    			const HnswNodeDistFarther<dist_t>& currEv = iter;
    			//Check condtion to end the search
    			dist_t lowerBound = closestDistQueue.top().getDistance();
    			if (currEv.getDistance() > lowerBound) {
    				break;
    			}
    
    			HnswNode *initNode = currEv.getMSWNodeHier();
    			candidateQueue.pop();
    
    			const vector<HnswNode*>& neighbor = (initNode)->getAllFriends(0);
    
    			size_t curId;
    
    			for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {
    				_mm_prefetch((char *)(*iter)->getData(), _MM_HINT_T0);
    				_mm_prefetch((char *)(massVisited + (*iter)->getId()), _MM_HINT_T0);
    			}
    			//calculate distance to each neighbor
    			for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {
    
    				curId = (*iter)->getId();
    				if (!(massVisited[curId] == currentV))
    				{
    					massVisited[curId] = currentV;
    					currObj = (*iter)->getData();
    					d = query->DistanceObjLeft(currObj);
    					if (closestDistQueue.top().getDistance() > d || closestDistQueue.size() < ef_) {
    						{
    							query->CheckAndAddToResult(d, currObj);
    							candidateQueue.emplace(d, *iter);
    							closestDistQueue.emplace(d, *iter);
    							if (closestDistQueue.size() > ef_) {
    								closestDistQueue.pop();
    							}
    						}
    					}
    				}
    			}
    
    		}
    		visitedlistpool->releaseVisitedList(vl);
    
    	}


    template class Hnsw<float>;
    template class Hnsw<double>;
    template class Hnsw<int>;

}
