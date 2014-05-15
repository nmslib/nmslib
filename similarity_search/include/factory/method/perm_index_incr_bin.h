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

#include "method/perm_index_incr_bin.h"

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreatePermutationIndexIncrementalBin(bool PrintProgress,
                           const string& SpaceType,
                           const Space<dist_t>* space,
                           const ObjectVector& dataObjects,
                           const AnyParams& allParams) {
  AnyParamManager pmgr(allParams);

  double    dbScanFrac = 0.05;
  size_t    numPivot   = 16;
  size_t    binThres   = 8;
  bool      skipChecking = false;
  bool      useSort    = true;
  size_t    maxHammingDist = numPivot = 4;

  pmgr.GetParamOptional("dbScanFrac",   dbScanFrac);
  pmgr.GetParamOptional("numPivot",     numPivot);
  pmgr.GetParamOptional("binThreshold", binThres);
  pmgr.GetParamOptional("skipChecking", skipChecking);
  pmgr.GetParamOptional("useSort",      useSort);
  pmgr.GetParamOptional("maxHammingDist", maxHammingDist);

  if (dbScanFrac < 0.0 || dbScanFrac > 1.0) {
    LOG(FATAL) << METH_PERMUTATION_INC_SORT_BIN << " requires that dbScanFrac is in the range [0,1]";
  }

  return new PermutationIndexIncrementalBin<dist_t, SpearmanRhoSIMD>(
                                                       space,
                                                       dataObjects,
                                                       numPivot,
                                                       binThres,
                                                       dbScanFrac,
                                                       maxHammingDist,
                                                       useSort,
                                                       skipChecking);

}

/*
 * End of creating functions.
 */

}

