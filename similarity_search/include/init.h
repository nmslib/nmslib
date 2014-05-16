/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
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

namespace similarity {


  /*
   *  If pLogFile is NULL, then stderr will be used
   */
  void initLibrary(const char*pLogFile);
}

#endif
