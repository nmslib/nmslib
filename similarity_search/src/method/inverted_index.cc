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

#include <algorithm>
#include <sstream>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "incremental_quick_select.h"
#include "inverted_index.h"
#include "utils.h"

namespace similarity {

template <typename dist_t>
InvertedIndex<dist_t>::InvertedIndex(
    const Space<dist_t>* space,
    const ObjectVector& data,
    const size_t num_pivot,
    const size_t num_pivot_index,
    const size_t num_pivot_search,
    const double db_scan_fraction)
    : data_(data),   // reference
      db_scan_(static_cast<size_t>(db_scan_fraction * data.size())),
      num_pivot_index_(num_pivot_index),
      num_pivot_search_(num_pivot_search) {
  CHECK(num_pivot_search > 0);
  CHECK(num_pivot_search <= num_pivot_index);
  CHECK(num_pivot_index <= num_pivot);
  LOG(INFO) << "# pivots             = " << num_pivot;
  LOG(INFO) << "# pivots index (ki)  = " << num_pivot_index_;
  LOG(INFO) << "# pivots search (ks) = " << num_pivot_search_;

  GetPermutationPivot(data, space, num_pivot, &pivot_);

  posting_lists_.resize(num_pivot);
  for (size_t i = 0; i < data.size(); ++i) {
    Permutation perm;
    GetPermutation(pivot_, space, data[i], &perm);
    for (size_t j = 0; j < perm.size(); ++j) {
      if (perm[j] < num_pivot_index_) {
        posting_lists_[j].push_back(IdPosPair(i, perm[j]));
      }
    }
  }
}

template <typename dist_t>
InvertedIndex<dist_t>::~InvertedIndex() {
}

template <typename dist_t>
const std::string InvertedIndex<dist_t>::ToString() const {
  std::stringstream str;
  str <<  "inverted index";
  return str.str();
}

template <typename dist_t>
template <typename QueryType>
void InvertedIndex<dist_t>::GenSearch(QueryType* query) {
  Permutation perm_q;
  GetPermutation(pivot_, query, &perm_q);

  std::vector<IntInt> perm_dists(data_.size());
  for (size_t i = 0; i < data_.size(); ++i) {
    perm_dists[i] = std::make_pair(num_pivot_search_ * num_pivot_index_, i);
  }

  for (size_t i = 0; i < perm_q.size(); ++i) {
    if (perm_q[i] < num_pivot_search_) {
      for (auto& v : posting_lists_[i]) {
        // spearman footrule
        int spearman_dist = std::abs(static_cast<int>(v.second) - static_cast<int>(perm_q[i]));
        // spearman rho
        //spearman_dist = spearman_dist * spearman_dist;
        perm_dists[v.first].first += spearman_dist - num_pivot_index_;
      }
    }
  }

  IncrementalQuickSelect<IntInt> quick_select(perm_dists);
  for (size_t i = 0; i < db_scan_; ++i) {
    const size_t idx = quick_select.GetNext().second;
    quick_select.Next();
    query->CheckAndAddToResult(data_[idx]);
  }
}

template <typename dist_t>
void InvertedIndex<dist_t>::Search(
    RangeQuery<dist_t>* query) {
  GenSearch(query);
}

template <typename dist_t>
void InvertedIndex<dist_t>::Search(
    KNNQuery<dist_t>* query) {
  GenSearch(query);
}

template class InvertedIndex<float>;
template class InvertedIndex<double>;
template class InvertedIndex<int>;

}  // namespace similarity
