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

#define SPACE_JS_SLOW         "jsslow"
#define SPACE_JS_FAST         "jsfast"
#define SPACE_JS_FAST_APPROX  "jsfastapprox"

namespace similarity {

template <typename dist_t>
class JSSpace : public VectorSpace<dist_t> {
 public:
  enum JSType {kJSSlow, kJSFastPrecomp, kJSFastPrecompApprox};

  explicit JSSpace(JSType type) : type_(type) {}
  virtual ~JSSpace() {}

  virtual std::string ToString() const;
  virtual Object* CreateObjFromVect(size_t id, const std::vector<dist_t>& InpVect) const;

 protected:
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const;
 private:
  JSType   type_;
};


}  // namespace similarity

#endif 
