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
#ifndef GENRAND_VECT_HPP
#define GENRAND_VECT_HPP

namespace similarity {

template <class T> 
inline void Normalize(T* pVect, size_t qty) {
  T sum = 0;
  for (size_t i = 0; i < qty; ++i) {
    sum += pVect[i];
  }
  if (sum != 0) {
    for (size_t i = 0; i < qty; ++i) {
      pVect[i] /= sum;
    }
  }
}


template <class T> 
inline void GenRandVect(T* pVect, size_t qty, T MinElem = T(0), T MaxElem = T(1), bool DoNormalize = false) {
  T sum = 0;
  for (size_t i = 0; i < qty; ++i) {
    pVect[i] = MinElem + (MaxElem - MinElem) * RandomReal<T>();
    sum += fabs(pVect[i]);
  }
  if (DoNormalize && sum != 0) {
    for (size_t i = 0; i < qty; ++i) {
      pVect[i] /= sum;
    }
  }
}

inline void GenRandIntVect(int* pVect, size_t qty) {
  for (size_t i = 0; i < qty; ++i) {
    pVect[i] = RandomInt();
  }
}

template <class T> 
inline void SetRandZeros(T* pVect, size_t qty, double pZero) {
    for (size_t j = 0; j < qty; ++j) if (RandomReal<T>() < pZero) pVect[j] = T(0);
}

}

#endif

