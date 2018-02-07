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
#include <math.h>
#include "distance.h"

namespace sqfd {

FeatureSignature::FeatureSignature(VRR& centers, VR& weights)
    : centers_(centers), weights_(weights) {}

FeatureSignature::FeatureSignature(std::ifstream& infile,
                                   int num_centers, int dim) {
  float w;
  for (int i = 0; i < num_centers; ++i) {
    VR center(dim);
    for (int j = 0; j < dim; ++j) {
      infile >> center[j];
    }
    centers_.push_back(center);
    infile >> w;
    weights_.push_back(w);
  }
}

FeatureSignature::~FeatureSignature() {
}

void FeatureSignature::Print() {
  for (size_t i = 0; i < centers_.size(); ++i) {
    for (auto v : centers_[i]) {
      std::cout << v << " ";
    }
    std::cout << "\t" << weights_[i] << std::endl;
  }
}

FeatureSignaturePtr readFeature(std::string filename) {
  std::ifstream infile(filename.c_str());
  int num_centers, dim;
  infile >> num_centers >> dim;
  return FeatureSignaturePtr(new FeatureSignature(infile, num_centers, dim));
}

float SQFD(std::shared_ptr<SimilarityFunction> simfunc,
           FeatureSignaturePtr x, FeatureSignaturePtr y) {
  const auto& wx = x->weights();
  const auto& wy = y->weights();
  VectorXd W(wx.size() + wy.size());
  for (size_t i = 0; i < wx.size(); ++i) {
    W(i) = wx[i];
  }
  for (size_t i = 0; i < wy.size(); ++i) {
    W(wx.size() + i) = -wy[i];
  }
  const size_t sz = wx.size() + wy.size();
  MatrixXd A(sz, sz);
  for (size_t i = 0; i < sz; ++i) {
    for (size_t j = i; j < sz; ++j) {
      const VR& p1 = i < wx.size() ? x->centers()[i] : y->centers()[i-wx.size()];
      const VR& p2 = j < wx.size() ? x->centers()[j] : y->centers()[j-wx.size()];
      A(i,j) = A(j,i) = simfunc->f(p1, p2);
    }
  }
  auto res = W.transpose() * A * W;
  return sqrt(res(0,0));
}

}
