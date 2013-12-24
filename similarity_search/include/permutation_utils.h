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

#ifndef _PERMUTATION_UTILS_H_
#define _PERMUTATION_UTILS_H_

#include <algorithm>
#include <iostream>
#include <unordered_set>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "permutation_type.h"
#include "distcomp.h"

namespace similarity {

using std::max;

template <typename dist_t>
using DistInt = std::pair<dist_t, PivotIdType>;     // <dist,      pivot_id>

typedef std::pair<PivotIdType, size_t> IntInt;      // <perm-dist, object_id>

template <typename dist_t>
void GetPermutationPivot(const ObjectVector& data,
                         const Space<dist_t>* space,
                         const size_t num_pivot,
                         ObjectVector* pivot) {
  CHECK(num_pivot < data.size());
  std::unordered_set<int> pivot_idx;
  for (size_t i = 0; i < num_pivot; ++i) {
    int p = RandomInt() % data.size();
    while (pivot_idx.count(p) != 0) {
      p = RandomInt() % data.size();
    }
    pivot_idx.insert(p);
    pivot->push_back(data[p]);
  }
}

template <typename dist_t>
void GetPermutation(const ObjectVector& pivot, const Space<dist_t>* space,
                    const Object* object, Permutation* p) {
  // get pivot id
  std::vector<DistInt<dist_t>> dists;
  for (size_t i = 0; i < pivot.size(); ++i) {
    dists.push_back(std::make_pair(space->IndexTimeDistance(pivot[i], object), static_cast<PivotIdType>(i)));
  }
  std::sort(dists.begin(), dists.end());
  // dists.second = pivot id    i.e.  \Pi_o(i)

  // get pivot id's pos         i.e. position in \Pi_o(i) = \Pi^{-1}(i)
  std::vector<IntInt> pivot_idx;
  for (size_t i = 0; i < pivot.size(); ++i) {
    pivot_idx.push_back(std::make_pair(dists[i].second, static_cast<PivotIdType>(i)));
  }
  std::sort(pivot_idx.begin(), pivot_idx.end());
  // pivot_idx.second = pos of pivot  (which is needed for computing the Rho func)
  for (size_t i = 0; i < pivot.size(); ++i) {
    p->push_back(pivot_idx[i].second);
  }
}

template <template<typename> class QueryType, typename dist_t>
void GenPermutation(const ObjectVector& pivot,
                    QueryType<dist_t>* query,
                    Permutation* p) {
  std::vector<DistInt<dist_t>> dists;
  for (size_t i = 0; i < pivot.size(); ++i) {
    /* Distance can be asymmetric, pivot should be always on the left side */
    dists.push_back(std::make_pair(query->DistanceObjLeft(pivot[i]),
                                   static_cast<PivotIdType>(i)));
  }
  std::sort(dists.begin(), dists.end());
  std::vector<IntInt> pivot_idx;
  for (size_t i = 0; i < pivot.size(); ++i) {
    pivot_idx.push_back(std::make_pair(dists[i].second,
                                       static_cast<PivotIdType>(i)));
  }
  std::sort(pivot_idx.begin(), pivot_idx.end());
  for (size_t i = 0; i < pivot.size(); ++i) {
    p->push_back(pivot_idx[i].second);
  }
}

template <typename dist_t>
void GetPermutation(
    const ObjectVector& pivot, RangeQuery<dist_t>* query, Permutation* p) {
  GenPermutation(pivot, query, p);
}

template <typename dist_t>
void GetPermutation(
    const ObjectVector& pivot, KNNQuery<dist_t>* query, Permutation* p) {
  GenPermutation(pivot, query, p);
}


// Permutation Prefix Index

template <typename dist_t>
void GetPermutationPPIndex(const ObjectVector& pivot,
                           const Space<dist_t>* space,
                           const Object* object,
                           Permutation* p) {
  // get pivot id
  std::vector<DistInt<dist_t>> dists;
  for (size_t i = 0; i < pivot.size(); ++i) {
    dists.push_back(std::make_pair(space->IndexTimeDistance(pivot[i], object),
                                   static_cast<PivotIdType>(i)));
  }
  std::sort(dists.begin(), dists.end());
  // dists.second = pivot id    i.e.  \Pi_o(i)

  for (size_t i = 0; i < pivot.size(); ++i) {
    p->push_back(dists[i].second);
  }
}

template <template<typename> class QueryType, typename dist_t>
void GetPermutationPPIndex(const ObjectVector& pivot,
                           QueryType<dist_t>* query,
                           Permutation* p) {
  std::vector<DistInt<dist_t>> dists;
  for (size_t i = 0; i < pivot.size(); ++i) {
    /* Distance can be asymmetric, pivot should be always on the left side */
    dists.push_back(std::make_pair(query->DistanceObjLeft(pivot[i]),
                                   static_cast<PivotIdType>(i)));
  }
  std::sort(dists.begin(), dists.end());
  for (size_t i = 0; i < pivot.size(); ++i) {
    p->push_back(dists[i].second);
  }
}

}  // namespace similarity

#endif     // _PERMUTATION_UTILS_H_

