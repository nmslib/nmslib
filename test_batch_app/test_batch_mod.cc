/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2017
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#include <iostream>

#include <vector>
#include <deque>
#include <unordered_set>

#include "init.h"
#include "space.h"
#include "index.h"
#include "spacefactory.h"
#include "methodfactory.h"
#include "params_def.h"
#include "cmd_options.h"
#include "params.h"
#include "ztimer.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "method/small_world_rand.h"

// This check only makes sense when we don't delete data
#define CHECK_IDS false

using namespace similarity;
using namespace std;

void ParseCommandLine(int argc, char* argv[], bool& bPrintProgress,
                      string&                 LogFile,
                      string&                 SpaceType,
                      shared_ptr<AnyParams>&  SpaceParams,
                      string&                 DataFile,
                      string&                 QueryFile,
                      unsigned&               MaxIterQty,
                      unsigned&               FirstBatchQty,
                      unsigned&               k,
                      string&                 MethodName,
                      shared_ptr<AnyParams>&  IndexTimeParams,
                      bool&                   bPatchFlag,
                      shared_ptr<AnyParams>&  QueryTimeParams,
                      unsigned&               BatchAddQty,
                      unsigned&               BatchDelQty) {
  k=0;

  string          indexTimeParamStr;
  string          queryTimeParamStr;
  string          spaceParamStr;

  bPatchFlag = false;
  bPrintProgress  = true;
  bool bSuppressPrintProgress;

  CmdOptions cmd_options;

  cmd_options.Add(new CmdParam(SPACE_TYPE_PARAM_OPT, SPACE_TYPE_PARAM_MSG,
                               &spaceParamStr, true));
  cmd_options.Add(new CmdParam(DATA_FILE_PARAM_OPT, DATA_FILE_PARAM_MSG,
                               &DataFile, true));
  cmd_options.Add(new CmdParam("max_iter_qty", "The maximum # of iterations",
                               &MaxIterQty, true));
  cmd_options.Add(new CmdParam("first_batch_qty", "The number of data points in the first batch",
                               &FirstBatchQty, true));
  cmd_options.Add(new CmdParam(QUERY_FILE_PARAM_OPT, QUERY_FILE_PARAM_MSG,
                               &QueryFile, true));
  cmd_options.Add(new CmdParam(KNN_PARAM_OPT, KNN_PARAM_MSG,
                               &k, true));
  cmd_options.Add(new CmdParam(QUERY_TIME_PARAMS_PARAM_OPT, QUERY_TIME_PARAMS_PARAM_MSG,
                               &queryTimeParamStr, false));
  cmd_options.Add(new CmdParam(INDEX_TIME_PARAMS_PARAM_OPT, INDEX_TIME_PARAMS_PARAM_MSG,
                               &indexTimeParamStr, false));
  cmd_options.Add(new CmdParam(METHOD_PARAM_OPT, METHOD_PARAM_MSG,
                               &MethodName, true));
  cmd_options.Add(new CmdParam(NO_PROGRESS_PARAM_OPT, NO_PROGRESS_PARAM_MSG,
                               &bSuppressPrintProgress, false));
  cmd_options.Add(new CmdParam("patch_flag", "Do we \"patch\" the index graph after deletion?",
                               &bPatchFlag, false, true));
  cmd_options.Add(new CmdParam(LOG_FILE_PARAM_OPT, LOG_FILE_PARAM_MSG,
                               &LogFile, false, LOG_FILE_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam("batch_add_qty", "A number of data points added in a batch",
                               &BatchAddQty, true));
  cmd_options.Add(new CmdParam("batch_del_qty", "A number of randomly selected data points deleted in a batch",
                               &BatchDelQty, true));

  try {
    cmd_options.Parse(argc, argv);
  } catch (const CmdParserException& e) {
    cmd_options.ToString();
    std::cout.flush();
    LOG(LIB_FATAL) << e.what();
  } catch (const std::exception& e) {
    cmd_options.ToString();
    std::cout.flush();
    LOG(LIB_FATAL) << e.what();
  } catch (...) {
    cmd_options.ToString();
    std::cout.flush();
    LOG(LIB_FATAL) << "Failed to parse cmd arguments";
  }

  bPrintProgress = !bSuppressPrintProgress;

  ToLower(spaceParamStr);
  ToLower(MethodName);

  try {
    {
      vector<string>     desc;
      ParseSpaceArg(spaceParamStr, SpaceType, desc);
      SpaceParams = shared_ptr<AnyParams>(new AnyParams(desc));
    }

    {
      vector<string>     desc;
      ParseArg(indexTimeParamStr, desc);
      IndexTimeParams = shared_ptr<AnyParams>(new AnyParams(desc));
    }

    {
      vector<string>  desc;

      ParseArg(queryTimeParamStr, desc);
      QueryTimeParams = shared_ptr<AnyParams>(new AnyParams(desc));
    }


    if (DataFile.empty()) {
      LOG(LIB_FATAL) << "data file is not specified!";
    }

    if (!DoesFileExist(DataFile)) {
      LOG(LIB_FATAL) << "data file " << DataFile << " doesn't exist";
    }

    if (!QueryFile.empty() && !DoesFileExist(QueryFile)) {
      LOG(LIB_FATAL) << "query file " << QueryFile << " doesn't exist";
    }

  } catch (const exception& e) {
    LOG(LIB_FATAL) << "Exception: " << e.what();
  }
}

void doWork(int argc, char* argv[]) {
  string                  LogFile;
  string                  SpaceType;
  shared_ptr<AnyParams>   SpaceParams;
  unsigned                ThreadTestQty;
  string                  DataFile;
  string                  QueryFile;
  unsigned                FirstBatchQty;
  unsigned                knnK;
  string                  MethodName;
  shared_ptr<AnyParams>   IndexTimeParams;
  bool                    bPatchFlag;
  shared_ptr<AnyParams>   QueryTimeParams;
  unsigned                MaxIterQty;
  unsigned                BatchAddQty;
  unsigned                BatchDelQty;
  bool                    bPrintProgress;

  ParseCommandLine(argc, argv, 
                  bPrintProgress,
                  LogFile,
                  SpaceType,
                  SpaceParams,
                  DataFile,
                  QueryFile,
                  MaxIterQty,
                  FirstBatchQty,
                  knnK,
                  MethodName,
                  IndexTimeParams,
                  bPatchFlag,
                  QueryTimeParams,
                  BatchAddQty,
                  BatchDelQty);

  CHECK_MSG(knnK > 0, "k-NN k should be > 0!");

  if (LogFile != "") 
    initLibrary(LIB_LOGFILE, LogFile.c_str());
  else
    initLibrary(LIB_LOGSTDERR, NULL); // Use STDERR for logging

  unique_ptr<Space<float>>  space(SpaceFactoryRegistry<float>::Instance().CreateSpace(SpaceType, *SpaceParams));

  ObjectVector     OrigDataSet, QuerySet;
  vector<string>   vIgnoreExternIds;


  space->ReadDataset(OrigDataSet, vIgnoreExternIds,
                     DataFile, 0);
  space->ReadDataset(QuerySet, vIgnoreExternIds,
                     QueryFile, 0);

  LOG(LIB_INFO) << "Total # of data points loaded: " << OrigDataSet.size();
  LOG(LIB_INFO) << "Total # of query points loaded: " << QuerySet.size();
  LOG(LIB_INFO) << "Patch flag: " << bPatchFlag;

  deque<const Object*> unused;
  for(const auto v: OrigDataSet)
    unused.push_back(v);

  ObjectVector IndexedData;
  for (size_t i = 0; i < FirstBatchQty && !unused.empty(); ++i) {
    IndexedData.push_back(unused.front());
    unused.pop_front();
  }

  unique_ptr<Index<float>>   index(MethodFactoryRegistry<float>::Instance().
                                      CreateMethod(bPrintProgress,
                                                   MethodName, 
                                                   SpaceType,
                                                   *space,
                                                   IndexedData
                                                   ));

  // This method MUST be called for proper initialization
  index->CreateIndex(*IndexTimeParams);

  WallClockTimer timerBatchAdd, timerBatchDel, timerQuery;

  double totalBatchAddTime = 0;
  double totalBatchDelTime = 0;

  unsigned iterId = 0;
  for (; !unused.empty(); ++iterId) {
    LOG(LIB_INFO) << "Batch id: " << iterId << " IndexedData.size() " << IndexedData.size();
    ObjectVector BatchData;
    // unused is filled from the back and emptied from the front
    for (size_t i = 0; i < BatchAddQty && !unused.empty(); ++i) {
      BatchData.push_back(unused.front());
      unused.pop_front();
    }
    for (auto const v : BatchData)
      IndexedData.push_back(v);

    LOG(LIB_INFO) << "BatchData.size(): "      << BatchData.size();
    LOG(LIB_INFO) << "IndexedData.size() (after addition): " << IndexedData.size();

    timerBatchAdd.reset();
    index->AddBatch(BatchData, false /* no print progress here */, CHECK_IDS);
    timerBatchAdd.split();
    totalBatchAddTime += timerBatchAdd.elapsed();


    CHECK_MSG(BatchDelQty <= IndexedData.size(), 
              "Data is too small to accommodate deletion of batches of size: " + ConvertToString(BatchDelQty))

    vector<IdType> NodeToDelIndx(BatchDelQty);

    // Reservoir sampling to select nodes to be deleted
    for (size_t indx = 0; indx < BatchDelQty; ++indx)
      NodeToDelIndx[indx] = indx;
    for (size_t indx = BatchDelQty; indx < IndexedData.size(); ++indx) {
      size_t newDelIndx = RandomInt() % IndexedData.size();
      if (newDelIndx < BatchDelQty)
        NodeToDelIndx[newDelIndx] = indx;
    }

    sort(NodeToDelIndx.begin(), NodeToDelIndx.end());
    // Now let's 
    // 1) retrieve and store to-be-deleted nodes 
    // 2) delete them from IndexedData array 
    // 3) return them to the dequeue of available nodes.

    ObjectVector NodesToDel;

    size_t indx1 = 0, pos2 = 0;

    ObjectVector NewIndexedData;

    while (indx1 < IndexedData.size() && pos2 < NodeToDelIndx.size()) {
      size_t indx2 = NodeToDelIndx[pos2];
      /* 
       * Invariant, it is true because 
       * NodeToDelIndx contains indices in the range [0, IndexedData.size() )
       */
      CHECK(indx1 <= indx2); 
      if (indx1 < indx2) {
        // Node is not to be deleted, keep it in the data array
        NewIndexedData.push_back(IndexedData[indx1]);
        ++indx1;
      } else if (indx1 == indx2) {
        // If the node is to be deleted, move the pointer to the 
        // dequeue
        NodesToDel.push_back(IndexedData[indx1]);
        unused.push_back(IndexedData[indx1]);
        ++indx1;
        ++pos2; // this will lead to change in indx2 (in the beginning of the loop body)
      }
    }

    while (indx1 < IndexedData.size()) {
      // Node is not to be deleted, keep it in the data array
      NewIndexedData.push_back(IndexedData[indx1]);
      ++indx1;
    }

    IndexedData = NewIndexedData;

    LOG(LIB_INFO) << "NewIndexedData.size(): " << NewIndexedData.size();
    LOG(LIB_INFO) << "unused.size(): "         << unused.size();
    LOG(LIB_INFO) << "NodesToDel.size(): "     << NodesToDel.size();

    timerBatchDel.reset();
    index->DeleteBatch(NodesToDel, bPatchFlag ? SmallWorldRand<float>::kNeighborsOnly : 
                                                SmallWorldRand<float>::kNone, 
                       CHECK_IDS);
    timerBatchDel.split();
    totalBatchDelTime += timerBatchDel.elapsed();
    

    float recall = 0;
    double queryTime = 0;
    for (const auto qo : QuerySet) {
      KNNQuery<float>        knnSeqQuery(*space, qo, knnK);
      unordered_set<IdType>  trueNN;
      for (const auto v : IndexedData)
        knnSeqQuery.CheckAndAddToResult(v);

      {
        unique_ptr<KNNQueue<float>> res(knnSeqQuery.Result()->Clone());
        while (!res->Empty()) {
          trueNN.insert(res->TopObject()->id());
          res->Pop();
        }
      }
      timerQuery.reset();
      KNNQuery<float>        knnRegQuery(*space, qo, knnK);

      index->Search(&knnRegQuery);

      timerQuery.split();
      queryTime += timerQuery.elapsed();

      {
        unique_ptr<KNNQueue<float>> res(knnRegQuery.Result()->Clone());
        while (!res->Empty()) {
          recall += int(trueNN.find(res->TopObject()->id()) != trueNN.end());
          res->Pop();
        }
      }
    }
    recall /= (knnK*QuerySet.size());
    LOG(LIB_INFO) << "Batch id: " << iterId << " recall: " << recall << " SINGLE-thread time (complete query set): " << queryTime/1000.0 << " ms";
    if (iterId >= MaxIterQty) {
      LOG(LIB_INFO) << "Stopping b/c we reach the maximum # of iterations";
      break;
    }
  }
  LOG(LIB_INFO) << "All input data is indexed, exiting!";
  LOG(LIB_INFO) << "Batch indexing average time per batch: " << totalBatchAddTime/1000.0/iterId << " ms";
  LOG(LIB_INFO) << "Batch deletion average time per batch: " << totalBatchDelTime/1000.0/iterId << " ms";

}

int main(int argc, char* argv[]) {

  try {
    doWork(argc, argv);
  } catch (const std::exception& e) {
    LOG(LIB_FATAL) << "Exception: " << e.what();
  } catch (...) {
    LOG(LIB_FATAL) << "Unknown exception";
  }

  return 0;
}
