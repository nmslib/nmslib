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

#ifndef _PERM_TYPE_H
#define _PERM_TYPE_H
namespace similarity {


/* 
 * Should be a (signed) int32_t type!!!
 * Otherwise, SIMD-based correlation functions won't work
 */
typedef int32_t PivotIdType;

typedef std::vector<PivotIdType> Permutation;


}
#endif
