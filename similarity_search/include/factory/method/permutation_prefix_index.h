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
#ifndef _FACTORY_PERM_PREF_INDEX_H_
#define _FACTORY_PERM_PREF_INDEX_H_

#include <method/permutation_prefix_index.h>

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreatePermutationPrefixIndex(bool PrintProgress,
                           const string& SpaceType,
                           Space<dist_t>& space,
                           const ObjectVector& DataObjects) {
  return new PermutationPrefixIndex<dist_t>(PrintProgress, space, DataObjects);
}

/*
 * End of creating functions.
 */

}

#endif
