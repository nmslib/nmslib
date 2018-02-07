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
#ifndef _PERMUTATION_INDEX_INCREMENTAL_BINARY_H_
#define _PERMUTATION_INDEX_INCREMENTAL_BINARY_H_

#include <vector>
#include "index.h"
#include "permutation_utils.h"


#define METH_PERMUTATION_INC_SORT_BIN       "perm_incsort_bin"
#define METH_PERMUTATION_INC_SORT_BIN_SYN   "perm_bin_incsort"

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
  PermutationIndexIncrementalBin(bool PrintProgress,
                              const Space<dist_t>& space,
                              const ObjectVector& data);

  void CreateIndex(const AnyParams& IndexParams) override;
  ~PermutationIndexIncrementalBin();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  virtual void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;

 private:
  const Space<dist_t>&  space_;
  bool                  PrintProgress_;

  ObjectVector        pivot_;

  size_t        num_pivot_;
  size_t        bin_threshold_;
  float         db_scan_frac_; 
  size_t        knn_amp_;
  size_t        bin_perm_word_qty_;
  bool          use_sort_;
  size_t        max_hamming_dist_;
  bool          skip_checking_;

  std::vector<uint32_t> permtable_;
  
  size_t computeDbScan(size_t K) const {
    if (knn_amp_) { return min(K * knn_amp_, this->data_.size()); }
    return static_cast<size_t>(db_scan_frac_ * this->data_.size());
  }

  template <typename QueryType> void GenSearch(QueryType* query, size_t K) const;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PermutationIndexIncrementalBin);
};

}  // namespace similarity

#endif     // _PERMUTATION_INDEX_INCREMENTAL_BINARY_H_

