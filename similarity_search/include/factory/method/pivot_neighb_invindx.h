/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _FACTORY_PIVOT_NEIGHB_H_
#define _FACTORY_PIVOT_NEIGHB_H_

#include <method/pivot_neighb_invindx.h>

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

/*
 * End of creating functions.
 */
}

#endif
