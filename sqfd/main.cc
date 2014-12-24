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

void Run(std::string outdir, int num_clusters) {
  for (;;) {
    auto next = GetNext();
    if (!next.second) {
      return;
    }
    try {
      FeatureExtractor feature_extractor(outdir, next.first, num_clusters);
      feature_extractor.Extract();
      feature_extractor.Print();
    } catch (ExtractorException& ex) {
      LogPrint("FAILED: %s", ex.what());
    }
  }
}

void RunParallel(std::string indir, std::string outdir, int num_clusters) {
  size_t n = std::thread::hardware_concurrency() >= 4 ?
      std::thread::hardware_concurrency() - 1 : 1;
  std::cout << "n " << n << std::endl;
  if (!IsDirectoryExists(outdir)) {
    MakeDirectory(outdir);
  }
  auto filenames = GetImageFiles(indir);
  for (auto f : filenames) {
    remaining.push(f);
  }
  std::vector<std::thread> threads;
  for (size_t i = 0; i < n; ++i) {
    threads.emplace_back(Run, outdir, num_clusters);
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

  //q->Print();
  //o->Print();

  float d = SQFD(simfunc, q, o);   // it gives 0.652 not 0.808
  if (fabs(d - 0.652) > kEPS) {
    std::cerr << "incorrect distance " << d << std::endl;
    exit(1);
  }
  std::cout << d << std::endl;

  /*
  >>> import numpy as np
  >>> w = np.array([0.5,0.5,-0.5,-0.25,-0.25])
  >>> a = np.array([[1.0, 0.135, 0.195, 0.137, 0.157],
                    [0.135, 1.0, 0.2, 0.309, 0.143],
                    [0.195, 0.2, 1.0, 0.157, 0.122],
                    [0.137, 0.309, 0.157, 1.0, 0.195],
                    [0.157, 0.143, 0.122, 0.195, 1.0]])
  >>> w.dot(a).dot(w.transpose())
  0.652625
  */
}

void DistSample(const std::string dirn) {
  std::vector<std::string> files;
  std::vector<FeatureSignaturePtr> feats;

  for (auto filename : GetAllFiles(dirn)) {
    if (Endswith(filename, ".feat")) {
      files.push_back(filename);
      feats.push_back(readFeature(filename));
    }
  }

  std::cout << "read " << feats.size() << " features" << std::endl;

  //std::shared_ptr<SimilarityFunction> simfunc(new HeuristicFunction(2.0));
  std::shared_ptr<SimilarityFunction> simfunc(new GaussianFunction(1.0));

  for (size_t i = 0; i < feats.size(); ++i) {
    for (size_t j = i + 1; j < feats.size(); ++j) {
      std::cout << files[i] << " "
                << files[j] << " "
                << SQFD(simfunc, feats[i], feats[j]) << std::endl;
    }
  }
}

int main() {
  DistExampleFromPaper();
  RunParallel("sample", "sample/feat", 100);
  DistSample("sample/feat");
}
