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

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "utils.h"
#include "method/multi_vantage_point_tree_utils.h"
#include "method/multi_vantage_point_tree.h"

namespace similarity {

template <typename dist_t>
MultiVantagePointTree<dist_t>::MultiVantagePointTree(
    const Space<dist_t>& space,
    const ObjectVector& data) : Index<dist_t>(data), space_(space) {
}

template <typename dist_t>
void MultiVantagePointTree<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("maxPathLen", MaxPathLength_,  5);
  pmgr.GetParamOptional("bucketSize", BucketSize_,     50);
  pmgr.GetParamOptional("chunkBucket", ChunkBucket_,   true);

  LOG(LIB_INFO) << "maxPathLen= " << MaxPathLength_;
  LOG(LIB_INFO) << "bucketSize= " << BucketSize_;
  LOG(LIB_INFO) << "chunkBucket=" << ChunkBucket_;

  pmgr.CheckUnused();

  if (BucketSize_ < 2) {
     throw runtime_error("Bug: The bucket size should be at least 2 (multi vantage point tree)");
  }

  Entries entries;
  entries.reserve(this->data_.size());
  for (size_t i = 0; i < this->data_.size(); ++i) {
    entries.push_back(Entry(this->data_[i]));
  }
  root_.reset(BuildTree(&space_, entries));
}

template <typename dist_t>
MultiVantagePointTree<dist_t>::~MultiVantagePointTree() {
}

template <typename dist_t>
const std::string MultiVantagePointTree<dist_t>::StrDesc() const {
  return "mvp-tree";
}

template <typename dist_t>
typename MultiVantagePointTree<dist_t>::Node*
MultiVantagePointTree<dist_t>::BuildTree(
    const Space<dist_t>* space,
    typename MultiVantagePointTree<dist_t>::Entries& entries) {
  if (entries.empty()) {
    return NULL;
  }

  const Object* pivot1 =
      Remove(entries, RandomInt() % entries.size()).object;
  const Object* pivot2 = NULL;

  if (entries.size() + 1 <= BucketSize_ + 2) {
    if (!entries.empty()) {
      size_t pivot2_idx = 0;
      dist_t pivot2_dist = 0;
      for (size_t i = 0; i < entries.size(); ++i) {
        entries[i].d1 = space->IndexTimeDistance(pivot1, entries[i].object);
        if (entries[i].d1 > pivot2_dist) {
          pivot2_idx = i;
          pivot2_dist = entries[i].d1;
        }
      }
      pivot2 = Remove(entries, pivot2_idx).object;
      for (size_t i = 0; i < entries.size(); ++i) {
        entries[i].d2 = space->IndexTimeDistance(pivot2, entries[i].object);
      }
    }
    LeafNode* node = new LeafNode(pivot1, pivot2, entries, ChunkBucket_);
    return node;
  } else {
    dist_t m1 = 0;
    dist_t m21 = 0, m22 = 0;
    MultiVantagePointTree::Entries entry_list11, entry_list12;
    MultiVantagePointTree::Entries entry_list21, entry_list22;
    if (!entries.empty()) {
      for (size_t i = 0; i < entries.size(); ++i) {
        entries[i].d1 = space->IndexTimeDistance(pivot1, entries[i].object);
        if (entries[i].path.size() < MaxPathLength_) {
          entries[i].path.push_back(entries[i].d1);
        }
      }
      std::sort(entries.begin(), entries.end(), Dist1AscComparator());
      CHECK(entries[0].d1 <= entries[entries.size() - 1].d1);
      // entries = elist1+elist2
      MultiVantagePointTree::Entries elist1, elist2;
      m1 = SplitByMedian(entries, elist1, elist2).d1;
      pivot2 = Remove(elist2, RandomInt() % elist2.size()).object;
      for (size_t i = 0; i < elist1.size(); ++i) {
        elist1[i].d2 = space->IndexTimeDistance(pivot2, elist1[i].object);
        if (elist1[i].path.size() < MaxPathLength_) {
          elist1[i].path.push_back(elist1[i].d2);
        }
      }
      for (size_t i = 0; i < elist2.size(); ++i) {
        elist2[i].d2 = space->IndexTimeDistance(pivot2, elist2[i].object);
        if (elist2[i].path.size() < MaxPathLength_) {
          elist2[i].path.push_back(elist2[i].d2);
        }
      }
      std::sort(elist1.begin(), elist1.end(), Dist2AscComparator());
      CHECK(elist1[0].d2 <= elist1[elist1.size() - 1].d2); 
      std::sort(elist2.begin(), elist2.end(), Dist2AscComparator());
      CHECK(elist2[0].d2 <= elist2[elist2.size() - 1].d2); 
      m21 = SplitByMedian(elist1, entry_list11, entry_list12).d2;
      m22 = SplitByMedian(elist2, entry_list21, entry_list22).d2;
    }
    InternalNode* node = new InternalNode(pivot1, pivot2, m1, m21, m22);
    node->child1_ = BuildTree(space, entry_list11);
    node->child2_ = BuildTree(space, entry_list12);
    node->child3_ = BuildTree(space, entry_list21);
    node->child4_ = BuildTree(space, entry_list22);
    return node;
  }
}

template <typename dist_t>
void MultiVantagePointTree<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  int mx = MaxLeavesToVisit_;
  Dists path(MaxPathLength_);
  GenericSearch(root_.get(), query, path, 0, mx);
}

template <typename dist_t>
void MultiVantagePointTree<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  int mx = MaxLeavesToVisit_;
  Dists path(MaxPathLength_);
  GenericSearch(root_.get(), query, path, 0, mx);
}

// Range search algorithm
template <typename dist_t>
template <typename QueryType>
void MultiVantagePointTree<dist_t>::GenericSearch(
    typename MultiVantagePointTree<dist_t>::Node* node,
    QueryType* query,
    Dists& path,
    size_t query_path_len,
    int& MaxLeavesToVisit) const {
  if (node == NULL) {
    return;
  }
  if (MaxLeavesToVisit <= 0) return; // early termination

  const bool exists_p1 = node->pivot1_ != NULL;
  const bool exists_p2 = node->pivot2_ != NULL;
  const dist_t dp1 = exists_p1 ? query->DistanceObjLeft(node->pivot1_) : DistMax<dist_t>();
  const dist_t dp2 = exists_p2 ? query->DistanceObjLeft(node->pivot2_) : DistMax<dist_t>();

  if (exists_p1) query->CheckAndAddToResult(dp1, node->pivot1_);
  if (exists_p2) query->CheckAndAddToResult(dp2, node->pivot2_);

  if (node->isleaf()) {
    --MaxLeavesToVisit;
    const LeafNode* leaf_node = reinterpret_cast<const LeafNode*>(node);
    const Entries& entries = leaf_node->entries_;
    for (size_t i = 0; i < entries.size(); ++i) {
      if (dp1 - query->Radius() <= entries[i].d1 &&
          dp1 + query->Radius() >= entries[i].d1 &&
          dp2 - query->Radius() <= entries[i].d2 &&
          dp2 + query->Radius() >= entries[i].d2) {
        size_t path_len = std::min(query_path_len, entries[i].path.size());
        bool ok = true;
        for (size_t k = 0; k < path_len; ++k) {
          if (path[k] - query->Radius() > entries[i].path[k] ||
              path[k] + query->Radius() < entries[i].path[k]) {
            ok = false;
            break;
          }
        }
        if (ok) {
          query->CheckAndAddToResult(entries[i].object);
        }
      }
    }
  } else {
    const InternalNode* internal_node =
        reinterpret_cast<const InternalNode*>(node);

    if (exists_p1 && query_path_len < MaxPathLength_) {
      path[query_path_len++] = dp1;
    }
    if (exists_p2 && query_path_len < MaxPathLength_) {
      path[query_path_len++] = dp2;
    }

    bool FirstRight1 = dp1 > internal_node->m1_; 

    for (int order1 = 0; order1 < 2; ++ order1) {
      if (order1 == int(FirstRight1) && dp1 - query->Radius() <= internal_node->m1_) {      // left
        bool FirstRight2 = dp2 > internal_node->m21_;
        for (int order2 = 0; order2 < 2; ++order2) {
          if (order2 == int(FirstRight2) && dp2 - query->Radius() <= internal_node->m21_) {   // left-left
            GenericSearch(internal_node->child1_, query, path, query_path_len, MaxLeavesToVisit);
          }
		  if (order2 != int(FirstRight2) && dp2 + query->Radius() >= internal_node->m21_) {   // left-right
            GenericSearch(internal_node->child2_, query, path, query_path_len, MaxLeavesToVisit);
          }
        }
      }
	  if (order1 != int(FirstRight1) && dp1 + query->Radius() >= internal_node->m1_) {      // right
        bool FirstRight2 = dp2 > internal_node->m22_;
        for (int order2 = 0; order2 < 2; ++order2) {
          if (order2 == int(FirstRight2) && dp2 - query->Radius() <= internal_node->m22_) {   // right-left
            GenericSearch(internal_node->child3_, query, path, query_path_len, MaxLeavesToVisit);
          }
		  if (order2 != int(FirstRight2) && dp2 + query->Radius() >= internal_node->m22_) {   // right-right
            GenericSearch(internal_node->child4_, query, path, query_path_len, MaxLeavesToVisit);
          }
        }
      }
    }
  }
}

template class MultiVantagePointTree<float>;
template class MultiVantagePointTree<double>;
template class MultiVantagePointTree<int>;

}  // namespace similarity

