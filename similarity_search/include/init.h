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

#ifndef _INIT_H_
#define _INIT_H_

/*
 * This function should be called only *ONCE*,
 * but before actually using library functionality.
 */

#include "logging.h"

namespace similarity {

  void initLibrary(LogChoice choice = LIB_LOGNONE, const char*pLogFile = NULL);
}

#endif
