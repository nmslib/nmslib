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

#ifndef _SPACE_AB_DIVERG_H_
#define _SPACE_AB_DIVERG_H_

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

#define SPACE_AB_DIVERG     "ab_diverg"

namespace similarity {



/*
 * Alpha-beta divergence.
 * 
 * Póczos, Barnabás, Liang Xiong, Dougal J. Sutherland, and Jeff Schneider (2012). “Nonparametric kernel
 * estimators for image classification”. In: Computer Vision and Pattern Recognition (CVPR), 2012 IEEE
 * Conference on, pages 2989–2996
 */

template <typename dist_t>
class SpaceAlphaBetaDiverg : public VectorSpaceSimpleStorage<dist_t> {
 public:
  explicit SpaceAlphaBetaDiverg(float alpha, float beta) : alpha_(alpha), beta_(beta) {}
  virtual ~SpaceAlphaBetaDiverg() {}

  virtual std::string StrDesc() const;
 protected:
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const;
 private:
  DISABLE_COPY_AND_ASSIGN(SpaceAlphaBetaDiverg);
  float alpha_, beta_;
};


}  // namespace similarity

#endif 
