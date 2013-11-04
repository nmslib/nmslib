/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _PERMUTATION_PREFIX_INDEX_H_
#define _PERMUTATION_PREFIX_INDEX_H_

#include <string>
#include "index.h"
#include "permutation_utils.h"

#define METH_PERMUTATION_PREFIX_IND "perm_prefix"

namespace similarity {

/*
 * Permutation Prefix Index: 
 * Esuli, 2012.
 * Using Permutation Prefixes for Efficient and Scalable Approximate Similarity Search.
 */

template <typename dist_t>
class Space;

class PrefixTree;

template <typename dist_t>
class PermutationPrefixIndex : public Index<dist_t> {
 public:
  PermutationPrefixIndex(const Space<dist_t>* space,
                         const ObjectVector& data,
                         const size_t num_pivot,
                         const size_t prefix_length,
                         const size_t min_candidate,
                         bool chunk_bucket);
  ~PermutationPrefixIndex();

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

 private:
  template <typename QueryType>
  void GenSearch(QueryType* query);

  // permutation prefix length (l in the original paper) in (0, num_pivot]
  const size_t prefix_length_;
  const size_t min_candidate_;
  // min # of candidates to be selected (z in the original paper)
  ObjectVector pivot_;
  PrefixTree* prefixtree_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PermutationPrefixIndex);
};

}  // namespace similarity

#endif     // _PERMUTATION_PREFIX_INDEX_H_

