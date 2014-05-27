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

#include "space/space_dummy.h"
#include "experimentconf.h"

namespace similarity {

template <typename dist_t>
dist_t SpaceDummy<dist_t>::HiddenDistance(const Object* obj1, const Object* obj2) const {
  LOG(LIB_INFO) << "Calculating the distance between objects: " << obj1->id() << " and " << obj2->id() << endl;
  CHECK(obj1->datalength() > 0);
  CHECK(obj1->datalength() == obj2->datalength());
  /* 
   * Data in the object is accessed using functions:
   * obj1->datalength()
   * obj1->data();
   * obj2->datalength()
   * obj2->data();
   */
  return static_cast<dist_t>(0);
}

template <typename dist_t>
void SpaceDummy<dist_t>::ReadDataset(
    ObjectVector& dataset,
    const ExperimentConfig<dist_t>* config,
    const char* FileName,
    const int MaxNumObjects) const {
  LOG(LIB_INFO) << "Reading at most " << MaxNumObjects << " from the file: " << FileName;
}

template class SpaceDummy<int>;
template class SpaceDummy<float>;
template class SpaceDummy<double>;

}  // namespace similarity
