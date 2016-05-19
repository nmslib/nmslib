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

#ifndef _FACTORY_PROJ_INDEX_INCR_H_
#define _FACTORY_PROJ_INDEX_INCR_H_

#include <method/projection_index_incremental.h>

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreateProjectionIndexIncremental(bool PrintProgress,
                           const string& SpaceType,
                           Space<dist_t>& space,
                           const ObjectVector& DataObjects) {
  return new ProjectionIndexIncremental<dist_t>(PrintProgress,
                                                space,
                                                DataObjects);

}

/*
 * End of creating functions.
 */

}

#endif
