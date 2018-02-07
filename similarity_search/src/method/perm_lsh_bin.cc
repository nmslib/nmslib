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

template <typename dist_t>
PermutationIndexLSHBin<dist_t>::PermutationIndexLSHBin(
    bool PrintProgress,
    const Space<dist_t>& space,
    const ObjectVector& data) : 
                                Index<dist_t>(data), space_(space), printProgress_(PrintProgress) {}

template <typename dist_t>
void PermutationIndexLSHBin<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("numPivot",     num_pivot_,       32);
  pmgr.GetParamOptional("binThreshold", bin_threshold_,   num_pivot_ / 2);
  pmgr.GetParamOptional("bitSampleQty", bit_sample_qty_,  20 /* good for 1m records */);
  pmgr.GetParamOptional("L",            num_hash_,        50);

  if (!bit_sample_qty_) {
    throw runtime_error("bitSampleQty should be non-zero");
  }

  if (bit_sample_qty_ > 63 || bit_sample_qty_ > num_pivot_) {
    stringstream err;
    err << "bitSampleQty cannot be larger than " << min(size_t(63), num_pivot_);
    throw runtime_error(err.str());
  }

  hash_table_size_ = 1 << bit_sample_qty_;
  
  LOG(LIB_INFO) << "# of hashes      = " << num_hash_;
  LOG(LIB_INFO) << "hashe table size = " << hash_table_size_;
  LOG(LIB_INFO) << "# pivots         = " << num_pivot_;
  LOG(LIB_INFO) << "bin threshold    = " << bin_threshold_;
  LOG(LIB_INFO) << "bit sample qty   = " << bit_sample_qty_;

  pmgr.CheckUnused();
  this->ResetQueryTimeParams();

  pivots_.resize(num_hash_);
  bit_sample_flags_.resize(num_hash_);

  for (size_t i = 0; i < num_hash_; ++i) {
    GetPermutationPivot(this->data_, space_, num_pivot_, &pivots_[i]);
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

  unique_ptr<ProgressDisplay> progress_bar(printProgress_ ?
                                new ProgressDisplay(this->data_.size(), cerr)
                                :NULL);
  for (size_t id = 0; id < this->data_.size(); ++id) {
    for (size_t hashId = 0; hashId < num_hash_; ++hashId) {
      size_t val = computeHashValue(hashId, this->data_[id], NULL); // Already <= hash_table_size_;
      //cout << val << endl;
      if (!hash_tables_[hashId][val]) {
        hash_tables_[hashId][val] = new vector<IdType>();
      }
      hash_tables_[hashId][val]->push_back(id);
    }
    if (progress_bar) ++(*progress_bar);
  }
}

template <typename dist_t>
PermutationIndexLSHBin<dist_t>::~PermutationIndexLSHBin() { 
  for (size_t hashId = 0; hashId < num_hash_; ++hashId) {
    for (auto e: hash_tables_[hashId]) delete e;
  }
}

template <typename dist_t>
template <typename QueryType>
void PermutationIndexLSHBin<dist_t>::GenSearch(QueryType* query) const {
  std::unordered_set<IdType> found;

  for (size_t hashId = 0; hashId < num_hash_; ++hashId) {
    size_t val = computeHashValue(hashId, NULL, query); // Already <= hash_table_size_;

    vector<IdType>* pObjIds = hash_tables_[hashId][val]; 
    if (pObjIds) {
      for (IdType id : *pObjIds)
      /*  
       * It's essential to check for previously added entries.
       * If we don't do this, the same close entry may be added multiple
       * times. At the same time, other relevant entries will be removed!
       */
      if (!found.count(id)) {
        query->CheckAndAddToResult(this->data_[id]);
        found.insert(id);
      }
    }
  }
}

template class PermutationIndexLSHBin<float>;
template class PermutationIndexLSHBin<double>;
template class PermutationIndexLSHBin<int>;

}  // namespace similarity

