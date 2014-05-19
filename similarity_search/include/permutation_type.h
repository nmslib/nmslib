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

#ifndef _PERM_TYPE_H
#define _PERM_TYPE_H

#include <vector>
#include <cstdint>

namespace similarity {


/* 
 * Should be a (signed) int32_t type!!!
 * We don't use integer of a smaller range, due to possible overflow.
 *
 * Don't change it, or the code will be broken.
 * In particular, SIMD-based correlation functions won't work
 */
typedef int32_t PivotIdType;

typedef std::vector<PivotIdType> Permutation;


}
#endif
