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

#ifndef _FACTORY_PERM_INV_INDEX_H_
#define _FACTORY_PERM_INV_INDEX_H_

#include <method/permutation_inverted_index.h>

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreatePermInvertedIndex(
    bool PrintProgress,
    const string& SpaceType,
    const Space<dist_t>* space,
    const ObjectVector& DataObjects,
    const AnyParams& AllParams) {
  AnyParamManager pmgr(AllParams);

  size_t num_pivot = 50;
  size_t num_pivot_index = 32;
  size_t num_pivot_search = 20;
  double db_scan_frac = 0.05;
  size_t max_pos_diff = num_pivot;

  pmgr.GetParamOptional("numPivot", num_pivot);
  pmgr.GetParamOptional("numPivotIndex", num_pivot_index);
  pmgr.GetParamOptional("numPivotSearch", num_pivot_search);
  pmgr.GetParamOptional("maxPosDiff", max_pos_diff);
  pmgr.GetParamOptional("dbScanFrac", db_scan_frac);

  if (num_pivot_search > num_pivot_index) {
    LOG(LIB_FATAL) << METH_PERM_INVERTED_INDEX << " requires that numPivotSearch "
               << "should be less than or equal to numPivotIndex";
  }

  if (num_pivot_index > num_pivot) {
    LOG(LIB_FATAL) << METH_PERM_INVERTED_INDEX << " requires that numPivotIndex "
               << "should be less than or equal to numPivot";
  }

  if (db_scan_frac < 0.0 || db_scan_frac > 1.0) {
    LOG(LIB_FATAL) << METH_PERM_INVERTED_INDEX << " requires that dbScanFrac is in the range [0,1]";
  }

  return new PermutationInvertedIndex<dist_t>(
      space,
      DataObjects,
      num_pivot,
      num_pivot_index,
      num_pivot_search,
      max_pos_diff,
      db_scan_frac
  );
}

/*
 * End of creating functions.
 */

}

#endif
