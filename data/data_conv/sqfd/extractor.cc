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
#include "extractor.h"
#include "lab.h"
#include "utils.h"

namespace sqfd {

std::mt19937 rnd;
std::uniform_int_distribution<unsigned> dist;

FeatureExtractor::FeatureExtractor(
    const std::string& image_file,
    const int num_clusters,
    const int num_rand_pixels)
  : num_clusters_(num_clusters) {
  cv::Mat img = cv::imread(image_file.c_str(), CV_LOAD_IMAGE_COLOR);
  if (!img.data) {
    std::stringstream ss;
    ss << "failed to load image file " << image_file;
    throw ExtractorException(ss.str());
  }
  assert(img.type() == CV_8UC3);
  cv::Mat img_gray;
  cv::cvtColor(img, img_gray, CV_BGR2GRAY);
  rows_ = img.rows;
  cols_ = img.cols;
  const int total_pixels = rows_ * cols_;
  if (total_pixels < num_rand_pixels) {
    std::stringstream ss;
    ss << "too small image " << image_file;
    throw ExtractorException(ss.str());
  }
  std::unordered_set<PairII, PairIIHash, PairIIEqual> selected_positions;
  for (int i = 0; i < num_rand_pixels; ++i) {
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
    // http://docs.opencv.org/doc/tutorials/introduction/load_save_image/load_save_image.html
    // imread has BGR default channel order in case of color images
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

const std::vector<Cluster>& FeatureExtractor::GetClusters() const {
  return clusters_;
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
      c.Update(features_.size());
    }
    if (fabs(prev_error - error) <= kEPS) {
      break;
    }
  }
}

void FeatureExtractor::Visualize(
    std::string output_file, int bubble_radius) {
  std::vector<Cluster> clusters(clusters_);
  std::sort(clusters.begin(), clusters.end(),
            [](const Cluster& x, const Cluster& y) {
              return x.weight > y.weight;
            });
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
    int radius = c.weight * bubble_radius;
    cv::circle(feature_img, cv::Point(c.col(), c.row()), radius,
               cv::Scalar(blue, green, red), CV_FILLED);
    cv::circle(feature_img, cv::Point(c.col(), c.row()), radius,
               cv::Scalar(0,0,0));
  }
  LogPrint("%s", output_file.c_str());
  cv::imwrite(output_file.c_str(), feature_img);
}

FileWriter::FileWriter(
    const std::string& output_file,
    const int num_clusters,
    const int num_rand_pixels) {
  out_.open(output_file);
  out_ << num_clusters << " "
       << kFeatureDims << " "
       << num_rand_pixels << std::endl << std::endl;
}

FileWriter::~FileWriter() {
  out_.close();
}

void FileWriter::Write(const std::string& image_file,
                       const std::vector<Cluster>& clusters) {
  std::lock_guard<std::mutex> lock(mutex_);
  out_ << GetBasename(image_file) << std::endl;
  for (const auto& c : clusters) {
    for (int i = 0; i < kFeatureDims; ++i) {
      out_ << c.center[i] << " ";
    }
    out_ << c.weight << std::endl;
  }
  out_ << std::endl;
}

}
