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

#ifndef FACTORY_SPACE_QA1_H
#define FACTORY_SPACE_QA1_H

#include <space/qa/space_qa1.h>

namespace similarity {

/*
 * Creating functions.
 */

Space<float>* CreateQA1(const AnyParams& AllParams) {
  AnyParamManager pmgr(AllParams);

  // No parameters

  pmgr.CheckUnused();

  return new SpaceQA1();
}

/*
 * End of creating functions.
 */

}

#endif
