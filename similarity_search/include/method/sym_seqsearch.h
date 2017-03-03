/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2017
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef _SYM_SEQSEARCH_H_
#define _SYM_SEQSEARCH_H_

#include <string>

#include "index.h"

#define METH_SYM_SEQ_SEARCH                 "sym_brute_force"

namespace similarity {

using std::string;
using std::vector;

// Sequential search

template <typename dist_t>
class SymSeqSearch : public Index<dist_t> {
 public:
  SymSeqSearch(Space<dist_t>& space, const ObjectVector& data);
  void CreateIndex(const AnyParams& ) override;

  const std::string StrDesc() const override { return METH_SYM_SEQ_SEARCH; }

  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  void SetQueryTimeParams(const AnyParams& params) override;
 private:
  Space<dist_t>&          space_;
  const ObjectVector&     data_;

  enum FilterType { kNone, kReverse, kMin, kAvg };
  
  FilterType              filterType_ = kMin;
  size_t                  filterK_ = 0;

  dist_t SymDistance(const Query<dist_t>*  q, const Object* o) const {
    if (filterType_ == kNone) return q->Distance(o, q->QueryObject());
    if (filterType_ == kReverse) return q->Distance(q->QueryObject(), o);
    dist_t d1 = q->Distance(q->QueryObject(), o);
    dist_t d2 = q->Distance(o, q->QueryObject());
    return filterType_ == kMin ? min(d1, d2) : (d1+d2)*0.5;
  }


  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(SymSeqSearch);
};

}   // namespace similarity

#endif     // _SEQSEARCH_H_
