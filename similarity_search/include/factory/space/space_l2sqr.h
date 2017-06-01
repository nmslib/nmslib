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

#ifndef FACTORY_SPACE_L2_SQR_H
#define FACTORY_SPACE_L2_SQR_H

#include <space/space_l2sqr.h>

namespace similarity {

/*
 * Creating functions.
 */

Space<float>* CreateL2SQR(const AnyParams& /* ignoring params */) {
  // Chebyshev Distance
  return new SpaceL2SQR();

}

/*
 * End of creating functions.
 */

}

#endif
