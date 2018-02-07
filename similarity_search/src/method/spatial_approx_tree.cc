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
#include <tuple>
#include <queue>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "method/spatial_approx_tree.h"

using namespace std;

namespace similarity {

template <typename dist_t>
struct SpatialApproxTree<dist_t>::SATKnn {
  dist_t lbound;     // lower bound
  dist_t mind;
  dist_t dist_qp;
  typename SpatialApproxTree<dist_t>::SATNode* node;

  SATKnn() : lbound(0), mind(0), dist_qp(0), node(NULL) {}
  SATKnn(dist_t lbound, dist_t mind, dist_t dist_qp, SATNode* node)
      : lbound(lbound), mind(mind), dist_qp(dist_qp), node(node) {}

  bool operator<(const SATKnn& other) const {
    return lbound > other.lbound;
  }
};

template <typename dist_t>
class SpatialApproxTree<dist_t>::SATNode {
 public:
  SATNode(const Space<dist_t>* space,
          const Object* pivot,
          const DistObjectPairVector<dist_t>& dp);

  ~SATNode();

  void Search(RangeQuery<dist_t>* query,
              dist_t dist_qp,
              dist_t mind) const;

 private:
  const Object*                           pivot_;
  dist_t                                  covering_radius_;
  vector<pair<const Object*, SATNode*>>   neighbors_;

  friend class SpatialApproxTree<dist_t>;
};

template <typename dist_t>
SpatialApproxTree<dist_t>::SATNode::SATNode(
    const Space<dist_t>*          space,
    const Object*                 pivot,
    const DistObjectPairVector<dist_t>& dp)
  : pivot_(pivot), covering_radius_(0) {
  if (dp.empty()) {    // leaf node
    return;
  }

  // dp is already sorted in ascending order
  covering_radius_ = dp.back().first;
  vector<tuple<const Object*, size_t, dist_t>> non_neighbors;

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
      non_neighbors.push_back(make_tuple(v, min_idx, min_dist));
    } else {
      neighbors_.push_back(make_pair(v, nullptr));
    }
  }

  vector<DistObjectPairVector<dist_t>> buckets(neighbors_.size());

  for (size_t i = 0; i < non_neighbors.size(); ++i) {
    const Object* v = get<0>(non_neighbors[i]);
    size_t min_idx  = get<1>(non_neighbors[i]);
    dist_t min_dist = get<2>(non_neighbors[i]);
    for (size_t j = min_idx + 1; j < neighbors_.size(); ++j) {
      dist_t d = space->IndexTimeDistance(v, neighbors_[j].first);
      if (min_dist > d) {
        min_dist = d;
        min_idx = j;
      }
    }
    buckets[min_idx].push_back(make_pair(min_dist, v));
  }

  for (size_t i = 0; i < neighbors_.size(); ++i) {
    sort(buckets[i].begin(), buckets[i].end(),
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
void SpatialApproxTree<dist_t>::SATNode::Search (
    RangeQuery<dist_t>* query,
    dist_t dist_qp,
    dist_t mind) const {
  if (dist_qp <= covering_radius_ + query->Radius()) {
    query->CheckAndAddToResult(dist_qp, pivot_);

    vector<dist_t> d(neighbors_.size());
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
    const Space<dist_t>& space,
    const ObjectVector& data) : Index<dist_t>(data), space_(space) {
}

template <typename dist_t>
void SpatialApproxTree<dist_t>::CreateIndex(const AnyParams& ) {
  size_t index = RandomInt() % this->data_.size();
  const Object* pivot = this->data_[index];

  DistObjectPairVector<dist_t> dp;
  for (size_t i = 0; i < this->data_.size(); ++i) {
    if (i != index) {
      dp.push_back(
          make_pair(space_.IndexTimeDistance(this->data_[i], pivot), this->data_[i]));
    }
  }

  sort(dp.begin(), dp.end(), DistObjectPairAscComparator<dist_t>());
  root_.reset(new SATNode(&space_, pivot, dp));
}

template <typename dist_t>
SpatialApproxTree<dist_t>::~SpatialApproxTree() {
}

template <typename dist_t>
const string SpatialApproxTree<dist_t>::StrDesc() const {
  return "satree";
}

template <typename dist_t>
void SpatialApproxTree<dist_t>::Search(KNNQuery<dist_t>* query, IdType const)  const {
  static dist_t kZERO = static_cast<dist_t>(0);
  priority_queue<SATKnn> heap;
  dist_t dist_qp = query->DistanceObjLeft(root_->pivot_);
  heap.push(SATKnn((max(kZERO, dist_qp - root_->covering_radius_)),
                   dist_qp, dist_qp, root_.get()));

  while (!heap.empty()) {
    SATKnn top = heap.top();
    dist_t lbound = (top.lbound);
    dist_t mind = top.mind;
    dist_t dist_qp = top.dist_qp;

    SATNode* node = top.node;

    heap.pop();
//cout << " bnd " << lbound << " -> " << query->Radius() << endl;
    if (lbound > query->Radius()) break;

    query->CheckAndAddToResult(dist_qp, node->pivot_);


    vector<dist_t> d(node->neighbors_.size());
    for (size_t i = 0; i < node->neighbors_.size(); ++i) {
      d[i] = query->DistanceObjLeft(node->neighbors_[i].first);
      mind = min(mind, d[i]);
    }

    for (size_t i = 0; i < node->neighbors_.size(); ++i) {
      /* 
       * In the original VLDB journal paper Fig. 7
       * The new lbound is computed as: max(lbound, mind/2, d(q,v)-R(v))
       *
       * This is most likely to be an error, because it contradicts to what
       * is written in section 4.3, in the discussion on lower bounds.
       *
       * The correct version, which seems to be the one in the Metric Spaces Library,
       * replaces the second MAX argument with:
       * (d(q,v)-mind)/2
       */
      dist_t new_lbound = std::max(std::max(lbound, (d[i] - mind)/2),
                                   (d[i] - node->neighbors_[i].second->covering_radius_));

      if (new_lbound < query->Radius()) {
        heap.push(SATKnn(new_lbound, mind, d[i], node->neighbors_[i].second));
      }
    }
  }
}

template <typename dist_t>
void SpatialApproxTree<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  dist_t dist_qp = query->DistanceObjLeft(root_->pivot_);
  root_->Search(query, dist_qp, dist_qp);
}

template class SpatialApproxTree<double>;
template class SpatialApproxTree<float>;
template class SpatialApproxTree<int>;

}    // namespace similarity

