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
#ifndef _PERMUTATION_INDEX_LSH_BIN_
#define _PERMUTATION_INDEX_LSH_BIN_

#include <vector>
#include "index.h"
#include "permutation_utils.h"


#define METH_PERMUTATION_LSH_BIN   "perm_lsh_bin"

namespace similarity {

/* 
 * A simplified version of the LSH based on binarized permutations.
 * 
 * Tellez, Eric Sadit, and Edgar Chavez. 
 * "On locality sensitive hashing in metric spaces." 
 * Proceedings of the SISAP 2010.
 */

template <typename dist_t>
class PermutationIndexLSHBin : public Index<dist_t> {
 public:
  PermutationIndexLSHBin(
                 bool PrintProgress,
                 const Space<dist_t>& space,
                 const ObjectVector& data);

  void CreateIndex(const AnyParams& MethParams) override;
  ~PermutationIndexLSHBin();

  const std::string StrDesc() const override { return "LSH (binary permutations)"; }
  void Search(RangeQuery<dist_t>* query, IdType) const override {GenSearch(query);}
  void Search(KNNQuery<dist_t>* query, IdType) const override {GenSearch(query);}

  void SetQueryTimeParams(const AnyParams &) override {}
 private:
  const Space<dist_t>&  space_;
  bool                  printProgress_;

  size_t                num_pivot_;
  size_t                bin_threshold_;
  size_t                bit_sample_qty_;
  size_t                num_hash_;
  size_t                hash_table_size_;
  vector<ObjectVector>  pivots_;
  vector<vector<char>>  bit_sample_flags_;

  vector<vector<vector<IdType>*>> hash_tables_;

  template <typename QueryType> void GenSearch(QueryType* query) const;

  /*
   * If pQuery is not NULL, distances are computed via the query.
   */
  size_t  computeHashValue(size_t hashId, const Object* pObj, Query<dist_t>* pQuery) const {
    Permutation perm_q;
    if (pQuery) {
      GetPermutation(pivots_[hashId], pQuery, &perm_q);
    } else {
      GetPermutation(pivots_[hashId], space_, pObj, &perm_q);
    }
    size_t res = 0, flag = 1;
    for (size_t i = 0; i < num_pivot_; ++i) 
    if (bit_sample_flags_[hashId][i]) {
      if (perm_q[i] >= bin_threshold_) {
        res |= flag;
      }
      flag <<= 1;
    }
    return res % hash_table_size_;
  }

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PermutationIndexLSHBin);
};

}  // namespace similarity

#endif     

