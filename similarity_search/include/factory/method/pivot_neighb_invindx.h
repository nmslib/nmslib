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
#ifndef _FACTORY_PIVOT_NEIGHB_H_
#define _FACTORY_PIVOT_NEIGHB_H_

#include <method/pivot_neighb_invindx.h>
#ifdef WITH_EXTRAS
#include <method/pivot_neighb_horder_invindx.h>
#endif

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreatePivotNeighbInvertedIndex(
    bool PrintProgress,
    const string& SpaceType,
    Space<dist_t>& space,
    const ObjectVector& DataObjects) {
  
  return new PivotNeighbInvertedIndex<dist_t>(
      PrintProgress,
      space,
      DataObjects
  );
}

#ifdef WITH_EXTRAS

template <typename dist_t>
Index<dist_t>* CreatePivotNeighbHorderInvIndex(
    bool PrintProgress,
    const string& SpaceType,
    Space<dist_t>& space,
    const ObjectVector& DataObjects) {
  
  return new PivotNeighbHorderInvIndex<dist_t>(
      PrintProgress,
      space,
      DataObjects
  );
}
#endif

/*
 * End of creating functions.
 */
}

#endif
