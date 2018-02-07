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

/*
 * This code is based on the BBTREE implementation written by Lawrence Cayton
 * Copyright (c) 2013-2018
 *
 * The algorithms were originally published in the papers:
 * 1) L. Cayton. Fast nearest neighbor retrieval for bregman divergences. 
 *    Twenty-Fifth International Conference on Machine Learning (ICML), 2008. 
 *
 * 2) L. Cayton. Efficient bregman range search.  
 * Advances in Neural Information Processing Systems 22 (NIPS), 2009. 
 *
 * See https://github.com/lcayton/bbtree and http://lcayton.com/code.html
 *
 * Because the original code is released under the terms of the GNU General Public License,
 * we had to release this file under the GNU license as well.
*/
#include <cmath>
#include <memory>

#include "space/space_bregman.h"
#include "knnquery.h"
#include "rangequery.h"
#include "method/bbtree.h"

namespace similarity {

using std::unique_ptr;

template <typename dist_t>
BBTree<dist_t>::BBTree(
    const Space<dist_t>& space, 
    const ObjectVector& data)  : Index<dist_t>(data) {
  BregmanDivSpace_ = BregmanDiv<dist_t>::ConvertFrom(&space); // Should be the special space!
}


template <typename dist_t>
void BBTree<dist_t>::CreateIndex(const AnyParams& MethParams) {
  AnyParamManager pmgr(MethParams);

  pmgr.GetParamOptional("bucketSize", BucketSize_, 50);
  pmgr.GetParamOptional("chunkBucket", ChunkBucket_, true);

  LOG(LIB_INFO) << "bucketSize  = " << BucketSize_;
  LOG(LIB_INFO) << "ChunkBucket = " << ChunkBucket_;


  pmgr.CheckUnused();

  root_node_.reset(new BBNode(BregmanDivSpace_, this->data_, BucketSize_, ChunkBucket_));
}

template <typename dist_t>
BBTree<dist_t>::~BBTree() {
}

template <typename dist_t>
const std::string BBTree<dist_t>::StrDesc() const {
  return "bbtree";
}


template <typename dist_t>
void BBTree<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  unique_ptr<Object> query_gradient(BregmanDivSpace_->GradientFunction(query->QueryObject()));

  /*
   * This is a basic version of the range search that is almost identical to NN search.
   * It is possible to do better though. See for details:
   * L. Cayton. Efficient bregman range search. 
   * Advances in Neural Information Processing Systems 22 (NIPS), 2009. 
   */

  int mx = MaxLeavesToVisit_;
  root_node_->LeftSearch(BregmanDivSpace_, query_gradient.get(), query, mx);
}

template <typename dist_t>
void BBTree<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  unique_ptr<Object> query_gradient(BregmanDivSpace_->GradientFunction(query->QueryObject()));

  int mx = MaxLeavesToVisit_;
  root_node_->LeftSearch(BregmanDivSpace_, query_gradient.get(), query, mx);
}

template <typename dist_t>
BBTree<dist_t>::BBNode::BBNode(
    const BregmanDiv<dist_t>* div, const ObjectVector& data, size_t bucket_size, bool use_optim)
    : center_(div->Mean(data)),
      center_gradf_(div->GradientFunction(center_)),
      covering_radius_(0.0),
      is_leaf_(false),
      bucket_(NULL),
      CacheOptimizedBucket_(NULL),
      left_child_(NULL),
      right_child_(NULL) {
  dist_t dist;
  for (size_t i = 0; i < data.size(); ++i) {
    dist = div->IndexTimeDistance(data[i], center_);
    if (dist > covering_radius_) {
      covering_radius_ = dist;
    }
  }

  if (data.size() <= bucket_size) {
    is_leaf_ = true;
    if (use_optim) {
      CreateCacheOptimizedBucket(data, CacheOptimizedBucket_, bucket_);
    } else {
      bucket_ = new ObjectVector(data);
    }
  } else {
    ObjectVector bucket_left;
    ObjectVector bucket_right;
    int retry = 0;
    while (retry < kMaxRetry && (bucket_left.empty() || bucket_right.empty())) {
      FindSplitKMeans(div, data, bucket_left, bucket_right);
      retry++;
    }
    if (retry < kMaxRetry) {
      if (!bucket_left.empty()) {
        left_child_ = new BBNode(div, bucket_left, bucket_size, use_optim);
      }
      if (!bucket_right.empty()) {
        right_child_ = new BBNode(div, bucket_right, bucket_size, use_optim);
      }
    } else {
      is_leaf_ = true;
      if (use_optim) {
        CreateCacheOptimizedBucket(data, CacheOptimizedBucket_, bucket_);
      } else {
        bucket_ = new ObjectVector(data);
      }
    }
  }
}

template <typename dist_t>
BBTree<dist_t>::BBNode::~BBNode() {
  delete left_child_;
  delete right_child_;
  delete center_;
  delete center_gradf_;
  ClearBucket(CacheOptimizedBucket_, bucket_);
}

template <typename dist_t>
bool BBTree<dist_t>::BBNode::IsLeaf() const {
  return is_leaf_;
}

template <typename dist_t>
void BBTree<dist_t>::BBNode::SelectCenters(
    const ObjectVector& data, ObjectVector& centers) {
  std::vector<int> center_idx(centers.size());
  for (size_t j, i = 0; i < centers.size(); ) {
    while (true) {
      int r = RandomInt() % data.size();
      for (j = 0; j < i; ++j) {
        if (center_idx[j] == r) break;
      }
      if (j == i) {
        center_idx[i++] = r;
        break;
      }
    }
  }

  for (size_t i = 0; i < centers.size(); ++i) {
    centers[i] = data[center_idx[i]]->Clone();
  }
}

template <typename dist_t>
void BBTree<dist_t>::BBNode::FindSplitKMeans(
    const BregmanDiv<dist_t>* div, const ObjectVector& data,
    ObjectVector& bucket_left, ObjectVector& bucket_right) {
  ObjectVector centers(2);
  SelectCenters(data, centers);

  for (int retry = 0; retry < kMaxRetry; ++retry) {
    bucket_left.clear();
    bucket_right.clear();

    for (size_t i = 0; i < data.size(); ++i) {
      const dist_t div_left = div->IndexTimeDistance(data[i], centers[0]);
      const dist_t div_right = div->IndexTimeDistance(data[i], centers[1]);
      if (div_left < div_right) {
        bucket_left.push_back(data[i]);
      } else {
        bucket_right.push_back(data[i]);
      }
    }

    for (size_t i = 0; i < centers.size(); ++i) {
      delete centers[i];
    }

    if (bucket_left.empty() || bucket_right.empty()) {
      SelectCenters(data, centers);
    } else {
      centers[0] = div->Mean(bucket_left);
      centers[1] = div->Mean(bucket_right);
    }
  }

  for (size_t i = 0; i < centers.size(); ++i) {
    delete centers[i];
  }
}

template <typename dist_t>
template <typename QueryType>
void BBTree<dist_t>::BBNode::LeftSearch(const BregmanDiv<dist_t>* div, 
                                        Object* query_gradient, 
                                        QueryType* query,
                                        int& MaxLeavesToVisit) const {
  if (MaxLeavesToVisit <= 0) return; // early termination
  if (IsLeaf()) {
    --MaxLeavesToVisit;
    dist_t dist;
    for (size_t j = 0; j < bucket_->size(); ++j) {
      dist = query->DistanceObjLeft((*bucket_)[j]);
      query->CheckAndAddToResult(dist, (*bucket_)[j]);
    }
  } else {
    const dist_t div_left = query->DistanceObjRight(left_child_->center_);
    const dist_t div_right = query->DistanceObjRight(right_child_->center_);

    if (div_left < div_right) {
      left_child_->LeftSearch(div, query_gradient, query, MaxLeavesToVisit);
      if (right_child_->NeedToSearch(div, query_gradient, query, query->Radius(), div_right)) {
        right_child_->LeftSearch(div, query_gradient, query, MaxLeavesToVisit);
      }
    } else {
      right_child_->LeftSearch(div, query_gradient, query, MaxLeavesToVisit);
      if (left_child_->NeedToSearch(div, query_gradient, query, query->Radius(), div_left)) {
        left_child_->LeftSearch(div, query_gradient, query, MaxLeavesToVisit);
      }
    }
  }
}

template <typename dist_t>
template <typename QueryType>
bool BBTree<dist_t>::BBNode::NeedToSearch(
    const BregmanDiv<dist_t>* div,
    Object* query_gradient, 
    QueryType* query,
    dist_t mindist_est,
    dist_t div_query_to_center) const {
  if (div_query_to_center < covering_radius_ ||
      div_query_to_center < mindist_est) {
    return true;
  }
  return RecBinSearch(div, query_gradient, query, mindist_est);
}

template <typename dist_t>
template <typename QueryType>
bool BBTree<dist_t>::BBNode::RecBinSearch(
    const BregmanDiv<dist_t>* div,
    Object* query_gradient, 
    QueryType* query, dist_t mindist_est,
    dist_t l, dist_t r, int depth) const {
  // sanity checks
  if (depth >= 1e6) {    // something is wrong!
    PREPARE_RUNTIME_ERR(err) << "Recursive depth exceeds " << depth << std::endl;
    THROW_RUNTIME_ERR(err);
  }
  CHECK(query->QueryObject()->datalength() == center_gradf_->datalength());

  const dist_t *qp = reinterpret_cast<const dist_t*>(query_gradient->data());
  const dist_t *cp = reinterpret_cast<const dist_t*>(center_gradf_->data());

  Object* tmp = Object::CreateNewEmptyObject(query->QueryObject()->datalength());
  dist_t *tmp_value = reinterpret_cast<dist_t*>(const_cast<char*>(tmp->data()));

  const size_t length = div->GetElemQty(tmp);

  const dist_t theta = (l + r) / 2.0;
  for (size_t i = 0; i < length; ++i) {
    tmp_value[i] = theta * qp[i] + (1.0 - theta) * cp[i];
  }

  Object *x = div->InverseGradientFunction(tmp);
  delete tmp;

  dist_t div_to_center = query->Distance(x, center_);     // d(x, center)
  dist_t div_to_query = query->DistanceObjLeft(x);        // d(x, query)
  delete x;

  dist_t lower_bound = div_to_query +
                       (1.0/theta - 1.0) * (div_to_center - covering_radius_);

  if (lower_bound >= mindist_est) {
    return false;
  }

  static dist_t kCloseEnough = 1e-3;

  // In C++ 11, std::abs is also defined for floating-point numbers
  if (std::abs(div_to_center - covering_radius_) < covering_radius_ * kCloseEnough) {
    return true;
  }

  if (div_to_center > covering_radius_) {
    return RecBinSearch(div, query_gradient, query, mindist_est, l, theta, depth+1);
  } else {
    if (div_to_query < mindist_est) {
      return true;
    }
    return RecBinSearch(div, query_gradient, query, mindist_est, theta, r, depth+1);
  }
}

template class BBTree<double>;
template class BBTree<float>;

}  // namespace similarity

