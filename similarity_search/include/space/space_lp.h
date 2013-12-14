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
#include "distcomp.h"

#define SPACE_L0  "l0"
#define SPACE_L1  "l1"
#define SPACE_L2  "l2"

namespace similarity {



template <typename dist_t>
class SpaceLpDist {
public:
  explicit SpaceLpDist(int p) : p_(p) {}

  dist_t operator()(const dist_t* x, const dist_t* y, size_t length) const {
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
  int getP() const { return p_; }
private:
  int p_;
};

template <typename dist_t>
class SpaceLp : public VectorSpace<dist_t> {
 public:
  explicit SpaceLp(int p) : distObj_(p) {}
  virtual ~SpaceLp() {}

  virtual std::string ToString() const;

 protected:
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const;
 private:
  SpaceLpDist<dist_t> distObj_;
};


}  // namespace similarity

#endif 
