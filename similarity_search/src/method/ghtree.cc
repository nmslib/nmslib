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
#include <limits>

#include "space.h"
#include "knnquery.h"
#include "rangequery.h"
#include "method/ghtree.h"
#include "utils.h"

namespace similarity {

template <typename dist_t>
GHTree<dist_t>::GHTree(const Space<dist_t>& space,
                       const ObjectVector& data,
                       bool use_random_center) : 
                                Index<dist_t>(data),
                                space_(space),
                                use_random_center_(use_random_center) {
}

template <typename dist_t>
void GHTree<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("bucketSize", BucketSize_,    50);
  pmgr.GetParamOptional("chunkBucket", ChunkBucket_,  true);

  LOG(LIB_INFO) << "bucketSize   = " << BucketSize_;
  LOG(LIB_INFO) << "chunkBucket  = " << ChunkBucket_;

  pmgr.CheckUnused();
  this->ResetQueryTimeParams();

  root_.reset(new GHNode(space_, this->data_,
                     BucketSize_, ChunkBucket_,
                     use_random_center_ /* random center */));
}

template <typename dist_t>
GHTree<dist_t>::~GHTree() { }

template <typename dist_t>
const std::string GHTree<dist_t>::StrDesc() const {
  return "ghtree";
}

template <typename dist_t>
void GHTree<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  int mx = MaxLeavesToVisit_;
  root_->GenericSearch(query, mx);
}

template <typename dist_t>
void GHTree<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  int mx = MaxLeavesToVisit_;
  root_->GenericSearch(query, mx);
}

template <typename dist_t>
GHTree<dist_t>::GHNode::GHNode(
    const Space<dist_t>& space, const ObjectVector& data,
    size_t bucket_size, bool chunk_bucket,
    const bool use_random_center)
  : pivot1_(NULL), pivot2_(NULL), left_child_(NULL), right_child_(NULL),
    bucket_(NULL), CacheOptimizedBucket_(NULL) {
  CHECK(!data.empty());

  if (data.size() <= bucket_size) {
    if (chunk_bucket) {
      CreateCacheOptimizedBucket(data, CacheOptimizedBucket_, bucket_);
    } else {
      bucket_ = new ObjectVector(data);
    }
    return;
  }

  int data_size = static_cast<int>(data.size());
  int pivot1_id = use_random_center ? RandomInt() % data_size : data_size - 1;
  int pivot2_id = -1;
  pivot1_ = data[pivot1_id];

  if (data.size() >= 2) {    // check if there exists at least 1 except pivot1
    dist_t maxd = 0;
    int id;
    for (int t = 0; t < 100; ++t) {
        id = RandomInt() % data_size;
        if (id != pivot1_id) {
            dist_t curd = space.IndexTimeDistance(pivot1_, data[id]);
            if (curd >= maxd) {
              maxd = curd;
              pivot2_id = id;
            }
        }
    }
    CHECK(pivot2_id >= 0);
    CHECK(pivot2_id != pivot1_id);
    pivot2_ = data[pivot2_id];
  }

  if (data.size() >= 3) {   // at least 1 object except pivot1 & 2
    ObjectVector left_subset, right_subset;
    dist_t dist_to_pivot1, dist_to_pivot2;
    for (int i = 0; i < data_size; ++i) {
      if (i == pivot1_id || i == pivot2_id)
        continue;
      dist_to_pivot1 = space.IndexTimeDistance(pivot1_, data[i]);
      dist_to_pivot2 = space.IndexTimeDistance(pivot2_, data[i]);
      if (dist_to_pivot1 < dist_to_pivot2) {    // close to pivot1
        left_subset.push_back(data[i]);
      } else {
        right_subset.push_back(data[i]);
      }
    }

    if (!left_subset.empty()) {
      left_child_ = new GHNode(space, left_subset, bucket_size, chunk_bucket, use_random_center);
    }

    if (!right_subset.empty()) {
      right_child_ = new GHNode(space, right_subset, bucket_size, chunk_bucket, use_random_center);
    }
  }
}

template <typename dist_t>
GHTree<dist_t>::GHNode::~GHNode() {
  delete left_child_;
  delete right_child_;
  ClearBucket(CacheOptimizedBucket_, bucket_);
}

template <typename dist_t>
template <typename QueryType>
void GHTree<dist_t>::GHNode::GenericSearch(QueryType* query, int& MaxLeavesToVisit) {
  if (MaxLeavesToVisit <= 0) return; // early termination
  if (bucket_) {
    --MaxLeavesToVisit;

    for (unsigned i = 0; i < bucket_->size(); ++i) {
      const Object* Obj = (*bucket_)[i];
      dist_t distQC = query->DistanceObjLeft(Obj);
      query->CheckAndAddToResult(distQC, Obj);
    }
    return;
  }

  dist_t dist_to_pivot1 = DistMax<dist_t>();
  dist_t dist_to_pivot2 = DistMax<dist_t>();
  bool exists_pivot1 = false;
  bool exists_pivot2 = false;
  if (pivot1_ != NULL) {
    exists_pivot1 = true;
    // Distance function is a symmetric metric, so it can be DistanceObjRight as well
    dist_to_pivot1 = query->DistanceObjLeft(pivot1_);
    query->CheckAndAddToResult(dist_to_pivot1, pivot1_);
  }
  if (pivot2_ != NULL) {
    exists_pivot2 = true;
    // Distance function is a symmetric metric, so it can be DistanceObjRight as well
    dist_to_pivot2 = query->DistanceObjLeft(pivot2_);
    query->CheckAndAddToResult(dist_to_pivot2, pivot2_);
  }
  if (exists_pivot1 && exists_pivot2) {
    if (dist_to_pivot1 < dist_to_pivot2) {
      // check left first
      if (left_child_ != NULL &&
          (dist_to_pivot1 - dist_to_pivot2) / 2 <= query->Radius()) {
        left_child_->GenericSearch(query, MaxLeavesToVisit);
      }
      // then right
      if (right_child_ != NULL &&
          (dist_to_pivot2 - dist_to_pivot1) / 2 <= query->Radius()) {
        right_child_->GenericSearch(query, MaxLeavesToVisit);
      }
    } else {
      // check right first
      if (right_child_ != NULL &&
          (dist_to_pivot2 - dist_to_pivot1) / 2 <= query->Radius()) {
        right_child_->GenericSearch(query, MaxLeavesToVisit);
      }
      // then left
      if (left_child_ != NULL &&
          (dist_to_pivot1 - dist_to_pivot2) / 2 <= query->Radius()) {
        left_child_->GenericSearch(query, MaxLeavesToVisit);
      }
    }
  } else {
    if (left_child_ != NULL) left_child_->GenericSearch(query, MaxLeavesToVisit);
    if (right_child_ != NULL) right_child_->GenericSearch(query, MaxLeavesToVisit);
  }
}

template class GHTree<double>;
template class GHTree<float>;
template class GHTree<int>;

}   // namespace similarity

