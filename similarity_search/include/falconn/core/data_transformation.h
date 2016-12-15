#ifndef __DATA_TRANSFORMATION_H__
#define __DATA_TRANSFORMATION_H__

#include <memory>

#include "../falconn_global.h"
#include "math_helpers.h"

namespace falconn {
namespace core {

class DataTransformationError : public FalconnError {
 public:
  DataTransformationError(const char* msg) : FalconnError(msg) {}
};

template <typename Point>
class IdentityTransformation {
 public:
  void apply(Point*) const { return; }
};

template <typename Point>
class NormalizingTransformation {
 public:
  void apply(Point* p) const {
    normalize(p);
    return;
  }
};

template <typename Point, typename DataStorage>
class CenteringTransformation {
 public:
  CenteringTransformation(const DataStorage& data) {
    typename DataStorage::FullSequenceIterator iter = data.get_full_sequence();
    if (!iter.is_valid()) {
      throw("Cannot center an empty data set.");
    }
    center_ = iter.get_point();
    int_fast64_t num_points = 1;
    ++iter;

    while (iter.is_valid()) {
      center_ += iter.get_point();
      num_points += 1;
      ++iter;
    }

    center_ /= num_points;
  }

  void apply(Point* p) const {
    *p -= center_;
    return;
  }

 private:
  Point center_;
};

// First applies Transformation2, then Transformation1.
template <typename Point, typename Transformation1, typename Transformation2>
class ComposedTransformation {
 public:
  ComposedTransformation(std::unique_ptr<Transformation1> transformation1,
                         std::unique_ptr<Transformation2> transformation2)
      : transformation1_(std::move(transformation1)),
        transformation2_(std::move(transformation2)) {}

  void apply(Point* p) const {
    transformation2_->apply(p);
    transformation1_->apply(p);
  }

 private:
  std::unique_ptr<Transformation1> transformation1_;
  std::unique_ptr<Transformation2> transformation2_;
};

}  // namespace core
}  // namespace falcon

#endif
