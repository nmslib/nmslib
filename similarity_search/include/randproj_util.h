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

#ifndef _RANDPROJ_UTILS_H_
#define _RANDPROJ_UTILS_H_

#include <vector>

#include "distcomp.h"

namespace similarity {

using std::vector;

template <class dist_t> void initRandProj(size_t nVect, size_t nElem, vector<vector<dist_t>>& res);

}


#endif
