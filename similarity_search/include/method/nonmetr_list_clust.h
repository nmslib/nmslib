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
#ifndef NONMETR_LIST_CLUST_H
#define NONMETR_LIST_CLUST_H

#include <string>
#include <sstream>
#include <memory>

#include "index.h"
#include "object.h"

#define METH_NON_METR_LISTCLUST        "nonmetr_list_clust"

namespace similarity {

using std::string;
using std::shared_ptr;

/*
 * A non-metric indexing based on one-level clustering. There are papers for 
 * metric and non-metric indexes exploited this idea, e.g.:
 * 
 * 1) DynDex: a dynamic and non-metric space indexer, KS Goh, B Li, E Chang, 2002.
 * 2) C. Li, E. Chang, H. Garcia-Molina, and G. Wiederhold. 
 *    Clustering for approximate similarity search in high-dimensional
 * 3) E. Chavez and G. Navarro.  
 *    A compact space decomposition for effective a metric indexing. 2005
 */

template <typename dist_t>
class NonMetrListClust : public Index<dist_t> {
 public:
  NonMetrListClust(bool printProgress,
                   Space<dist_t>& space,
                   const ObjectVector& data) : Index<dist_t>(data), printProgress_(printProgress), space_(space) {
    maxObjId_ = 0;
    for (const Object* o: data) {
      maxObjId_ = max(maxObjId_, o->id());
      CHECK_MSG(o->id() >= 0, "Bug: detected negative object id");
    }
  }

  void CreateIndex(const AnyParams& IndexParams) override;
  // SaveIndex is not necessarily implemented
  virtual void SaveIndex(const string& location) override {
    throw runtime_error("SaveIndex is not implemented for method: " + StrDesc());
  }
  // LoadIndex is not necessarily implemented
  virtual void LoadIndex(const string& location) override {
    throw runtime_error("LoadIndex is not implemented for method: " + StrDesc());
  }

  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;

  ~NonMetrListClust(){};

  const std::string StrDesc() const override { 
    return string("list of clusters for non-metric indexing");
  }

  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  virtual bool DuplicateData() const override { return false; }

 private:
  bool                    printProgress_;
  Space<dist_t>&          space_;

  size_t                  db_scan_;
  
  ObjectVector                                        vCenters_;      // Cluster centers
  vector<shared_ptr<DistObjectPairVector<dist_t>>>    vClusterAssign_; // Cluster assignment
  ObjectVector                                        vUnassigned_;
  IdType                                              maxObjId_;

  template <typename QueryType>
  void GenericSearch(QueryType* query, IdType) const;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(NonMetrListClust);
};

}   // namespace similarity

#endif
