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

#ifndef _EXPERIMENTS_H
#define _EXPERIMENTS_H

#include <errno.h>
#include <string.h>

#include <iostream>
#include <vector>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <thread>
#include <mutex>
#include <memory>

#include "global.h"
#include "object.h"
#include "memory.h"
#include "ztimer.h"
#include "utils.h"
#include "experimentconf.h"
#include "space.h"
#include "index.h"
#include "logging.h"
#include "methodfactory.h"
#include "eval_results.h"
#include "meta_analysis.h"
#include "query_creator.h"

/* 
 * If defined, we active the older effectiveness evaluation mode,
 * where effectiveness metrics such as recall were evaluated in a separate step.
 */
//#define SEPARATE_EFFECTIVENESS_EVAL 

namespace similarity {

using std::vector;
using std::shared_ptr;
using std::string;
using std::stringstream;
using std::mutex;
using std::thread;
using std::ref;
using std::lock_guard;
using std::unique_ptr;

template <typename dist_t>
class Experiments {
public:
  typedef Index<dist_t> IndexType;

  static void RunAll(bool LogInfo, 
                     unsigned ThreadTestQty,
                     size_t TestSetId,
                     const GoldStandardManager<dist_t>& managerGS,
                     vector<vector<MetaAnalysis*>>&   ExpResRange,
                     vector<vector<MetaAnalysis*>>&   ExpResKNN,
                     const ExperimentConfig<dist_t>&  config,
                     const vector<shared_ptr<IndexType>>& IndexPtrs,
                     const vector<bool>& isNewIndex,
                     const vector<shared_ptr<MethodWithParams>>& MethodsDesc) {

    if (LogInfo) LOG(LIB_INFO) << ">>>> TestSetId: " << TestSetId;
    if (LogInfo) LOG(LIB_INFO) << ">>>> Will use: "  << ThreadTestQty << " threads in efficiency testing";
    if (LogInfo) config.PrintInfo();

    if (!config.GetRange().empty()) {
      for (size_t i = 0; i < config.GetRange().size(); ++i) {
        const dist_t radius = config.GetRange()[i];
        RangeCreator<dist_t>  cr(radius);
        Execute<RangeQuery<dist_t>, RangeCreator<dist_t>>(LogInfo,
                                                  ThreadTestQty, TestSetId,
                                                  managerGS.GetRangeGS(i),
                                                  ExpResRange[i], config, cr, 
                                                  IndexPtrs, isNewIndex, MethodsDesc);
      }
    }

    if (!config.GetKNN().empty()) {
      for (size_t i = 0; i < config.GetKNN().size(); ++i) {
        const size_t K = config.GetKNN()[i];
        KNNCreator<dist_t>  cr(K, config.GetEPS());
        Execute<KNNQuery<dist_t>, KNNCreator<dist_t>>(LogInfo,
                                              ThreadTestQty, TestSetId,
                                              managerGS.GetKNNGS(i),
                                              ExpResKNN[i], config, cr, 
                                              IndexPtrs, isNewIndex, MethodsDesc);
      }
    }
    if (LogInfo) LOG(LIB_INFO) << "experiment done at " << LibGetCurrentTime();

  }

  template <typename QueryType, typename QueryCreatorType>
  struct  BenchmarkThreadParams {
    BenchmarkThreadParams(
              mutex&                          UpdateStat,
              unsigned                        ThreadQty,
              unsigned                        QueryPart,
              size_t                          TestSetId, 
              std::vector<MetaAnalysis*>&     ExpRes,
              const ExperimentConfig<dist_t>& config,
              const QueryCreatorType&         QueryCreator,
              Index<dist_t>&                  Method,
              unsigned                        MethNum,
              vector<uint64_t>&               SearchTime,
              vector<double>&                 AvgNumDistComp,
              vector<unsigned>&               max_result_size,
              vector<double>&                 avg_result_size,
              vector<uint64_t>&               DistCompQty) :
    UpdateStat_(UpdateStat),
    ThreadQty_(ThreadQty),
    QueryPart_(QueryPart),
    TestSetId_(TestSetId),
    ExpRes_(ExpRes),
    config_(config),
    QueryCreator_(QueryCreator),
    Method_(Method),
    MethNum_(MethNum),
    SearchTime_(SearchTime),

    AvgNumDistComp_(AvgNumDistComp),
    max_result_size_(max_result_size),
    avg_result_size_(avg_result_size),
    DistCompQty_(DistCompQty)
    {}

    mutex&                          UpdateStat_;
    unsigned                        ThreadQty_;
    unsigned                        QueryPart_;
    size_t                          TestSetId_;
    std::vector<MetaAnalysis*>&     ExpRes_;
    const ExperimentConfig<dist_t>& config_;
    const QueryCreatorType&         QueryCreator_;
    Index<dist_t>&                  Method_;
    unsigned                        MethNum_;
    vector<uint64_t>&               SearchTime_;

    vector<double>&                 AvgNumDistComp_;
    vector<unsigned>&               max_result_size_;
    vector<double>&                 avg_result_size_;
    vector<uint64_t>&               DistCompQty_;
#ifndef SEPARATE_EFFECTIVENESS_EVAL
    vector<size_t>                  queryIds;
    vector<unique_ptr<QueryType>>   queries; // queries with results
#endif

  };

  template <typename QueryType, typename QueryCreatorType> 
  struct BenchmarkThread {
    void operator ()(BenchmarkThreadParams<QueryType, QueryCreatorType>& prm) {
      size_t numquery = prm.config_.GetQueryObjects().size();

      WallClockTimer wtm;

      wtm.reset();

      unsigned MethNum = prm.MethNum_;
      unsigned QueryPart = prm.QueryPart_;
      unsigned ThreadQty = prm.ThreadQty_;

      for (size_t q = 0; q < numquery; ++q) {
        if ((q % ThreadQty) == QueryPart) {
          unique_ptr<QueryType> query(prm.QueryCreator_(prm.config_.GetSpace(), 
                                      prm.config_.GetQueryObjects()[q]));
          uint64_t  t1 = wtm.split();
          prm.Method_.Search(query.get());
          uint64_t  t2 = wtm.split();

          {
            lock_guard<mutex> g(prm.UpdateStat_);

            prm.ExpRes_[MethNum]->AddDistComp(prm.TestSetId_, query->DistanceComputations());
            prm.ExpRes_[MethNum]->AddQueryTime(prm.TestSetId_, (1.0*t2 - t1)/1e3);


            prm.DistCompQty_[MethNum] += query->DistanceComputations();
            prm.avg_result_size_[MethNum] += query->ResultSize();

            if (query->ResultSize() > prm.max_result_size_[MethNum]) {
              prm.max_result_size_[MethNum] = query->ResultSize();
            }
#ifndef SEPARATE_EFFECTIVENESS_EVAL
            prm.queryIds.push_back(q);
            prm.queries.push_back(std::move(query));
#endif
          }
        }
      }
    }
  };

  template <typename QueryType, typename QueryCreatorType>
  static void Execute(bool LogInfo, unsigned ThreadTestQty, size_t TestSetId,
                     const vector<GoldStandard<dist_t>> &         goldStand,
                     std::vector<MetaAnalysis*>&                  ExpRes,
                     const ExperimentConfig<dist_t>&              config,
                     const QueryCreatorType&                      QueryCreator,
                     const vector<shared_ptr<IndexType>>&         IndexPtrs,
                     const vector<bool>&                          isNewIndex,
                     const vector<shared_ptr<MethodWithParams>>&  MethodsDesc) {
    size_t numquery = config.GetQueryObjects().size();

      /*
       *  We make 2 passes:
       *    1) Only measure CPU time & # of distance computations
       *    2) Compute effectiveness metrics for approximate methods & check correctness for exact ones
       */

    // 1-st pass
    if (LogInfo) LOG(LIB_INFO) << "##### Query type: " << QueryType::Type();
    if (LogInfo) LOG(LIB_INFO) << ">>>> query params = "  << QueryCreator.ParamsForPrint();
    if (LogInfo) LOG(LIB_INFO) << ">>>> Computing efficiency metrics ";

    unsigned MethQty = IndexPtrs.size();

    vector<uint64_t>  SearchTime(MethQty); 

    vector<double>    ClassAccuracy(MethQty);
    vector<double>    Recall(MethQty);
    vector<double>    NumCloser(MethQty);
    vector<double>    LogPosErr(MethQty);
    vector<double>    PrecisionOfApprox(MethQty);
    vector<double>    SystemTimeElapsed(MethQty);

    uint64_t  SeqSearchTime     = 0;

    vector<double>    AvgNumDistComp(MethQty);
    vector<double>    ImprDistComp(MethQty);
    vector<unsigned>  max_result_size(MethQty);
    vector<double>    avg_result_size(MethQty);
    vector<uint64_t>  DistCompQty(MethQty);

    mutex             UpdateStat;

    config.GetSpace()->SetQueryPhase();

    for (auto it = IndexPtrs.begin(); it != IndexPtrs.end(); ++it) {
      size_t MethNum = it - IndexPtrs.begin();
      Index<dist_t>& Method = **it;
      
     /* 
      * If we are reusing the same object instance of a method,
      * we have to reset query time parameters.

      * Setting query time parameters must be done 
      * before running any tests, in particular, because
      * the function SetQueryTimeParams is NOT supposed to be THREAD-SAFE. 
      */
      Method.SetQueryTimeParams(MethodsDesc[MethNum]->methPars_);

      if (LogInfo) LOG(LIB_INFO) << ">>>> Efficiency test for: "<< Method.ToString();

      WallClockTimer wtm;

      wtm.reset();

      if (!ThreadTestQty) ThreadTestQty = 1;

      vector<BenchmarkThreadParams<QueryType, QueryCreatorType>*>       ThreadParams(ThreadTestQty);
      vector<thread>                                                    Threads(ThreadTestQty);
      AutoVectDel<BenchmarkThreadParams<QueryType, QueryCreatorType>>   DelThreadParams(ThreadParams);


      for (unsigned QueryPart = 0; QueryPart < ThreadTestQty; ++QueryPart) {
        ThreadParams[QueryPart] =  new BenchmarkThreadParams<QueryType, QueryCreatorType>(
                                              UpdateStat,
                                              ThreadTestQty,
                                              QueryPart,
                                              TestSetId, 
                                              ExpRes,
                                              config,
                                              QueryCreator,
                                              Method,
                                              MethNum,
                                              SearchTime,
                                              AvgNumDistComp,
                                              max_result_size,
                                              avg_result_size,
                                              DistCompQty);
      }

      if (ThreadTestQty> 1) {
        for (unsigned QueryPart = 0; QueryPart < ThreadTestQty; ++QueryPart) {
          Threads[QueryPart] = std::thread(BenchmarkThread<QueryType, QueryCreatorType>(), 
                                     ref(*ThreadParams[QueryPart]));
        }
        for (unsigned QueryPart = 0; QueryPart < ThreadTestQty; ++QueryPart) {
          Threads[QueryPart].join();
        }
      } else {
        CHECK(ThreadTestQty == 1);
        BenchmarkThread<QueryType, QueryCreatorType>()(*ThreadParams[0]);
      }

      wtm.split();

      SearchTime[MethNum] = wtm.elapsed();

      AvgNumDistComp[MethNum] = static_cast<double>(DistCompQty[MethNum])/numquery;
      ImprDistComp[MethNum]   = config.GetDataObjects().size() / AvgNumDistComp[MethNum];

      ExpRes[MethNum]->SetImprDistComp(TestSetId, ImprDistComp[MethNum]);

      avg_result_size[MethNum] /= static_cast<double>(numquery);

      if (LogInfo) LOG(LIB_INFO) << ">>>> Computing effectiveness metrics for " << Method.ToString();

#ifndef SEPARATE_EFFECTIVENESS_EVAL
      for (unsigned QueryPart = 0; QueryPart < ThreadTestQty; ++QueryPart) {
        const BenchmarkThreadParams<QueryType, QueryCreatorType>*   params = ThreadParams[QueryPart];
       
        for (size_t qi = 0; qi < params->queries.size(); ++qi) {
          size_t            q = params->queryIds[qi] ;
          const QueryType*  pQuery = params->queries[qi].get();

          unique_ptr<QueryType> queryGS(QueryCreator(config.GetSpace(), config.GetQueryObjects()[q]));

          const GoldStandard<dist_t>&  QueryGS = goldStand[q];

          EvalResults<dist_t>     Eval(config.GetSpace(), pQuery, QueryGS);

          NumCloser[MethNum]    += Eval.GetNumCloser();
          LogPosErr[MethNum]    += Eval.GetLogRelPos();
          Recall[MethNum]       += Eval.GetRecall();
          double addAccuracy = (Eval.GetClassCorrect() == kClassCorrect ? 1:0);
          ClassAccuracy[MethNum]+= addAccuracy;
          PrecisionOfApprox[MethNum] += Eval.GetPrecisionOfApprox();

          ExpRes[MethNum]->AddPrecisionOfApprox(TestSetId, Eval.GetPrecisionOfApprox());
          ExpRes[MethNum]->AddRecall(TestSetId, Eval.GetRecall());
          ExpRes[MethNum]->AddClassAccuracy(TestSetId, addAccuracy);
          ExpRes[MethNum]->AddLogRelPosError(TestSetId, Eval.GetLogRelPos());
          ExpRes[MethNum]->AddNumCloser(TestSetId, Eval.GetNumCloser());

        }
      }
#endif
    }

    config.GetSpace()->SetIndexPhase();

#ifndef SEPARATE_EFFECTIVENESS_EVAL
     /* 
      * Sequential search times should be computed only once.
      * If we have multiple methods, they might be duplicated!
      */
     for (size_t q = 0; q < numquery; ++q) {
        const GoldStandard<dist_t>&  QueryGS = goldStand[q];
        SeqSearchTime     += QueryGS.GetSeqSearchTime();
     }
#else
    // 2d pass
    if (LogInfo) LOG(LIB_INFO) << ">>>> Computing effectiveness metrics " ;

    for (size_t q = 0; q < numquery; ++q) {
      unique_ptr<QueryType> queryGS(QueryCreator(config.GetSpace(), config.GetQueryObjects()[q]));

      const GoldStandard<dist_t>&  QueryGS = goldStand[q];

      SeqSearchTime     += QueryGS.GetSeqSearchTime();

      for (auto it = IndexPtrs.begin(); it != IndexPtrs.end(); ++it) {
        size_t MethNum = it - IndexPtrs.begin();
        Index<dist_t>& Method = **it;

        /* 
         * Reset the query-time parameters again,
         * because they could have changed previously (also by answering
         * another query during the effectiveness evaluation phase, i.e.,
         * for smaller values of the query id (q) in the loop (see on level up)
         */
        /*
         * !!!! NOTE (1) !!!!! If we ever make effectiveness testing multi-threaded,
         *                 setting of parameters must be removed from here, b/c
         *                 it's NOT THREAD-SAFE. See, how it's done in the 
         *                 efficiency testing phase.
         * !!! NOTE (2) !!!!! Now that we have an auto-tuning procedure for the VP-tree,
         *                 we can't really use this evaluation mode, b/c the tunning procedure is
         *                 invoked for each query.
         */
        Method.SetQueryTimeParams(MethodsDesc[MethNum]->methPars_);
      
        unique_ptr<QueryType> query(QueryCreator(config.GetSpace(), config.GetQueryObjects()[q]));
        
        Method.Search(query.get());

        EvalResults<dist_t>     Eval(config.GetSpace(), query.get(), QueryGS);

        NumCloser[MethNum]    += Eval.GetNumCloser();
        LogPosErr[MethNum]    += Eval.GetLogRelPos();
        Recall[MethNum]       += Eval.GetRecall();
        double addAccuracy = (Eval.GetClassCorrect() == kClassCorrect ? 1:0);
        ClassAccuracy[MethNum]+= addAccuracy;
        PrecisionOfApprox[MethNum] += Eval.GetPrecisionOfApprox();

        ExpRes[MethNum]->AddPrecisionOfApprox(TestSetId, Eval.GetPrecisionOfApprox());
        ExpRes[MethNum]->AddRecall(TestSetId, Eval.GetRecall());
        ExpRes[MethNum]->AddClassAccuracy(TestSetId, addAccuracy);
        ExpRes[MethNum]->AddLogRelPosError(TestSetId, Eval.GetLogRelPos());
        ExpRes[MethNum]->AddNumCloser(TestSetId, Eval.GetNumCloser());
      }
    }
#endif

    for (auto it = IndexPtrs.begin(); it != IndexPtrs.end(); ++it) {
      size_t MethNum = it - IndexPtrs.begin();
      Index<dist_t>& Method = **it;

      double timeSec = SearchTime[MethNum]/double(1e6);
      double queryPerSec = numquery / timeSec;

      if (LogInfo) {
        LOG(LIB_INFO) << "=========================================";
        LOG(LIB_INFO) << ">>>> Index type is "<< Method.ToString();
        LOG(LIB_INFO) << "=========================================";
        LOG(LIB_INFO) << ">>>> max # results = " << max_result_size[MethNum];
        LOG(LIB_INFO) << ">>>> avg # results = " << avg_result_size[MethNum];
        LOG(LIB_INFO) << ">>>> # of distance computations = " << AvgNumDistComp[MethNum];
        LOG(LIB_INFO) << ">>>> Impr in # of dist comp: " << ImprDistComp[MethNum];
        LOG(LIB_INFO) << "=========================================";
        LOG(LIB_INFO) << ">>>> Time elapsed:           " << timeSec << " sec";
        LOG(LIB_INFO) << ">>>> # of queries per sec: : " << queryPerSec;
        LOG(LIB_INFO) << ">>>> Avg time per query:     " << (timeSec/1e3/numquery) << " msec";
        LOG(LIB_INFO) << ">>>> System time elapsed:    " << (SystemTimeElapsed[MethNum]/double(1e6)) << " sec";
        LOG(LIB_INFO) << "=========================================";
      }

      // This number is adjusted for the number of threads!
      double  ImprEfficiency = static_cast<double>(SeqSearchTime)/(SearchTime[MethNum]*ThreadTestQty);

      ExpRes[MethNum]->SetImprEfficiency(TestSetId, ImprEfficiency);
      ExpRes[MethNum]->SetQueryPerSec(TestSetId, queryPerSec);

      Recall[MethNum]            /= static_cast<double>(numquery);
      ClassAccuracy[MethNum]     /= static_cast<double>(numquery);
      NumCloser[MethNum]         /= static_cast<double>(numquery);
      LogPosErr[MethNum]         /= static_cast<double>(numquery);
      PrecisionOfApprox[MethNum] /= static_cast<double>(numquery);
    
      if (LogInfo) {
        LOG(LIB_INFO) << "=========================================";
        LOG(LIB_INFO) << ">>>> # of test threads:              " << ThreadTestQty;
        LOG(LIB_INFO) << ">>>> Seq. search time elapsed:       " << (SeqSearchTime/double(1e6)) << " sec";
        LOG(LIB_INFO) << ">>>> Avg Seq. search time per query: " << (SeqSearchTime/double(1e3)/numquery) << " msec";
        LOG(LIB_INFO) << ">>>> Impr. in Efficiency = "           << ImprEfficiency;
        LOG(LIB_INFO) << ">>>> Recall              = "           << Recall[MethNum];
        LOG(LIB_INFO) << ">>>> PrecisionOfApprox   = "           << PrecisionOfApprox[MethNum];
        LOG(LIB_INFO) << ">>>> RelPosError         = "           << exp(LogPosErr[MethNum]);
        LOG(LIB_INFO) << ">>>> NumCloser           = "           << NumCloser[MethNum];
        LOG(LIB_INFO) << ">>>> Class. accuracy     = "           << ClassAccuracy[MethNum];
      }
    }

    if (LogInfo) LOG(LIB_INFO) << "#### Finished " << QueryType::Type() << " " << LibGetCurrentTime();
  }
};

}   // namespace similarity

#endif

