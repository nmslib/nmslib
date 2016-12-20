/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2016
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#include "distcomp.h"

namespace similarity {

/*
 * Finds the size of the intersection:
 * A fast scalar scheme designed by N. Kurz.
 * Adopted from https://github.com/lemire/SIMDCompressionAndIntersection (which is also Apache 2)
 */
unsigned IntersectSizeScalarFast(const IdType *A, const size_t lenA, 
                       const IdType *B, const size_t lenB) {
  if (lenA == 0 || lenB == 0)
    return 0;
  unsigned res = 0;

  const IdType *endA = A + lenA;
  const IdType *endB = B + lenB;

  while (1) {
    while (*A < *B) {
    SKIP_FIRST_COMPARE:
      if (++A == endA)
        return res;
    }
    while (*A > *B) {
      if (++B == endB)
        return res;
    }
    if (*A == *B) {
      ++res;
      if (++A == endA || ++B == endB)
        return res;
    } else {
      goto SKIP_FIRST_COMPARE;
    }
  }

  return res; // NOTREACHED
};

// Text-book algo, but it is less efficient
unsigned IntersectSizeScalarStand(const IdType *A, const size_t lenA, 
                                  const IdType *B, const size_t lenB) {
  if (lenA == 0 || lenB == 0)
    return 0;
  unsigned res = 0;

  const IdType *endA = A + lenA;
  const IdType *endB = B + lenB;

  while (A < endA && B < endB) {
    if (*A < *B) {
      A++;
    } else if (*A > *B) {
      B++;
    } else {
    // *A == *B
      ++res;
      A++; B++;
    }
  }

  return res; 
};

}

