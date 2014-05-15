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
#include "vptree.h"
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

  for (unsigned iter = 0; iter < MaxIter; ++iter) {
    LOG(INFO) << "Iteration: " << iter << " StepFactor: " << StepFactor;
    double MinRecall = 1.0;
    double MaxRecall = 0;
      
    for (int left = 0; left < StepN; ++left) {
      for (int right = 0; right < StepN; ++right) {
        MethPars.ChangeParam("alphaLeft", alpha_left_base * pow(StepFactor, left - StepN/2));
        MethPars.ChangeParam("alphaRight", alpha_right_base * pow(StepFactor, right - StepN/2));

        LOG(INFO) << "left: " << (left - StepN/2) << " right: " << (right - StepN/2);

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
          LOG(INFO) << ">>>> Test set id: " << TestSetId << " (set qty: " << config.GetTestSetQty() << ")";
          std::shared_ptr<Index<dist_t>> MethodPtr( MethodFactoryRegistry<dist_t>::Instance().
                                           CreateMethod(false, 
                                                        METH_VPTREE, 
                                                        SpaceType, config.GetSpace(), 
                                                        config.GetDataObjects(), 
                                                        MethPars) );

          vector<shared_ptr<Index<dist_t>>>          IndexPtrs;
          vector<shared_ptr<MethodWithParams>>       MethodsDesc;
          
          IndexPtrs.push_back(MethodPtr);
          MethodsDesc.push_back(shared_ptr<MethodWithParams>(new MethodWithParams(METH_VPTREE, MethPars)));

          Experiments<dist_t>::RunAll(false /* don't print info */, 1 /* thread */,
                                      TestSetId,
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
        LOG(INFO) << "Recall: " << Stat.GetRecallAvg() << " Query time: " << Stat.GetQueryTimeAvg();
        MinRecall = std::min(MinRecall, Stat.GetRecallAvg());
        MaxRecall = std::max(MaxRecall, Stat.GetRecallAvg());
      }
    }
    LOG(INFO) << " MinRecall: " << MinRecall << " MaxRecall: " << MaxRecall << " BestTime: "<< time_best;;
    // Now let's see, if we need to increase/decrease base alpha levels
    if (MaxRecall < DesiredRecall) {
      if (ChangeAlphaState == 2) break; // We sufficiently increased alphas, now stop
      alpha_left_base /= FastIterFactor;
      alpha_right_base /= FastIterFactor;
      ChangeAlphaState = 1;
      LOG(INFO) << "Decreasing alphas by " << FastIterFactor;
    } else if (MinRecall > DesiredRecall) {
      if (ChangeAlphaState == 1) break; // We sufficiently decreased alphas, now stop
      alpha_left_base  *= FastIterFactor;
      alpha_right_base *= FastIterFactor;
      ChangeAlphaState = 2;
      LOG(INFO) << "Increasing alphas by " << FastIterFactor;
    } else {
      CHECK(MaxRecall >= DesiredRecall && MinRecall <= DesiredRecall); 
      // Continue moving by at a slower pace
      if (ChangeAlphaState == 1) {
        alpha_left_base  /= SlowIterFactor;
        alpha_right_base /= SlowIterFactor;
        LOG(INFO) << "Decreasing alphas by " << SlowIterFactor;
      } else {
      // If previous 0 == ChangeAlphaState, we prefer to increase alphas 
        ChangeAlphaState = 2;
        alpha_left_base  *= SlowIterFactor;
        alpha_right_base *= SlowIterFactor;
        LOG(INFO) << "Increasing alphas by " << SlowIterFactor;
      }
    }
  }

  if (recall < DesiredRecall) {
    LOG(FATAL) << "Failed to get the desired recall using " << MaxIter << " iterations, try to choose different start values for AlphaLeft and AlphaRight";
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
      LOG(FATAL) << "Wrong format of the range argument: '" << RangeArg << "' Should be a list of coma-separated values.";
    }
  }

  if (Methods.size() != 1 ||
      Methods[0]->methName_ != METH_VPTREE) {
    LOG(FATAL) << "Should specify only the single method: " << METH_VPTREE;
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

      LOG(INFO) << "Optimization results";
      LOG(INFO) << "Range: "  << rangeAll[i];
      LOG(INFO) << "Recall: " << recall;
      LOG(INFO) << "Best time: " << time_best;
      LOG(INFO) << "alpha_left: " << alpha_left;
      LOG(INFO) << "alpha_right: " << alpha_right;
      
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

      LOG(INFO) << "Optimization results";
      LOG(INFO) << "K: "  << knnAll[i];
      LOG(INFO) << "Recall: " << recall;
      LOG(INFO) << "Best time: " << time_best;
      LOG(INFO) << "alpha_left: " << alpha_left;
      LOG(INFO) << "alpha_right: " << alpha_right;
    }

  } catch (const std::exception& e) {
    LOG(FATAL) << "Exception: " << e.what();
  } catch (...) {
    LOG(FATAL) << "Unknown exception";
  }
}

int main(int ac, char* av[]) {
  initLibrary();
  WallClockTimer timer;
  timer.reset();

  string                  DistType;
  string                  SpaceType;
  shared_ptr<AnyParams>   SpaceParams;
  bool                    DoAppend;
  string                  ResFilePrefix;
  unsigned                TestSetQty;
  string                  DataFile;
  string                  QueryFile;
  unsigned                MaxNumData;
  unsigned                MaxNumQuery;
  vector<unsigned>        knn;
  string                  RangeArg;
  unsigned                dimension;
  unsigned                ThreadTestQty;
  float                   eps;
  vector<shared_ptr<MethodWithParams>> Methods;



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
                       Methods);

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
    LOG(FATAL) << "Unknown distance value type: " << DistType;
  }

  timer.split();
  LOG(INFO) << "Time elapsed = " << timer.elapsed() / 1e6;
  LOG(INFO) << "Finished at " << CurrentTime();

  return 0;
}
