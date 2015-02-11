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
#include <memory>
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
#include "index.h"
#include "method/vptree.h"
#include "logging.h"
#include "spacefactory.h"
#include "methodfactory.h"

#include "meta_analysis.h"
#include "params.h"

using namespace similarity;

using std::vector;
using std::map;
using std::string;
using std::stringstream;

const unsigned MaxIter             = 20;
const int      StepN               = 7;
const float    FullFactor          = 8.0;
const float    FastIterFactor      = sqrt(sqrt(FullFactor));
const float    SlowIterFactor      = sqrt(FastIterFactor);

template <typename dist_t>
void GetOptimalAlphas(ExperimentConfig<dist_t>& config, 
                      const string& SpaceType,
                      AnyParams AllParams, 
                      float& recall, float& time_best, float& alpha_left_best, float& alpha_right_best) {
  time_best = std::numeric_limits<float>::max();
  alpha_left_best = 0;
  alpha_right_best = 0;
  recall = 0;

  AnyParamManager pmgr(AllParams);

  const float    StepFactor = exp(log(FullFactor)/StepN); 

  float alpha_left_base = 0;
  float alpha_right_base = 0;
  float DesiredRecall = 0;

  pmgr.GetParamRequired("alphaLeft", alpha_left_base); 
  pmgr.GetParamRequired("alphaRight", alpha_right_base); 
  pmgr.GetParamRequired("desiredRecall", DesiredRecall);

  unsigned ChangeAlphaState = 0;

  AnyParams  MethPars = pmgr.ExtractParametersExcept({"desiredRecall"});

  vector<shared_ptr<GoldStandardManager<dist_t>>>  vManagerGS(config.GetTestSetQty());
  vector<shared_ptr<Index<dist_t>>>                vIndexFollAllSetsPtrs(config.GetTestSetQty());

  for (unsigned iter = 0; iter < MaxIter; ++iter) {
    LOG(LIB_INFO) << "Iteration: " << iter << " StepFactor: " << StepFactor;
    double MinRecall = 1.0;
    double MaxRecall = 0;
      
    for (int left = 0; left < StepN; ++left) {
      for (int right = 0; right < StepN; ++right) {
        MethPars.ChangeParam("alphaLeft", alpha_left_base * pow(StepFactor, left - StepN/2));
        MethPars.ChangeParam("alphaRight", alpha_right_base * pow(StepFactor, right - StepN/2));

        LOG(LIB_INFO) << "left: " << (left - StepN/2) << " right: " << (right - StepN/2);

        vector<vector<MetaAnalysis*>> ExpResRange(config.GetRange().size(),
                                                vector<MetaAnalysis*>(1));
        vector<vector<MetaAnalysis*>> ExpResKNN(config.GetKNN().size(),
                                              vector<MetaAnalysis*>(1));

        MetaAnalysis    Stat(config.GetTestSetQty());

        // Stat is used exactly only once: for one GetRange() or one GetKNN() (but not both)
        for (size_t i = 0; i < config.GetRange().size(); ++i) {
          ExpResRange[i][0] = &Stat;
        }
        for (size_t i = 0; i < config.GetKNN().size(); ++i) {
          ExpResKNN[i][0] = &Stat;
        }
        for (int TestSetId = 0; TestSetId < config.GetTestSetQty(); ++TestSetId) {
          config.SelectTestSet(TestSetId);
          LOG(LIB_INFO) << ">>>> Test set id: " << TestSetId << " (set qty: " << config.GetTestSetQty() << ")";
          if (!vManagerGS[TestSetId].get()) {
            vManagerGS[TestSetId].reset(new GoldStandardManager<dist_t>(config));
            vManagerGS[TestSetId]->Compute();
          } else {
            LOG(LIB_INFO) << "Using existing GS for test set id: " << TestSetId;
          }

          std::shared_ptr<Index<dist_t>> MethodPtr = vIndexFollAllSetsPtrs[TestSetId];
          if (!MethodPtr.get()) {
            LOG(LIB_INFO) << "Creating a new index";
            MethodPtr.reset(MethodFactoryRegistry<dist_t>::Instance().
                                           CreateMethod(false, 
                                                        METH_VPTREE, 
                                                        SpaceType, config.GetSpace(), 
                                                        config.GetDataObjects(), 
                                                        MethPars) );
            vIndexFollAllSetsPtrs[TestSetId] = MethodPtr;
          } else {
            LOG(LIB_INFO) << "Reusing an existing index";
          }

          vector<shared_ptr<Index<dist_t>>>          IndexPtrs;
          vector<shared_ptr<MethodWithParams>>       MethodsDesc;
          
          IndexPtrs.push_back(MethodPtr);
          MethodsDesc.push_back(shared_ptr<MethodWithParams>(new MethodWithParams(METH_VPTREE, MethPars)));

          Experiments<dist_t>::RunAll(false /* don't print info */, 1 /* thread */,
                                      TestSetId,
                                      *vManagerGS[TestSetId],
                                      ExpResRange, ExpResKNN,
                                      config, 
                                      IndexPtrs,
                                      MethodsDesc);
        }
        Stat.ComputeAll();
        if (Stat.GetRecallAvg() >= DesiredRecall &&
            Stat.GetQueryTimeAvg() < time_best) {
            recall =    Stat.GetRecallAvg();
            time_best = Stat.GetQueryTimeAvg();

            AnyParamManager pmgr(MethPars);

            /* 
             * We need delete other parameters, otherwise the destructor would
             * complain about unused ones.
             */
            AnyParams  MethPars = pmgr.ExtractParametersExcept({"alphaLeft", "alphaRight"});

            pmgr.GetParamRequired("alphaLeft", alpha_left_best);
            pmgr.GetParamRequired("alphaRight", alpha_right_best);
        }
        LOG(LIB_INFO) << "Recall: " << Stat.GetRecallAvg() << " Query time: " << Stat.GetQueryTimeAvg();
        MinRecall = std::min(MinRecall, Stat.GetRecallAvg());
        MaxRecall = std::max(MaxRecall, Stat.GetRecallAvg());
      }
    }
    LOG(LIB_INFO) << " MinRecall: " << MinRecall << " MaxRecall: " << MaxRecall << " BestTime: "<< time_best;;
    // Now let's see, if we need to increase/decrease base alpha levels
    if (MaxRecall < DesiredRecall) {
      if (ChangeAlphaState == 2) break; // We sufficiently increased alphas, now stop
      alpha_left_base /= FastIterFactor;
      alpha_right_base /= FastIterFactor;
      ChangeAlphaState = 1;
      LOG(LIB_INFO) << "Decreasing alphas by " << FastIterFactor;
    } else if (MinRecall > DesiredRecall) {
      if (ChangeAlphaState == 1) break; // We sufficiently decreased alphas, now stop
      alpha_left_base  *= FastIterFactor;
      alpha_right_base *= FastIterFactor;
      ChangeAlphaState = 2;
      LOG(LIB_INFO) << "Increasing alphas by " << FastIterFactor;
    } else {
      CHECK(MaxRecall >= DesiredRecall && MinRecall <= DesiredRecall); 
      // Continue moving by at a slower pace
      if (ChangeAlphaState == 1) {
        alpha_left_base  /= SlowIterFactor;
        alpha_right_base /= SlowIterFactor;
        LOG(LIB_INFO) << "Decreasing alphas by " << SlowIterFactor;
      } else {
      // If previous 0 == ChangeAlphaState, we prefer to increase alphas 
        ChangeAlphaState = 2;
        alpha_left_base  *= SlowIterFactor;
        alpha_right_base *= SlowIterFactor;
        LOG(LIB_INFO) << "Increasing alphas by " << SlowIterFactor;
      }
    }
  }

  if (recall < DesiredRecall) {
    LOG(LIB_FATAL) << "Failed to get the desired recall using " << MaxIter << " iterations, try to choose different start values for AlphaLeft and AlphaRight";
  }
}

template <typename dist_t>
void RunExper(const vector<shared_ptr<MethodWithParams>>& Methods,
             const string&                  SpaceType,
             const shared_ptr<AnyParams>&   SpaceParams,
             unsigned                       dimension,
             unsigned                       TestSetQty,
             const string&                  DataFile,
             const string&                  QueryFile,
             unsigned                       MaxNumData,
             unsigned                       MaxNumQuery,
             vector<unsigned>               knnAll,
             float                          eps,
             const string&                  RangeArg
)
{
  vector<dist_t> rangeAll;

  if (!RangeArg.empty()) {
    if (!SplitStr(RangeArg, rangeAll, ',')) {
      LOG(LIB_FATAL) << "Wrong format of the range argument: '" << RangeArg << "' Should be a list of coma-separated values.";
    }
  }

  if (Methods.size() != 1 ||
      Methods[0]->methName_ != METH_VPTREE) {
    LOG(LIB_FATAL) << "Should specify only the single method: " << METH_VPTREE;
  }

  const AnyParams& MethPars = Methods[0]->methPars_;

  try {

    for (unsigned i = 0; i < rangeAll.size(); ++i) {
      vector<dist_t>      range;
      vector<unsigned>    knn;

      range.push_back(rangeAll[i]);

      // Note that space will be deleted by the destructor of ExperimentConfig
      ExperimentConfig<dist_t> config(SpaceFactoryRegistry<dist_t>::
                                      Instance().CreateSpace(SpaceType, *SpaceParams),
                                      DataFile, QueryFile, TestSetQty,
                                      MaxNumData, MaxNumQuery,
                                      dimension, knn, eps, range);

      config.ReadDataset();

      float recall, time_best, alpha_left, alpha_right;
      GetOptimalAlphas(config, SpaceType, MethPars, recall, time_best, alpha_left, alpha_right);

      LOG(LIB_INFO) << "Optimization results";
      LOG(LIB_INFO) << "Range: "  << rangeAll[i];
      LOG(LIB_INFO) << "Recall: " << recall;
      LOG(LIB_INFO) << "Best time: " << time_best;
      LOG(LIB_INFO) << "alpha_left: " << alpha_left;
      LOG(LIB_INFO) << "alpha_right: " << alpha_right;
      
    }

    for (unsigned i = 0; i < knnAll.size(); ++i) {
      vector<dist_t>      range;
      vector<unsigned>    knn;

      knn.push_back(knnAll[i]);

      // Note that space will be deleted by the destructor of ExperimentConfig
      ExperimentConfig<dist_t> config(SpaceFactoryRegistry<dist_t>::
                                      Instance().CreateSpace(SpaceType, *SpaceParams),
                                      DataFile, QueryFile, TestSetQty,
                                      MaxNumData, MaxNumQuery,
                                      dimension, knn, eps, range);

      config.ReadDataset();

      float recall, time_best, alpha_left, alpha_right;
      GetOptimalAlphas(config, SpaceType, MethPars, recall, time_best, alpha_left, alpha_right);

      LOG(LIB_INFO) << "Optimization results";
      LOG(LIB_INFO) << "K: "  << knnAll[i];
      LOG(LIB_INFO) << "Recall: " << recall;
      LOG(LIB_INFO) << "Best time: " << time_best;
      LOG(LIB_INFO) << "alpha_left: " << alpha_left;
      LOG(LIB_INFO) << "alpha_right: " << alpha_right;
    }

  } catch (const std::exception& e) {
    LOG(LIB_FATAL) << "Exception: " << e.what();
  } catch (...) {
    LOG(LIB_FATAL) << "Unknown exception";
  }
}

int main(int ac, char* av[]) {
  WallClockTimer timer;
  timer.reset();

  string                  LogFile;
  string                  DistType;
  string                  SpaceType;
  shared_ptr<AnyParams>   SpaceParams;
  bool                    DoAppend;
  string                  ResFilePrefix;
  unsigned                TestSetQty;
  string                  DataFile;
  string                  QueryFile;
  string                  CacheGSFilePrefix;
  unsigned                MaxNumData;
  unsigned                MaxNumQuery;
  vector<unsigned>        knn;
  string                  RangeArg;
  unsigned                dimension;
  unsigned                ThreadTestQty;
  float                   eps;
  vector<shared_ptr<MethodWithParams>> Methods;



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
                       MaxNumData,
                       MaxNumQuery,
                       knn,
                       eps,
                       RangeArg,
                       Methods);

  if (!CacheGSFilePrefix.empty()) {
    LOG(LIB_FATAL) << "Caching of gold standard data is not yet implemented for this utility";
  }


  initLibrary(LogFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, LogFile.c_str());

  ToLower(DistType);

  if ("int" == DistType) {
    RunExper<int>(Methods,
                  SpaceType,
                  SpaceParams,
                  dimension,
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
    RunExper<float>(Methods,
                  SpaceType,
                  SpaceParams,
                  dimension,
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
    RunExper<double>(Methods,
                  SpaceType,
                  SpaceParams,
                  dimension,
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
    LOG(LIB_FATAL) << "Unknown distance value type: " << DistType;
  }

  timer.split();
  LOG(LIB_INFO) << "Time elapsed = " << timer.elapsed() / 1e6;
  LOG(LIB_INFO) << "Finished at " << LibGetCurrentTime();

  return 0;
}
