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

#ifndef _SPACE_H_
#define _SPACE_H_

#include <string>
#include <map>
#include <stdexcept>
#include <iostream>
#include <cmath>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "permutation_type.h"

namespace similarity {

using std::map;
using std::runtime_error;

template <typename dist_t>
struct ExperimentConfig;

template <typename dist_t>
const char* DistTypeName();

template <> inline const char* DistTypeName<float>() { return "FLOAT"; }
template <> inline const char* DistTypeName<double>() { return "DOUBLE"; }
template <> inline const char* DistTypeName<int>() { return "INT"; }
template <typename dist_t> inline const char* DistTypeName() { return typeid(dist_t).name(); }

template <typename dist_t>
class Query;

template <typename dist_t>
class KNNQuery;

template <typename dist_t>
class RangeQuery;

template <typename dist_t>
struct Experiments;

template <typename dist_t>
class Space {
 public:
  virtual ~Space() {}
  // This function is public and it is not supposed to be used in the query-mode
  dist_t IndexTimeDistance(const Object* obj1, const Object* obj2) const {
    if (!bIndexPhase) {
      throw runtime_error(string("The public function ") + __func__ + 
                                 " function is accessible only during the indexing phase!");
    }
    return HiddenDistance(obj1, obj2);
  }
  virtual void ReadDataset(ObjectVector& dataset,
                      const ExperimentConfig<dist_t>* config,
                      const char* inputfile,
                      const int MaxNumObjects) const = 0;
  virtual std::string ToString() const = 0;
  virtual void PrintInfo() const { LOG(INFO) << ToString(); }

 protected:
  void SetIndexPhase() const { bIndexPhase = true; }
  void SetQueryPhase() const { bIndexPhase = false; }

  friend class Query<dist_t>;
  friend class RangeQuery<dist_t>;
  friend class KNNQuery<dist_t>;
  friend class Experiments<dist_t>;
  /* 
   * This function is private, but it will be accessible by the friend class Query
   * IndexTimeDistance access can be disable/enabled only by function friends 
   */
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const = 0;
 private:
  bool mutable bIndexPhase = true;
};

/*
 * This version of intrinsic dimensionality is defined in 
 * E. Chavez, G. Navarro, R. Baeza-Yates, and J. L. Marroquin, 2001, Searching in metric spaces.
 *
 * Note that this measure may be irrelevant in non-metric spaces.
 */
template <typename dist_t>
void ComputeIntrinsicDimensionality(const Space<dist_t>& space, 
                               const ObjectVector& dataset,
                               double& IntrDim,
                               double& DistMean,
                               double& DistSigma,
                               size_t SampleQty = 1000000) {
  std::vector<double> dist;
  DistMean = 0;
  for (size_t n = 0; n < SampleQty; ++n) {
    size_t r1 = RandomInt() % dataset.size();
    size_t r2 = RandomInt() % dataset.size();
    CHECK(r1 < dataset.size());
    CHECK(r2 < dataset.size());
    const Object* obj1 = dataset[r1];
    const Object* obj2 = dataset[r2];
    dist_t d = space.IndexTimeDistance(obj1, obj2);
    dist.push_back(d);
    if (std::isnan(d)) {
      /* 
       * TODO: @leo Dump object contents here. To this end,
       *            we need to subclass objects, so that sparse
       *            vectors, dense vectors and other objects
       *            can implement their own dump function.
       */
      LOG(FATAL) << "!!! Bug: a distance returned NAN!";
    }
    DistMean += d;
  }
  DistMean /= double(SampleQty);
  DistSigma = 0;
  for (size_t i = 0; i < SampleQty; ++i) {
    DistSigma += (dist[i] - DistMean) * (dist[i] - DistMean);
  }
  DistSigma /= double(SampleQty);
  IntrDim = DistMean * DistMean / (2 * DistSigma);
  DistSigma = sqrt(DistSigma);
}

template <typename dist_t>
void ReportIntrinsicDimensionality(const string& reportName,
                                   const Space<dist_t>& space, 
                                   const ObjectVector& dataset,
                                   size_t SampleQty = 1000000) {
    double DistMean, DistSigma, IntrDim;

    ComputeIntrinsicDimensionality(space, dataset,
                                  IntrDim,
                                  DistMean,
                                  DistSigma,
                                  SampleQty);

    LOG(INFO) << "### " << reportName;
    LOG(INFO) << "### intrinsic dim: " << IntrDim;
    LOG(INFO) << "### distance mean: " << DistMean;
    LOG(INFO) << "### distance sigma: " << DistSigma;
}

}  // namespace similarity

#endif
