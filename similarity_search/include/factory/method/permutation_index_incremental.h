/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
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

#ifndef _FACTORY_PERM_INDEX_INCR_H_
#define _FACTORY_PERM_INDEX_INCR_H_

#include "method/permutation_index_incremental.h"

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreatePermutationIndexIncremental(bool PrintProgress,
                           const string& SpaceType,
                           const Space<dist_t>* space,
                           const ObjectVector& DataObjects,
                           const AnyParams& AllParams) {
  AnyParamManager pmgr(AllParams);

  double    DbScanFrac = 0.05;
  size_t    NumPivot   = 16;

  pmgr.GetParamOptional("dbScanFrac", DbScanFrac);
  pmgr.GetParamOptional("numPivot", NumPivot);

  if (DbScanFrac < 0.0 || DbScanFrac > 1.0) {
    LOG(FATAL) << METH_PERMUTATION_INC_SORT << " requires that dbScanFrac is in the range [0,1]";
  }

  return new PermutationIndexIncremental<dist_t, SpearmanRhoSIMD>(
                                                       space,
                                                       DataObjects,
                                                       NumPivot,
                                                       DbScanFrac);

}

/*
 * End of creating functions.
 */

}

#endif
