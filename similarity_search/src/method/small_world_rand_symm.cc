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
// This is only for _mm_prefetch
#include <mmintrin.h>

#include "space.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "rangequery.h"
#include "ported_boost_progress.h"
#include "method/small_world_rand_symm.h"
#include "sort_arr_bi.h"

#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <typeinfo>

#define MERGE_BUFFER_ALGO_SWITCH_THRESHOLD 100

//#define START_WITH_E0
#define START_WITH_E0_AT_QUERY_TIME

#define USE_BITSET_FOR_INDEXING 1

namespace similarity {

using namespace std;

template <typename dist_t>
struct IndexThreadParamsSWSymm {
  const Space<dist_t>&                        space_;
  SmallWorldRandSymm<dist_t>&                 index_;
  const ObjectVector&                         data_;
  size_t                                      index_every_;
  size_t                                      out_of_;
  ProgressDisplay*                            progress_bar_;
  mutex&                                      display_mutex_;
  size_t                                      progress_update_qty_;
  
  IndexThreadParamsSWSymm(
                     const Space<dist_t>&             space,
                     SmallWorldRandSymm<dist_t>&          index, 
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
struct IndexThreadSWSymm {
  void operator()(IndexThreadParamsSWSymm<dist_t>& prm) {
    ProgressDisplay*  progress_bar = prm.progress_bar_;
    mutex&            display_mutex(prm.display_mutex_); 
    /* 
     * Skip the first element, it was added already
     */
    size_t nextQty = prm.progress_update_qty_;
    for (size_t id = 1; id < prm.data_.size(); ++id) {
      if (prm.index_every_ == id % prm.out_of_) {
        MSWNode* node = new MSWNode(prm.data_[id], id);
        prm.index_.add(node);
      
        if ((id + 1 >= min(prm.data_.size(), nextQty)) && progress_bar) {
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
SmallWorldRandSymm<dist_t>::SmallWorldRandSymm(bool PrintProgress,
                                       const Space<dist_t>& space,
                                       const ObjectVector& data) : 
                                       space_(space), data_(data), PrintProgress_(PrintProgress), use_proxy_dist_(false) {}

template <typename dist_t>
void SmallWorldRandSymm<dist_t>::CreateIndex(const AnyParams& IndexParams) 
{
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("NN",                 NN_,                  10);
  pmgr.GetParamOptional("efConstruction",     efConstruction_,      NN_);
  efSearch_ = NN_;
  pmgr.GetParamOptional("initIndexAttempts",  initIndexAttempts_,   1);
  pmgr.GetParamOptional("indexThreadQty",     indexThreadQty_,      thread::hardware_concurrency());
  pmgr.GetParamOptional("useProxyDist",       use_proxy_dist_,      false);

  string s;
  pmgr.GetParamOptional(SYM_TYPE_PARAM, s, SYM_TYPE_NONE);
  indexSymm_ = getSymmType(s);

 

  LOG(LIB_INFO) << "NN                   = " << NN_;
  LOG(LIB_INFO) << "efConstruction_      = " << efConstruction_;
  LOG(LIB_INFO) << "initIndexAttempts    = " << initIndexAttempts_;
  LOG(LIB_INFO) << "indexThreadQty       = " << indexThreadQty_;
  LOG(LIB_INFO) << "useProxyDist         = " << use_proxy_dist_;
  LOG(LIB_INFO) << "symmType (index-time)="  << indexSymm_;

  pmgr.CheckUnused();

  SetQueryTimeParams(getEmptyParams());

  if (data_.empty()) return;

  // 2) One entry should be added before all the threads are started, or else add() will not work properly
  addCriticalSection(new MSWNode(data_[0], 0 /* id == 0 */));

  unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                                new ProgressDisplay(data_.size(), cerr)
                                :NULL);

  if (indexThreadQty_ <= 1) {
    // Skip the first element, one element is already added
    if (progress_bar) ++(*progress_bar);
    for (size_t id = 1; id < data_.size(); ++id) {
      MSWNode* node = new MSWNode(data_[id], id);
      add(node);
      if (progress_bar) ++(*progress_bar);
    }
  } else {
    vector<thread>                                    threads(indexThreadQty_);
    vector<shared_ptr<IndexThreadParamsSWSymm<dist_t>>>   threadParams; 
    mutex                                             progressBarMutex;

    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threadParams.push_back(shared_ptr<IndexThreadParamsSWSymm<dist_t>>(
                              new IndexThreadParamsSWSymm<dist_t>(space_, *this, data_, i, indexThreadQty_,
                                                              progress_bar.get(), progressBarMutex, 200)));
    }
    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threads[i] = thread(IndexThreadSWSymm<dist_t>(), ref(*threadParams[i]));
    }
    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threads[i].join();
    }
    if (ElList_.size() != data_.size()) {
      stringstream err;
      err << "Bug: ElList_.size() (" << ElList_.size() << ") isn't equal to data_.size() (" << data_.size() << ")";
      LOG(LIB_INFO) << err.str();
      throw runtime_error(err.str());
    }
    LOG(LIB_INFO) << indexThreadQty_ << " indexing threads have finished";
  }
}

template <typename dist_t>
void 
SmallWorldRandSymm<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);
  pmgr.GetParamOptional("initSearchAttempts", initSearchAttempts_,  1);
  pmgr.GetParamOptional("efSearch", efSearch_, NN_);

  string s;
  pmgr.GetParamOptional(SYM_TYPE_PARAM, s, SYM_TYPE_NONE);
  querySymm_ = getSymmType(s);
  
  if (querySymm_ != kSymmNone) {
    pmgr.GetParamRequired(SYM_K_PARAM, symmCandK_);
  }

  pmgr.CheckUnused();
  LOG(LIB_INFO) << "Set SmallWorldRandSymm query-time parameters:";
  LOG(LIB_INFO) << "initSearchAttempts   =" << initSearchAttempts_;
  LOG(LIB_INFO) << "efSearch             =" << efSearch_;
  LOG(LIB_INFO) << "symmType (query-time)=" << querySymm_;
  LOG(LIB_INFO) << "symmCandK            =" << symmCandK_;
}

template <typename dist_t>
const std::string SmallWorldRandSymm<dist_t>::StrDesc() const {
  return METH_SMALL_WORLD_RAND;
}

template <typename dist_t>
SmallWorldRandSymm<dist_t>::~SmallWorldRandSymm() {
}

template <typename dist_t>
MSWNode* SmallWorldRandSymm<dist_t>::getRandomEntryPointLocked() const
{
  unique_lock<mutex> lock(ElListGuard_);
  MSWNode* res = getRandomEntryPoint();
  return res;
}

template <typename dist_t>
size_t SmallWorldRandSymm<dist_t>::getEntryQtyLocked() const
{
  unique_lock<mutex> lock(ElListGuard_);
  size_t res = ElList_.size();
  return res;
}

template <typename dist_t>
MSWNode* SmallWorldRandSymm<dist_t>::getRandomEntryPoint() const {
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
SmallWorldRandSymm<dist_t>::searchForIndexing(const Object *queryObj,
                                          priority_queue<EvaluatedMSWNodeDirect<dist_t>> &resultSet) const
{
#if USE_BITSET_FOR_INDEXING
/*
 * The trick of using large dense bitsets instead of unordered_set was 
 * borrowed from Wei Dong's kgraph: https://github.com/aaalgo/kgraph
 *
 * This trick works really well even in a multi-threaded mode. Indeed, the amount
 * of allocated memory is small. For example, if one has 8M entries, the size of
 * the bitmap is merely 1 MB. Furthermore, setting 1MB of entries to zero via memset would take only
 * a fraction of millisecond.
 */
  vector<bool>                        visitedBitset(data_.size()); // seems to be working fine even in a multi-threaded mode.
#else
  unordered_set<MSWNode*>             visited;
#endif

  vector<MSWNode*> neighborCopy;

  for (size_t i=0; i < initIndexAttempts_; i++){

    /**
     * Search for the k most closest elements to the query.
     */
#ifdef START_WITH_E0
    MSWNode* provider = i ? getRandomEntryPointLocked() : ElList_[0];
#else
    MSWNode* provider = getRandomEntryPointLocked();
#endif

    priority_queue <dist_t>                     closestDistQueue;                      
    priority_queue <EvaluatedMSWNodeReverse<dist_t>>   candidateSet; 

/*
    dist_t d = use_proxy_dist_ ?  space_.ProxyDistance(queryObj, provider->getData()) : 
                                  space_.IndexTimeDistance(queryObj, provider->getData());
*/
    dist_t d = IndexTimeSymmDistance(provider->getData(), queryObj);
    EvaluatedMSWNodeReverse<dist_t> ev(d, provider);

    candidateSet.push(ev);
    closestDistQueue.push(d);

    if (closestDistQueue.size() > efConstruction_) {
      closestDistQueue.pop();
    }
 
#if USE_BITSET_FOR_INDEXING
    size_t nodeId = provider->getId();
    if (nodeId >= data_.size()) {
      stringstream err;
      err << "Bug: nodeId > data_size()";
      LOG(LIB_INFO) << err.str();
      throw runtime_error(err.str());
    }
    visitedBitset[nodeId] = true;
#else
    visited.insert(provider);
#endif
    resultSet.emplace(d, provider);
        
    if (resultSet.size() > NN_) { // TODO check somewhere that NN > 0
      resultSet.pop();
    }        

    while (!candidateSet.empty()) {
      const EvaluatedMSWNodeReverse<dist_t>& currEv = candidateSet.top();
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
      size_t neighborQty = 0;
      {
        unique_lock<mutex> lock(currNode->accessGuard_);

        //const vector<MSWNode*>& neighbor = currNode->getAllFriends();
        const vector<MSWNode*>& neighbor = currNode->getAllFriends();
        neighborQty = neighbor.size();
        if (neighborQty > neighborCopy.size()) neighborCopy.resize(neighborQty);
        for (size_t k = 0; k < neighborQty; ++k)
          neighborCopy[k]=neighbor[k];
      }

      // Can't access curEv anymore! The reference would become invalid
      candidateSet.pop();

      // calculate distance to each neighbor
      for (size_t neighborId = 0; neighborId < neighborQty; ++neighborId) {
        MSWNode* pNeighbor = neighborCopy[neighborId];
#if USE_BITSET_FOR_INDEXING
        size_t nodeId = pNeighbor->getId();
        if (nodeId >= data_.size()) {
          stringstream err;
          err << "Bug: nodeId > data_size()";
          LOG(LIB_INFO) << err.str();
          throw runtime_error(err.str());
        }
        if (!visitedBitset[nodeId]) {
          visitedBitset[nodeId] = true;
#else
        if (visited.find(pNeighbor) == visited.end()) {
          visited.insert(pNeighbor);
#endif
/*
          d = use_proxy_dist_ ? space_.ProxyDistance(queryObj, pNeighbor->getData()) : 
                                space_.IndexTimeDistance(queryObj, pNeighbor->getData());
*/
          dist_t d = IndexTimeSymmDistance(pNeighbor->getData(), queryObj);

          if (closestDistQueue.size() < efConstruction_ || d < closestDistQueue.top()) {
            closestDistQueue.push(d);
            if (closestDistQueue.size() > efConstruction_) {
              closestDistQueue.pop();
            }
            candidateSet.emplace(d, pNeighbor);
          }

          if (resultSet.size() < NN_ || resultSet.top().getDistance() > d) {
            resultSet.emplace(d, pNeighbor);
            if (resultSet.size() > NN_) { // TODO check somewhere that NN > 0
              resultSet.pop();
            }
          }
        }
      }
    }
  }
}


template <typename dist_t>
void SmallWorldRandSymm<dist_t>::add(MSWNode *newElement){
  newElement->removeAllFriends(); 

  bool isEmpty = false;

  {
    unique_lock<mutex> lock(ElListGuard_);
    isEmpty = ElList_.empty();
  }

  if(isEmpty){
  // Before add() is called, the first node should be created!
    LOG(LIB_INFO) << "Bug: the list of nodes shouldn't be empty!";
    throw runtime_error("Bug: the list of nodes shouldn't be empty!");
  }

  {
    priority_queue<EvaluatedMSWNodeDirect<dist_t>> resultSet;

    searchForIndexing(newElement->getData(), resultSet);

    // TODO actually we might need to add elements in the reverse order in the future.
    // For the current implementation, however, the order doesn't seem to matter
    while (!resultSet.empty()) {
      link(resultSet.top().getMSWNode(), newElement);
      resultSet.pop();
    }
  }

  addCriticalSection(newElement);
}

template <typename dist_t>
void SmallWorldRandSymm<dist_t>::addCriticalSection(MSWNode *newElement){
  unique_lock<mutex> lock(ElListGuard_);

  ElList_.push_back(newElement);
}

template <typename dist_t>
void SmallWorldRandSymm<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  throw runtime_error("Range search is not supported!");
}

template <typename dist_t>
void SmallWorldRandSymm<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  if (querySymm_ == kSymmNone) {
  // Straightforward mode, no symmetrization, no pre-filtering
    KNNQueue<dist_t>   resQueue(query->GetK());
    SearchInternal(*query, resQueue); 

    while (!resQueue.Empty()) {
      // No need to recompute distances, they have not been previously symmterized
      query->CheckAndAddToResult(resQueue.TopDistance(), resQueue.TopObject());
      resQueue.Pop();
    }
  } else {
   // Filtering mode, using larger queue
    KNNQueue<dist_t>   resQueue(symmCandK_);
    SearchInternal(*query, resQueue); 

    while (!resQueue.Empty()) {
      // In the filtering mode, we need to recompute the distance to every candidate
      query->CheckAndAddToResult(resQueue.TopObject());
      resQueue.Pop();
    }
  }
}

template <typename dist_t>
void SmallWorldRandSymm<dist_t>::SearchInternal(const KNNQuery<dist_t>& query, KNNQueue<dist_t>& resQueue) const {
  if (ElList_.empty()) return;
  CHECK_MSG(efSearch_ > 0, "efSearch should be > 0");

/*
 * The trick of using large dense bitsets instead of unordered_set was
 * borrowed from Wei Dong's kgraph: https://github.com/aaalgo/kgraph
 *
 * This trick works really well even in a multi-threaded mode. Indeed, the amount
 * of allocated memory is small. For example, if one has 8M entries, the size of
 * the bitmap is merely 1 MB. Furthermore, setting 1MB of entries to zero via memset would take only
 * a fraction of millisecond.
 */
  vector<bool>                        visitedBitset(ElList_.size());

  for (size_t i=0; i < initSearchAttempts_; i++) {
    /**
     * Search of most k-closest elements to the query.
     */
#ifdef START_WITH_E0_AT_QUERY_TIME
    MSWNode* provider = i ? getRandomEntryPoint(): ElList_[0];
#else
    MSWNode* provider = getRandomEntryPoint();
#endif
    //MSWNode* provider = getRandomEntryPoint();

    priority_queue <dist_t>                          closestDistQueue; //The set of all elements which distance was calculated
    priority_queue <EvaluatedMSWNodeReverse<dist_t>> candidateQueue; //the set of elements which we can use to evaluate

    const Object* currObj = provider->getData();
    //dist_t d = query.DistanceObjLeft(currObj);
    dist_t d = QueryTimeSymmDistance(query, currObj);
    // This should be done before the object goes to the queue: otherwise it will not be compared to the query at all!
    //query->CheckAndAddToResult(d, currObj); 
    resQueue.Push(d, currObj);
  

    EvaluatedMSWNodeReverse<dist_t> ev(d, provider);
    candidateQueue.push(ev);
    closestDistQueue.emplace(d);

    size_t nodeId = provider->getId();
    // data_.size() is guaranteed to be equal to ElList_.size()
    if (nodeId >= data_.size()) {
      stringstream err;
      err << "Bug: nodeId > data_size()";
      LOG(LIB_INFO) << err.str();
      throw runtime_error(err.str());
    }
    visitedBitset[nodeId] = true;

    while(!candidateQueue.empty()){

      auto iter = candidateQueue.top(); // This one was already compared to the query
      const EvaluatedMSWNodeReverse<dist_t>& currEv = iter;

      // Did we reach a local minimum?
      if (currEv.getDistance() > closestDistQueue.top()) {
        break;
      }

      for (MSWNode* neighbor : (currEv.getMSWNode())->getAllFriends()) {
        _mm_prefetch(reinterpret_cast<const char*>(const_cast<const Object*>(neighbor->getData())), _MM_HINT_T0);
      }
      for (MSWNode* neighbor : (currEv.getMSWNode())->getAllFriends()) {
        _mm_prefetch(const_cast<const char*>(neighbor->getData()->data()), _MM_HINT_T0);
      }

      const vector<MSWNode*>& neighbor = (currEv.getMSWNode())->getAllFriends();

      // Can't access curEv anymore! The reference would become invalid
      candidateQueue.pop();

      //calculate distance to each neighbor
      for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter){
        nodeId = (*iter)->getId();
        // data_.size() is guaranteed to be equal to ElList_.size()
        if (nodeId >= data_.size()) {
          stringstream err;
          err << "Bug: nodeId > data_size()";
          LOG(LIB_INFO) << err.str();
          throw runtime_error(err.str());
        }
        if (!visitedBitset[nodeId]) {
          currObj = (*iter)->getData();
          //d = query.DistanceObjLeft(currObj);
          d = QueryTimeSymmDistance(query, currObj);
          visitedBitset[nodeId] = true;

          if (closestDistQueue.size() < efSearch_ || d < closestDistQueue.top()) {
            closestDistQueue.emplace(d);
            if (closestDistQueue.size() > efSearch_) {
              closestDistQueue.pop();
            }

            candidateQueue.emplace(d, *iter);
          }

          //query->CheckAndAddToResult(d, currObj);
          resQueue.Push(d, currObj);
        }
      }
    }
  }
}

template <typename dist_t>
void SmallWorldRandSymm<dist_t>::SaveIndex(const string &location) {
  ofstream outFile(location);
  CHECK_MSG(outFile, "Cannot open file '" + location + "' for writing");
  outFile.exceptions(std::ios::badbit);
  size_t lineNum = 0;

  WriteField(outFile, METHOD_DESC, StrDesc()); lineNum++;
  WriteField(outFile, "NN", NN_); lineNum++;

  for (const MSWNode* pNode: ElList_) {
    IdType nodeID = pNode->getId();
    CHECK_MSG(nodeID >= 0 && nodeID < data_.size(),
              "Bug: unexpected node ID " + ConvertToString(nodeID) +
              " for object ID " + ConvertToString(pNode->getData()->id()) +
              "data_.size() = " + ConvertToString(data_.size()));
    outFile << nodeID << ":" << pNode->getData()->id() << ":";
    for (const MSWNode* pNodeFriend: pNode->getAllFriends()) {
      IdType nodeFriendID = pNodeFriend->getId();
      CHECK_MSG(nodeFriendID >= 0 && nodeFriendID < data_.size(),
                "Bug: unexpected node ID " + ConvertToString(nodeFriendID) +
                " for object ID " + ConvertToString(pNodeFriend->getData()->id()) +
                "data_.size() = " + ConvertToString(data_.size()));
      outFile << ' ' << nodeFriendID;
    }
    outFile << endl; lineNum++;
  }
  outFile << endl; lineNum++; // The empty line indicates the end of data entries
  WriteField(outFile, LINE_QTY, lineNum + 1 /* including this line */);
  outFile.close();
}

template <typename dist_t>
void SmallWorldRandSymm<dist_t>::LoadIndex(const string &location) {
  vector<MSWNode *> ptrMapper(data_.size());

  for (unsigned pass = 0; pass < 2; ++ pass) {
    ifstream inFile(location);
    CHECK_MSG(inFile, "Cannot open file '" + location + "' for reading");
    inFile.exceptions(std::ios::badbit);

    size_t lineNum = 1;
    string methDesc;
    ReadField(inFile, METHOD_DESC, methDesc);
    lineNum++;
    CHECK_MSG(methDesc == StrDesc(),
              "Looks like you try to use an index created by a different method: " + methDesc);
    ReadField(inFile, "NN", NN_);
    lineNum++;

    string line;
    while (getline(inFile, line)) {
      if (line.empty()) {
        lineNum++; break;
      }
      stringstream str(line);
      str.exceptions(std::ios::badbit);
      char c1, c2;
      IdType nodeID, objID;
      CHECK_MSG((str >> nodeID) && (str >> c1) && (str >> objID) && (str >> c2),
                "Bug or inconsitent data, wrong format, line: " + ConvertToString(lineNum)
      );
      CHECK_MSG(c1 == ':' && c2 == ':',
                string("Bug or inconsitent data, wrong format, c1=") + c1 + ",c2=" + c2 +
                " line: " + ConvertToString(lineNum)
      );
      CHECK_MSG(nodeID >= 0 && nodeID < data_.size(),
                DATA_MUTATION_ERROR_MSG + " (unexpected node ID " + ConvertToString(nodeID) +
                " for object ID " + ConvertToString(objID) +
                " data_.size() = " + ConvertToString(data_.size()) + ")");
      CHECK_MSG(data_[nodeID]->id() == objID,
                DATA_MUTATION_ERROR_MSG + " (unexpected object ID " + ConvertToString(data_[nodeID]->id()) +
                " for data element with ID " + ConvertToString(nodeID) +
                " expected object ID: " + ConvertToString(objID) + ")"
      );
      if (pass == 0) {
        unique_ptr<MSWNode> node(new MSWNode(data_[nodeID], nodeID));
        ptrMapper[nodeID] = node.get();
        ElList_.push_back(node.release());
      } else {
        MSWNode *pNode = ptrMapper[nodeID];
        CHECK_MSG(pNode != NULL,
                  "Bug, got NULL pointer in the second pass for nodeID " + ConvertToString(nodeID));
        IdType nodeFriendID;
        while (str >> nodeFriendID) {
          CHECK_MSG(nodeFriendID >= 0 && nodeFriendID < data_.size(),
                    "Bug: unexpected node ID " + ConvertToString(nodeFriendID) +
                    "data_.size() = " + ConvertToString(data_.size()));
          MSWNode *pFriendNode = ptrMapper[nodeFriendID];
          CHECK_MSG(pFriendNode != NULL,
                    "Bug, got NULL pointer in the second pass for nodeID " + ConvertToString(nodeFriendID));
          pNode->addFriend(pFriendNode, false /* don't check for duplicates */);
        }
        CHECK_MSG(str.eof(),
                  "It looks like there is some extract erroneous stuff in the end of the line " +
                  ConvertToString(lineNum));
      }
      ++lineNum;
    }

    size_t ExpLineNum;
    ReadField(inFile, LINE_QTY, ExpLineNum);
    CHECK_MSG(lineNum == ExpLineNum,
              DATA_MUTATION_ERROR_MSG + " (expected number of lines " + ConvertToString(ExpLineNum) +
              " read so far doesn't match the number of read lines: " + ConvertToString(lineNum) + ")");
    inFile.close();
  }
}

template class SmallWorldRandSymm<float>;
template class SmallWorldRandSymm<double>;
template class SmallWorldRandSymm<int>;

}
