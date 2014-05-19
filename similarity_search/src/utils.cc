/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <cmath>
#include <cstring>


#ifdef _MSC_VER
#include <time.h>
#include <io.h>
#ifndef F_OK
#define F_OK 0
#include <direct.h>       // for mkdir
#define mkdir(name, mode) mkdir(name)
#endif
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>     // for mkdir
#endif

#include "global.h"       // for snprintf
#include "utils.h"
#include "logging.h"

namespace similarity {


const char* GetFileName(const char* fullpath) {
  for (int i = strlen(fullpath) - 1; i >= 0; --i) {
    if (fullpath[i] == '\\' || fullpath[i] == '/') {
      return fullpath + i + 1;
    }
  }
  return fullpath;
}

bool IsFileExists(const char* filename) {
#ifdef _MSC_VER
  return _access(filename, F_OK) == 0;
#else
  return access(filename, F_OK) == 0;
#endif
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

