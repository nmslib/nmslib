/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2013-2018
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef _DISTANCE_H_
#define _DISTANCE_H_

#include <Eigen/Dense>
#include "global.h"
#include "utils.h"

namespace sqfd {

using Eigen::MatrixXd;
using Eigen::VectorXd;

class FeatureSignature {
 public:
  FeatureSignature(VRR& centers, VR& weights);
  FeatureSignature(std::ifstream& infile, int num_centers, int dim);
  ~FeatureSignature();

  void Print();

  const VRR& centers() const { return centers_; }
  const VR& weights() const { return weights_; }

 private:
  VRR centers_;
  VR weights_;
};

using FeatureSignaturePtr = std::shared_ptr<FeatureSignature>;
FeatureSignaturePtr readFeature(std::string filename);

class SimilarityFunction {
 public:
  virtual ~SimilarityFunction() {}
  virtual float f(const VR& p1, const VR& p2) = 0;
};

class MinusFunction : public SimilarityFunction {
 public:
  float f(const VR& p1, const VR& p2) {
    return -EuclideanDistance(p1, p2);
  }
};

class HeuristicFunction : public SimilarityFunction {
 public:
  HeuristicFunction(float alpha) : alpha_(alpha) {}
  float f(const VR& p1, const VR& p2) {
    return 1.0 / (alpha_ + EuclideanDistance(p1, p2));
  }
 private:
  const float alpha_;
};

class GaussianFunction : public SimilarityFunction {
 public:
  GaussianFunction(float alpha) : alpha_(alpha) {}
  float f(const VR& p1, const VR& p2) {
    const float d = EuclideanDistance(p1, p2);
    return exp(-alpha_ * d * d);
  }
 private:
  const float alpha_;
};

float SQFD(std::shared_ptr<SimilarityFunction> simfunc,
           FeatureSignaturePtr x, FeatureSignaturePtr y);

}

#endif    // _DISTANCE_H_
