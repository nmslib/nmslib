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

#include <limits>

#include "space.h"
#include "method/lsh_space.h"
#include "method/lsh_multiprobe.h"
#include "knnquery.h"
#include "rangequery.h"

namespace similarity {

template <typename dist_t>
MultiProbeLSH<dist_t>::MultiProbeLSH(const Space<dist_t>* space,
                                     const ObjectVector& data)
    : data_(data) {
}

template <typename dist_t>
MultiProbeLSH<dist_t>::CreateIndex(const AnyParams& IndexParams) {
    unsigned  LSH_M = 20;
    unsigned  LSH_L = 50;
    unsigned  LSH_H = 1017881;
    float     LSH_W = 20;
    unsigned  LSH_T = 10;
    unsigned  LSH_TuneK = 1;
    float     DesiredRecall = 0.5;

    AnyParamManager pmgr(IndexParams);

    pmgr.GetParamOptional("M",  LSH_M);
    pmgr.GetParamOptional("L",  LSH_L);
    pmgr.GetParamOptional("H",  LSH_H);
    pmgr.GetParamOptional("W",  LSH_W);
    pmgr.GetParamOptional("T",  LSH_T);
    pmgr.GetParamOptional("tuneK",  LSH_TuneK);
    pmgr.GetParamOptional("desiredRecall",  DesiredRecall);


    // For FitData():
    // number of points to use
    unsigned N1 = DataObjects.size();
    // number of pairs to sample
    unsigned P = 10000;
    // number of queries to sample
    unsigned Q = 1000;
    // search for K neighbors neighbors
    unsigned K = LSH_TuneK;

    pmgr.GetParamOptional("numSamplePairs",  P);
    pmgr.GetParamOptional("numSampleQueries",  Q);

    LOG(LIB_INFO) << "lshTuneK: " << K;
    // divide the sample to F folds
    unsigned F = 10;
    // For MPLSHTune():
    // dataset size
    unsigned N2 = DataObjects.size();
    // desired recall

    CreateIndexInternal(
                  space,
                  DataObjects,
                  N1, P, Q, K, F, N2,
                  DesiredRecall,
                  LSH_L,
                  LSH_T,
                  LSH_H,
                  LSH_M,
                  LSH_W
                  );
}

template <typename dist_t>
MultiProbeLSH<dist_t>::CreateIndexInternal(
                                     unsigned N1,
                                     unsigned P,
                                     unsigned Q,
                                     unsigned K,
                                     unsigned F,
                                     unsigned N2,
                                     double R,
                                     unsigned L,
                                     unsigned T,
                                     unsigned H,
                                     int    M,
                                     float  W) {
  int is_float = std::is_same<float,dist_t>::value;
  CHECK(is_float);
  CHECK(sizeof(dist_t) == sizeof(float));
  CHECK(!data.empty());
  const size_t datalength = data[0]->datalength();
  dim_ = static_cast<int>(datalength / sizeof(float));
  matrix_ = new lshkit::FloatMatrix(dim_, static_cast<int>(data.size()));
  for (size_t i = 0; i < data.size(); ++i) {
    CHECK(datalength == data[i]->datalength());
    const float* x = reinterpret_cast<const float*>(data[i]->data());
    for (int j = 0; j < dim_; ++j) {
      (*matrix_)[i][j] = x[j];
    }
  }
  T_ = T;

  CHECK_MSG(W > 0, "LshW must be > 0");

#define TUNE_MPLSH_PARAMS
#ifdef TUNE_MPLSH_PARAMS
  R_ = R;

  const std::string fit_data = lshkit::FitData(*matrix_, N1, P, Q, K, F);

  lshkit::MPLSHTune(N2, fit_data, T_, L, R, K, M, W);
#endif

  LOG(LIB_INFO) << "M (# of hash functions) : "  << M;
  LOG(LIB_INFO) << "L (# of hash tables) :    "  << L;
  LOG(LIB_INFO) << "H (# hash table size) :   "  << H;
  LOG(LIB_INFO) << "W (width) :               "  << W;
  LOG(LIB_INFO) << "T (# of probes) :         "  << T;
  LOG(LIB_INFO) << "R (desired recall) :      "  << R;
  LOG(LIB_INFO) << "P (# of sample pairs) :   "  << P;
  LOG(LIB_INFO) << "Q (# of sample queries) : "  << Q;

  lshkit::FloatMatrix::Accessor accessor(*matrix_);
  LshIndexType::Parameter param;
  param.W = W;
  param.range = H;
  param.repeat = M;
  param.dim = dim_;
  lshkit::DefaultRng rng;

  index_ = new LshIndexType;
  index_->init(param, rng, L);

  for (int i = 0; i < matrix_->getSize(); ++i) {
    index_->insert(i, (*matrix_)[i]);
  }
}

template <typename dist_t>
MultiProbeLSH<dist_t>::~MultiProbeLSH() {
  delete matrix_;
  delete index_;
}

template <typename dist_t>
const std::string MultiProbeLSH<dist_t>::ToString() const {
  return "multiprobe lsh";
}

template <typename dist_t>
void MultiProbeLSH<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  LOG(LIB_FATAL) << "Not applicable";
}

template <typename dist_t>
void MultiProbeLSH<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  const size_t datalength = query->QueryObject()->datalength();
  const int dim = static_cast<int>(datalength / sizeof(float));
  CHECK(dim == dim_);

  const float* q = reinterpret_cast<const float*>(query->QueryObject()->data());

  lshkit::FloatMatrix::Accessor accessor(*matrix_);
  LSHMultiProbeLpSpace<dist_t> lp(dim_, query);
  lshkit::TopkScanner<lshkit::FloatMatrix::Accessor, LSHMultiProbeLpSpace<dist_t>>
      query_scanner(accessor, lp, query->GetK());
  query_scanner.reset(q);

  index_->query(q, T_, query_scanner);

  const lshkit::Topk<uint32_t>& knn = query_scanner.topk();
  for (size_t i = 0; i < knn.size(); ++i) {
    if (knn[i].key != std::numeric_limits<uint32_t>::max()) {
      query->CheckAndAddToResult(sqrt(knn[i].dist), data_[knn[i].key]);
    }
  }
}

template class MultiProbeLSH<float>;

}   // namespace similarity
