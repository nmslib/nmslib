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
#ifndef _LIST_OF_CLUSTERS_H_
#define _LIST_OF_CLUSTERS_H_

#include "index.h"
#include "lcstrategy.h"
#include "params.h"

#define METH_LIST_CLUSTERS          "list_clusters"

namespace similarity {

/* 
 * E. Chavez and G. Navarro.
 * A compact space decomposition for effective a metric indexing. 
 * Pattern Recognition Letters, 26(9):1363-1376, 2005
 *
 * The method also resembles canopy clustering: https://en.wikipedia.org/wiki/Canopy_clustering_algorithm
 *
 * Note that similar ideas were also proposed earlier:
 *
 * 1) DynDex: a dynamic and non-metric space indexer, KS Goh, B Li, E Chang, 2002.
 * 2) C. Li, E. Chang, H. Garcia-Molina, and G. Wiederhold. 
 *    Clustering for approximate similarity search in high-dimensional
 * 3) E. Chavez and G. Navarro.  
 *    A compact space decomposition for effective a metric indexing. 2005
 */

template <typename dist_t>
class Space;

template <typename dist_t>
class ListClusters : public Index<dist_t> {
 public:
  ListClusters(const Space<dist_t>& space,
               const ObjectVector& data);

  void CreateIndex(const AnyParams& MethParams) override;
  ~ListClusters();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  static const Object* SelectNextCenter(
      DistObjectPairVector<dist_t>& remaining,
      ListClustersStrategy strategy);

  virtual bool DuplicateData() const override { return ChunkBucket_; }
  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;
 private:

  template <typename QueryType>
  void GenSearch(QueryType* query) const;

  class Cluster {
   public:
    Cluster(const Object* center);
    ~Cluster();

    void OptimizeBucket();
    void AddObject(const Object* object,
                   const dist_t dist);

    const Object* GetCenter();
    const dist_t GetCoveringRadius();
    const ObjectVector& GetBucket();

    template <typename QueryType>
    void Search(QueryType* query) const;

   private:
    const Object* center_;
    dist_t covering_radius_;
    char* CacheOptimizedBucket_;
    ObjectVector* bucket_;
    int MaxLeavesToVisit_;
  };

  const Space<dist_t>&  space_;

  std::vector<Cluster*> cluster_list_;

  ListClustersStrategy Strategy_; 
  bool                 UseBucketSize_;
  size_t               BucketSize_;
  dist_t               Radius_;
  int                  MaxLeavesToVisit_;
  bool                 ChunkBucket_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(ListClusters);
};

}   // namespace similarity

#endif     // _METRIC_GHTREE_H_

