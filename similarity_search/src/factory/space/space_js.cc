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
#include "space_js.h"
#include "spacefactory.h"

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Space<dist_t>* CreateJSDivSlow(const AnyParams& /* ignoring params */) {
  return new SpaceJSDiv<dist_t>(SpaceJSDiv<dist_t>::kJSSlow);
}

template <typename dist_t>
Space<dist_t>* CreateJSDivFastPrecomp(const AnyParams& /* ignoring params */) {
  return new SpaceJSDiv<dist_t>(SpaceJSDiv<dist_t>::kJSFastPrecomp);
}

template <typename dist_t>
Space<dist_t>* CreateJSDivFastPrecompApprox(const AnyParams& /* ignoring params */) {
  return new SpaceJSDiv<dist_t>(SpaceJSDiv<dist_t>::kJSFastPrecompApprox);
}


template <typename dist_t>
Space<dist_t>* CreateJSMetricSlow(const AnyParams& /* ignoring params */) {
  return new SpaceJSMetric<dist_t>(SpaceJSMetric<dist_t>::kJSSlow);
}

template <typename dist_t>
Space<dist_t>* CreateJSMetricFastPrecomp(const AnyParams& /* ignoring params */) {
  return new SpaceJSMetric<dist_t>(SpaceJSMetric<dist_t>::kJSFastPrecomp);
}

template <typename dist_t>
Space<dist_t>* CreateJSMetricFastPrecompApprox(const AnyParams& /* ignoring params */) {
  return new SpaceJSMetric<dist_t>(SpaceJSMetric<dist_t>::kJSFastPrecompApprox);
}



/*
 * End of creating functions.
 */

/*
 * Let's register creating functions in a method factory.
 *
 * IMPORTANT NOTE: don't include this source-file into a library.
 * Sometimes C++ carries out a lazy initialization of global objects
 * that are stored in a library. Then, the registration code doesn't work.
 */

REGISTER_SPACE_CREATOR(float,  SPACE_JS_DIV_SLOW, CreateJSDivSlow)
REGISTER_SPACE_CREATOR(double, SPACE_JS_DIV_SLOW, CreateJSDivSlow)
REGISTER_SPACE_CREATOR(float,  SPACE_JS_DIV_FAST, CreateJSDivFastPrecomp)
REGISTER_SPACE_CREATOR(double, SPACE_JS_DIV_FAST, CreateJSDivFastPrecomp)
REGISTER_SPACE_CREATOR(float,  SPACE_JS_DIV_FAST_APPROX, CreateJSDivFastPrecompApprox)
REGISTER_SPACE_CREATOR(double, SPACE_JS_DIV_FAST_APPROX, CreateJSDivFastPrecompApprox)

REGISTER_SPACE_CREATOR(float,  SPACE_JS_METR_SLOW, CreateJSMetricSlow)
REGISTER_SPACE_CREATOR(double, SPACE_JS_METR_SLOW, CreateJSMetricSlow)
REGISTER_SPACE_CREATOR(float,  SPACE_JS_METR_FAST, CreateJSMetricFastPrecomp)
REGISTER_SPACE_CREATOR(double, SPACE_JS_METR_FAST, CreateJSMetricFastPrecomp)
REGISTER_SPACE_CREATOR(float,  SPACE_JS_METR_FAST_APPROX, CreateJSMetricFastPrecompApprox)
REGISTER_SPACE_CREATOR(double, SPACE_JS_METR_FAST_APPROX, CreateJSMetricFastPrecompApprox)

}

