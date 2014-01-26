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

#ifndef _PERMUTATION_INDEX_INCREMENTAL_BINARY_H_
#define _PERMUTATION_INDEX_INCREMENTAL_BINARY_H_

#include <vector>
#include "index.h"
#include "permutation_utils.h"


#define METH_PERMUTATION_INC_SORT_BIN   "perm_incsort_bin"

namespace similarity {

/* 
 * A simplified version of the brief permutation index.
 * Unlike the original version, all binarized permutations
 * are sought for sequentially.
 * 
 * A brief index for proximity searching
 * ES Téllez, E Chávez, A Camarena-Ibarrola
 */

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
class PermutationIndexIncrementalBin : public Index<dist_t> {
 public:
  PermutationIndexIncrementalBin(const Space<dist_t>* space,
                              const ObjectVector& data,
                              const size_t num_pivot,
                              const size_t bin_threshold,
                              const double db_scan_percentage);
  ~PermutationIndexIncrementalBin();

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

 private:
  const ObjectVector& data_;
  ObjectVector        pivot_;
  const size_t        bin_threshold_;
  const size_t        db_scan_;
  const size_t        bin_perm_word_qty_;

  std::vector<uint32_t> permtable_;

  template <typename QueryType> void GenSearch(QueryType* query);

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PermutationIndexIncrementalBin);
};

}  // namespace similarity

#endif     // _PERMUTATION_INDEX_INCREMENTAL_BINARY_H_

