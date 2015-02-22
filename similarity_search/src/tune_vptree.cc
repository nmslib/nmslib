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

#include <boost/program_options.hpp>

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
#include "method/proj_vptree.h"
#include "method/permutation_vptree.h"
#include "method/perm_bin_vptree.h"
#include "logging.h"
#include "spacefactory.h"
#include "methodfactory.h"

#include "meta_analysis.h"
#include "params.h"

//#define DETAILED_LOG_INFO 1

using namespace similarity;

using std::vector;
using std::map;
using std::make_pair;
using std::string;
using std::stringstream;

namespace po = boost::program_options;

static void Usage(const char *prog,
                  const po::options_description& desc) {
    std::cout << prog << std::endl
              << desc << std::endl;
}

template <typename dist_t>
void GetOptimalAlphas(ExperimentConfig<dist_t>&     config, 
                      const string&                 SpaceType,
                      const string&                 methodName,
                      float                         DesiredRecall,
                      AnyParams                     MethPars, 
                      float                         StepFactor, 
                      float                         alpha_left_base,
                      float                         alpha_right_base,
                      vector<shared_ptr<GoldStandardManager<dist_t>>>&  
                                                    vManagerGS,
                      vector<shared_ptr<Index<dist_t>>>&
                                                    vIndexForAllSetsPtrs,
                      float& recall, float& time_best, float& impr_best,
                      float& alpha_left_best, unsigned exp_left,
                      float& alpha_right_best, unsigned exp_right,
                      unsigned MaxIter,
                      unsigned MaxRecDepth,
                      int StepN,
                      unsigned maxCacheGSQty,
                      unsigned recLevel) {
  if (recLevel >= MaxRecDepth) {
    LOG(LIB_INFO) << "Reached the maximum recursion level: " << recLevel;
    return;
  }

  LOG(LIB_INFO) << "================================================================";
  LOG(LIB_INFO) << ALPHA_LEFT_PARAM << ": " << alpha_left_base << " " << ALPHA_RIGHT_PARAM << ": " << alpha_right_base;
  LOG(LIB_INFO) << EXP_LEFT_PARAM << ": " << exp_left << " " << EXP_RIGHT_PARAM ": " << exp_right;
  LOG(LIB_INFO) << "================================================================";

  MethPars.AddChangeParam(EXP_LEFT_PARAM, exp_left);
  MethPars.AddChangeParam(EXP_RIGHT_PARAM, exp_right);

  for (unsigned iter = 0; iter < MaxIter; ++iter) {
    LOG(LIB_INFO) << "Iteration: " << iter << " Level: " << recLevel << " StepFactor: " << StepFactor;
    double MinRecall = 1.0;
    double MaxRecall = 0;
      
    for (int left = -StepN; left < StepN; ++left) {
      for (int right = -StepN; right < StepN; ++right) {
        float alphaLeftCurr = alpha_left_base * pow(StepFactor, left);
        float alphaRightCurr = alpha_right_base * pow(StepFactor, right);
        MethPars.AddChangeParam(ALPHA_LEFT_PARAM, alphaLeftCurr);
        MethPars.AddChangeParam(ALPHA_RIGHT_PARAM, alphaRightCurr);

#ifdef DETAILED_LOG_INFO
        LOG(LIB_INFO) << "left: " << left << " right: " << right;
#endif

        vector<vector<MetaAnalysis*>> ExpResRange(config.GetRange().size(),
                                                vector<MetaAnalysis*>(1));
        vector<vector<MetaAnalysis*>> ExpResKNN(config.GetKNN().size(),
                                              vector<MetaAnalysis*>(1));

        MetaAnalysis    Stat(config.GetTestSetToRunQty());

        // Stat is used exactly only once: for one GetRange() or one GetKNN() (but not both)
        for (size_t i = 0; i < config.GetRange().size(); ++i) {
          ExpResRange[i][0] = &Stat;
        }
        for (size_t i = 0; i < config.GetKNN().size(); ++i) {
          ExpResKNN[i][0] = &Stat;
        }
        for (int TestSetId = 0; TestSetId < config.GetTestSetToRunQty(); ++TestSetId) {
          config.SelectTestSet(TestSetId);
#ifdef DETAILED_LOG_INFO
          LOG(LIB_INFO) << "****************************************************";
          LOG(LIB_INFO) << "*** Test set id: " << TestSetId << 
                            " (set qty: " << config.GetTestSetToRunQty() << ")" << " iteration: " << iter << "\t***";
          LOG(LIB_INFO) << "****************************************************";
#endif
          if (!vManagerGS[TestSetId].get()) {
            vManagerGS[TestSetId].reset(new GoldStandardManager<dist_t>(config));
            vManagerGS[TestSetId]->Compute(maxCacheGSQty); 
          } else {
#ifdef DETAILED_LOG_INFO
            LOG(LIB_INFO) << "Using existing GS for test set id: " << TestSetId;
#endif
          }

          std::shared_ptr<Index<dist_t>> MethodPtr = vIndexForAllSetsPtrs[TestSetId];
          
          if (!MethodPtr.get()) {
            LOG(LIB_INFO) << "Creating a new index, params: " << MethPars.ToString();
            MethodPtr.reset(MethodFactoryRegistry<dist_t>::Instance().
                                           CreateMethod(true, 
                                                        methodName,
                                                        SpaceType, config.GetSpace(), 
                                                        config.GetDataObjects(), 
                                                        MethPars) );
            vIndexForAllSetsPtrs[TestSetId] = MethodPtr;
          } else {
#ifdef DETAILED_LOG_INFO
            LOG(LIB_INFO) << "Reusing an existing index, only reseting params";
#endif
            MethodPtr->SetQueryTimeParams(MethPars);
          }

          vector<shared_ptr<Index<dist_t>>>          IndexPtrs;
          vector<shared_ptr<MethodWithParams>>       MethodsDesc;
          
          IndexPtrs.push_back(MethodPtr);
          MethodsDesc.push_back(shared_ptr<MethodWithParams>(new MethodWithParams(methodName, MethPars)));

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
          Stat.GetImprEfficiencyAvg() > impr_best) {
          recall =    Stat.GetRecallAvg();
          time_best = Stat.GetQueryTimeAvg();
          impr_best = Stat.GetImprEfficiencyAvg();
          alpha_left_best  = alphaLeftCurr; 
          alpha_right_best = alphaRightCurr; 

          LOG(LIB_INFO) << " ************* BETTER EFFICIENCY POINT ******************* ";
          LOG(LIB_INFO) << "iteration: " << iter << " out of " << MaxIter;
          LOG(LIB_INFO) << ALPHA_LEFT_PARAM  << "=" << alpha_left_best  << " " << EXP_LEFT_PARAM  << "=" << exp_left << " "
                           ALPHA_RIGHT_PARAM << "=" << alpha_right_best << " " << EXP_RIGHT_PARAM << "=" << exp_right;
          LOG(LIB_INFO) << "Recall: " << Stat.GetRecallAvg();
          LOG(LIB_INFO) << "Query time: " << Stat.GetQueryTimeAvg();
          LOG(LIB_INFO) << "Impr. in efficiency: " << Stat.GetImprEfficiencyAvg();
          LOG(LIB_INFO) << "Impr. in dist comp:  " << Stat.GetImprDistCompAvg();
          LOG(LIB_INFO) << " ********************************************************** ";

        }
        MinRecall = std::min(MinRecall, Stat.GetRecallAvg());
        MaxRecall = std::max(MaxRecall, Stat.GetRecallAvg());
      }
    }
    LOG(LIB_INFO) << " ********** After iteration statistics ******************** ";
    LOG(LIB_INFO) << " Local: MinRecall: " << MinRecall << " MaxRecall: " << MaxRecall << " Desired: " << DesiredRecall;

    LOG(LIB_INFO) << " Global: best impr. in efficiency: " << impr_best << " Time: "<< time_best << " Recall: " << recall;
    LOG(LIB_INFO) << " Using: " << ALPHA_LEFT_PARAM  << "=" << alpha_left_best  << " " << EXP_LEFT_PARAM  << "=" << exp_left << " "
                                << ALPHA_RIGHT_PARAM << "=" << alpha_right_best << " " << EXP_RIGHT_PARAM << "=" << exp_right;

    // Now let's see, if we need to increase/decrease base alpha levels
    if (MaxRecall < DesiredRecall) {
      // Two situations are possible
      if (recall < DesiredRecall) { 
        // we never got a required recall, let's decrease existing alphas
        // so that recall will go up.
        alpha_left_base  = alpha_left_base / StepFactor;
        alpha_right_base = alpha_right_base / StepFactor;

        LOG(LIB_INFO) << "[CHANGE] max recall < desired recall, setting alpha_left_base=" << alpha_left_base << " alpha_right_base=" << alpha_right_base;
      } else {
        // we encountered the required recall before, but we somehow managed to select
        // large alphas, so that even the maximum recall is below DesiredRecall.
        // If this is the case, let's return to the previously know good point,
        // with a decreased step factor.
        LOG(LIB_INFO) << "[CHANGE] max recall < desired recall, setting alpha_left_base=" << alpha_left_base << " alpha_right_base=" << alpha_right_base;

        GetOptimalAlphas(config, SpaceType, methodName,
                   DesiredRecall,
                   MethPars, 
                   sqrt(StepFactor), alpha_left_best, alpha_right_best,
                   vManagerGS, vIndexForAllSetsPtrs,
                   recall, time_best, impr_best,
                   alpha_left_best, exp_left,
                   alpha_right_best, exp_right,
                   MaxIter, MaxRecDepth, StepN, maxCacheGSQty,
                   recLevel + 1);
        return;
      }
    } else if (MinRecall > DesiredRecall) {
    /* 
     * If we are above the minimum recall, this means we chose alphas two pessimistically.
     * Let's multiply our best values so far by StepFactor.
     */
      alpha_left_base  = alpha_left_best * StepFactor;
      alpha_right_base = alpha_right_best * StepFactor;
      LOG(LIB_INFO) << "[CHANGE] max recall > desired recall, setting alpha_left_base=" << alpha_left_base << " alpha_right_base=" << alpha_right_base;
    } else {
      CHECK(MaxRecall >= DesiredRecall && MinRecall <= DesiredRecall); 
        // Let's return to the previously know good point,
        // with a decreased step factor.

      LOG(LIB_INFO) << "[CHANGE] desired recall is between min and max recall, decreasing factor, setting alpha_left_base=" << alpha_left_best << " alpha_right_base=" << alpha_right_best;

        GetOptimalAlphas(config, SpaceType, methodName,
                   DesiredRecall,
                   MethPars, 
                   sqrt(StepFactor), alpha_left_best, alpha_right_best,
                   vManagerGS, vIndexForAllSetsPtrs,
                   recall, time_best, impr_best,
                   alpha_left_best, exp_right,
                   alpha_right_best, exp_right,
                   MaxIter, MaxRecDepth, StepN, maxCacheGSQty,
                   recLevel + 1);
        return;
    }
  }

  LOG(LIB_INFO) << "Exhausted " << MaxIter << " iterations";
}

template <typename dist_t>
void GetOptimalAlphas(ExperimentConfig<dist_t>& config, 
                      const string&   SpaceType,
                      const string&   methodName,
                      AnyParams       AllParams, 
                      float& recall, float& time_best, float& impr_best,
                      float& alpha_left_best,  unsigned exp_left,
                      float& alpha_right_best, unsigned exp_right,
                      unsigned               MaxIter,
                      unsigned               MaxRecDepth,
                      unsigned               StepN,
                      float                  FullFactor,
                      unsigned               maxCacheGSQty) {
  time_best = std::numeric_limits<float>::max();
  impr_best = 0;
  recall = 0;

  LOG(LIB_INFO) << EXP_LEFT_PARAM << ": " << exp_left << " " << EXP_RIGHT_PARAM ": " << exp_right;

  AnyParamManager pmgr(AllParams);

  LOG(LIB_INFO) << "Method parameters: " << AllParams.ToString();

  float DesiredRecall = 0;

  pmgr.GetParamRequired("desiredRecall", DesiredRecall);

  vector<shared_ptr<GoldStandardManager<dist_t>>>  vManagerGS(config.GetTestSetToRunQty());
  vector<shared_ptr<Index<dist_t>>>                vIndexForAllSetsPtrs(config.GetTestSetToRunQty());

  GetOptimalAlphas(config, SpaceType, methodName,
                   DesiredRecall,
                   pmgr.ExtractParametersExcept({"desiredRecall"}), 
                   pow(FullFactor, 1.0/StepN), alpha_left_best, alpha_right_best,
                   vManagerGS, vIndexForAllSetsPtrs,
                   recall, time_best, impr_best,
                   alpha_left_best, exp_right,
                   alpha_right_best, exp_right,
                   MaxIter, MaxRecDepth, StepN, maxCacheGSQty,
                   0 /* recLevel */);
}

template <typename dist_t>
void RunExper(unsigned AddRestartQty, 
             shared_ptr<MethodWithParams>  Method,
             const string&                  SpaceType,
             const shared_ptr<AnyParams>&   SpaceParams,
             unsigned                       TestSetQty,
             const string&                  DataFile,
             const string&                  QueryFile,
             unsigned                       MaxNumData,
             unsigned                       MaxNumQuery,
             vector<unsigned>               knnAll,
             float                          eps,
             const string&                  RangeArg,
             const string&                  ResFile,
             unsigned                       MinExp,
             unsigned                       MaxExp,
             unsigned                       MaxIter,
             unsigned                       MaxRecDepth,
             unsigned                       StepN,
             float                          FullFactor,
             unsigned                       maxCacheGSQty
)
{
  vector<dist_t> rangeAll;

  if (!RangeArg.empty()) {
    if (!SplitStr(RangeArg, rangeAll, ',')) {
      LOG(LIB_FATAL) << "Wrong format of the range argument: '" << RangeArg << "' Should be a list of coma-separated values.";
    }
  }

  vector<string>  vAllowedMeth = {METH_VPTREE, METH_PROJ_VPTREE, METH_PERMUTATION_VPTREE, METH_PERM_BIN_VPTREE};
  string          allowedMethList;

  for (string s: vAllowedMeth) allowedMethList += s + " ";

  const string methodName = Method->methName_;

  bool ok = false;
  for (string s: vAllowedMeth) {
    if (methodName  == s) {
      ok = true;
      break;
    }
  }

  if (!ok) {
    LOG(LIB_FATAL) << "Wrong method name, " << 
                      "you should specify only a single method from the list: " << allowedMethList;
  }

  LOG(LIB_INFO) << "We are going to tune parameters for " << methodName;

  const AnyParams& MethPars = Method->methPars_;

  static  std::random_device          rd;
  static  std::mt19937                engine(rd());
  static  std::normal_distribution<>  normGen(0.0f, log(FullFactor));

  AnyParamManager pmgr(MethPars);

  float DesiredRecall = 0;

  pmgr.GetParamRequired("desiredRecall", DesiredRecall);

  try {

    if (!MaxExp) throw runtime_error("MaxExp can't be zero!");
    if (MaxExp < MinExp) throw runtime_error("MaxExp can't be < MinExp!");

    if (rangeAll.size() + knnAll.size() != 1) {
      LOG(LIB_FATAL) << "You need to specify exactly one range or one knn search!";
    }

    unique_ptr<ExperimentConfig<dist_t>>  config;

    for (unsigned i = 0; i < rangeAll.size(); ++i) {
      vector<dist_t>      range;
      vector<unsigned>    knn;

      range.push_back(rangeAll[i]);

      Space<dist_t>*  pSpace = SpaceFactoryRegistry<dist_t>::Instance().CreateSpace(SpaceType, *SpaceParams);
      // Note that space will be deleted by the destructor of ExperimentConfig
      config.reset(new ExperimentConfig<dist_t>(pSpace,
                                      DataFile, QueryFile, TestSetQty,
                                      MaxNumData, MaxNumQuery,
                                      0, knn, eps, range));
    }

    for (unsigned i = 0; i < knnAll.size(); ++i) {
      vector<dist_t>      range;
      vector<unsigned>    knn;

      knn.push_back(knnAll[i]);

      Space<dist_t>*  pSpace = SpaceFactoryRegistry<dist_t>::Instance().CreateSpace(SpaceType, *SpaceParams);
      // Note that space will be deleted by the destructor of ExperimentConfig
      config.reset(new ExperimentConfig<dist_t>(pSpace,
                                      DataFile, QueryFile, TestSetQty,
                                      MaxNumData, MaxNumQuery,
                                      0, knn, eps, range));
    }

    CHECK(config.get());

    config->ReadDataset();

    float recall = 0, time_best = 0, impr_best = -1, alpha_left = 0, alpha_right = 0; 
    unsigned exp_left = 0, exp_right = 0;

    for (unsigned ce = MinExp; ce <= MaxExp; ++ce)
    for (unsigned k = 0; k < 1 + AddRestartQty; ++k) {
      unsigned expLeft = ce, expRight = ce;
      float recall_loc, time_best_loc, impr_best_loc, 
            alpha_left_loc = 1.0, alpha_right_loc = 1.0; // These are initial values

      if (k > 0) {
        // Let's do some random normal fun
        alpha_left_loc = exp(normGen(engine));
        alpha_right_loc = exp(normGen(engine));
        LOG(LIB_INFO) << " RANDOM STARTING POINTS: " << alpha_left_loc << " " << alpha_right_loc;
      } 

      GetOptimalAlphas(*config, SpaceType, methodName, MethPars, 
                     recall_loc, 
                     time_best_loc, impr_best_loc,
                     alpha_left_loc, expLeft, alpha_right_loc, expRight,
                     MaxIter, MaxRecDepth, StepN, FullFactor, maxCacheGSQty);

      if (impr_best_loc > impr_best) {
        recall = recall_loc; 
        time_best = time_best_loc; 
        impr_best = impr_best_loc;
        alpha_left = alpha_left_loc; 
        alpha_right = alpha_right_loc;
        exp_left = expLeft;
        exp_right = expRight;
      }
    }

    stringstream  bestParams;
    bestParams << ALPHA_LEFT_PARAM << "=" << alpha_left << "," << ALPHA_RIGHT_PARAM << "=" << alpha_right << ","
                 << EXP_LEFT_PARAM   << "=" << exp_left   << "," << EXP_RIGHT_PARAM   << "=" << exp_right;

    LOG(LIB_INFO) << "Optimization results";
    if (!knnAll.empty()) {
      LOG(LIB_INFO) << "K: "  << knnAll[0];
    } else {
      LOG(LIB_INFO) << "Range: "  << rangeAll[0];
    }
    LOG(LIB_INFO) << "Recall: " << recall;
    LOG(LIB_INFO) << "Best time: " << time_best;
    LOG(LIB_INFO) << "Best impr. eff.: " << impr_best;
    LOG(LIB_INFO) << "alpha_left: " << alpha_left;
    LOG(LIB_INFO) << "exp_left: " << exp_left;
    LOG(LIB_INFO) << "alpha_right: " << alpha_right;
    LOG(LIB_INFO) << "exp_right: " << exp_right;
    LOG(LIB_INFO) << "optimal parameters: " << bestParams.str();

    if (recall < DesiredRecall) {
      LOG(LIB_FATAL) << "Failed to get the desired recall!";
    }

    if (!ResFile.empty()) {
      ofstream out(ResFile, ios::trunc);

      if (!out) {
        LOG(LIB_FATAL) << "Can't open file: '" << ResFile << "' for writing";
      }

      out << bestParams.str() << endl;
      out.close();
    }

  } catch (const std::exception& e) {
    LOG(LIB_FATAL) << "Exception: " << e.what();
  } catch (...) {
    LOG(LIB_FATAL) << "Unknown exception";
  }
}

void ParseCommandLineForTuning(int argc, char*argv[],
                      string&                 LogFile,
                      string&                 DistType,
                      string&                 SpaceType,
                      shared_ptr<AnyParams>&  SpaceParams,
                      string&                 ResFile,
                      unsigned&               TestSetQty,
                      string&                 DataFile,
                      string&                 QueryFile,
                      size_t&                 maxCacheGSQty,
                      unsigned&               MaxNumData,
                      unsigned&               MaxNumQuery,
                      vector<unsigned>&       knn,
                      float&                  eps,
                      string&                 RangeArg,
                      unsigned&               MinExp,   
                      unsigned&               MaxExp,   
                      unsigned&               MaxIter,   
                      unsigned&               MaxRecDepth,
                      unsigned&               StepN,      
                      float&                  FullFactor,
                      unsigned&               addRestartQty,
                      shared_ptr<MethodWithParams>& pars) {
  knn.clear();
  RangeArg.clear();

  string          methParams;
  string          knnArg;
  // Using double due to an Intel's bug with __builtin_signbit being undefined for float
  double          epsTmp;
  double          fullFactorTmp;

  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
    ("help,h", "produce help message")
    ("spaceType,s",     po::value<string>(&SpaceType)->required(),
                        "space type, e.g., l1, l2, lp:p=0.5")
    ("distType",        po::value<string>(&DistType)->default_value(DIST_TYPE_FLOAT),
                        "distance value type: int, float, double")
    ("dataFile,i",      po::value<string>(&DataFile)->required(),
                        "input data file")
    ("maxNumData",      po::value<unsigned>(&MaxNumData)->default_value(0),
                        "if non-zero, only the first maxNumData elements are used")
    ("queryFile,q",     po::value<string>(&QueryFile)->default_value(""),
                        "query file")
    ("maxCacheGSQty",   po::value<size_t>(&maxCacheGSQty)->default_value(1000),
                       "a maximum number of gold standard entries to compute/cache")
    ("logFile,l",       po::value<string>(&LogFile)->default_value(""),
                        "log file")
    ("maxNumQuery",     po::value<unsigned>(&MaxNumQuery)->default_value(0),
                        "if non-zero, use maxNumQuery query elements"
                        "(required in the case of bootstrapping)")
    ("testSetQty,b",    po::value<unsigned>(&TestSetQty)->default_value(0),
                        "# of test sets obtained by bootstrapping;"
                        " ignored if queryFile is specified")
    ("knn,k",           po::value< string>(&knnArg),
                        "comma-separated values of K for the k-NN search")
    ("range,r",         po::value<string>(&RangeArg),
                        "comma-separated radii for the range searches")
    ("eps",             po::value<double>(&epsTmp)->default_value(0.0),
                        "the parameter for the eps-approximate k-NN search.")
    ("method,m",        po::value<string>(&methParams)->required(),
                        "one method with comma-separated parameters in the format:\n"
                        "<method name>:<param1>,<param2>,...,<paramK>")
    ("outFile,o",       po::value<string>(&ResFile)->default_value(""),
                        "output file")
    ("minExp",      po::value<unsigned>(&MinExp)->default_value(1),
                    "the minimum exponent in the pruning oracle.")
    ("maxExp",      po::value<unsigned>(&MaxExp)->default_value(2),
                    "the maximum exponent in the pruning oracle.")
    ("maxIter",     po::value<unsigned>(&MaxIter)->default_value(10),
                    "the maximum number of iteration while we are looking for a point where a desired recall can be achieved.")
    ("maxRecDepth", po::value<unsigned>(&MaxRecDepth)->default_value(10),
                    "the maximum recursion in the maximization algorithm (each recursion leads to decrease in the grid search step).")
    ("stepN",       po::value<unsigned>(&StepN)->default_value(2),
                    "each local step of the grid search involves (2StepN+1)^2 mini-iterations.")
    ("addRestartQty",  po::value<unsigned>(&addRestartQty)->default_value(0),
                    "number of *ADDITIONAL* restarts, initial values are selected randomly")
    ("fullFactor",  po::value<double>(&fullFactorTmp)->default_value(8.0),
                    "the maximum factor used in the local grid search (i.e., if (A, B) is a starting point for the grid search, the first element will be in the range: [A/Factor,A*Factor], while the second element will be in the range [B/Factor,B*Factor]. In the beginning, Factor==FullFactor, but it gradually decreases as the algorithm converges.")
    ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, ProgOptDesc), vm);
    po::notify(vm);
  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    LOG(LIB_FATAL) << e.what();
  }

  eps = epsTmp;
  FullFactor = fullFactorTmp;

  if (vm.count("help")  ) {
    Usage(argv[0], ProgOptDesc);
    exit(0);
  }

  ToLower(DistType);
  ToLower(SpaceType);
  
  try {
    {
      vector<string> SpaceDesc;
      string str = SpaceType;
      ParseSpaceArg(str, SpaceType, SpaceDesc);
      SpaceParams = shared_ptr<AnyParams>(new AnyParams(SpaceDesc));
    }

    string          MethName;
    vector<string>  MethodDesc;
    ParseMethodArg(methParams, MethName, MethodDesc);
    pars = shared_ptr<MethodWithParams>(new MethodWithParams(MethName, MethodDesc));
    
    if (vm.count("knn")) {
      if (!SplitStr(knnArg, knn, ',')) {
        Usage(argv[0], ProgOptDesc);
        LOG(LIB_FATAL) << "Wrong format of the KNN argument: '" << knnArg;
      }
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

    if (!MaxNumQuery && QueryFile.empty()) {
      LOG(LIB_FATAL) << "Set a positive # of queries or specify a query file!"; 
    }
  } catch (const exception& e) {
    LOG(LIB_FATAL) << "Exception: " << e.what();
  }
}

int main(int ac, char* av[]) {
  WallClockTimer timer;
  timer.reset();

  string                  LogFile;
  string                  DistType;
  string                  SpaceType;
  shared_ptr<AnyParams>   SpaceParams;
  string                  ResFile;
  unsigned                TestSetQty;
  string                  DataFile;
  string                  QueryFile;
  size_t                  MaxCacheGSQty;
  unsigned                MaxNumData;
  unsigned                MaxNumQuery;
  vector<unsigned>        knn;
  string                  RangeArg;
  float                   eps;
  shared_ptr<MethodWithParams> Method;

  unsigned MinExp;
  unsigned MaxExp;
  unsigned MaxIter;
  unsigned MaxRecDepth;
  unsigned StepN;
  float    FullFactor;
  unsigned AddRestartQty;

  ParseCommandLineForTuning(ac, av, 
                       LogFile,
                       DistType,
                       SpaceType,
                       SpaceParams,
                       ResFile,
                       TestSetQty,
                       DataFile,
                       QueryFile,
                       MaxCacheGSQty,
                       MaxNumData,
                       MaxNumQuery,
                       knn,
                       eps,
                       RangeArg,
                       MinExp,
                       MaxExp,
                       MaxIter,
                       MaxRecDepth,
                       StepN,
                       FullFactor,
                       AddRestartQty,
                       Method);

  initLibrary(LogFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, LogFile.c_str());

  ToLower(DistType);

  if (DIST_TYPE_INT == DistType) {
    RunExper<int>(AddRestartQty, Method,
                  SpaceType,
                  SpaceParams,
                  TestSetQty,
                  DataFile,
                  QueryFile,
                  MaxNumData,
                  MaxNumQuery,
                  knn,
                  eps,
                  RangeArg,
                  ResFile,
                  MinExp,
                  MaxExp,
                  MaxIter,
                  MaxRecDepth,
                  StepN,
                  FullFactor,
                  MaxCacheGSQty 
                 );
  } else if (DIST_TYPE_FLOAT == DistType) {
    RunExper<float>(AddRestartQty, Method,
                  SpaceType,
                  SpaceParams,
                  TestSetQty,
                  DataFile,
                  QueryFile,
                  MaxNumData,
                  MaxNumQuery,
                  knn,
                  eps,
                  RangeArg,
                  ResFile,
                  MinExp,
                  MaxExp,
                  MaxIter,
                  MaxRecDepth,
                  StepN,
                  FullFactor,
                  MaxCacheGSQty
                 );
  } else if (DIST_TYPE_DOUBLE == DistType) {
    RunExper<double>(AddRestartQty, Method,
                  SpaceType,
                  SpaceParams,
                  TestSetQty,
                  DataFile,
                  QueryFile,
                  MaxNumData,
                  MaxNumQuery,
                  knn,
                  eps,
                  RangeArg,
                  ResFile,
                  MinExp,
                  MaxExp,
                  MaxIter,
                  MaxRecDepth,
                  StepN,
                  FullFactor,
                  MaxCacheGSQty
                 );
  } else {
    LOG(LIB_FATAL) << "Unknown distance value type: " << DistType;
  }

  timer.split();
  LOG(LIB_INFO) << "Time elapsed = " << timer.elapsed() / 1e6;
  LOG(LIB_INFO) << "Finished at " << LibGetCurrentTime();

  return 0;
}
