/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
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
#include "distcomp.h"
#include "string.h"

#include <cstdlib>
#include <limits>
#include <algorithm>

namespace similarity {

using namespace std;
/*
 * Scalar-product
 *
 * TODO: @leo implement a more efficient version
 */

template <class T>
T NormScalarProduct(const T *p1, const T *p2, size_t qty) 
{ 
    constexpr T eps = numeric_limits<T>::min() * 2;

    T sum = 0;
    T norm1 = 0;
    T norm2 = 0;

    for (size_t i = 0; i < qty; i++) {
      norm1 += p1[i] * p1[i];
      norm2 += p2[i] * p2[i];
      sum += p1[i] * p2[i];
    }

    if (norm1 < eps) { /* 
                        * This shouldn't normally happen for this space, but 
                        * if it does, we don't want to get NANs 
                        */
      if (norm2 < eps) return 1;
      return 0;
    } 
    /* 
     * Sometimes due to rounding errors, we get values > 1 or < -1.
     * This throws off other functions that use scalar product, e.g., acos
     */
    return max(T(-1), min(T(1), sum / sqrt(norm1) / sqrt(norm2)));
}

template float  NormScalarProduct<float>(const float* pVect1, const float* pVect2, size_t qty);
template double NormScalarProduct<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * Angular distance (a proper metric)
 *
 */

template <class T>
T AngularDistance(const T *p1, const T *p2, size_t qty) 
{ 
    return acos(NormScalarProduct(p1, p2, qty));
}

template float  AngularDistance<float>(const float* pVect1, const float* pVect2, size_t qty);
template double AngularDistance<double>(const double* pVect1, const double* pVect2, size_t qty);

/*
 * Cosine similarity (not exactly a metric)
 *
 */

template <class T>
T CosineSimilarity(const T *p1, const T *p2, size_t qty) 
{ 
    return std::max(T(0), 1 - NormScalarProduct(p1, p2, qty));
}

template float  CosineSimilarity<float>(const float* pVect1, const float* pVect2, size_t qty);
template double CosineSimilarity<double>(const double* pVect1, const double* pVect2, size_t qty);

}
