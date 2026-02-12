/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/nmslib/nmslib
 *
 * Copyright (c) 2013-2026
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 * Contributor: Amrith Kumar
 */

#include <cmath>
#include <vector>
#include <random>

#include "bunit.h"
#include "distcomp.h"

namespace similarity {

using namespace std;

// Test L2SqrSIMDBatched correctness
TEST(L2SqrSIMDBatched_Correctness) {
  const size_t DIM = 128;
  float v1[DIM], v2[DIM];
  
  // Initialize with known values
  for (size_t i = 0; i < DIM; ++i) {
    v1[i] = static_cast<float>(i);
    v2[i] = static_cast<float>(i + 1);
  }
  
  // Compute reference (no threshold)
  float expected = 0.0f;
  for (size_t i = 0; i < DIM; ++i) {
    float diff = v1[i] - v2[i];
    expected += diff * diff;
  }
  
  // Test with high threshold (should not abort)
  float result = L2SqrSIMDBatched(v1, v2, DIM, 16, 1e10f);
  EXPECT_EQ_EPS(result, expected, 1e-3f);
  
  // Test with low threshold (should abort early, returning partial sum > threshold)
  result = L2SqrSIMDBatched(v1, v2, DIM, 16, 1.0f);
  EXPECT_EQ(result > 1.0f, true);
}

// Test L2SqrSIMDBatched with zero vectors
TEST(L2SqrSIMDBatched_ZeroVectors) {
  const size_t DIM = 64;
  float zeros[DIM] = {0};
  
  float result = L2SqrSIMDBatched(zeros, zeros, DIM, 16, 1e10f);
  EXPECT_EQ_EPS(result, 0.0f, 1e-5f);
}

// Test L2SqrSIMDBatched with different batch sizes
TEST(L2SqrSIMDBatched_BatchSizes) {
  const size_t DIM = 128;
  float v1[DIM], v2[DIM];
  
  for (size_t i = 0; i < DIM; ++i) {
    v1[i] = static_cast<float>(i % 10);
    v2[i] = static_cast<float>((i + 3) % 10);
  }
  
  // Compute reference
  float expected = 0.0f;
  for (size_t i = 0; i < DIM; ++i) {
    float diff = v1[i] - v2[i];
    expected += diff * diff;
  }
  
  // Test different batch sizes
  for (size_t batch_size : {4, 8, 16, 32}) {
    float result = L2SqrSIMDBatched(v1, v2, DIM, batch_size, 1e10f);
    EXPECT_EQ_EPS(result, expected, 1e-3f);
  }
}

// Test LInfNormSIMDBatched correctness
TEST(LInfNormSIMDBatched_Correctness) {
  const size_t DIM = 128;
  float v1[DIM], v2[DIM];
  
  for (size_t i = 0; i < DIM; ++i) {
    v1[i] = static_cast<float>(i);
    v2[i] = static_cast<float>(i + 1);
  }
  
  // Expected: max absolute difference is 1.0
  float expected = 1.0f;
  
  // Test with high threshold (should not abort)
  float result = LInfNormSIMDBatched(v1, v2, DIM, 16, 1e10f);
  EXPECT_EQ_EPS(result, expected, 1e-5f);
  
  // Test with low threshold (should abort early, returning value > threshold)
  result = LInfNormSIMDBatched(v1, v2, DIM, 16, 0.5f);
  EXPECT_EQ(result > 0.5f, true);
}

// Test LInfNormSIMDBatched with varying differences
TEST(LInfNormSIMDBatched_MaxDifference) {
  const size_t DIM = 64;
  float v1[DIM], v2[DIM];
  
  // Most differences are small, one is large
  for (size_t i = 0; i < DIM; ++i) {
    v1[i] = 1.0f;
    v2[i] = 1.1f;
  }
  v1[DIM/2] = 10.0f;
  v2[DIM/2] = 5.0f;
  
  float result = LInfNormSIMDBatched(v1, v2, DIM, 16, 1e10f);
  EXPECT_EQ_EPS(result, 5.0f, 1e-5f);
}

// Test NormScalarProductSIMDBatched correctness
TEST(NormScalarProductSIMDBatched_Correctness) {
  const size_t DIM = 128;
  float v1[DIM], v2[DIM];
  
  // Create normalized vectors
  float sum1 = 0.0f, sum2 = 0.0f;
  for (size_t i = 0; i < DIM; ++i) {
    v1[i] = static_cast<float>(i + 1);
    v2[i] = static_cast<float>(DIM - i);
    sum1 += v1[i] * v1[i];
    sum2 += v2[i] * v2[i];
  }
  
  float norm1 = sqrtf(sum1);
  float norm2 = sqrtf(sum2);
  for (size_t i = 0; i < DIM; ++i) {
    v1[i] /= norm1;
    v2[i] /= norm2;
  }
  
  // Compute expected dot product
  float expected = 0.0f;
  for (size_t i = 0; i < DIM; ++i) {
    expected += v1[i] * v2[i];
  }
  
  // Test with low threshold (should not abort)
  float result = NormScalarProductSIMDBatched(v1, v2, DIM, 16, -1.0f);
  EXPECT_EQ_EPS(result, expected, 1e-3f);
  
  // Test with high threshold (should abort early, returning partial dot product < threshold)
  result = NormScalarProductSIMDBatched(v1, v2, DIM, 16, 1.0f);
  EXPECT_EQ(result < 1.0f, true);
}

// Test NormScalarProductSIMDBatched with identical vectors
TEST(NormScalarProductSIMDBatched_Identical) {
  const size_t DIM = 64;
  float v[DIM];
  
  // Create normalized vector
  float sum = 0.0f;
  for (size_t i = 0; i < DIM; ++i) {
    v[i] = static_cast<float>(i + 1);
    sum += v[i] * v[i];
  }
  float norm = sqrtf(sum);
  for (size_t i = 0; i < DIM; ++i) {
    v[i] /= norm;
  }
  
  // Dot product of normalized vector with itself should be 1.0
  float result = NormScalarProductSIMDBatched(v, v, DIM, 16, -1.0f);
  EXPECT_EQ_EPS(result, 1.0f, 1e-5f);
}

// Test NormScalarProductSIMDBatched with orthogonal vectors
TEST(NormScalarProductSIMDBatched_Orthogonal) {
  const size_t DIM = 4;
  float v1[DIM] = {1.0f, 0.0f, 0.0f, 0.0f};
  float v2[DIM] = {0.0f, 1.0f, 0.0f, 0.0f};
  
  // Orthogonal vectors have dot product 0
  float result = NormScalarProductSIMDBatched(v1, v2, DIM, 2, -1.0f);
  EXPECT_EQ_EPS(result, 0.0f, 1e-5f);
}

// Test early abort behavior
TEST(BatchedSIMD_EarlyAbort) {
  const size_t DIM = 1024;
  float v1[DIM], v2[DIM];
  
  // Create vectors that will exceed threshold early
  for (size_t i = 0; i < DIM; ++i) {
    v1[i] = 100.0f;
    v2[i] = 0.0f;
  }
  
  // L2: Should abort early with low threshold, returning value > threshold
  float result = L2SqrSIMDBatched(v1, v2, DIM, 16, 100.0f);
  EXPECT_EQ(result > 100.0f, true);
  
  // LInf: Should abort early, returning value > threshold
  result = LInfNormSIMDBatched(v1, v2, DIM, 16, 50.0f);
  EXPECT_EQ(result > 50.0f, true);
}

}  // namespace similarity
