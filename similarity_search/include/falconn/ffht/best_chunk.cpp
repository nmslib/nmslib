#include "fht.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <map>

#include <cstdlib>

double measureTime(int n, int times, int chunk, bool isDouble) {
  if (isDouble) {
    double *a;
    void *aux;
    int res = posix_memalign(&aux, 32, std::max(n, 32) * sizeof(double));
    if (res) {
      throw std::runtime_error("error in posix_memalign");
    }
    a = (double*)aux;
    for (int i = 0; i < n; ++i) {
      a[i] = i + 1.0;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int it = 0; it < times; ++it) {
      if (FHTDouble(a, n, chunk) == -1) {
	throw std::runtime_error("error in FHT");
      }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    free(a);
    return std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  }
  else {
    float *a;
    void *aux;
    int res = posix_memalign(&aux, 32, std::max(n, 32) * sizeof(float));
    if (res) {
      throw std::runtime_error("error in posix_memalign");
    }
    a = (float*)aux;
    for (int i = 0; i < n; ++i) {
      a[i] = i + 1.0;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int it = 0; it < times; ++it) {
      if (FHTFloat(a, n, chunk) == -1) {
	throw std::runtime_error("error in FHT");
      }
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    free(a);
    return std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  }
}

double measureTime(int n, int chunk, bool isDouble) {
  int times = 10;
  for (;;) {
    double t = measureTime(n, times, chunk, isDouble);
    if (t >= 1.0) {
      return t / times;
    }
    times *= 2;
  }
}

int main() {
  try {
    int n;
    std::cout << "n = " << std::flush;
    std::cin >> n;
    if (n < 1) {
      throw std::runtime_error("invalid n");
    }
    if (n & (n - 1)) {
      throw std::runtime_error("not a power of 2");
    }
    for (int it = 0; it < 2; ++it) {
      bool isDouble = (bool)it;
      std::cout << "determining the best chunk size for "
		<< ((isDouble) ? "double" : "float")
		<< std::endl;
      int chunk = 8;
      std::map<int, double> data;
      for (;;) {
	std::cout << "chunk size " << chunk << ": ";
	data[chunk] = measureTime(n, chunk, isDouble);
	std::cout << data[chunk] << std::endl;
	if (chunk >= n) {
	  break;
	}
	chunk *= 2;
      }
      int best_chunk = -1;
      double best_time = 1e100;
      for (auto it = data.begin(); it != data.end(); ++it) {
	if (it->second < best_time) {
	  best_time = it->second;
	  best_chunk = it->first;
	}
      }
      std::cout << "best chunk: " << best_chunk << std::endl;
      std::cout << "best time: " << best_time << std::endl;
    }
  }
  catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  catch (...) {
    std::cerr << "ERROR" << std::endl;
    return 1;
  }
  return 0;
}
