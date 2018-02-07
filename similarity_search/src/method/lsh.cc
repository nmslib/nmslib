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
#include "knnquery.h"
#include "rangequery.h"
#include "method/lsh.h"

namespace similarity {

template <typename dist_t, typename lsh_t, typename paramcreator_t>
LSH<dist_t, lsh_t, paramcreator_t>::LSH(const Space<dist_t>& space,
                                        const ObjectVector& data,
                                        int P) : Index<dist_t>(data), p_(P) {

}

template <typename dist_t, typename lsh_t, typename paramcreator_t>
void LSH<dist_t, lsh_t, paramcreator_t>::CreateIndex(const AnyParams& IndexParams) {
  float     LSH_W;
  unsigned  LSH_M;
  unsigned  LSH_L;
  unsigned  LSH_H;

  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("W",  LSH_W, 20);
  pmgr.GetParamOptional("M",  LSH_M, 20);
  pmgr.GetParamOptional("L",  LSH_L, 50);
  pmgr.GetParamOptional("H",  LSH_H, this->data_.size() + 1);

  int is_float = std::is_same<float,dist_t>::value;
  CHECK_MSG(is_float, "LSH works only for single-precision numbers");
  CHECK_MSG(sizeof(dist_t) == sizeof(float), "LSH works only for single-precision numbers");
  CHECK_MSG(!this->data_.empty(), "The data set shouldn't be empty");
  CHECK_MSG(p_ == 1 || p_ == 2, "The value of the space selector should be either 1 or 2!");

  const size_t datalength = this->data_[0]->datalength();

  LOG(LIB_INFO) << "W (window size (used only for LSHCauchy and LSHGaussian)) : "  << LSH_W;
  LOG(LIB_INFO) << "M (# of hash functions) : "  << LSH_M;
  LOG(LIB_INFO) << "L (# of hash tables) :    "  << LSH_L;
  LOG(LIB_INFO) << "H (# hash table size) :   "  << LSH_H;

  const int dim = static_cast<int>(datalength / sizeof(float));
  matrix_ = new lshkit::FloatMatrix(dim, static_cast<int>(this->data_.size()));

  for (size_t i = 0; i < this->data_.size(); ++i) {
    CHECK(datalength == this->data_[i]->datalength());
    const float* x = reinterpret_cast<const float*>(this->data_[i]->data());
    for (int j = 0; j < dim; ++j) {
      (*matrix_)[i][j] = x[j];
    }
  }

  LOG(LIB_INFO) << paramcreator_t::StrDesc();
  lshkit::FloatMatrix::Accessor accessor(*matrix_);
  lshkit::DefaultRng rng;
  index_ = new LshIndexType;
  index_->init(paramcreator_t::GetParameter(*matrix_, LSH_H, LSH_M, LSH_W), rng, LSH_L);

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
const std::string LSH<dist_t, lsh_t, paramcreator_t>::StrDesc() const {
  return "lsh";
}

template <typename dist_t, typename lsh_t, typename paramcreator_t>
void LSH<dist_t, lsh_t, paramcreator_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  throw runtime_error("Range search isn't supported by LSH");
}

template <typename dist_t, typename lsh_t, typename paramcreator_t>
void LSH<dist_t, lsh_t, paramcreator_t>::Search(KNNQuery<dist_t>* query, IdType) const {
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
      query->CheckAndAddToResult(knn[i].dist, this->data_[knn[i].key]);
    }
  }
}

template class LSH<float, lshkit::ThresholdingLsh, ParameterCreator<TailRepeatHashThreshold>>;
template class LSH<float, lshkit::CauchyLsh, ParameterCreator<TailRepeatHashCauchy>>;
template class LSH<float, lshkit::GaussianLsh, ParameterCreator<TailRepeatHashGaussian>>;

}   // namespace similarity
