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

#include <memory>
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdint>

#include "utils.h"
#include "space/space_sparse_scalar_fast.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"

namespace similarity {

float 
SpaceSparseCosineSimilarityFast::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj2->datalength() > 0);

  float val = 1 - NormSparseScalarProductFast(obj1->data(), obj1->datalength(),
                                              obj2->data(), obj2->datalength());

  return max(val, float(0));
}

float 
SpaceSparseAngularDistanceFast::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj2->datalength() > 0);

  return acos(NormSparseScalarProductFast(obj1->data(), obj1->datalength(),
                                          obj2->data(), obj2->datalength()));
}

float
SpaceSparseNegativeScalarProductFast::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj2->datalength() > 0);

  return -SparseScalarProductFast(obj1->data(), obj1->datalength(),
                                  obj2->data(), obj2->datalength());

}

float
SpaceSparseQueryNormNegativeScalarProductFast::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj2->datalength() > 0);

  return -QueryNormSparseScalarProductFast(obj1->data(), obj1->datalength(),
                                           obj2->data(), obj2->datalength());

}


}  // namespace similarity
