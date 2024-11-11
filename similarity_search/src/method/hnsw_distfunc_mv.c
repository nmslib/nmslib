#include "method/hnsw_distfunc_mv.h"
#include <stdio.h>

#ifdef EMAX7
#ifdef ARMZYNQ
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

#define IMAX_KERNEL_COL_SIZE 56
#define IMAX_KERNEL_ROW_SIZE 4
#define NCHIP 1

int imax_search_mv(float *curdist, int *curNodeNum, float *pVectq, int *data, size_t qty, size_t size, char *data_level0_memory_, size_t memoryPerObject_, size_t offsetData_) {
    int LANE = 0;
    int imax_emb = qty % (IMAX_KERNEL_COL_SIZE*2) ? ((qty/(IMAX_KERNEL_COL_SIZE*2)) + 1)*IMAX_KERNEL_COL_SIZE*2 : qty;
    int imax_size = size % 32 ? ((size/32) + 1)*32 : size;
    #ifdef ARMZYNQ
    if (membase == NULL) {sysinit(1024*1024*128*sizeof(float), 32);}
    int key_size = imax_size*imax_emb*sizeof(float);
    int query_size = imax_emb*sizeof(float);
    int result_size = imax_size*sizeof(float);
    // 多分ここで問題発生してる
    float *imax_key_array = (float *)membase;
    float *imax_query_array = imax_key_array + (imax_size*imax_emb);
    float *imax_result_array = imax_query_array + imax_emb;
    #else
    float imax_key_array[imax_size*imax_emb];
    float imax_query_array[imax_emb];
    float imax_result_array[imax_size];
    #endif
    // printf("imax_key_array=%p, imax_query_array=%p, imax_result_array=%p\n", imax_key_array, imax_query_array, imax_result_array);
    // printf("imax_key_size=%d, imax_query_size=%d, imax_result_size=%d\n", imax_size*imax_emb, imax_emb, imax_size);
    int changed = 0;
    for (int j = 0; j < imax_emb; j++) {
        if (j < qty) {
            imax_query_array[j] = pVectq[j];
        } else {
            imax_query_array[j] = 0;
        }
    }
    for (int j = 1; j <= imax_size; j++) {
        if (j <= size) {
            int tnum = *(data + j);
            for (int k = 0; k < imax_emb; k++) {
                if (k < qty) {
                    imax_key_array[(j-1)*imax_emb + k] = *(data_level0_memory_ + tnum * memoryPerObject_ + offsetData_ + 16 + k);
                } else {
                    imax_key_array[(j-1)*imax_emb + k] = 0;
                }
            }
        } else {
            for (int k = 0; k < imax_emb; k++) {
                imax_key_array[(j-1)*imax_emb + k] = 0;
            }
        }
        imax_result_array[j-1] = 0;
    }

    printf("imax_search_mv: imax_emb=%d, imax_size=%d\n", imax_emb, imax_size);
    for (int row_blk_idx = 0; row_blk_idx*IMAX_KERNEL_ROW_SIZE < imax_size; row_blk_idx++) {
        Ull qaddr[IMAX_KERNEL_COL_SIZE];
        Ull kaddr[IMAX_KERNEL_COL_SIZE*4];
        Ull raddr[4];
        for (int k = 0; k < IMAX_KERNEL_COL_SIZE; k++) {
            for (int j = 0; j < 4; j++) {
                kaddr[j + k*4] = ((Ull)imax_key_array) + ((row_blk_idx*IMAX_KERNEL_ROW_SIZE)+j)*imax_emb*4 + k*8;
            }
            qaddr[k] = ((Ull)imax_query_array) + k*8;
        }
        for (int j = 0; j < 4; j++) {
            raddr[j] = ((Ull)imax_result_array) + (IMAX_KERNEL_ROW_SIZE*row_blk_idx)*4 + j*4;
        }
        printf("qaddr=%p, kaddr[0]=%p, raddr[0]=%p %d\n", qaddr, kaddr[0], raddr[0], row_blk_idx);
        printf("qaddr_float=%f, %f, %f, %f\n", *(float *)qaddr[0], *(float *)qaddr[1], *(float *)qaddr[2], *(float *)qaddr[3]);
        printf("kaddr_float=%f, %f, %f, %f\n", *(float *)kaddr[0], *(float *)kaddr[1], *(float *)kaddr[2], *(float *)kaddr[3]);

        Ull CHIP, LOLP, INIT0, INIT1, LOOP0, LOOP1;
        Ull cofs, rofs, oofs;
        Ull fetch_size = imax_emb * IMAX_KERNEL_ROW_SIZE * 8;
        Ull rofs_init = ((0-IMAX_KERNEL_ROW_SIZE*4LL)<<32)|((0-IMAX_KERNEL_ROW_SIZE*imax_emb*4LL)&0xFFFFFFFF);
        Ull cofs_init = 0<<32|((0-IMAX_KERNEL_COL_SIZE*8LL)&0xFFFFFFFF);
        Ull rofs_add = (IMAX_KERNEL_ROW_SIZE*4LL<<32)|((IMAX_KERNEL_ROW_SIZE*imax_emb*4LL)&0xFFFFFFFF);
        Ull cofs_add = 0<<32|((IMAX_KERNEL_COL_SIZE*8LL)&0xffffffff);
        Ull BR[64][4][4], AR[64][4];

#define mv1_core(r, rm1, k0, k1, k2, k3, q) \
                    mop(OP_LDR,  3, &BR[rm1][0][1], (Ull)kaddr[k0], (Ull)cofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size); \
                    mop(OP_LDR,  3, &BR[rm1][0][0], (Ull)kaddr[k1], (Ull)cofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size); \
                    mop(OP_LDR,  3, &BR[rm1][1][1], (Ull)kaddr[k2], (Ull)cofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size); \
                    mop(OP_LDR,  3, &BR[rm1][1][0], (Ull)kaddr[k3], (Ull)cofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size); \
                    mop(OP_LDR,  3, &BR[rm1][2][1], (Ull)qaddr[q],  (Ull)cofs, MSK_W0, (Ull)qaddr[0],  imax_emb,   0, 0, (Ull)NULL, imax_emb  ); \
                    exe(OP_FMA, &AR[r][3], AR[rm1][3], EXP_H3210, BR[rm1][0][1], EXP_H3210, BR[rm1][2][1], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                    exe(OP_FMA, &AR[r][2], AR[rm1][2], EXP_H3210, BR[rm1][0][0], EXP_H3210, BR[rm1][2][1], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                    exe(OP_FMA, &AR[r][1], AR[rm1][1], EXP_H3210, BR[rm1][1][1], EXP_H3210, BR[rm1][2][1], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                    exe(OP_FMA, &AR[r][0], AR[rm1][0], EXP_H3210, BR[rm1][1][0], EXP_H3210, BR[rm1][2][1], EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL)

#define mv1_store(r, rm1) \
                    mop(OP_LDWR, 3, &BR[rm1][0][1], (Ull)raddr[0], (Ull)oofs, MSK_W0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, imax_size); \
                    mop(OP_LDWR, 3, &BR[rm1][0][0], (Ull)raddr[1], (Ull)oofs, MSK_W0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, imax_size); \
                    mop(OP_LDWR, 3, &BR[rm1][1][1], (Ull)raddr[2], (Ull)oofs, MSK_W0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, imax_size); \
                    mop(OP_LDWR, 3, &BR[rm1][1][0], (Ull)raddr[3], (Ull)oofs, MSK_W0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, imax_size); \
                    exe(OP_FAD, &AR[r][3], BR[rm1][0][1], EXP_H1010, AR[rm1][3], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                    exe(OP_FAD, &AR[r][2], BR[rm1][0][0], EXP_H1010, AR[rm1][2], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                    exe(OP_FAD, &AR[r][1], BR[rm1][1][1], EXP_H1010, AR[rm1][1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                    exe(OP_FAD, &AR[r][0], BR[rm1][1][0], EXP_H1010, AR[rm1][0], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL); \
                    mop(OP_STWR, 3, &AR[r][3], (Ull)oofs, (Ull)raddr[0], MSK_D0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, imax_size); \
                    mop(OP_STWR, 3, &AR[r][2], (Ull)oofs, (Ull)raddr[1], MSK_D0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, imax_size); \
                    mop(OP_STWR, 3, &AR[r][1], (Ull)oofs, (Ull)raddr[2], MSK_D0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, imax_size); \
                    mop(OP_STWR, 3, &AR[r][0], (Ull)oofs, (Ull)raddr[3], MSK_D0, (Ull)raddr[0], imax_size, 0, 0, (Ull)NULL, imax_size)

//EMAX5A begin mv1 mapdist=0
        for (CHIP=0;CHIP<NCHIP;CHIP++) {
            for (INIT1=1,LOOP1=IMAX_KERNEL_ROW_SIZE/4,rofs=rofs_init;LOOP1--;INIT1=0) {
                for (INIT0=1,LOOP0=imax_emb/(IMAX_KERNEL_COL_SIZE*2),cofs=cofs_init;LOOP0--;INIT0=0) {
                    exe(OP_ADD, &cofs, INIT0?cofs:cofs, EXP_H3210, cofs_add, EXP_H3210, 0LL, EXP_H3210, OP_AND, 0xffffffffffffffffLL, OP_NOP, 0LL);
                    exe(OP_ADD, &rofs, rofs, EXP_H3210, INIT0?rofs_add:0, EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                    exe(OP_ADD, &oofs, rofs, EXP_H3232, 0LL, EXP_H3210, 0LL, EXP_H3210, OP_AND, 0xffffffffLL, OP_NOP, 0LL);

                    mop(OP_LDR,  3, &BR[1][0][1], (Ull)kaddr[0], (Ull)cofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size);
                    mop(OP_LDR,  3, &BR[1][0][0], (Ull)kaddr[1], (Ull)cofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size);
                    mop(OP_LDR,  3, &BR[1][1][1], (Ull)kaddr[2], (Ull)cofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size);
                    mop(OP_LDR,  3, &BR[1][1][0], (Ull)kaddr[3], (Ull)cofs, MSK_W0, (Ull)kaddr[0], fetch_size, 0, 0, (Ull)NULL, fetch_size);
                    mop(OP_LDR,  3, &BR[1][2][1], (Ull)qaddr[0], (Ull)cofs, MSK_W0, (Ull)qaddr[0], imax_emb  , 0, 0, (Ull)NULL, imax_emb  );

                    exe(OP_FML, &AR[2][3], BR[1][0][1], EXP_H3210, BR[1][2][1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                    exe(OP_FML, &AR[2][2], BR[1][0][0], EXP_H3210, BR[1][2][1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                    exe(OP_FML, &AR[2][1], BR[1][0][1], EXP_H3210, BR[1][2][1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                    exe(OP_FML, &AR[2][0], BR[1][0][0], EXP_H3210, BR[1][2][1], EXP_H3210, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);

                    mv1_core( 3,  2,  4,  5,  6,  7,  1);mv1_core( 4,  3,  8,  9, 10, 11,  2);
                    mv1_core( 5,  4, 12, 13, 14, 15,  3);mv1_core( 6,  5, 16, 17, 18, 19,  4);
                    mv1_core( 7,  6, 20, 21, 22, 23,  5);mv1_core( 8,  7, 24, 25, 26, 27,  6);
                    mv1_core( 9,  8, 28, 29, 30, 31,  7);mv1_core(10,  9, 32, 33, 34, 35,  8);
                    mv1_core(11, 10, 36, 37, 38, 39,  9);mv1_core(12, 11, 40, 41, 42, 43, 10);
                    mv1_core(13, 12, 44, 45, 46, 47, 11);mv1_core(14, 13, 48, 49, 50, 51, 12);
                    mv1_core(15, 14, 52, 53, 54, 55, 13);mv1_core(16, 15, 56, 57, 58, 59, 14);
                    mv1_core(17, 16, 60, 61, 62, 63, 15);mv1_core(18, 17, 64, 65, 66, 67, 16);
                    mv1_core(19, 18, 68, 69, 70, 71, 17);mv1_core(20, 19, 72, 73, 74, 75, 18);
                    mv1_core(21, 20, 76, 77, 78, 79, 19);mv1_core(22, 21, 80, 81, 82, 83, 20);
                    mv1_core(23, 22, 84, 85, 86, 87, 21);mv1_core(24, 23, 88, 89, 90, 91, 22);
                    mv1_core(25, 24, 92, 93, 94, 95, 23);mv1_core(26, 25, 96, 97, 98, 99, 24);
                    mv1_core(27, 26,100,101,102,103, 25);mv1_core(28, 27,104,105,106,107, 26);
                    mv1_core(29, 28,108,109,110,111, 27);mv1_core(30, 29,112,113,114,115, 28);
                    mv1_core(31, 30,116,117,118,119, 29);mv1_core(32, 31,120,121,122,123, 30);
                    mv1_core(33, 32,124,125,126,127, 31);mv1_core(34, 33,128,129,130,131, 32);
                    mv1_core(35, 34,132,133,134,135, 33);mv1_core(36, 35,136,137,138,139, 34);
                    mv1_core(37, 36,140,141,142,143, 35);mv1_core(38, 37,144,145,146,147, 36);
                    mv1_core(39, 38,148,149,150,151, 37);mv1_core(40, 39,152,153,154,155, 38);
                    mv1_core(41, 40,156,157,158,159, 39);mv1_core(42, 41,160,161,162,163, 40);
                    mv1_core(43, 42,164,165,166,167, 41);mv1_core(44, 43,168,169,170,171, 42);
                    mv1_core(45, 44,172,173,174,175, 43);mv1_core(46, 45,176,177,178,179, 44);
                    mv1_core(47, 46,180,181,182,183, 45);mv1_core(48, 47,184,185,186,187, 46);
                    mv1_core(49, 48,188,189,190,191, 47);mv1_core(50, 49,192,193,194,195, 48);
                    mv1_core(51, 50,196,197,198,199, 49);mv1_core(52, 51,200,201,202,203, 50);
                    mv1_core(53, 52,204,205,206,207, 51);mv1_core(54, 53,208,209,210,211, 52);
                    mv1_core(55, 54,212,213,214,215, 53);mv1_core(56, 55,216,217,218,219, 54);
                    mv1_core(57, 56,220,221,222,223, 55);

                    // mv1_core(58, 57,224,225,226,227, 56);
                    // mv1_core(59, 58,228,229,230,231, 57);
                    // mv1_core(60, 59,232,233,234,235, 58);
                    // mv1_core(61, 60,236,237,238,239, 59);

                    exe(OP_FAD, &AR[58][3], AR[57][3], EXP_H3232, AR[57][3], EXP_H1010, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                    exe(OP_FAD, &AR[58][2], AR[57][2], EXP_H3232, AR[57][2], EXP_H1010, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                    exe(OP_FAD, &AR[58][1], AR[57][1], EXP_H3232, AR[57][1], EXP_H1010, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);
                    exe(OP_FAD, &AR[58][0], AR[57][0], EXP_H3232, AR[57][0], EXP_H1010, 0LL, EXP_H3210, OP_NOP, 0LL, OP_NOP, 0LL);

                    mv1_store(62, 58);
                }
            }
        }
//EMAX5A end
    }
//EMAX5A drain_dirty_lmm

    float minDist = INFINITY;
    printf("IMAX Result: [");
    for (int j = 1; j <= size; j++) {
        float result = -imax_result_array[j-1];
        printf("%f", result);
        if (j < size) {
            printf(", ");
        }
        if (result < minDist) {
            minDist = result;
        }
        if (minDist < *curdist) {
            *curdist = minDist;
            *curNodeNum = *(data + j);
            changed = 1;
        }
    }
    printf("]\n");
    printf("IMAX Done\n");
    #ifdef ARMZYNQ
    // imax_dealloc(key_size, 32);
    // imax_dealloc(query_size, 32);
    // imax_dealloc(result_size, 32);
    #endif
    printf("imax_search_mv: changed=%d\n", changed);
    return changed;
}
#endif