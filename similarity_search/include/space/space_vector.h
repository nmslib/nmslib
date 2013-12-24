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

#ifndef _SPACE_VECTOR_H_
#define _SPACE_VECTOR_H_

#include <string>
#include <map>
#include <stdexcept>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"

namespace similarity {

template <typename dist_t>
class VectorSpace : public Space<dist_t> {
 public:
  virtual ~VectorSpace() {}

  virtual void ReadDataset(ObjectVector& dataset,
                      const ExperimentConfig<dist_t>* config,
                      const char* inputfile,
                      const int MaxNumObjects) const;
  virtual Object* CreateObjFromVect(size_t id, const std::vector<dist_t>& InpVect) const;
 protected:
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const = 0;
  void ReadVec(std::string line, std::vector<dist_t>& v) const;
};

}  // namespace similarity

#endif
