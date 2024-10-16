
/* EMAX7 Compiter                      */
/*        Copyright (C) 2012 by NAIST. */
/*         Primary writer: Y.Nakashima */
/*                nakashim@is.naist.jp */

/*=======================================================================
=       conv-c2d.h      2012/9/21                                       =
=======================================================================*/
#include <sys/types.h>
#include <sys/param.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define         OBJSUFX "-emax7.c"
#define         SC1SUFX "-emax7s1.c"
#define         SC2SUFX "-emax7s2.c"
#define         FIGSUFX "-emax7.obj"

char            *srcprog;
char            *objprog;
char            *sc1prog;
char            *sc2prog;
char            *figfile;
FILE            *ofile; /* object file */
FILE            *s1fil; /* sc file1 (header) */
FILE            *s2fil; /* sc file2 (footer) */
FILE            *ffile; /* tgif file */

int             s1fil_header_ready;

#define YY_SKIP_YYWRAP

int             y_lineno;
int             y_errornum;

#define         yywrap()        (1)
#define         rehash(x)       ((x+137) % ID_NUM+1)

/* variable */
#define         ID_NUM          4096
/* variable.type */
#define         T_NONE          0x00
#define         T_IMMEDIATE     0x01
#define         T_EXRNO         0x02
#define         T_ALRNO         0x03
#define         T_BDRNO         0x04
#define         T_INITNO        0x05
#define         T_LOOPNO        0x06
#define         T_VARIABLE      0x07
#define         T_ASIS          0x08

#define BUF_MAXLEN 1024
char buf[BUF_MAXLEN+1];

struct id {
  char               *name;
  char               type;  /* T_XXX */
  char               itype; /* ITYPE_XXX where id is defined as dst */
  char               chip;  /* 0:shared(default), 1:core by core(CHIP is specified) */
  char               cidx;  /* 0:shared(default), 1:core by core(xxx[CHIP] is specified) */
  char               row;   /* -1:undefined, 0-EMAX_DEPTH-1:location of destination */
  char               col;   /* -1:undefined, 0-EMAX_WIDTH-1:location of destination */
  unsigned long long val;   /* immediate / absolute address */
} id[ID_NUM];
