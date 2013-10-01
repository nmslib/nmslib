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

#include <cmath>
#include <fstream>
#include <string>
#include <sstream>

#include "space_lp.h"
#include "scoped_ptr.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"

namespace similarity {

template <typename dist_t>
dist_t LpSpace<dist_t>::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj1->datalength() == obj2->datalength());
  const dist_t* x = reinterpret_cast<const dist_t*>(obj1->data());
  const dist_t* y = reinterpret_cast<const dist_t*>(obj2->data());
  const size_t length = obj1->datalength() / sizeof(dist_t);

  CHECK(p_ >= 0);

  if (p_ == 0) {
    return LInfNormSIMD(x, y, length);
  } else if (p_ == 1) {
    return L1NormSIMD(x, y, length);
  } else if (p_ == 2) {
    return L2NormSIMD(x, y, length);
  } 

  // This one will be rather slow
  return LPGenericDistance(x, y, length, p_);
}

template <typename dist_t>
std::string LpSpace<dist_t>::ToString() const {
  std::stringstream stream;
  stream << "LpSpace: p = " << p_;
  return stream.str();
}

template class LpSpace<float>;
template class LpSpace<double>;

}  // namespace similarity
