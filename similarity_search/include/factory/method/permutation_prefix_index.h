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

}

#endif
