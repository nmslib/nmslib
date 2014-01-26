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

#ifndef _SPACE_SPARSE_SCALAR_H_
#define _SPACE_SPARSE_SCALAR_H_

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

#define SPACE_SPARSE_COSINE_SIMILARITY  "cosinesimil_sparse"
#define SPACE_SPARSE_ANGULAR_DISTANCE   "angulardist_sparse"

namespace similarity {

template <typename dist_t>
class SpaceSparseAngularDistance : public SpaceSparseVector<dist_t> {
 public:
  explicit SpaceSparseAngularDistance() {}
  virtual ~SpaceSparseAngularDistance() {}

  virtual std::string ToString() const {
    return "AngularDistance";
  }

 protected:
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const {
    return SpaceSparseVector<dist_t>::ComputeDistanceHelper(obj1, obj2, distObjAngular_);
  }
 private:
  struct SpaceAngularDist {
    dist_t operator()(const dist_t* x, const dist_t* y, size_t length) const {
      dist_t val = AngularDistance(x, y, length);
      // TODO: @leo shouldn't happen any more, but let's keep this check here for a while
      if (std::isnan(val)) LOG(FATAL) << "Bug: NAN dist!!!!";
      return val;
    }
  };
  SpaceAngularDist        distObjAngular_;
};

template <typename dist_t>
class SpaceSparseCosineSimilarity : public SpaceSparseVector<dist_t> {
 public:
  explicit SpaceSparseCosineSimilarity() {}
  virtual ~SpaceSparseCosineSimilarity() {}

  virtual std::string ToString() const {
    return "CosineSimilarity";
  }

 protected:
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const {
    return SpaceSparseVector<dist_t>::ComputeDistanceHelper(obj1, obj2, distObjCosineSimilarity_);
  }
 private:
  struct SpaceCosineSimilarityDist {
    dist_t operator()(const dist_t* x, const dist_t* y, size_t length) const {
      dist_t val = CosineSimilarity(x, y, length);
      // TODO: @leo shouldn't happen any more, but let's keep this check here for a while
      if (std::isnan(val)) LOG(FATAL) << "Bug: NAN dist!!!!";
      return val;
    }
  };
  SpaceCosineSimilarityDist        distObjCosineSimilarity_;
};


}  // namespace similarity

#endif 
