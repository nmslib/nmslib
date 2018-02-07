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
#include <stdio.h>
#include <stdarg.h>
#include "global.h"

namespace sqfd {

void LogPrint(const char* format, ...) {
  static std::mutex m;
  std::lock_guard<std::mutex> lock(m);
  va_list args;
  va_start(args, format);
  char buffer[1024];
  vsprintf(buffer, format, args);
  va_end(args);
  fprintf(stdout, "%s\n", buffer);
}

// Tamura et al., Textural Features Corresponding to Visual Perception

float AverageNeighborhood(cv::Mat& mat, int r, int c, int k) {
  if (r < 0 || r >= mat.rows || c < 0 || c >= mat.cols) {
    return 0;
  }
  assert(k >= 1);
  const int shift = 1 << (k - 1);
  const int beg_r = std::max(0, r - shift);
  const int end_r = std::min(mat.rows - 1, r + shift - 1);
  const int beg_c = std::max(0, c - shift);
  const int end_c = std::min(mat.cols - 1, c + shift - 1);
  float avg = 0;
  for (int i = beg_r; i <= end_r; ++i) {
    for (int j = beg_c; j <= end_c; ++j) {
      avg += static_cast<int>(mat.at<uchar>(i, j));
    }
  }
  avg /= (end_r - beg_r + 1) * (end_c - beg_c + 1);
  return avg;
}

float DiffNeighborhood(cv::Mat& mat, int r, int c, int k) {
  assert(k >= 1);
  const int shift = 1 << (k - 1);     // 2^(k-1)
  float diff_vertical = std::abs(
      AverageNeighborhood(mat, r + shift, c, k) -
      AverageNeighborhood(mat, r - shift, c, k));
  float diff_horizontal = std::abs(
      AverageNeighborhood(mat, r, c + shift, k) -
      AverageNeighborhood(mat, r, c - shift, k));
  return std::max(diff_vertical, diff_horizontal);
}

float Coarseness(cv::Mat& mat, int r, int c) {
  int best_k = 0;
  float best_val = 0;
  const static int K = 5;
  for (int k = 1; k <= K; ++k) {
    float val = DiffNeighborhood(mat, r, c, k);
    if (val > best_val) {
      best_val = val;
      best_k = k;
    }
  }
  return static_cast<float>(1 << best_k) / (1 << K);
}

float Contrast(cv::Mat& mat, int r, int c, int window) {
  const int beg_r = std::max(0, r - window / 2);
  const int end_r = std::min(mat.rows - 1, r + window / 2);
  const int beg_c = std::max(0, c - window / 2);
  const int end_c = std::min(mat.cols - 1, c + window / 2);
  float mean = 0;
  for (int i = beg_r; i <= end_r; ++i) {
    for (int j = beg_c; j <= end_c; ++j) {
      mean += static_cast<float>(mat.at<uchar>(i, j));
    }
  }
  mean /= (end_r - beg_r + 1) * (end_c - beg_c + 1);
  float variance = 0;
  float fourth_moment = 0;
  for (int i = beg_r; i <= end_r; ++i) {
    for (int j = beg_c; j <= end_c; ++j) {
      float diff = static_cast<float>(mat.at<uchar>(i, j)) - mean;
      variance += sqr(diff);             // diff^2
      fourth_moment += sqr(sqr(diff));   // diff^4
    }
  }
  variance /= window * window;
  fourth_moment /= window * window;
  return fourth_moment == 0 ? 0 : variance / pow(fourth_moment, 1.0/4.0);
}

}
