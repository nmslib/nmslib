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

#include <cmath>
#include <fstream>
#include <string>
#include <sstream>

#include "space/space_scalar.h"
#include "logging.h"
#include "experimentconf.h"
#include "my_isnan_isinf.h"

namespace similarity {

template <typename dist_t>
dist_t SpaceCosineSimilarity<dist_t>::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj1->datalength() == obj2->datalength());
  const dist_t* x = reinterpret_cast<const dist_t*>(obj1->data());
  const dist_t* y = reinterpret_cast<const dist_t*>(obj2->data());
  const size_t length = obj1->datalength() / sizeof(dist_t);

  dist_t val = CosineSimilarity(x, y, length);
  // TODO: @leo shouldn't happen any more, but let's keep this check here for a while
  if (my_isnan(val)) throw runtime_error("Bug: NAN dist! (SpaceCosineSimilarity)");
  return val;
}

template class SpaceCosineSimilarity<float>;
template class SpaceCosineSimilarity<double>;

template <typename dist_t>
dist_t SpaceAngularDistance<dist_t>::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj1->datalength() == obj2->datalength());
  const dist_t* x = reinterpret_cast<const dist_t*>(obj1->data());
  const dist_t* y = reinterpret_cast<const dist_t*>(obj2->data());
  const size_t length = obj1->datalength() / sizeof(dist_t);

  dist_t val = AngularDistance(x, y, length);
  // TODO: @leo shouldn't happen any more, but let's keep this check here for a while
  if (my_isnan(val)) throw runtime_error("Bug: NAN dist! (SpaceAngularDistance)");
  return val;
}

template class SpaceAngularDistance<float>;
template class SpaceAngularDistance<double>;

template <typename dist_t>
dist_t SpaceNegativeScalarProduct<dist_t>::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj1->datalength() == obj2->datalength());
  const dist_t* x = reinterpret_cast<const dist_t*>(obj1->data());
  const dist_t* y = reinterpret_cast<const dist_t*>(obj2->data());
  const size_t length = obj1->datalength() / sizeof(dist_t);

  return -ScalarProductSIMD(x, y, length);
}

template class SpaceNegativeScalarProduct<float>;
template class SpaceNegativeScalarProduct<double>;

}  // namespace similarity
