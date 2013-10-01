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

#ifndef _SPACE_LP_H_
#define _SPACE_LP_H_

#include <string>
#include <map>
#include <stdexcept>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"
#include "space_vector.h"

#define SPACE_L0  "l0"
#define SPACE_L1  "l1"
#define SPACE_L2  "l2"

namespace similarity {

template <typename dist_t>
class LpSpace : public VectorSpace<dist_t> {
 public:
  explicit LpSpace(int p) : p_(p) {}
  virtual ~LpSpace() {}

  virtual std::string ToString() const;

 protected:
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const;
 private:
  int p_;
};


}  // namespace similarity

#endif 
