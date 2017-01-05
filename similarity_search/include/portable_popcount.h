#pragma once


#ifdef _MSC_VER
#include <intrin.h>

#define  __builtin_popcount(t) __popcnt(t)

#endif
