/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2016
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _SPACE_RENYI_DIVERG_H_
#define _SPACE_RENYI_DIVERG_H_

#include <string>
#include <limits>
#include <map>
#include <stdexcept>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"
#include "space_vector.h"
#include "distcomp.h"

#define SPACE_RENYI_DIVERG     "renyi_diverg"

namespace similarity {



/*
 * Renyi divergence.
 * 
 */

template <typename dist_t>
class SpaceRenyiDiverg : public VectorSpaceSimpleStorage<dist_t> {
 public:
  explicit SpaceRenyiDiverg(float alpha) : alpha_(alpha) {}
  virtual ~SpaceRenyiDiverg() {}

  virtual std::string StrDesc() const;
 protected:
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const;
 private:
  DISABLE_COPY_AND_ASSIGN(SpaceRenyiDiverg);
  float alpha_;
};


}  // namespace similarity

#endif 
