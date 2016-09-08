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

#ifndef FACTORY_SPACE_AB_DIVERG_H
#define FACTORY_SPACE_AB_DIVERG_H

#include <space/space_ab_diverg.h>

namespace similarity {

template <typename dist_t>
Space<dist_t>* CreateAlphaBetaDiverg(const AnyParams& AllParams) {
  AnyParamManager pmgr(AllParams);

  float alpha = 1.0, beta = 1.0;

  pmgr.GetParamOptional("alpha",  alpha, alpha);
  pmgr.GetParamOptional("beta",   beta,  beta);

  return new SpaceAlphaBetaDiverg<dist_t>(alpha, beta);
}

/*
 * End of creating functions.
 */

}

#endif
