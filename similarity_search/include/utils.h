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

#ifndef _UTILS_H_
#define _UTILS_H_

#include <cstddef>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <map>
#include <typeinfo>


namespace similarity {

using std::string;
using std::vector;
using std::stringstream;

using namespace std;

const char* CurrentTime();

const char* GetFileName(const char* fullpath);

bool CreateDir(const char* name, int mode = 0777);

bool IsFileExists(const char* filename);

inline bool IsFileExists(const string& filename) { return IsFileExists(filename.c_str()); }

void RandomReset();

int RandomInt();

double RandomReal();

void GenUniform(const char* filename, const int total, const int dimension,
                const double maxrange);

void RStrip(char* str);

double Mean(const double* array, const unsigned size);

double Variance(const double* array, const unsigned size);

double Variance(const double* array, const unsigned size, const double mean);

double StdDev(const double* array, const unsigned size);

/*
 * We want to avoid an overflow in the case when the distance is an integer type.
 */
template <typename dist_t>
inline dist_t DistMax() {
  return std::numeric_limits<dist_t>::max() / 2;
}

template <typename T>
inline T Abs(const T& x) {
  return x >= 0 ? x : -x;
}

template <typename T>
inline bool ApproxEqual(const T& x, const T& y) {
  return Abs(x - y) <= std::numeric_limits<T>::epsilon();
}

template <typename T>
inline bool ApproxLess(const T& x, const T& y) {
  return x + std::numeric_limits<T>::epsilon() < y;
}

inline double round1(double x) { return round(x*10.0)/10.0; }
inline double round2(double x) { return round(x*100.0)/100.0; }

/*
 * This function will only work for strings without spaces
 * TODO(@leo) replace, perhaps, it with a more generic version
 */
template <typename ElemType>
inline bool SplitStr(const std::string& str_, vector<ElemType>& res, const char SplitChar) {
  res.clear();
  std::string str = str_;

  for (auto it = str.begin(); it != str.end(); ++it) {
    if (*it == SplitChar) *it = ' ';
  }
  std::stringstream inp(str);

  while (!inp.eof()) {
    ElemType token;
    if (!(inp >> token)) {
      return false;
    }
    res.push_back(token);
  }

  return true;
}

inline void ToLower(string &s) {
  for (size_t i = 0; i < s.size(); ++i) s[i] = std::tolower(s[i]);
}

inline void ReplacePunct(string &s) {
  for (size_t i = 0; i < s.size(); ++i) if (ispunct(s[i])) s[i] = ' ';
}

template <class T>
struct AutoVectDel {
  AutoVectDel(typename std::vector<T *>& Vector) : mVector(Vector) {}
  ~AutoVectDel() {
    for (auto i: mVector) { delete i; }
  }
  std::vector<T *>& mVector;
};

template <class U, class V>
struct AutoMapDel {
  AutoMapDel(typename std::map<U, V*>& Map) : mMap(Map) {}
  ~AutoMapDel() {
    for (auto i: mMap) { delete i.second; }
  }
    typename std::map<U, V*>& mMap;
};

template <class U, class V>
struct AutoMultiMapDel {
  AutoMultiMapDel(typename std::multimap<U, V*>& Map) : mMap(Map) {}
  ~AutoMultiMapDel() {
    for (auto i: mMap) { delete i.second; }
  }
    typename std::multimap<U, V*>& mMap;
};

}  // namespace similarity

#endif   // _UTILS_H_

