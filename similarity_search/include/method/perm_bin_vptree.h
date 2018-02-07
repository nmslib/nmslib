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
#ifndef _PERM_BIN_VPTREE_H_
#define _PERM_BIN_VPTREE_H_

#include <vector>

#include "index.h"
#include "space/space_bit_hamming.h"
#include "permutation_utils.h"
#include "vptree.h"
#include "params.h"
#include "searchoracle.h"

#define METH_PERM_BIN_VPTREE     "perm_bin_vptree"

namespace similarity {

/*
 * Brief permutation index. Binarized permutations are stored in the VP-tree.
 *
 * Similar to :
 *
 * A brief index for proximity searching
 * ES Téllez, E Chávez, A Camarena-Ibarrola 
 *
 * The difference from their work: we search permutations using APPROXIMATE near-neighbor search.
 *
 */
template <typename dist_t, PivotIdType (*CorrelDistFunc)(const PivotIdType*, const PivotIdType*, size_t)>
class PermBinVPTree : public Index<dist_t> {
 public:
  PermBinVPTree(bool PrintProgress,
                Space<dist_t>& space,
                const ObjectVector& data);

  void CreateIndex(const AnyParams& IndexParams) override;

  ~PermBinVPTree();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;
 private:

  Space<dist_t>&            space_;
  bool                      PrintProgress_;
  size_t                    bin_threshold_;
  size_t                    bin_perm_word_qty_;
  size_t                    db_scan_qty_;
  ObjectVector              pivots_;
  ObjectVector              BinPermData_;

  unique_ptr<VPTree<int, PolynomialPruner<int>>>   VPTreeIndex_;
  unique_ptr<SpaceBitHamming>                      VPTreeSpace_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PermBinVPTree);
};

}  // namespace similarity

#endif     // _PERM_BIN_VPTREE_H_

