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
#include <unordered_map>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "incremental_quick_select.h"
#include "permutation_inverted_index.h"
#include "utils.h"

namespace similarity {

using namespace std;

template <typename dist_t>
PermutationInvertedIndex<dist_t>::PermutationInvertedIndex(
    const Space<dist_t>* space,
    const ObjectVector& data,
    const size_t num_pivot,
    const size_t num_pivot_index,
    const size_t num_pivot_search,
    const size_t max_pos_diff,
    const double db_scan_fraction)
    : data_(data),   // reference
      db_scan_(static_cast<size_t>(db_scan_fraction * data.size())),
      num_pivot_index_(min(num_pivot_index, num_pivot_search + max_pos_diff)),
      num_pivot_search_(num_pivot_search),
      max_pos_diff_(max_pos_diff) {
  CHECK(num_pivot_search > 0);
  CHECK(num_pivot_search <= num_pivot_index);
  CHECK(num_pivot_index <= num_pivot);
  LOG(INFO) << "# pivots             = " << num_pivot;
  LOG(INFO) << "# pivots to index requested (ki)  = " << num_pivot_index;
  LOG(INFO) << "# pivots to index effective (ki)  = " << num_pivot_index_;
  LOG(INFO) << "# pivots search (ks) = " << num_pivot_search_;
  LOG(INFO) << "# max position difference = " << max_pos_diff_;

  GetPermutationPivot(data, space, num_pivot, &pivot_);

  posting_lists_.resize(num_pivot);


  for (size_t id = 0; id < data.size(); ++id) {
    Permutation perm;
    GetPermutation(pivot_, space, data[id], &perm);
    for (size_t j = 0; j < perm.size(); ++j) {
      if (perm[j] < num_pivot_index_) {
        posting_lists_[j].push_back(ObjectInvEntry(id, perm[j]));
      }
    }
  }
  for (size_t j = 0; j < posting_lists_.size(); ++j) {
    sort(posting_lists_[j].begin(), posting_lists_[j].end());
  }
}

template <typename dist_t>
PermutationInvertedIndex<dist_t>::~PermutationInvertedIndex() {
}

template <typename dist_t>
const string PermutationInvertedIndex<dist_t>::ToString() const {
  stringstream str;
  str <<  "inverted index";
  return str.str();
}

template <typename dist_t>
template <typename QueryType>
void PermutationInvertedIndex<dist_t>::GenSearch(QueryType* query) {
  Permutation perm_q;
  GetPermutation(pivot_, query, &perm_q);

// TODO @leo or @bileg If you uncomment, you get higher recall for some reason, strange...
#if 0
  vector<IntInt> perm_dists(data_.size());
  for (size_t i = 0; i < data_.size(); ++i) {
    perm_dists[i] = make_pair(num_pivot_search_ * num_pivot_index_, i);
  }

  for (size_t i = 0; i < perm_q.size(); ++i) {
    if (perm_q[i] < num_pivot_search_) {
      for (auto& v : posting_lists_[i]) {
        // spearman footrule
        int spearman_dist = std::abs(static_cast<int>(v.pos_) - static_cast<int>(perm_q[i]));
        // spearman rho
        //spearman_dist = spearman_dist * spearman_dist;
        perm_dists[v.id_].first += spearman_dist - static_cast<int>(num_pivot_index_);
      }
    }
  }

  IncrementalQuickSelect<IntInt> quick_select(perm_dists);

  size_t scan_qty = min(db_scan_, perm_dists.size());

  for (size_t i = 0; i < scan_qty; ++i) {
    const size_t idx = quick_select.GetNext().second;
    quick_select.Next();
    query->CheckAndAddToResult(data_[idx]);
  }
#else
  vector<vector<ObjectInvEntry>::iterator>  iterBegs;
  vector<vector<ObjectInvEntry>::iterator>  iterEnds;

  size_t maxScanQty = 0;

  for (size_t i = 0; i < perm_q.size(); ++i) {
    if (perm_q[i] < num_pivot_search_) {
      ObjectInvEntry  o1(0, std::max(perm_q[i] - static_cast<int>(max_pos_diff_), 0));
      ObjectInvEntry  o2(0, std::min(perm_q[i] + static_cast<int>(max_pos_diff_) + 1, static_cast<int>(num_pivot_index_)));

      auto itEnd = lower_bound(posting_lists_[i].begin(), posting_lists_[i].end(), o2);
      auto it = lower_bound(posting_lists_[i].begin(), itEnd, o1);

      maxScanQty += itEnd - it;
      iterBegs.push_back(it);
      iterEnds.push_back(itEnd);
    }
  }

  bool bUseMap = maxScanQty < USE_MAP_THRESHOLD * data_.size();  // TODO: @leo this is rather adhoc

  vector<IntInt> perm_dists;

  //LOG(INFO) << "bUseMap: " << bUseMap << " maxScanQty: " << maxScanQty << " data.size() " << data_.size();

  bUseMap = false;

  if (bUseMap) {
    unordered_map<int,int> perm_dists_set;

    int MaxDist = (num_pivot_search_ - 1) * num_pivot_index_; 

    for (size_t i = 0, inum = 0; i < perm_q.size(); ++i) {
      if (perm_q[i] < num_pivot_search_) {
        auto        it    = iterBegs[inum];
        const auto& itEnd = iterEnds[inum];
        ++inum;

        for (; it != itEnd; ++it) {
          int id = it->id_;
          int pos = it->pos_;

          CHECK(std::abs(pos - perm_q[i]) <= max_pos_diff_);

          // spearman footrule
          int spearman_dist = std::abs(static_cast<int>(pos) - static_cast<int>(perm_q[i]));

          auto iter = perm_dists_set.find(id);

          if (iter != perm_dists_set.end()) {
            iter->second += spearman_dist - static_cast<int>(num_pivot_index_);
          } else {
            perm_dists_set.insert(make_pair(id, MaxDist + spearman_dist));
          }
        }
      }
    }
    // Copy data from set to array
    perm_dists.reserve(perm_dists_set.size());

    for(const auto it: perm_dists_set) {
      // Id goes second
      perm_dists.push_back(make_pair(it.second, it.first));
    }
  } else {
    int MaxDist = num_pivot_search_ * num_pivot_index_; 

    perm_dists.reserve(data_.size());

    for (size_t i = 0; i < data_.size(); ++i)
      perm_dists.push_back(make_pair(MaxDist, i));

    for (size_t i = 0, inum = 0; i < perm_q.size(); ++i) {
      if (perm_q[i] < num_pivot_search_) {
        auto        it    = iterBegs[inum];
        const auto& itEnd = iterEnds[inum];
        ++inum;

        for (; it < itEnd; ++it) {
          int id = it->id_;
          int pos = it->pos_;

          CHECK(std::abs(pos - perm_q[i]) <= max_pos_diff_);

          // spearman footrule
          int spearman_dist = std::abs(static_cast<int>(pos) - static_cast<int>(perm_q[i]));
          perm_dists[id].first += spearman_dist - static_cast<int>(num_pivot_index_);
        }
      }
    }
  }

  size_t scan_qty = min(db_scan_, perm_dists.size());

  IncrementalQuickSelect<IntInt> quick_select(perm_dists);
  for (size_t i = 0; i < scan_qty; ++i) {
    const size_t idx = quick_select.GetNext().second;
    quick_select.Next();
    query->CheckAndAddToResult(data_[idx]);
  }
#endif
}

template <typename dist_t>
void PermutationInvertedIndex<dist_t>::Search(
    RangeQuery<dist_t>* query) {
  GenSearch(query);
}

template <typename dist_t>
void PermutationInvertedIndex<dist_t>::Search(
    KNNQuery<dist_t>* query) {
  GenSearch(query);
}

template class PermutationInvertedIndex<float>;
template class PermutationInvertedIndex<double>;
template class PermutationInvertedIndex<int>;

}  // namespace similarity
