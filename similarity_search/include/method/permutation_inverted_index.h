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
#ifndef _PERM_INVERTED_INDEX_H_
#define _PERM_INVERTED_INDEX_H_

#include <vector>
#include "index.h"
#include "permutation_utils.h"

#define METH_PERM_INVERTED_INDEX       "perm_inv_indx"
#define METH_PERM_INVERTED_INDEX_SYN   "mi-file"

#define USE_MAP_THRESHOLD 0.01

namespace similarity {

/*
 * Giuseppe Amato and Pasquale Savino,
 * Approximate Similarity Search in Metric Spaces Using Inverted Files,
 * Infoscale (2008)
 *
 * Later dubbed as MI-File (metric inverted file).
 */

template <typename dist_t>
class PermutationInvertedIndex : public Index<dist_t> {
 public:
  PermutationInvertedIndex(
                bool  PrintProgress,
                const Space<dist_t>& space,
                const ObjectVector& data);

  void CreateIndex(const AnyParams& params) override;
  ~PermutationInvertedIndex();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;
  
  virtual void SetQueryTimeParams(const AnyParams& params) override;

 private:
  const Space<dist_t>&  space_;
  bool                  PrintProgress_;

  float  db_scan_frac_;
  size_t num_pivot_;            // overall number of pivots
  size_t num_pivot_index_;      // ki in the original paper
  size_t num_pivot_search_;     // ks in the original paper
  size_t max_pos_diff_;
  size_t knn_amp_;
  ObjectVector pivot_;

  size_t computeDbScan(size_t K) const {
    if (knn_amp_) { return min(K * knn_amp_, this->data_.size()); }
    return static_cast<size_t>(db_scan_frac_ * this->data_.size());
  }

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

  // K==0 for range search
  template <typename QueryType> void GenSearch(QueryType* query, size_t K) const;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PermutationInvertedIndex);
};

}  // namespace similarity

#endif     // _INVERTED_INDEX_H_
