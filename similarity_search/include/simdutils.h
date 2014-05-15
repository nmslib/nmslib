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

#ifdef __SSE4_2__
#include <immintrin.h>
#include <smmintrin.h>
#include <tmmintrin.h>


/*
 * Based on explanations/suggestions from here
 * http://stackoverflow.com/questions/5526658/intel-sse-why-does-mm-extract-ps-return-int-instead-of-float
 */

#define MM_EXTRACT_DOUBLE(v,i) _mm_cvtsd_f64(_mm_shuffle_pd(v, v, _MM_SHUFFLE2(0, i)))
#define MM_EXTRACT_FLOAT(v,i) _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, i)))

#endif
