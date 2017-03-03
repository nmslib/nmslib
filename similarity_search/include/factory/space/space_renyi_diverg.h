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

#ifndef FACTORY_SPACE_RENYI_DIVERG_H
#define FACTORY_SPACE_RENYI_DIVERG_H

#include <limits>
#include <cmath>

#include <space/space_renyi_diverg.h>

namespace similarity {

template <typename dist_t>
Space<dist_t>* CreateRenyiDiverg(const AnyParams& AllParams) {
  AnyParamManager pmgr(AllParams);

  float alpha = 0.5;

  pmgr.GetParamOptional("alpha",  alpha, alpha);

  CHECK_MSG(std::fabs(alpha - 1) > 2*std::numeric_limits<float>::min() && alpha > 0,
            "alpha should be > 0 and != 1");

  return new SpaceRenyiDiverg<dist_t>(alpha);
}

/*
 * End of creating functions.
 */

}

#endif
