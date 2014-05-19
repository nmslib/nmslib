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

#ifndef FACTORY_SPACE_SCALAR_H
#define FACTORY_SPACE_SCALAR_H

#include <space/space_scalar.h>

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Space<dist_t>* CreateCosineSimilarity(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceCosineSimilarity<dist_t>();
}


template <typename dist_t>
Space<dist_t>* CreateAngularDistance(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceAngularDistance<dist_t>();
}

/*
 * End of creating functions.
 */

}

#endif
