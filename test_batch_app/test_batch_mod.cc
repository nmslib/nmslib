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

#define CHECK_IDS true

using namespace similarity;
using namespace std;

void ParseCommandLine(int argc, char* argv[], bool& bPrintProgress,
                      string&                 LogFile,
                      string&                 SpaceType,
                      shared_ptr<AnyParams>&  SpaceParams,
                      string&                 DataFile,
                      string&                 QueryFile,
                      unsigned&               FirstBatchQty,
                      unsigned&               k,
                      string&                 MethodName,
                      shared_ptr<AnyParams>&  IndexTimeParams,
                      shared_ptr<AnyParams>&  QueryTimeParams,
                      unsigned&               BatchAddQty,
                      unsigned&               BatchDelQty) {
  k=0;

  string          indexTimeParamStr;
  string          queryTimeParamStr;
  string          spaceParamStr;

  bPrintProgress  = true;
  bool bSuppressPrintProgress;

  CmdOptions cmd_options;

  cmd_options.Add(new CmdParam(SPACE_TYPE_PARAM_OPT, SPACE_TYPE_PARAM_MSG,
                               &spaceParamStr, true));
  cmd_options.Add(new CmdParam(DATA_FILE_PARAM_OPT, DATA_FILE_PARAM_MSG,
                               &DataFile, true));
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
  unsigned                k;
  string                  MethodName;
  shared_ptr<AnyParams>   IndexTimeParams;
  shared_ptr<AnyParams>   QueryTimeParams;
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
                  FirstBatchQty,
                  k,
                  MethodName,
                  IndexTimeParams,
                  QueryTimeParams,
                  BatchAddQty,
                  BatchDelQty);

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

  WallClockTimer timerBatchAdd, timerQuery;

  double totalBatchAddTime = 0;

  unsigned iterId = 0;
  for (; !unused.empty(); ++iterId) {
    LOG(LIB_INFO) << "Batch id: " << iterId;
    ObjectVector BatchData;
    // unused is filled from the back and emptied from the front
    for (size_t i = 0; i < BatchAddQty && !unused.empty(); ++i) {
      BatchData.push_back(unused.front());
      unused.pop_front();
    }
    for (auto const v : BatchData)
      IndexedData.push_back(v);

    timerBatchAdd.reset();
    index->AddBatch(BatchData, CHECK_IDS);
    timerBatchAdd.split();
    totalBatchAddTime += timerBatchAdd.elapsed();

    float recall = 0;
    double queryTime = 0;
    for (const auto qo : QuerySet) {
      KNNQuery<float>        knnSeqQuery(*space, qo, k);
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
      KNNQuery<float>        knnRegQuery(*space, qo, k);

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
    recall /= (k*QuerySet.size());
    LOG(LIB_INFO) << "Batch id: " << iterId << " recall: " << recall << " time (whole batch): " << queryTime/1000.0 << " ms";
  }
  LOG(LIB_INFO) << "All input data is indexed, exiting!";
  LOG(LIB_INFO) << "batch indexing average time per batch: " << totalBatchAddTime/1000.0/iterId << " ms";

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
