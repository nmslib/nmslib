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

namespace similarity {

using namespace std;

template <typename T> T alpha_beta_divergence(const T* x, const T* y, const int length, float alpha, float beta) {
  T res = 0;
  for (int i = 0; i < length; ++i) {
    res += pow(x[i], (T)alpha)*pow(y[i], beta);  
  } 
  return res;
}

template  float alpha_beta_divergence(const float* x, const float* y, const int length, float alpha, float beta);
template  double alpha_beta_divergence(const double* x, const double* y, const int length, float alpha, float beta);

}
