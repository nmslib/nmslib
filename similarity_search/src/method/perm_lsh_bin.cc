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

#include <algorithm>
#include <sstream>

#include "space.h"
#include "params.h"
#include "rangequery.h"
#include "knnquery.h"
#include "method/perm_lsh_bin.h"
#include "ported_boost_progress.h"
#include "utils.h"

namespace similarity {

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
PermutationIndexLSHBin<dist_t, perm_func>::PermutationIndexLSHBin(
    bool PrintProgress,
    const Space<dist_t>* space,
    const ObjectVector& data,
    const AnyParams& MethParams) 
	: data_(data), 
    space_(space),
    num_pivot_(32), 
    bin_threshold_(8), 
    bit_sample_qty_(20), // for 1M records
    num_hash_(50)
{
  AnyParamManager pmgr(MethParams);

  pmgr.GetParamOptional("numPivot",     num_pivot_);
  pmgr.GetParamOptional("binThreshold", bin_threshold_);
  pmgr.GetParamOptional("bitSampleQty", bit_sample_qty_);
  pmgr.GetParamOptional("L",            num_hash_);
  
  LOG(LIB_INFO) << "# of hashes      = " << num_hash_;
  LOG(LIB_INFO) << "# pivots         = " << num_pivot_;
  LOG(LIB_INFO) << "bin threshold    = " << bin_threshold_;
  LOG(LIB_INFO) << "bit sample qty   = " << bit_sample_qty_;

  if (!bit_sample_qty_) {
    throw runtime_error("bitSampleQty should be non-zero");
  }

  if (bit_sample_qty_ > 63 || bit_sample_qty_ > num_pivot_) {
    stringstream err;
    err << "bitSampleQty cannot be larger than " << min(size_t(63), num_pivot_);
    throw runtime_error(err.str());
  }

  hash_table_size_ = 1 << bit_sample_qty_;

  pivots_.resize(num_hash_);
  bit_sample_flags_.resize(num_hash_);

  for (size_t i = 0; i < num_hash_; ++i) {
    GetPermutationPivot(data, space, num_pivot_, &pivots_[i]);
    bit_sample_flags_[i].resize(num_pivot_);


    if (bit_sample_qty_ <= num_pivot_) {
      fill(bit_sample_flags_[i].begin(),
           bit_sample_flags_[i].end(), 0);
      // Select bit_sample_qty 1s randomly
      size_t toSelQty = bit_sample_qty_;
      while (toSelQty) {
        while (true) {
          size_t p = RandomInt() % bit_sample_qty_;
          if (!bit_sample_flags_[i][p]) {
            bit_sample_flags_[i][p] = 1;
            --toSelQty;
            break;
          }
        }
      }
    } else {
      // Inversion trick
      fill(bit_sample_flags_[i].begin(),
           bit_sample_flags_[i].end(), 1);
      // Select (num_pivot_ - bit_sample_qty) 0s randomly
      size_t toSelQty = num_pivot_ - bit_sample_qty_;
      while (toSelQty) {
        while (true) {
          size_t p = RandomInt() % bit_sample_qty_;
          if (bit_sample_flags_[i][p]) {
            bit_sample_flags_[i][p] = 0;
            --toSelQty;
            break;
          }
        }
      }
    }
  }

  hash_tables_.resize(num_hash_);

  for (size_t hashId = 0; hashId < num_hash_; ++hashId) {
    hash_tables_[hashId].resize(hash_table_size_);
  }

  unique_ptr<ProgressDisplay> progress_bar(PrintProgress ?
                                new ProgressDisplay(data.size(), cerr)
                                :NULL);
  for (size_t id = 0; id < data.size(); ++id) {
    for (size_t hashId = 0; hashId < num_hash_; ++hashId) {
      size_t val = computeHashValue(hashId, data[id], NULL); // Already <= hash_table_size_;
      //cout << val << endl;
      if (!hash_tables_[hashId][val]) {
        hash_tables_[hashId][val] = new vector<IdType>();
      }
      hash_tables_[hashId][val]->push_back(id);
    }
    if (progress_bar) ++(*progress_bar);
  }
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)>
PermutationIndexLSHBin<dist_t, perm_func>::~PermutationIndexLSHBin() { 
  for (size_t hashId = 0; hashId < num_hash_; ++hashId) {
    for (auto e: hash_tables_[hashId]) delete e;
  }
}

template <typename dist_t, PivotIdType (*perm_func)(const PivotIdType*, const PivotIdType*, size_t)> 
template <typename QueryType>
void PermutationIndexLSHBin<dist_t, perm_func>::GenSearch(QueryType* query) {
  for (size_t hashId = 0; hashId < num_hash_; ++hashId) {
    size_t val = computeHashValue(hashId, NULL, query); // Already <= hash_table_size_;

    vector<IdType>* pObjIds = hash_tables_[hashId][val]; 
    if (pObjIds) {
      for (IdType id : *pObjIds)
        query->CheckAndAddToResult(data_[id]);
    }
  }
}

template class PermutationIndexLSHBin<float,SpearmanRho>;
template class PermutationIndexLSHBin<double,SpearmanRho>;
template class PermutationIndexLSHBin<int,SpearmanRho>;
template class PermutationIndexLSHBin<float,SpearmanFootrule>;
template class PermutationIndexLSHBin<double,SpearmanFootrule>;
template class PermutationIndexLSHBin<int,SpearmanFootrule>;
template class PermutationIndexLSHBin<float,SpearmanRhoSIMD>;
template class PermutationIndexLSHBin<double,SpearmanRhoSIMD>;
template class PermutationIndexLSHBin<int,SpearmanRhoSIMD>;
template class PermutationIndexLSHBin<float,SpearmanFootruleSIMD>;
template class PermutationIndexLSHBin<double,SpearmanFootruleSIMD>;
template class PermutationIndexLSHBin<int,SpearmanFootruleSIMD>;

}  // namespace similarity

