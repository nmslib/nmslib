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

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static const int kMaxFilenameLength = 255;

#define ARRAY_SIZE(x) (static_cast<int>(sizeof(x)/sizeof(*x)))

#ifdef __GNUC__
#define DEPRECATED(func)  __attribute__ ((deprecated)) func
#elif defined(_MSC_VER)
#define DEPRECATED(func)  __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func)  func
#endif

namespace similarity {
/*
 * disable copy and assign
 * Let's not do it on Windows
 * as it generates way to many warnings C4661
 */
#if defined(_MSC_VER)
#undef DISABLE_COPY_AND_ASSIGN
#define DISABLE_COPY_AND_ASSIGN(Type) 
#else
#undef DISABLE_COPY_AND_ASSIGN
#define DISABLE_COPY_AND_ASSIGN(Type) \
  explicit Type(const Type&); \
  Type& operator=(const Type&)
#endif

}   // namespace similarity

#endif    // _GLOBAL_H_

