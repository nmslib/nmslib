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
#include <limits>

#include "space.h"
#include "method/lsh_space.h"
#include "method/lsh_multiprobe.h"
#include "knnquery.h"
#include "rangequery.h"

namespace similarity {

template <typename dist_t>
MultiProbeLSH<dist_t>::MultiProbeLSH(const Space<dist_t>& space,
                                     const ObjectVector& data) : Index<dist_t>(data) {
}

template <typename dist_t>
void MultiProbeLSH<dist_t>::CreateIndex(const AnyParams& IndexParams) {
    unsigned  LSH_M = 20;
    unsigned  LSH_L = 50;
    unsigned  LSH_H = 1017881;
    float     LSH_W = 20;
    unsigned  LSH_T = 10;
    unsigned  LSH_TuneK = 1;
    float     DesiredRecall = 0.9;

    AnyParamManager pmgr(IndexParams);

    pmgr.GetParamOptional("M",  LSH_M,  20);
    pmgr.GetParamOptional("L",  LSH_L,  50);
    pmgr.GetParamOptional("H",  LSH_H,  this->data_.size() + 1);
    pmgr.GetParamOptional("W",  LSH_W,  20);
    pmgr.GetParamOptional("T",  LSH_T,  10);
    pmgr.GetParamOptional("tuneK",  LSH_TuneK, 1);
    pmgr.GetParamOptional("desiredRecall",  DesiredRecall, 0.9);


    // For FitData():
    // number of points to use
    unsigned N1 = this->data_.size();
    // number of pairs to sample
    unsigned P = 10000;
    // number of queries to sample
    unsigned Q = 1000;
    // search for K neighbors neighbors
    unsigned K = LSH_TuneK;

    pmgr.GetParamOptional("numSamplePairs",    P,  10000);
    pmgr.GetParamOptional("numSampleQueries",  Q,  1000);

    pmgr.CheckUnused();


    LOG(LIB_INFO) << "M (# of hash functions) = "  << LSH_M;
    LOG(LIB_INFO) << "L (# of hash tables)    = "  << LSH_L;
    LOG(LIB_INFO) << "H (# hash table size)   = "  << LSH_H;
    LOG(LIB_INFO) << "W (window size)         = "  << LSH_W;
    LOG(LIB_INFO) << "T (# of probes)         = "  << LSH_T;
    LOG(LIB_INFO) << "desiredRecall (desired recall)            = "  << DesiredRecall;
    LOG(LIB_INFO) << "numSamplePairs   (# of sample pairs, P)   = "  << P;
    LOG(LIB_INFO) << "numSampleQueries (# of sample queries, Q) = "  << Q;
    LOG(LIB_INFO) << "lshTuneK                                  = " << K;

    this->ResetQueryTimeParams(); // set query-time parameters to default values

    // divide the sample to F folds
    unsigned F = 10;
    // For MPLSHTune():
    // dataset size
    unsigned N2 = this->data_.size();
    // desired recall

    CreateIndexInternal(
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
void MultiProbeLSH<dist_t>::CreateIndexInternal(
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
  CHECK(!this->data_.empty());
  const size_t datalength = this->data_[0]->datalength();
  dim_ = static_cast<int>(datalength / sizeof(float));
  matrix_ = new lshkit::FloatMatrix(dim_, static_cast<int>(this->data_.size()));
  for (size_t i = 0; i < this->data_.size(); ++i) {
    CHECK(datalength == this->data_[i]->datalength());
    const float* x = reinterpret_cast<const float*>(this->data_[i]->data());
    for (int j = 0; j < dim_; ++j) {
      (*matrix_)[i][j] = x[j];
    }
  }
  T_ = T;

  CHECK_MSG(W > 0, "W must be > 0");

#define TUNE_MPLSH_PARAMS
#ifdef TUNE_MPLSH_PARAMS
  R_ = R;

  const std::string fit_data = lshkit::FitData(*matrix_, N1, P, Q, K, F);

  lshkit::MPLSHTune(N2, fit_data, T_, L, R, K, M, W);
#endif

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
const std::string MultiProbeLSH<dist_t>::StrDesc() const {
  return "multiprobe lsh";
}

template <typename dist_t>
void MultiProbeLSH<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  throw runtime_error("Range search isn't supported by LSH");
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
      query->CheckAndAddToResult(sqrt(knn[i].dist), this->data_[knn[i].key]);
    }
  }
}

template class MultiProbeLSH<float>;

}   // namespace similarity
