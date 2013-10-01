/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <math.h>
#include <string.h>       // for strlen
#include <time.h>

#include <sys/time.h>

#ifdef _MSC_VER
#include <io.h>
#ifndef F_OK
#define F_OK 0
#include <direct.h>       // for mkdir
#define mkdir(name, mode) mkdir(name)
#endif
#else
#include <unistd.h>
#include <sys/stat.h>     // for mkdir
#endif

#include "global.h"       // for snprintf
#include "mt19937ar.h"
#include "utils.h"
#include "logging.h"

namespace similarity {

const char* CurrentTime() {
  static char buffer[255];
  time_t now;
  time(&now);
  snprintf(buffer, sizeof(buffer), "%s", ctime(&now));
  return buffer;
}

bool CreateDir(const char* name, int mode) {
  return mkdir(name, mode) == 0;
}

const char* GetFileName(const char* fullpath) {
  for (int i = strlen(fullpath) - 1; i >= 0; --i) {
    if (fullpath[i] == '\\' || fullpath[i] == '/') {
      return fullpath + i + 1;
    }
  }
  return fullpath;
}

bool IsFileExists(const char* filename) {
  return access(filename, F_OK) == 0;
}

// Mersenne-Twister
static unsigned long init[4] = {0x123, 0x234, 0x345, 0x456};
static unsigned long length = 4;
static bool inited = false;
inline static void init_rand() {
  if (!inited) {
    inited = true;
    // init_by_array(init, length);
    // fprintf(stderr, "MT rand generator initialized ...\n");
    unsigned seed = time(NULL);
    init_genrand(seed);
    LOG(INFO) << "MT rand generator initialized ... seed = " <<  seed;
  }
}

void RandomReset() {
  printf("No RandomReset\n");
  // inited = false;
}

int RandomInt() {
  init_rand();
  // return genrand_int32();
  return genrand_int31();
}

double RandomReal() {
  init_rand();
  return genrand_real2();
}

void GenUniform(const char* filename, const int total, const int dimension,
                const double maxrange) {
  printf("generating uniformly distributed data .......\n");
  printf("filename  = %s\n", filename);
  printf("total     = %d\n", total);
  printf("dimension = %d\n", dimension);
  printf("maxrange  = %lf\n", maxrange);   // [0,maxrange)

  init_by_array(init, length);
  FILE* fp = fopen(filename, "wt");
  for (int i = 0; i < total; ++i) {
    for (int j = 0; j < dimension; ++j) {
      fprintf(fp, "%lf\t", genrand_real2() * maxrange);
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
}

void RStrip(char* str) {
  int i = strlen(str) - 1;
  while ((i >= 0) &&
         (str[i] == '\r' || str[i] == '\n' || str[i] == ' ' || str[i] == '\t'))
    str[i--] = '\0';
}

double Mean(const double* array, const unsigned size) {
  double result = 0.0;
  for (unsigned i = 0; i < size; ++i)
    result += array[i];
  return result / size;
}

double Variance(const double* array, const unsigned size, const double mean) {
  double result = 0.0;
  CHECK(size > 1);
  for (unsigned i = 0; i < size; ++i) {
    double diff = (mean - array[i]);
    result += diff * diff; 
  }
  return result / size;
}

double Variance(const double* array, const unsigned size) {
  return Variance(array, size, Mean(array, size));
}

double StdDev(const double* array, const unsigned size) {
  return sqrt(Variance(array, size));
}

}  // namespace similarity

