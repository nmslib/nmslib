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

#ifndef _PIVOT_NEIGHBORHOOD_INVINDEX_H
#define _PIVOT_NEIGHBORHOOD_INVINDEX_H

#include <vector>
#include "index.h"
#include "permutation_utils.h"

#define METH_PIVOT_NEIGHB_INVINDEX   "pivot_neighb_invindx"

#define PERM_PROC_FAST_SCAN       "scan"
#define PERM_PROC_MAP             "map"
#define PERM_PROC_MERGE           "merge"

namespace similarity {

using std::vector;

/*
 * The main idea of the method (indexing K most closest pivots using an inverted file)
 * was taken from the paper:
 *
 * Eric Sadit Tellez, Edgar Chavez and Gonzalo Navarro,
 * Succinct Nearest Neighbor Search, SISAP 2011
 *
 * In this implementation, we introduce several important modifications:
 * 1) The inverted file is split into small parts
 * 2) The index is not compressed (thought it could be)
 * 3) Instead of the adaptive union algorithm, we use a well-known ScanCount algorithm (by default). 
 *    The overall time spent on processing of the inverted file is 20-30% of the overall
 *    search time. Thus, the retrieval time cannot be substantially improved by
 *    replacing ScanCount with even better approach (should one exist).
 * 4) We also implemented several other simple algorithms for posting processing, to compare
 *    against ScanCount. For instance, the merge-sort union algorithms is about 2-3 times as slow.
 *
 *  For an example of using ScanCount see, e.g.:
 *
 *        Li, Chen, Jiaheng Lu, and Yiming Lu. 
 *       "Efficient merging and filtering algorithms for approximate string searches." 
 *        In Data Engineering, 2008. ICDE 2008. 
 *        IEEE 24th International Conference on, pp. 257-266. IEEE, 2008.
 */

typedef vector<int> PostingListInt;

template <typename dist_t>
class PivotNeighbInvertedIndex : public Index<dist_t> {
 public:
  PivotNeighbInvertedIndex(const Space<dist_t>* space,
                           const ObjectVector& data,
                           const AnyParams& AllParams);

  ~PivotNeighbInvertedIndex();

  const std::string ToString() const;
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);
  
  virtual void SetQueryTimeParams(AnyParamManager& );
  virtual vector<string> GetQueryTimeParamNames() const;

 private:
  const ObjectVector& data_;
  size_t  chunk_index_size_;
  size_t  db_scan_;
  size_t  num_prefix_;       // K in the original paper
  size_t  min_times_;        // t in the original paper
  bool    use_sort_;
  bool    skip_checking_;

  enum eAlgProctype {
    kScan,
    kMap,
    kMerge
  } inv_proc_alg_;


  ObjectVector pivot_;

  void ComputeDbScan(float db_scan_frac) {
    if (db_scan_frac < 0.0 || db_scan_frac > 1.0) {
      LOG(FATAL) << METH_PIVOT_NEIGHB_INVINDEX << " requires that dbScanFrac is in the range [0,1]";
    }    
    db_scan_ = std::max(size_t(1),static_cast<size_t>(db_scan_frac * data_.size()));
  }
  
  vector<vector<PostingListInt>> posting_lists_;

  template <typename QueryType> void GenSearch(QueryType* query);

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(PivotNeighbInvertedIndex);
};

}  // namespace similarity

#endif     // _PERMUTATION_SUCCINCT_INDEX_H_

