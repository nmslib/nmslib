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

#include "init.h"

/*
 * Important note: there is some issue when compling using Intel.
 * It involves header inclusion order. If we include spaces first
 * it then somehow handicaps ability of 
 * boost/math/special_functions/detail/lanczos_sse2.hpp
 * to properly use SSE.
 */
#include "factory/init_methods.h"
#include "factory/init_spaces.h"

#include "logging.h"

namespace similarity {

void initLibrary(LogChoice choice, const char* pLogFile) {
  InitializeLogger(choice, pLogFile);
  initSpaces();
  initMethods();
}

}
