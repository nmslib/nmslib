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
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <math.h>
#include <assert.h>

#include <iostream>
#include <fstream>
#include <tuple>
#include <vector>
#include <random>
#include <string>
#include <sstream>
#include <algorithm>
#include <array>
#include <queue>
#include <unordered_set>
#include <thread>
#include <mutex>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace sqfd {

const int kFeatureDims = 7;   // <L,a,b,row,col,contrast,coarseness>
const int kWindowSize = 5;
const int kMaxIter = 100;
const float kEPS = 1e-3;

const float kMinL = 0.0;
const float kMaxL = 100.0;
const float kMinA = -86.185;
const float kMaxA = 98.255;
const float kMinB = -107.865;
const float kMaxB = 94.483;
const float kMinContrast = 0.0;
const float kMaxContrast = 140.0;
const float kMinCoarseness = 0.0;
const float kMaxCoarseness = 1.0;

using PairII = std::pair<int,int>;
using VectorPairII = std::vector<PairII>;
using Float3 = std::array<float,3>;
using Feature = std::array<float,kFeatureDims>;
using VR = std::vector<float>;
using VRR = std::vector<VR>;

template <typename T>
inline T sqr(const T& v) {
  return v*v;
}

struct PairIIHash {
  size_t operator()(const PairII& x) const {
    return std::hash<int>()(x.first) * 31 +
           std::hash<int>()(x.second);
  }
};

struct PairIIEqual {
  bool operator()(const PairII& x, const PairII& y) const {
    return x.first == y.first && x.second == y.second;
  }
};

template <typename T>
std::string ToString(const T& t) {
  std::stringstream ss;
  ss << t;
  return ss.str();
}

void LogPrint(const char* format, ...);

class ExtractorException : std::exception {
 public:
  ExtractorException(const std::string& msg) : msg_(msg) {}
  const char* what() const noexcept { return msg_.c_str(); }
 private:
  const std::string msg_;
};

float Coarseness(cv::Mat& mat, int r, int c);

float Contrast(cv::Mat& mat, int r, int c, int window);

template <typename T>
float EuclideanDistance(const T& x, const T& y) {
  assert(x.size() == y.size());
  float sum = 0;
  for (size_t i = 0; i < x.size(); ++i) {
    sum += sqr(x[i] - y[i]);
  }
  return sqrt(sum);
}

}

#endif    // _GLOBAL_H_
