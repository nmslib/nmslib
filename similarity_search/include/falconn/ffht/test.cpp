#include "fht.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

#include <cstdlib>

template<typename T> void referenceFHTHelper(T *buffer, int len) {
  if (len == 1) {
    return;
  }
  int hl = len / 2;
  referenceFHTHelper(buffer, hl);
  referenceFHTHelper(buffer + hl, hl);
  for (int i = 0; i < hl; ++i) {
    T u = buffer[i];
    T v = buffer[i + hl];
    buffer[i] = u + v;
    buffer[i + hl] = u - v;
  }
}

template<typename T> void referenceFHT(T *buffer, int len) {
  if (len < 1 || (len & (len - 1))) {
    throw std::runtime_error("invalid length in the model FFT");
  }
  referenceFHTHelper(buffer, len);
  T s = 1.0 / sqrt(len + 0.0);
  for (int i = 0; i < len; ++i) {
    buffer[i] *= s;
  }
}

void testFloat(int n, int chunk) {
  std::cout << "testing float: (" << n << ", " << chunk << ")" << std::endl;
  float *a;
  void *aux;
  int res = posix_memalign(&aux, 32, std::max(n, 32) * sizeof(float));
  if (res) {
    throw std::runtime_error("error in posix_memalign");
  }
  a = (float*)aux;
  for (int i = 0; i < n; ++i) {
    a[i] = sqrt(i + 0.0);
  }
  if (FHTFloat(a, n, chunk)) {
    throw std::runtime_error("error in FHT");
  }
  referenceFHT(a, n);
  for (int i = 0; i < n; ++i) {
    if (fabs(a[i] - sqrt(i + 0.0)) > 0.001) {
      throw std::runtime_error("bug");
    }
  }
  free(a);
}

void testDouble(int n, int chunk) {
  std::cout << "testing double: (" << n << ", " << chunk << ")" << std::endl;
  double *a;
  void *aux;
  int res = posix_memalign(&aux, 32, std::max(n, 32) * sizeof(double));
  if (res) {
    throw std::runtime_error("error in posix_memalign");
  }
  a = (double*)aux;
  for (int i = 0; i < n; ++i) {
    a[i] = sqrt(i + 0.0);
  }
  if (FHTDouble(a, n, chunk)) {
    throw std::runtime_error("error in FHT");
  }
  referenceFHT(a, n);
  for (int i = 0; i < n; ++i) {
    if (fabs(a[i] - sqrt(i + 0.0)) > 0.001) {
      throw std::runtime_error("bug");
    }
  }
  free(a);
}

int main() {
  try {
    for (int n = 1; n <= (1 << 20); n *= 2) {
      int chunk = 8;
      for (;;) {
	testFloat(n, chunk);
	testDouble(n, chunk);
	if (chunk > n) {
	  break;
	}
	chunk *= 2;
      }
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
