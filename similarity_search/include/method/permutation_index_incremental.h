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

#ifndef _PERMUTATION_INDEX_INCREMENTAL_H_
#define _PERMUTATION_INDEX_INCREMENTAL_H_

#include <vector>
#include "index.h"
#include "permutation_utils.h"

// If set, permutation vectors are stored contiguously in memory
#define CONTIGUOUS_STORAGE

#define METH_PERMUTATION_INC_SORT   "perm_incsort"

namespace similarity {

/* 
 * Edgar Chávez et al., Effective Proximity Retrieval by Ordering Permutations.
 *                      IEEE Trans. Pattern Anal. Mach. Intell. (2008)
 */

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
class PermutationIndexIncremental : public Index<dist_t> {
 public:
  PermutationIndexIncremental(const Space<dist_t>* space,
                              const ObjectVector& data,
                              const size_t num_pivot,
                              const double db_scan_percentage);
  ~PermutationIndexIncremental();

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

 private:
  const ObjectVector& data_;
  const size_t db_scan_;
  ObjectVector pivot_;
#ifdef CONTIGUOUS_STORAGE
  std::vector<PivotIdType> permtable_;
#else
  std::vector<Permutation> permtable_;
#endif

  template <typename QueryType> void GenSearch(QueryType* query);

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PermutationIndexIncremental);
};

}  // namespace similarity

#endif     // _PERMUTATION_INDEX_INCREMENTAL_H_

