#ifndef INIT_METHODS_H
#define INIT_METHODS_H
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

#include "methodfactory.h"

#include "factory/method/lsh.h"
#include "factory/method/lsh_multiprobe.h"
#include "factory/method/dummy.h"
#include "factory/method/bbtree.h"
#include "factory/method/ghtree.h"
#include "factory/method/list_clusters.h"
#include "factory/method/multi_index.h"
#include "factory/method/multi_vantage_point_tree.h"
#include "factory/method/perm_bin_vptree.h"
#include "factory/method/perm_index_incr_bin.h"
#include "factory/method/permutation_index.h"
#include "factory/method/permutation_index_incremental.h"
#include "factory/method/permutation_inverted_index.h"
#include "factory/method/permutation_prefix_index.h"
#include "factory/method/permutation_vptree.h"
#include "factory/method/pivot_neighb_invindx.h"
#include "factory/method/proj_vptree.h"
#include "factory/method/seqsearch.h"
#include "factory/method/small_world_rand.h"
#include "factory/method/spatial_approx_tree.h"
#include "factory/method/vptree.h"

namespace similarity {


inline void initMethods() {
  // a dummy method
  REGISTER_METHOD_CREATOR(float,  METH_DUMMY, CreateDummy)
  REGISTER_METHOD_CREATOR(double, METH_DUMMY, CreateDummy)
  REGISTER_METHOD_CREATOR(int,    METH_DUMMY, CreateDummy)
  
  // bbtree
  REGISTER_METHOD_CREATOR(float,  METH_BBTREE, CreateBBTree)
  REGISTER_METHOD_CREATOR(double, METH_BBTREE, CreateBBTree)

  // ghtree
  REGISTER_METHOD_CREATOR(float,  METH_GHTREE, CreateGHTree)
  REGISTER_METHOD_CREATOR(double, METH_GHTREE, CreateGHTree)
  REGISTER_METHOD_CREATOR(int,    METH_GHTREE, CreateGHTree)

  // list of clusters
  REGISTER_METHOD_CREATOR(float,  METH_LIST_CLUSTERS, CreateListClusters)
  REGISTER_METHOD_CREATOR(double, METH_LIST_CLUSTERS, CreateListClusters)
  REGISTER_METHOD_CREATOR(int,    METH_LIST_CLUSTERS, CreateListClusters)

  // Regular LSH
  REGISTER_METHOD_CREATOR(float,  METH_LSH_CAUCHY, CreateLSHCauchy)
  REGISTER_METHOD_CREATOR(float,  METH_LSH_GAUSSIAN, CreateLSHGaussian)
  REGISTER_METHOD_CREATOR(float,  METH_LSH_THRESHOLD, CreateLSHThreshold)

  // Multiprobe LSH
  REGISTER_METHOD_CREATOR(float,  METH_LSH_MULTIPROBE, CreateLSHMultiprobe)

  // Multi-vantage point tree
  REGISTER_METHOD_CREATOR(float,  METH_MVPTREE, CreateMultiVantagePointTree)
  REGISTER_METHOD_CREATOR(double, METH_MVPTREE, CreateMultiVantagePointTree)
  REGISTER_METHOD_CREATOR(int,    METH_MVPTREE, CreateMultiVantagePointTree)

  // Vptree over binarized permutations
  REGISTER_METHOD_CREATOR(float,  METH_PERM_BIN_VPTREE, CreatePermutationBinVPTree)
  REGISTER_METHOD_CREATOR(double, METH_PERM_BIN_VPTREE, CreatePermutationBinVPTree)
  REGISTER_METHOD_CREATOR(int,    METH_PERM_BIN_VPTREE, CreatePermutationBinVPTree)

  // Sequential-search binarized permutation index with incremental sorting
  REGISTER_METHOD_CREATOR(float,  METH_PERMUTATION_INC_SORT_BIN, CreatePermutationIndexIncrementalBin)
  REGISTER_METHOD_CREATOR(double, METH_PERMUTATION_INC_SORT_BIN, CreatePermutationIndexIncrementalBin)
  REGISTER_METHOD_CREATOR(int,    METH_PERMUTATION_INC_SORT_BIN, CreatePermutationIndexIncrementalBin)

  // Sequential-search permutation index without incremental sorting
  REGISTER_METHOD_CREATOR(float,  METH_PERMUTATION, CreatePermutationIndex)
  REGISTER_METHOD_CREATOR(double, METH_PERMUTATION, CreatePermutationIndex)
  REGISTER_METHOD_CREATOR(int,    METH_PERMUTATION, CreatePermutationIndex)

  // Sequential-search permutation index with incremental sorting
  REGISTER_METHOD_CREATOR(float,  METH_PERMUTATION_INC_SORT, CreatePermutationIndexIncremental)
  REGISTER_METHOD_CREATOR(double, METH_PERMUTATION_INC_SORT, CreatePermutationIndexIncremental)
  REGISTER_METHOD_CREATOR(int,    METH_PERMUTATION_INC_SORT, CreatePermutationIndexIncremental)

  // Inverted index over permutations
  REGISTER_METHOD_CREATOR(float,  METH_PERM_INVERTED_INDEX, CreatePermInvertedIndex)
  REGISTER_METHOD_CREATOR(double, METH_PERM_INVERTED_INDEX, CreatePermInvertedIndex)
  REGISTER_METHOD_CREATOR(int,    METH_PERM_INVERTED_INDEX, CreatePermInvertedIndex)

  // Prefix index over permutations
  REGISTER_METHOD_CREATOR(float,  METH_PERMUTATION_PREFIX_IND, CreatePermutationPrefixIndex)
  REGISTER_METHOD_CREATOR(double, METH_PERMUTATION_PREFIX_IND, CreatePermutationPrefixIndex)
  REGISTER_METHOD_CREATOR(int,    METH_PERMUTATION_PREFIX_IND, CreatePermutationPrefixIndex)

  // VP-tree built over permutations
  REGISTER_METHOD_CREATOR(float,  METH_PERMUTATION_VPTREE, CreatePermutationVPTree)
  REGISTER_METHOD_CREATOR(double, METH_PERMUTATION_VPTREE, CreatePermutationVPTree)
  REGISTER_METHOD_CREATOR(int,    METH_PERMUTATION_VPTREE, CreatePermutationVPTree)

  // Inverted index over permutation-based neighborhoods
  REGISTER_METHOD_CREATOR(float,  METH_PIVOT_NEIGHB_INVINDEX, CreatePivotNeighbInvertedIndex)
  REGISTER_METHOD_CREATOR(double, METH_PIVOT_NEIGHB_INVINDEX, CreatePivotNeighbInvertedIndex)
  REGISTER_METHOD_CREATOR(int,    METH_PIVOT_NEIGHB_INVINDEX, CreatePivotNeighbInvertedIndex)

  // VP-tree built over projections
  REGISTER_METHOD_CREATOR(float,  METH_PROJ_VPTREE, CreateProjVPTree)
  REGISTER_METHOD_CREATOR(double, METH_PROJ_VPTREE, CreateProjVPTree)

  // Just sequential searching
  REGISTER_METHOD_CREATOR(float,  METH_SEQ_SEARCH, CreateSeqSearch)
  REGISTER_METHOD_CREATOR(double, METH_SEQ_SEARCH, CreateSeqSearch)
  REGISTER_METHOD_CREATOR(int,    METH_SEQ_SEARCH, CreateSeqSearch)

  // Small-word with randomly generated neighborhood-networks
  REGISTER_METHOD_CREATOR(float,  METH_SMALL_WORLD_RAND, CreateSmallWorldRand)
  REGISTER_METHOD_CREATOR(double, METH_SMALL_WORLD_RAND, CreateSmallWorldRand)
  REGISTER_METHOD_CREATOR(int,    METH_SMALL_WORLD_RAND, CreateSmallWorldRand)

  // SA-tree
  REGISTER_METHOD_CREATOR(float,  METH_SATREE, CreateSATree)
  REGISTER_METHOD_CREATOR(double, METH_SATREE, CreateSATree)
  REGISTER_METHOD_CREATOR(int,    METH_SATREE, CreateSATree)

  // VP-tree, piecewise-linear approximation of the decision rule
  REGISTER_METHOD_CREATOR(int,    METH_VPTREE, CreateVPTreeTriang)
  REGISTER_METHOD_CREATOR(float,  METH_VPTREE, CreateVPTreeTriang)
  REGISTER_METHOD_CREATOR(double, METH_VPTREE, CreateVPTreeTriang)

  // VP-tree, sampling-based (not so good yet) approximation of the decision rule
  REGISTER_METHOD_CREATOR(int,    METH_VPTREE_SAMPLE, CreateVPTreeSample)
  REGISTER_METHOD_CREATOR(float,  METH_VPTREE_SAMPLE, CreateVPTreeSample)
  REGISTER_METHOD_CREATOR(double, METH_VPTREE_SAMPLE, CreateVPTreeSample)

  // A multi-index combination
  REGISTER_METHOD_CREATOR(float,  METH_MULT_INDEX, CreateMultiIndex)
  REGISTER_METHOD_CREATOR(double, METH_MULT_INDEX, CreateMultiIndex)
  REGISTER_METHOD_CREATOR(int,    METH_MULT_INDEX, CreateMultiIndex)

}



}

#endif
