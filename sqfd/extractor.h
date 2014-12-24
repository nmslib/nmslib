/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2015
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _EXTRACTOR_H_
#define _EXTRACTOR_H_

#include "global.h"

namespace sqfd {

struct Cluster {
  //  0,1,2, 3,  4,    5,       6
  // <L,a,b,row,col,contrast,coarseness>
  int num_points;
  Feature center;
  Feature coords_sum;

  Cluster(const Feature& f) {
    for (int i = 0; i < kFeatureDims; ++i) {
      center[i] = f[i];
      coords_sum[i] = 0;
    }
    num_points = 0;
  }

  void Clear() {
    num_points = 0;
    for (int i = 0; i < kFeatureDims; ++i) {
      coords_sum[i] = 0;
    }
  }

  void Add(const Feature& f) {
    num_points++;
    for (int i = 0; i < kFeatureDims; ++i) {
      coords_sum[i] += f[i];
    }
  }

  void Update() {
    if (num_points > 0) {
      for (int i = 0; i < kFeatureDims; ++i) {
        center[i] = coords_sum[i] / num_points;
      }
    }
  }

  void Print() const {
    std::cout << num_points << "\t";
    for (int i = 0; i < kFeatureDims; ++i) {
      std::cout << center[i] << " ";
    }
    std::cout << " weight=" << weight() << std::endl;
  }

  void Print(std::ofstream& out) const {
    for (int i = 0; i < kFeatureDims; ++i) {
      if (i) out << " ";
      out << center[i];
    }
    out << "\t" << weight() << std::endl;
  }

  inline float weight(int norm_val = kSelectRandPixels) const {
    return static_cast<float>(num_points) / norm_val;
  }

  Float3 asLab() { return Float3{{center[0], center[1], center[2]}}; }
  float row() { return center[3]; }
  float col() { return center[4]; }
};

class FeatureExtractor {
 public:
  FeatureExtractor(const std::string& outdir,
                   const std::string& filename,
                   const int num_clusters);
  ~FeatureExtractor();

  void Extract();
  void Print();
  void Visualize(int bubble_radius);

 private:
  std::string feature_dir_;
  std::string feature_file_;
  int num_clusters_;
  int rows_;
  int cols_;
  std::vector<Feature> features_;
  std::vector<Cluster> clusters_;
};

}

#endif    // _EXTRACTOR_H_
