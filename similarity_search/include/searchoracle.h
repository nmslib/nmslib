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
#ifndef SEARCH_ORACLE_HPP
#define SEARCH_ORACLE_HPP

#include <string>
#include <sstream>

#include "object.h"
#include "space.h"
#include "experimentconf.h"
#include "logging.h"

namespace similarity {

using std::string; 
using std::stringstream;

enum VPTreeVisitDecision { kVisitLeft = 1, kVisitRight = 2, kVisitBoth = 3 };


template <typename dist_t>
class TriangIneq {
public:
  static std::string GetName() { return "triangle inequality"; }
  TriangIneq(double alpha_left, double alpha_right) : alpha_left_(alpha_left), alpha_right_(alpha_right){}

  inline VPTreeVisitDecision Classify(dist_t dist, dist_t MaxDist, dist_t MedianDist) {
/*
 * Stretching triangle inequality similar to the description in:
 *
 * Probabilistic proximity search: Fighting the curse of dimensionality in metric spaces
 * E Chavez, G Navarro 
 *
 */

    /*
     * If the median is in both subtrees (e.g., this is often the case of a discreete metric)
     * and the distance to the pivot is MedianDist, we need to visit both subtrees.
     * Hence, we check for the strict inequality!!! Even if MaxDist == 0, 
     * for the case of dist == MedianDist, 0 < 0 is false. 
     * Thus, we visit both subtrees. 
     */
    if (double(MaxDist) < alpha_left_ * (double(MedianDist) - dist)) return (kVisitLeft);
    if (double(MaxDist) < alpha_right_* (dist - double(MedianDist))) return (kVisitRight);

    return (kVisitBoth);
  }
  string Dump() { 
    stringstream str;

    str << "AlphaLeft: " << alpha_left_ << " AlphaRight: " << alpha_right_;
    return str.str();
  }
private:
  double alpha_left_;
  double alpha_right_;
};

template <typename dist_t>
class TriangIneqCreator {
public:
  TriangIneqCreator(double alpha_left, double alpha_right) : alpha_left_(alpha_left), alpha_right_(alpha_right){
    LOG(LIB_INFO) << "alphaLeft (left stretch coeff)= "   << alpha_left;
    LOG(LIB_INFO) << "alphaRight (right stretch coeff)= " << alpha_right;

  }
  TriangIneq<dist_t>* Create(unsigned level, const Object* /*pivot_*/, const DistObjectPairVector<dist_t>& /*dists*/) const {
    return new TriangIneq<dist_t>(alpha_left_, alpha_right_);
  }
private:
  double alpha_left_;
  double alpha_right_;
};

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
