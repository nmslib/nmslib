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
#include <limits>
#include <algorithm>

namespace similarity {

using namespace std;

/*
 * Cosine-distance (a proper metric)
 *
 * TODO: @leo implement an efficient version
 *
 */

template <class T>
T CosineDistance(const T *p1, const T *p2, size_t qty) 
{ 
    T sum = 0;
    T norm1 = 0;
    T norm2 = 0;

    for (size_t i = 0; i < qty; i++) {
      norm1 += p1[i] * p1[i];
      norm2 += p2[i] * p2[i];
      sum += p1[i] * p2[i];
    }
    /* 
     * Sometimes due to rounding errors, the arg of arccos gets > 1 or < -1
     * then acos returns NaN 
     */
    T val = max(T(-1), min(T(1), sum / sqrt(norm1) / sqrt(norm2)));
    return acos(val);
}

template float  CosineDistance<float>(const float* pVect1, const float* pVect2, size_t qty);
template double CosineDistance<double>(const double* pVect1, const double* pVect2, size_t qty);

}
