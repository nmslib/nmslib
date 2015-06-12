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
#include "searchoracle.h"
#include "method/vptree_utils.h"
#include "space.h"
#include "method/vptree.h"
#include "tune.h"

#include <vector>
#include <iostream>
#include <queue>
#include <cmath>

namespace similarity {

const size_t TUNE_QTY_DEFAULT = 20000;
const size_t TUNE_QUERY_QTY   = 200;
const size_t TUNE_SPLIT_QTY   = 5;
const size_t TOTAL_QUERY_QTY  = TUNE_QUERY_QTY * TUNE_SPLIT_QTY;
const size_t MIN_TUNE_QTY     = TOTAL_QUERY_QTY; 

using std::vector;
using std::round;
using std::endl;
using std::cout;

template <typename dist_t>
void PolynomialPruner<dist_t>::SetParams(AnyParamManager& pmgr) {
  // Default values are for the classic triangle inequality
  alpha_left_  = 1.0;
  exp_left_    = MIN_EXP_DEFAULT;
  alpha_right_ = 1.0;
  exp_right_   = MAX_EXP_DEFAULT;
  pmgr.GetParamOptional(ALPHA_LEFT_PARAM, alpha_left_);
  pmgr.GetParamOptional(ALPHA_RIGHT_PARAM, alpha_right_);
  pmgr.GetParamOptional(EXP_LEFT_PARAM, exp_left_);
  pmgr.GetParamOptional(EXP_RIGHT_PARAM, exp_right_);

  if (pmgr.hasParam(TUNE_R_PARAM) && pmgr.hasParam(TUNE_K_PARAM)) {
    stringstream err;

    err << "Specify only one paramter: " << TUNE_R_PARAM << " or " << TUNE_K_PARAM;
    LOG(LIB_INFO) << err.str();
    throw runtime_error(err.str());
  }

  /*
   * The tuning part is hardcoded to make only with the VP-tree.
   */
  if (pmgr.hasParam(TUNE_R_PARAM) || pmgr.hasParam(TUNE_K_PARAM)) {
  // Obtain optimal parameters automatically
    size_t fullBucketSize;
    pmgr.GetParamRequired("bucketSize", fullBucketSize);

    if (data_.size() < TOTAL_QUERY_QTY) {
      stringstream err;
      err << "The data size is too small: should be > " << MIN_TUNE_QTY;
      LOG(LIB_INFO) << err.str();
      throw runtime_error(err.str());
    }

    size_t treeHeightQty = static_cast<size_t>(round(float(data_.size() - TUNE_QUERY_QTY)/ fullBucketSize)); 

    size_t tuneQty = TUNE_QTY_DEFAULT;
    pmgr.GetParamOptional(TUNE_QTY_PARAM, tuneQty);

    if (treeHeightQty > tuneQty) {
      stringstream err;
      LOG(LIB_INFO) << "Increasing the value of '" << TUNE_QTY_PARAM << "', because it should be >= " << treeHeightQty;
      tuneQty = treeHeightQty;
    }
    if (treeHeightQty < MIN_TUNE_QTY) {
      stringstream err;
      err << "The data size is too small or the bucket size is too big. Select the parameters so that <total # of records> is NOT less than <bucket size> * " << MIN_TUNE_QTY; 
      LOG(LIB_INFO) << err.str();
      throw runtime_error(err.str());
    }

    size_t bucketSize = tuneQty / treeHeightQty;
    if (!bucketSize) {
      LOG(LIB_INFO) << "Bug: bucket size is zero in the tunning code";
      throw runtime_error("Bug: bucket size is zero in the tunning code");
    }
    LOG(LIB_INFO) << "tuneQty: " << tuneQty << " Chosen bucket size: " << bucketSize;

    float desiredRecall;
    pmgr.GetParamRequired(DESIRED_RECALL_PARAM, desiredRecall);

    vector<string>                methodDesc;
    stringstream                  methStrDesc;
    string                        methName;

    methStrDesc << METH_VPTREE << ":bucketSize=" << bucketSize;

    ParseMethodArg(methStrDesc.str(), methName, methodDesc);
    shared_ptr<MethodWithParams>  method = shared_ptr<MethodWithParams>(new MethodWithParams(methName, methodDesc));

    size_t minExp = MIN_EXP_DEFAULT, maxExp = MAX_EXP_DEFAULT;

    pmgr.GetParamOptional(MIN_EXP_PARAM, minExp);
    pmgr.GetParamOptional(MAX_EXP_PARAM, maxExp);

    if (!maxExp) throw runtime_error(string(MIN_EXP_PARAM) + " can't be zero!");
    if (maxExp < minExp) throw runtime_error(string(MAX_EXP_PARAM) + " can't be < " + string(MIN_EXP_PARAM));


    string metricName = OPTIM_METRIC_DEFAULT; 

    pmgr.GetParamOptional(OPTIM_METRIC_PARAMETER, metricName);

    OptimMetric metric = getOptimMetric(metricName);

    if (IMPR_INVALID == metric) {
      stringstream err;
  
      err << "Invalid metric name: " << metricName;
      LOG(LIB_INFO) << err.str();
      throw runtime_error(err.str());
    }


    unique_ptr<ExperimentConfig<dist_t>>  config;

    dist_t eps = 0;

    ObjectVector emptyQueries;

    vector<unsigned>                  knn;
    vector<dist_t>                    range;
    // The cloned space will be in the indexing mode.
    // Otherwise, the tunning code will crash while accessing function IndexTimeDistance() 
    unique_ptr<const Space<dist_t>>   clonedSpace(space_->Clone());

    if (pmgr.hasParam(TUNE_R_PARAM)) {
      dist_t r;
      pmgr.GetParamRequired(TUNE_R_PARAM, r);
      range.push_back(r);
      
      config.reset(new ExperimentConfig<dist_t>(clonedSpace.get(), 
                                      data_, emptyQueries, 
                                      TUNE_SPLIT_QTY,
                                      tuneQty, TUNE_QUERY_QTY,
                                      0, knn, eps, range));
  
    }

    if (pmgr.hasParam(TUNE_K_PARAM)) {
      size_t k;
      pmgr.GetParamRequired(TUNE_K_PARAM, k);
      knn.push_back(k);
      
      config.reset(new ExperimentConfig<dist_t>(clonedSpace.get(), 
                                      data_, emptyQueries, 
                                      TUNE_SPLIT_QTY,
                                      tuneQty, TUNE_QUERY_QTY,
                                      0, knn, eps, range));
  
    }

    CHECK(config.get());

    config->ReadDataset();

    size_t maxCacheGSQty = MAX_CACHE_GS_QTY_DEFAULT;
    pmgr.GetParamOptional(MAX_CACHE_GS_QTY_PARAM, maxCacheGSQty);
    size_t maxIter = MAX_ITER_DEFAULT;
    pmgr.GetParamOptional(MAX_ITER_PARAM, maxIter);
    size_t maxRecDepth = MAX_REC_DEPTH_DEFAULT;
    pmgr.GetParamOptional(MAX_REC_DEPTH_PARAM, maxRecDepth);
    size_t stepN = STEP_N_DEFAULT;
    pmgr.GetParamOptional(STEP_N_PARAM, stepN);
    size_t addRestartQty = ADD_RESTART_QTY_DEFAULT;
    pmgr.GetParamOptional(ADD_RESTART_QTY_PARAM, addRestartQty);
    float fullFactor = FULL_FACTOR_DEFAULT;
    pmgr.GetParamOptional(FULL_FACTOR_PARAM, fullFactor);

    float recall = 0, time_best = 0, impr_best = -1, alpha_left = 0, alpha_right = 0; 
    unsigned exp_left = 0, exp_right = 0;

    static  std::random_device          rd;
    static  std::mt19937                engine(rd());
    static  std::normal_distribution<>  normGen(0.0f, log(fullFactor));


    for (unsigned ce = minExp; ce <= maxExp; ++ce)
    for (unsigned k = 0; k < 1 + addRestartQty; ++k) {
      unsigned expLeft = ce, expRight = ce;
      float recall_loc, time_best_loc, impr_best_loc, 
            alpha_left_loc = 1.0, alpha_right_loc = 1.0; // These are initial values

      if (k > 0) {
        // Let's do some random normal fun
        alpha_left_loc = exp(normGen(engine));
        alpha_right_loc = exp(normGen(engine));
        LOG(LIB_INFO) << " RANDOM STARTING POINTS: " << alpha_left_loc << " " << alpha_right_loc;
      } 

      string SpaceType; // VP-tree doesn't use the space type during creation.

      GetOptimalAlphas(printProgress_,
                     *config, 
                     metric, desiredRecall,
                     SpaceType, 
                     METH_VPTREE, 
                     method->methPars_, 
                     recall_loc, 
                     time_best_loc, impr_best_loc,
                     alpha_left_loc, expLeft, alpha_right_loc, expRight,
                     maxIter, maxRecDepth, stepN, fullFactor, maxCacheGSQty);

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

    alpha_left_ = alpha_left;
    alpha_right_ = alpha_right;
    exp_left_ = exp_left;
    exp_right_ = exp_right;

    stringstream  bestParams;
    bestParams << ALPHA_LEFT_PARAM << "=" << alpha_left_ << "," << ALPHA_RIGHT_PARAM << "=" << alpha_right_ << ","
                 << EXP_LEFT_PARAM   << "=" << exp_left_   << "," << EXP_RIGHT_PARAM   << "=" << exp_right_;

    if (printProgress_) {
      cout << "===============================================================" << endl;
      cout << "|                      OPTIMIZATION RESULTS                   |" << endl;
      cout << "===============================================================" << endl;
      if (!knn.empty()) {
        cout << "K: "  << knn[0] << endl;
      } else {
        cout << "Range: "  << range[0] << endl;
      }
      cout << "Recall: " << recall << endl;
      cout << "Best time: " << time_best << endl;
      cout << "Best impr. " << impr_best << " (" << getOptimMetricName(metric) << ")" << endl; 
      cout << "alpha_left: " << alpha_left << endl;
      cout << "exp_left: " << exp_left << endl;
      cout << "alpha_right: " << alpha_right << endl;
      cout << "exp_right: " << exp_right << endl;
      cout << "optimal parameters: " << bestParams.str() << endl;
      cout << "===============================================================" << endl;
    }

    if (recall < desiredRecall) {
      LOG(LIB_INFO) << "Failed to get the desired recall!";
      throw runtime_error("Failed to get the desired recall!");
    }
 
  }
}

template <typename dist_t>
SamplingOracleCreator<dist_t>::SamplingOracleCreator(
                   const Space<dist_t>* space,
                   const ObjectVector& AllVectors,
                   bool   DoRandSample,
                   size_t MaxK,
                   float  QuantileStepPivot,
                   float  QuantileStepPseudoQuery,
                   size_t NumOfPseudoQueriesInQuantile,
                   float  DistLearnThreshold
                   ) : space_(space),
                       AllVectors_(AllVectors),
                       DoRandSample_(DoRandSample),
                       MaxK_(MaxK),
                       QuantileStepPivot_(QuantileStepPivot),
                       QuantileStepPseudoQuery_(QuantileStepPseudoQuery),
                       NumOfPseudoQueriesInQuantile_(NumOfPseudoQueriesInQuantile),
                       DistLearnThreshold_(DistLearnThreshold)
{
  LOG(LIB_INFO) << "MaxK              = " << MaxK;
  LOG(LIB_INFO) << "DoRandSamp        = " << DoRandSample;
  LOG(LIB_INFO) << "DistLearnThresh   = " << DistLearnThreshold;
  LOG(LIB_INFO) << "QuantileStepPivot = " << QuantileStepPivot;
  LOG(LIB_INFO) << "QuantileStepPseudoQuery       = " << QuantileStepPseudoQuery;
  LOG(LIB_INFO) << "NumOfPseudoQueriesInQuantile  = " << NumOfPseudoQueriesInQuantile;
}

template <typename dist_t>
SamplingOracle<dist_t>::SamplingOracle(
                   const Space<dist_t>* space,
                   const ObjectVector& AllVectors,
                   const Object* pivot,
                   const DistObjectPairVector<dist_t>& DistsOrig,
                   bool   DoRandSample,
                   size_t MaxK,
                   float  QuantileStepPivot,
                   float  QuantileStepPseudoQuery,
                   size_t NumOfPseudoQueriesInQuantile,
                   float  DistLearnThreshold
                   ) : NotEnoughData_(false)
{
  CHECK(QuantileStepPivot > 0 && QuantileStepPivot < 1);
  CHECK(QuantileStepPseudoQuery > 0 && QuantileStepPseudoQuery < 1);

  dist_t               PivotR = GetMedian(DistsOrig).first;

  /*
   * Note that here using float (or any other floating-point type) is not
   * a mistake. This is not related to dist_t.
   */
  vector<float>   quantiles;
#if 1
  for (float val = 0; val <= 1.00001 - QuantileStepPivot; val += QuantileStepPivot)
      quantiles.push_back(val);
#else
  // Using finer-grained quantiles seems to be not very helpful
  for (float val = 0; val <= 1.00001 - QuantileStepPivot; ) {
    quantiles.push_back(val);
    if (fabs(val - 0.5) > 0.1) {
      val += QuantileStepPivot;
    } else if (fabs(val - 0.5) > 0.02) {
      val += QuantileStepPivot/5;
    } else
      val += QuantileStepPivot/50;

  }
#endif

  DistObjectPairVector<dist_t> dists = DistsOrig;

  size_t MinReqSize = quantiles.size();

  if (dists.size() < MinReqSize) {
    // Let's ignore a few unlikely duplicates
    for (unsigned i = 0; i < MinReqSize - dists.size(); ++i) {
      size_t RandId = RandomInt() % AllVectors.size();

      // Distance can be asymmetric, pivot should be on the left
      dists.push_back(std::make_pair(space->IndexTimeDistance(pivot, AllVectors[RandId]), AllVectors[RandId]));
    }
  }

  vector<size_t> qind = EstimateQuantileIndices(dists, quantiles);

  if (qind.size()) {
  // Insert the last and the first indices
    if (qind[0] != 0) qind.insert(qind.begin(), 0);
    size_t LastIndx =  dists.size() - 1;
    if (qind[qind.size() - 1] != LastIndx) qind.push_back(LastIndx);
  }

  for (unsigned i = 0; i < qind.size(); ++i) {
    QuantilePivotDists.push_back(dists[qind[i]].first);
  }

  if (QuantilePivotDists.size() >= MinQuantIndQty) {
    CHECK(QuantilePivotDists.size() > 0);
    size_t N = QuantilePivotDists.size() - 1;
    QuantileMaxPseudoQueryDists.resize(N);

    for (size_t quant = 0; quant < N; ++ quant) {
      std::vector<size_t>                       RandIds;
      std::vector<std::pair<dist_t, bool> >     SampleRes;

      for (size_t i = 0; i < NumOfPseudoQueriesInQuantile; ++i) {
        size_t d = qind[quant + 1] - qind[quant];
        CHECK(d > 0);

        size_t Id = qind[quant] + static_cast<size_t>(RandomInt() % d);

        CHECK(qind[quant] <= Id && Id < qind[quant + 1]);

        RandIds.push_back(Id);
      }

      std::sort(RandIds.begin(), RandIds.end());

        /*
         * If we are doing random sampling, we can reuse the same (pseudo) query
         * multiple times. Anyways, the data points will be different most of the time.
         * However, if we are computing the MaxK-neighborhood of a pseudoquery
         * reusing queries doesn't give us additional information. So, instead of reusing
         * the same pseudoquery, we simply increase the size of the neighborhood
         */
      if (!DoRandSample) {
        RandIds.erase(std::unique(RandIds.begin(), RandIds.end()), RandIds.end());
      }

      for (auto it = RandIds.begin(); it != RandIds.end(); ++it) {
        const Object* PseudoQuery = dists[*it].second;
        dist_t  Pivot2QueryDist = space->IndexTimeDistance(pivot, PseudoQuery);
        bool    QueryLeft  =  Pivot2QueryDist <= PivotR;
        bool    QueryRight =  Pivot2QueryDist >= PivotR;

          /*
           * If we didn't find enough points in the interval,
           * let's try to compensate for this by using larger neighborhoods.
           * See, the comment above: this strategy is not necessary, if sampling
           * is random.
           */

        size_t SampleQty = MaxK;
        if (!DoRandSample) {
          SampleQty *= NumOfPseudoQueriesInQuantile * quantiles.size() / N / RandIds.size();

          std::priority_queue<std::pair<dist_t, bool> >  NN;

          for (size_t j = 0; j < dists.size(); ++j) {
            const Object* ThatObj = dists[j].second;

            // distance can be asymmetric, but the query should be on the right
            dist_t Query2ObjDist = space->IndexTimeDistance(ThatObj, PseudoQuery);

            /*
            * If:
            *    (1) the queue contains < MaxK elements and
            *    (2) ThatObj is farther from the query than the objects in the query
            * Then  ignore ThatObj
            */
            if (NN.size() < MaxK || NN.top().first > Query2ObjDist) {
              // distance can be asymmetric, but the pivot is on the left
              dist_t  Pivot2ObjDist = space->IndexTimeDistance(pivot, ThatObj);

              bool    ObjLeft  =  Pivot2ObjDist <= PivotR;
              bool    ObjRight =  Pivot2ObjDist >= PivotR;
              bool    IsSameBall = ObjLeft == QueryLeft || ObjRight == QueryRight;

              // distance can be asymmetric, but query is on the right side
              NN.push(std::make_pair(Query2ObjDist, IsSameBall));
              if (NN.size() > MaxK) NN.pop();
            }

            while (!NN.empty()) {
              const std::pair<dist_t, bool>&  elem = NN.top();
              SampleRes.push_back(elem);
              NN.pop();
            }
          }
        } else {
          for (size_t k = 0; k < MaxK; ++k) {
            size_t Id = static_cast<size_t>(RandomInt() % dists.size());
            const Object* ThatObj = dists[Id].second;

            // distance can be asymmetric, but the pivot is on the left
            dist_t  Pivot2ObjDist = space->IndexTimeDistance(pivot, ThatObj);

            bool    ObjLeft  =  Pivot2ObjDist <= PivotR;
            bool    ObjRight =  Pivot2ObjDist >= PivotR;
            bool    IsSameBall = ObjLeft == QueryLeft || ObjRight == QueryRight;

            // distance can be asymmetric, but query is on the right side
            SampleRes.push_back(std::make_pair(space->IndexTimeDistance(ThatObj, PseudoQuery), IsSameBall));
          }
        }
      }
      {
        sort(SampleRes.begin(), SampleRes.end());
        vector<float>   quantiles;
        for (float val = 0; val <= 1.00001 - QuantileStepPseudoQuery; val += QuantileStepPseudoQuery)
          quantiles.push_back(val);

        vector<size_t> qind = EstimateQuantileIndices(SampleRes, quantiles);

        dist_t PrevQueryR = static_cast<dist_t>(0);
        size_t PrevIndx = 0;

        float QtyAllSum  = 0;
        float QtyDiffSum = 0;

        for(size_t i = 0; i < qind.size(); ++i) {
          size_t indx = qind[i];

          float Qty = indx - PrevIndx + (i == 0);
          QtyAllSum += Qty;
          for (size_t j = PrevIndx + (i != 0); j <= indx; ++j)
            QtyDiffSum += !SampleRes[j].second;

          if (QtyAllSum > 0 && QtyDiffSum > QtyAllSum * DistLearnThreshold) {
            /*
             * We dont' just re-use PrevQueryR, because this results in
             * a very conservative estimate.
             */
            PrevQueryR = (PrevQueryR + SampleRes[indx].first) / 2;
            break;
          }
          PrevQueryR = SampleRes[indx].first;
          PrevIndx = indx;
        }
        QuantileMaxPseudoQueryDists[quant] = PrevQueryR;
      }
    }

#if 0
    std::cout << " # created a sampler, dists.size() = " << dists.size() << std::endl;
#endif
  } else {
    NotEnoughData_ = true;
  }
}

template class SamplingOracleCreator<int>;
template class SamplingOracleCreator<short int>;
template class SamplingOracleCreator<float>;
template class SamplingOracleCreator<double>;

template class PolynomialPruner<int>;
template class PolynomialPruner<float>;
template class PolynomialPruner<double>;


}
