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
#include "method/small_world_rand_split.h"

#include <vector>
#include <set>
#include <map>
#include <sstream>
#include <typeinfo>

//#define START_WITH_E0
#define START_WITH_E0_AT_QUERY_TIME

//#define USE_ALTERNATIVE_FOR_INDEXING

namespace similarity {

using namespace std;

template <typename dist_t>
struct IndexThreadParamsSplitSW {
  const Space<dist_t>&                        space_;
  SmallWorldRandSplit<dist_t>&                index_;
  const ObjectVector&                         data_;
  size_t                                      index_every_;
  size_t                                      out_of_;
  size_t                                      start_;
  size_t                                      end_;
  ProgressDisplay*                            progress_bar_;
  mutex&                                      display_mutex_;
  size_t                                      progress_update_qty_;
  vector<bool>                                visitedBitset_;

  IndexThreadParamsSplitSW(
                     const Space<dist_t>&             space,
                     SmallWorldRandSplit<dist_t>&     index, 
                     const ObjectVector&              data,
                     size_t                           index_every,
                     size_t                           out_of,
                     size_t                           start,
                     size_t                           end,
                     ProgressDisplay*                 progress_bar,
                     mutex&                           display_mutex,
                     size_t                           progress_update_qty
                      ) : 
                     space_(space),
                     index_(index), 
                     data_(data),
                     index_every_(index_every),
                     out_of_(out_of),
                     start_(start),
                     end_(end),
                     progress_bar_(progress_bar),
                     display_mutex_(display_mutex),
                     progress_update_qty_(progress_update_qty),
                     visitedBitset_(end - start)
                     { }
};

template <typename dist_t>
struct IndexThreadSplitSW {
  void operator()(IndexThreadParamsSplitSW<dist_t>& prm) {
    ProgressDisplay*  progress_bar = prm.progress_bar_;
    mutex&            display_mutex(prm.display_mutex_); 

    size_t nextQty = prm.progress_update_qty_;
    for (size_t id = prm.start_; id < prm.end_; ++id) {
      if (prm.index_every_ == id % prm.out_of_) {
        typename SmallWorldRandSplit<dist_t>::MSWNode* node = new typename SmallWorldRandSplit<dist_t>::MSWNode(prm.data_[id], id);
        prm.index_.add(node, prm.start_, prm.end_, prm.visitedBitset_);
      
        if ((id + 1 >= min(prm.data_.size(), nextQty)) && progress_bar) {
          unique_lock<mutex> lock(display_mutex);
          (*progress_bar) += (nextQty - progress_bar->count());
          nextQty += prm.progress_update_qty_;
        }
      }
    }
  }
};

template <typename dist_t>
SmallWorldRandSplit<dist_t>::SmallWorldRandSplit(bool PrintProgress,
                                       const Space<dist_t>& space,
                                       const ObjectVector& data) : 
                                       space_(space), data_(data), PrintProgress_(PrintProgress) {}

template <typename dist_t>
void SmallWorldRandSplit<dist_t>::CreateIndex(const AnyParams& IndexParams) 
{
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("NN",                 NN_,                  10);
  pmgr.GetParamOptional("efConstruction",     efConstruction_,      NN_);
  pmgr.GetParamOptional("chunkIndexSize",     chunkIndexSize_,      data_.size());
  CHECK_MSG(chunkIndexSize_ > 0, "chunkIndexSize should be > 0");

  chunkIndexSize_ = min(chunkIndexSize_, data_.size());
  efSearch_ = NN_;
  pmgr.GetParamOptional("initIndexAttempts",  initIndexAttempts_,   2);
  pmgr.GetParamOptional("indexThreadQty",     indexThreadQty_,      thread::hardware_concurrency());
  if (indexThreadQty_ <=0) indexThreadQty_ = 1;

  LOG(LIB_INFO) << "NN                  = " << NN_;
  LOG(LIB_INFO) << "efConstruction      = " << efConstruction_;
  LOG(LIB_INFO) << "chunkIndexSize      = " << chunkIndexSize_;
  LOG(LIB_INFO) << "initIndexAttempts   = " << initIndexAttempts_;
  LOG(LIB_INFO) << "indexThreadQty      = " << indexThreadQty_;

  pmgr.CheckUnused();

  SetQueryTimeParams(getEmptyParams());

  if (data_.empty()) return;

  unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                                new ProgressDisplay(data_.size(), cerr)
                                :NULL);

  for (size_t start = 0, chunkNum = 0; start < data_.size(); start += chunkIndexSize_, ++chunkNum) {
    size_t end = min(data_.size(), start + chunkIndexSize_);
    CHECK(end > start);

    vector<thread>                                          threads(indexThreadQty_);
    vector<shared_ptr<IndexThreadParamsSplitSW<dist_t>>>    threadParams;
    mutex                                                   progressBarMutex;

    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threadParams.push_back(shared_ptr<IndexThreadParamsSplitSW<dist_t>>(
          new IndexThreadParamsSplitSW<dist_t>(space_, *this, data_,
                                          i, indexThreadQty_,
                                          start, end,
                                          progress_bar.get(), progressBarMutex, 200)));
    }
    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threads[i] = thread(IndexThreadSplitSW<dist_t>(), ref(*threadParams[i]));
    }
    for (size_t i = 0; i < indexThreadQty_; ++i) {
      threads[i].join();
    }
  }

  if (progress_bar) {
    (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
  }

  if (ElList_.size() != data_.size()) {
    stringstream err;
    err << "Bug: Indexing seems to be incomplete ElList_.size() (" << ElList_.size() << ") isn't equal to data_.size() (" << data_.size() << ")";
    LOG(LIB_INFO) << err.str();
    throw runtime_error(err.str());
  }
}

template <typename dist_t>
void 
SmallWorldRandSplit<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);
  pmgr.GetParamOptional("initSearchAttempts", initSearchAttempts_,  3);
  pmgr.GetParamOptional("efSearch", efSearch_, NN_);
  pmgr.CheckUnused();
  LOG(LIB_INFO) << "Set SmallWorldRandSplit query-time parameters:";
  LOG(LIB_INFO) << "initSearchAttempts =" << initSearchAttempts_;
  LOG(LIB_INFO) << "efSearch           =" << efSearch_;
}

template <typename dist_t>
const std::string SmallWorldRandSplit<dist_t>::StrDesc() const {
  return METH_SMALL_WORLD_RAND_SPLIT;
}

template <typename dist_t>
SmallWorldRandSplit<dist_t>::~SmallWorldRandSplit() {
}

template <typename dist_t>
typename SmallWorldRandSplit<dist_t>::MSWNode* SmallWorldRandSplit<dist_t>::getRandomEntryPointLocked(size_t start, size_t end) const
{
  unique_lock<mutex> lock(ElListGuard_);
  MSWNode* res = getRandomEntryPoint(start, end);
  return res;
}

template <typename dist_t>
size_t SmallWorldRandSplit<dist_t>::getEntryQtyLocked() const
{
  unique_lock<mutex> lock(ElListGuard_);
  size_t res = ElList_.size();
  return res;
}

template <typename dist_t>
typename SmallWorldRandSplit<dist_t>::MSWNode* SmallWorldRandSplit<dist_t>::getRandomEntryPoint(size_t start, size_t end) const {
  if(end <= start) {
    return NULL;
  } else {
    size_t num = RandomInt()% (end - start);
    return ElList_[start + num];
  }
}

template <typename dist_t>
void 
SmallWorldRandSplit<dist_t>::searchForIndexing(const Object *queryObj,
                                          size_t chunkStart, size_t chunkEnd, size_t randomEntryPointEnd,
                                          vector<bool>& visitedBitset,
                                          priority_queue<EvaluatedMSWNodeDirect> &resultSet) const
{
/*
 * The trick of using large dense bitsets instead of unordered_set was 
 * borrowed from Wei Dong's kgraph: https://github.com/aaalgo/kgraph
 *
 * This trick works really well even in a multi-threaded mode. Indeed, the amount
 * of allocated memory is small. For example, if one has 8M entries, the size of
 * the bitmap is merely 1 MB. Furthermore, setting 1MB of entries to zero via memset would take only
 * a fraction of millisecond.
 */
  visitedBitset.assign(visitedBitset.size(), false); // clear the bitset

  vector<MSWNode*> neighborCopy;

  for (size_t i=0; i < initIndexAttempts_; i++){
    /**
     * Search for the k most closest elements to the query.
     */

    MSWNode* provider = NULL;

    // Some entries will hold NULLs temporarily
    for (int att = 0; ((provider = getRandomEntryPointLocked(chunkStart, randomEntryPointEnd)) == NULL) && att < 100; ++att);

    if (provider == NULL) {
      unique_lock<mutex> lock(ElListGuard_);
      provider = ElList_[chunkStart];
    }

    priority_queue <dist_t>                    closestDistQueue;
    priority_queue <EvaluatedMSWNodeReverse>   candidateSet;

#ifdef USE_ALTERNATIVE_FOR_INDEXING
    dist_t d = space_.ProxyDistance(queryObj, provider->getData());
    #pragma message "Using an alternative/proxy function for indexing, not the original one!"          
#else
    dist_t d = space_.IndexTimeDistance(queryObj, provider->getData());
#endif
    EvaluatedMSWNodeReverse ev(d, provider);

    candidateSet.push(ev);
    closestDistQueue.push(d);

    if (closestDistQueue.size() > efConstruction_) {
      closestDistQueue.pop();
    }

    size_t nodeId = provider->getId();
    CHECK_MSG(nodeId >= chunkStart && nodeId < chunkEnd,
              "Bug, expecting node ID in the semi-open interval [" + ConvertToString(chunkStart) + "," + ConvertToString(chunkEnd) + ")");

    visitedBitset[nodeId - chunkStart] = true;

    resultSet.emplace(d, provider);
        
    if (resultSet.size() > NN_) { // TODO check somewhere that NN > 0
      resultSet.pop();
    }        

    while (!candidateSet.empty()) {
      const EvaluatedMSWNodeReverse& currEv = candidateSet.top();
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

        size_t nodeId = pNeighbor->getId();
        CHECK_MSG(nodeId >= chunkStart && nodeId < chunkEnd,
                  "Bug, expecting node ID in the semi-open interval [" + ConvertToString(chunkStart) + "," + ConvertToString(chunkEnd) + ")");

        if (!visitedBitset[nodeId - chunkStart]) {
          visitedBitset[nodeId - chunkStart] = true;

#ifdef USE_ALTERNATIVE_FOR_INDEXING
          d = space_.ProxyDistance(queryObj, pNeighbor->getData());
          #pragma message "Using an alternative/proxy function for indexing, not the original one!"          
#else
          d = space_.IndexTimeDistance(queryObj, pNeighbor->getData());
#endif

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
void SmallWorldRandSplit<dist_t>::add(MSWNode *newElement,
                                 const size_t chunkStart, const size_t chunkEnd,
                                 vector<bool>& visitedBitset){
  newElement->removeAllFriends();

  size_t randomEntryPointEnd = 0;

  size_t insertIndex = 0;
  {
    unique_lock<mutex> lock(ElListGuard_);
    CHECK_MSG(chunkIndexSize_ > 0, "chunkIndexSize should be > 0");
    CHECK(ElList_.size() >= chunkStart && ElList_.size() < chunkEnd);
    size_t chunkIndexId = ElList_.size() % chunkIndexSize_ ;
    if (0 == chunkIndexId) {
      // If we start a new chunk, don't connect chunk elements to previously inserted entries!
      ElList_.push_back(newElement);
      return;
    }

    CHECK(chunkIndexSize_ <= data_.size());
#ifdef START_WITH_E0
    randomEntryPointEnd = chunkStart + 1;
#else
    // don't think that we can ever have a data set so large that this summation would cause an overflow,
    // also we ensure that chunkIndexSize_ <= data_.size()
    randomEntryPointEnd = min(ElList_.size(), chunkStart + chunkIndexSize_);
#endif
    insertIndex = ElList_.size();
    /*
     * We need to claim the element space, otherwise we will get overlapping partitions in the multi-threaded mode.
     * NULL shouldn't cause problems during indexing, because NULLed entries do not appear as neighbors,
     * they can only be retrieved via a getRandomEntryPointLocked(). However, the function searchForIndexing
     * calls getRandomEntryPointLocked() until a non-NULL entry is returned.
     * After several failed attempts, we will use the first entry in the chunk as the starting point,
     * as this entry is guranteed to be non-NULL.
     */
    ElList_.push_back(NULL);
  }

  CHECK(randomEntryPointEnd > chunkStart);

  {
    priority_queue<EvaluatedMSWNodeDirect> resultSet;

    searchForIndexing(newElement->getData(), chunkStart, chunkEnd, randomEntryPointEnd, visitedBitset, resultSet);

    // TODO actually we might need to add elements in the reverse order in the future.
    // For the current implementation, however, the order doesn't seem to matter
    while (!resultSet.empty()) {
      link(resultSet.top().getMSWNode(), newElement);
      resultSet.pop();
    }
  }

  {
    unique_lock<mutex> lock(ElListGuard_);
    CHECK(NULL == ElList_[insertIndex]);
    ElList_[insertIndex] = newElement;
  }

}

template <typename dist_t>
void SmallWorldRandSplit<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  throw runtime_error("Range search is not supported!");
}


template <typename dist_t>
void SmallWorldRandSplit<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  if (ElList_.empty()) return;
/*
 * The trick of using large dense bitsets instead of unordered_set was 
 * borrowed from Wei Dong's kgraph: https://github.com/aaalgo/kgraph
 *
 * This trick works really well even in a multi-threaded mode. Indeed, the amount
 * of allocated memory is small. For example, if one has 8M entries, the size of
 * the bitmap is merely 1 MB. Furthermore, setting 1MB of entries to zero via memset would take only
 * a fraction of millisecond.
 */
  vector<bool>                        visitedBitset(chunkIndexSize_);

  // don't think that we can every have a data set so this would cause an overflow, also
  // we ensure that chunkIndexSize_ <= data_.size()
  CHECK(chunkIndexSize_ <= data_.size());
  for (size_t start = 0; start < ElList_.size(); start += chunkIndexSize_) {
    size_t end = min(ElList_.size(), start + chunkIndexSize_);
    CHECK(end > start);
    if (start) visitedBitset.assign(visitedBitset.size(), false); // Clear the visited array when moving to another chunk
    for (size_t attempId =0; attempId < initSearchAttempts_; attempId++) {
      /**
       * Search of most k-closest elements to the query.
       */

      priority_queue <dist_t>                  closestDistQueue; //The set of all elements which distance was calculated
      priority_queue <EvaluatedMSWNodeReverse> candidateQueue; //the set of elements which we can use to evaluate

#ifdef START_WITH_E0_AT_QUERY_TIME
      size_t randomEntryPointEnd = start + 1;
#else
      size_t randomEntryPointEnd = end;
#endif
      MSWNode *provider = getRandomEntryPoint(start, randomEntryPointEnd);

      const Object* currObj = provider->getData();
      dist_t d = query->DistanceObjLeft(currObj);
      query->CheckAndAddToResult(d, currObj); // This should be done before the object goes to the queue: otherwise it will not be compared to the query at all!

      EvaluatedMSWNodeReverse ev(d, provider);
      candidateQueue.push(ev);
      closestDistQueue.emplace(d);

      size_t nodeId = provider->getId();
      CHECK_MSG(nodeId >= start && nodeId < end,
                "Bug, expecting node ID in the semi-open interval [" + ConvertToString(start) + "," + ConvertToString(end) + ")");
      visitedBitset[nodeId-start] = true;

      while(!candidateQueue.empty()) {
        auto iter = candidateQueue.top(); // This one was already compared to the query
        const EvaluatedMSWNodeReverse& currEv = iter;

        dist_t lowerBound = closestDistQueue.top();

        // Did we reach a local minimum?
        if (currEv.getDistance() > lowerBound) {
          break;
        }

        const vector<MSWNode*>& neighbor = (currEv.getMSWNode())->getAllFriends();

        // Can't access curEv anymore! The reference would become invalid
        candidateQueue.pop();

        //calculate distance to each neighbor
        for (auto iter = neighbor.begin(); iter != neighbor.end(); ++iter){
          size_t nodeId = (*iter)->getId();
          CHECK_MSG(nodeId >= start && nodeId < end,
                    "Bug, expecting node ID in the semi-open interval [" + ConvertToString(start) + "," + ConvertToString(end) + ")");

          size_t nodeIdDiff = nodeId - start;
          if (!visitedBitset[nodeIdDiff]) {
            const Object* currObj = (*iter)->getData();
            dist_t d = query->DistanceObjLeft(currObj);

            visitedBitset[nodeIdDiff] = true;

            if (closestDistQueue.size() < efSearch_ || d < closestDistQueue.top()) {
              closestDistQueue.emplace(d);
              if (closestDistQueue.size() > efSearch_) {
                  closestDistQueue.pop(); 
              }
              candidateQueue.emplace(d, *iter);
            }

            query->CheckAndAddToResult(d, currObj);
          }
        }
      }
    }
  }
}

template <typename dist_t>
void SmallWorldRandSplit<dist_t>::SaveIndex(const string &location) {
  ofstream outFile(location);
  CHECK_MSG(outFile, "Cannot open file '" + location + "' for writing");
  outFile.exceptions(std::ios::badbit);
  size_t lineNum = 0;

  WriteField(outFile, METHOD_DESC, StrDesc()); lineNum++;
  WriteField(outFile, "NN", NN_); lineNum++;
  WriteField(outFile, "chunkIndexSize", chunkIndexSize_); lineNum++;

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
void SmallWorldRandSplit<dist_t>::LoadIndex(const string &location) {
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

    ReadField(inFile, "chunkIndexSize", chunkIndexSize_);
    CHECK_MSG(chunkIndexSize_ <= data_.size(), "chunkIndexSize is larger than the # of data points, did you create this index for a larger data set?");
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

template class SmallWorldRandSplit<float>;
template class SmallWorldRandSplit<double>;
template class SmallWorldRandSplit<int>;

}
