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
#include <tuple>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "spatial_approx_tree.h"

namespace similarity {

template <typename dist_t>
struct SpatialApproxTree<dist_t>::SATKnn {
  dist_t lbound;     // lower bound
  dist_t mind;
  dist_t dist_qp;
  SpatialApproxTree<dist_t>::SATNode* node;

  SATKnn() : lbound(0), mind(0), dist_qp(0), node(NULL) {}
  SATKnn(dist_t lbound, dist_t mind, dist_t dist_qp, SATNode* node)
      : lbound(lbound), mind(mind), dist_qp(dist_qp), node(node) {}

  bool operator<(const SATKnn& other) const {
    return lbound < other.lbound;
  }
};

template <typename dist_t>
class SpatialApproxTree<dist_t>::SATNode {
 public:
  SATNode(const Space<dist_t>* space,
          const Object* pivot,
          DistObjectPairVector<dist_t>& dp);

  ~SATNode();

  void Search(RangeQuery<dist_t>* query,
              dist_t dist_qp,
              dist_t mind);

 private:
  const Object* pivot_;
  dist_t covering_radius_;
  std::vector<std::pair<const Object*, SATNode*>> neighbors_;
  friend class SpatialApproxTree<dist_t>;
};

template <typename dist_t>
SpatialApproxTree<dist_t>::SATNode::SATNode(
    const Space<dist_t>* space,
    const Object* pivot,
    DistObjectPairVector<dist_t>& dp)
  : pivot_(pivot), covering_radius_(0) {
  if (dp.empty()) {    // leaf node
    return;
  }

  // dp is already sorted in ascending order
  covering_radius_ = dp.back().first;
  std::vector<std::tuple<const Object*, size_t, dist_t>> non_neighbors;

  for (size_t i = 0; i < dp.size(); ++i) {
    dist_t dist_p = dp[i].first;
    const Object* v = dp[i].second;

    dist_t min_dist = dist_p;
    size_t min_idx = 0;
    bool found = false;
    for (size_t j = 0; j < neighbors_.size(); ++j) {
      dist_t d = space->IndexTimeDistance(v, neighbors_[j].first);
      if (min_dist > d) {
        min_dist = d;
        min_idx = j;
        found = true;
      }
    }

    if (found) {
      non_neighbors.push_back(
          std::make_tuple(v, min_idx, min_dist));
    } else {
      neighbors_.push_back(std::make_pair(v, nullptr));
    }
  }

  std::vector<DistObjectPairVector<dist_t>> buckets(neighbors_.size());

  for (size_t i = 0; i < non_neighbors.size(); ++i) {
    const Object* v = std::get<0>(non_neighbors[i]);
    size_t min_idx  = std::get<1>(non_neighbors[i]);
    dist_t min_dist = std::get<2>(non_neighbors[i]);
    for (size_t j = min_idx + 1; j < neighbors_.size(); ++j) {
      dist_t d = space->IndexTimeDistance(v, neighbors_[j].first);
      if (min_dist > d) {
        min_dist = d;
        min_idx = j;
      }
    }
    buckets[min_idx].push_back(std::make_pair(min_dist, v));
  }

  for (size_t i = 0; i < neighbors_.size(); ++i) {
    std::sort(buckets[i].begin(), buckets[i].end(),
              DistObjectPairAscComparator<dist_t>());
    neighbors_[i].second =
        new SATNode(space, neighbors_[i].first, buckets[i]);
  }
}

template <typename dist_t>
SpatialApproxTree<dist_t>::SATNode::~SATNode() {
  for (auto& n : neighbors_) {
    delete n.second;
  }
}

template <typename dist_t>
void SpatialApproxTree<dist_t>::SATNode::Search(
    RangeQuery<dist_t>* query,
    dist_t dist_qp,
    dist_t mind) {
  if (dist_qp <= covering_radius_ + query->Radius()) {
    query->CheckAndAddToResult(dist_qp, pivot_);

    std::vector<dist_t> d(neighbors_.size());
    for (size_t i = 0; i < neighbors_.size(); ++i) {
      d[i] = query->DistanceObjLeft(neighbors_[i].first);
      if (mind > d[i]) {
        mind = d[i];
      }
    }

    for (size_t i = 0; i < neighbors_.size(); ++i) {
      if ((d[i] - mind) / 2 <= query->Radius()) {
        neighbors_[i].second->Search(query, d[i], mind);
      }
    }
  }
}

template <typename dist_t>
SpatialApproxTree<dist_t>::SpatialApproxTree(
    const Space<dist_t>* space,
    const ObjectVector& data) {
  size_t index = 0;     // RandomInt() % data.size();
  const Object* pivot = data[index];

  DistObjectPairVector<dist_t> dp;
  for (size_t i = 0; i < data.size(); ++i) {
    if (i != index) {
      dp.push_back(
          std::make_pair(
              space->IndexTimeDistance(data[i], pivot),
              data[i]));
    }
  }

  std::sort(dp.begin(), dp.end(),
            DistObjectPairAscComparator<dist_t>());
  root_ = new SATNode(space, pivot, dp);
}

template <typename dist_t>
SpatialApproxTree<dist_t>::~SpatialApproxTree() {
  delete root_;
}

template <typename dist_t>
const std::string SpatialApproxTree<dist_t>::ToString() const {
  return "satree";
}

template <typename dist_t>
void SpatialApproxTree<dist_t>::Search(KNNQuery<dist_t>* query) {
  static dist_t kZERO = static_cast<dist_t>(0);
  std::vector<SATKnn> heap;
  dist_t dist_qp = query->DistanceObjLeft(root_->pivot_);
  heap.push_back(
      SATKnn(-(std::max(kZERO, dist_qp - root_->covering_radius_)),
             dist_qp, dist_qp, root_));

  while (!heap.empty()) {
    SATKnn top = *(heap.begin());
    dist_t lbound = -(top.lbound);
    dist_t mind = top.mind;
    dist_t dist_qp = top.dist_qp;
    SATNode* node = top.node;
    pop_heap(heap.begin(), heap.end());
    heap.pop_back();

    if (lbound > query->Radius()) break;

    query->CheckAndAddToResult(dist_qp, node->pivot_);

    std::vector<dist_t> d(node->neighbors_.size());
    for (size_t i = 0; i < node->neighbors_.size(); ++i) {
      d[i] = query->DistanceObjLeft(node->neighbors_[i].first);
      if (mind > d[i]) {
        mind = d[i];
      }
    }

    for (size_t i = 0; i < node->neighbors_.size(); ++i) {
      // TODO(double check): In the original VLDB journal paper
      // The new lbound is computed as:
      //     max(lbound, mind/2, d(q,v)-R(v))
      //
      // The second param is (mind/2) but it's in the SISAP library (d(q,v)-mind)/2

      dist_t new_lbound = std::max(
          std::max(lbound, (d[i] - mind)/2),
          (d[i] - node->neighbors_[i].second->covering_radius_));

      if (new_lbound < query->Radius()) {
        heap.push_back(
            SATKnn(-(new_lbound), mind, d[i], node->neighbors_[i].second));
        push_heap(heap.begin(), heap.end());
      }
    }
  }
}

template <typename dist_t>
void SpatialApproxTree<dist_t>::Search(RangeQuery<dist_t>* query) {
  dist_t dist_qp = query->DistanceObjLeft(root_->pivot_);
  root_->Search(query, dist_qp, dist_qp);
}

template class SpatialApproxTree<double>;
template class SpatialApproxTree<float>;
template class SpatialApproxTree<int>;

}    // namespace similarity

