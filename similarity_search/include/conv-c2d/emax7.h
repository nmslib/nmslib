
/* EMAX7 Compiler                      */
/*         Copyright (C) 2012 by NAIST */
/*         Primary writer: Y.Nakashima */
/*                nakashim@is.naist.jp */

#ifndef UTYPEDEF
#define UTYPEDEF
typedef unsigned char      Uchar;
typedef unsigned short     Ushort;
typedef unsigned int       Uint;
typedef unsigned long long Ull;
typedef long long int      Sll;
#if __AARCH64EL__ == 1
typedef long double Dll;
#else
typedef struct {Ull u[2];} Dll;
#endif
#endif

#if 1
void /*__attribute__((always_inline))*/ cex(Uint, Ull*,    Ull, Ull, Ull, Ull, Ushort);
void /*__attribute__((always_inline))*/ ex4(Uint, Ull*,    Ull*, Uint, Ull*, Uint, Ull*, Uint, Uint, Ull*, Uint, Ull*);
Ull  /*__attribute__((always_inline))*/ exm(Ull,  Uchar);
int  /*__attribute__((always_inline))*/ exe(Uint, Ull*,    Ull, Uint, Ull, Uint, Ull, Uint, Uint, Ull, Uint, Ull);
void /*__attribute__((always_inline))*/ mex(Uint, Uchar**, Uchar*, Ull, Uint, Uchar**, Uchar*, Ull, Ull, Ull, Ull);
void /*__attribute__((always_inline))*/ mo4(Uint, Ull,     Ull*, Ull, Ull, Uchar, Ull, Uint, Uint, Uchar, Ull, Uint);
void /*__attribute__((always_inline))*/ mop(Uint, Ull,     Ull*, Ull, Ull, Uchar, Ull, Uint, Uint, Uchar, Ull, Uint);
/* internal use only */
Ull  /*__attribute__((always_inline))*/ eam(Ull,  Uchar);
void /*__attribute__((always_inline))*/ eag(Ull*, Ull,     Ull);
void /*__attribute__((always_inline))*/ mmp(Uint, Ull,     Ull*, Ull, Ull, Uint, Uint);
#endif

#ifndef EMAX7DEF
#define EMAX7DEF

/*******************************************/
/* EMAX7A **********************************/
/*******************************************/

/* EMAX_NLANE (Macro-pipelining LANE) <= MAXCORE # of EMAX for macro-pipeliing/multi-AXI */
#define EMAX_NLANE      8
int     NLANE;
/* EMAX_NCHIP (Neighbor CHIP)         <= MAXCORE # of EMAX for cascading/single-AXI */
#define EMAX_NCHIP      4
#define AMAP_DEPTH      64
//#define EMAX_DEPTH    64
int     EMAX_DEPTH; /* extracted from emax7[i].reg_ctrl.i[0].stat */
#define EMAX_WIDTH      4
#define INSN_DEPTH      (EMAX_WIDTH*AMAP_DEPTH*4)
#define LMEM_SIZE       524288
int     LMM_SIZE;   /* extracted from emax7[i].reg_ctrl.i[0].stat */
#define LMEM_UMASK0     0
#define LMEM_UMASK1     0
#define LMEM_UMASK2     2
#define LMEM_UMASK3     3
#define LMEM_LMASK0     0x00000
#define LMEM_LMASK1     0x7ffff
#define LMEM_LMASK2     0x3ffff
#define LMEM_LMASK3     0x1ffff
#define UNIT_WIDTH      4

#define EXRNUM          10
#define CC_N            0x8LL
#define CC_Z            0x4LL
#define CC_C            0x2LL
#define CC_V            0x1LL

#define OP_NOP          0x00
#define OP_CEXE         0x01
#define OP_WHILE        0x01
#define OP_FOR          0x02

#define OP_CVT53        0x06
#define OP_SML8         0x07
#define OP_SFMA         0x08
#define OP_CFMA         0x0a

#define OP_FMA          0x10
#define OP_FMS          0x11
#define OP_FML          0x12
#define OP_FAD          0x13
#define OP_FML3         0x14
#define OP_ADD3         0x15
#define OP_SUB3         0x16
#define OP_ADD          0x17
#define OP_SUB          0x18
#define OP_CMP_EQ       0x19
#define OP_CMP_NE       0x1a
#define OP_CMP_LT       0x1b
#define OP_CMP_LE       0x1c
#define OP_CMP_GT       0x1d
#define OP_CMP_GE       0x1e
#define OP_CMOV         0x1f
#define OP_MAUH3        0x20
#define OP_MAUH         0x21
#define OP_MSUH3        0x22
#define OP_MSUH         0x23
#define OP_MLUH         0x24
#define OP_MMRG         0x25
#define OP_MSSAD        0x26
#define OP_MSAD         0x27
#define OP_MINL3        0x28
#define OP_MINL         0x29
#define OP_MH2BW        0x2a
#define OP_MCAS         0x2b
#define OP_MMID3        0x2c
#define OP_MMAX3        0x2d
#define OP_MMIN3        0x2e
#define OP_MMAX         0x2f
#define OP_MMIN         0x30
#define OP_MAJ          0x31
#define OP_CH           0x32

#define OP_AND          0x01
#define OP_OR           0x02
#define OP_XOR          0x03
#define OP_SUMHH        0x04
#define OP_SUMHL        0x05
#define OP_AD24         0x06
#define OP_ROTS         0x07

#define OP_SLL          0x01
#define OP_SRL          0x02
#define OP_SRAA         0x03
#define OP_SRAB         0x04
//#define OP_SRAC       0x05
//#define OP_SRAD       0x06
#define OP_SRLM         0x07

#define EXP_H3210       0
#define EXP_H1010       1
#define EXP_H3232       2
#define EXP_B5410       3
#define EXP_B7632       4
#define EXP_CVT00       0
#define EXP_CVT01       1
#define EXP_CVT02       2
#define EXP_CVT03       3
#define EXP_CVT04       4
#define EXP_CVT05       5
#define EXP_CVT06       6
#define EXP_CVT07       7

#define OP_ALWAYS       1
#define OP_CMPA_LE      2
#define OP_CMPA_GE      3

#define OP_LDR          0x01
#define OP_LDWR         0x02
//#define OP_LDHR       0x03
#define OP_LDBR         0x04
#define OP_LDRQ         0x08
#define OP_IM_BUFRD     0x0e
#define OP_IM_DRAIN     0x0f

#define OP_STR          0x11
#define OP_STWR         0x12
//#define OP_STHR       0x13
#define OP_STBR         0x14
#define OP_STRQ         0x18
#define OP_LDDMQ        0x19
#define OP_TR           0x1a
#define OP_IM_BUFWR     0x1e
#define OP_IM_PREF      0x1f

#define MSK_B0		0
#define MSK_B1		1
#define MSK_B2		2
#define MSK_B3		3
#define MSK_B4		4
#define MSK_B5		5
#define MSK_B6		6
#define MSK_B7		7
#define MSK_H0		8
#define MSK_H1		9
#define MSK_H2		10
#define MSK_H3		11
#define MSK_W0		12
#define MSK_W1		13
#define MSK_D0		14

/* insn */
int current_prefix;
int current_mapdist;
int current_nchip; /* 1:default, 1<=:CHIP specified */
int current_lmmwb;
int last_insn;

#ifndef BUF_MAXLEN
#define BUF_MAXLEN 1024
#endif
char forinit[2][EMAX_NCHIP][BUF_MAXLEN];
char forinit_cidx[2]; /* CHIP?1:0 */

struct cex {
  /* aluop�Τ���cmov������,cond=0/1�ˤ�äƥ����������ؤ��� */
  /* aluop�Τ���cmov�ʳ���memop������,cond=1�λ��Τ�ư�� */
  char op        ; /* opcd */
  char bit0v     ; /* id.type */
  int  bit0h     ; /* imm/reg number hash for cond_bit0 */
  char bit1v     ; /* id.type */
  int  bit1h     ; /* imm/reg number hash for cond_bit1 */
  char bit2v     ; /* id.type */
  int  bit2h     ; /* imm/reg number hash for cond_bit2 */
  char bit3v     ; /* id.type */
  int  bit3h     ; /* imm/reg number hash for cond_bit3 */
  Ull  table  :16; /* b3.b2.b1.b0���ȹ礻 1111,1110,1101,1100,....,0011,0010,0001,0000�γơ���0/1�������Ƥ�16bit����� */
  char cexdv     ; /* id.type */
  int  cexdh     ; /* destination hash for exrno */
};

struct exe {
  char op1       ; /* 0:none, 1:ex1 active */
  char op2       ; /* 0:none, 1:ex2 active */
  char op3       ; /* 0:none, 1:ex3 active */
  Ull  updt   : 1; /* 0:none, 1:self_update */
  Ull  init   : 2; /*         init.0=0:s1 from reg (!one_shot)                        */
                   /*         init.0=1:s1 from reg if LOOP0=1                         */
                   /*         init.1=0:s2 from reg (normal)                           */
                   /*         init.1=1:s2 from reg if LOOP1=1                         */
  char src1v     ; /* id.type */
  int  src1h     ; /* hash val */
  char src1s     ; /* suffix for var[s], bdr[][][s] */
  char src1e     ; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
  char src2v     ; /* id.type */
  int  src2h     ; /* hash val */
  char src2s     ; /* suffix for var[s], bdr[][][s] */
  char src2e     ; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
  char src3v     ; /* id.type */
  int  src3h     ; /* hash val */
  char src3s     ; /* suffix for var[s], bdr[][][s] */
  char src3e     ; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
  char src4v     ; /* id.type */
  int  src4h     ; /* hash val */
  char src4s     ; /* suffix for var[s], bdr[][][s] */
  char src5v     ; /* id.type */
  int  src5h     ; /* hash val */
  char src5s     ; /* suffix for var[s], bdr[][][s] */
  char exedv     ; /* id.type */
  int  exedh     ; /* hash val */
  char exeds     ; /* suffix for var[s], alr[][s], bdr[][][s] */
};

struct mex {
  char op0       ; /* mex(sparse matrix) conditional 0:NOP, 1:AL, 2:OP_CMPA_LE, 3:GE */
  char op1       ; /* mex(sparse matrix) conditional 0:NOP, 1:AL, 2:OP_CMPA_LE, 3:GE */
  Ull  init   : 1; /* mex(sparse matrix) 0:none, 1:INIT0? */
  char adr1v     ; /* id.type */
  int  adr1h     ; /* hash val */
  char adr1s     ; /* suffix for bdr[][][s] */
  char adr2v     ; /* id.type */
  int  adr2h     ; /* hash val */
  char adr2s     ; /* suffix for bdr[][][s] */
  char dist1v    ; /* id.type */
  int  dist1h    ; /* hash val */
  char adr3v     ; /* id.type */
  int  adr3h     ; /* hash val */
  char adr3s     ; /* suffix for bdr[][][s] */
  char adr4v     ; /* id.type */
  int  adr4h     ; /* hash val */
  char adr4s     ; /* suffix for bdr[][][s] */
  char dist2v    ; /* id.type */
  int  dist2h    ; /* hash val */
  char limitv    ; /* id.type */
  int  limith    ; /* hash val */
  char src1v     ; /* id.type */
  int  src1h     ; /* hash val */
  char src1s     ; /* suffix for var[s], bdr[][][s] */
  char src2v     ; /* id.type */
  int  src2h     ; /* hash val */
  char src2s     ; /* suffix for var[s], bdr[][][s] */
  char mexd0v    ; /* id.type */
  int  mexd0h    ; /* hash val */
  char mexd0s    ; /* suffix for var[s] */
  char mexd1v    ; /* id.type */
  int  mexd1h    ; /* hash val */
  char mexd1s    ; /* suffix for var[s] */
};

struct mop {
  char op        ; /* mop1:load, mop0:store/tr/load2/lddmq */
#define MTYPE_RLOAD  1
#define MTYPE_DLOAD  2
#define MTYPE_RSTORE 3
#define MTYPE_TRANS  4
#define RTYPE_DATA   1
#define RTYPE_BASE   2
#define RTYPE_OFFS   3
#define RTYPE_TOP    4
#define RTYPE_LEN    5
#define RTYPE_FORCE  6
#define RTYPE_PTOP   7
  char mtype     ;
  Ull  updt   : 1; /* 0:none, 1:self_update */
  char exv       ; /* id.type */
  int  exh       ; /* src exrno */
  char mopdv     ; /* id.type */
  int  mopdh     ;
  char mopds     ; /* suffix for var[s], bdr[][][s] */
  char basev     ; /* id.type */
  int  baseh     ;
  char bases     ; /* suffix for var[s], bdr[][][s] */
  char offsv     ; /* id.type */
  int  offsh     ;
  char offss     ; /* suffix for var[s], bdr[][][s] */
  char offsm     ; /* 0:B0, 1:B1, ... 7:B7, 8:H0, 9:H1, 10:H2, 11:H3, 12:W0, 13:W1, 14:D0, 15:-- */
  char topv      ; /* id.type */
  int  toph      ;
  int  lenv      ; /* id.type */
  int  lenh      ;
  char blk       ; /* T_IMMEDIATE *//* 0:inf, 1:16, 2:32, 3:64 width*block=page(burst)_size(bytes) */
  int  forcev    ; /* id.type */
  int  forceh    ;
  char ptopv     ; /* id.type */
  int  ptoph     ;
  int  plenv     ; /* id.type */
  int  plenh     ;
};

struct insn { /* EMAX7 instruction format */
  struct header {
#define ITYPE_WHILE 1
#define ITYPE_FOR   2
#define ITYPE_CEX   3
#define ITYPE_EX4   4
#define ITYPE_EXE   5
#define ITYPE_MEX   6
#define ITYPE_MO4   7
#define ITYPE_MOP   8
    char type      ; /* type 0:none, 1:WHILE, 2:FOR, 3:CEX, 4:EX4, 5:EXE, 6:MEX, 7:MO4, 8:MOP */
    char row       ; /* init=-1, programmed/fixed loc from dst-BR[][]/AR[] max 0-15 */
    char col       ; /* init=-1, programmed/fixed loc from dst-BR[][]/AR[] max 0-3 */
    char rdep      ; /* init=0,  allowed row by dependency */
    char fixed     ; /* 0:not fixed, 1:temporally fixed according to dependency, 2:fixed by AR[]/BR[][] */
  } iheader;
  struct cex icex;
  struct exe iexe;
  struct mex imex;
  struct mop imop;
} insn[INSN_DEPTH];

/* decode */
struct dec { /* EMAX7 instruction analysis */
  struct cex dcex;
  struct exu {
    char op1       ; /* 0:none, 1:ex1 active */
    char op2       ; /* 0:none, 1:ex2 active */
    char op3       ; /* 0:none, 1:ex3 active */
    Ull  updt   : 1; /* 0:none, 1:self_update */
    Ull  init   : 2; /*         init.0=0:s1 from reg (!one_shot)                        */
                     /*         init.0=1:s1 from reg if LOOP0=1                         */
                     /*         init.1=0:s2 from reg (normal)                           */
                     /*         init.1=1:s2 from reg if LOOP1=1                         */
    Ull  fold   : 1; /* 0:normal, 1:load-exe-store folding assumed                      */
    char ex1v      ; /* id.type */
    int  ex1h      ; /* hash val */
    char ex1s      ; /* suffix for var[s], bdr[][][s] */
    char ex1e      ; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
    char ex2v      ; /* id.type */
    int  ex2h      ; /* hash val */
    char ex2s      ; /* suffix for var[s], bdr[][][s] */
    char ex2e      ; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
    char ex3v      ; /* id.type */
    int  ex3h      ; /* hash val */
    char ex3s      ; /* suffix for var[s], bdr[][][s] */
    char ex3e      ; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
    char e2iv      ; /* id.type */
    int  e2ih      ; /* hash val */
    char e2is      ; /* selector 0:e2imm, 1:ex2, 2:ex3 */
    char e3iv      ; /* id.type */
    int  e3ih      ; /* hash val */
    char e3is      ; /* selector 0:e3imm, 1:ex3 */
    char exdv      ; /* id.type */
    int  exdh      ; /* hash val */
    char exds      ; /* suffix for var[s], bdr[][][s] */
  } dexu;
  struct mex dmex;  /* mex(sparse matrix) conditional 0:NOP, 1:AL, 2:OP_CMPA_LE, 3:GE */
  struct mop dmop0; /* mop0:store/load, load_single is assigned to BR[r][c][0] */
  struct mop dmop1; /* mop1:load only,  load_single is assigned to BR[r][c][1] */
} dec[AMAP_DEPTH][EMAX_WIDTH];

/* regmap & busmap */
struct bus {
  /* CEX signal */
  char cexdv  ; /* type=ex[0-9] */
  int  cexdh  ; /* hash=ex[0-9] */
  /* EXE regs */
  char exdrv  ;
  int  exdrh  ;
  char exdrs  ;
  /* EA0 regs */
  char ea0brv  ;
  int  ea0brh  ;
  char ea0orv  ;
  int  ea0orh  ;
  char ea0drv  ;
  int  ea0drh  ;
  char ea0woofsv; //MEX+sort
  int  ea0woofsh; //MEX+sort
  /* EA1 regs */
  char ea1brv  ;
  int  ea1brh  ;
  char ea1orv  ;
  int  ea1orh  ;
  char ea1drv  ;
  int  ea1drh  ;
  char ea1woofsv; //MEX+sort
  int  ea1woofsh; //MEX+sort
  /* axibusmap */
  struct {
    char v  ;
    int  h  ; /* hash */
    char s  ; /* suffix */
  } lmwd[UNIT_WIDTH], lmrd[UNIT_WIDTH];
  /* transmission regs */
  struct {
    char v  ;
    int  h  ; /* hash */
    char s  ; /* suffix */
  } tr[UNIT_WIDTH];
  /* lmm_write regs */
  struct {
    char v  ;
    int  h  ; /* hash */
    char s  ; /* suffix */
  } mw[UNIT_WIDTH];
  /* boundary regs */
  struct {
    char v  ;
    int  h  ; /* hash */
    char s  ; /* suffix */
  } br[UNIT_WIDTH];
} bus[AMAP_DEPTH][EMAX_WIDTH];

/* conf */
struct conf { /* final configuration info. for EMAX7-CGRA */
  struct cdw0 { /* select EXE-in */
    Ull  v      :  1; /* 0:inv, 1:insn mapped */
    Ull  op1    :  6; /* alu_opcd */
    Ull  op2    :  3; /* logical_opcd */
    Ull  op3    :  3; /* sft_opcd */
    Ull  ex1brs :  4; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
    Ull  ex1s   :  1; /* 0:ex1brs, 1:exdr(self-loop) */
    Ull  ex1exp :  3; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
    Ull  ex2brs :  4; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
    Ull  ex2exp :  3; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
    Ull  ex3brs :  4; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
    Ull  ex3exp :  3; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
    Ull  e2is   :  2; /* 0:e2imm, 1:ex2, 2:ex3 */
#define E3IMMBITS  6
    Ull  e3imm  : E3IMMBITS;
    Ull  e3is   :  1; /* 0:e3imm, 1:ex3 */
    Ull  init   :  2; /* bit0:activate s1+INIT0 bit1:activate s2+INIT0 */
    Ull  fold   :  1; /* 0:normal, 1:load-exe-store folding */
    /* sparse matrix */
    Ull  mex0op :  2; /* mex(sparse matrix) conditional 0:NOP, 1:AL, 2:OP_CMPA_LE, 3:OP_CMPA_GE */
    Ull  mex0init: 1; /* mex(sparse matrix) 0:none, 1:INIT0? */
    Ull  mex0dist: 3; /* distance 0:0, 1:1, 2:2, 3:4, 4:8, 5:16, 6:32, 7:64byte */
    Ull  mex1op :  2; /* mex(sparse matrix) conditional 0:NOP, 1:AL, 2:OP_CMPA_LE, 3:OP_CMPA_GE */
    Ull  mex1init: 1; /* mex(sparse matrix) 0:none, 1:INIT0? */
    Ull  mex1dist: 3; /* distance 0:0, 1:1, 2:2, 3:4, 4:8, 5:16, 6:32, 7:64byte */
    Ull  mexlimit: 4; /* limit 0:0, 1:8, 2:16, .... 10:4096, 11:8192, 12:16384, 13:32768 */
    Ull  dmy00  :  1;
  } cdw0;

  struct cdw1 { /* select CEX-in and EAG-in */
    Ull  cs0    :  4; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
    Ull  cs1    :  4; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
    Ull  cs2    :  4; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
    Ull  cs3    :  4; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
    Ull  cex_tab: 16; /* c3.c2.c1.c0���ȹ礻 (cop=NOP�ξ��,ffff) */
                      /* 1111,1110,1101,1100,....,0001,0000 �γơ���0/1�������Ƥ�16bit����� */
    Ull  ea0op  :  5; /* mem_opcd */
    Ull  ea0bs  :  2; /* 0:ea0br, 1:ea0dr(ea0br+self-loop), 2:eabbrs, 3:ea0dr(eabbrs+self-loop) */
    Ull  ea0os  :  1; /* 0:ea0or, 1:eaobrs */
    Ull  ea0msk :  4; /* 14:64bit, 13:word1, 12:word0, 11-8:half3-0, 7-0:byte7-0 of offset */
    Ull  ea1op  :  5; /* mem_opcd */
    Ull  ea1bs  :  2; /* 0:ea1br, 1:ea1dr(ea1br+self-loop), 2:eabbrs, 3:ea1dr(eabbrs+self-loop) */
    Ull  ea1os  :  1; /* 0:ea1or, 1:eaobrs */
    Ull  ea1msk :  4; /* 14:64bit, 13:word1, 12:word0, 11-8:half3-0, 7-0:byte7-0 of offset */
    Ull  eabbrs :  4; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
    Ull  eaobrs :  4; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
  } cdw1;

  struct cdw2 { /* select TR/BR-in */
    Ull  ts0    :  4; /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
    Ull  ts1    :  4; /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
    Ull  ts2    :  4; /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
    Ull  ts3    :  4; /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
    Ull  trs0   :  2; /* 0:lmwd0, 1:exdr, 2:ts0 *//* 0:TR�����񤭹�����, 1,2:EX/TS�񤭹����� */
    Ull  trs1   :  2; /* 0:lmwd1, 1:exdr, 2:ts1 */
    Ull  trs2   :  2; /* 0:lmwd2. 1:exdr, 2:ts2 */
    Ull  trs3   :  2; /* 0:lmwd3, 1:exdr, 2:ts3 */
    Ull  mwsa   :  1; /* 0:lmwa,  1:ea0d        *//* 0:���lmwd��ǽ, 1,2:EXEC���ʳ��϶���lmwd��ǽ */
    Ull  mws0   :  2; /* 0:lmwd0, 1:exdr, 2:ts0 *//* 0:���lmwd��ǽ, 1,2:EXEC���ʳ��϶���lmwd��ǽ */
    Ull  mws1   :  2; /* 0:lmwd1, 1:exdr, 2:ts1 */
    Ull  mws2   :  2; /* 0:lmwd2, 1:exdr, 2:ts2 */
    Ull  mws3   :  2; /* 0:lmwd3, 1:exdr, 2:ts3 */
    Ull  brs0   :  2; /* 0:off, 1:mr10, 2:tr0, 3:mr0  */
    Ull  brs1   :  2; /* 0:off, 1:mr11, 2:tr1, 3:mr1  */
    Ull  brs2   :  2; /* 0:off, 1:mr12, 2:tr2, 3:exdr(brs3=3�ξ��,ea0woofs����³) */
    Ull  brs3   :  2; /* 0:off, 1:mr13, 2:tr3  3:ea1woofs */
    Ull  mapdist:  6; /* ����UNIT��ˤ��뤬,�����ʪ��UNIT��1�ĤǤ褤 */
    Ull  lmm_mode: 2; /* ����LMM��˥��å� 0:̵��, 1:ʬ��̵, 2:2ʬ��, 3:4ʬ�� */
    Ull  lmm_axiw: 1; /* AXI->LMM write�о�(lmp/lmr/lmf/lmx�ξ��1) */
    Ull  lmm_axir: 1; /* AXI<-LMM read �о�(lmd/lmw/lmx    �ξ��1) */
    Ull  dmy20  : 13;
  } cdw2;

  struct cdw3 { /* e2 immediate */
    Ull  e2imm  : 64;
  } cdw3;
} conf[AMAP_DEPTH][EMAX_WIDTH]; /* 4dwords/unit costs 1cycle/unit: 4-parallel conf costs 1cycle/stage */

/* lmminfo */
struct lmmi { /* final FSM configuration for EMAX7-CGRA */
  Ull v    : 1; /* valid */
  Ull rw   : 1; /* 0:load(mem->lmm), 1:store(lmm->mem) */
  Ull f    : 1; /* load: 0:reuse LMM as possible, 1:force read */
                /* store:0:none,                  1:force read */
  Ull p    : 1; /* 0:normal, 1:prefetch/drain */
  Ull bcas : 4; /* column-bitmap for lmm broadcasting: slave��row_lmm_en�˻��� */
  Ull hcopy: 1; /* lmmi��load����(horizontal)�˻���. bcas_master�ϻ��Ѥ��ʤ� */
  Ull vcopy: 1; /* lmmi��load����(vertical)�˻��� */
  Ull blk  : 2; /* 0:inf, 1:16, 2:32, 3:64 width*block=page(burst)_size(bytes) */
  Ull cidx : 1; /* 0:shared(default), 1:core by core(xxx[CHIP] is specified) (for "top" only) */
  Ull len  :19; /* words of current stream (words) */
  Ull ofs  :32; /* lmp/lmd offset for f=0,p=1,mapdist=0 */
  Ull top  :64; /* top of current stream / TCU function() */
} lmmi[AMAP_DEPTH][EMAX_WIDTH]; /* 2dwords/unit costs 0.5cycle/unit: 4-parallel conf costs 0.5cycle/stage */
int   lmmi_first_loc;           /* �ǽ��lmr/lmf���֤򵭲�.+mapdist���֤ο���top����Ӥ��ۤʤ��SCON��� */
Ull   lmmi_bitmap[EMAX_WIDTH];  /* based on lmmi[*][EMAX_WIDTH][2].v */
Ull   range_bitmap[EMAX_WIDTH]; /* based on lmmi[*][EMAX_WIDTH][2].v */
Uchar range_link[AMAP_DEPTH][EMAX_WIDTH]; /* valid depth# of vcopy!=0 */
struct lmmx { 
  int  forcev    ; /* id.type */
  int  forceh    ;
  int  lenv      ; /* id.type */
  int  lenh      ;
} lmmx[AMAP_DEPTH][EMAX_WIDTH]; /* 2dwords/unit costs 0.5cycle/unit: 4-parallel conf costs 0.5cycle/stage */

/* regv template */
struct {
  struct {
    int  v; /* id.type */
    int  h;
    int  s; /* suffix */
  } br[UNIT_WIDTH];
  int  ea0b_v; /* id.type */
  int  ea0b_h;
  int  ea0b_s; /* suffix */
  int  ea0o_v; /* id.type */
  int  ea0o_h;
  int  ea0o_s; /* suffix */
  int  ea1b_v; /* id.type */
  int  ea1b_h;
  int  ea1b_s; /* suffix */
  int  ea1o_v; /* id.type */
  int  ea1o_h;
  int  ea1o_s; /* suffix */
} regv[AMAP_DEPTH][EMAX_WIDTH];

/*******************************************/
/* EMAX7T **********************************/
/*******************************************/
#define TRANS_DEPTH    16
int trans_pc;

/* transaction */
struct trans {
  Ull  rw            : 1;  /* 0:read, 1:write     */
  Ull  base_type     : 2;  /* 0:reg_val, 1:reg_adr, 2:mem_adr */
  Ull  base_num      : 3;  /* reg_num */
  char *base_symbol;       /* mem_adr */
  Ull  offset_type   : 1;  /* 0:reg,  1:immediate */
  Ull  offset        : 4;  /* reg_num, immediate  */
  Ull  offset_suffix : 3;  /* 0:imm 1:SUFLO 2:SUFHI 3:SUFFL 4:SUFB0 5:SUFB1 6:SUFB2 7:SUFB3 */
  Ull  offset_sll    : 3;  /* 0-7: shift left     */
  Ull  op_type       : 3;  /* 0:none, 1:+, 2:?eq, 3:?ne, 4:?ge, */
  Ull  op_val_type   : 2;  /* 0:reg,  1:immediate, 2:label */
  Ull  op_val_num;         /* reg_num, immediate */
  char *op_val_symbol;     /* mem_adr */
  Ull  t_action_type : 2;  /* 0:none, 1:term, 2:error, 3:goto */
  Ull  t_action      : 4;  /* target trans_pc */
  Ull  f_action_type : 2;  /* 0:none, 1:term, 2:error, 3:goto */
  Ull  f_action      : 4;  /* target trans_pc */
  Ull  reg_type      : 1;  /* 0:reg,  1:label */
  Ull  reg_num       : 3;  /* reg_num */
  char *reg_symbol;        /* mem_adr */
} trans[TRANS_DEPTH];

/* conf */
struct tconf {
  Ull  rw            : 1;  /* 0:read, 1:write     */
  Ull  base_type     : 2;  /* 0:reg_val, 1:reg_adr, 2:mem_adr */
  Ull  offset_type   : 1;  /* 0:reg,  1:immediate */
  Ull  offset        : 4;  /* reg_num, immediate  */
  Ull  offset_suffix : 3;  /* 0:imm 1:SUFLO 2:SUFHI 3:SUFFL 4:SUFB0 5:SUFB1 6:SUFB2 7:SUFB3 */
  Ull  offset_sll    : 3;  /* 0-7: shift left     */
  Ull  op_type       : 3;  /* 0:none, 1:+, 2:?eq, 3:?ne, 4:?ge, */
  Ull  op_val_type   : 1;  /* 0:reg,  1:immediate */
  Ull  t_action_type : 2;  /* 0:none, 1:term, 2:error, 3:goto */
  Ull  t_action      : 4;  /* target trans_pc */
  Ull  f_action_type : 2;  /* 0:none, 1:term, 2:error, 3:goto */
  Ull  f_action      : 4;  /* target trans_pc */
  Ull  reg_type      : 1;  /* 0:reg,  1:immediate */
  Ull  dmy           : 1;
  Ull  base;               /* reg_num, immediate */
  Ull  op_val;             /* reg_num, immediate */
  Ull  reg;                /* reg_num, immediate */
} tconf[TRANS_DEPTH]; /* 4 words */

#endif

/**/
