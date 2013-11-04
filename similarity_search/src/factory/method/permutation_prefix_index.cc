/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
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

#include "searchoracle.h"
#include "permutation_prefix_index.h"
#include "methodfactory.h"

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreatePermutationPrefixIndex(bool PrintProgress,
                           const string& SpaceType,
                           const Space<dist_t>* space,
                           const ObjectVector& DataObjects,
                           const AnyParams& AllParams) {
  AnyParamManager pmgr(AllParams);

  size_t    NumPivot        = 16;
  size_t    PrefixLength    = 4;
  size_t    MinCandidate    = 1000;
  bool      ChunkBucket     = true;

  pmgr.GetParamOptional("prefixLength", PrefixLength);
  pmgr.GetParamOptional("numPivot", NumPivot);
  pmgr.GetParamOptional("minCandidate", MinCandidate);
  pmgr.GetParamOptional("chunkBucket", ChunkBucket);

  if (PrefixLength == 0 || PrefixLength > NumPivot) {
    LOG(FATAL) << METH_PERMUTATION_PREFIX_IND
               << " requires that prefix length should lie in [1,"
               << NumPivot << "]";
  }
  if (MinCandidate == 0) {
    LOG(FATAL) << METH_PERMUTATION_PREFIX_IND
               << " requires a min # of candidates should be > 0";
  }

  return new PermutationPrefixIndex<dist_t>(
                space, 
                DataObjects,
                NumPivot, 
                PrefixLength, 
                MinCandidate,
                ChunkBucket);

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

REGISTER_METHOD_CREATOR(float,  METH_PERMUTATION_PREFIX_IND, CreatePermutationPrefixIndex)
REGISTER_METHOD_CREATOR(double, METH_PERMUTATION_PREFIX_IND, CreatePermutationPrefixIndex)
REGISTER_METHOD_CREATOR(int,    METH_PERMUTATION_PREFIX_IND, CreatePermutationPrefixIndex)


}

