
static char RcsHeader[] = "$Header: /usr/home/nakashim/proj-arm64/src/conv-c2d/RCS/main.c,v 1.1 2023/04/10 14:47:16 nakashim Exp nakashim $";

/* EMAX7 Compiler                      */
/*        Copyright (C) 2012 by NAIST. */
/*         Primary writer: Y.Nakashima */
/*                nakashim@is.naist.jp */

/* main.c: emin assembler 2012/3/16 */ 

#include "conv-c2d.h"
#include "emax7.h"

extern FILE *yyin;

main(argc, argv) int argc; char **argv;
{
  char *version();
  struct stat stbuf;
  int i, j, offset;

  EMAX_DEPTH = 64; /* default */
  /* オプション解析 */
  for (argc--, argv++; argc; argc--, argv++) {
    if (**argv == '-') { /* regard as a command */
      switch (*(*argv+1)) {
      case 'v':
	printf("Conv-c2d Version %s\n", version());
	break;
      case 'u':
	sscanf(*argv+2, "%d", &EMAX_DEPTH);
	switch (EMAX_DEPTH) {
	case 64:
	case 32:
	case 16:
	case  8:
	  break;
	default:
	  printf("usage: conv-c2d [-v] [-uxx] src_prog\n");
	  printf(" xx should be 64,32,16,8\n");
	  exit(1);
	}
	break;
      default:
	printf("usage: conv-c2d [-v] [-uxx] src_prog\n");
	printf("       -v: vervose\n");
	printf("       -u64: 64 units\n");
	printf("       -u32: 32 units\n");
	printf("       -u16: 16 units\n");
	printf("       -u8 :  8 units\n");
	exit(1);
      }
    }
    else { /* regard as a filename */
      strncpy(srcprog = (char*)malloc(strlen(*argv)+1), *argv, strlen(*argv)+1);
    }
  }

  /* ファイルオープン */
  if ((yyin = fopen(srcprog, "r")) == NULL) {
    fprintf(stderr, "can't open source:\"%s\"\n", srcprog);
    exit(1);
  }
  strncpy(objprog = (char*)malloc(strlen(srcprog)+strlen(OBJSUFX)+1), srcprog, strlen(srcprog)+1); /* xxx.x -> xxx-emax7.c */
  for (i=0; i<strlen(srcprog); i++) {
    if (objprog[i] == '.' || objprog[i] == '\0' ) {
      strncpy(objprog+i, OBJSUFX, strlen(OBJSUFX)+1);
      break;
    }
  }
  if ((ofile = fopen(objprog, "w")) == NULL) {
    fprintf(stderr, "can't open output:\"%s\"\n", objprog);
    exit(1);
  }
  strncpy(sc1prog = (char*)malloc(strlen(srcprog)+strlen(SC1SUFX)+1), srcprog, strlen(srcprog)+1); /* xxx.x -> xxx-emax7pt.c */
  for (i=0; i<strlen(srcprog); i++) {
    if (sc1prog[i] == '.' || sc1prog[i] == '\0' ) {
      strncpy(sc1prog+i, SC1SUFX, strlen(SC1SUFX)+1);
      break;
    }
  }
  if ((s1fil = fopen(sc1prog, "w")) == NULL) {
    fprintf(stderr, "can't open output:\"%s\"\n", sc1prog);
    exit(1);
  }
  strncpy(sc2prog = (char*)malloc(strlen(srcprog)+strlen(SC2SUFX)+1), srcprog, strlen(srcprog)+1); /* xxx.x -> xxx-emax7pt.c */
  for (i=0; i<strlen(srcprog); i++) {
    if (sc2prog[i] == '.' || sc2prog[i] == '\0' ) {
      strncpy(sc2prog+i, SC2SUFX, strlen(SC2SUFX)+1);
      break;
    }
  }
  if ((s2fil = fopen(sc2prog, "w")) == NULL) {
    fprintf(stderr, "can't open output:\"%s\"\n", sc2prog);
    exit(1);
  }

  /* ASM読み込み */
  y_lineno = 1;
  y_errornum = 0;
  while (yyparse());
  fclose(yyin);
  fclose(ofile);
  fclose(s1fil);
  fclose(s2fil);

  if (y_errornum) exit(1);

  exit(0);
}

char *
version()
{
  char *i;

  for (i=RcsHeader; *i && *i!=' '; i++);
  for (           ; *i && *i==' '; i++);
  for (           ; *i && *i!=' '; i++);
  for (           ; *i && *i==' '; i++);
  return (i);
}
