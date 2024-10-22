#pragma once
#include <stdio.h>
#include <math.h>
int LANE = 1;
#include "conv-c2d/emax7.h"
#include "conv-c2d/emax7lib.h"

#if defined(ARMZYNQ) && defined(EMAX7)
#if __AARCH64EL__ == 1
typedef long double Dll;
#else
typedef struct { Ull u[2]; } Dll;
#endif

Uchar* membase = NULL;
Uchar* prev = NULL;

Uchar* sysinit(Uint memsize, Uint alignment) {
#if defined(ARMZYNQ) && defined(EMAX7)
    if (emax7_open(1) == NULL) exit(1);
    membase = emax_info[0].ddr_mmap;
    {int i;for (i = 0; i < (memsize + sizeof(Dll) - 1) / sizeof(Dll); i++)*((Dll *)membase + i) = 0;}
    prev = (Uchar *)((Dll *)membase + (memsize + sizeof(Dll) - 1) / sizeof(Dll));
#elif __linux__ == 1
    posix_memalign((void**)&membase, alignment, memsize);
#else
    membase = (void *)malloc(memsize + alignment);
    prev = membase;
    if ((Ull)membase & (Ull)(alignment - 1)) {membase = (void *)(((Ull)membase & ~(Ull)(alignment - 1)) + alignment);}
#endif

#if !defined(ARMZYNQ) && defined(EMAX7)
    emax_info[0].dma_phys = DMA_BASE2_PHYS; /* defined in emax7lib.h */
    emax_info[0].dma_mmap = emax_info[0].dma_phys;
    emax_info[0].reg_phys = REG_BASE2_PHYS; /* defined in emax7lib.h */
    emax_info[0].reg_mmap = emax_info[0].reg_phys;
    emax_info[0].lmm_phys = LMM_BASE2_PHYS;
    emax_info[0].lmm_mmap = emax_info[0].lmm_phys;
    emax_info[0].ddr_phys = membase;
    emax_info[0].ddr_mmap = emax_info[0].ddr_phys;
#endif
#if (defined(ARMSIML) || defined(ARMZYNQ)) && defined(EMAX6)
    emax6.dma_ctrl = emax_info.dma_mmap;
    emax6.reg_ctrl = emax_info.reg_mmap;
    ((struct reg_ctrl *)emax6.reg_ctrl)->i[0].cmd = CMD_RESET;
#if defined(ARMZYNQ)
    usleep(1);
#endif
    switch (((struct reg_ctrl *)emax6.reg_ctrl)->i[0].stat >> 8 & 0xf) {
    case 3:
        EMAX_DEPTH = 64;
        break;
    case 2:
        EMAX_DEPTH = 32;
        break;
    case 1:
        EMAX_DEPTH = 16;
        break;
    default:
        EMAX_DEPTH = 8;
        break;
    }
    ((struct reg_ctrl *)emax6.reg_ctrl)->i[0].adtr = emax_info.ddr_mmap - emax_info.lmm_phys;
    ((struct reg_ctrl *)emax6.reg_ctrl)->i[0].dmrp = 0LL;
#endif
#if (defined(ARMSIML) || defined(ARMZYNQ)) && defined(EMAX7)
    emax7[0].dma_ctrl = emax_info[0].dma_mmap;
    emax7[0].reg_ctrl = emax_info[0].reg_mmap;
    ((struct reg_ctrl *)emax7[0].reg_ctrl)->i[0].cmd = CMD_RESET;
#if defined(ARMZYNQ)
    usleep(1);
#endif
    switch (((struct reg_ctrl *)emax7[0].reg_ctrl)->i[0].stat >> 8 & 0xf) {
    case 3:
        EMAX_DEPTH = 64;
        break;
    case 2:
        EMAX_DEPTH = 32;
        break;
    case 1:
        EMAX_DEPTH = 16;
        break;
    default:
        EMAX_DEPTH = 8;
        break;
    }
    ((struct reg_ctrl *)emax7[0].reg_ctrl)->i[0].adtr = emax_info[0].ddr_mmap - emax_info[0].lmm_phys;
    ((struct reg_ctrl *)emax7[0].reg_ctrl)->i[0].dmrp = 0LL;
#endif
    return membase;
}

Uchar* imax_alloc(Uint memsize, Uint alignment) {
    if (membase == NULL) {return sysinit(memsize, alignment);}
    else {
#if defined(ARMZYNQ) && defined(EMAX7)
        membase = prev;
        {int i; for (i=0; i<(memsize+sizeof(Dll)-1)/sizeof(Dll); i++) *((Dll*)prev+i)=0;}
        prev = (Uchar*)&((Dll*)prev)[((memsize+sizeof(Dll)-1)/sizeof(Dll))];
#elif __linux__ == 1
        posix_memalign((void**)&membase, alignment, memsize);
#else
        membase = (void*)malloc(memsize+alignment);
        prev = membase;
        if ((Ull)membase & (Ull)(alignment-1)) {membase = (void*)(((Ull)membase & ~(Ull)(alignment-1))+alignment);}
#endif
        return membase;
    }
}

void imax_dealloc(Uint memsize, Uint alignment) {
    if (membase != NULL) {
#if defined(ARMZYNQ) && defined(EMAX7)
        prev = (Uchar*)&((Dll*)prev)[((memsize+sizeof(Dll)-1)/sizeof(Dll))];
#endif
    }
}

void imemcpy(Uint *dst, Uint *src, int words) {
    union {
        Uint i[4];
        Ull l[2];
        Dll d;
    } buf;

    Uint loop, i;
    if (words >= 1 && ((Ull)dst & sizeof(Uint))) { /* 4B-access odd */
        *dst++ = *src++;
        words--;
    }
    if (words >= 2 && ((Ull)dst & sizeof(Ull))) { /* 8B-access odd */
        if ((Ull)src & sizeof(Uint)) {
            buf.i[0] = *src++;
            buf.i[1] = *src++;
            *(Ull *)dst = buf.l[0];
        } else {
            *(Ull *)dst = *(Ull *)src;
            src += sizeof(Ull) / sizeof(Uint);
        }
        dst += sizeof(Ull) / sizeof(Uint);
        words -= 2;
    }

    if (loop = words / (sizeof(Dll) / sizeof(Uint))) {
        if ((Ull)src & sizeof(Uint)) {
            for (i = 0; i < loop; i++) {
                buf.i[0] = *src++;
                buf.i[1] = *src++;
                buf.i[2] = *src++;
                buf.i[3] = *src++;
                *(Dll *)dst = buf.d;
                dst += sizeof(Dll) / sizeof(Uint);
            }
        } else if ((Ull)src & sizeof(Ull)) {
            for (i = 0; i < loop; i++) {
                buf.l[0] = *(Ull *)src;
                src += sizeof(Ull) / sizeof(Uint);
                buf.l[1] = *(Ull *)src;
                src += sizeof(Ull) / sizeof(Uint);
                *(Dll *)dst = buf.d;
                dst += sizeof(Dll) / sizeof(Uint);
            }
        } else {
            for (i = 0; i < loop; i++) {
                *(Dll *)dst = *(Dll *)src;
                src += sizeof(Dll) / sizeof(Uint);
                dst += sizeof(Dll) / sizeof(Uint);
            }
        }
        words -= loop * (sizeof(Dll) / sizeof(Uint));
    }

    if (words >= 2) { /* 8B-access */
        if ((Ull)src & sizeof(Uint)) {
            buf.i[0] = *src++;
            buf.i[1] = *src++;
            *(Ull *)dst = buf.l[0];
        } else {
            *(Ull *)dst = *(Ull *)src;
            src += sizeof(Ull) / sizeof(Uint);
        }
        dst += sizeof(Ull) / sizeof(Uint);
        words -= 2;
    }
    if (words >= 1) { /* 4B-access */
        *dst++ = *src++;
        words--;
    }
}

void xmax_bzero(Uint *dst, int words) {
    Uint loop, i;
    if (words >= 1 && ((Ull)dst & sizeof(Uint))) { /* 4B-access odd */
        *dst++ = 0;
        words--;
    }
    if (words >= 2 && ((Ull)dst & sizeof(Ull))) { /* 8B-access odd */
        *(Ull *)dst = 0;
        dst += sizeof(Ull) / sizeof(Uint);
        words -= 2;
    }

    if (loop = words / (sizeof(Dll) / sizeof(Uint))) {
        for (i = 0; i < loop; i++) {
#if __AARCH64EL__ == 1
            *((Dll *)dst) = 0;
#else
            ((Dll *)dst)->u[0] = 0;
            ((Dll *)dst)->u[1] = 0;
#endif
            dst += sizeof(Dll) / sizeof(Uint);
        }
        words -= loop * (sizeof(Dll) / sizeof(Uint));
    }

    if (words >= 2) { /* 8B-access */
        *(Ull *)dst = 0;
        dst += sizeof(Ull) / sizeof(Uint);
        words -= 2;
    }
    if (words >= 1) { /* 4B-access */
        *dst++ = 0;
        words--;
    }
}

#endif

int imax_search_mv(float curdist, int curNodeNum, float *pVectq, int *data, size_t qty, size_t size, char *data_level0_memory_, size_t memoryPerObject_, size_t offsetData_);
