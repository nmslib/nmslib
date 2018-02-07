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
#ifndef _SPATIAL_APPROXIMATION_TREE_H_
#define _SPATIAL_APPROXIMATION_TREE_H_

#include <string>

#include "index.h"
#include "params.h"

#define METH_SATREE                 "satree"

namespace similarity {

/* 
 * (Static) Spatial Approximation Tree
 * G. Navarro: Searching in metric spaces by spatial approximation
 * VLDB Journal 11: pp 28-46 (2002)
 */

template <typename dist_t>
class Space;

template <typename dist_t>
class SpatialApproxTree : public Index<dist_t> {
 public:
  SpatialApproxTree(const Space<dist_t>& space,
                    const ObjectVector& data);

  void CreateIndex(const AnyParams&) override;

  ~SpatialApproxTree();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  void SetQueryTimeParams(const AnyParams&) override {}

 private:
  struct SATKnn;
  class SATNode;

  const Space<dist_t>& space_;
  
  unique_ptr<SATNode> root_;
};

}    // namespace similarity

#endif     // _SPATIAL_APPROXIMATION_TREE_H_

