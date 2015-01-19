/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * Patent ALERT: even though the code is released under the liberal Apache 2 license,
 * the underlying search method is patented. Therefore, it's free to use in research,
 * but may be problematic in a production setting.
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef _OMEDRANK_METHOD_H_
#define _OMEDRANK_METHOD_H_

#include <string>
#include <vector>
#include <sstream>

#include "index.h"

#define METH_OMEDRANK             "omedrank"

namespace similarity {

using std::string;

/*
 * This is an implementation of Fagin's OMEDRANK method for arbitrary spaces.
 * The main difference is that instead of projections, we use a distance to randomly
 * selected pivots.
 *
 * Patent: 
 *    Efficient similarity search and classification via rank aggregation
 *    United States Patent Application 20040249831
 *
 * Paper: 
 *    Fagin, Ronald, Ravi Kumar, and Dandapani Sivakumar. 
 *    "Efficient similarity search and classification via rank aggregation." 
 *    Proceedings of the 2003 ACM SIGMOD international conference on Management of data. ACM, 2003.
 */

template <typename dist_t>
class OMedRank : public Index<dist_t> {
 public:
  OMedRank(const Space<dist_t>* space,
           const ObjectVector& data,
           const size_t num_pivot,
           AnyParamManager &pmgr);
  ~OMedRank(){};

  const std::string ToString() const { return "omedrank" ; }
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

  virtual vector<string> GetQueryTimeParamNames() const;

 private:
  virtual void SetQueryTimeParamsInternal(AnyParamManager& );

  const ObjectVector&     data_;
  float					          db_scan_frac_;
  float                   min_freq_;
  size_t                  db_scan_;
  ObjectVector            pivot_;

  struct ObjectInvEntry {
    IdType    id_;
    dist_t    pivot_dist_;
    ObjectInvEntry(IdType id, dist_t pivot_dist) : id_(id), pivot_dist_(pivot_dist) {} 
    bool operator<(const ObjectInvEntry& o) const { 
      if (pivot_dist_ != o.pivot_dist_) return pivot_dist_ < o.pivot_dist_; 
      return id_ < o.id_;
    };
  };

  typedef std::vector<ObjectInvEntry> PostingList;

  std::vector<PostingList> posting_lists_;
  
  void ComputeDbScan(float db_scan_fraction) {
    db_scan_ = max(size_t(1), static_cast<size_t>(db_scan_fraction * data_.size()));
  }

  template <typename QueryType> void GenSearch(QueryType* query);

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(OMedRank);
};

}   // namespace similarity

#endif     // _OMEDRANK_METHOD_H_
