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
/*
 * Patent ALERT: even though the code is released under the liberal Apache 2 license,
 * the underlying search method is patented. Therefore, it's free to use in research,
 * but may be problematic in a production setting.
*/

#ifndef _OMEDRANK_METHOD_H_
#define _OMEDRANK_METHOD_H_

#include <string>
#include <vector>
#include <sstream>
#include <memory>

#include "index.h"
#include "projection.h"
#include "ported_boost_progress.h"

#define METH_OMEDRANK             "omedrank"

namespace similarity {

using std::string;
using std::vector;

/*
 * This is an implementation of Fagin's OMEDRANK method for arbitrary spaces.
 * The main difference is that we allow the user to select several types of
 * projections, while the original Fagin's OMEDRANK relies only on the random
 * projections.
 *
 * Patent: 
 *    Efficient similarity search and classification via rank aggregation
 *    United States Patent Application 20040249831
 *
 * Paper: 
 *    Fagin, Ronald, Ravi Kumar, and Dandapani Sivakumar. 
 *    "Efficient similarity search and classification via rank aggregation." 
 *    Proceedings of the 2003 ACM SIGMOD international conference on Management of data. ACM, 2003.
 *
 * In this implementation, we introduce several modifications:
 * 1) Book-keeping (i.e., counting the number of occurrences) relies on a well-known ScanCount algorithm.
 * 2) The inverted file is split into small parts. In doing so, we aim to
 *    achieve better caching properties of the counter array used in ScanCount.
 *    
 *  For an example of using ScanCount see, e.g.:
 *
 *        Li, Chen, Jiaheng Lu, and Yiming Lu. 
 *       "Efficient merging and filtering algorithms for approximate string searches." 
 *        In Data Engineering, 2008. ICDE 2008. 
 *        IEEE 24th International Conference on, pp. 257-266. IEEE, 2008.
 */

template <typename dist_t>
class OMedRank : public Index<dist_t> {
 public:
  OMedRank(bool PrintProgress,
           const Space<dist_t>& space,
           const ObjectVector& data);

  void CreateIndex(const AnyParams& IndexParams) override;
  virtual ~OMedRank(){};

  const std::string StrDesc() const override { return "omedrank" ; }
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;
 private:
  void IndexChunk(size_t chunkId, ProgressDisplay* displayBar);

  const Space<dist_t>&    space_;
  bool                    PrintProgress_;

  size_t                  num_pivot_;
  size_t                  num_pivot_search_;
  size_t                  chunk_index_size_;
  size_t                  index_qty_;
  size_t                  db_scan_;
  bool                    skip_check_;
  string                  proj_type_;
  size_t                  interm_dim_; // used only for sparse vector spaces
  size_t                  K_;
  size_t                  knn_amp_;
  float					          db_scan_frac_;
  float                   min_freq_;
  std::unique_ptr<Projection<dist_t>>  projection_;

  struct ObjectInvEntry {
    IdType    id_;
    float     pivot_dist_;
    ObjectInvEntry(IdType id, dist_t pivot_dist) : id_(id), pivot_dist_(pivot_dist) {} 
    bool operator<(const ObjectInvEntry& o) const { 
      if (pivot_dist_ != o.pivot_dist_) return pivot_dist_ < o.pivot_dist_; 
      return id_ < o.id_;
    };
  };

  typedef vector<ObjectInvEntry> PostingList;

  vector<shared_ptr<vector<PostingList>>> posting_lists_;
  
  // Heuristics: try to read db_scan_fraction/index_qty entries from each index part
  // or alternatively K * knn_amp_ entries, for KNN-search
  size_t computeDbScan(size_t K) const {
    if (knn_amp_) { return min(K * knn_amp_, this->data_.size()); }
    return static_cast<size_t>(db_scan_frac_ * this->data_.size());
  }

  template <typename QueryType> void GenSearch(QueryType* query, size_t K) const;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(OMedRank);
};

}   // namespace similarity

#endif     // _OMEDRANK_METHOD_H_
