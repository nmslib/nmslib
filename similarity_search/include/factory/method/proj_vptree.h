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

#ifndef _FACTORY_PROJ_VPTREE_H
#define _FACTORY_PROJ_VPTREE_H

#include <method/proj_vptree.h>
#include <space/space_sparse_scalar.h>
#include <space/space_sparse_scalar_fast.h>

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreateProjVPTree(bool PrintProgress,
                           const string& SpaceType,
                           const Space<dist_t>* space,
                           const ObjectVector& DataObjects,
                           const AnyParams& AllParams) {

    if (
        SpaceType != SPACE_SPARSE_COSINE_SIMILARITY &&
        SpaceType != SPACE_SPARSE_ANGULAR_DISTANCE &&
        SpaceType != SPACE_SPARSE_COSINE_SIMILARITY_FAST &&
        SpaceType != SPACE_SPARSE_ANGULAR_DISTANCE_FAST 
        ) LOG(LIB_FATAL) << "Unsupported space: " << SpaceType << " "
                   << METH_PROJ_VPTREE << 
        " works only with one of the following spaces: " 
                    << SPACE_SPARSE_ANGULAR_DISTANCE  << ", "
                    << SPACE_SPARSE_COSINE_SIMILARITY <<  ", "
                    << SPACE_SPARSE_ANGULAR_DISTANCE_FAST  << ", "
                    << SPACE_SPARSE_COSINE_SIMILARITY_FAST;
      ;

    return new ProjectionVPTree<dist_t>(space, DataObjects, AllParams);
}

/*
 * End of creating functions.
 */

}

#endif
