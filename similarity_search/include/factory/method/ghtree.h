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

#ifndef _FACTORY_GH_TREE_H_
#define _FACTORY_GH_TREE_H_

#include <method/ghtree.h>

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreateGHTree(bool PrintProgress,
                           const string& SpaceType,
                           Space<dist_t>& space,
                           const ObjectVector& DataObjects) {

  return new GHTree<dist_t>(space, DataObjects);
}

/*
 * End of creating functions.
 */

}

#endif
