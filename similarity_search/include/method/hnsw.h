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
#pragma once

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
#include <thread>
#include <condition_variable>
#include <queue>


#define METH_HNSW      "hnsw"
#define METH_HNSW_SYN      "Hierarchical_NSW"

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
    class VisitedListPool;
    template <typename dist_t> class HnswNodeDistCloser;
    template <typename dist_t> class HnswNodeDistFarther;

    class HnswNode {
    public:
        HnswNode(const Object *Obj, size_t id) {
            data_ = Obj;
            id_ = id;
        }
        ~HnswNode() {};

        template <typename dist_t>
        void getNeighborsByDistanceHeuristic(priority_queue<HnswNodeDistCloser<dist_t>> &resultSet1, const int NN, const Space<dist_t>* space) {
            if (resultSet1.size() < NN) {
                return;
            }
            priority_queue<HnswNodeDistFarther<dist_t>> resultSet;
            priority_queue<HnswNodeDistFarther<dist_t>> templist;
            vector<HnswNodeDistFarther<dist_t>> returnlist;
            while (resultSet1.size() > 0)
            {
                resultSet.emplace(resultSet1.top().getDistance(), resultSet1.top().getMSWNodeHier());
                resultSet1.pop();
            }
            while (resultSet.size()) {

                if (returnlist.size() >= NN)
                    break;
                HnswNodeDistFarther<dist_t> curen = resultSet.top();
                dist_t dist_to_query = curen.getDistance();
                resultSet.pop();
                bool good = true;
                for (HnswNodeDistFarther<dist_t> curen2 : returnlist) {
                    dist_t curdist = space->IndexTimeDistance(curen2.getMSWNodeHier()->getData(), curen.getMSWNodeHier()->getData());

                    if (curdist <= dist_to_query) {
                        good = false;
                        break;
                    }
                }
                if (good)
                    returnlist.push_back(curen);
                else
                    templist.push(curen);

            }

            while (returnlist.size() < NN && templist.size() > 0) {
                returnlist.push_back(templist.top());
                templist.pop();
            }

            for (HnswNodeDistFarther<dist_t> curen2 : returnlist) {
                resultSet1.emplace(curen2.getDistance(), curen2.getMSWNodeHier());
            }

        };
        /*
        * Experimental method for selection of neighbors based on local search. However is peforms worse than the previous method
        * Note that it works correctly only in a single thread!!!
        */
        template <typename dist_t>
        void getNeighborsByMiniGreedySearches(priority_queue<HnswNodeDistCloser<dist_t>> &resultSet1, const int NN, const Space<dist_t>* space, int level) {
            if (resultSet1.size() < NN) {
                return;
            }
            priority_queue<HnswNodeDistFarther<dist_t>> inputSet;
            priority_queue<HnswNodeDistFarther<dist_t>> templist;

            set <HnswNode*> result;

            while (resultSet1.size() > 0)
            {
                inputSet.emplace(resultSet1.top().getDistance(), resultSet1.top().getMSWNodeHier());
                resultSet1.pop();
            }
            result.insert(inputSet.top().getMSWNodeHier());
            inputSet.pop();
            while (inputSet.size()) {
                if (result.size() >= NN)
                    break;
                bool good = true;
                HnswNodeDistFarther<dist_t> curen = inputSet.top();
                inputSet.pop();
                dist_t curdist = curen.getDistance();
                HnswNode *curNode = curen.getMSWNodeHier();

                bool changed = true;
                while (changed) {
                    changed = false;

                    const vector<HnswNode*>& neighbor = curNode->getAllFriends(level);
                    for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {
                        _mm_prefetch((char *)(*iter)->getData(), _MM_HINT_T0);
                    }
                    for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter) {
                        const Object *currObj = (*iter)->getData();
                        dist_t d = space->IndexTimeDistance(this->data_, currObj);
                        if (d < curdist) {
                            curdist = d;
                            curNode = *iter;
                            changed = true;
                        }
                    }
                    if (result.count(curNode)) {
                        good = false;
                        break;
                    }
                }
                if (good)
                    result.insert(curen.getMSWNodeHier());
                else
                    templist.push(curen);

            }

            while (result.size() < NN && templist.size() > 0) {
                result.insert(templist.top().getMSWNodeHier());
                templist.pop();
            }
            resultSet1.empty();
            for (HnswNode *curNode : result) {
                resultSet1.emplace(0, curNode);
            }

        };
        template <typename dist_t>
        void addFriendlevel(int level, HnswNode* element, const Space<dist_t>* space, int delaunay_type) {

            unique_lock<mutex> lock(accessGuard_);
            for (int i = 0; i < allFriends[level].size(); i++)
                if (allFriends[level][i] == element) {
                    cerr << "This should not happen. For some reason the elements is already added";
                    return;
                }
            allFriends[level].push_back(element);
            bool shrink = false;
            if (level > 0)
                if (allFriends[level].size() > maxsize)
                    shrink = true;
                else
                    shrink = false;
            else
                if (allFriends[level].size() > maxsize0)
                    shrink = true;
                else
                    shrink = false;
            if (shrink)
            {
                if (delaunay_type > 0) {
                    priority_queue<HnswNodeDistCloser<dist_t>> resultSet;
                    for (int i = 1; i < allFriends[level].size(); i++) {
                        resultSet.emplace(space->IndexTimeDistance(this->getData(), allFriends[level][i]->getData()), allFriends[level][i]);
                    }
                    if (delaunay_type == 1)
                        this->getNeighborsByDistanceHeuristic(resultSet, resultSet.size() - 1, space);
                    else if (delaunay_type == 2)
                        this->getNeighborsByMiniGreedySearches(resultSet, resultSet.size() - 1, space, level);
                    allFriends[level].clear();

                    while (resultSet.size()) {
                        allFriends[level].push_back(resultSet.top().getMSWNodeHier());
                        resultSet.pop();

                    }
                }
                else {
                    dist_t max = space->IndexTimeDistance(this->getData(), allFriends[level][0]->getData());
                    int maxi = 0;
                    for (int i = 1; i < allFriends[level].size(); i++) {

                        dist_t curd = space->IndexTimeDistance(this->getData(), allFriends[level][i]->getData());
                        if (curd > max) {
                            max = curd;
                            maxi = i;
                        }
                    }
                    allFriends[level].erase(allFriends[level].begin() + maxi);
                }
            }




        }

    public: void init(int level1, int maxFriends, int maxfriendslevel0) {

        level = level1;
        maxsize = maxFriends;
        maxsize0 = maxfriendslevel0;
        allFriends.resize(level + 1);
        for (int i = 0; i <= level; i++) {
            allFriends[i].reserve(maxsize + 1);
        }
        allFriends[0].reserve(maxsize0 + 1);

    }
    public: void copyDataAndLevel0LinksToOptIndex(char *mem1, size_t offsetlevels, size_t offsetData) {
        char *mem = mem1;
        //Level
        *((int *)(mem)) = level;
        mem += sizeof(int);

        char *memlevels = mem1 + offsetlevels;

        char *memt = memlevels;
        *((int *)(memt)) = (int)allFriends[0].size();
        memt += sizeof(int);
        for (int j = 0; j < allFriends[0].size(); j++) {
            *((int *)(memt)) = (int)allFriends[0][j]->getId();
            memt += sizeof(int);
        }
        mem = mem1 + offsetData;
        memcpy(mem, data_->buffer(), data_->bufferlength());

        return;
    }
    public: void copyHigherLevelLinksToOptIndex(char *mem1, size_t offsetlevels) {
        char *mem = mem1;

        *((int *)(mem)) = level;
        mem += sizeof(int);

        char *memlevels = mem1 + offsetlevels;

        for (int i = 1; i <= level; i++) {

            char *memt = memlevels;
            *((int *)(memt)) = (int)allFriends[i].size();

            memt += sizeof(int);
            for (int j = 0; j < allFriends[i].size(); j++) {
                *((int *)(memt)) = (int)allFriends[i][j]->getId();
                memt += sizeof(int);
            }
            memlevels += (1 + maxsize)* sizeof(int);
        };
        return;
    }
            const Object* getData() const {
                return data_;
            }
            size_t getId() const { return id_; }
            const vector<HnswNode*>& getAllFriends(int level) const {
                return allFriends[level];
            }

            mutex accessGuard_;
    public:

    private:
        const Object*       data_;
    public:size_t              id_;
           vector<vector<HnswNode*>> allFriends;

           int maxsize0;
           int maxsize;
           int level;


    };


    //----------------------------------
    template <typename dist_t>
    class HnswNodeDistFarther {
    public:
        HnswNodeDistFarther() {
            distance = 0;
            element = NULL;
        }
        HnswNodeDistFarther(dist_t di, HnswNode* node) {
            distance = di;
            element = node;
        }
        ~HnswNodeDistFarther() {}
        dist_t getDistance() const { return distance; }
        HnswNode* getMSWNodeHier() const { return element; }
        bool operator< (const HnswNodeDistFarther &obj1) const {
            return (distance > obj1.getDistance());
        }

    private:
        dist_t distance;
        HnswNode* element;
    };

    template <typename dist_t>
    class EvaluatedMSWNodeInt {
    public:
        EvaluatedMSWNodeInt() {
            distance = 0;
            element = NULL;
        }
        EvaluatedMSWNodeInt(dist_t di, int element1) {
            distance = di;
            element = element1;
        }
        ~EvaluatedMSWNodeInt() {}
        dist_t getDistance() const { return distance; }
        int getMSWNodeHier() const { return element; }
        bool operator< (const EvaluatedMSWNodeInt &obj1) const {
            return (distance < obj1.getDistance());
        }

    private:
        dist_t distance;
    public:int element;
    };

    template <typename dist_t>
    class HnswNodeDistCloser {
    public:
        HnswNodeDistCloser() {
            distance = 0;
            element = NULL;
        }
        HnswNodeDistCloser(dist_t di, HnswNode* node) {
            distance = di;
            element = node;
        }
        ~HnswNodeDistCloser() {}
        dist_t getDistance() const { return distance; }
        HnswNode* getMSWNodeHier() const { return element; }
        bool operator< (const HnswNodeDistCloser &obj1) const {
            return (distance < obj1.getDistance());
        }

    private:
        dist_t distance;
        HnswNode* element;
    };

    template <typename dist_t>
    class Hnsw : public Index<dist_t> {
    public:
        virtual void SaveIndex(const string &location) override;

        virtual void LoadIndex(const string &location) override;

        Hnsw(bool PrintProgress,
            const Space<dist_t>& space,
            const ObjectVector& data);
        void CreateIndex(const AnyParams& IndexParams) override;

        ~Hnsw();


        const std::string StrDesc() const override;
        void Search(RangeQuery<dist_t>* query, IdType) const override;
        void Search(KNNQuery<dist_t>* query, IdType) const override;

        void SetQueryTimeParams(const AnyParams&) override;
    private:



        typedef std::vector<HnswNode*> ElementList;
        void baseSearchAlgorithm(KNNQuery<dist_t>* query);
        void listPassingModifiedAlgorithm(KNNQuery<dist_t>* query);
        void SearchL2Custom(KNNQuery<dist_t>* query);
        void SearchCosineNormalized(KNNQuery<dist_t>* query);

        

    private: 	int getRandomLevel(double revSize) {
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        double r = -log(distribution(*generator)) * revSize;
        return (int)r;
    }

    public:
        void kSearchElementsWithAttemptsLevel(const Space<dist_t>* space,
            const Object* queryObj, size_t NN,
            std::priority_queue<HnswNodeDistCloser<dist_t>>& resultSet, HnswNode* ep, int level) const;
        void add(const Space<dist_t>* space, HnswNode *newElement);
        void addToElementListSynchronized(HnswNode *newElement);

        void link(HnswNode* first, HnswNode* second, int level, const Space<dist_t>* space, int delaunay_type) {
            // We have to pass the Space, since we need to know what elements can be deleted from the list
            first->addFriendlevel(level, second, space, delaunay_type);
            second->addFriendlevel(level, first, space, delaunay_type);
        }

        //
    private:

        std::default_random_engine *generator;
        size_t                M_;
        size_t                maxM_;
        size_t                maxM0_;
        size_t                efConstruction_;
        size_t                ef_;
        size_t                searchMethod_;
        size_t                indexThreadQty_;
        VisitedListPool       *visitedlistpool;
        const Space<dist_t>&  space_;
        bool                  PrintProgress_;
        int                   delaunay_type_;
        double                mult_;
        int                   maxlevel_;
        HnswNode           *enterpoint_;
        unsigned int enterpointId_;
        unsigned int totalElementsStored_;

        const ObjectVector&   data_;// We do not copy objects
        //ObjectVector          data_; // We copy all the data

        mutable mutex         ElListGuard_;
        mutable mutex         MaxLevelGuard_;
        ElementList           ElList_;

        int                   vectorlength_ = 0;
        int dist_func_type_;
        bool                  iscosine_ = false;
        size_t                offsetData_, offsetLevel0_;
        char                  *data_level0_memory_;
        char                  **linkLists_;
        size_t                memoryPerObject_;
        float                 (*fstdistfunc_)(const float* pVect1, const float* pVect2, size_t &qty, float *TmpRes);
    protected:
        DISABLE_COPY_AND_ASSIGN(Hnsw);
    };


    class VisitedList {
    public: unsigned int curV;
    public:	unsigned int *mass;
    public: unsigned int numelements;
    public:
        VisitedList(int  numelements1) {
            curV = 0;
            numelements = numelements1;
            mass = new unsigned int[numelements];
        }
        void reset() {
            curV++;
            if (curV == 0)
            {
                memset(mass, 0, sizeof(int)*numelements);
            }
        };
        ~VisitedList() {

            delete mass;
        }
    };
    ///////////////////////////////////////////////////////////
    //
    // Class for multi-threaded pool-management of VisitedLists
    //
    /////////////////////////////////////////////////////////

    class VisitedListPool {
        vector<unsigned int*> listPool;
        vector<unsigned int*> curV;
        deque<VisitedList*> pool;
        mutex poolguard;
        int maxpools;
        int numelements;
    public:
        VisitedListPool(int initmaxpools, int numelements1) {
            numelements = numelements1;
            for (int i = 0; i < initmaxpools; i++)
                pool.push_front(new VisitedList(numelements));
        }
        VisitedList * getFreeVisitedList() {
            unique_lock<mutex> lock(poolguard);
            if (pool.size() > 0) {
                VisitedList *rez = pool.front();
                rez->reset();
                pool.pop_front();
                return rez;
            }
            else {
                return new VisitedList(numelements);
            }

        };
        void releaseVisitedList(VisitedList *vl) {
            unique_lock<mutex> lock(poolguard);
            pool.push_front(vl);
        };
        ~VisitedListPool() {
            LOG(LIB_INFO) << "Total " << pool.size() << " lists allocated";
            while (pool.size()) {
                VisitedList *rez = pool.front();
                pool.pop_front();
                delete rez;
            }
        };
    };







}
