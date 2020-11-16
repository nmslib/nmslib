/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/nmslib/nmslib
 *
 * Copyright (c) 2013-2018
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
  float weight;
  Feature center;
  Feature coords_sum;

  Cluster(const Feature& f) {
    num_points = 0;
    weight = 0.0;
    for (int i = 0; i < kFeatureDims; ++i) {
      center[i] = f[i];
      coords_sum[i] = 0;
    }
  }

  void Clear() {
    num_points = 0;
    weight = 0.0;
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

  void Update(size_t tot_points) {
    if (num_points > 0) {
      for (int i = 0; i < kFeatureDims; ++i) {
        center[i] = coords_sum[i] / num_points;
      }
      weight = static_cast<float>(num_points) / tot_points;
    }
  }

  Float3 asLab() { return Float3{{center[0], center[1], center[2]}}; }
  float row() { return center[3]; }
  float col() { return center[4]; }
};

class FileWriter {
 public:
  FileWriter(const std::string& output_file,
             const int num_clusters,
             const int num_rand_pixels);
  ~FileWriter();
  void Write(const std::string& image_file,
             const std::vector<Cluster>& clusters);
 private:
  std::ofstream out_;
  std::mutex mutex_;
};

class FeatureExtractor {
 public:
  FeatureExtractor(const std::string& image_file,
                   const int num_clusters,
                   const int num_rand_pixels);
  ~FeatureExtractor();
  const std::vector<Cluster>& GetClusters() const;
  void Extract();
  void Visualize(std::string output_file, int bubble_radius);
 private:
  int num_clusters_;
  int rows_;
  int cols_;
  std::vector<Feature> features_;
  std::vector<Cluster> clusters_;
};

}

#endif    // _EXTRACTOR_H_
