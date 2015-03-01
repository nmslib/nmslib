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
#include "report.h"

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
  string  mMethodDesc;
  float   mRecallMin;
  float   mRecallMax;
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
               string methodDesc,
               unsigned  knn,
               float range, 
               float recallMin = 0, 
               float recallMax = 0,
               float numCloserMin = 0, 
               float numCloserMax = 0,
               float imprDistCompMin = 0, 
               float imprDistCompMax = 0) :
                   mDistType(distType),
                   mSpaceType(spaceType),
                   mDataSet(dataSet),
                   mMethodDesc(methodDesc),
                   mRecallMin(recallMin),
                   mRecallMax(recallMax),
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
                    const string& dataSet,
                    const string& distType,
                    const string& spaceType,
                    const MethodTestCase& testCase,
                    const ExperimentConfig<dist_t>& config,
                    MetaAnalysis& ExpRes,
                    const string& MethDescStr,
                    const string& MethParamStr,
                    string& PrintStr // For display
                    ) {
  std::stringstream Print, Data, Header;

  ExpRes.ComputeAll();

  PrintStr  = produceHumanReadableReport(config, ExpRes, MethDescStr, MethParamStr);

  bool bFail = false;

  if (ExpRes.GetRecallAvg() < testCase.mRecallMin) {
    cerr << "Failed to meet min recall requirement, expect >= " << testCase.mRecallMin 
         << " got " << ExpRes.GetRecallAvg() << endl 
         << " method: " << MethDescStr << " ; "
         << " data set: " << dataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  if (ExpRes.GetRecallAvg() > testCase.mRecallMax) {
    cerr << "Failed to meet max recall requirement, expect <= " << testCase.mRecallMax 
         << " got " << ExpRes.GetRecallAvg() << endl 
         << " method: " << MethDescStr << " ; "
         << " data set: " << dataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  if (ExpRes.GetNumCloserAvg() < testCase.mNumCloserMin) {
    cerr << "Failed to meet min # of points closer requirement, expect >= " << testCase.mNumCloserMin 
         << " got " << ExpRes.GetNumCloserAvg() << endl 
         << " method: " << MethDescStr << " ; "
         << " data set: " << dataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  if (ExpRes.GetNumCloserAvg() > testCase.mNumCloserMax) {
    cerr << "Failed to meet max # of points closer requirement, expect <= " << testCase.mNumCloserMax 
         << " got " << ExpRes.GetNumCloserAvg() << endl 
         << " method: " << MethDescStr << " ; "
         << " data set: " << dataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  if (ExpRes.GetImprDistCompAvg() < testCase.mImprDistCompMin) {
    cerr << "Failed to meet min improvement requirement in the # of distance computations, expect >= "
         << testCase.mImprDistCompMin << " got " << ExpRes.GetImprDistCompAvg() << endl 
         << " method: " << MethDescStr << " ; "
         << " data set: " << dataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  if (ExpRes.GetImprDistCompAvg() > testCase.mImprDistCompMax) {
    cerr << "Failed to meet max improvement requirement in the # of distance computations, expect <= "
         << testCase.mImprDistCompMax << " got " << ExpRes.GetImprDistCompAvg() << endl 
         << " method: " << MethDescStr << " ; "
         << " data set: " << dataSet << " ; "
         << " dist value  type: " << distType << " ; "
         << " space type: " << spaceType << endl << cmdStr << endl;
    bFail = true;
  }

  return !bFail;
};

/*
 * returns the # of failed tests
 */
template <typename dist_t>
size_t RunTestExper(const vector<MethodTestCase>& vTestCases,
             const string& cmdStr,
             const vector<shared_ptr<MethodWithParams>>& MethodsDesc,
             const string&                DataSet,
             const string&                DistType, 
             const string&                SpaceType,
             const shared_ptr<AnyParams>& SpaceParams,
             unsigned                     dimension,
             unsigned                     ThreadTestQty,
             unsigned                     TestSetQty,
             const string&                DataFile,
             const string&                QueryFile,
             unsigned                     MaxNumData,
             unsigned                     MaxNumQuery,
             const                        vector<unsigned>& knn,
             const                        float eps,
             const string&                RangeArg
)
{
  vector<dist_t> range;

  CHECK(MethodsDesc.size() == vTestCases.size());

  size_t nFail = 0;


  if (!RangeArg.empty()) {
    if (!SplitStr(RangeArg, range, ',')) {
      LOG(LIB_FATAL) << "Wrong format of the range argument: '" << RangeArg << "' Should be a list of coma-separated values.";
    }
  }

  // Note that space will be deleted by the destructor of ExperimentConfig
  ExperimentConfig<dist_t> config(SpaceFactoryRegistry<dist_t>::
                                  Instance().CreateSpace(SpaceType, *SpaceParams),
                                  DataFile, QueryFile, TestSetQty,
                                  MaxNumData, MaxNumQuery,
                                  dimension, knn, eps, range);

  config.ReadDataset();
  MemUsage  mem_usage_measure;


  std::vector<std::string>          MethDescStr;
  std::vector<std::string>          MethParams;
  vector<double>                    MemUsage;

  vector<vector<MetaAnalysis*>> ExpResRange(config.GetRange().size(),
                                                vector<MetaAnalysis*>(MethodsDesc.size()));
  vector<vector<MetaAnalysis*>> ExpResKNN(config.GetKNN().size(),
                                              vector<MetaAnalysis*>(MethodsDesc.size()));

  size_t MethNum = 0;

  for (auto it = MethodsDesc.begin(); it != MethodsDesc.end(); ++it, ++MethNum) {

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

    ReportIntrinsicDimensionality("Main data set" , *config.GetSpace(), config.GetDataObjects());

    vector<shared_ptr<Index<dist_t>>>  IndexPtrs;

    bool bFailDueExcept = false;

    try {
      
      for (const auto& methElem: MethodsDesc) {
        MethNum = &methElem - &MethodsDesc[0];
        
        const string& MethodName  = methElem->methName_;
        const AnyParams& MethPars = methElem->methPars_;
        const string& MethParStr = MethPars.ToString();

        LOG(LIB_INFO) << ">>>> Index type : " << MethodName;
        LOG(LIB_INFO) << ">>>> Parameters: " << MethParStr;
        const double vmsize_before = mem_usage_measure.get_vmsize();


        WallClockTimer wtm;

        wtm.reset();
        
        bool bCreateNew = true;
        
        if (MethNum && MethodName == MethodsDesc[MethNum-1]->methName_) {
          vector<string> exceptList = IndexPtrs.back()->GetQueryTimeParamNames();
          
          if (MethodsDesc[MethNum-1]->methPars_.equalsIgnoreInList(MethPars, exceptList)) {
            bCreateNew = false;
          }
        }

        LOG(LIB_INFO) << (bCreateNew ? "Creating a new index":"Using a previosuly created index");

        IndexPtrs.push_back(
                bCreateNew ?
                           shared_ptr<Index<dist_t>>(
                           MethodFactoryRegistry<dist_t>::Instance().
                           CreateMethod(true /* print progress */,
                                        MethodName, 
                                        SpaceType, config.GetSpace(), 
                                        config.GetDataObjects(), MethPars)
                           )
                           :IndexPtrs.back());

        LOG(LIB_INFO) << "==============================================";

        const double vmsize_after = mem_usage_measure.get_vmsize();

        const double data_size = DataSpaceUsed(config.GetDataObjects()) / 1024.0 / 1024.0;

        const double TotalMemByMethod = vmsize_after - vmsize_before + data_size;

        wtm.split();

        LOG(LIB_INFO) << ">>>> Process memory usage: " << vmsize_after << " MBs";
        LOG(LIB_INFO) << ">>>> Virtual memory usage: " << TotalMemByMethod << " MBs";
        LOG(LIB_INFO) << ">>>> Data size:            " << data_size << " MBs";
        LOG(LIB_INFO) << ">>>> Time elapsed:         " << (wtm.elapsed()/double(1e6)) << " sec";


        for (size_t i = 0; i < config.GetRange().size(); ++i) {
          MetaAnalysis* res = ExpResRange[i][MethNum];
          res->SetMem(TestSetId, TotalMemByMethod);
        }
        for (size_t i = 0; i < config.GetKNN().size(); ++i) {
          MetaAnalysis* res = ExpResKNN[i][MethNum];
          res->SetMem(TestSetId, TotalMemByMethod);
        }

        if (!TestSetId) {
          MethDescStr.push_back(IndexPtrs.back()->ToString());
          MethParams.push_back(MethParStr);
        }
      }

      GoldStandardManager<dist_t> managerGS(config);
      managerGS.Compute(0); // Keeping all GS entries, should be Ok here because our data sets are smallish

      Experiments<dist_t>::RunAll(true /* print info */, 
                                      ThreadTestQty,
                                      TestSetId,
                                      managerGS,
                                      ExpResRange, ExpResKNN,
                                      config, 
                                      IndexPtrs, MethodsDesc);


    } catch (const std::exception& e) {
      LOG(LIB_ERROR) << "Exception: " << e.what();
      bFailDueExcept = true;
    } catch (...) {
      LOG(LIB_ERROR) << "Unknown exception";
      bFailDueExcept = true;
    }

    if (bFailDueExcept) {
      LOG(LIB_FATAL) << "Failure due to an exception!";
    }
  }

  for (auto it = MethDescStr.begin(); it != MethDescStr.end(); ++it) {
    size_t MethNum = it - MethDescStr.begin();


    string Print, Data, Header;

    for (size_t i = 0; i < config.GetRange().size(); ++i) {
      MetaAnalysis* res = ExpResRange[i][MethNum];

      if (!ProcessAndCheckResults(cmdStr, DataSet, DistType, SpaceType, 
                                  vTestCases[MethNum], config, *res, MethDescStr[MethNum], MethParams[MethNum], Print)) {
        nFail++;
      }
      LOG(LIB_INFO) << "Range: " << config.GetRange()[i];
      LOG(LIB_INFO) << Print;
      LOG(LIB_INFO) << "Data: " << Header << Data;

      delete res;
    }

    for (size_t i = 0; i < config.GetKNN().size(); ++i) {
      MetaAnalysis* res = ExpResKNN[i][MethNum];

      if (!ProcessAndCheckResults(cmdStr, DataSet, DistType, SpaceType, 
                                  vTestCases[MethNum], config, *res, MethDescStr[MethNum], MethParams[MethNum], Print)) {
        nFail++;
      }
      LOG(LIB_INFO) << "KNN: " << config.GetKNN()[i];
      LOG(LIB_INFO) << Print;
      LOG(LIB_INFO) << "Data: " << Header << Data;

      delete res;
    }
  }

  return nFail;
}

inline char * StrDupNew(const char *p, size_t len = 0) {
  if (!len) len = strlen(p);
  char *res = new char[len + 1];
  copy(p, p + len + 1, res);
  return res;
};

bool RunOneTest(const vector<MethodTestCase>& vTestCases, 
                const vector<string>& vArgv) {
  vector<char *>   argv = {StrDupNew("experiment")}; // the first argument is a program's name

  bool bTestRes = true;

  stringstream cmd;

  for (string s: vArgv)
    cmd << s << " ";

  cerr << endl << "Test arguments: " << cmd.str() << endl;

  for (unsigned i = 0; i < vArgv.size(); ++i) argv.push_back(StrDupNew(vArgv[i].c_str()));

  size_t                threadQty = 1;
  string                DistType;
  string                SpaceType;
  shared_ptr<AnyParams> SpaceParams;
  bool                  DoAppend = false;
  string                ResFilePrefix;
  unsigned              TestSetQty = 10;
  string                DataFile;
  string                QueryFile;
  string                CacheGSFilePrefix;
  size_t                MaxCacheGSQty;
  unsigned              MaxNumData = 0;
  unsigned              MaxNumQuery = 1000;
  vector<unsigned>      knn;
  string                RangeArg;
  string                tmp1;
  unsigned              tmp2;
  unsigned              dimension = 0;
  float                 eps = 0.0;


  vector<shared_ptr<MethodWithParams>>        MethodsDesc;

  if (!CacheGSFilePrefix.empty()) {
    LOG(LIB_FATAL) << "Caching of gold standard data is not yet implemented for this utility";
  }
 
  ParseCommandLine(argv.size(), &argv[0], tmp1,
                       DistType,
                       SpaceType,
                       SpaceParams,
                       dimension,
                       tmp2,
                       DoAppend, 
                       ResFilePrefix,
                       TestSetQty,
                       DataFile,
                       QueryFile,
                       CacheGSFilePrefix,
                       MaxCacheGSQty,
                       MaxNumData,
                       MaxNumQuery,
                       knn,
                       eps,
                       RangeArg,
                       MethodsDesc);



  ToLower(DistType);

  if (DIST_TYPE_INT == DistType) {
    bTestRes = RunTestExper<int>(vTestCases,
                  cmd.str(),
                  MethodsDesc,
                  DataFile,
                  DistType,
                  SpaceType,
                  SpaceParams,
                  dimension,
                  threadQty,
                  TestSetQty,
                  DataFile,
                  QueryFile,
                  MaxNumData,
                  MaxNumQuery,
                  knn,
                  eps,
                  RangeArg
                 );
  } else if (DIST_TYPE_FLOAT == DistType) {
    bTestRes = RunTestExper<float>(vTestCases,
                  cmd.str(),
                  MethodsDesc,
                  DataFile,
                  DistType,
                  SpaceType,
                  SpaceParams,
                  dimension,
                  threadQty,
                  TestSetQty,
                  DataFile,
                  QueryFile,
                  MaxNumData,
                  MaxNumQuery,
                  knn,
                  eps,
                  RangeArg
                 );
  } else if (DIST_TYPE_DOUBLE == DistType) {
    bTestRes = RunTestExper<double>(vTestCases,
                  cmd.str(),
                  MethodsDesc,
                  DataFile,
                  DistType,
                  SpaceType,
                  SpaceParams,
                  dimension,
                  threadQty,
                  DoAppend, 
                  DataFile,
                  QueryFile,
                  MaxNumData,
                  MaxNumQuery,
                  knn,
                  eps,
                  RangeArg
                 );
  } else {
    LOG(LIB_FATAL) << "Unknown distance value type: " << DistType;
  }

  for (char *p: argv) delete [] p;

  return bTestRes;
};

#endif
