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
#include "searchoracle.h"
#include "method/vptree_utils.h"
#include "space.h"

#include <vector>
#include <iostream>
#include <queue>

namespace similarity {

using std::vector;

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
  LOG(INFO) << "MaxK              = " << MaxK;
  LOG(INFO) << "DoRandSamp        = " << DoRandSample;
  LOG(INFO) << "DistLearnThresh   = " << DistLearnThreshold;
  LOG(INFO) << "QuantileStepPivot = " << QuantileStepPivot;
  LOG(INFO) << "QuantileStepPseudoQuery       = " << QuantileStepPseudoQuery;
  LOG(INFO) << "NumOfPseudoQueriesInQuantile  = " << NumOfPseudoQueriesInQuantile;
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


}
