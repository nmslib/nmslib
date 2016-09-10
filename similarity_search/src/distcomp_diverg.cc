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

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <cmath>

#include "distcomp.h"
#include "logging.h"
#include "utils.h"

namespace similarity {

using namespace std;

template <typename T> T alpha_beta_divergence(const T* x, const T* y, const int length, float alpha, float beta) {
  T res = 0;
  float alpha1 = alpha + 1;
  for (int i = 0; i < length; ++i) {
    res += pow(x[i], alpha1)*pow(y[i], beta);  
  } 
  return res;
}

template  float alpha_beta_divergence(const float* x, const float* y, const int length, float alpha, float beta);
template  double alpha_beta_divergence(const double* x, const double* y, const int length, float alpha, float beta);

template <typename T> T renyi_divergence(const T* x, const T* y, const int length, float alpha) {
  T sum = 0;
  T eps = -1e-6;
  float t = alpha-1; 
  for (int i = 0; i < length; ++i) {
    sum += x[i]*pow(x[i]/y[i], t);
  } 
  float res = 1/t * log(sum);
  CHECK_MSG(res >= eps, "Expected a non-negative result, but got " + ConvertToString(res) + " for alpha="  + ConvertToString(alpha));
  // Might be slightly negative due to rounding errors
  return max<T>(0, res);
}

template  float renyi_divergence(const float* x, const float* y, const int length, float alpha);
template  double renyi_divergence(const double* x, const double* y, const int length, float alpha);

}
