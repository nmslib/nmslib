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

#ifndef _SPACE_SPARSE_SCALAR_FAST_H_
#define _SPACE_SPARSE_SCALAR_FAST_H_

#include <string>
#include <limits>
#include <map>
#include <stdexcept>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"
#include "space_sparse_vector_inter.h"
#include "distcomp.h"


#define SPACE_SPARSE_COSINE_SIMILARITY_FAST  "cosinesimil_sparse_fast"
#define SPACE_SPARSE_ANGULAR_DISTANCE_FAST   "angulardist_sparse_fast"

#define SPACE_SPARSE_NEGATIVE_SCALAR_FAST             "negdotprod_sparse_fast"
#define SPACE_SPARSE_QUERY_NORM_NEGATIVE_SCALAR_FAST  "querynorm_negdotprod_sparse_fast"


namespace similarity {

class SpaceSparseCosineSimilarityFast : public SpaceSparseVectorInter<float> {
public:
  explicit SpaceSparseCosineSimilarityFast(){}
  virtual std::string StrDesc() const {
    return "CosineSimilarity (fast)";
  }
protected:
  virtual float HiddenDistance(const Object* obj1, const Object* obj2) const;
  DISABLE_COPY_AND_ASSIGN(SpaceSparseCosineSimilarityFast);
};

class SpaceSparseAngularDistanceFast : public SpaceSparseVectorInter<float> {
public:
  explicit SpaceSparseAngularDistanceFast(){}
  virtual std::string StrDesc() const {
    return "AngularDistance (fast)";
  }
protected:
  virtual float HiddenDistance(const Object* obj1, const Object* obj2) const;
  DISABLE_COPY_AND_ASSIGN(SpaceSparseAngularDistanceFast);
};

class SpaceSparseNegativeScalarProductFast : public SpaceSparseVectorInter<float> {
public:
  explicit SpaceSparseNegativeScalarProductFast(){}
  virtual std::string StrDesc() const {
    return "NegativeScalarProduct (fast)";
  }
protected:
  virtual float HiddenDistance(const Object* obj1, const Object* obj2) const;
  DISABLE_COPY_AND_ASSIGN(SpaceSparseNegativeScalarProductFast);
};

class SpaceSparseQueryNormNegativeScalarProductFast : public SpaceSparseVectorInter<float> {
public:
  explicit SpaceSparseQueryNormNegativeScalarProductFast(){}
  virtual std::string StrDesc() const {
    return "QueryNormNegativeScalarProduct (fast)";
  }
protected:
  virtual float HiddenDistance(const Object* obj1, const Object* obj2) const;
  DISABLE_COPY_AND_ASSIGN(SpaceSparseQueryNormNegativeScalarProductFast);
};


}  // namespace similarity

#endif 
