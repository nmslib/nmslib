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

#include "space_sparse_lp.h"
#include "space_sparse_vector.h"
#include "scoped_ptr.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"

namespace similarity {

template <typename dist_t>
dist_t SpaceSparseLp<dist_t>::HiddenDistance(const Object* obj1, const Object* obj2) const {
  return SpaceSparseVector<dist_t>::ComputeDistanceHelper(obj1, obj2, distObj_);
}

template <typename dist_t>
std::string SpaceSparseLp<dist_t>::ToString() const {
  std::stringstream stream;
  stream << "SpaceSparseLp: p = " << distObj_.getP();
  return stream.str();
}

template class SpaceSparseLp<float>;
template class SpaceSparseLp<double>;

}  // namespace similarity
