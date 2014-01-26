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

#include "searchoracle.h"
#include "space_lp.h"
#include "spacefactory.h"

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Space<dist_t>* CreateLINF(const AnyParams& /* ignoring params */) {
  // Chebyshev Distance
  return new SpaceLp<dist_t>(-1);

}
template <typename dist_t>
Space<dist_t>* CreateL1(const AnyParams& /* ignoring params */) {
  return new SpaceLp<dist_t>(1);
}
template <typename dist_t>
Space<dist_t>* CreateL2(const AnyParams& /* ignoring params */) {
  return new SpaceLp<dist_t>(2);
}

template <typename dist_t>
Space<dist_t>* CreateL(const AnyParams& AllParams) {
  AnyParamManager pmgr(AllParams);

  dist_t p;

  pmgr.GetParamRequired("p",  p);

  return new SpaceLp<dist_t>(p);
}

/*
 * End of creating functions.
 */

/*
 * Let's register creating functions in a space factory.
 *
 * IMPORTANT NOTE: don't include this source-file into a library.
 * Sometimes C++ carries out a lazy initialization of global objects
 * that are stored in a library. Then, the registration code doesn't work.
 */

REGISTER_SPACE_CREATOR(float,  SPACE_L,  CreateL)
REGISTER_SPACE_CREATOR(double, SPACE_L,  CreateL)
REGISTER_SPACE_CREATOR(float,  SPACE_LINF, CreateLINF)
REGISTER_SPACE_CREATOR(double, SPACE_LINF, CreateLINF)
REGISTER_SPACE_CREATOR(float,  SPACE_L1, CreateL1)
REGISTER_SPACE_CREATOR(double, SPACE_L1, CreateL1)
REGISTER_SPACE_CREATOR(float,  SPACE_L2, CreateL2)
REGISTER_SPACE_CREATOR(double, SPACE_L2, CreateL2)

}

