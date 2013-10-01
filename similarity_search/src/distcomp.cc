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
#include "distcomp.h"
#include "string.h"

#include <cstdlib>

namespace similarity {

using namespace std;

/*
 * LInf-norm.
 */

template <class T>
T LInfNormStandard(const T *p1, const T *p2, size_t qty) 
{ 
    T res = 0;

    for (size_t i = 0; i < qty; i++) { 
      res = max(res, fabs(p1[i]-p2[i]));
    }

    return res;
}

template float LInfNormStandard<float>(const float *p1, const float *p2, size_t qty);
template double LInfNormStandard<double>(const double *p1, const double *p2, size_t qty);


template <class T>
T LInfNorm(const T* pVect1, const T* pVect2, size_t qty) {
    T res = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    while (pVect1 < pEnd1) {
        res = max(res, fabs(*pVect1++ - *pVect2++));
        res = max(res, fabs(*pVect1++ - *pVect2++));
        res = max(res, fabs(*pVect1++ - *pVect2++));
        res = max(res, fabs(*pVect1++ - *pVect2++));
    }

    while (pVect1 < pEnd2) {
        res = max(res, fabs(*pVect1++ - *pVect2++));
    }

    return res;
}

template float LInfNorm<float>(const float* pVect1, const float* pVect2, size_t qty);
template double LInfNorm<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * L1-norm.
 */

template <class T>
T L1NormStandard(const T *p1, const T *p2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += fabs(p1[i]-p2[i]);
    }

    return sum;
}

template float L1NormStandard<float>(const float *p1, const float *p2, size_t qty);
template double L1NormStandard<double>(const double *p1, const double *p2, size_t qty);


template <class T>
T L1Norm(const T* pVect1, const T* pVect2, size_t qty) {
    T res = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    while (pVect1 < pEnd1) {
        res += fabs(*pVect1++ - *pVect2++);
        res += fabs(*pVect1++ - *pVect2++);
        res += fabs(*pVect1++ - *pVect2++);
        res += fabs(*pVect1++ - *pVect2++);
    }

    while (pVect1 < pEnd2) {
        res += fabs(*pVect1++ - *pVect2++);
    }

    return res;
}

template float L1Norm<float>(const float* pVect1, const float* pVect2, size_t qty);
template double L1Norm<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * L2-norm.
 */

template <class T>
T L2NormStandard(const T *p1, const T *p2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      T diff = (p1[i]-p2[i]);
      sum += diff * diff;
    }

    return sqrt(sum);
}

template  float L2NormStandard<float>(const float *p1, const float *p2, size_t qty);
template  double L2NormStandard<double>(const double *p1, const double *p2, size_t qty);


template <class T>
T L2Norm(const T* pVect1, const T* pVect2, size_t qty) {
    T res = 0, diff = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    while (pVect1 < pEnd1) {
        diff = *pVect1++ - *pVect2++; res += diff * diff;
        diff = *pVect1++ - *pVect2++; res += diff * diff;
        diff = *pVect1++ - *pVect2++; res += diff * diff;
        diff = *pVect1++ - *pVect2++; res += diff * diff;
    }

    while (pVect1 < pEnd2) {
        diff = *pVect1++ - *pVect2++; res += diff * diff;
    }

    return sqrt(res);
}

template float  L2Norm<float>(const float* pVect1, const float* pVect2, size_t qty);
template double L2Norm<double>(const double* pVect1, const double* pVect2, size_t qty);

template <class T> T KLStandard(const T *pVect1, const T *pVect2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += pVect1[i] * log(pVect1[i]/pVect2[i]);
    }

    return sum;
}

template float KLStandard<float>(const float *pVect1, const float *pVect2, size_t qty); 
template double KLStandard<double>(const double *pVect1, const double *pVect2, size_t qty);

template <class T> T KLStandardLogDiff(const T *p1, const T *p2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += p1[i] * (log(p1[i])-log(p2[i]));
    }

    return sum;
}

template float KLStandardLogDiff<float>(const float *p1, const float *p2, size_t qty);
template double KLStandardLogDiff<double>(const double *p1, const double *p2, size_t qty);

template <class T> T KLGeneralStandard(const T *pVect1, const T *pVect2, size_t qty) 
{ 
    T sum = 0;

    for (size_t i = 0; i < qty; i++) { 
      sum += pVect1[i] * log(pVect1[i]/pVect2[i]) + pVect2[i] - pVect1[i];
    }

    return sum;
}

template float KLGeneralStandard<float>(const float *pVect1, const float *pVect2, size_t qty); 
template double KLGeneralStandard<double>(const double *pVect1, const double *pVect2, size_t qty);

template <class T> 
T KLGeneralPrecomp(const T* pVect1, const T* pVect2, size_t qty) {
    T sum = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    const T* pVectLog1 = pVect1 + qty;
    const T* pVectLog2 = pVect2 + qty;

    while (pVect1 < pEnd1) {
        sum += (*pVect1) * (*pVectLog1 - *pVectLog2) + *pVect2 - *pVect1; ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
        sum += (*pVect1) * (*pVectLog1 - *pVectLog2) + *pVect2 - *pVect1; ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
        sum += (*pVect1) * (*pVectLog1 - *pVectLog2) + *pVect2 - *pVect1; ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
        sum += (*pVect1) * (*pVectLog1 - *pVectLog2) + *pVect2 - *pVect1; ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
    }

    while (pVect1 < pEnd2) {
        sum += (*pVect1) * (*pVectLog1 - *pVectLog2) + *pVect2 - *pVect1; ++pVect1; ++pVect2; ++pVectLog1; ++pVectLog2;
    }

    return sum;
}

template float KLGeneralPrecomp<float>(const float* pVect1, const float* pVect2, size_t qty);
template double KLGeneralPrecomp<double>(const double* pVect1, const double* pVect2, size_t qty);


template <class T> 
T KLPrecomp(const T* pVect1, const T* pVect2, size_t qty) {
    T sum = 0;

    size_t qty4 = qty/4;
    const T* pEnd1 = pVect1 + qty4 * 4;
    const T* pEnd2 = pVect1 + qty;

    const T* pVectLog1 = pVect1 + qty;
    const T* pVectLog2 = pVect2 + qty;

    while (pVect1 < pEnd1) {
        sum += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
        sum += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
        sum += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
        sum += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
    }

    while (pVect1 < pEnd2) {
        sum += (*pVect1++) * ((*pVectLog1++) - (*pVectLog2++));
    }

    return sum;
}

template float KLPrecomp<float>(const float* pVect1, const float* pVect2, size_t qty);
template double KLPrecomp<double>(const double* pVect1, const double* pVect2, size_t qty);


// Edit distance - O(kMaxLen) space, O(len1*len2) time
unsigned ED1(const char* s1, const int len1, const char* s2, const int len2) {
  static const int kMaxLen = 10000;
  static unsigned d[2][kMaxLen + 1];

  if (len1 == 0) return len2;
  if (len2 == 0) return len1;
  if (len1 == len2 && memcmp(s1, s2, len1) == 0) return 0;

  int cur = 0, next;
  d[0][0] = d[1][0] = 0;
  for (int i = 1; i <= len2; ++i) d[cur][i] = i;
  for (int i = 1; i <= len1; ++i) {
    next = !cur;
    d[next][0] = i;
    for (int j = 1; j <= len2; ++j) {
      d[next][j] = std::min(std::min(d[cur][j] + 1, d[next][j - 1] + 1),
                            d[cur][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
    }
    cur = next;
  }
  return d[cur][len2];
}

static unsigned d[7000][7000];

// Edit distance - O(len1*len2) time and space
unsigned ED2(const char* s1, const int len1, const char* s2, const int len2) {
  if (len1 == 0) return len2;
  if (len2 == 0) return len1;
  if (len1 == len2 && memcmp(s1, s2, len1) == 0) return 0;
  d[0][0] = 0;
  for(int i = 1; i <= len1; ++i) d[i][0] = i;
  for(int i = 1; i <= len2; ++i) d[0][i] = i;

  for(int i = 1; i <= len1; ++i) {
    for(int j = 1; j <= len2; ++j) {
      d[i][j] = std::min(std::min(d[i - 1][j] + 1, d[i][j - 1] + 1),
                         d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
    }
  }
  return d[len1][len2];
}


}
