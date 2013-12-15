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

#ifndef _SPACE_SPARSE_COSINE_H_
#define _SPACE_SPARSE_COSINE_H_

#include <string>
#include <map>
#include <cmath>
#include <stdexcept>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"
#include "space_sparse_vector.h"
#include "distcomp.h"

#define SPACE_SPARSE_COSINE  "cosine_sparse"

namespace similarity {

template <typename dist_t>
class SpaceSparseCosine : public SpaceSparseVector<dist_t> {
 public:
  explicit SpaceSparseCosine() {}
  virtual ~SpaceSparseCosine() {}

  virtual std::string ToString() const {
    return "CosineDistance";
  }

 protected:
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const {
    return SpaceSparseVector<dist_t>::ComputeDistanceHelper(obj1, obj2, distObjCosine_);
  }
 private:
  struct SpaceCosineDist {
    dist_t operator()(const dist_t* x, const dist_t* y, size_t length) const {
      dist_t val = CosineDistance(x, y, length);
      // TODO: @leo shouldn't happen any more, but let's keep this check here for a while
      if (std::isnan(val)) LOG(FATAL) << "NAN dist!!!!";
      return val;
    }
  };
  SpaceCosineDist        distObjCosine_;
};


}  // namespace similarity

#endif 
