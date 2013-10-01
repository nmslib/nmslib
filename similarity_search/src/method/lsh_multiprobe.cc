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

#include <limits>

#include "space.h"
#include "lsh_space.h"
#include "knnquery.h"
#include "rangequery.h"
#include "lsh_multiprobe.h"

namespace similarity {

template <typename dist_t>
MultiProbeLSH<dist_t>::MultiProbeLSH(const Space<dist_t>* space,
                                     const ObjectVector& data,
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
                                     float  W)
    : data_(data) {
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

  if (W <= 0) {
    LOG(FATAL) << "LshW must be > 0";
  }

#define TUNE_MPLSH_PARAMS
#ifdef TUNE_MPLSH_PARAMS
  R_ = R;

  const std::string fit_data = lshkit::FitData(*matrix_, N1, P, Q, K, F);

  lshkit::MPLSHTune(N2, fit_data, T_, L, R, K, M, W);
#endif

  LOG(INFO) << "M (# of hash functions) : "  << M;
  LOG(INFO) << "L (# of hash tables) :    "  << L;
  LOG(INFO) << "H (# hash table size) :   "  << H;
  LOG(INFO) << "W (width) :               "  << W;
  LOG(INFO) << "T (# of probes) :         "  << T;
  LOG(INFO) << "R (desired recall) :      "  << R;

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
void MultiProbeLSH<dist_t>::Search(RangeQuery<dist_t>* query) {
  LOG(FATAL) << "Not applicable";
}

template <typename dist_t>
void MultiProbeLSH<dist_t>::Search(KNNQuery<dist_t>* query) {
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
