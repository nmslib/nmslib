/**
 * Non-metric Space Library
 *
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

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "seqsearch.h"

namespace similarity {

template <typename dist_t>
void SeqSearch<dist_t>::Search(RangeQuery<dist_t>* query) {
  for (size_t i = 0; i < data_.size(); ++i) {
    query->CheckAndAddToResult(data_[i]);
  }
}

template <typename dist_t>
void SeqSearch<dist_t>::Search(KNNQuery<dist_t>* query) {
  for (size_t i = 0; i < data_.size(); ++i) {
    query->CheckAndAddToResult(data_[i]);
  }
}

template class SeqSearch<float>;
template class SeqSearch<double>;
template class SeqSearch<int>;

}
