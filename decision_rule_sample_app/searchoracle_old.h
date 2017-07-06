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
#ifndef SEARCH_ORACLE_OLD_HPP
#define SEARCH_ORACLE_OLD_HPP

#include <string>
#include <cmath>
#include <vector>
#include <sstream>

#include "object.h"
#include "space.h"
#include "pow.h"
#include "params.h"
#include "experimentconf.h"
#include "logging.h"
#include "searchoracle.h"

namespace similarity {

using std::string; 
using std::vector; 
using std::stringstream;

template <typename dist_t>
class SamplingOracle {
public:
    SamplingOracle(const typename similarity::Space<dist_t>* space,
                   const ObjectVector& AllVectors,
                   const Object* pivot,
                   const DistObjectPairVector<dist_t>& dists,
                   bool   DoRandSample,
                   size_t MaxK,
                   float  QuantileStepPivot,
                   float  QuantileStepPseudoQuery,
                   size_t NumOfPseudoQueriesInQuantile,
                   float  DistLearnThreshold
                   );
    static std::string GetName() { return "sampling"; }

    inline VPTreeVisitDecision Classify(dist_t dist, dist_t MaxDist, dist_t MedianDist) {
        if (NotEnoughData_ || dist == MedianDist) return kVisitBoth;

        if (dist < QuantilePivotDists[0]) return kVisitBoth;

        auto it = std::lower_bound(QuantilePivotDists.begin(), QuantilePivotDists.end(), dist);

        if (QuantilePivotDists.end() == it) return kVisitBoth;

        size_t quant = it - QuantilePivotDists.begin();
      
        if (quant >= QuantileMaxPseudoQueryDists.size()) return kVisitBoth;

        dist_t MaxQueryR = QuantileMaxPseudoQueryDists[quant];

        if (MaxQueryR <= MaxDist) return kVisitBoth;

        CHECK(dist != MedianDist); // should return kVisitBoth before reaching this point
        return dist < MedianDist ? kVisitLeft : kVisitRight;
    }
    string Dump() { 
      stringstream str1, str2;

      for (unsigned i = 0; i < QuantileMaxPseudoQueryDists.size(); ++i) {
        if (i) {
          str1 << ",";
          str2 << ",";
        }
        str1 << QuantilePivotDists[i];
        str2 << QuantileMaxPseudoQueryDists[i];
      }

      return str1.str() + "\n" + str2.str() + "\n";
    }

private:
  const  unsigned MinQuantIndQty = 4;  
  bool   NotEnoughData_; // If true, the classifier returns kVisitBoth

  std::vector<dist_t>  QuantilePivotDists;
  std::vector<dist_t>  QuantileMaxPseudoQueryDists;

  
};

template <typename dist_t>
class SamplingOracleCreator {
public:
    SamplingOracle<dist_t>* Create(unsigned level, const Object* pivot, const DistObjectPairVector<dist_t>& dists) const {
      try {
        return new SamplingOracle<dist_t>(space_,
                                          AllVectors_,
                                          pivot,
                                          dists,
                                          DoRandSample_,
                                          MaxK_,
                                          QuantileStepPivot_,
                                          QuantileStepPseudoQuery_,
                                          NumOfPseudoQueriesInQuantile_,
                                          DistLearnThreshold_);
      } catch (const std::exception& e) {
        LOG(LIB_FATAL) << "Exception while creating sampling oracle: " << e.what();
      } catch (...) {
        LOG(LIB_FATAL) << "Unknown exception while creating sampling oracle";
      } 
      return NULL;
    }
    SamplingOracleCreator(const typename similarity::Space<dist_t>* space,
                   const ObjectVector& AllVectors,
                   bool   DoRandSample,
                   size_t MaxK,
                   float  QuantileStepPivotDists,
                   float  QuantileStepPseudoQuery,
                   size_t NumOfPseudoQueriesInQuantile,
                   float  FractToDetectFuncVal
                   );
private:
  const  typename similarity::Space<dist_t>* space_;
  const  ObjectVector& AllVectors_;
  bool   DoRandSample_; // If true, we don't compute K-neighborhood exactly, MaxK points are sampled randomly.
  size_t MaxK_;
  float  QuantileStepPivot_;       // Quantiles for the distances to a pivot
  float  QuantileStepPseudoQuery_;      // Quantiles for the distances to a pseudo-query
  size_t NumOfPseudoQueriesInQuantile_; /* The number of pseudo-queries, 
                                          which are selected in each distance quantile.
                                        */
  float  DistLearnThreshold_; /* A fraction of observed kVisitBoth-type points we wnat to encounter
                                  before declaring that some radius r is the maximum radius for which
                                  all results are within the same ball as the query point.
                                  The smaller is FractToDetectFuncVal, the close our sampling-based 
                                  procedure to the exact searching. That is, the highest recall
                                  would be for FractToDetectFuncVal == 0.
                                */
};

}

#endif
