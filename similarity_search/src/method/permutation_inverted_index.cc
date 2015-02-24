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
#include <unordered_map>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "incremental_quick_select.h"
#include "method/permutation_inverted_index.h"
#include "ported_boost_progress.h"
#include "utils.h"

namespace similarity {

using namespace std;


template <typename dist_t>
vector<string>
PermutationInvertedIndex<dist_t>::GetQueryTimeParamNames() const {
  return vector<string>({"numPivotSearch", "maxPosDiff", "dbScanFraction", "knnAmp"});
}
    
    
template <typename dist_t>
void 
PermutationInvertedIndex<dist_t>::SetQueryTimeParamsInternal(AnyParamManager& pmgr) {
  if (pmgr.hasParam("dbScanFrac") && pmgr.hasParam("knnAmp")) {
    throw runtime_error("One shouldn't specify both parameters dbScanFrac and knnAmp");
  }
  if (!pmgr.hasParam("dbScanFrac") && !pmgr.hasParam("knnAmp")) {
    throw runtime_error("One should specify either dbScanFrac or knnAmp");
  }
  if (pmgr.hasParam("knnAmp")) {
    db_scan_frac_ = 0;
  } else {
    knn_amp_ = 0;
  }
  pmgr.GetParamOptional("dbScanFrac",   db_scan_frac_);
  pmgr.GetParamOptional("knnAmp",  knn_amp_);


  pmgr.GetParamOptional("numPivotSearch", num_pivot_search_);
  pmgr.GetParamOptional("maxPosDiff",     max_pos_diff_);

  if (num_pivot_search_ > num_pivot_index_) {
    stringstream err;
    err << METH_PERM_INVERTED_INDEX << " requires that numPivotSearch "
               << "should be less than or equal to numPivotIndex";
    throw runtime_error(err.str());
  }
}
   

template <typename dist_t>
PermutationInvertedIndex<dist_t>::PermutationInvertedIndex(
    bool  PrintProgress,
    const Space<dist_t>* space,
    const ObjectVector& data,
    AnyParams params)
    : data_(data) {
  db_scan_frac_     = 0;
  knn_amp_          = 0;
  num_pivot_        = 512;
  num_pivot_index_  = 32;
  num_pivot_search_ = 16;
  max_pos_diff_ = num_pivot_;

  AnyParamManager pmgr(params);

  pmgr.GetParamOptional("numPivot", num_pivot_);
  pmgr.GetParamOptional("numPivotIndex", num_pivot_index_);

  SetQueryTimeParamsInternal(pmgr);

  pmgr.CheckUnused();

  if (num_pivot_index_ > num_pivot_) {
    stringstream err;
    err << METH_PERM_INVERTED_INDEX << " requires that numPivotIndex "
               << "should be less than or equal to numPivot";
    throw runtime_error(err.str());
  }

  LOG(LIB_INFO) << "# pivots             = "              << num_pivot_;
  LOG(LIB_INFO) << "# pivots to index requested (ki)  = " << num_pivot_index_;
  LOG(LIB_INFO) << "# pivots to search (ks) = "           << num_pivot_search_;
  LOG(LIB_INFO) << "# max position difference = "         << max_pos_diff_;
  LOG(LIB_INFO) << "# dbScanFrac              = "         << db_scan_frac_;
  LOG(LIB_INFO) << "# knnAmp                  = "         << knn_amp_;

  unique_ptr<ProgressDisplay>   progress_bar(PrintProgress ? 
                                              new ProgressDisplay(data.size(), cerr):
                                              NULL);


  GetPermutationPivot(data, space, num_pivot_, &pivot_);

  posting_lists_.resize(num_pivot_);


  for (size_t id = 0; id < data.size(); ++id) {
    Permutation perm;
    GetPermutation(pivot_, space, data[id], &perm);
    for (size_t j = 0; j < perm.size(); ++j) {
      if (perm[j] < num_pivot_index_) {
        posting_lists_[j].push_back(ObjectInvEntry(id, perm[j]));
      }
    }
  if (progress_bar) ++(*progress_bar);
  }
  for (size_t j = 0; j < posting_lists_.size(); ++j) {
    sort(posting_lists_[j].begin(), posting_lists_[j].end());
  }
  if (progress_bar) { // make it 100%
    (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
  }
}

template <typename dist_t>
PermutationInvertedIndex<dist_t>::~PermutationInvertedIndex() {
}

template <typename dist_t>
const string PermutationInvertedIndex<dist_t>::ToString() const {
  stringstream str;
  str <<  "(permutation) inverted index";
  return str.str();
}

template <typename dist_t>
template <typename QueryType>
void PermutationInvertedIndex<dist_t>::GenSearch(QueryType* query, size_t K) {
  // Let's make this check here. Otherwise, if you misspell dbScanFrac, you will get 
  // a strange error message that says: dbScanFrac should be in the range [0,1].
  if (!knn_amp_) {
    if (db_scan_frac_ < 0.0 || db_scan_frac_ > 1.0) {
      stringstream err;
      err << METH_PERM_INVERTED_INDEX << " requires that dbScanFrac is in the range [0,1]";
      throw runtime_error(err.str());
    }
  }


  size_t db_scan = computeDbScan(K);

  Permutation perm_q;
  GetPermutation(pivot_, query, &perm_q);
  vector<typename vector<typename PermutationInvertedIndex<dist_t>::ObjectInvEntry>::iterator>  iterBegs;
  vector<typename vector<typename PermutationInvertedIndex<dist_t>::ObjectInvEntry>::iterator>  iterEnds;

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

  //LOG(LIB_INFO) << "bUseMap: " << bUseMap << " maxScanQty: " << maxScanQty << " data.size() " << data_.size();

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
  size_t scan_qty = min(db_scan, perm_dists.size());

  IncrementalQuickSelect<IntInt> quick_select(perm_dists);
  for (size_t i = 0; i < scan_qty; ++i) {
    const size_t idx = quick_select.GetNext().second;
    quick_select.Next();
    query->CheckAndAddToResult(data_[idx]);
  }
}

template <typename dist_t>
void PermutationInvertedIndex<dist_t>::Search(
    RangeQuery<dist_t>* query) {
  GenSearch(query, 0);
}

template <typename dist_t>
void PermutationInvertedIndex<dist_t>::Search(
    KNNQuery<dist_t>* query) {
  GenSearch(query, query->GetK());
}

template class PermutationInvertedIndex<float>;
template class PermutationInvertedIndex<double>;
template class PermutationInvertedIndex<int>;

}  // namespace similarity
