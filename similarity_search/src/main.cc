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

#include "global.h"
#include "utils.h"
#include "memory.h"
#include "ztimer.h"
#include "experiments.h"
#include "experimentconf.h"
#include "space.h"
#include "spacefactory.h"
#include "logging.h"
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

  LOG(INFO) << "DoAppend? " << DoAppend;

  std::ofstream   OutFileData(FileNameData.c_str(),
                              (DoAppend ? std::ios::app : (std::ios::trunc | std::ios::out)));

  if (!OutFileData) {
    LOG(FATAL) << "Cannot create output file: '" << FileNameData << "'";
  }
  OutFileData.exceptions(std::ios::badbit);

  std::ofstream   OutFileRep(FileNameRep.c_str(),
                              (DoAppend ? std::ios::app : (std::ios::trunc | std::ios::out)));

  if (!OutFileRep) {
    LOG(FATAL) << "Cannot create output file: '" << FileNameRep << "'";
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

  Header << "MethodName\tRecall\tRelPosError\tNumCloser\tQueryTime\tDistComp\tImprEfficiency\tImprDistComp\tMem\tMethodParams" << std::endl;

  Data << "\"" << MethDescStr << "\"\t";
  Data << ExpRes.GetRecallAvg() << "\t";
  Data << ExpRes.GetRelPosErrorAvg() << "\t";
  Data << ExpRes.GetNumCloserAvg() << "\t";
  Data << ExpRes.GetQueryTimeAvg() << "\t";
  Data << ExpRes.GetDistCompAvg() << "\t";
  Data << ExpRes.GetImprEfficiencyAvg() << "\t";
  Data << ExpRes.GetImprDistCompAvg() << "\t";
  Data << size_t(ExpRes.GetMemAvg()) << "\t";
  Data << "\"" << MethParamStr << "\"";
  Data << std::endl;

  Print << std::endl << 
            "===================================" << std::endl;
  Print << MethDescStr << std::endl;
  Print << MethParamStr << std::endl;
  Print << "===================================" << std::endl;
  Print << "# of points: " << config.GetDataObjects().size() << std::endl;
  Print << "# of queries: " << config.GetQueryQty() << std::endl;
  Print << "------------------------------------" << std::endl;
  Print << "Recall:         " << ExpRes.GetRecallAvg()              << " -> " << "[" << ExpRes.GetRecallConfMin() << " " << ExpRes.GetRecallConfMax() << "]" << std::endl;
  Print << "RelPosError:    " << round2(ExpRes.GetRelPosErrorAvg())  << " -> " << "[" << round2(ExpRes.GetRelPosErrorConfMin()) << " \t" << round2(ExpRes.GetRelPosErrorConfMax()) << "]" << std::endl;
  Print << "NumCloser:      " << round2(ExpRes.GetNumCloserAvg())    << " -> " << "[" << round2(ExpRes.GetNumCloserConfMin()) << " \t" << round2(ExpRes.GetNumCloserConfMax()) << "]" << std::endl;
  Print << "------------------------------------" << std::endl;
  Print << "QueryTime:      " << round2(ExpRes.GetQueryTimeAvg())    << " -> " << "[" << round2(ExpRes.GetQueryTimeConfMin()) << " \t" << round2(ExpRes.GetQueryTimeConfMax()) << "]" << std::endl;
  Print << "DistComp:       " << round2(ExpRes.GetDistCompAvg())     << " -> " << "[" << round2(ExpRes.GetDistCompConfMin()) << " \t" << round2(ExpRes.GetDistCompConfMax()) << "]" << std::endl;
  Print << "------------------------------------" << std::endl;
  Print << "ImprEfficiency: " << round2(ExpRes.GetImprEfficiencyAvg()) << " -> " << "["  <<  round2(ExpRes.GetImprEfficiencyConfMin()) << " \t" << round2(ExpRes.GetImprEfficiencyConfMax()) << "]" << std::endl;
  Print << "ImprDistComp:   " << round2(ExpRes.GetImprDistCompAvg()) << " -> " << "[" << round2(ExpRes.GetImprDistCompAvg()) << " \t"<< round2(ExpRes.GetImprDistCompConfMax()) << "]" << std::endl;
  Print << "------------------------------------" << std::endl;
  Print << "Memory Usage:   " << round2(ExpRes.GetMemAvg()) << " MB" << std::endl;
  Print << "------------------------------------" << std::endl;

  PrintStr  = Print.str();
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
             unsigned                     MaxNumData,
             unsigned                     MaxNumQuery,
             const                        vector<unsigned>& knn,
             const                        float eps,
             const string&                RangeArg
)
{
  LOG(INFO) << "### Append? : "       << DoAppend;
  LOG(INFO) << "### OutFilePrefix : " << ResFilePrefix;
  vector<dist_t> range;

  bool bFail = false;


  if (!RangeArg.empty()) {
    if (!SplitStr(RangeArg, range, ',')) {
      LOG(FATAL) << "Wrong format of the range argument: '" << RangeArg << "' Should be a list of coma-separated values.";
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
      ExpResRange[i][MethNum] = new MetaAnalysis(config.GetTestSetQty());
    }
    for (size_t i = 0; i < config.GetKNN().size(); ++i) {
      ExpResKNN[i][MethNum] = new MetaAnalysis(config.GetTestSetQty());
    }
  }


  for (int TestSetId = 0; TestSetId < config.GetTestSetQty(); ++TestSetId) {
    config.SelectTestSet(TestSetId);

    LOG(INFO) << ">>>> Test set id: " << TestSetId << " (set qty: " << config.GetTestSetQty() << ")";

    ReportIntrinsicDimensionality("Main data set" , *config.GetSpace(), config.GetDataObjects());

    vector<shared_ptr<Index<dist_t>>>  IndexPtrs;

    try {
      
      for (const auto& methElem: MethodsDesc) {
        MethNum = &methElem - &MethodsDesc[0];
        
        const string& MethodName  = methElem->methName_;
        const AnyParams& MethPars = methElem->methPars_;
        const string& MethParStr = MethPars.ToString();

        LOG(INFO) << ">>>> Index type : " << MethodName;
        LOG(INFO) << ">>>> Parameters: " << MethParStr;
        const double vmsize_before = mem_usage_measure.get_vmsize();


        WallClockTimer wtm;
        CPUTimer       ctm;

        wtm.reset();
        ctm.reset();
        
        bool bCreateNew = true;
        
        if (MethNum && MethodName == MethodsDesc[MethNum-1]->methName_) {
          vector<string> exceptList = IndexPtrs.back()->GetQueryTimeParamNames();
          
          if (MethodsDesc[MethNum-1]->methPars_.equalsIgnoreInList(MethPars, exceptList)) {
            bCreateNew = false;
          }
        }

        LOG(INFO) << (bCreateNew ? "Creating a new index":"Using a previosuly created index");

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

        LOG(INFO) << "==============================================";
        wtm.split();
        ctm.split();

        const double vmsize_after = mem_usage_measure.get_vmsize();

        const double data_size = DataSpaceUsed(config.GetDataObjects()) / 1024.0 / 1024.0;

        const double TotalMemByMethod = vmsize_after - vmsize_before + data_size;

        LOG(INFO) << ">>>> Process memory usage: " << vmsize_after << " MBs";
        LOG(INFO) << ">>>> Virtual memory usage: " << TotalMemByMethod << " MBs";
        LOG(INFO) << ">>>> Data size:            " << data_size << " MBs";
        LOG(INFO) << ">>>> Time elapsed:         " << (wtm.elapsed()/double(1e6)) << " sec";
        LOG(INFO) << ">>>> CPU time elapsed:     " << (ctm.userelapsed()/double(1e6)) << " sec";
        LOG(INFO) << ">>>> System time elapsed:  " << (ctm.systemelapsed()/double(1e6)) << " sec";


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
                                      ExpResRange, ExpResKNN,
                                      config, 
                                      IndexPtrs, MethodsDesc);


    } catch (const std::exception& e) {
      LOG(ERROR) << "Exception: " << e.what();
      bFail = true;
    } catch (...) {
      LOG(ERROR) << "Unknown exception";
      bFail = true;
    }

    if (bFail) {
      LOG(FATAL) << "Failure due to an exception!";
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
      LOG(INFO) << "Range: " << config.GetRange()[i];
      LOG(INFO) << Print;
      LOG(INFO) << "Data: " << Header << Data;

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
      LOG(INFO) << "KNN: " << config.GetKNN()[i];
      LOG(INFO) << Print;
      LOG(INFO) << "Data: " << Header << Data;

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
  WallClockTimer timer;
  timer.reset();

  string                DistType;
  string                SpaceType;
  shared_ptr<AnyParams> SpaceParams;
  bool                  DoAppend;
  string                ResFilePrefix;
  unsigned              TestSetQty;
  string                DataFile;
  string                QueryFile;
  unsigned              MaxNumData;
  unsigned              MaxNumQuery;
  vector<unsigned>      knn;
  string                RangeArg;
  unsigned              dimension;
  float                 eps = 0.0;
  unsigned              ThreadTestQty;

  vector<shared_ptr<MethodWithParams>>        MethodsDesc;

  ParseCommandLine(ac, av,
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
                       MaxNumData,
                       MaxNumQuery,
                       knn,
                       eps,
                       RangeArg,
                       MethodsDesc);

  ToLower(DistType);

  if ("int" == DistType) {
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
                  MaxNumData,
                  MaxNumQuery,
                  knn,
                  eps,
                  RangeArg
                 );
  } else if ("float" == DistType) {
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
                  MaxNumData,
                  MaxNumQuery,
                  knn,
                  eps,
                  RangeArg
                 );
  } else if ("double" == DistType) {
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
                  MaxNumData,
                  MaxNumQuery,
                  knn,
                  eps,
                  RangeArg
                 );
  } else {
    LOG(FATAL) << "Unknown distance value type: " << DistType;
  }

  timer.split();
  LOG(INFO) << "Time elapsed = " << timer.elapsed() / 1e6;
  LOG(INFO) << "Finished at " << CurrentTime();

  return 0;
}
