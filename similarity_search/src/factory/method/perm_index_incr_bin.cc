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

#include "searchoracle.h"
#include "perm_index_incr_bin.h"
#include "methodfactory.h"

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

/*
 * Let's register creating functions in a method factory.
 *
 * IMPORTANT NOTE: don't include this source-file into a library.
 * Sometimes C++ carries out a lazy initialization of global objects
 * that are stored in a library. Then, the registration code doesn't work.
 */

REGISTER_METHOD_CREATOR(float,  METH_PERMUTATION_INC_SORT_BIN, CreatePermutationIndexIncrementalBin)
REGISTER_METHOD_CREATOR(double, METH_PERMUTATION_INC_SORT_BIN, CreatePermutationIndexIncrementalBin)
REGISTER_METHOD_CREATOR(int,    METH_PERMUTATION_INC_SORT_BIN, CreatePermutationIndexIncrementalBin)


}

