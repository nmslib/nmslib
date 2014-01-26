/**
 * Non-metric Space Library
 *#
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef _INDEX_STRUCTURE_H_
#define _INDEX_STRUCTURE_H_

#include <stdio.h>
#include <string>

namespace similarity {

template <typename dist_t>
class RangeQuery;

template <typename dist_t>
class KNNQuery;

/*
 * Abstract class for all index structures
 */
template <typename dist_t>
class Index {
 public:
  virtual ~Index() {}
  virtual void Search(RangeQuery<dist_t>* query) = 0;
  virtual void Search(KNNQuery<dist_t>* query) = 0;
  virtual const std::string ToString() const = 0;
};

}  // namespace similarity

#endif     // _METRIC_INDEX_STRUCTURE_H_
