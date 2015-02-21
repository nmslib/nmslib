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
  PermBinVPTree(const Space<dist_t>* space,
                   const ObjectVector& data,
                   const AnyParams& MethPars);

  ~PermBinVPTree();

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

  vector<string> GetQueryTimeParamNames() const { return VPTreeIndex_->GetQueryTimeParamNames(); }
 private:
  void SetQueryTimeParamsInternal(AnyParamManager& pmgr) {
    AnyParams params;
    pmgr.ExtractParametersExcept({});
    VPTreeIndex_->SetQueryTimeParams(params);
  }

  const Space<dist_t>*      space_;
  const ObjectVector&       data_;
  size_t                    bin_threshold_;
  size_t                    bin_perm_word_qty_;
  size_t                    db_scan_qty_;
  ObjectVector              pivots_;
  ObjectVector              BinPermData_;

  VPTree<int, PolynomialPruner<int>>*   VPTreeIndex_;
  const SpaceBitHamming*                VPTreeSpace_;

  void SetQueryTimeParamsInternal(AnyParams params) { VPTreeIndex_->SetQueryTimeParams(params); }

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PermBinVPTree);
};

}  // namespace similarity

#endif     // _PERM_BIN_VPTREE_H_

