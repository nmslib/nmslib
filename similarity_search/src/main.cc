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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cmath>
#include <limits>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <map>

#include "init.h"
#include "global.h"
#include "utils.h"
#include "memory.h"
#include "ztimer.h"
#include "experiments.h"
#include "experimentconf.h"
#include "space.h"
#include "spacefactory.h"
#include "logging.h"
#include "report.h"
#include "methodfactory.h"

#include "meta_analysis.h"
#include "params.h"

using namespace similarity;

using std::multimap;
using std::vector;
using std::string;
using std::stringstream;

void OutData(bool DoAppend, const string& FilePrefix,
             const string& Print, const string& Header, const string& Data) {
  string FileNameData = FilePrefix + ".dat";
  string FileNameRep  = FilePrefix + ".rep";

  LOG(LIB_INFO) << "DoAppend? " << DoAppend;

  std::ofstream   OutFileData(FileNameData.c_str(),
                              (DoAppend ? std::ios::app : (std::ios::trunc | std::ios::out)));

  if (!OutFileData) {
    LOG(LIB_FATAL) << "Cannot create output file: '" << FileNameData << "'";
  }
  OutFileData.exceptions(std::ios::badbit);

  std::ofstream   OutFileRep(FileNameRep.c_str(),
                              (DoAppend ? std::ios::app : (std::ios::trunc | std::ios::out)));

  if (!OutFileRep) {
    LOG(LIB_FATAL) << "Cannot create output file: '" << FileNameRep << "'";
  }
  OutFileRep.exceptions(std::ios::badbit);

  if (!DoAppend) {
      OutFileData << Header;
  }
  OutFileData<< Data;
  OutFileRep<< Print;

  OutFileRep.close();
  OutFileData.close();
}

template <typename dist_t>
void ProcessResults(const ExperimentConfig<dist_t>& config,
                    MetaAnalysis& ExpRes,
                    const string& MethDescStr,
                    const string& MethParamStr,
                    string& PrintStr, // For display
                    string& HeaderStr,
                    string& DataStr   /* to be processed by a script */) {
  std::stringstream Print, Data, Header;

  ExpRes.ComputeAll();

  Header << "MethodName\tRecall\tPrecisionOfApprox\tRelPosError\tNumCloser\tClassAccuracy\tQueryTime\tDistComp\tImprEfficiency\tImprDistComp\tMem\tMethodParams\tNumData" << std::endl;

  Data << "\"" << MethDescStr << "\"\t";
  Data << ExpRes.GetRecallAvg() << "\t";
  Data << ExpRes.GetPrecisionOfApproxAvg() << "\t";
  Data << ExpRes.GetRelPosErrorAvg() << "\t";
  Data << ExpRes.GetNumCloserAvg() << "\t";
  Data << ExpRes.GetClassAccuracyAvg() << "\t";
  Data << ExpRes.GetQueryTimeAvg() << "\t";
  Data << ExpRes.GetDistCompAvg() << "\t";
  Data << ExpRes.GetImprEfficiencyAvg() << "\t";
  Data << ExpRes.GetImprDistCompAvg() << "\t";
  Data << size_t(ExpRes.GetMemAvg()) << "\t";
  Data << "\"" << MethParamStr << "\"" << "\t";
  Data << config.GetDataObjects().size();
  Data << std::endl;

  PrintStr  = produceHumanReadableReport(config, ExpRes, MethDescStr, MethParamStr);

  DataStr   = Data.str();
  HeaderStr = Header.str();
};

template <typename dist_t>
void RunExper(const vector<shared_ptr<MethodWithParams>>& MethodsDesc,
             const string                 SpaceType,
             const shared_ptr<AnyParams>& SpaceParams,
             unsigned                     dimension,
             unsigned                     ThreadTestQty,
             bool                         DoAppend, 
             const string&                ResFilePrefix,
             unsigned                     TestSetQty,
             const string&                DataFile,
             const string&                QueryFile,
             const string&                CacheGSFilePrefix,
             size_t                       MaxCacheGSQty,
             unsigned                     MaxNumData,
             unsigned                     MaxNumQuery,
             const                        vector<unsigned>& knn,
             const                        float eps,
             const string&                RangeArg
)
{
  LOG(LIB_INFO) << "### Append? : "       << DoAppend;
  LOG(LIB_INFO) << "### OutFilePrefix : " << ResFilePrefix;
  vector<dist_t> range;

  bool bWriteGSCache = false;
  bool bReadGSCache = false;
  bool bFail = false;
  bool bCacheGS = !CacheGSFilePrefix.empty();

  unique_ptr<fstream>      cacheGSControl;
  unique_ptr<fstream>      cacheGSBinary;

  if (!RangeArg.empty()) {
    if (!SplitStr(RangeArg, range, ',')) {
      LOG(LIB_FATAL) << "Wrong format of the range argument: '" << RangeArg << "' Should be a list of coma-separated values.";
    }
  }

  
  unique_ptr<Space<dist_t>> space (SpaceFactoryRegistry<dist_t>::
                                  Instance().CreateSpace(SpaceType, *SpaceParams));

  ExperimentConfig<dist_t> config(space.get(),
                                  DataFile, QueryFile, TestSetQty,
                                  MaxNumData, MaxNumQuery,
                                  dimension, knn, eps, range);

  size_t cacheDataSetQty = 0;
  if (bCacheGS) {
    const string& cacheGSControlName = CacheGSFilePrefix + "_ctrl.txt";
    const string& cacheGSBinaryName  = CacheGSFilePrefix + "_data.bin";

    if (DoesFileExist(cacheGSControlName)) {
    // Cache exists => reuse it
      if (!DoesFileExist(cacheGSBinaryName)) {
        throw runtime_error("Inconsistent cache state, there is a text control file: '" +
                            cacheGSControlName + "' but no binary data file: '" +
                            cacheGSBinaryName + "'");
      }
      cacheGSControl.reset(new fstream(cacheGSControlName.c_str(),
                                        std::ios::in));
      cacheGSBinary.reset(new fstream(cacheGSBinaryName.c_str(),
                                        std::ios::in));

      bReadGSCache = true;
    } else {
      if (DoesFileExist(cacheGSBinaryName)) {
        throw runtime_error("Inconsistent cache state, there is no text control file: '" +
                            cacheGSControlName + "' but there is binary data file: '" +
                            cacheGSBinaryName + "'");
      }
    // No cache => create new file
      cacheGSControl.reset(new fstream(cacheGSControlName.c_str(),
                                        std::ios::trunc | std::ios::out));
      cacheGSBinary.reset(new fstream(cacheGSBinaryName.c_str(),
                                        std::ios::trunc | std::ios::out));
      bWriteGSCache = true;
    }

    cacheGSControl->exceptions(std::ios::badbit);
    cacheGSBinary->exceptions(std::ios::badbit);

    /*
     * If the cache exists, it should be read before ReadData() is called.
     */
    if (!bWriteGSCache) {
      config.Read(*cacheGSControl, *cacheGSBinary, cacheDataSetQty);
    }
  }

  config.ReadDataset();

  if (bReadGSCache) {
    // Let's check the number of data entries, must exactly coincide with
    // what was used to create the cache!
    if (config.GetOrigDataQty() != cacheDataSetQty) {
      stringstream err;
      err << "The number of entries in the file, or the maximum number "
          << "of data elements don't match the value in the cache file: "
          << cacheDataSetQty;
      throw runtime_error(err.str());
    }
  }

  /*
   * Yet, if we need to create a new cache file, we must write the cache
   * after reading the data set.
   */
  if (bWriteGSCache) {
    config.Write(*cacheGSControl, *cacheGSBinary);
  }



  MemUsage  mem_usage_measure;


  std::vector<std::string>          MethDescStr;
  std::vector<std::string>          MethParams;
  vector<double>                    MemUsage;

  vector<vector<MetaAnalysis*>> ExpResRange(config.GetRange().size(),
                                                vector<MetaAnalysis*>(MethodsDesc.size()));
  vector<vector<MetaAnalysis*>> ExpResKNN(config.GetKNN().size(),
                                              vector<MetaAnalysis*>(MethodsDesc.size()));

  GoldStandardManager<dist_t> managerGS(config);



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

    // SelectTestSet must go before managerGS.Compute()!!!

    if (bReadGSCache) {
      size_t cacheTestId = 0;
      managerGS.Read(*cacheGSControl, *cacheGSBinary,
                     config.GetTotalQueryQty(), cacheTestId);
      if (cacheTestId != TestSetId) {
        stringstream err;
        err << "Perhaps, the input file is corrput (or is incompatible with "
            << "program parameters), expect test set id=" << TestSetId
            << "but obtained " << cacheTestId;
        throw runtime_error(err.str());
      }
    } else {
      managerGS.Compute(MaxCacheGSQty);
      if (bWriteGSCache) {
        LOG(LIB_INFO) << "Saving the cache, at most: " << MaxCacheGSQty << " entries";
        managerGS.Write(*cacheGSControl, *cacheGSBinary, TestSetId, MaxCacheGSQty);
      }
    }

    LOG(LIB_INFO) << ">>>> Test set id: " << TestSetId << " (set qty: " << config.GetTestSetToRunQty() << ")";

    //ReportIntrinsicDimensionality("Main data set" , *config.GetSpace(), config.GetDataObjects());

    vector<shared_ptr<Index<dist_t>>>  IndexPtrs;
    vector<bool>                       isNewIndex;

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

        isNewIndex.push_back(bCreateNew);
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

      Experiments<dist_t>::RunAll(true /* print info */,
                                      ThreadTestQty, 
                                      TestSetId,
                                      managerGS,
                                      ExpResRange, ExpResKNN,
                                      config, 
                                      IndexPtrs, isNewIndex,
                                      MethodsDesc);


    } catch (const std::exception& e) {
      LOG(LIB_ERROR) << "Exception: " << e.what();
      bFail = true;
    } catch (...) {
      LOG(LIB_ERROR) << "Unknown exception";
      bFail = true;
    }

    if (bFail) {
      LOG(LIB_FATAL) << "Failure due to an exception!";
    }
  }

  for (auto it = MethDescStr.begin(); it != MethDescStr.end(); ++it) {
    size_t MethNum = it - MethDescStr.begin();

    // Don't overwrite file after we output data at least for one method!
    bool DoAppendHere = DoAppend || MethNum;

    string Print, Data, Header;

    for (size_t i = 0; i < config.GetRange().size(); ++i) {
      MetaAnalysis* res = ExpResRange[i][MethNum];

      ProcessResults(config, *res, MethDescStr[MethNum], MethParams[MethNum], Print, Header, Data);
      LOG(LIB_INFO) << "Range: " << config.GetRange()[i];
      LOG(LIB_INFO) << Print;
      LOG(LIB_INFO) << "Data: " << Header << Data;

      if (!ResFilePrefix.empty()) {
        stringstream str;
        str << ResFilePrefix << "_r=" << config.GetRange()[i];
        OutData(DoAppendHere, str.str(), Print, Header, Data);
      }

      delete res;
    }

    for (size_t i = 0; i < config.GetKNN().size(); ++i) {
      MetaAnalysis* res = ExpResKNN[i][MethNum];

      ProcessResults(config, *res, MethDescStr[MethNum], MethParams[MethNum], Print, Header, Data);
      LOG(LIB_INFO) << "KNN: " << config.GetKNN()[i];
      LOG(LIB_INFO) << Print;
      LOG(LIB_INFO) << "Data: " << Header << Data;

      if (!ResFilePrefix.empty()) {
        stringstream str;
        str << ResFilePrefix << "_K=" << config.GetKNN()[i];
        OutData(DoAppendHere, str.str(), Print, Header, Data);
      }

      delete res;
    }
  }
}

int main(int ac, char* av[]) {
  // This should be the first function called before

  WallClockTimer timer;
  timer.reset();


  string                LogFile;
  string                DistType;
  string                SpaceType;
  shared_ptr<AnyParams> SpaceParams;
  bool                  DoAppend;
  string                ResFilePrefix;
  unsigned              TestSetQty;
  string                DataFile;
  string                QueryFile;
  string                CacheGSFilePrefix;
  size_t                MaxCacheGSQty;
  unsigned              MaxNumData;
  unsigned              MaxNumQuery;
  vector<unsigned>      knn;
  string                RangeArg;
  unsigned              dimension;
  float                 eps = 0.0;
  unsigned              ThreadTestQty;

  vector<shared_ptr<MethodWithParams>>        MethodsDesc;

  ParseCommandLine(ac, av, LogFile,
                       DistType,
                       SpaceType,
                       SpaceParams,
                       dimension,
                       ThreadTestQty,
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

  initLibrary(LogFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, LogFile.c_str());

  LOG(LIB_INFO) << "Program arguments are processed";

  ToLower(DistType);

  if (DIST_TYPE_INT == DistType) {
    RunExper<int>(MethodsDesc,
                  SpaceType,
                  SpaceParams,
                  dimension,
                  ThreadTestQty,
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
                  RangeArg
                 );
  } else if (DIST_TYPE_FLOAT == DistType) {
    RunExper<float>(MethodsDesc,
                  SpaceType,
                  SpaceParams,
                  dimension,
                  ThreadTestQty,
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
                  RangeArg
                 );
  } else if (DIST_TYPE_DOUBLE == DistType) {
    RunExper<double>(MethodsDesc,
                  SpaceType,
                  SpaceParams,
                  dimension,
                  ThreadTestQty,
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
                  RangeArg
                 );
  } else {
    LOG(LIB_FATAL) << "Unknown distance value type: " << DistType;
  }

  timer.split();
  LOG(LIB_INFO) << "Time elapsed = " << timer.elapsed() / 1e6;
  LOG(LIB_INFO) << "Finished at " << LibGetCurrentTime();

  return 0;
}
