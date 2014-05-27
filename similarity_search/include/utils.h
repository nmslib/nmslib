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

#ifndef _UTILS_H_
#define _UTILS_H_

#include <cctype>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <map>
#include <typeinfo>
#include <random>
#include <climits>

// compiler_warning.h
#define STRINGISE_IMPL(x) #x
#define STRINGISE(x) STRINGISE_IMPL(x)

/*
 * This solution for generating
 * cross-platform warnings
 * is taken from http://stackoverflow.com/a/1911632/2120401
 * Use: #pragma message WARN("My message")
 *
 * Note: We may need other other definitions for other compilers,
 *       but so far it worked for MSVS, GCC, CLang, and Intel.
 */
#   define FILE_LINE_LINK __FILE__ "(" STRINGISE(__LINE__) ") : "
#   define WARN(exp) (FILE_LINE_LINK "WARNING: " exp)

#ifdef _MSC_VER
#define PATH_SEPARATOR "\\"
#else 
#define PATH_SEPARATOR "/"
#endif


#ifdef _MSC_VER
#define ISNAN _isnan
#define __func__ __FUNCTION__ 
#else
#define ISNAN std::isnan
#endif

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

namespace similarity {

using std::string;
using std::vector;
using std::stringstream;
using std::random_device;
using std::mt19937;

using namespace std;


const char* GetFileName(const char* fullpath);

bool CreateDir(const char* name, int mode = 0777);

bool IsFileExists(const char* filename);

inline bool IsFileExists(const string& filename) { return IsFileExists(filename.c_str()); }

inline int RandomInt() {
    // Static is thread-safe in C++ 11
    static random_device rdev;
    static mt19937 gen(rdev());
    static std::uniform_int_distribution<int> distr(0, std::numeric_limits<int>::max());
  
    return distr(gen); 
}

template <class T>
inline T RandomReal() {
    // Static is thread-safe in C++ 11
    static random_device rdev;
    static mt19937 gen(rdev());
    static std::uniform_real_distribution<T> distr(0, 1);

    return distr(gen); 
}

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
inline bool ApproxEqual(const T& x, const T& y) {
  // In C++ 11, std::abs is also defined for floating-point numbers
  return std::abs(x - y) <= std::numeric_limits<T>::epsilon();
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

// Don't remove period here
inline void ReplaceSomePunct(string &s) {
  for (size_t i = 0; i < s.size(); ++i) if (s[i] == ',' || s[i] == ':') s[i] = ' ';
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

