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
#include <fstream>
#include <string>
#include <sstream>
#include <Eigen/Dense>

#include "object.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"
#include "space/space_sqfd.h"

namespace similarity {

using Eigen::MatrixXd;
using Eigen::VectorXd;

template <typename dist_t>
SpaceSqfd<dist_t>::SpaceSqfd(SqfdFunction<dist_t>* func)
    : func_(func) {
}

template <typename dist_t>
SpaceSqfd<dist_t>::~SpaceSqfd() {
  delete func_;
}

template <typename dist_t>
void SpaceSqfd<dist_t>::ReadDataset(
    ObjectVector& dataset,
    const ExperimentConfig<dist_t>* config,
    const char* FileName,
    const int MaxNumObjects) const {
  dataset.clear();
  dataset.reserve(MaxNumObjects);

  std::ifstream infile(FileName);

  if (!infile) {
      LOG(LIB_FATAL) << "Cannot open file: " << FileName;
  }

  infile.exceptions(std::ios::badbit);

  try {
    int linenum = 0;
    std::string line;
    // header information
    getline(infile, line);
    linenum++;
    std::stringstream ss(line);
    ss.exceptions(std::ios::badbit);
    int num_clusters, feature_dimension, num_rand_pixels;
    ss >> num_clusters >> feature_dimension >> num_rand_pixels;
    LOG(LIB_INFO) << "header information\n"
        << "num_clusters = " << num_clusters << " "
        << "feature_dimension = " << feature_dimension << " "
        << "num_rand_pixels = " << num_rand_pixels;
    // empty line
    getline(infile, line);
    linenum++;
    if (line.length() != 0) {
      LOG(LIB_FATAL) << "Expected empty line at line " << linenum;
    }
    // objects
    int id = 0;
    const int feature_weight = feature_dimension + 1;   // feature+weight
    const int object_size =
        2 * sizeof(int) + // num_clusters & feature_dimension
        num_clusters * feature_weight * sizeof(dist_t);
    std::vector<char> buf(object_size);
    int* h = reinterpret_cast<int*>(&buf[0]);
    h[0] = num_clusters;
    h[1] = feature_dimension;
    dist_t* obj = reinterpret_cast<dist_t*>(&buf[2*sizeof(int)]);

    while (getline(infile, line) /* image file name */ &&
           (!MaxNumObjects || static_cast<int>(dataset.size()) < MaxNumObjects)) {
      linenum++;
      int pos = 0;
      for (int i = 0; i < num_clusters; ++i) {
        getline(infile, line);
        linenum++;
        std::stringstream ss(line);
        ss.exceptions(std::ios::badbit);
        int n = 0;
        dist_t v;
        while (ss >> v) {
          obj[pos++] = v;
          n++;
        }
        if (n != feature_weight) {
          LOG(LIB_FATAL) << "Found incorrectly line at " << linenum;
        }
      }
      // empty line
      if (!getline(infile, line) || line.length() != 0) {
        LOG(LIB_FATAL) << "Expected empty line";
      }
      dataset.push_back(new Object(id++, -1, object_size, &buf[0]));
    }
  } catch (const std::exception &e) {
    LOG(LIB_ERROR) << "Exception: " << e.what();
    LOG(LIB_FATAL) << "Failed to read/parse the file: '" << FileName << "'";
  }
}

template <typename dist_t>
dist_t SpaceSqfd<dist_t>::HiddenDistance(
    const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj2->datalength() > 0);
  const int* h1 = reinterpret_cast<const int*>(obj1->data());
  const int* h2 = reinterpret_cast<const int*>(obj2->data());
  const int num_clusters1 = h1[0], feature_dimension1 = h1[1];
  const int num_clusters2 = h2[0], feature_dimension2 = h2[1];
  const dist_t* x = reinterpret_cast<const dist_t*>(
      obj1->data() + 2*sizeof(int));
  const dist_t* y = reinterpret_cast<const dist_t*>(
      obj2->data() + 2*sizeof(int));
  CHECK(feature_dimension1 == feature_dimension2);
  const int sz = num_clusters1 + num_clusters2;
  VectorXd W(sz);
  int pos = feature_dimension1;
  for (int i = 0; i < num_clusters1; ++i) {
    W(i) = x[pos];
    pos += feature_dimension1 + 1;
  }
  pos = feature_dimension2;
  for (int i = 0; i < num_clusters2; ++i) {
    W(num_clusters1 + i) = -y[pos];
    pos += feature_dimension2 + 1;
  }
  MatrixXd A(sz,sz);
  for (int i = 0; i < sz; ++i) {
    for (int j = i; j < sz; ++j) {
      const dist_t* p1 = i < num_clusters1
          ? x + i * (feature_dimension1 + 1)
          : y + (i - num_clusters1) * (feature_dimension1 + 1);
      const dist_t* p2 = j < num_clusters1
          ? x + j * (feature_dimension1 + 1)
          : y + (j - num_clusters1) * (feature_dimension1 + 1);
      A(i,j) = A(j,i) = func_->f(p1, p2, feature_dimension1);
    }
  }
  auto res = W.transpose() * A * W;
  return res(0,0);
}

template <typename dist_t>
std::string SpaceSqfd<dist_t>::ToString() const {
  std::stringstream stream;
  stream << "SpaceSqfd: " << func_->ToString();
  return stream.str();
}

template class SpaceSqfd<float>;
template class SpaceSqfd<double>;
}  // namespace similarity
