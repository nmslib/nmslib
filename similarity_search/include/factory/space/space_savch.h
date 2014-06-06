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

#ifndef FACTORY_SPACE_SAVCH_H
#define FACTORY_SPACE_SAVCH_H

#include <limits.h>
#include <cmath>

#include <space/space_vector_gen.h>

namespace similarity {

using std::sqrt;

const int DELTA=1;
const int POINTS_IN_W=10;
const int POINTS_IN_H=10;
const int HISTO_SIZE=8;

#define DISTANCE CHI_SQUARE

/*
 * Creating functions.
 */

template <class ElementType>
struct SavchSpace {
  ElementType accum_dist (ElementType a, ElementType b) const {
    ElementType res = 0;
#if DISTANCE==MANHATTEN
    res = std::abs(a-b);
#elif DISTANCE==EUC
    res =(a-b)*(a-b);
#elif DISTANCE==CHI_SQUARE
    if((a+b)>0)
      res=(a-b)*(a-b)/(a+b);
#endif
    return res;
  };

  ElementType operator()(const ElementType* x, const ElementType* y, size_t qty) const {
    const ElementType *lhs, *rhs;
    ElementType result = 0;
    for (int i = 0; i < POINTS_IN_H; ++i) {
      size_t iMin = i >= DELTA ? i - DELTA : 0;
      size_t iMax = i + DELTA;
      if (iMax >= POINTS_IN_H)
        iMax = POINTS_IN_H - 1;
      for (int j = 0; j < POINTS_IN_W; ++j) {
        size_t jMin = j >= DELTA ? j - DELTA : 0;
        size_t jMax = j + DELTA;
        if (jMax >= POINTS_IN_W)
        jMax = POINTS_IN_W - 1;
        ElementType minSum = numeric_limits<ElementType>::max();
        size_t i1 = i, j1 = j;
        lhs = y + (i1*POINTS_IN_W + j1)*HISTO_SIZE;
        for (size_t i2 = iMin; i2 <= iMax; ++i2){
          for (size_t j2 = jMin; j2 <= jMax; ++j2){
            rhs = x + (i2*POINTS_IN_W + j2)*HISTO_SIZE;
            ElementType curSum = 0;
            const ElementType* llhs = lhs;
            for (size_t ind = 0; ind < HISTO_SIZE; ++ind){
              float d1 = *llhs++;
              float d2 = *(rhs + ind);

              //curSum += accum_dist(d1, d2, 1);
              curSum += accum_dist(d1, d2);
            }
            if (curSum<0)
              curSum = 0;
            if (minSum>curSum){
              minSum = curSum;
            }
          }
        }
        result += sqrt(minSum);
      }
    }
    return result;
  }
};

template <typename dist_t>
Space<dist_t>* CreateSavch(const AnyParams& /* ignoring params */) {
  return new VectorSpaceGen<dist_t, SavchSpace<dist_t>>();
}

/*
 * End of creating functions.
 */

}

#endif
