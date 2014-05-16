/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
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
#include "knnquery.h"
#include "rangequery.h"
#include "method/lsh.h"

namespace similarity {

template <typename dist_t, typename lsh_t, typename paramcreator_t>
LSH<dist_t, lsh_t, paramcreator_t>::LSH(const Space<dist_t>* space,
                                        const ObjectVector& data,
                                        int P,
                                        float W,
                                        unsigned M,
                                        unsigned L,
                                        unsigned H)
    : data_(data), p_(P) {
  int is_float = std::is_same<float,dist_t>::value;
  CHECK(is_float);
  CHECK(sizeof(dist_t) == sizeof(float));
  CHECK(!data.empty());
  CHECK(P == 1 || P == 2);
  const size_t datalength = data[0]->datalength();

  LOG(INFO) << "M (# of hash functions) : "  << M;
  LOG(INFO) << "L (# of hash tables) :    "  << L;
  LOG(INFO) << "H (# hash table size) :   "  << H;

  const int dim = static_cast<int>(datalength / sizeof(float));
  matrix_ = new lshkit::FloatMatrix(dim, static_cast<int>(data.size()));

  for (size_t i = 0; i < data.size(); ++i) {
    CHECK(datalength == data[i]->datalength());
    const float* x = reinterpret_cast<const float*>(data[i]->data());
    for (int j = 0; j < dim; ++j) {
      (*matrix_)[i][j] = x[j];
    }
  }

  LOG(INFO) << paramcreator_t::ToString();
  lshkit::FloatMatrix::Accessor accessor(*matrix_);
  lshkit::DefaultRng rng;
  index_ = new LshIndexType;
  index_->init(paramcreator_t::GetParameter(*matrix_, H, M, W), rng, L);

  for (int i = 0; i < matrix_->getSize(); ++i) {
    index_->insert(i, (*matrix_)[i]);
  }
}

template <typename dist_t, typename lsh_t, typename paramcreator_t>
LSH<dist_t, lsh_t, paramcreator_t>::~LSH() {
  delete matrix_;
  delete index_;
}

template <typename dist_t, typename lsh_t, typename paramcreator_t>
const std::string LSH<dist_t, lsh_t, paramcreator_t>::ToString() const {
  return "lsh";
}

template <typename dist_t, typename lsh_t, typename paramcreator_t>
void LSH<dist_t, lsh_t, paramcreator_t>::Search(RangeQuery<dist_t>* query) {
  LOG(FATAL) << "Not applicable!";
}

template <typename dist_t, typename lsh_t, typename paramcreator_t>
void LSH<dist_t, lsh_t, paramcreator_t>::Search(KNNQuery<dist_t>* query) {
  const size_t datalength = query->QueryObject()->datalength();
  const int dim = static_cast<int>(datalength / sizeof(float));
  CHECK(dim == matrix_->getDim());

  const float* q = reinterpret_cast<const float*>(query->QueryObject()->data());

  lshkit::FloatMatrix::Accessor accessor(*matrix_);
  LSHLpSpace<dist_t> lp(dim, p_, query);
  lshkit::TopkScanner<lshkit::FloatMatrix::Accessor, LSHLpSpace<dist_t>>
      query_scanner(accessor, lp, query->GetK());
  query_scanner.reset(q);

  index_->query(q, query_scanner);

  const lshkit::Topk<uint32_t>& knn = query_scanner.topk();
  for (size_t i = 0; i < knn.size(); ++i) {
    if (knn[i].key != std::numeric_limits<uint32_t>::max()) {
      query->CheckAndAddToResult(knn[i].dist, data_[knn[i].key]);
    }
  }
}

template class LSH<float, lshkit::ThresholdingLsh, ParameterCreator<TailRepeatHashThreshold>>;
template class LSH<float, lshkit::CauchyLsh, ParameterCreator<TailRepeatHashCauchy>>;
template class LSH<float, lshkit::GaussianLsh, ParameterCreator<TailRepeatHashGaussian>>;

}   // namespace similarity
