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
#include "global.h"
#include "lab.h"
#include "extractor.h"
#include "distance.h"
#include "utils.h"

/**
 * Signature Quadratic Form Distance
 *    See Section 3.4: http://darwin.bth.rwth-aachen.de/opus3/volltexte/2013/4807/
 *    Also http://dme.rwth-aachen.de/en/system/files/file_upload/publications/p438_Beecks.pdf
 */

using namespace sqfd;

std::mutex m;
std::queue<std::string> remaining;

std::pair<std::string, bool> GetNext() {
  std::unique_lock<std::mutex> lock(m);
  if (remaining.empty()) {
    return std::make_pair("", false);
  }
  std::string filename = remaining.front();
  remaining.pop();
  return std::make_pair(filename, true);
}

void Run(int num_clusters,
         int num_rand_pixels,
         FileWriter* writer) {
  for (;;) {
    auto next = GetNext();
    if (!next.second) {
      return;
    }
    try {
      FeatureExtractor extractor(
          next.first, num_clusters, num_rand_pixels);
      extractor.Extract();
      writer->Write(next.first, extractor.GetClusters());
    } catch (ExtractorException& ex) {
      LogPrint("FAILED: %s", ex.what());
    }
  }
}

void RunParallel(int num_clusters,
                 int num_rand_pixels,
                 FileWriter* writer) {
  size_t n = std::thread::hardware_concurrency() >= 4 ?
      std::thread::hardware_concurrency() - 1 : 1;
  std::vector<std::thread> threads;
  for (size_t i = 0; i < n; ++i) {
    threads.emplace_back(Run, num_clusters, num_rand_pixels, writer);
  }
  for (auto& thread : threads) {
    thread.join();
  }
}

void DistExampleFromPaper() {
  std::shared_ptr<SimilarityFunction> simfunc(new HeuristicFunction(1.0));

  VRR cq = {{3,3}, {8,7}};
  VR wq = {0.5, 0.5};
  auto q = FeatureSignaturePtr(new FeatureSignature(cq, wq));

  VRR co = {{4,7}, {9,5}, {8,1}};
  VR wo = {0.5, 0.25, 0.25};
  auto o = FeatureSignaturePtr(new FeatureSignature(co, wo));

  float d = SQFD(simfunc, q, o);
  if (fabs(d - 0.808) > kEPS) {
    std::cerr << "incorrect distance " << d << std::endl;
    exit(1);
  }
  std::cout << d << std::endl;

  /*
  >>> import numpy as np
  >>> import math
  >>> w = np.array([0.5,0.5,-0.5,-0.25,-0.25])
  >>> a = np.array([[1.0, 0.135, 0.195, 0.137, 0.157],
                    [0.135, 1.0, 0.2, 0.309, 0.143],
                    [0.195, 0.2, 1.0, 0.157, 0.122],
                    [0.137, 0.309, 0.157, 1.0, 0.195],
                    [0.157, 0.143, 0.122, 0.195, 1.0]])
  >>> math.sqrt(w.dot(a).dot(w.transpose()))
  0.807
  */
}

int main(int argc, char* argv[]) {
  if (argc != 5) {
    std::cout << "usage: " << argv[0] << " "
              << "<num_clusters> <num_rand_pixels> "
              << "<image_lists_file> <output_file>" << std::endl;
    exit(1);
  }
  int num_clusters = atoi(argv[1]);
  int num_rand_pixels = atoi(argv[2]);
  std::ifstream listfile(argv[3]);
  std::string line;
  while (std::getline(listfile, line)) {
    remaining.push(line);
  }
  listfile.close();
  FileWriter writer(argv[4], num_clusters, num_rand_pixels);
  RunParallel(num_clusters, num_rand_pixels, &writer);
}
