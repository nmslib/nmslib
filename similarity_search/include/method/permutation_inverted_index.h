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
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _PERM_INVERTED_INDEX_H_
#define _PERM_INVERTED_INDEX_H_

#include <vector>
#include "index.h"
#include "permutation_utils.h"

#define METH_PERM_INVERTED_INDEX   "perm_inv_indx"

#define USE_MAP_THRESHOLD 0.01

namespace similarity {

/*
 * Giuseppe Amato and Pasquale Savino,
 * Approximate Similarity Search in Metric Spaces Using Inverted Files,
 * Infoscale (2008)
 */

template <typename dist_t>
class PermutationInvertedIndex : public Index<dist_t> {
 public:
  PermutationInvertedIndex(const Space<dist_t>* space,
                const ObjectVector& data,
                AnyParams params);
  ~PermutationInvertedIndex();

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

  virtual vector<string> GetQueryTimeParamNames() const;

 private:
  virtual void SetQueryTimeParamsInternal(AnyParamManager& );

  const ObjectVector& data_;
  float  db_scan_frac_;
  size_t db_scan_;
  size_t num_pivot_;            // overall number of pivots
  size_t num_pivot_index_;      // ki in the original paper
  size_t num_pivot_search_;     // ks in the original paper
  size_t max_pos_diff_;
  ObjectVector pivot_;

  struct ObjectInvEntry {
    IdType    id_;
    int       pos_;
    ObjectInvEntry(IdType id, int pos) : id_(id), pos_(pos) {} 
    bool operator<(const ObjectInvEntry& o) const { 
      if (pos_ != o.pos_) return pos_ < o.pos_; 
      return id_ < o.id_;
    };
  };

  typedef std::vector<ObjectInvEntry> PostingList;

  std::vector<PostingList> posting_lists_;

  template <typename QueryType> void GenSearch(QueryType* query);

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PermutationInvertedIndex);
};

}  // namespace similarity

#endif     // _INVERTED_INDEX_H_
