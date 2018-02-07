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
#ifndef _MULT_INDEX_H_
#define _MULT_INDEX_H_

#include <vector>

#include "index.h"
#include "params.h"
#include "methodfactory.h"

#define METH_MULT_INDEX     "mult_index"

namespace similarity {

/*
 * This is a generic method that can create multiple copies of any method indices.
 * During search, results obtained from all the copies are aggregated.
 * This allows us to achieve higher recall when an approximate search method is used.
 * In other words, using multiple copies of the index created using the same method 
 * permits to trade efficiency for effectiveness. However, this makes sense only for the
 * methods where creation of the index is NON-deterministic, e.g., when
 * pivots are created randomly.
 * 
 */

template <typename dist_t> class Space;

template <typename dist_t>
class MultiIndex : public Index<dist_t> {
 public:
  MultiIndex(bool PrintProgress,
             const string& SpaceType,
             Space<dist_t>& space, 
             const ObjectVector& data);

  void CreateIndex(const AnyParams& IndexParams) override;

  ~MultiIndex();

  const std::string StrDesc() const override;

  void Search(RangeQuery<dist_t>* query, IdType ) const override;
  void Search(KNNQuery<dist_t>* query, IdType ) const override;

  virtual void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;
 protected:

  std::vector<Index<dist_t>*> indices_;
  Space<dist_t>&              space_;
  string                      SpaceType_;
  bool                        PrintProgress_;
  size_t                      IndexQty_;
  string                      MethodName_;
};

}   // namespace similarity

#endif
