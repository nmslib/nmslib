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
#include "space.h"
#include "knnquery.h"
#include "rangequery.h"
#include "method/list_clusters.h"
#include "utils.h"
#include "logging.h"

#include <queue>
#include <utility>

namespace similarity {

using std::priority_queue;
using std::pair;
using std::make_pair;

template <typename dist_t>
using DistDistObjectTuple = std::tuple<dist_t, dist_t, const Object*>;

template <typename dist_t>
struct DistDistObjectTupleAscComparator {
  bool operator()(const DistDistObjectTuple<dist_t>& x,
                  const DistDistObjectTuple<dist_t>& y) const {
    return std::get<0>(x) < std::get<0>(y);
  }
};
    
template <typename dist_t>
void 
ListClusters<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);
  pmgr.GetParamOptional("maxLeavesToVisit", MaxLeavesToVisit_, FAKE_MAX_LEAVES_TO_VISIT);
  LOG(LIB_INFO) << "Set list of clusters query-time parameters:";
  LOG(LIB_INFO) << "maxLeavesToVisit=" << MaxLeavesToVisit_;
  pmgr.CheckUnused();
}


template <typename dist_t>
ListClusters<dist_t>::ListClusters(
    const Space<dist_t>& space,
    const ObjectVector& data) : Index<dist_t>(data), space_(space) { }

template <typename dist_t>
void ListClusters<dist_t>::CreateIndex(const AnyParams& IndexParams) 
{
  AnyParamManager pmgr(IndexParams);

  string sVal; 

  pmgr.GetParamOptional("strategy", sVal, "random");

  if (sVal == "random") {
    Strategy_ =  ListClustersStrategy::kRandom;
  } else if (sVal == "closestPrevCenter") {
    Strategy_ = ListClustersStrategy::kClosestPrevCenter;
  } else if (sVal == "farthestPrevCenter") {
    Strategy_ = ListClustersStrategy::kFarthestPrevCenter;
  } else if (sVal == "minSumDistPrevCenters") {
    Strategy_ = ListClustersStrategy::kMinSumDistPrevCenters;
  } else if (sVal == "maxSumDistPrevCenters") {
    Strategy_ = ListClustersStrategy::kMaxSumDistPrevCenters;
  } else {
    PREPARE_RUNTIME_ERR(err) << "Incorrect value :'" << sVal << "' for parameter strategy ";
    THROW_RUNTIME_ERR(err);
  }


  pmgr.GetParamOptional("useBucketSize", UseBucketSize_, true);
  pmgr.GetParamOptional("bucketSize",    BucketSize_,    50);
  pmgr.GetParamOptional("radius",        Radius_,        1);
  pmgr.GetParamOptional("chunkBucket",   ChunkBucket_,   true);

  pmgr.CheckUnused();

  this->ResetQueryTimeParams();

  // <distance to previous centers, object>
  DistObjectPairVector<dist_t> remaining;
  for (const auto& object : this->data_) {
    remaining.push_back(std::make_pair(0, object));
  }

  while (!remaining.empty()) {
    const Object* center = SelectNextCenter(remaining, Strategy_);
    Cluster* new_cluster = new Cluster(center);
    cluster_list_.push_back(new_cluster);

    if (remaining.size() == 1) {
      break;
    }

    DistObjectPairVector<dist_t> outside;
    if (UseBucketSize_) {    // use bucket size
      // <d(object, center), distance to previous centers, object>
      std::vector<DistDistObjectTuple<dist_t>> dp;
      bool center_skipped = false;
      for (const auto& p : remaining) {
        if (p.second == center) {
          // sanity check
          if (center_skipped) {
            PREPARE_RUNTIME_ERR(err) << "found skipped center again" << std::endl;
            THROW_RUNTIME_ERR(err);
          }
          center_skipped = true;
        } else {
          dp.push_back(std::make_tuple(
                  space_.IndexTimeDistance(p.second, center), p.first, p.second));
        }
      }
      std::sort(dp.begin(), dp.end(),
                DistDistObjectTupleAscComparator<dist_t>());
      for (const auto& p : dp) {
        if (new_cluster->GetBucket().size() < BucketSize_) {
          new_cluster->AddObject(std::get<2>(p), std::get<0>(p));
        } else {
          outside.push_back(std::make_pair(
                  std::get<0>(p) + std::get<1>(p), std::get<2>(p)));
        }
      }
    } else {   // use radius
      bool center_skipped = false;
      for (const auto& p : remaining) {
        if (p.second == center) {
          // sanity check
          if (center_skipped) {
            PREPARE_RUNTIME_ERR(err) << "found skipped center again" << std::endl;
            THROW_RUNTIME_ERR(err);
          }
          center_skipped = true;
        } else {
          const dist_t dist = space_.IndexTimeDistance(p.second, center);
          if (dist < Radius_) {
            new_cluster->AddObject(p.second, dist);
          } else {
            outside.push_back(std::make_pair(p.first + dist, p.second));
          }
        }
      }
    }

    remaining.swap(outside);
  }

  if (ChunkBucket_) {
    for (auto i: cluster_list_) {
      i->OptimizeBucket();
    }
  }
  LOG(LIB_INFO) << "Indexing is finished, created: " << cluster_list_.size() << " clusters";
}

template <typename dist_t>
ListClusters<dist_t>::~ListClusters() {
  for (auto& cluster : cluster_list_) {
    delete cluster;
  }
}

template <typename dist_t>
const std::string ListClusters<dist_t>::StrDesc() const {
  return "list of clusters";
}

template <typename dist_t>
void ListClusters<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  GenSearch(query);
}

template <typename dist_t>
void ListClusters<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  GenSearch(query);
}

template <typename dist_t>
template <typename QueryType>
void ListClusters<dist_t>::GenSearch(QueryType* query) const {
  if (MaxLeavesToVisit_ == FAKE_MAX_LEAVES_TO_VISIT) {
    for (const auto& cluster : cluster_list_) {
      const dist_t dist_qc = query->DistanceObjLeft(cluster->GetCenter());
      query->CheckAndAddToResult(dist_qc, cluster->GetCenter());

      if (dist_qc - query->Radius() < cluster->GetCoveringRadius()) {
        cluster->Search(query);
        if (dist_qc + query->Radius() < cluster->GetCoveringRadius()) {
        /* 
        * All the query points are inside the current cluster,
        * they have all been compared to the query already.
        */
          break;
        }
      }
    }
  } else {
     /*   
      *             NOTE that the below code is a naive early termination algorithm
      *             that WAS NOT proposed by Chavez and Navarro!!!! 
      *
      * TODO (@leo) Ideally, if the MaxLeavesToVisit_ is very large, 
      *             the code should work as an exact method. Yet, it does not
      *             due to specifics of the index construction algorithm.
      *             Think about possible improvements for the early termination strategy.
      */
    struct Elem {
      Cluster* cluster;
      dist_t   dist_qc;
      bool operator<(const Elem& o) const { return dist_qc > o.dist_qc; }
      Elem(Cluster* c, dist_t d) : cluster(c), dist_qc(d) {}
    };

    priority_queue<Elem> queue;

    for (const auto& cluster : cluster_list_) {
      const dist_t dist_qc = query->DistanceObjLeft(cluster->GetCenter());
      query->CheckAndAddToResult(dist_qc, cluster->GetCenter());

      if (dist_qc - query->Radius() < cluster->GetCoveringRadius()) {
        queue.push(Elem(cluster, dist_qc));
      }
    }

    dist_t  PrevDist = 0;

    int lproc = 0;

    while (!queue.empty() && lproc < MaxLeavesToVisit_) {
      Cluster* cluster = queue.top().cluster;
      const dist_t dist_qc = queue.top().dist_qc;

      CHECK(dist_qc >= PrevDist);
      PrevDist = dist_qc;

      cluster->Search(query);
      ++lproc;

      if (dist_qc + query->Radius() < cluster->GetCoveringRadius()) {
       return;
      }
      queue.pop();
    }
  }
}

template <typename dist_t>
const Object* ListClusters<dist_t>::SelectNextCenter(
    DistObjectPairVector<dist_t>& remaining,
    ListClustersStrategy strategy) {
  CHECK(!remaining.empty());
  size_t idx;
  switch (strategy) {
    case ListClustersStrategy::kRandom:
      return remaining[RandomInt() % remaining.size()].second;

    case ListClustersStrategy::kClosestPrevCenter:
      return remaining.front().second;

    case ListClustersStrategy::kFarthestPrevCenter:
      return remaining.back().second;

    case ListClustersStrategy::kMinSumDistPrevCenters:
      idx = RandomInt() % remaining.size();
      for (size_t i = 0; i < remaining.size(); ++i) {
        if (remaining[i].first < remaining[idx].first) {
          idx = i;
        }
      }
      return remaining[idx].second;

    case ListClustersStrategy::kMaxSumDistPrevCenters:
      idx = RandomInt() % remaining.size();
      for (size_t i = 0; i < remaining.size(); ++i) {
        if (remaining[i].first > remaining[idx].first) {
          idx = i;
        }
      }
      return remaining[idx].second;
  }

  throw runtime_error("Unknown CenterStrategy");
  return remaining[0].second;
}

template <typename dist_t>
ListClusters<dist_t>::Cluster::Cluster(const Object* center)
  : center_(center), covering_radius_(0), 
    CacheOptimizedBucket_(NULL), bucket_(new(ObjectVector)) {
}

template <typename dist_t>
ListClusters<dist_t>::Cluster::~Cluster() {
  ClearBucket(CacheOptimizedBucket_, bucket_);
}

template <typename dist_t>
void ListClusters<dist_t>::Cluster::OptimizeBucket() {
  ObjectVector* OldBucket = bucket_;

  CreateCacheOptimizedBucket(*OldBucket, CacheOptimizedBucket_, bucket_);
  
  delete OldBucket;
}

template <typename dist_t>
void ListClusters<dist_t>::Cluster::AddObject(
    const Object* object,
    const dist_t dist) {
  bucket_->push_back(object);
  if (covering_radius_ < dist) {
    covering_radius_ = dist;
  }
}

template <typename dist_t>
const Object* ListClusters<dist_t>::Cluster::GetCenter() {
  return center_;
}

template <typename dist_t>
const dist_t ListClusters<dist_t>::Cluster::GetCoveringRadius() {
  return covering_radius_;
}

template <typename dist_t>
const ObjectVector& ListClusters<dist_t>::Cluster::GetBucket() {
  return *bucket_;
}

template <typename dist_t>
template <typename QueryType>
void ListClusters<dist_t>::Cluster::Search(QueryType* query) const {
  for (const auto& object : (*bucket_)) {
    query->CheckAndAddToResult(object);
  }
}

template class ListClusters<double>;
template class ListClusters<float>;
template class ListClusters<int>;

}   // namespace similarity

