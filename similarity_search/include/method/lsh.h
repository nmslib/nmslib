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
#ifndef _LSH_H_
#define _LSH_H_

#include "index.h"
#include "space.h"
#include "lshkit.h"

#define METH_LSH_THRESHOLD          "lsh_threshold"
#define METH_LSH_CAUCHY             "lsh_cauchy"
#define METH_LSH_GAUSSIAN           "lsh_gaussian"

namespace similarity {

// this class is a wrapper around lshkit and
// but lshkit can handle only float!

template <typename dist_t>
class Space;

template <typename lsh_t>
using TailRepeatHash = lshkit::Tail<lshkit::RepeatHash<lsh_t>>;

using TailRepeatHashThreshold = TailRepeatHash<lshkit::ThresholdingLsh>;
using TailRepeatHashCauchy = TailRepeatHash<lshkit::CauchyLsh>;
using TailRepeatHashGaussian = TailRepeatHash<lshkit::GaussianLsh>;

template <typename lsh_t>
class ParameterCreator {
 public:
  static typename lsh_t::Parameter GetParameter(
    const lshkit::FloatMatrix& matrix,
    unsigned H, unsigned M, float W) {
    throw runtime_error("not allowed: dummy parameter creator");
    return lsh_t::Parameter();
  }

  static std::string StrDesc() {
    return "ParameterCreator<dummy>";
  }
};

template <>
class ParameterCreator<TailRepeatHashThreshold> {
 public:
  static TailRepeatHashThreshold::Parameter GetParameter(
    const lshkit::FloatMatrix& matrix,
    unsigned H, unsigned M, float W) {
    TailRepeatHashThreshold::Parameter param;
    param.range = H;
    param.repeat = M;
    param.dim = matrix.getDim();
    param.min = std::numeric_limits<float>::max();
    param.max = std::numeric_limits<float>::min();
    for (int i = 0; i < matrix.getSize(); ++i) {
      for (int j = 0; j < matrix.getDim(); ++j) {
        if (matrix[i][j] < param.min) param.min = matrix[i][j];
        if (matrix[i][j] > param.max) param.max = matrix[i][j];
      }
    }
    LOG(LIB_INFO) << "min  " << param.min << " max " << param.max;
    return param;
  }

  static std::string StrDesc() {
    return "ParameterCreator<TailRepeatHashThreshold> (l1 distance)";
  }
};

template <>
class ParameterCreator<TailRepeatHashCauchy> {
 public:
  static TailRepeatHashCauchy::Parameter GetParameter(
    const lshkit::FloatMatrix& matrix,
    unsigned H, unsigned M, float W) {
    TailRepeatHashCauchy::Parameter param;
    param.range = H;
    param.repeat = M;
    param.dim = matrix.getDim();
    param.W = W;
    return param;
  }

  static std::string StrDesc() {
    return "ParameterCreator<TailRepeatHashCauchy> (l1 distance)";
  }
};

template <>
class ParameterCreator<TailRepeatHashGaussian> {
 public:
  static TailRepeatHashGaussian::Parameter GetParameter(
    const lshkit::FloatMatrix& matrix,
    unsigned H, unsigned M, float W) {
    TailRepeatHashGaussian::Parameter param;
    param.range = H;
    param.repeat = M;
    param.dim = matrix.getDim();
    param.W = W;
    return param;
  }

  static std::string StrDesc() {
    return "ParameterCreator<TailRepeatHashGaussian> (l2 distance)";
  }
};


template <typename dist_t, typename lsh_t, typename paramcreator_t>
class LSH : public Index<dist_t> {
 public:
  LSH(const Space<dist_t>& space,
      const ObjectVector& data,
      int P      // lp (l1 or l2)
  );

  void CreateIndex(const AnyParams& IndexParams) override;

  ~LSH();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType ) const override;
  void Search(KNNQuery<dist_t>* query, IdType ) const override;
  void SetQueryTimeParams(const AnyParams& params)  override {};

  // LSH does copy all data
  virtual bool DuplicateData() const override { return true; }
 private:
  typedef lshkit::LshIndex<TailRepeatHash<lsh_t>, unsigned> LshIndexType;

  int p_;
  lshkit::FloatMatrix* matrix_;
  LshIndexType* index_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(LSH);
};

// for l1 distance
template <typename dist_t>
using LSHThreshold = LSH<dist_t, lshkit::ThresholdingLsh,
                         ParameterCreator<TailRepeatHashThreshold>>;

// for l1 distance
template <typename dist_t>
using LSHCauchy = LSH<dist_t, lshkit::CauchyLsh,
                      ParameterCreator<TailRepeatHashCauchy>>;

// for l2 distance
template <typename dist_t>
using LSHGaussian = LSH<dist_t, lshkit::GaussianLsh,
                        ParameterCreator<TailRepeatHashGaussian>>;

}   // namespace similarity

#endif     // _LSH_H_
