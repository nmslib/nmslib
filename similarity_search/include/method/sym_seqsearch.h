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

#define SYM_TYPE_PARAM "symmType"
#define SYM_K_PARAM    "symmCandK"

#define SYM_TYPE_MIN    "min"
#define SYM_TYPE_AVG    "avg"
#define SYM_TYPE_REV    "reverse"
#define SYM_TYPE_NONE   "none"

namespace similarity {

using std::string;
using std::vector;

enum SymmType { kSymmNone, kSymmReverse, kSymmMin, kSymmAvg };

// Sequential search

inline
SymmType getSymmType(string s) {
  ToLower(s);
  if (s == SYM_TYPE_NONE) return kSymmNone;
  else if (s == SYM_TYPE_REV) return kSymmReverse;
  else if (s == SYM_TYPE_MIN) return kSymmMin;
  else if (s == SYM_TYPE_AVG) return kSymmAvg;
  else {
    PREPARE_RUNTIME_ERR(err) << "Invalid " << SYM_TYPE_PARAM << " param value: " << s;
    THROW_RUNTIME_ERR(err);
  }
}

template <typename dist_t>
inline dist_t SymmDistance(const Space<dist_t>&  s, const Object* o1, const Object* o2, SymmType stype) {
  if (stype == kSymmNone) return s.IndexTimeDistance(o1, o2);
  if (stype == kSymmReverse) return s.IndexTimeDistance(o2, o1);
  dist_t d1 = s.IndexTimeDistance(o1, o2);
  dist_t d2 = s.IndexTimeDistance(o2, o1);
  return stype == kSymmMin ? min(d1, d2) : (d1+d2)*0.5;
}

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

  
  SymmType                symmType_  = kSymmMin;
  size_t                  symmCandK_ = 0;

  dist_t SymmDistance(const Query<dist_t>*  q, const Object* o) const {
    if (symmType_ == kSymmNone) return q->Distance(o, q->QueryObject());
    if (symmType_ == kSymmReverse) return q->Distance(q->QueryObject(), o);
    dist_t d1 = q->Distance(q->QueryObject(), o);
    dist_t d2 = q->Distance(o, q->QueryObject());
    return symmType_ == kSymmMin ? min(d1, d2) : (d1+d2)*0.5;
  }


  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(SymSeqSearch);
};

}   // namespace similarity

#endif     // _SEQSEARCH_H_
