/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan, Leonid Boytsov.
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

/* 
 * Note that __GNUC__ is also defined for Intel and Clang,
 * which do understand __attribute__ ((aligned(16)))
 */

#ifndef SIMDUTILS_H
#define SIMDUTILS_H

#if defined(__GNUC__)
#define PORTABLE_ALIGN16 __attribute__((aligned(16)))
#else
#define PORTABLE_ALIGN16 __declspec(align(16))
#endif

// On Win64 SSE2 is always enabled
// http://stackoverflow.com/questions/1067630/sse2-option-in-visual-c-x64
#if defined(__SSE2__) || defined(__AVX__) || defined(_MSC_VER)
#define PORTABLE_SSE2
#endif

// Unfortunately on Win32/64, there is not separate option for SSE4
#if defined(__SSE4_2__) || defined(__AVX__)
#define PORTABLE_SSE4
#endif


#ifdef PORTABLE_SSE4
#include <simd.h>


/*
 * Based on explanations/suggestions from here
 * http://stackoverflow.com/questions/5526658/intel-sse-why-does-mm-extract-ps-return-int-instead-of-float
 */

#define MM_EXTRACT_DOUBLE(v,i) _mm_cvtsd_f64(_mm_shuffle_pd(v, v, _MM_SHUFFLE2(0, i)))
#define MM_EXTRACT_FLOAT(v,i) _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, i)))


 /*
 * However, if we need to extract many numbers to sum the up, it is more efficient no
 * not to use the above MM_EXTRACT_FLOAT (https://github.com/searchivarius/BlogCode/tree/master/2016/bench_sums)
 *
 */

#endif



#ifdef _MSC_VER
#include <intrin.h>

#define  __builtin_popcount(t) __popcnt(t)

#endif


#endif
