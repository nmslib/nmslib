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

#ifndef FACTORY_SPACE_EDIST_H
#define FACTORY_SPACE_EDIST_H

#include <space/space_leven.h>

namespace similarity {

/*
 * Creating functions.
 */

Space<int>* CreateLevenshtein(const AnyParams& AllParams) {
  return new SpaceLevenshtein();
}

Space<float>* CreateLevenshteinNormalized(const AnyParams& AllParams) {
  return new SpaceLevenshteinNormalized();
}

/*
 * End of creating functions.
 */

}

#endif
