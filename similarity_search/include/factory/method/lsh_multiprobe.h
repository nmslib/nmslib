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

#ifndef _FACTORY_LSH_MULTIPROBE_H_
#define _FACTORY_LSH_MULTIPROBE_H_

#include <method/lsh_multiprobe.h>

#include "logging.h"

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreateLSHMultiprobe(bool PrintProgress,
                           const string& SpaceType,
                           Space<dist_t>& space,
                           const ObjectVector& DataObjects) {
  CHECK_MSG(SpaceType == "l2", "Multiprobe LSH works only with L2");

  return new MultiProbeLSH<dist_t>(space, DataObjects);
}

/*
 * End of creating functions.
 */

}

#endif
