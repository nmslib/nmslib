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
#ifndef TEST_INTEGR_UTILS_H
#define TEST_INTEGR_UTILS_H

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <map>

#include "utils.h"
#include "bunit.h"
#include "report.h"
#include "params_def.h"
#include "params_cmdline.h"
#include "report_intr_dim.h"

using std::set;
using std::multimap;
using std::vector;
using std::copy;
using std::string;
using std::stringstream;

using namespace similarity;

/*
 * A data structure that defines 
 * 1) Search parameters
 * 2) Method parameters
 * 3) Search outcome (recall range, range for the improvement in distance computation)
 */
struct MethodTestCase {
  string  mDistType;
  string  mSpaceType;
  string  mDataSet;
  string  mMethodName;
  string  mIndexParams;
  string  mQueryTimeParams;
  float   mRecallMin;
  float   mRecallMax;
  bool    mRecallOnly;
  float   mNumCloserMin;
  float   mNumCloserMax;
  float   mImprDistCompMin;
  float   mImprDistCompMax;

  unsigned   mKNN;
  float      mRange;

  MethodTestCase(
               string distType,
               string spaceType,
               string dataSet,
               string methodName,
               string indexParams,
               string queryTimeParams,
               unsigned  knn,
               float range, 
               float recallMin,
               float recallMax,
               float numCloserMin,
               float numCloserMax,
               float imprDistCompMin,
               float imprDistCompMax,
               bool recallOnly = false) :
                   mDistType(distType),
                   mSpaceType(spaceType),
                   mDataSet(dataSet),
                   mMethodName(methodName),
                   mIndexParams(indexParams),
                   mQueryTimeParams(queryTimeParams),
                   mRecallMin(recallMin),
                   mRecallMax(recallMax),
                   mRecallOnly(recallOnly),
                   mNumCloserMin(numCloserMin),
                   mNumCloserMax(numCloserMax),
                   mImprDistCompMin(imprDistCompMin),
                   mImprDistCompMax(imprDistCompMax),
                   mKNN(knn), mRange(range)
                {
                  ToLower(mDistType);
                  ToLower(spaceType);
                }
};

template <typename dist_t>
bool ProcessAndCheckResults(
                    const string& cmdStr,
                    const string& distType,
                    const string& spaceType,
                    const MethodTestCase& testCase,
                    const ExperimentConfig<dist_t>& config,
                    MetaAnalysis& ExpRes,
                    string& PrintStr // For display
                    ) {
  std::stringstream Print, Data, Header;

  ExpRes.ComputeAll();

  PrintStr  = produceHumanReadableReport(config, ExpRes, testCase.mMethodName, testCase.mIndexParams, testCase.mQueryTimeParams);

  bool bFail = false;

  if (ExpRes.GetRecallAvg() < testCase.mRecallMin) {
    cerr << "Failed to meet min recall requirement, expect >= " << testCase.mRecallMin 
         << " got " << ExpRes.GetRecallAvg() << endl 
         << " method: " << testCase.mMethodName << " ; "
         << " index-time params: " << testCase.mIndexParams << " ; "
         << " query-time params: " << testCase.mQueryTimeParams << " ; "
         << " data set: " << testCase.mDataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  if (ExpRes.GetRecallAvg() > testCase.mRecallMax) {
    cerr << "Failed to meet max recall requirement, expect <= " << testCase.mRecallMax 
         << " got " << ExpRes.GetRecallAvg() << endl 
         << " method: " << testCase.mMethodName << " ; "
         << " index-time params: " << testCase.mIndexParams << " ; "
         << " query-time params: " << testCase.mQueryTimeParams << " ; "
         << " data set: " << testCase.mDataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  if (ExpRes.GetNumCloserAvg() < testCase.mNumCloserMin) {
    cerr << "Failed to meet min # of points closer requirement, expect >= " << testCase.mNumCloserMin 
         << " got " << ExpRes.GetNumCloserAvg() << endl 
         << " method: " << testCase.mMethodName << " ; "
         << " index-time params: " << testCase.mIndexParams << " ; "
         << " query-time params: " << testCase.mQueryTimeParams << " ; "
         << " data set: " << testCase.mDataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  if (ExpRes.GetNumCloserAvg() > testCase.mNumCloserMax) {
    cerr << "Failed to meet max # of points closer requirement, expect <= " << testCase.mNumCloserMax 
         << " got " << ExpRes.GetNumCloserAvg() << endl 
         << " method: " << testCase.mMethodName << " ; "
         << " index-time params: " << testCase.mIndexParams << " ; "
         << " query-time params: " << testCase.mQueryTimeParams << " ; "
         << " data set: " << testCase.mDataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  if (ExpRes.GetImprDistCompAvg() < testCase.mImprDistCompMin) {
    cerr << "Failed to meet min improvement requirement in the # of distance computations, expect >= "
         << testCase.mImprDistCompMin << " got " << ExpRes.GetImprDistCompAvg() << endl 
         << " method: " << testCase.mMethodName << " ; "
         << " index-time params: " << testCase.mIndexParams << " ; "
         << " query-time params: " << testCase.mQueryTimeParams << " ; "
         << " data set: " << testCase.mDataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  if (ExpRes.GetImprDistCompAvg() > testCase.mImprDistCompMax) {
    cerr << "Failed to meet max improvement requirement in the # of distance computations, expect <= "
         << testCase.mImprDistCompMax << " got " << ExpRes.GetImprDistCompAvg() << endl 
         << " method: " << testCase.mMethodName << " ; "
         << " index-time params: " << testCase.mIndexParams << " ; "
         << " query-time params: " << testCase.mQueryTimeParams << " ; "
         << " data set: " << testCase.mDataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  return !bFail;
};

inline string getFirstParam(const string& paramDef) {
  vector<string>  vParDefs;
  
  if (!SplitStr(paramDef, vParDefs, ',')) {
    PREPARE_RUNTIME_ERR(err) << "Cannot comma-split the parameter definition: '" << paramDef << "'";
    THROW_RUNTIME_ERR(err);
  }
  if (vParDefs.empty()) {
    PREPARE_RUNTIME_ERR(err) << "Empty definition list in the parameter definition: '" << paramDef << "'";
    THROW_RUNTIME_ERR(err);
  }
  return "--" + vParDefs[0];  
}

inline string quoteEmpty(const string& str) {
  return str.empty() ? string("\"\"") : str;
};

string CreateCmdStr(
             const MethodTestCase&        testCase,
             bool                         isRange,
             const string&                rangeOrKnnArg,
             const string&                DistType, 
             string                       SpaceTypeStr,
             unsigned                     ThreadTestQty,
             unsigned                     TestSetQty,
             const string&                DataFile,
             const string&                QueryFile,
             unsigned                     MaxNumData,
             unsigned                     MaxNumQuery,
             const                        float eps) {
  stringstream res;
  // our test data files shouldn't have spaces, so we won't escape them
  // anyways, this is only a debug output 
  res << getFirstParam(DATA_FILE_PARAM_OPT)    << " " << DataFile << " " 
      << getFirstParam(MAX_NUM_DATA_PARAM_OPT) << " " << MaxNumData << " " 
      << getFirstParam(DIST_TYPE_PARAM_OPT)    << " " << DistType << " " 
      << getFirstParam(SPACE_TYPE_PARAM_OPT)   << " " << SpaceTypeStr << " " 
      << getFirstParam(THREAD_TEST_QTY_PARAM_OPT)<< " " << ThreadTestQty << " " 
      << getFirstParam(EPS_PARAM_OPT)          << " " << eps << " " ;
   if (QueryFile.empty())
     res << getFirstParam(TEST_SET_QTY_PARAM_OPT) << " " << TestSetQty << " " ;
   else
     res << getFirstParam(QUERY_FILE_PARAM_OPT) << " " << QueryFile << " " ;

  res << getFirstParam(MAX_NUM_QUERY_PARAM_OPT) << " " << MaxNumQuery << " " 
      << getFirstParam(isRange ? RANGE_PARAM_OPT : KNN_PARAM_OPT)    << " " << rangeOrKnnArg << " " 
      << getFirstParam(METHOD_PARAM_OPT)        << " " << testCase.mMethodName << " " 
      << getFirstParam(INDEX_TIME_PARAMS_PARAM_OPT) << " " << quoteEmpty(testCase.mIndexParams) << " " 
      << getFirstParam(QUERY_TIME_PARAMS_PARAM_OPT) << " " << quoteEmpty(testCase.mQueryTimeParams);

  return res.str();
};
                    

/*
 * returns the # of failed tests
 */
template <typename dist_t>
size_t RunTestExper(const vector<MethodTestCase>& vTestCases,
             const string&                DistType, 
             string                       SpaceTypeStr,
             unsigned                     ThreadTestQty,
             unsigned                     TestSetQty,
             const string&                DataFile,
             const string&                QueryFile,
             unsigned                     MaxNumData,
             unsigned                     MaxNumQuery,
             const string&                KnnArg,
             const                        float eps,
             const string&                RangeArg
)
{
  vector<unsigned> knn;
  vector<dist_t>   range;

  size_t nFail = 0;

  if (!KnnArg.empty()) {
    if (!SplitStr(KnnArg, knn, ',')) {
      PREPARE_RUNTIME_ERR(err) << "Wrong format of the knn argument: '" << KnnArg << "' Should be a list of coma-separated int > 0 values.";
      THROW_RUNTIME_ERR(err);
    }
  }

  if (!RangeArg.empty()) {
    if (!SplitStr(RangeArg, range, ',')) {
      PREPARE_RUNTIME_ERR(err) << "Wrong format of the range argument: '" << RangeArg << "' Should be a list of coma-separated distance-type values.";
      THROW_RUNTIME_ERR(err);
    }
  }

  ToLower(SpaceTypeStr);
  string SpaceType;

  shared_ptr<AnyParams> SpaceParams;

  {
    vector<string>     desc;
    ParseSpaceArg(SpaceTypeStr, SpaceType, desc);
    SpaceParams = shared_ptr<AnyParams>(new AnyParams(desc));
  }


  unique_ptr<Space<dist_t>> space(SpaceFactoryRegistry<dist_t>::
                                  Instance().CreateSpace(SpaceType, *SpaceParams));

  if (NULL == space.get()) {
    PREPARE_RUNTIME_ERR(err) << "Cannot create space: '" << SpaceType;
    THROW_RUNTIME_ERR(err);
  }
 
  ExperimentConfig<dist_t> config(*space,
                                  DataFile, QueryFile, TestSetQty,
                                  MaxNumData, MaxNumQuery,
                                  knn, eps, range);

  config.ReadDataset();
  MemUsage  mem_usage_measure;


  std::vector<std::string>          MethDescStr;
  std::vector<std::string>          MethParams;
  vector<double>                    MemUsage;

  vector<vector<MetaAnalysis*>> ExpResRange(config.GetRange().size(),
                                                vector<MetaAnalysis*>(vTestCases.size()));
  vector<vector<MetaAnalysis*>> ExpResKNN(config.GetKNN().size(),
                                              vector<MetaAnalysis*>(vTestCases.size()));

  for (size_t MethNum = 0; MethNum < vTestCases.size(); ++MethNum) {
    for (size_t i = 0; i < config.GetRange().size(); ++i) {
      ExpResRange[i][MethNum] = new MetaAnalysis(config.GetTestSetToRunQty());
    }
    for (size_t i = 0; i < config.GetKNN().size(); ++i) {
      ExpResKNN[i][MethNum] = new MetaAnalysis(config.GetTestSetToRunQty());
    }
  }


  for (int TestSetId = 0; TestSetId < config.GetTestSetToRunQty(); ++TestSetId) {
    config.SelectTestSet(TestSetId);

    LOG(LIB_INFO) << ">>>> Test set id: " << TestSetId << " (set qty: " << config.GetTestSetToRunQty() << ")";

    GoldStandardManager<dist_t> managerGS(config);
    managerGS.Compute(ThreadTestQty, 0); // Keeping all GS entries, should be Ok here because our data sets are smallish
    
    for (size_t MethNum = 0; MethNum < vTestCases.size(); ++MethNum) {
      const string& MethodName  = vTestCases[MethNum].mMethodName;

      shared_ptr<AnyParams>           IndexParams; 
      vector<shared_ptr<AnyParams>>   vQueryTimeParams;

      bool recallOnly = vTestCases[MethNum].mRecallOnly;

      {
        vector<string>     desc;
        ParseArg(vTestCases[MethNum].mIndexParams, desc);
        IndexParams = shared_ptr<AnyParams>(new AnyParams(desc));
      }

      {
        vector<string>     desc;
        ParseArg(vTestCases[MethNum].mQueryTimeParams, desc);
        vQueryTimeParams.push_back(shared_ptr<AnyParams>(new AnyParams(desc)));
      }

      LOG(LIB_INFO) << ">>>> Index type : " << MethodName;
      LOG(LIB_INFO) << ">>>> Index-time parameters: " << IndexParams->ToString();

      const double vmsize_before = mem_usage_measure.get_vmsize();

      WallClockTimer wtm;

      wtm.reset();
      

      LOG(LIB_INFO) << "Creating a new index" ;

      shared_ptr<Index<dist_t>> IndexPtr(
                         MethodFactoryRegistry<dist_t>::Instance().
                         CreateMethod(false /* don't print progress */,
                                      MethodName, 
                                      SpaceType, config.GetSpace(), 
                                      config.GetDataObjects())
                         );

      IndexPtr->CreateIndex(*IndexParams);

      LOG(LIB_INFO) << "==============================================";

      const double vmsize_after = mem_usage_measure.get_vmsize();

      const double data_size = DataSpaceUsed(config.GetDataObjects()) / 1024.0 / 1024.0;

      const double TotalMemByMethod = vmsize_after - vmsize_before + data_size;

      wtm.split();

      LOG(LIB_INFO) << ">>>> Process memory usage: " << vmsize_after << " MBs";
      LOG(LIB_INFO) << ">>>> Virtual memory usage: " << TotalMemByMethod << " MBs";
      LOG(LIB_INFO) << ">>>> Data size:            " << data_size << " MBs";
      LOG(LIB_INFO) << ">>>> Time elapsed:         " << (wtm.elapsed()/double(1e6)) << " sec";

      /* 
       * We need to repackage MetaAnalysis arrays:
       * RunAll will deal with only a single method and 
       * a single set of query-time parameters.
       */ 

      vector<vector<MetaAnalysis*>> ExpResRangeTmp(config.GetRange().size(), vector<MetaAnalysis*>(1));
      vector<vector<MetaAnalysis*>> ExpResKNNTmp(config.GetKNN().size(), vector<MetaAnalysis*>(1));


      for (size_t i = 0; i < config.GetRange().size(); ++i) {
        MetaAnalysis* res = ExpResRange[i][MethNum];
        res->SetMem(TestSetId, TotalMemByMethod);
        ExpResRangeTmp[i][0] = res;
      }
      for (size_t i = 0; i < config.GetKNN().size(); ++i) {
        MetaAnalysis* res = ExpResKNN[i][MethNum];
        res->SetMem(TestSetId, TotalMemByMethod);
        ExpResKNNTmp[i][0] = res;
      }

      CHECK_MSG(vQueryTimeParams.size() == 1, 
                "Test integration code is currently can execute only one set of query-time parameters!");

      Experiments<dist_t>::RunAll(true /* print progress */, 
                                  ThreadTestQty,
                                  TestSetId,
                                  managerGS,
                                  recallOnly,
                                  ExpResRangeTmp, ExpResKNNTmp,
                                  config, 
                                  *IndexPtr, 
                                  vQueryTimeParams);

      }

  }

  for (size_t MethNum = 0; MethNum < vTestCases.size(); ++MethNum) {
    string Print, Data, Header;

    for (size_t i = 0; i < config.GetRange().size(); ++i) {
      MetaAnalysis* res = ExpResRange[i][MethNum];

      string cmdStr = CreateCmdStr(vTestCases[MethNum],
                                             true,
                                             ConvertToString(config.GetRange()[i]),
                                             DistType, 
                                             SpaceTypeStr,
                                             ThreadTestQty,
                                             TestSetQty,
                                             DataFile,
                                             QueryFile,
                                             MaxNumData,
                                             MaxNumQuery,
                                             eps);
      cout << cmdStr << endl;
      LOG(LIB_INFO) << "Command line params: " << cmdStr;
      if (!ProcessAndCheckResults(cmdStr, 
                                  DistType, SpaceType, 
                                  vTestCases[MethNum], config, *res, Print)) {
        nFail++;
        cout << red << "failed" << no_color << " (see logs for more details) " << endl;
      } else {
        cout << green << "passed" << no_color << endl;
      }
      LOG(LIB_INFO) << "Range: " << config.GetRange()[i];
      LOG(LIB_INFO) << Print;
      LOG(LIB_INFO) << "Data: " << Header << Data;

      delete res;
    }

    for (size_t i = 0; i < config.GetKNN().size(); ++i) {
      MetaAnalysis* res = ExpResKNN[i][MethNum];

      string cmdStr = CreateCmdStr(vTestCases[MethNum],
                                             false,
                                             ConvertToString(config.GetKNN()[i]),
                                             DistType, 
                                             SpaceTypeStr,
                                             ThreadTestQty,
                                             TestSetQty,
                                             DataFile,
                                             QueryFile,
                                             MaxNumData,
                                             MaxNumQuery,
                                             eps);

      cout << cmdStr << endl;
      LOG(LIB_INFO) << "Command line params: " << cmdStr;
      if (!ProcessAndCheckResults(cmdStr,
                                  DistType, SpaceType, 
                                  vTestCases[MethNum], config, *res, Print)) {
        cout << red << "failed" << no_color << " (see logs for more details) " << endl;
        nFail++;
      } else {
        cout << green << "passed" << no_color << endl;
      }
      LOG(LIB_INFO) << "KNN: " << config.GetKNN()[i];
      LOG(LIB_INFO) << Print;
      LOG(LIB_INFO) << "Data: " << Header << Data;

      delete res;
    }
  }

  return nFail;
}

inline bool RunOneTest(const vector<MethodTestCase>& vTestCases,
               string                       DistType, 
               string                       SpaceTypeStr,
               unsigned                     ThreadTestQty,
               unsigned                     TestSetQty,
               const string&                DataFile,
               const string&                QueryFile,
               unsigned                     MaxNumData,
               unsigned                     MaxNumQuery,
               const string&                KnnArg,
               const                        float eps,
               const string&                RangeArg) {
  bool bTestRes = false;
  ToLower(DistType);
  if (DIST_TYPE_INT == DistType) {
    bTestRes = RunTestExper<int>(vTestCases,
                  DistType,
                  SpaceTypeStr,
                  ThreadTestQty,
                  TestSetQty,
                  DataFile,
                  QueryFile,
                  MaxNumData,
                  MaxNumQuery,
                  KnnArg,
                  eps,
                  RangeArg
                 );
  } else if (DIST_TYPE_FLOAT == DistType) {
    bTestRes = RunTestExper<float>(vTestCases,
                  DistType,
                  SpaceTypeStr,
                  ThreadTestQty,
                  TestSetQty,
                  DataFile,
                  QueryFile,
                  MaxNumData,
                  MaxNumQuery,
                  KnnArg,
                  eps,
                  RangeArg
                 );
  } else if (DIST_TYPE_DOUBLE == DistType) {
    bTestRes = RunTestExper<double>(vTestCases,
                  DistType,
                  SpaceTypeStr,
                  ThreadTestQty,
                  TestSetQty,
                  DataFile,
                  QueryFile,
                  MaxNumData,
                  MaxNumQuery,
                  KnnArg,
                  eps,
                  RangeArg
                 );
  } else {
    PREPARE_RUNTIME_ERR(err) << "Unknown distance value type: " << DistType;
    THROW_RUNTIME_ERR(err);
  }

  return bTestRes;
};

#endif
