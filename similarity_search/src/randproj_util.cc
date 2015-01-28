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

#include <cmath>
#include <random>
#include <vector>

#include "randproj_util.h"

namespace similarity {

using namespace std;

template <class dist_t> void initRandProj(size_t nVect, size_t nElem, vector<vector<dist_t>>& res) {
  // Static is thread-safe in C++-11
  static  std::random_device          rd;
  static  std::mt19937                engine(rd());
  static  std::normal_distribution<>  normGen(0.0f, 1.0f);

  res.resize(nVect);
  for (size_t i = 0; i < nVect; ++i) {
    res[i].resize(nElem);
    for (size_t j = 0; j < nElem; ++j)
      res[i][j] = normGen(engine);
  }
}

template void initRandProj<float>(size_t nVect, size_t nElem, vector<vector<float>>& res);
template void initRandProj<double>(size_t nVect, size_t nElem, vector<vector<double>>& res);

}
