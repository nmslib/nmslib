#ifndef __HNSW_DISTFUNC_MV_H__
#define __HNSW_DISTFUNC_MV_H__
#include <stdio.h>
#include <math.h>
#include "conv-c2d/emax7.h"
#include "conv-c2d/emax7lib.h"

#if defined(ARMZYNQ) && defined(EMAX7)
#if __AARCH64EL__ == 1
typedef long double Dll;
#else
typedef struct { Ull u[2]; } Dll;
#endif
Uchar* sysinit(Uint memsize, Uint alignment);
Uchar* imax_alloc(Uint memsize, Uint alignment);
void imax_dealloc(Uint memsize, Uint alignment);
void imemcpy(Uint *dst, Uint *src, int words);
void xmax_bzero(Uint *dst, int words);
#endif

#if __cplusplus
extern "C" {
#endif
int imax_search_mv(float curdist, int curNodeNum, float *pVectq, int *data, size_t qty, size_t size, char *data_level0_memory_, size_t memoryPerObject_, size_t offsetData_);
#if __cplusplus
}
#endif
#endif