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
#ifndef _LSH_MULTI_PROBE_H_
#define _LSH_MULTI_PROBE_H_

#include "index.h"
#include "space.h"
#include "lshkit.h"

#define METH_LSH_MULTIPROBE_SYN       "lsh_multiprobe"
#define METH_LSH_MULTIPROBE           "mplsh"

namespace similarity {

// this class is a wrapper around lshkit and
// but lshkit can handle only float!

template <typename dist_t>
class Space;

template <typename dist_t>
class MultiProbeLSH : public Index<dist_t> {
 public:
  MultiProbeLSH(const Space<dist_t>& space,
                const ObjectVector& data);

  void CreateIndex(const AnyParams& IndexParams) override;
  ~MultiProbeLSH();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const  override;
  void Search(KNNQuery<dist_t>* query, IdType) const  override;

  void SetQueryTimeParams(const AnyParams& params)  override {};

  // LSH does copy all data
  virtual bool DuplicateData() const override { return true; }
 private:
  typedef lshkit::MultiProbeLshIndex<unsigned> LshIndexType;

  int dim_;
  lshkit::FloatMatrix* matrix_;
  LshIndexType* index_;
  unsigned T_;
  float R_;
  
  void CreateIndexInternal(
                // for FitData()
                unsigned N1,          // number of points to use
                unsigned P,           // number of pairs to sample
                unsigned Q,           // number of queries to sample
                unsigned K,           // search for K neighbors neighbors
                unsigned F,           // divide the sample to F folds
                // for MPLSHTune()
                unsigned N2,          // dataset size
                double R,             // desired recall
                // other config
                unsigned L,           // # of hash tables
                unsigned T,           // # of bins probed in each hash table
                unsigned H,           // hash table size
                int      M,           // # of hash functions
                float  W              // width
                );

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(MultiProbeLSH);
};

}   // namespace similarity

#endif     // _LSH_MULTI_PROBE_H_
