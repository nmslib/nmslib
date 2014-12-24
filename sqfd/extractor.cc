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

#include "extractor.h"
#include "lab.h"
#include "utils.h"

namespace sqfd {

std::mt19937 rnd;
std::uniform_int_distribution<unsigned> dist;

FeatureExtractor::FeatureExtractor(
    const std::string& outdir,
    const std::string& filename,
    const int num_clusters) {
  feature_dir_ = outdir;
  if (feature_dir_[feature_dir_.size() - 1] != '/') {
    feature_dir_ += "/";
  }
  feature_file_  = feature_dir_ + GetBasename(filename) + "_" +
      ToString(num_clusters) + ".feat";
  num_clusters_ = num_clusters;
  if (IsFileExists(feature_file_)) {
    std::stringstream ss;
    ss << "feature file " << filename << " already exists";
    throw ExtractorException(ss.str());
  }
  cv::Mat img = cv::imread(filename.c_str(), CV_LOAD_IMAGE_COLOR);
  if (!img.data) {
    std::stringstream ss;
    ss << "failed to load image file " << filename;
    throw ExtractorException(ss.str());
  }
  assert(img.type() == CV_8UC3);
  cv::Mat img_gray;
  cv::cvtColor(img, img_gray, CV_BGR2GRAY);
  rows_ = img.rows;
  cols_ = img.cols;
  const int total_pixels = rows_ * cols_;
  if (total_pixels < kSelectRandPixels) {
    std::stringstream ss;
    ss << "too small image " << filename;
    throw ExtractorException(ss.str());
  }
  std::unordered_set<PairII, PairIIHash, PairIIEqual> selected_positions;
  for (int i = 0; i < kSelectRandPixels; ++i) {
    for (;;) {
      int r = dist(rnd) % rows_;
      int c = dist(rnd) % cols_;
      PairII pixel = PairII(r, c);
      if (selected_positions.count(pixel) == 0) {
        selected_positions.insert(pixel);
        break;
      }
    }
  }
  for (auto p : selected_positions) {
    auto pixel = img.at<cv::Vec3b>(p.first, p.second);
    float b = pixel[0];
    float g = pixel[1];
    float r = pixel[2];
    auto lab = RgbToLab(Float3{{r, g, b}});
    float contrast = Contrast(img_gray, p.first, p.second, kWindowSize);
    float coarseness = Coarseness(img_gray, p.first, p.second);
    features_.push_back(
        Feature{{Normalize(lab[0], kMinL, kMaxL),
                 Normalize(lab[1], kMinA, kMaxA),
                 Normalize(lab[2], kMinB, kMaxB),
                 Normalize(p.first, 0, img.rows),
                 Normalize(p.second, 0, img.cols),
                 Normalize(contrast, kMinContrast, kMaxContrast),
                 Normalize(coarseness, kMinCoarseness, kMaxCoarseness)}});
  }
  // select cluster centers
  std::unordered_set<int> selected_center_ids;
  for (int i = 0; i < num_clusters_; ++i) {
    for (;;) {
      unsigned idx = dist(rnd) % features_.size();
      if (selected_center_ids.count(idx) == 0) {
        selected_center_ids.insert(idx);
        break;
      }
    }
  }
  for (auto idx : selected_center_ids) {
    clusters_.emplace_back(features_[idx]);
  }
}

FeatureExtractor::~FeatureExtractor() {
}

void FeatureExtractor::Extract() {
  // k-means
  float error = 1e8;
  for (int iter = 0; iter < kMaxIter; ++iter) {
    for (auto& c : clusters_) {
      c.Clear();
    }
    float prev_error = error;
    error = 0;
    for (const auto& ft : features_) {
      size_t min_center = 0;
      float min_dist = -1.0;
      for (size_t k = 0; k < clusters_.size(); ++k) {
        float dist = EuclideanDistance(clusters_[k].center, ft);
        if (min_dist < 0.0 || dist < min_dist) {
          min_dist = dist;
          min_center = k;
        }
      }
      clusters_[min_center].Add(ft);
      error += sqr(min_dist);
    }
    for (auto& c : clusters_) {
      c.Update();
    }
    if (fabs(prev_error - error) <= kEPS) {
      break;
    }
  }
}

void FeatureExtractor::Print() {
  LogPrint("feature file %s", feature_file_.c_str());
  std::ofstream out;
  out.open(feature_file_);
  out << clusters_.size() << " " << kFeatureDims << std::endl;
  for (const auto& c : clusters_) {
    c.Print(out);
  }
  out.close();
}

void FeatureExtractor::Visualize(int bubble_radius) {
  std::vector<Cluster> clusters(clusters_);
  std::sort(clusters.begin(), clusters.end(),
            [](const Cluster& x, const Cluster& y) {
              return x.weight() > y.weight();
            });

  for (auto& c : clusters) {
    c.Print();
  }
  cv::Mat feature_img(rows_, cols_, CV_8UC3);
  feature_img.setTo(cv::Scalar(255,255,255));
  for (auto& c : clusters) {
    c.center[0] = Denormalize(c.center[0], kMinL, kMaxL);
    c.center[1] = Denormalize(c.center[1], kMinA, kMaxA);
    c.center[2] = Denormalize(c.center[2], kMinB, kMaxB);
    c.center[3] = Denormalize(c.center[3], 0, rows_);
    c.center[4] = Denormalize(c.center[4], 0, cols_);
  }
  for (auto c : clusters) {
    auto rgb = LabToRgb(c.asLab());
    int red = rgb[0];
    int green = rgb[1];
    int blue = rgb[2];
    int radius = c.weight() * bubble_radius;
    cv::circle(feature_img, cv::Point(c.col(), c.row()), radius,
               cv::Scalar(blue, green, red), CV_FILLED);
    cv::circle(feature_img, cv::Point(c.col(), c.row()), radius,
               cv::Scalar(0,0,0));
  }
  std::string outfile = feature_file_ + ".jpg";
  LogPrint("%s", outfile.c_str());
  cv::imwrite(outfile.c_str(), feature_img);
}

}
