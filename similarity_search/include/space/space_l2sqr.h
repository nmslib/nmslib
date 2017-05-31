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

#ifndef _SPACE_L2_SQR_H_
#define _SPACE_L2_SQR_H_

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

#define SPACE_L2_SQR   "l2sqr"

namespace similarity {


class SpaceL2SQR : public VectorSpaceSimpleStorage<float> {
 public:
  explicit SpaceL2SQR() {}
  virtual ~SpaceL2SQR() {}

  virtual std::string StrDesc() const { return SPACE_L2_SQR; }
 protected:
  virtual float HiddenDistance(const Object* obj1, const Object* obj2) const override {
    CHECK(obj1->datalength() > 0);
    CHECK(obj1->datalength() == obj2->datalength());
    const float* x = reinterpret_cast<const float*>(obj1->data());
    const float* y = reinterpret_cast<const float*>(obj2->data());
    const size_t length = obj1->datalength() / 4;

    return L2SqrSIMD(x, y, length);
  }
 private:
  DISABLE_COPY_AND_ASSIGN(SpaceL2SQR);
};


}  // namespace similarity

#endif 
