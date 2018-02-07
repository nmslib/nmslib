/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2013-2018
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef _FACTORY_LSH_
#define _FACTORY_LSH_

#include <method/lsh.h>

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreateLSHCauchy(bool PrintProgress,
                           const string& SpaceType,
                           Space<dist_t>& space,
                           const ObjectVector& DataObjects) {

    int SpaceSelector = 1;
    CHECK_MSG(SpaceType == "l1", "LSH (Cauchy) works only with L1");

    return new LSHCauchy<dist_t>(
                      space,
                      DataObjects,
                      SpaceSelector
                  );
}

template <typename dist_t>
Index<dist_t>* CreateLSHGaussian(bool PrintProgress,
                           const string& SpaceType,
                           Space<dist_t>& space,
                           const ObjectVector& DataObjects) {
    int SpaceSelector = 2;
    CHECK_MSG(SpaceType == "l2", "LSH (Guassian) works only with L2");

    return new LSHGaussian<dist_t>(
                      space,
                      DataObjects,
                      SpaceSelector
                  );
}

template <typename dist_t>
Index<dist_t>* CreateLSHThreshold(bool PrintProgress,
                           const string& SpaceType,
                           Space<dist_t>& space,
                           const ObjectVector& DataObjects) {
    int SpaceSelector = 1;
    CHECK_MSG(SpaceType == "l1", "LSH (Threshold) works only with L1");

    return new LSHThreshold<dist_t>(
                      space,
                      DataObjects,
                      SpaceSelector);
}

/*
 * End of creating functions.
 */

}

#endif
