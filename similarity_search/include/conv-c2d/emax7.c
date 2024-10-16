
static char RcsHeader[] = "$Header: /usr/home/nakashim/proj-arm64/src/conv-c2d/RCS/emax7.c,v 1.12 2023/12/01 05:33:20 nakashim Exp nakashim $";

/* EMAX7 Compiler                      */
/*         Copyright (C) 2012 by NAIST */
/*         Primary writer: Y.Nakashima */
/*                nakashim@is.naist.jp */

/* emax7.c 2012/9/22 */

#include <stdio.h>
#include "conv-c2d.h"
#include "emax7.h"

emit_emax7a(int mode) /* 0:array, 1:drain */
{
  int c, i, j, k, flag;
  int last_row = -1; /* last location */
  int last_col = -1; /* last location */
  int last_mop = -1; /* 0:mop0, 1:mop1 */
  int folding;
  struct cex *dcex;
  struct exu *dexu;
  struct mex *dmex;
  struct mop *dmop0, *dmop1;

  if (mode == 1) { /* emit drain */
    fprintf(ofile, "#ifndef EMAXSC\n");
    goto mode_drain_dirty_lmm;
  }

  /**********************************************************************************************************/
  /* ●insn[]に格納されるオペランド種類（★は絶対位置指定因子）                                             */
  /*              T_IMMEDIATE  T_VARIABLE              T_EXRNO     T_ALRNO                  T_BDRNO         */
  /* ------------------------------------------------------------------------------------------------------ */
  /* while_src1 :              VARIABLE,                                                                    */
  /* while_src2 : IMMEDIATE,                                                                                */
  /* while_dst  :              VARIABLE,                                                                    */
  /* ------------------------------------------------------------------------------------------------------ */
  /* cex_src    : IMMEDIATE,   VARIABLE,                                                                    */
  /* cex_dst    :                                       &EXRNO                                              */
  /* ------------------------------------------------------------------------------------------------------ */
  /* ex4_src    :                          VARIABLE([]),           ALRNO[r]([]),            BDRNO[r][c]([]) */
  /*                                     前段alu出力はBDRNO[r][*][2]を経由 前段ld出力はBDRNO[r][c][*]を経由 */
  /* ex4_dstd   :                          VARIABLE([]),         ★ALRNO[r]([]),       次段伝搬時にはBR経由 */
  /*                                       自段のalu出力           自段のalu出力       コンパイラが自動配置 */
  /* ------------------------------------------------------------------------------------------------------ */
  /* exe_src1   : IMMEDIATE,   VARIABLE,   VARIABLE[s],            ALRNO[r][s],             BDRNO[r][c][s]  */
  /* exe_src2   : IMMEDIATE,   VARIABLE,   VARIABLE[s],            ALRNO[r][s],             BDRNO[r][c][s]  */
  /* exe_src3   : IMMEDIATE,   VARIABLE,   VARIABLE[s],            ALRNO[r][s],             BDRNO[r][c][s]  */
  /* exe_src4   : IMMEDIATE,   VARIABLE,   VARIABLE[s],            ALRNO[r][s],             BDRNO[r][c][s]  */
  /* exe_src5   : IMMEDIATE,   VARIABLE,   VARIABLE[s],            ALRNO[r][s],             BDRNO[r][c][s]  */
  /* exe_dstd   :             &VARIABLE,  &VARIABLE[s],         ★&ALRNO[r][s],                             */
  /* ------------------------------------------------------------------------------------------------------ */
  /* mop_ex     : IMMEDIATE,                           ▲EXRNO                                              */
  /* mo4_srcdst :                          VARIABLE([]),           ALRNO[r]([]),          ★BDRNO[r][c]([]) */
  /*                                            store4,                 store4,                       load4 */
  /* mop_srcdst :             &VARIABLE,  &VARIABLE[s],           &ALRNO[r][s],          ★&BDRNO[r][c][01] */
  /*                             store1,        store1, store1,         store1,                       load1 */
  /*               store-dataのvariable/gprnoは,必ず同一段のALU出力.前段のBRをstoreする場合は,必ずALUを通す */
  /* mop_base   :              VARIABLE,    VARIABLE[s],                                    BDRNO[r][c][s]  */
  /* mop.offset : IMMEDIATE,   VARIABLE,    VARIABLE[s],                                    BDRNO[r][c][s]  */
  /* mop.stream :              VARIABLE,                                                                    */
  /**********************************************************************************************************/
  /* ●insn[]->decode[r][c] 配置ルール                                                                      */
  /*   ★絶対配置（当面配置ヒントに使用．将来的には削除）                                                   */
  /* - ex4(),exe():            dstにALRNO[]かBDRNO[][]が指定されている場合,指定位置に配置                   */
  /* - mo4(load),mop(load):    dstにBDRNO[][]が指定されている場合,指定位置に配置                            */
  /*   ▲隣接配置                                                                                           */
  /* - cex():                  srcにcxが指定されている場合,cx生成段の次段に配置                             */
  /* - mo4(exx):               srcにexxが指定されている場合,exx生成段と同段に配置                           */
  /**********************************************************************************************************/
  /*******************************************************************************************************************************************************/
  /* ●lmmi指示ルール (copy from conv-c2d/emac5.c)                                                                  lmmi-loc  v  top  blk  len  rw  f  p */
  /* LD with force-read=0 and ptop==NULL generates current(lmr) and reuse LMM. same as lmr in EMAX4                     curr  1  top  blk  len   0  0  0 */
  /* LD with force-read=1 and ptop==NULL generates current(lmf) and !reuse LMM. same as lmf in EMAX4                    curr  1  top  blk  len   0  1  0 */
  /* LD with force-read=0 and ptop!=NULL generates current(lmr) and next(lmp). mapdist!=0                               curr  1  top  blk  len   0  0  0 */
  /*                                                                                                                  c+dist  1 ptop  blk  len   0  0  1 */
  /* LD with force-read=0 and ptop!=NULL generates current(lmr) and next(lmp). mapdist==0                  ofs=ptop-top curr  1  top  blk  len   0  0  1 */
  /*                                                                                                               p=1の場合,pref-addrは常にlmmi.top+ofs */
  /* LDDMQ set f=1 and p=1 in lmmc automatically                                                                        curr  1  top  -    -     0  1  1 */
  /*******************************************************************************************************************************************************/
  /* ST with force-read=0 and ptop==NULL generates current(lmw) and reuse+wback LMM. same as lmw in EMAX4               curr  1  top  blk  len   1  0  0 */
  /* ST with force-read=1 and ptop==NULL generates current(lmx) and !reuse+wback LMM. same as lmx in EMAX4              curr  1  top  blk  len   1  1  0 */
  /* ST with force-read=0 and ptop!=NULL generates current(lmw) and prev(lmd). mapdist!=0                               curr  1  top  blk  len   1  0  0 */
  /*                                                                                                                  c-dist  1 ptop  blk  len   1  0  1 */
  /* ST with force-read=0 and ptop!=NULL generates current(lmw) and prev(lmd). mapdist==0                  ofs=ptop-top curr  1  top  blk  len   1  0  1 */
  /*                                                                                                              p=1の場合,drain-addrは常にlmmi.top+ofs */
  /* TR    set f=1 and p=1 in lmmc automatically                                                                        curr  1  top  -    -     1  1  1 */
  /*******************************************************************************************************************************************************/
  /**********************************************************************************************************/
  /* Step 1 ... decode[][]登録と伝搬レジスタ                                                                */
  /* 1-1.  各insn[]のsrc毎に依存関係検査対象を抽出し,insn[].header.rdepと比較・rdepを下方へ更新             */
  /*                              ●get_valid_row(hash) ... id[hash].typeにより区別                         */
  /*                                src毎に先行id[].row,col(初期値は-1)と順次比較                           */
  /* 1.2a  絶対位置指定★row,colがない場合(row=-1)                                                          */
  /*                              ●last_rowに,last_rowとrdepの大きい方を採用                               */
  /*                                last_row,last_colを増加しつつ配置                                       */
  /* 1.2b  絶対位置指定★row,colがある場合(row>=0)                                                          */
  /*                              ●指定row < rdepとlast_rowの大きい方ならerror                             */
  /*                                last_rowを指定rowに更新                                                 */
  /* 1-3.  先行写像との競合検査                                                                             */
  /*                              ●exe()とmop()の入力レジスタ競合,伝搬reg不足,reg間nw競合は,検査対象外     */
  /* 1-4.  insn[]->decode[][]コピーおよびALU機能割り当て可否検査                                            */
  /* 1-5.  busmap+伝搬レジスタ設定●emaxと異なり,更新変数は,同一行別カラムでの参照を禁止(逐次実行との互換)  */
  /*                           ---srcに関するbusmap---                                                      */
  /*                              ●WHILE stype=VAR                                                         */
  /*                                   -id.row=-1ならARMが直接前段BRの空きに設定                            */
  /*                                   -id.row>=0ならerror                                                  */
  /*                              ●CEX stype=VAR                                                           */
  /*                                   -id.row=-1ならARMが直接前段BRの空きに設定                            */
  /*                                   -id.row>=0 & row=自段(id.itype=CEX)なら ×                           */
  /*                                   -          & row=自段(id.itype=EXE)なら ×                           */
  /*                                   -          & row=自段(id.itype=MOP)なら ×                           */
  /*                                   -          & row=前段(id.itype=CEX)なら ×                           */
  /*                                   -          & row=前段(id.itype=EXE)なら,AR探索->BR割付               */
  /*                                   -          & row=前段(id.itype=MOP)なら,BR探索                       */
  /*                                   -          & row=前々(id.itype=CEX)なら ×                           */
  /*                                   -          & row=前々(id.itype=EXE)なら,AR探索->BR割付->BR伝搬       */
  /*                                   -          & row=前々(id.itype=MOP)なら,BR探索        ->BR伝搬       */
  /*                              ●EX4 stype=VAR,AR,BR                                                     */
  /*                              ●EXE stype=VAR,AR,BR,EX                                                  */
  /*                                   -id.row=-1ならARMが直接前段BRの空きに設定                            */
  /*                                   -id.row>=0 & row=自段(id.itype=CEX)なら,EX ★                        */
  /*                                   -          & row=自段(id.itype=EXE)なら ×                           */
  /*                                   -          & row=自段(id.itype=MOP)なら ×                           */
  /*                                   -          & row=前段(id.itype=CEX)なら ×                           */
  /*                                   -          & row=前段(id.itype=EXE)なら,AR探索->BR割付               */
  /*                                   -          & row=前段(id.itype=MOP)なら,BR探索                       */
  /*                                   -          & row=前々(id.itype=CEX)なら ×                           */
  /*                                   -          & row=前々(id.itype=EXE)なら,AR探索->BR割付->BR伝搬       */
  /*                                   -          & row=前々(id.itype=MOP)なら,BR探索        ->BR伝搬       */
  /*                              ●MO4 stype=VAR,AR,BR,EX                                                  */
  /*                              ●MOP stype=VAR,AR,BR,EX                                                  */
  /*                                   -id.row=-1ならARMが直接前段BRの空きに設定                            */
  /*                                   -id.row>=0 & row=自段(id.itype=CEX)なら,EX ★                        */
  /*                                   -          & row=自段(id.itype=EXE)なら,AR ★                        */
  /*                                   -          & row=自段(id.itype=MOP)なら ×                           */
  /*                                   -          & row=前段(id.itype=CEX)なら ×                           */
  /*                                   -          & row=前段(id.itype=EXE)なら,AR探索->BR割付               */
  /*                                   -          & row=前段(id.itype=MOP)なら,BR探索                       */
  /*                                   -          & row=前々(id.itype=CEX)なら ×                           */
  /*                                   -          & row=前々(id.itype=EXE)なら,AR探索->BR割付->BR伝搬       */
  /*                                   -          & row=前々(id.itype=MOP)なら,BR探索        ->BR伝搬       */
  /* 1-6.  位置を確定.dst変数の位置情報をid[].row,colに記録                                                 */
  /*                           ---dstに関するbusmap---                                                      */
  /*                              ●WHILEの位置暗黙指定は[0,0]．       busmapはEXDR(AR)止め                 */
  /*                              ●CEX  の位置明示指定は無．          busmapはCEXD止め (UNIT内で消費)      */
  /*                              ●EXE  の位置明示指定はout-AR．      busmapはEXDR(AR)止め                 */
  /*                 MTYPE_*LOAD  ●LD   の位置明示指定はout-BR．      busmapはBR止め                       */
  /*                 MTYPE_*STORE ●ST/TRの位置明示指定は同row内in-AR．busmapはTR止め                       */
  /**********************************************************************************************************/
  for (i=0; i<last_insn; i++) {
    char type  =  insn[i].iheader.type;
    char row   =  insn[i].iheader.row; /* 暫定 */
    char col   =  insn[i].iheader.col; /* 暫定 */
    char *rdep = &insn[i].iheader.rdep;
    if (row >= EMAX_DEPTH || col >= EMAX_WIDTH) {
      printf("in %s: specified [%d][%d] exceed limits (EMAX_ROW=%d EMAX_COL=%d)\n", id[current_prefix].name, row, col, EMAX_DEPTH, EMAX_WIDTH);
      exit(1);
    }
#if 1
    printf("%s:insn%03.3d:type=%d [%3.2d,%3.2d] ->", id[current_prefix].name, i, type, row, col);
#endif
    switch (type) {
    case ITYPE_WHILE: /* WHILE */
      /****************************************/
      /* WHILE is mapped only on decode[0][0] */
      /* and has only loc-free variable       */
      /* 1-1                                  */
      /****************************************/
      switch (insn[i].iexe.op1) {
      case OP_WHILE:
        /* 先行opはないので,iheader.rdepは0のまま更新不要 */
        break;
      default:
        printf("in %s: while() found illegal op=%d\n", id[current_prefix].name, insn[i].iexe.op1);
        exit(1);
      }
      if (row < 0) { /* none of WHILE/ALR/BDR is specified */
        /* 1-2a */
        /* never reached */
      }
      else { /* WHILE/ALR/BDR is specified */
        /* 1-2b */
        if (row > 0 || col > 0) { /* 0,0 is specified in conv-c2d.y */
          printf("in %s: [%d][%d] while() found\n", id[current_prefix].name, row, col);
          exit(1);
        }
        if (last_row < row) { /* 指定rowまで間が空くのはCとしてOK *//* whileは常にここを通過 */
          last_row = row;
          last_col = 0;
        }
        else { /* last_row >= row *//* 戻る方向はCとして意味が変わるので当面error *//* whileが先頭にない場合に該当 */
          printf("in %s: while() found violation of sequence (last_row=%d >= row=%d)\n", id[current_prefix].name, last_row, row);
          exit(1);
        }
      }
      /* 1-3 */
      if (last_row >= EMAX_DEPTH || last_col >= EMAX_WIDTH) { /* copy前に検査 */
        printf("in %s: [%d][%d] while() exceeds EMAX_DEPTH/EMAX_WIDTH\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      if (dec[last_row][last_col].dexu.op1 || dec[last_row][last_col].dexu.op2 || dec[last_row][last_col].dexu.op3) { /* 先行写像との競合検査 */
        printf("in %s: [%d][%d] while() conflicts\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      /* 1-4 */
      dexu = &dec[last_row][last_col].dexu;
      dexu->op1    = insn[i].iexe.op1;
      dexu->op2    = insn[i].iexe.op2;
      dexu->op3    = insn[i].iexe.op3;
      dexu->updt   = insn[i].iexe.updt;
      dexu->init   = insn[i].iexe.init;
      dexu->fold   = 0; /* normal */
      dexu->ex1v   = insn[i].iexe.src1v; /* default */
      dexu->ex1h   = insn[i].iexe.src1h; /* default */
      dexu->ex1s   = insn[i].iexe.src1s; /* default */
      dexu->ex1e   = insn[i].iexe.src1e; /* default */
      dexu->ex2v   = insn[i].iexe.src2v; /* default */
      dexu->ex2h   = insn[i].iexe.src2h; /* default */
      dexu->ex2s   = insn[i].iexe.src2s; /* default */
      dexu->ex2e   = insn[i].iexe.src2e; /* default */
      dexu->ex3v   = T_NONE;
      dexu->ex3h   = -1;
      dexu->ex3s   = -1;
      dexu->ex3e   = 0;
      dexu->e2iv   = T_NONE;
      dexu->e2ih   = -1;
      dexu->e2is   = 0;
      dexu->e3iv   = T_NONE;
      dexu->e3ih   = -1;
      dexu->e3is   = 0;
      dexu->exdv   = insn[i].iexe.exedv; /* default */
      dexu->exdh   = insn[i].iexe.exedh; /* default */
      dexu->exds   = insn[i].iexe.exeds; /* default */
      /* 1-5 *//* EMAX4と異なり逐次実行互換のため,prop_skpは不要 */
      /*     *//* id[].row,colが-1の場合,srcはARMがセット.前段の場合br不要.それ以外はbr伝搬. */
      set_reg_path(last_row, last_col, 0, type, RTYPE_DATA, 0, dexu->ex1v, dexu->ex1h, dexu->ex1s);
      set_reg_path(last_row, last_col, 0, type, RTYPE_DATA, 0, dexu->ex2v, dexu->ex2h, dexu->ex2s);
      /* 1-6 *//* 格納先busmapはEXDR止まり */
      bus[last_row][last_col].exdrv = dexu->exdv;
      bus[last_row][last_col].exdrh = dexu->exdh;
      bus[last_row][last_col].exdrs = dexu->exds;
      id[insn[i].iexe.exedh].itype = ITYPE_WHILE;
      id[insn[i].iexe.exedh].row   = last_row;
      id[insn[i].iexe.exedh].col   = last_col;
      insn[i].iheader.fixed = 1;
      break;
    case ITYPE_FOR: /* FOR */
      /*****************************************/
      /* FOR is mapped only on decode[0][LOOP#]*/
      /* and has only loc-free variable        */
      /* 1-1                                   */
      /*****************************************/
      switch (insn[i].iexe.op1) {
      case OP_FOR:
        /* 先行opはないので,iheader.rdepは0のまま更新不要 */
        break;
      default:
        printf("in %s: for() found illegal op=%d\n", id[current_prefix].name, insn[i].iexe.op1);
        exit(1);
      }
      if (row < 0) { /* none of WHILE/ALR/BDR is specified */
        /* 1-2a */
        /* never reached */
      }
      else { /* WHILE/ALR/BDR is specified */
        /* 1-2b */
        if (row > 0 || col > 1) { /* 0,<2 is specified in conv-c2d.y */
          printf("in %s: [%d][%d] for() found\n", id[current_prefix].name, row, col);
          exit(1);
        }
        if (last_row <= row) { /* for()はcol指定が1,0の逆順 *//* forは常にここを通過 */
          last_row = row;
          last_col = col;
        }
        else { /* last_row >= row *//* 戻る方向はCとして意味が変わるので当面error *//* whileが先頭にない場合に該当 */
          printf("in %s: for() found violation of sequence (last_row=%d >= row=%d)\n", id[current_prefix].name, last_row, row);
          exit(1);
        }
      }
      /* 1-3 */
      if (last_row >= EMAX_DEPTH || last_col >= EMAX_WIDTH) { /* copy前に検査 */
        printf("in %s: [%d][%d] for() exceeds EMAX_DEPTH/EMAX_WIDTH\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      if (dec[last_row][last_col].dexu.op1 || dec[last_row][last_col].dexu.op2 || dec[last_row][last_col].dexu.op3) { /* 先行写像との競合検査 */
        printf("in %s: [%d][%d] for() conflicts\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      /* 1-4 */
      dexu = &dec[last_row][last_col].dexu;
      dexu->op1    = insn[i].iexe.op1;
      dexu->op2    = insn[i].iexe.op2;
      dexu->op3    = insn[i].iexe.op3;
      dexu->updt   = insn[i].iexe.updt;
      dexu->init   = insn[i].iexe.init;
      dexu->fold   = 0; /* normal */
      dexu->ex1v   = insn[i].iexe.src1v; /* default */
      dexu->ex1h   = insn[i].iexe.src1h; /* default */
      dexu->ex1s   = insn[i].iexe.src1s; /* default */
      dexu->ex1e   = insn[i].iexe.src1e; /* default */
      dexu->ex2v   = insn[i].iexe.src2v; /* default */
      dexu->ex2h   = insn[i].iexe.src2h; /* default */
      dexu->ex2s   = insn[i].iexe.src2s; /* default */
      dexu->ex2e   = insn[i].iexe.src2e; /* default */
      dexu->ex3v   = T_NONE;
      dexu->ex3h   = -1;
      dexu->ex3s   = -1;
      dexu->ex3e   = 0;
      dexu->e2iv   = T_NONE;
      dexu->e2ih   = -1;
      dexu->e2is   = 0;
      dexu->e3iv   = T_NONE;
      dexu->e3ih   = -1;
      dexu->e3is   = 0;
      dexu->exdv   = insn[i].iexe.exedv; /* default */
      dexu->exdh   = insn[i].iexe.exedh; /* default */
      dexu->exds   = insn[i].iexe.exeds; /* default */
      /* 1-5 *//* EMAX4と異なり逐次実行互換のため,prop_skpは不要 */
      /*     *//* id[].row,colが-1の場合,srcはARMがセット.前段の場合br不要.それ以外はbr伝搬. */
      set_reg_path(last_row, last_col, 0, type, RTYPE_DATA, 0, dexu->ex1v, dexu->ex1h, dexu->ex1s);
      set_reg_path(last_row, last_col, 0, type, RTYPE_DATA, 0, dexu->ex2v, dexu->ex2h, dexu->ex2s);
      /* 1-6 *//* 格納先busmapはEXDR止まり */
      bus[last_row][last_col].exdrv = dexu->exdv;
      bus[last_row][last_col].exdrh = dexu->exdh;
      bus[last_row][last_col].exdrs = dexu->exds;
      id[insn[i].iexe.exedh].itype = ITYPE_FOR;
      id[insn[i].iexe.exedh].row   = last_row;
      id[insn[i].iexe.exedh].col   = last_col;
      insn[i].iheader.fixed = 1;
      break;
    case ITYPE_CEX: /* CEX */
      /*********************************************/
      /* CEX is mapped on the row next to exe_dstc */
      /* and has no absolute location              */
      /* 1-1                                       */
      /*********************************************/
      switch (insn[i].icex.op) {
      case OP_CEXE:
        get_valid_row(ITYPE_CEX, 0, insn[i].icex.bit0v, insn[i].icex.bit0h, rdep);
        get_valid_row(ITYPE_CEX, 0, insn[i].icex.bit1v, insn[i].icex.bit1h, rdep);
        get_valid_row(ITYPE_CEX, 0, insn[i].icex.bit2v, insn[i].icex.bit2h, rdep);
        get_valid_row(ITYPE_CEX, 0, insn[i].icex.bit3v, insn[i].icex.bit3h, rdep);
        break;
      default:
        printf("in %s: cexe found illegal op=%d\n", id[current_prefix].name, insn[i].icex.op);
        exit(1);
      }

      folding = 0; /* reset */

      if (row < 0) { /* none of WHILE/ALR/BDR is specified */
        /* 1-2a *//* 同一rowに複数CEXEが配置されることがある */
        if (last_row < *rdep) { /* 間を空けて配置可能 */
          last_row = *rdep;
          last_col = 0;
        }
        else { /* last_row >= *rdep *//* 最終位置>=依存関係制約位置 */
          if (dec[last_row][last_col].dcex.op) /* 同一row後続CEXEが該当.先行写像との競合検査 */
            last_col++; /* overflowはあとで検査.CEXEがWIDTH以上あればerror */
          if (dec[last_row][last_col].dmop0.op) /* 同一row先行STOREが該当.先行写像との競合検査 */
            last_col++; /* overflowはあとで検査.CEXEがWIDTH以上あればerror */
        }
	if (dec[last_row][last_col].dexu.fold) { /* exeがfoldなら同一unit.storeも無条件でfold */
	  printf("load-exe-store folding assumed ");
	  folding = 1; /* load-exe-store folding */
	}
      }
      else { /* WHILE/ALR/BDR is specified */
        /* 1-2b */
        /* never reached */
      }
      /* 1-3 */
      if (last_row >= EMAX_DEPTH || last_col >= EMAX_WIDTH) { /* copy前に検査 */
        printf("in %s: [%d][%d] cexe exceeds EMAX_DEPTH/EMAX_WIDTH\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      if (dec[last_row][last_col].dcex.op) { /* 先行写像との競合検査 */
        printf("in %s: [%d][%d] cexe conflicts\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      /* 1-4 */
      dcex = &dec[last_row][last_col].dcex;
      *dcex = insn[i].icex;
      /* 1-5 *//* EMAX4と異なり逐次実行互換のため,prop_skpは不要 */
      /*     *//* id[].row,colが-1の場合,srcはARMがセット.前段の場合br不要.それ以外はbr伝搬. */
      set_reg_path(last_row, last_col, folding, type, RTYPE_DATA, 0, dcex->bit0v, dcex->bit0h, -1);
      set_reg_path(last_row, last_col, folding, type, RTYPE_DATA, 0, dcex->bit1v, dcex->bit1h, -1);
      set_reg_path(last_row, last_col, folding, type, RTYPE_DATA, 0, dcex->bit2v, dcex->bit2h, -1);
      set_reg_path(last_row, last_col, folding, type, RTYPE_DATA, 0, dcex->bit3v, dcex->bit3h, -1);
      /* 1-6 *//* 格納先busmapはCEXD止まり */
      bus[last_row][last_col].cexdv = dcex->cexdv;
      bus[last_row][last_col].cexdh = dcex->cexdh;
      id[insn[i].icex.cexdh].itype = ITYPE_CEX;
      id[insn[i].icex.cexdh].row   = last_row;
      id[insn[i].icex.cexdh].col   = last_col;
      insn[i].iheader.fixed = 1;
      break;
    case ITYPE_EX4: /* EX4 */
      /**********************************************/
      /* EX4 has variable([]) or bdr[][]([]) as src */
      /* 1-1                                        */
      /**********************************************/
      switch (insn[i].iexe.op1) {
      case OP_SML8: /* 2 */
      case OP_FML:  /* 2 */
      case OP_FAD:  /* 2 */
      case OP_ADD:  /* 2 */
      case OP_SUB:  /* 2 */
        get_valid_row(ITYPE_EX4, 0, insn[i].iexe.src1v, insn[i].iexe.src1h, rdep);
        get_valid_row(ITYPE_EX4, 0, insn[i].iexe.src2v, insn[i].iexe.src2h, rdep);
        insn[i].iexe.src3v = T_NONE; /* delete */
        break;
      case OP_CVT53:/* 3 */
      case OP_SFMA: /* 3 */
      case OP_FMA:  /* 3 */
      case OP_FMS:  /* 3 */
      case OP_FML3: /* 3 */
      case OP_ADD3: /* 3 */
      case OP_SUB3: /* 3 */
        get_valid_row(ITYPE_EX4, 0, insn[i].iexe.src1v, insn[i].iexe.src1h, rdep);
        get_valid_row(ITYPE_EX4, 0, insn[i].iexe.src2v, insn[i].iexe.src2h, rdep);
        get_valid_row(ITYPE_EX4, 0, insn[i].iexe.src3v, insn[i].iexe.src3h, rdep);
        break;
      default:
        printf("in %s: ex4 found illegal op1=%d\n", id[current_prefix].name, insn[i].iexe.op1);
        exit(1);
      }
      switch (insn[i].iexe.op2) {
      case OP_NOP:
        insn[i].iexe.src4v = T_IMMEDIATE; /* OP_SFMA */
        break;
      default:
        printf("in %s: exe found illegal op2=%d\n", id[current_prefix].name, insn[i].iexe.op2);
	exit(1);
      }
      switch (insn[i].iexe.op3) {
      case OP_NOP:
        insn[i].iexe.src5v = T_IMMEDIATE; /* OP_SFMA */
        break;
      default:
	printf("in %s: exe found illegal op3=%d\n", id[current_prefix].name, insn[i].iexe.op3);
	exit(1);
      }

      folding = 0; /* reset */

      if (row < 0) { /* none of WHILE/ALR/BDR is specified */
        /* 1-2a */
        if (last_row < *rdep) { /* 間を空けて配置可能 */
	  if (last_row == *rdep-1 && insn[i].iexe.src1h == insn[i].iexe.exedh) { /* load-sfma-store foldingの場合,例えばrow=7,src=6,7 fold=1にセットしてトライ */
	    for (j=0; j<EMAX_WIDTH; j++) {
	      if (insn[i].iexe.src1h == dec[last_row][j].dmop1.mopdh) {
		last_col = j;
		break;
	      }
	    }
	    if (j<EMAX_WIDTH) {
	      printf("load-sfma-store folding w/ ALU loop assumed ");
	      folding = 1;
	    }
	    else {
	      last_row = *rdep;
	      last_col = 0;
	    }
	  }
          else {
	    last_row = *rdep;
	    last_col = 0; /* 位置指定可能ex4.dstはALRのみ.var経由の同一row内後続st4()を配置可能 */
	  }
        }
        else { /* last_row >= *rdep *//* 最終位置>=依存関係制約位置 */
          if (dec[last_row][0].dexu.op1 || dec[last_row][0].dexu.op2 || dec[last_row][0].dexu.op3) { /* 先行写像との競合検査 */
            last_row++; /* overflowはあとで検査.CEXEがWIDTH以上あればerror */
            last_col = 0; /* 位置指定可能ex4.dstはALRのみ.var経由の同一row内後続st4()を配置可能 */
          }
        }
      }
      else { /* WHILE/ALR/BDR is specified */
        /* 1-2b */
        if (row < *rdep) { /* 指定rowには配置困難 */
          printf("in %s: ex4 cannot locate destination ALRNO (row=%d)\n", id[current_prefix].name, row);
          exit(1);
        }
        else if (row < last_row) {
          printf("in %s: ex4 found violation of sequence (row=%d < last_row=%d)\n", id[current_prefix].name, row, last_row);
          exit(1);
        }
        else {
          last_row = row;
          last_col = 0; /* 位置指定可能ex4.dstはALRのみ.ALR経由の同一row内後続st4()を配置可能 */
        }
      }
      /* 1-3 */
      if (last_row >= EMAX_DEPTH || last_col >= EMAX_WIDTH) { /* copy前に検査 */
        printf("in %s: [%d][%d] ex4 exceeds EMAX_DEPTH/EMAX_WIDTH\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      /* ex4[row] is expanded into all columns in decode[row][] */
      for (j=0; j<EMAX_WIDTH; j++) {
        if (dec[last_row][j].dexu.op1 || dec[last_row][j].dexu.op2 || dec[last_row][j].dexu.op3) { /* 先行写像との競合検査 */
          printf("in %s: [%d][%d] ex4 conflicts\n", id[current_prefix].name, last_row, j);
          exit(1);
        }
      }
      /* 1-4 */
      for (j=0; j<EMAX_WIDTH; j++) {
        dexu = &dec[last_row][j].dexu;
        dexu->op1    = insn[i].iexe.op1;
        dexu->op2    = insn[i].iexe.op2;
        dexu->op3    = insn[i].iexe.op3;
        dexu->updt   = insn[i].iexe.updt;
	dexu->init   = insn[i].iexe.init;
	dexu->fold   = folding;
        dexu->ex1v   = insn[i].iexe.src1v; /* default */
        dexu->ex1h   = insn[i].iexe.src1h; /* default */
        dexu->ex1s   = insn[i].iexe.op1==OP_SFMA?-1:j;
        dexu->ex1e   = insn[i].iexe.src1e; /* default */
        dexu->ex2v   = insn[i].iexe.src2v; /* default */
        dexu->ex2h   = insn[i].iexe.src2h; /* default */
        dexu->ex2s   = j;
        dexu->ex2e   = insn[i].iexe.src2e; /* default */
        dexu->ex3v   = insn[i].iexe.src3v; /* default */
        dexu->ex3h   = insn[i].iexe.src3h; /* default */
        dexu->ex3s   = j;
        dexu->ex3e   = insn[i].iexe.src3e; /* default */
        dexu->e2iv   = insn[i].iexe.src4v; /* default */
        dexu->e2ih   = insn[i].iexe.src4h; /* default */
        dexu->e2is   = 0;                  /* e2imm */
        dexu->e3iv   = insn[i].iexe.src5v; /* default */
        dexu->e3ih   = insn[i].iexe.src5h; /* default */
        dexu->e3is   = 0;                  /* e3imm */
        dexu->exdv   = insn[i].iexe.exedv; /* default */
        dexu->exdh   = insn[i].iexe.exedh; /* default */
        dexu->exds   = insn[i].iexe.op1==OP_SFMA?-1:j;
      }
      /* 1-5 *//* EMAX4と異なり逐次実行互換のため,prop_skpは不要 */
      /*     *//* id[].row,colが-1の場合,srcはARMがセット.前段の場合br不要.それ以外はbr伝搬. */
      for (j=0; j<EMAX_WIDTH; j++) {
        dexu = &dec[last_row][j].dexu;
        set_reg_path(last_row, last_col, folding, type, RTYPE_DATA, 0, dexu->ex1v, dexu->ex1h, dexu->ex1s); /* ex1s=3,2,1,0 */
        set_reg_path(last_row, last_col, folding, type, RTYPE_DATA, 0, dexu->ex2v, dexu->ex2h, dexu->ex2s); /* ex2s=3,2,1,0 */
        set_reg_path(last_row, last_col, folding, type, RTYPE_DATA, 0, dexu->ex3v, dexu->ex3h, dexu->ex3s); /* ex3s=3,2,1,0 */
      }
      /* 1-6 *//* 格納先busmapはEXDR止まり */
      for (j=0; j<EMAX_WIDTH; j++) {
        dexu = &dec[last_row][j].dexu;
        bus[last_row][j].exdrv = dexu->exdv;
        bus[last_row][j].exdrh = dexu->exdh;
        bus[last_row][j].exdrs = dexu->exds; /* exds=3,2,1,0 */
      }
      id[insn[i].iexe.exedh].itype = ITYPE_EX4;
      id[insn[i].iexe.exedh].row   = last_row;
      id[insn[i].iexe.exedh].col   = -1; /* ★★★ exe->AR[]の場合,全columnで共通なので，問題なし */
      insn[i].iheader.fixed = 1;
      break;
    case ITYPE_EXE: /* EXE */
      /*****************************************/
      /* 1-1                                   */
      /* EXE has imm, var, var[], bdr[][][]    */
      /* ★load-exe-store foldingの場合        */
      /*          □BR                         */
      /*          │                --row#6--  */
      /*          exe                          */
      /*          □AR6                        *//*    mm+rmm.cの例                                    */
      /*          │                           *//* (1)mop(OP_LDWR,  &BR[7][0][1])                     */
      /*          □BR6                        *//* (2)exe(OP_FAD,   &AR[7][0], AR[6][0], BR[7][0][1]) */
      /*  (1)     │      (2)   (3) --row#7--  *//*    ★dst:rowのAR,src:rowのBRの場合,                */
      /*  eag ┌─│─┬┐exe   eag            *//*      srcへのパスは一旦rowのBRまで繋ぎ,             */
      /*  □LD│  □TR│  □AR7 □ST           *//*      そこから戻してs1,s2,s3に繋ぐ.                 */
      /*  │  │  │  │　│    │             *//*      s1,s2,s3が定数の場合も次rowにセットして戻す   */
      /*  □  │  │  │　└──□LMM(ST)      */
      /*  │  │  │  │                       *//* (3)mop(OP_STWR,  &AR[7][0])                        */
      /*  □BR┘  □BR┘                       *//*    ★このパターンは従来通り                        */
      /*****************************************/
      switch (insn[i].iexe.op1) {
      case OP_NOP:
        get_valid_row(ITYPE_EXE, 0, insn[i].iexe.src1v, insn[i].iexe.src1h, rdep);
        insn[i].iexe.src2v = T_NONE; /* delete */
        insn[i].iexe.src3v = T_NONE; /* delete */
        break;
      case OP_SML8:   /* 2 */
      case OP_FML:    /* 2 */
      case OP_FAD:    /* 2 */
      case OP_ADD:    /* 2 */
      case OP_SUB:    /* 2 */
      case OP_CMP_EQ: /* 2 */
      case OP_CMP_NE: /* 2 */
      case OP_CMP_LT: /* 2 */
      case OP_CMP_LE: /* 2 */
      case OP_CMP_GT: /* 2 */
      case OP_CMP_GE: /* 2 */
      case OP_MAUH:   /* 2 */
      case OP_MSUH:   /* 2 */
      case OP_MLUH:   /* 2 */
      case OP_MSAD:   /* 2 */
      case OP_MINL:   /* 2 */
      case OP_MH2BW:  /* 2 */
      case OP_MCAS:   /* 2 */
      case OP_MMAX:   /* 2 */
      case OP_MMIN:   /* 2 */
        get_valid_row(ITYPE_EXE, 0, insn[i].iexe.src1v, insn[i].iexe.src1h, rdep);
        get_valid_row(ITYPE_EXE, 0, insn[i].iexe.src2v, insn[i].iexe.src2h, rdep);
        insn[i].iexe.src3v = T_NONE; /* delete */
        break;
      case OP_CVT53:  /* 3 */
      case OP_CFMA:   /* 3 */
      case OP_FMA:    /* 3 */
      case OP_FMS:    /* 3 */
      case OP_FML3:   /* 3 */
      case OP_ADD3:   /* 3 */
      case OP_SUB3:   /* 3 */
      case OP_CMOV:   /* 3 */
      case OP_MAUH3:  /* 3 */
      case OP_MSUH3:  /* 3 */
      case OP_MMRG:   /* 3 */
      case OP_MSSAD:  /* 3 */
      case OP_MINL3:  /* 3 */
      case OP_MMID3:  /* 3 */
      case OP_MMAX3:  /* 3 */
      case OP_MMIN3:  /* 3 */
      case OP_MAJ:    /* 3 */
      case OP_CH:     /* 3 */
        get_valid_row(ITYPE_EXE, 0, insn[i].iexe.src1v, insn[i].iexe.src1h, rdep);
        get_valid_row(ITYPE_EXE, 0, insn[i].iexe.src2v, insn[i].iexe.src2h, rdep);
        get_valid_row(ITYPE_EXE, 0, insn[i].iexe.src3v, insn[i].iexe.src3h, rdep);
        break;
      default:
        printf("in %s: exe found illegal op1=%d\n", id[current_prefix].name, insn[i].iexe.op1);
        exit(1);
      }
      switch (insn[i].iexe.op2) {
      case OP_NOP:
        insn[i].iexe.src4v = T_NONE; /* delete */
        break;
      case OP_AND:
      case OP_OR:
      case OP_XOR:
      case OP_SUMHH:
      case OP_SUMHL:
      case OP_AD24:
      case OP_ROTS:
        get_valid_row(ITYPE_EXE, 0, insn[i].iexe.src4v, insn[i].iexe.src4h, rdep);
        break;
      default:
        printf("in %s: exe found illegal op2=%d\n", id[current_prefix].name, insn[i].iexe.op2);
        exit(1);
      }
      switch (insn[i].iexe.op3) {
      case OP_NOP:
        insn[i].iexe.src5v = T_NONE; /* delete */
        break;
      case OP_SLL:
      case OP_SRL:
      case OP_SRAA:
      case OP_SRAB:
//    case OP_SRAC:
//    case OP_SRAD:
      case OP_SRLM:
        get_valid_row(ITYPE_EXE, 0, insn[i].iexe.src5v, insn[i].iexe.src5h, rdep);
        break;
      default:
        printf("in %s: exe found illegal op3=%d\n", id[current_prefix].name, insn[i].iexe.op3);
        exit(1);
      }

      folding = 0; /* reset */

      if (row < 0) { /* none of WHILE/ALR/BDR is specified */
        /* 1-2a *//* 同一rowに複数EXEが配置されることがある */
        if (last_row < *rdep) { /* 間を空けて配置可能 */
	  if (last_row == *rdep-1 && insn[i].iexe.src1h == insn[i].iexe.exedh) { /* load-exe-store foldingの場合,例えばrow=7,src=6,7 fold=1にセットしてトライ */
	    for (j=0; j<EMAX_WIDTH; j++) {
	      if (insn[i].iexe.src1h == dec[last_row][j].dmop1.mopdh) {
		last_col = j;
		break;
	      }
	    }
	    if (j<EMAX_WIDTH) {
	      printf("load-exe-store folding w/ ALU loop assumed ");
	      folding = 1;
	    }
	    else {
	      last_row = *rdep;
	      last_col = 0;
	    }
	  }
          else {
	    last_row = *rdep;
	    last_col = 0;
	  }
        }
        else { /* last_row >= *rdep *//* 最終位置と依存関係制約位置が一致 */
          if (dec[last_row][last_col].dexu.op1 || dec[last_row][last_col].dexu.op2 || dec[last_row][last_col].dexu.op3) { /* 先行写像との競合検査 */
            last_col++; /* overflowはあとで検査.EXEがWIDTH以上あれば次段へ移動 */
            if (last_col >= EMAX_WIDTH) {
              last_row++;
              last_col = 0;
            }
          }
        }
      }
      else { /* WHILE/ALR/BDR is specified */
        /* 1-2b */
        if (row < *rdep) {
	  if (row == *rdep-1 && insn[i].iexe.src1h != insn[i].iexe.exedh) { /* load-exe-store foldingの場合,例えばrow=7,src=6,7 fold=1にセットしてトライ */
	    last_row = row;
	    last_col = col;
	    printf("load-exe-store folding w/o ALU loop assumed ");
	    folding = 1;
	  }
	  else { /* 指定rowには配置困難 */
	    printf("in %s: exe cannot locate destination ALRNO/BDRNO (row=%d)\n", id[current_prefix].name, row);
	    exit(1);
	  }
        }
        else if (row < last_row) { /* 位置を上に戻せない ⇒ sort-msege:戻せるように変更 20221026 */
          //printf("in %s: exe found violation of sequence (row=%d < last_row=%d)\n", id[current_prefix].name, row, last_row);
          //exit(1);
	  last_row = row;
	  last_col = col;
        }
        else {
          last_row = row;
          last_col = col;
        }
      }
      /* 1-3 */
      if (last_row >= EMAX_DEPTH || last_col >= EMAX_WIDTH) { /* copy前に検査 */
        printf("in %s: [%d][%d] exe exceeds EMAX_DEPTH/EMAX_WIDTH\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      if (dec[last_row][last_col].dexu.op1 || dec[last_row][last_col].dexu.op2 || dec[last_row][last_col].dexu.op3) {
        printf("in %s: [%d][%d] exe conflicts\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      /* 1-4 */
      /* check combination of sources */
      /*    src1 src2 src3        src4        src5       */
      /* A: ex1, ex2, ex3 -> EX1, e2i -> EX2, e3i -> EX3 */
      /* B: ex1, ex2      -> EX1, ex3 -> EX2, e3i -> EX3 */
      /* C: ex1, ex2      -> EX1, e2i -> EX2, ex3 -> EX3 */
      /* D: ex1           -> EX1, ex2 -> EX2, e3i -> EX3 */
      /* E: ex1           -> nop, ex2 -> EX2, ex3 -> EX3 */
      /* F: ex1           -> nop, e2i -> EX2, e3i -> EX3 */
      /* G: ex1           -> nop, e2i -> EX2, ex3 -> EX3 */
      dexu = &dec[last_row][last_col].dexu;
      dexu->op1    = insn[i].iexe.op1;
      dexu->op2    = insn[i].iexe.op2;
      dexu->op3    = insn[i].iexe.op3;
      dexu->updt   = insn[i].iexe.updt;
      dexu->init   = insn[i].iexe.init;
      dexu->fold   = folding;
      dexu->ex1v   = insn[i].iexe.src1v; /* default */
      dexu->ex1h   = insn[i].iexe.src1h; /* default */
      dexu->ex1s   = insn[i].iexe.src1s; /* default */
      dexu->ex1e   = insn[i].iexe.src1e; /* default */
      if (insn[i].iexe.src2v) { /* case A,B,C */
        dexu->ex2v   = insn[i].iexe.src2v; /* default */
        dexu->ex2h   = insn[i].iexe.src2h; /* default */
        dexu->ex2s   = insn[i].iexe.src2s; /* default */
        dexu->ex2e   = insn[i].iexe.src2e; /* default */
      }
      if (insn[i].iexe.src3v) { /* case A */
        dexu->ex3v   = insn[i].iexe.src3v; /* default */
        dexu->ex3h   = insn[i].iexe.src3h; /* default */
        dexu->ex3s   = insn[i].iexe.src3s; /* default */
        dexu->ex3e   = insn[i].iexe.src3e; /* default */
      }
      if (insn[i].iexe.src4v==T_IMMEDIATE) { /* case A,C,F,G */
        dexu->e2iv   = insn[i].iexe.src4v; /* default */
        dexu->e2ih   = insn[i].iexe.src4h; /* default */
        dexu->e2is   = 0;                  /* e2imm */
      }
      else if (insn[i].iexe.src4v) { /* case B,D,E */
        if (dexu->ex3v) {
          printf("in %s: insn[%d].iexe has too many T_VARs\n", id[current_prefix].name, i);
          exit(1);
        }
        else if (dexu->ex2v) { /* case B */
          dexu->ex3v   = insn[i].iexe.src4v; /* default */
          dexu->ex3h   = insn[i].iexe.src4h; /* default */
          dexu->ex3s   = insn[i].iexe.src4s; /* default */
          dexu->ex3e   = 0;
          dexu->e2is   = 2;                  /* ex3 */
        }
        else { /* case D,E */
          dexu->ex2v   = insn[i].iexe.src4v; /* default */
          dexu->ex2h   = insn[i].iexe.src4h; /* default */
          dexu->ex2s   = insn[i].iexe.src4s; /* default */
          dexu->ex2e   = 0;
          dexu->e2is   = 1;                  /* ex2 */
        }
      }
      if (insn[i].iexe.src5v==T_IMMEDIATE) { /* case A,B,D,F */
        dexu->e3iv   = insn[i].iexe.src5v; /* default */
        dexu->e3ih   = insn[i].iexe.src5h; /* default */
        dexu->e3is   = 0;                  /* e3imm */
      }
      else if (insn[i].iexe.src5v) { /* case C,E,G */
        if (dexu->ex3v) {
          printf("in %s: insn[%d].iexe has too many T_VARs\n", id[current_prefix].name, i);
          exit(1);
        }
        else { /* case C,E,G */
          dexu->ex3v   = insn[i].iexe.src5v; /* default */
          dexu->ex3h   = insn[i].iexe.src5h; /* default */
          dexu->ex3s   = insn[i].iexe.src5s; /* default */
          dexu->ex3e   = 0;
          dexu->e3is   = 1;                  /* ex3 */
        }
      }
      dexu->exdv   = insn[i].iexe.exedv; /* default */
      dexu->exdh   = insn[i].iexe.exedh; /* default */
      dexu->exds   = insn[i].iexe.exeds; /* default */
      /* 1-5 */
      set_reg_path(last_row, last_col, folding, type, RTYPE_DATA, 0, dexu->ex1v, dexu->ex1h, dexu->ex1s); /* discrete */
      set_reg_path(last_row, last_col, folding, type, RTYPE_DATA, 0, dexu->ex2v, dexu->ex2h, dexu->ex2s); /* discrete */
      set_reg_path(last_row, last_col, folding, type, RTYPE_DATA, 0, dexu->ex3v, dexu->ex3h, dexu->ex3s); /* discrete */
      /* 1-6 *//* 格納先busmapはEXDR止まり */
      bus[last_row][last_col].exdrv = dexu->exdv;
      bus[last_row][last_col].exdrh = dexu->exdh;
      bus[last_row][last_col].exdrs = dexu->exds; /* &VARの場合-1, &VAR[c]の場合c, &AR[r][c]の場合c */
      id[insn[i].iexe.exedh].itype = ITYPE_EXE;
      id[insn[i].iexe.exedh].row   = last_row;
      id[insn[i].iexe.exedh].col   = dexu->exds<0 /* ★★★ exe->&VARの場合,last_colをセット */
                                   ? last_col     /* ★★★ exe->VAR[c],AR[r][c]をバラで使う場合,colでは区別不可 */
                                   : -1;          /* ★★★ col位置は固定なので-1にしておく */
      insn[i].iheader.fixed = 1;
      break;
    case ITYPE_MEX: /* MEX */
      /*****************************************/
      /* old mex(OP_CMPA_LE, &b0[h],       INIT0?b:b0[h],                INIT0?0:8, BR[r][2][1], BR[r][2][0]);*/
      /* old mex(OP_CMPA_GE, &a0[h][CHIP], INIT0?a[h][CHIP]:a0[h][CHIP], INIT0?0:8, BR[r][2][1], BR[r][2][0]);*/
      /* new mex(OP_CMPA_LE, &b0[h][0], INIT0?b[0]:b0[h][0], INIT0?0LL:8LL, OP_CMPA_GE, &a0[h][0][CHIP], INIT0?a[h][CHIP]:a0[h][0][CHIP], INIT0?0LL:8LL, 0LL, BR[r][2][1], BR[r][2][0]);*/
      /* new mex(OP_CMPA_LE, &J[x],     INIT0?0LL:J[x],      INIT0?0LL:8LL, OP_CMPA_GE, &K[x],           INIT0?0LL:K[x],                  INIT0?0LL:8LL, BE8, BR[r][2][1], BR[r][2][0]);*/
      /*     mop(OP_LDR, 3,  &BR[r][2][1], b0[h],       bofs, MSK_W1, b,          2*LP*RMGRP,  0, 0, NULL, 2*LP*RMGRP); */
      /*     mop(OP_LDR, 3,  &BR[r][2][0], a0[h][CHIP], cofs, MSK_W1, a[h][CHIP], 2*LP,        0, 0, NULL, 2*LP); */
      switch (insn[i].imex.op0) {
      case OP_ALWAYS:
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.adr1v, insn[i].imex.adr1h, rdep);
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.adr2v, insn[i].imex.adr2h, rdep);
        break;
      case OP_CMPA_LE:
      case OP_CMPA_GE:
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.adr1v, insn[i].imex.adr1h, rdep);
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.adr2v, insn[i].imex.adr2h, rdep);
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.src1v, insn[i].imex.src1h, rdep);
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.src2v, insn[i].imex.src2h, rdep);
        break;
      }
      switch (insn[i].imex.op1) {
      case OP_ALWAYS:
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.adr3v, insn[i].imex.adr3h, rdep);
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.adr4v, insn[i].imex.adr4h, rdep);
        break;
      case OP_CMPA_LE:
      case OP_CMPA_GE:
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.adr3v, insn[i].imex.adr3h, rdep);
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.adr4v, insn[i].imex.adr4h, rdep);
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.src1v, insn[i].imex.src1h, rdep);
        get_valid_row(ITYPE_MEX, 0, insn[i].imex.src2v, insn[i].imex.src2h, rdep);
        break;
      }

      folding = 1; /* set */

      if (row < 0) { /* OP_ALWAYSの場合,BR[][]指定無し */
        /* 1-2a */
	if (last_row < *rdep) { /* 間を空けて配置可能 */
	  last_row = *rdep;
	  last_col = 0;
	}
	else { /* last_row >= *rdep *//* 最終位置と依存関係制約位置が一致 */
	  last_col++; /* overflowはあとで検査.EXEがWIDTH以上あれば次段へ移動 */
	  if (last_col >= EMAX_WIDTH) {
	    last_row++;
	    last_col = 0;
	  }
	}
      }
      else { /* WHILE/ALR/BDR is specified */
        /* 1-2b */
        if (row < *rdep) {
	  printf("in %s: mex cannot locate destination BDRNO (row=%d)\n", id[current_prefix].name, row);
	  exit(1);
	}
	else if (row < last_row) {
          printf("in %s: mex found violation of sequence (row=%d < last_row=%d)\n", id[current_prefix].name, row, last_row);
          exit(1);
        }
        else {
          last_row = row;
          last_col = col;
        }
      }
      /* 1-3 */
      //printf("ITYPE_MEX: row=%d col=%d rdep=%d last_row=%d last_col=%d\n", row, col, *rdep, last_row, last_col);
      if (last_row >= EMAX_DEPTH || last_col >= EMAX_WIDTH) { /* copy前に検査 */
        printf("in %s: [%d][%d] mex exceeds EMAX_DEPTH/EMAX_WIDTH\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      /* 1-4 */
      dmex  = &dec[last_row][last_col].dmex;
      *dmex = insn[i].imex; /* new mex()は1つで2つ分指定 */
      /* 1-5 */
      /* BR[][][1],BR[][][0]からの戻りは常時接続なのでselector追加不要 */
      /* ea14woofs,ea04woofsからの戻りは常時接続なのでselector追加不要 */
      set_reg_path(last_row, last_col, 0, type, RTYPE_BASE, 0, dmex->adr1v, dmex->adr1h, dmex->adr1s);
      set_reg_path(last_row, last_col, 0, type, RTYPE_BASE, 1, dmex->adr3v, dmex->adr3h, dmex->adr3s);
      /* 1-6 */
      /* BR[][][1],BR[][][0]からの戻りは常時接続なのでbus設定不要 */
      /* ea14woofs,ea04woofsからの戻りは常時接続なのでbus設定不要 */
      bus[last_row][last_col].ea0woofsv = dmex->mexd0v;
      bus[last_row][last_col].ea0woofsh = dmex->mexd0h;
      bus[last_row][last_col].ea1woofsv = dmex->mexd1v;
      bus[last_row][last_col].ea1woofsh = dmex->mexd1h;
      id[insn[i].imex.mexd0h].itype = ITYPE_MEX;
      id[insn[i].imex.mexd0h].row   = last_row;
      id[insn[i].imex.mexd0h].col   = last_col;
      id[insn[i].imex.mexd1h].itype = ITYPE_MEX;
      id[insn[i].imex.mexd1h].row   = last_row;
      id[insn[i].imex.mexd1h].col   = last_col;
      insn[i].iheader.fixed = 1;
      break;
    case ITYPE_MO4: /* MO4 */
      /**************************************************/
      /* 1-1                                            */
      /* MO4 has var([]), alr[]([]), bdr[][]([]) as src */
      /*         store    store      load               */
      /**************************************************/
      switch (insn[i].imop.op) {
      case OP_STRQ:  /* random_store on mop0->TR (+implicit mop1->AXI) */
        current_lmmwb = 1; /* mark as dirty L1 shold be flushed+cleared before starting EMAX */
      case OP_TR:    /* transaction on mop0->TR (+implicit mop1->AXI) */
        get_valid_row(ITYPE_MO4, 1, insn[i].imop.mopdv, insn[i].imop.mopdh, rdep);
      case OP_LDRQ:  /* random_load on mop1->BR (+implicit AXI->mop0->TR) */
      case OP_LDDMQ: /* direct_load on mop0->AXI->TR->BR */
        get_valid_row(ITYPE_MO4, 1, insn[i].imop.exv,   insn[i].imop.exh,   rdep);
        get_valid_row(ITYPE_MO4, 0, insn[i].imop.basev, insn[i].imop.baseh, rdep);
        get_valid_row(ITYPE_MO4, 0, insn[i].imop.offsv, insn[i].imop.offsh, rdep);
        break;
      default:
        printf("in %s: insn[%d].imop.op=%d is undefined\n", id[current_prefix].name, i, insn[i].imop.op);
        exit(1);
      }

      folding = 0; /* reset */

      if (row < 0) { /* none of WHILE/ALR/BDR is specified */
        /* 1-2a */
        if (last_row < *rdep) { /* 間を空けて配置可能 */
          last_row = *rdep;
          last_col = 0; /* 位置指定可能ex4.dstはALRのみ.var経由の同一row内後続st4()を配置可能 */
        }
        else { /* last_row >= *rdep *//* 最終位置と依存関係制約位置が一致 */
          if (dec[last_row][last_col].dmop0.op || dec[last_row][last_col].dmop1.op) { /* 先行写像との競合検査 */
            last_col++; /* overflowはあとで検査.MOP4がWIDTH以上あれば次段へ移動 */
            if (last_col >= EMAX_WIDTH) {
              last_row++;
              last_col = 0;
            }
          }
        }
      }
      else { /* WHILE/ALR/BDR is specified */
        /* 1-2b */
        if (row < *rdep) { /* 指定rowには配置困難 */
          printf("in %s: mo4 cannot locate destination ALRNO/BDRNO (row=%d)\n", id[current_prefix].name, row);
          exit(1);
        }
        else if (row < last_row) {
          printf("in %s: mo4 found violation of sequence (row=%d < last_row=%d)\n", id[current_prefix].name, row, last_row);
          exit(1);
        }
        else {
          last_row = row;
          if (col >= 0) /* AR指定の場合,col=-1なので,colは無変更 */
            last_col = col;
        }
      }
      /* 1-3 */
      if (last_row >= EMAX_DEPTH || last_col >= EMAX_WIDTH) { /* copy前に検査 */
        printf("in %s: [%d][%d] mo4 exceeds EMAX_DEPTH/EMAX_WIDTH\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      if (dec[last_row][last_col].dmop0.op || dec[last_row][last_col].dmop1.op) {
        printf("in %s: [%d][%d] mo4 conflicts\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      /* 1-4 */
      dmop0 = &dec[last_row][last_col].dmop0;
      dmop1 = &dec[last_row][last_col].dmop1;
      if (insn[i].imop.mtype == MTYPE_RLOAD) /* mop1 */
        *dmop1 = insn[i].imop;
      else /* MTYPE_RSTORE, MTYPE_DLOAD, MTYPE_TRANS */
        *dmop0 = insn[i].imop;
      if (insn[i].imop.mtype == MTYPE_DLOAD) { /* MTYPE_DLOADの主記憶ADDR送信設定 */
        /*-------------★★★★★★★★-----------------*/
        /* dec[last_row][last_col].dmop0.op = OP_LDDMQ; /* may be redundant */
        /* setup EX1 for ddr-addr */
        dexu = &dec[last_row][last_col].dexu;
        if (dexu->op1 || dexu->op2 || dexu->op3) {
          printf("in %s: [%d][%d] LDDMQ address generation conflicts in EXU\n", id[current_prefix].name, last_row, last_col);
          exit(1);
        }
        dexu->op1    = OP_ADD;
        dexu->op2    = OP_NOP;
        dexu->op3    = OP_NOP;
        dexu->updt   = 0; /* 0:none, 1:self_update */
	dexu->init   = 0;
	dexu->fold   = 0; /* normal */
        dexu->ex1v   = dmop0->basev; /* id.type */
        dexu->ex1h   = dmop0->baseh; /* hash val */
        dexu->ex1s   = dmop0->bases; /* suffix for var[s], bdr[][][s] */
        dexu->ex1e   = EXP_H3210;    /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
        dexu->ex2v   = dmop0->offsv; /* id.type */
        dexu->ex2h   = dmop0->offsh; /* hash val */
        dexu->ex2s   = dmop0->offss; /* suffix for var[s], bdr[][][s] */
        dexu->ex2e   = EXP_H3210;    /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
        dexu->ex3v   = T_NONE;       /* id.type */
        dexu->ex3h   = -1;           /* hash val */
        dexu->ex3s   = -1;           /* suffix for var[s], bdr[][][s] */
        dexu->ex3e   =  0;           /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
        dexu->e2iv   = T_NONE;       /* id.type */
        dexu->e2ih   = -1;           /* hash val */
        dexu->e2is   =  0;           /* selector 0:e2imm, 1:ex2, 2:ex3 */
        dexu->e3iv   = T_NONE;       /* id.type */
        dexu->e3ih   = -1;           /* hash val */
        dexu->e3is   =  0;           /* selector 0:e3imm, 1:ex3 */
        dexu->exdv   = dmop0->basev; /* id.type */
        dexu->exdh   = dmop0->baseh; /* hash val */
        dexu->exds   = dmop0->bases; /* suffix for var[s], bdr[][][s] */
        dmop1->updt  = 1;            /* for internal update of ea1 */
        dmop1->offsm = 14;           /* for internal update of ea1 */
      }
      else if (insn[i].imop.mtype == MTYPE_TRANS) { /* MTYPE_TRANSのPARAM*4送信設定 */
        /*-------------★★★★★★★★-----------------*/
        dmop1->updt  = 1;            /* for internal update of ea1 */
        dmop1->offsm = 14;           /* for internal update of ea1 */
      }
      /* 1-5 */
      if (insn[i].imop.mtype == MTYPE_RLOAD) { /* mop1 */
        /* LD with force-read=0 and ptop==NULL generates current(lmr) and reuse LMM. same as lmr in EMAX4                     curr  1  top  blk  len   0  0  0 */
        /* LD with force-read=1 and ptop==NULL generates current(lmf) and !reuse LMM. same as lmf in EMAX4                    curr  1  top  blk  len   0  1  0 */
        /* LD with force-read=0 and ptop!=NULL generates current(lmr) and next(lmp). mapdist!=0                               curr  1  top  blk  len   0  0  0 */
        /*                                                                                                                  c+dist  1 ptop  blk  len   0  0  1 */
        /* LD with force-read=0 and ptop!=NULL generates current(lmr) and next(lmp). mapdist==0                  ofs=ptop-top curr  1  top  blk  len   0  0  1 */
        /*                                                                                                               p=1の場合,pref-addrは常にlmmi.top+ofs */
        /* LDDMQ set f=1 and p=1 in lmmc automatically                                                                        curr  1  top  -    -     0  1  1 */
	set_reg_path(last_row, last_col, 0, type, RTYPE_BASE, 1, dmop1->basev, dmop1->baseh, dmop1->bases);
        set_reg_path(last_row, last_col, 0, type, RTYPE_OFFS, 1, dmop1->offsv, dmop1->offsh, dmop1->offss);
        if (dmop1->topv  == T_VARIABLE) {
	  int rw = (dmop1->mtype==MTYPE_RLOAD||dmop1->mtype==MTYPE_DLOAD)?0:1;
	  int f  = id[dmop1->forceh].val||dmop1->forcev==T_VARIABLE; /* force=変数の場合もf=1として扱う 20240410 Nakashima */
	  int p  = 0; /* initial value */
	  switch ((rw<<2)|(f<<1)|p) { /* AXI->LMM write対象(lmr/lmf/lmp/lmxの場合1:rw_f_p=000,010,001,110) *//* AXI<-LMM read対象(lmw/lmx/lmdの場合1:rw_f_p=100,110,101) */
	  case 0: /* lmr */ case 1: /* lmp */ case 2: /* lmf */ conf[last_row][last_col].cdw2.lmm_axiw = 1; conf[last_row][last_col].cdw2.lmm_axir = 0; break;
	  case 4: /* lmw */ case 5: /* lmd */                   conf[last_row][last_col].cdw2.lmm_axiw = 0; conf[last_row][last_col].cdw2.lmm_axir = 1; break;
	  case 6: /* lmx */                                     conf[last_row][last_col].cdw2.lmm_axiw = 1; conf[last_row][last_col].cdw2.lmm_axir = 1; break;
	  case 3: /* lddmq */ case 7: /* tr */                  conf[last_row][last_col].cdw2.lmm_axiw = 0; conf[last_row][last_col].cdw2.lmm_axir = 0; break;
	  }
	  conf[last_row][last_col].cdw2.lmm_mode = 3;
	  lmmi[last_row][last_col].v     = 1;
	  lmmi[last_row][last_col].rw    = rw;
	  lmmi[last_row][last_col].f     = f;
	  lmmi[last_row][last_col].p     = p;
	  lmmi[last_row][last_col].blk   = dmop1->blk;
	  lmmi[last_row][last_col].cidx  = id[dmop1->toph].cidx;
	  lmmi[last_row][last_col].len   = id[dmop1->lenh].val-1;
	  lmmi[last_row][last_col].ofs   = 0; /* initial value */
	  lmmi[last_row][last_col].top   = (Ull)id[dmop1->toph].name;
	  lmmx[last_row][last_col].forcev= dmop1->forcev;
	  lmmx[last_row][last_col].forceh= dmop1->forceh;
	  lmmx[last_row][last_col].lenv  = dmop1->lenv;
	  lmmx[last_row][last_col].lenh  = dmop1->lenh;
        }
        if (dmop1->ptopv == T_VARIABLE) { /* lmp */
          if (last_row+current_mapdist >= EMAX_DEPTH) { /* copy前に検査 */
            printf("in %s: [%d][%d] prefetch exceeds EMAX_DEPTH\n", id[current_prefix].name, last_row, last_col);
            exit(1);
          }
          if (dec[last_row+current_mapdist][last_col].dmop0.op) {
            printf("in %s: [%d][%d] prefetch may conflict with other mop\n", id[current_prefix].name, last_row, last_col);
            exit(1);
          }
          if (current_mapdist==0) { /* old_LDBFQ */
            if (id[dmop1->forceh].val) { /* reserved for LDDMQ */
              /* f=1を指定してもignored */
            }
	    lmmi[last_row][last_col].p   = 1; /* optional prefetch *//* lmm_axiw/lmm_axirは，lmrとlmpとで同じ扱いなので設定済 */
	    lmmi[last_row][last_col].ofs = (Uint)(id[dmop1->ptoph].name - id[dmop1->toph].name);
          }
          else {
	    int rw = (dmop1->mtype==MTYPE_RLOAD||dmop1->mtype==MTYPE_DLOAD)?0:1;
	    int f  = 0;
	    int p  = 1;
	    switch ((rw<<2)|(f<<1)|p) { /* AXI->LMM write対象(lmr/lmf/lmp/lmxの場合1:rw_f_p=000,010,001,110) *//* AXI<-LMM read対象(lmw/lmx/lmdの場合1:rw_f_p=100,110,101) */
	    case 0: /* lmr */ case 1: /* lmp */ case 2: /* lmf */ conf[last_row+current_mapdist][last_col].cdw2.lmm_axiw = 1; conf[last_row+current_mapdist][last_col].cdw2.lmm_axir = 0; break;
	    case 4: /* lmw */ case 5: /* lmd */                   conf[last_row+current_mapdist][last_col].cdw2.lmm_axiw = 0; conf[last_row+current_mapdist][last_col].cdw2.lmm_axir = 1; break;
	    case 6: /* lmx */                                     conf[last_row+current_mapdist][last_col].cdw2.lmm_axiw = 1; conf[last_row+current_mapdist][last_col].cdw2.lmm_axir = 1; break;
	    case 3: /* lddmq */ case 7: /* tr */                  conf[last_row+current_mapdist][last_col].cdw2.lmm_axiw = 0; conf[last_row+current_mapdist][last_col].cdw2.lmm_axir = 0; break;
	    }
	    conf[last_row+current_mapdist][last_col].cdw2.lmm_mode = 3;
	    lmmi[last_row+current_mapdist][last_col].v     = 1;
	    lmmi[last_row+current_mapdist][last_col].rw    = rw;
	    lmmi[last_row+current_mapdist][last_col].f     = f;
	    lmmi[last_row+current_mapdist][last_col].p     = p;
	    lmmi[last_row+current_mapdist][last_col].blk   = dmop1->blk;
	    lmmi[last_row+current_mapdist][last_col].cidx  = id[dmop1->ptoph].cidx;
	    lmmi[last_row+current_mapdist][last_col].len   = id[dmop1->plenh].val-1;
	    lmmi[last_row+current_mapdist][last_col].ofs   = 0;
	    lmmi[last_row+current_mapdist][last_col].top   = (Ull)id[dmop1->ptoph].name;
	    lmmx[last_row+current_mapdist][last_col].forcev= dmop1->forcev;
	    lmmx[last_row+current_mapdist][last_col].forceh= dmop1->forceh;
	    lmmx[last_row+current_mapdist][last_col].lenv  = dmop1->plenv;
	    lmmx[last_row+current_mapdist][last_col].lenh  = dmop1->plenh;
          }
        }
      }
      else { /* MTYPE_RSTORE, MTYPE_DLOAD, MTYPE_TRANS */
        /* ST with force-read=0 and ptop==NULL generates current(lmw) and reuse+wback LMM. same as lmw in EMAX4               curr  1  top  blk  len   1  0  0 */
        /* ST with force-read=1 and ptop==NULL generates current(lmx) and !reuse+wback LMM. same as lmx in EMAX4              curr  1  top  blk  len   1  1  0 */
        /* ST with force-read=0 and ptop!=NULL generates current(lmw) and prev(lmd). mapdist!=0                               curr  1  top  blk  len   1  0  0 */
        /*                                                                                                                  c-dist  1 ptop  blk  len   1  0  1 */
        /* ST with force-read=0 and ptop!=NULL generates current(lmw) and prev(lmd). mapdist==0                  ofs=ptop-top curr  1  top  blk  len   1  0  1 */
        /*                                                                                                              p=1の場合,drain-addrは常にlmmi.top+ofs */
        /* TR    set f=1 and p=1 in lmmc automatically                                                                        curr  1  top  -    -     1  1  1 */
        if (insn[i].imop.mtype == MTYPE_RSTORE) { /* 3,2,1,0 */
          set_reg_path(last_row, last_col, 0, type, RTYPE_BASE, 0, dmop0->basev, dmop0->baseh, dmop0->bases);
          set_reg_path(last_row, last_col, 0, type, RTYPE_OFFS, 0, dmop0->offsv, dmop0->offsh, dmop0->offss);
          for (j=0; j<UNIT_WIDTH; j++)
            set_reg_path(last_row, last_col, 0, type, RTYPE_DATA, 0, dmop0->mopdv, dmop0->mopdh, j);
        }
        else if (insn[i].imop.mtype == MTYPE_DLOAD) {
          set_reg_path(last_row, last_col, 0, 4 /*EXE*/, RTYPE_DATA, 0, dmop0->basev, dmop0->baseh, dmop0->bases);
          set_reg_path(last_row, last_col, 0, 4 /*EXE*/, RTYPE_DATA, 0, dmop0->offsv, dmop0->offsh, dmop0->offss);
        }
        else if (insn[i].imop.mtype == MTYPE_TRANS) { /* 3,2,1,0 */
          for (j=0; j<=last_col; j++) /* OP_TR can accept empty portion */
            set_reg_path(last_row, last_col, 0, type, RTYPE_DATA, 0, dmop0->mopdv, dmop0->mopdh, j);
        }
        if (dmop0->topv  == T_VARIABLE) {
	  int rw = (dmop0->mtype==MTYPE_RLOAD||dmop0->mtype==MTYPE_DLOAD)?0:1;
	  int f  = (dmop0->mtype==MTYPE_DLOAD||dmop0->mtype==MTYPE_TRANS)?1:id[dmop0->forceh].val||dmop0->forcev==T_VARIABLE; /* force=変数の場合もf=1として扱う 20240410 Nakashima */
	  int p  = (dmop0->mtype==MTYPE_DLOAD||dmop0->mtype==MTYPE_TRANS)?1:0; /* initial value */
	  switch ((rw<<2)|(f<<1)|p) { /* AXI->LMM write対象(lmr/lmf/lmp/lmxの場合1:rw_f_p=000,010,001,110) *//* AXI<-LMM read対象(lmw/lmx/lmdの場合1:rw_f_p=100,110,101) */
	  case 0: /* lmr */ case 1: /* lmp */ case 2: /* lmf */ conf[last_row][last_col].cdw2.lmm_axiw = 1; conf[last_row][last_col].cdw2.lmm_axir = 0; break;
	  case 4: /* lmw */ case 5: /* lmd */                   conf[last_row][last_col].cdw2.lmm_axiw = 0; conf[last_row][last_col].cdw2.lmm_axir = 1; break;
	  case 6: /* lmx */                                     conf[last_row][last_col].cdw2.lmm_axiw = 1; conf[last_row][last_col].cdw2.lmm_axir = 1; break;
	  case 3: /* lddmq */ case 7: /* tr */                  conf[last_row][last_col].cdw2.lmm_axiw = 0; conf[last_row][last_col].cdw2.lmm_axir = 0; break;
	  }
	  conf[last_row][last_col].cdw2.lmm_mode = 3;
	  lmmi[last_row][last_col].v     = 1;
	  lmmi[last_row][last_col].rw    = rw;
	  lmmi[last_row][last_col].f     = f;
	  lmmi[last_row][last_col].p     = p;
	  lmmi[last_row][last_col].blk   = dmop0->blk;
	  lmmi[last_row][last_col].cidx  = id[dmop0->toph].cidx;
	  lmmi[last_row][last_col].len   = id[dmop0->lenh].val-1;
	  lmmi[last_row][last_col].ofs   = 0; /* initial value */
	  lmmi[last_row][last_col].top   = (Ull)id[dmop0->toph].name;
	  lmmx[last_row][last_col].forcev= dmop0->forcev;
	  lmmx[last_row][last_col].forceh= dmop0->forceh;
	  lmmx[last_row][last_col].lenv  = dmop0->lenv;
	  lmmx[last_row][last_col].lenh  = dmop0->lenh;
        }
        else { /* LDDMQはtopv==NULL */
          if (insn[i].imop.mtype == MTYPE_DLOAD) {
	    int rw = 0;
	    int f  = 1;
	    int p  = 1;
	    switch ((rw<<2)|(f<<1)|p) { /* AXI->LMM write対象(lmr/lmf/lmp/lmxの場合1:rw_f_p=000,010,001,110) *//* AXI<-LMM read対象(lmw/lmx/lmdの場合1:rw_f_p=100,110,101) */
	    case 0: /* lmr */ case 1: /* lmp */ case 2: /* lmf */ conf[last_row][last_col].cdw2.lmm_axiw = 1; conf[last_row][last_col].cdw2.lmm_axir = 0; break;
	    case 4: /* lmw */ case 5: /* lmd */                   conf[last_row][last_col].cdw2.lmm_axiw = 0; conf[last_row][last_col].cdw2.lmm_axir = 1; break;
	    case 6: /* lmx */                                     conf[last_row][last_col].cdw2.lmm_axiw = 1; conf[last_row][last_col].cdw2.lmm_axir = 1; break;
	    case 3: /* lddmq */ case 7: /* tr */                  conf[last_row][last_col].cdw2.lmm_axiw = 0; conf[last_row][last_col].cdw2.lmm_axir = 0; break;
	    }
	    conf[last_row][last_col].cdw2.lmm_mode = 3;
	    lmmi[last_row][last_col].v     = 1;
	    lmmi[last_row][last_col].rw    = rw;
	    lmmi[last_row][last_col].f     = f;
	    lmmi[last_row][last_col].p     = p;
	    lmmi[last_row][last_col].blk   = 0;
	    lmmi[last_row][last_col].cidx  = id[dmop0->toph].cidx;
	    lmmi[last_row][last_col].len   = 0;
	    lmmi[last_row][last_col].ofs   = 0;
	    lmmi[last_row][last_col].top   = (Ull)id[dmop0->toph].name;
	    lmmx[last_row][last_col].forcev= dmop0->forcev;
	    lmmx[last_row][last_col].forceh= dmop0->forceh;
	    lmmx[last_row][last_col].lenv  = dmop0->lenv;
	    lmmx[last_row][last_col].lenh  = dmop0->lenh;
          }
        }
        if (dmop0->ptopv == T_VARIABLE) { /* lmd */
          if (last_row-current_mapdist < 0) { /* copy前に検査 */
            printf("in %s: [%d][%d] drain exceeds EMAX_DEPTH\n", id[current_prefix].name, last_row, last_col);
            exit(1);
          }
          if (dec[last_row-current_mapdist][last_col].dmop1.op) {
            printf("in %s: [%d][%d] drain may conflict with other mop\n", id[current_prefix].name, last_row, last_col);
            exit(1);
          }
          if (current_mapdist==0) { /* old_STBFQ */
            if (id[dmop0->forceh].val) { /* reserved for TR */
              /* f=1を指定してもignored */
            }
	    lmmi[last_row][last_col].p   = 1; /* optional postdrain *//* lmm_axiw/lmm_axirは，lmwとlmdとで同じ扱いなので設定済 */
	    lmmi[last_row][last_col].ofs = (Uint)(id[dmop0->ptoph].name - id[dmop0->toph].name);
          }
          else {
	    if (lmmi[last_row-current_mapdist][last_col].v) {
	      printf("in %s: [%d][%d] drain may conflict with other lmm\n", id[current_prefix].name, last_row, last_col);
	      exit(1);
	    }
	    int rw = (dmop0->mtype==MTYPE_RLOAD||dmop0->mtype==MTYPE_DLOAD)?0:1;
	    int f  = 0;
	    int p  = 1;
	    switch ((rw<<2)|(f<<1)|p) { /* AXI->LMM write対象(lmr/lmf/lmp/lmxの場合1:rw_f_p=000,010,001,110) *//* AXI<-LMM read対象(lmw/lmx/lmdの場合1:rw_f_p=100,110,101) */
	    case 0: /* lmr */ case 1: /* lmp */ case 2: /* lmf */ conf[last_row-current_mapdist][last_col].cdw2.lmm_axiw = 1; conf[last_row-current_mapdist][last_col].cdw2.lmm_axir = 0; break;
	    case 4: /* lmw */ case 5: /* lmd */                   conf[last_row-current_mapdist][last_col].cdw2.lmm_axiw = 0; conf[last_row-current_mapdist][last_col].cdw2.lmm_axir = 1; break;
	    case 6: /* lmx */                                     conf[last_row-current_mapdist][last_col].cdw2.lmm_axiw = 1; conf[last_row-current_mapdist][last_col].cdw2.lmm_axir = 1; break;
	    case 3: /* lddmq */ case 7: /* tr */                  conf[last_row-current_mapdist][last_col].cdw2.lmm_axiw = 0; conf[last_row-current_mapdist][last_col].cdw2.lmm_axir = 0; break;
	    }
	    conf[last_row-current_mapdist][last_col].cdw2.lmm_mode = 3;
	    lmmi[last_row-current_mapdist][last_col].v     = 1;
	    lmmi[last_row-current_mapdist][last_col].rw    = rw;
	    lmmi[last_row-current_mapdist][last_col].f     = f;
	    lmmi[last_row-current_mapdist][last_col].p     = p;
	    lmmi[last_row-current_mapdist][last_col].blk   = dmop0->blk;
	    lmmi[last_row-current_mapdist][last_col].cidx  = id[dmop0->ptoph].cidx;
	    lmmi[last_row-current_mapdist][last_col].len   = id[dmop0->plenh].val-1;
	    lmmi[last_row-current_mapdist][last_col].ofs   = 0;
	    lmmi[last_row-current_mapdist][last_col].top   = (Ull)id[dmop0->ptoph].name;
	    lmmx[last_row-current_mapdist][last_col].forcev= dmop0->forcev;
	    lmmx[last_row-current_mapdist][last_col].forceh= dmop0->forceh;
	    lmmx[last_row-current_mapdist][last_col].lenv  = dmop0->plenv;
	    lmmx[last_row-current_mapdist][last_col].lenh  = dmop0->plenh;
          }
        }
      }
      /* 1-6 *//* RLOAD,DLOAD:格納先busmapはBR止まり RSTORE,TRANS: 格納先busmapはMW止まり */
      if (insn[i].imop.mtype == MTYPE_RLOAD) {
        if (dmop1->topv  == T_VARIABLE) {
          for (j=0; j<UNIT_WIDTH; j++) {
            bus[last_row][last_col].lmwd[j].v = dmop1->topv;
            bus[last_row][last_col].lmwd[j].h = dmop1->toph;
            bus[last_row][last_col].lmwd[j].s = j;
            /* mwは実行時設定を優先(STATUS_LOADが実行時設定を無視) */
          }
        }
        if (dmop1->ptopv == T_VARIABLE) { /* lmp */
          dec[last_row+current_mapdist][last_col].dmop0.op = OP_IM_PREF; /* mapdist=0含む */
          for (j=0; j<UNIT_WIDTH; j++) {
            bus[last_row+current_mapdist][last_col].lmwd[j].v = dmop1->ptopv;
            bus[last_row+current_mapdist][last_col].lmwd[j].h = dmop1->ptoph;
            bus[last_row+current_mapdist][last_col].lmwd[j].s = j;
            bus[last_row+current_mapdist][last_col].mw[j].v = dmop1->ptopv;
            bus[last_row+current_mapdist][last_col].mw[j].h = dmop1->ptoph;
            bus[last_row+current_mapdist][last_col].mw[j].s = j;
          }
        }
        for (j=0; j<UNIT_WIDTH; j++) {
          bus[last_row][last_col].br[j].v = dmop1->mopdv;
          bus[last_row][last_col].br[j].h = dmop1->mopdh;
          bus[last_row][last_col].br[j].s = j;
        }
        conf[last_row][last_col].cdw2.brs0 = 1; /* 1:mr10 */
        conf[last_row][last_col].cdw2.brs1 = 1; /* 1:mr11 */
        conf[last_row][last_col].cdw2.brs2 = 1; /* 1:mr12 */
        conf[last_row][last_col].cdw2.brs3 = 1; /* 1:mr13 */
        id[insn[i].imop.mopdh].itype = ITYPE_MO4;
        id[insn[i].imop.mopdh].row   = last_row;
        id[insn[i].imop.mopdh].col   = last_col;
      }
      else if (insn[i].imop.mtype == MTYPE_DLOAD) { /* MTYPE_DLOADの主記憶ADDR送信設定 */
        /*-------------★★★★★★★★-----------------*/
        bus[last_row][last_col].exdrv = dexu->exdv;
        bus[last_row][last_col].exdrh = dexu->exdh;
        bus[last_row][last_col].exdrs = dexu->exds; /* &VARの場合-1, &VAR[c]の場合c, &AR[r][c]の場合c */
        /* setup LMM as FIFO */
        bus[last_row][last_col].ea0brv = T_IMMEDIATE; /* drain offset=32 通常lmmiとの競合検査のため場所予約 */
        bus[last_row][last_col].ea0brh = -1;
        bus[last_row][last_col].ea0orv = T_IMMEDIATE; /* drain offset=32 通常lmmiとの競合検査のため場所予約 */
        bus[last_row][last_col].ea0orh = -1;
        bus[last_row][last_col].ea1brv = T_IMMEDIATE; /* drain offset=32 通常lmmiとの競合検査のため場所予約 */
        bus[last_row][last_col].ea1brh = -1;
        bus[last_row][last_col].ea1orv = T_IMMEDIATE; /* drain offset=32 通常lmmiとの競合検査のため場所予約 */
        bus[last_row][last_col].ea1orh = -1;
        regv[last_row][last_col].ea0b_v = T_IMMEDIATE;
        regv[last_row][last_col].ea0b_h = hash_reg_immediate(0LL);
        regv[last_row][last_col].ea0b_s = -1;
        regv[last_row][last_col].ea0o_v = T_IMMEDIATE;
        regv[last_row][last_col].ea0o_h = hash_reg_immediate(32LL);
        regv[last_row][last_col].ea0o_s = -1;
        regv[last_row][last_col].ea1b_v = T_IMMEDIATE;
        regv[last_row][last_col].ea1b_h = hash_reg_immediate(0LL);
        regv[last_row][last_col].ea1b_s = -1;
        regv[last_row][last_col].ea1o_v = T_IMMEDIATE;
        regv[last_row][last_col].ea1o_h = hash_reg_immediate(32LL);
        regv[last_row][last_col].ea1o_s = -1;

        bus[last_row][last_col].mw[0].v = dmop0->basev;
        bus[last_row][last_col].mw[0].h = dmop0->baseh;
        bus[last_row][last_col].mw[0].s = dmop0->bases;
        bus[last_row][last_col].lmrd[0].v = dmop0->basev; /* for DLOAD-addr */
        bus[last_row][last_col].lmrd[0].h = dmop0->baseh; /* for DLOAD-addr */
        bus[last_row][last_col].lmrd[0].s = dmop0->bases; /* for DLOAD-addr */
        for (j=0; j<UNIT_WIDTH; j++) {
          bus[last_row][last_col].lmwd[j].v = dmop0->mopdv;
          bus[last_row][last_col].lmwd[j].h = dmop0->mopdh;
          bus[last_row][last_col].lmwd[j].s = j;
          bus[last_row][last_col].tr[j].v   = dmop0->mopdv;
          bus[last_row][last_col].tr[j].h   = dmop0->mopdh;
          bus[last_row][last_col].tr[j].s   = j;
          bus[last_row][last_col].br[j].v   = dmop0->mopdv;
          bus[last_row][last_col].br[j].h   = dmop0->mopdh;
          bus[last_row][last_col].br[j].s   = j;
        }
        conf[last_row][last_col].cdw2.brs0 = 2; /* 2:tr0 */
        conf[last_row][last_col].cdw2.brs1 = 2; /* 2:tr1 */
        conf[last_row][last_col].cdw2.brs2 = 2; /* 2:tr2 */
        conf[last_row][last_col].cdw2.brs3 = 2; /* 2:tr3 */
        id[insn[i].imop.mopdh].itype = ITYPE_MO4;
        id[insn[i].imop.mopdh].row   = last_row;
        id[insn[i].imop.mopdh].col   = last_col;
      }
      else if (insn[i].imop.mtype == MTYPE_TRANS) { /* MTYPE_TRANSのPARAM*4送信設定 */
        /*-------------★★★★★★★★-----------------*/
        /* dec[last_row][last_col].dmop0.op = OP_TR; /* may be redundant */
        /* setup LMM as FIFO */
        bus[last_row][last_col].ea0brv = T_IMMEDIATE; /* drain offset=32 通常lmmiとの競合検査のため場所予約 */
        bus[last_row][last_col].ea0brh = -1;
        bus[last_row][last_col].ea0orv = T_IMMEDIATE; /* drain offset=32 通常lmmiとの競合検査のため場所予約 */
        bus[last_row][last_col].ea0orh = -1;
        bus[last_row][last_col].ea1brv = T_IMMEDIATE; /* drain offset=32 通常lmmiとの競合検査のため場所予約 */
        bus[last_row][last_col].ea1brh = -1;
        bus[last_row][last_col].ea1orv = T_IMMEDIATE; /* drain offset=32 通常lmmiとの競合検査のため場所予約 */
        bus[last_row][last_col].ea1orh = -1;
        regv[last_row][last_col].ea0b_v = T_IMMEDIATE;
        regv[last_row][last_col].ea0b_h = hash_reg_immediate(0LL);
        regv[last_row][last_col].ea0b_s = -1;
        regv[last_row][last_col].ea0o_v = T_IMMEDIATE;
        regv[last_row][last_col].ea0o_h = hash_reg_immediate(32LL);
        regv[last_row][last_col].ea0o_s = -1;
        regv[last_row][last_col].ea1b_v = T_IMMEDIATE;
        regv[last_row][last_col].ea1b_h = hash_reg_immediate(0LL);
        regv[last_row][last_col].ea1b_s = -1;
        regv[last_row][last_col].ea1o_v = T_IMMEDIATE;
        regv[last_row][last_col].ea1o_h = hash_reg_immediate(32LL);
        regv[last_row][last_col].ea1o_s = -1;

        for (j=0; j<=last_col; j++) { /* OP_TR can accept empty portion */
          bus[last_row][last_col].mw[j].v = dmop0->mopdv;
          bus[last_row][last_col].mw[j].h = dmop0->mopdh;
          bus[last_row][last_col].mw[j].s = j;
          bus[last_row][last_col].lmrd[j].v = dmop0->mopdv;
          bus[last_row][last_col].lmrd[j].h = dmop0->mopdh;
          bus[last_row][last_col].lmrd[j].s = j;
        }
      }
      else { /* MTYPE_RSTORE */
        if (dmop0->ptopv == T_VARIABLE) { /* lmd */
          dec[last_row-current_mapdist][last_col].dmop1.op = OP_IM_DRAIN; /* mapdist=0含む */
          for (j=0; j<UNIT_WIDTH; j++) {
            bus[last_row-current_mapdist][last_col].lmrd[j].v = dmop0->ptopv;
            bus[last_row-current_mapdist][last_col].lmrd[j].h = dmop0->ptoph;
            bus[last_row-current_mapdist][last_col].lmrd[j].s = j;
          }
        }
        for (j=0; j<UNIT_WIDTH; j++) {
          bus[last_row][last_col].mw[j].v = dmop0->mopdv;
          bus[last_row][last_col].mw[j].h = dmop0->mopdh;
          bus[last_row][last_col].mw[j].s = j;
        }
      }
      insn[i].iheader.fixed = 1;
      break;
    case ITYPE_MOP: /* MOP */
      /***************************************************************/
      /* 1-1                                                         */
      /* MOP has &var,   &var[], &gpr,   &alr[][], &bdr[][][] as src */
      /*         store1, store1, store1, store1,   load1             */
      /***************************************************************/
      switch (insn[i].imop.op) {
      case OP_STR:  /* 64bit */
      case OP_STWR: /* 32bit */
//    case OP_STHR: /* 16bit */
      case OP_STBR: /*  8bit */
        current_lmmwb = 1; /* mark as dirty L1 shold be flushed+cleared before starting EMAX */
        get_valid_row(ITYPE_MOP, 1, insn[i].imop.mopdv, insn[i].imop.mopdh, rdep);
      case OP_LDR:  /* 64bit */
      case OP_LDWR: /* 32bit unsigned */
//    case OP_LDHR: /* 16bit unsigned */
      case OP_LDBR: /*  8bit unsigned */
        get_valid_row(ITYPE_MOP, 1, insn[i].imop.exv,   insn[i].imop.exh,   rdep);
        get_valid_row(ITYPE_MOP, 0, insn[i].imop.basev, insn[i].imop.baseh, rdep);
        get_valid_row(ITYPE_MOP, 0, insn[i].imop.offsv, insn[i].imop.offsh, rdep);
        break;
      default:
        printf("in %s: mop found illegal op=%d\n", id[current_prefix].name, insn[i].imop.op);
        exit(1);
      }

      folding = 0; /* reset */

      if (insn[i].imop.mtype == MTYPE_RLOAD)
        last_mop = 1;
      else /* MTYPE_RSTORE, MTYPE_DLOAD, MTYPE_TRANS */
        last_mop = 0;
      if (row < 0) { /* none of WHILE/ALR/BDR is specified */
        /* 1-2a */
        if (last_row < *rdep) { /* 間を空けて配置可能 */
          last_row = *rdep;
          last_col = 0; /* 位置指定可能ex4.dstはALRのみ.var経由の同一row内後続st4()を配置可能 */
        }
        else { /* last_row >= *rdep *//* 最終位置と依存関係制約位置が一致 */
          int op0 = dec[last_row][last_col].dmop0.op;
          int op1 = dec[last_row][last_col].dmop1.op;
          int typ = dec[last_row][last_col].dmop1.mtype;
          int tpv = dec[last_row][last_col].dmop1.topv;
          int tph = dec[last_row][last_col].dmop1.toph;
          int lev = dec[last_row][last_col].dmop1.lenv;
          int leh = dec[last_row][last_col].dmop1.lenh;
          int blk = dec[last_row][last_col].dmop1.blk;
          int fcev= dec[last_row][last_col].dmop1.forcev;
          int fceh= dec[last_row][last_col].dmop1.forceh;
          int ldx2_and_op0_is_empty = (op1 && !op0) && (typ == MTYPE_RLOAD)
                                   && (typ == insn[i].imop.mtype)
                                   && (tpv == insn[i].imop.topv)
                                   && (tph == insn[i].imop.toph)
                                   && (lev == insn[i].imop.lenv)
                                   && (leh == insn[i].imop.lenh)
                                   && (blk == insn[i].imop.blk)
                                   && (fcev== insn[i].imop.forcev)
                                   && (fceh== insn[i].imop.forceh);
          if (ldx2_and_op0_is_empty)
            last_mop = 0; /* secoundary load */
	  else if (insn[i].imop.mtype == MTYPE_RSTORE) { /* first pair of EXE0->ST0;EXE1->ST1 */
	    for (j=0; j<EMAX_WIDTH; j++) {
	      if (insn[i].imop.mopdh == dec[last_row][j].dexu.exdh) {
		last_col = j;
		break;
	      }
	    }
	    if (dec[last_row][last_col].dexu.fold) { /* exeがfoldなら同一unit.storeも無条件でfold */
	      printf("load-exe-store folding assumed ");
	      folding = 1; /* load-exe-store folding */
	    }
	  }
	  else if (op0 && !op1 && insn[i].imop.mtype == MTYPE_RLOAD) { /* double buffering (st+ld) */
	    /* allocate LD at the same col as ST */
	  }
          else if (op0 || op1) { /* mop0(l/s)=full */
	    /* new_load,new_storeは次colへ */
	    last_col++; /* overflowはあとで検査.EXEがWIDTH以上あれば次段へ移動 */
	    if (last_col >= EMAX_WIDTH) {
	      last_row++;
	      last_col = 0;
	    }
          }
        }
      }
      else { /* WHILE/ALR/BDR is specified */
        /* 1-2b */
        if (row < *rdep) { /* 指定rowには配置困難 */
          printf("in %s: mop cannot locate destination ALRNO/BDRNO (row=%d)\n", id[current_prefix].name, row);
          exit(1);
        }
        else if (row < last_row) {
          printf("in %s: mop found violation of sequence (row=%d < last_row=%d)\n", id[current_prefix].name, row, last_row);
          exit(1);
        }
        else {
          last_row = row;
          last_col = col;
          int op0 = dec[last_row][last_col].dmop0.op;
          int op1 = dec[last_row][last_col].dmop1.op;
          int typ = dec[last_row][last_col].dmop1.mtype;
          int tpv = dec[last_row][last_col].dmop1.topv;
          int tph = dec[last_row][last_col].dmop1.toph;
          int lev = dec[last_row][last_col].dmop1.lenv;
          int leh = dec[last_row][last_col].dmop1.lenh;
          int blk = dec[last_row][last_col].dmop1.blk;
          int fcev= dec[last_row][last_col].dmop1.forcev;
          int fceh= dec[last_row][last_col].dmop1.forceh;
          int ldx2_and_op0_is_empty = (op1 && !op0) && (typ == MTYPE_RLOAD)
                                   && (typ == insn[i].imop.mtype)
                                   && (tpv == insn[i].imop.topv)
                                   && (tph == insn[i].imop.toph)
                                   && (lev == insn[i].imop.lenv)
                                   && (leh == insn[i].imop.lenh)
                                   && (blk == insn[i].imop.blk)
                                   && (fcev== insn[i].imop.forcev)
                                   && (fceh== insn[i].imop.forceh);
          int mex2_and_op0_is_empty = (op1 && !op0) && (typ == MTYPE_RLOAD)
                                   && (typ == insn[i].imop.mtype)
                                   && (blk == insn[i].imop.blk)
                                   && (fcev== insn[i].imop.forcev)
                                   && (fceh== insn[i].imop.forceh)
	                           && (dec[last_row][last_col].dmex.op0)
	                           && (dec[last_row][last_col].dmex.op1);
          if (ldx2_and_op0_is_empty)
            last_mop = 0; /* load */
	  else if (mex2_and_op0_is_empty) /* mex should merge op1(map to LMM/col2) and op0(map to LMM/col1) w/ different top */
	    last_mop = 0; /* mex load */
	  else if (insn[i].imop.mtype == MTYPE_RSTORE) { /* first pair of EXE0->ST0;EXE1->ST1 */
	    if (dec[last_row][last_col].dexu.fold) { /* exeがfoldなら同一unit.storeも無条件でfold */
	      printf("load-exe-store folding assumed ");
	      folding = 1; /* load-exe-store folding */
	    }
	  }
        }
      }
      /* 1-3 */
      if (last_row >= EMAX_DEPTH || last_col >= EMAX_WIDTH) { /* copy前に検査 */
        printf("in %s: [%d][%d] mop exceeds EMAX_DEPTH/EMAX_WIDTH\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      if (last_mop==0 && dec[last_row][last_col].dmop0.op) {
        printf("in %s: [%d][%d] mop conflicts\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      if (last_mop==1 && dec[last_row][last_col].dmop1.op) {
        printf("in %s: [%d][%d] mop conflicts\n", id[current_prefix].name, last_row, last_col);
        exit(1);
      }
      /* 1-4 */
      dmex  = &dec[last_row][last_col].dmex;
      dmop0 = &dec[last_row][last_col].dmop0;
      dmop1 = &dec[last_row][last_col].dmop1;
      if (last_mop==1) { /* load */
        *dmop1 = insn[i].imop;
	if (dmex->op1 && insn[i].imex.op1) {
	  printf("in %s: [%d][%d] mex1 & mop1(adr++) conflicts\n", id[current_prefix].name, last_row, last_col);
	  exit(1);
	}
	else if (insn[i].imex.op0) { /* mop()のINCR情報はmex0にある */
	  dmex->op1    = insn[i].imex.op0;
	  dmex->dist2v = insn[i].imex.dist1v;
	  dmex->dist2h = insn[i].imex.dist1h;
	}
      }
      else { /* store/load */
        *dmop0 = insn[i].imop;
	if (dmex->op0 && insn[i].imex.op0) {
	  printf("in %s: [%d][%d] mex0 & mop0(adr++) conflicts\n", id[current_prefix].name, last_row, last_col);
	  exit(1);
	}
	else if (insn[i].imex.op0) { /* mop()のINCR情報はmex0にある */
	  *dmex = insn[i].imex;
	  dmex->op0    = insn[i].imex.op0;
	  dmex->dist1v = insn[i].imex.dist1v;
	  dmex->dist1h = insn[i].imex.dist1h;
	}
      }
      /* 1-5 */
      if (last_mop==1) { /* load */
        /* LD with force-read=0 and ptop==NULL generates current(lmr) and reuse LMM. same as lmr in EMAX4                     curr  1  top  blk  len   0  0  0 */
        /* LD with force-read=1 and ptop==NULL generates current(lmf) and !reuse LMM. same as lmf in EMAX4                    curr  1  top  blk  len   0  1  0 */
        /* LD with force-read=0 and ptop!=NULL generates current(lmr) and next(lmp). mapdist!=0                               curr  1  top  blk  len   0  0  0 */
        /*                                                                                                                  c+dist  1 ptop  blk  len   0  0  1 */
        /* LD with force-read=0 and ptop!=NULL generates current(lmr) and next(lmp). mapdist==0                  ofs=ptop-top curr  1  top  blk  len   0  0  1 */
        /*                                                                                                               p=1の場合,pref-addrは常にlmmi.top+ofs */
        /* LDDMQ set f=1 and p=1 in lmmc automatically                                                                        curr  1  top  -    -     0  1  1 */
	if (!(dmex->op1 && dmex->mexd1h == dmop1->baseh))
	  set_reg_path(last_row, last_col, 0, type, RTYPE_BASE, 1, dmop1->basev, dmop1->baseh, dmop1->bases);
        set_reg_path(last_row, last_col, 0, type, RTYPE_OFFS, 1, dmop1->offsv, dmop1->offsh, dmop1->offss);
        if (dmop1->topv  == T_VARIABLE) {
	  int rw = (dmop1->mtype==MTYPE_RLOAD||dmop1->mtype==MTYPE_DLOAD)?0:1;
	  int f  = id[dmop1->forceh].val||dmop1->forcev==T_VARIABLE; /* force=変数の場合もf=1として扱う 20240410 Nakashima */
	  int p  = 0; /* initial value */
	  int last_col_mex = last_col;
          if (dmop1->mtype == MTYPE_RLOAD && (dmex->op1== OP_CMPA_LE || dmex->op1 == OP_CMPA_GE)) {
	    //printf("MOP1 CMPA RLOAD\n");
	  }
	  else if (last_col == 1 && dmop1->mtype == MTYPE_RLOAD && (dec[last_row][2].dmex.op0 == OP_CMPA_LE || dec[last_row][2].dmex.op0 == OP_CMPA_GE)) { /* load in load-cfma-store */
	    //printf("MOP0 CMPA RLOAD(load-cfms-store)\n");
	    last_col_mex = 0; //★★★ last_col=1の場合,mop0のlast_col_mexは0固定でよい
	  }
	  switch ((rw<<2)|(f<<1)|p) { /* AXI->LMM write対象(lmr/lmf/lmp/lmxの場合1:rw_f_p=000,010,001,110) *//* AXI<-LMM read対象(lmw/lmx/lmdの場合1:rw_f_p=100,110,101) */
	  case 0: /* lmr */ case 1: /* lmp */ case 2: /* lmf */ conf[last_row][last_col_mex].cdw2.lmm_axiw = 1; conf[last_row][last_col_mex].cdw2.lmm_axir = 0; break;
	  case 4: /* lmw */ case 5: /* lmd */                   conf[last_row][last_col_mex].cdw2.lmm_axiw = 0; conf[last_row][last_col_mex].cdw2.lmm_axir = 1; break;
	  case 6: /* lmx */                                     conf[last_row][last_col_mex].cdw2.lmm_axiw = 1; conf[last_row][last_col_mex].cdw2.lmm_axir = 1; break;
	  case 3: /* lddmq */ case 7: /* tr */                  conf[last_row][last_col_mex].cdw2.lmm_axiw = 0; conf[last_row][last_col_mex].cdw2.lmm_axir = 0; break;
	  }
	  conf[last_row][last_col_mex].cdw2.lmm_mode = 3;
	  lmmi[last_row][last_col_mex].v     = 1;
	  lmmi[last_row][last_col_mex].rw    = rw;
	  lmmi[last_row][last_col_mex].f     = f;
	  lmmi[last_row][last_col_mex].p     = p;
	  lmmi[last_row][last_col_mex].blk   = dmop1->blk;
	  lmmi[last_row][last_col_mex].cidx  = id[dmop1->toph].cidx;
	  lmmi[last_row][last_col_mex].len   = id[dmop1->lenh].val-1;
	  lmmi[last_row][last_col_mex].ofs   = 0; /* initial value */
	  lmmi[last_row][last_col_mex].top   = (Ull)id[dmop1->toph].name;
	  lmmx[last_row][last_col_mex].forcev= dmop1->forcev;
	  lmmx[last_row][last_col_mex].forceh= dmop1->forceh;
	  lmmx[last_row][last_col_mex].lenv  = dmop1->lenv;
	  lmmx[last_row][last_col_mex].lenh  = dmop1->lenh;
        }
        if (dmop1->ptopv == T_VARIABLE) { /* lmp */
          if (last_row+current_mapdist >= EMAX_DEPTH) { /* copy前に検査 */
            printf("in %s: [%d][%d] prefetch exceeds EMAX_DEPTH\n", id[current_prefix].name, last_row, last_col);
            exit(1);
          }
          if (dec[last_row+current_mapdist][last_col].dmop0.op) {
            printf("in %s: [%d][%d] prefetch may conflict with other mop\n", id[current_prefix].name, last_row, last_col);
            exit(1);
          }
          if (current_mapdist==0) { /* old_LDBF */
            if (id[dmop1->forceh].val) { /* reserved for LDDMQ */
              /* f=1を指定してもignored */
            }
	    lmmi[last_row][last_col].p   = 1; /* optional prefetch *//* lmm_axiw/lmm_axirは，lmrとlmpとで同じ扱いなので設定済 */
	    lmmi[last_row][last_col].ofs = (Uint)(id[dmop1->ptoph].name - id[dmop1->toph].name);
          }
          else {
	    int rw = (dmop1->mtype==MTYPE_RLOAD||dmop1->mtype==MTYPE_DLOAD)?0:1;
	    int f  = 0;
	    int p  = 1;
	    switch ((rw<<2)|(f<<1)|p) { /* AXI->LMM write対象(lmr/lmf/lmp/lmxの場合1:rw_f_p=000,010,001,110) *//* AXI<-LMM read対象(lmw/lmx/lmdの場合1:rw_f_p=100,110,101) */
	    case 0: /* lmr */ case 1: /* lmp */ case 2: /* lmf */ conf[last_row+current_mapdist][last_col].cdw2.lmm_axiw = 1; conf[last_row+current_mapdist][last_col].cdw2.lmm_axir = 0; break;
	    case 4: /* lmw */ case 5: /* lmd */                   conf[last_row+current_mapdist][last_col].cdw2.lmm_axiw = 0; conf[last_row+current_mapdist][last_col].cdw2.lmm_axir = 1; break;
	    case 6: /* lmx */                                     conf[last_row+current_mapdist][last_col].cdw2.lmm_axiw = 1; conf[last_row+current_mapdist][last_col].cdw2.lmm_axir = 1; break;
	    case 3: /* lddmq */ case 7: /* tr */                  conf[last_row+current_mapdist][last_col].cdw2.lmm_axiw = 0; conf[last_row+current_mapdist][last_col].cdw2.lmm_axir = 0; break;
	    }
	    conf[last_row+current_mapdist][last_col].cdw2.lmm_mode = 3;
	    lmmi[last_row+current_mapdist][last_col].v     = 1;
	    lmmi[last_row+current_mapdist][last_col].rw    = rw;
	    lmmi[last_row+current_mapdist][last_col].f     = f;
	    lmmi[last_row+current_mapdist][last_col].p     = p;
	    lmmi[last_row+current_mapdist][last_col].blk   = dmop1->blk;
	    lmmi[last_row+current_mapdist][last_col].cidx  = id[dmop1->ptoph].cidx;
	    lmmi[last_row+current_mapdist][last_col].len   = id[dmop1->plenh].val-1;
	    lmmi[last_row+current_mapdist][last_col].ofs   = 0;
	    lmmi[last_row+current_mapdist][last_col].top   = (Ull)id[dmop1->ptoph].name;
	    lmmx[last_row+current_mapdist][last_col].forcev= dmop1->forcev;
	    lmmx[last_row+current_mapdist][last_col].forceh= dmop1->forceh;
	    lmmx[last_row+current_mapdist][last_col].lenv  = dmop1->plenv;
	    lmmx[last_row+current_mapdist][last_col].lenh  = dmop1->plenh;
          }
        }
      }
      else { /* store/load */
        /* ST with force-read=0 and ptop==NULL generates current(lmw) and reuse+wback LMM. same as lmw in EMAX4               curr  1  top  blk  len   1  0  0 */
        /* ST with force-read=1 and ptop==NULL generates current(lmx) and !reuse+wback LMM. same as lmx in EMAX4              curr  1  top  blk  len   1  1  0 */
        /* ST with force-read=0 and ptop!=NULL generates current(lmw) and prev(lmd). mapdist!=0                               curr  1  top  blk  len   1  0  0 */
        /*                                                                                                                  c-dist  1 ptop  blk  len   1  0  1 */
        /* ST with force-read=0 and ptop!=NULL generates current(lmw) and prev(lmd). mapdist==0                  ofs=ptop-top curr  1  top  blk  len   1  0  1 */
        /*                                                                                                              p=1の場合,drain-addrは常にlmmi.top+ofs */
        /* TR    set f=1 and p=1 in lmmc automatically                                                                        curr  1  top  -    -     1  1  1 */
        if (insn[i].imop.mtype == MTYPE_RSTORE)
          set_reg_path(last_row, last_col, folding, type, RTYPE_DATA, 0, dmop0->mopdv, dmop0->mopdh, dmop0->mopds);
	if (!(dmex->op0 && dmex->mexd0h == dmop0->baseh))
	  set_reg_path(last_row, last_col, folding, type, RTYPE_BASE, 0, dmop0->basev, dmop0->baseh, dmop0->bases);
        set_reg_path(last_row, last_col, folding, type, RTYPE_OFFS, 0, dmop0->offsv, dmop0->offsh, dmop0->offss);
        if (dmop0->topv  == T_VARIABLE) {
	  int rw = (dmop0->mtype==MTYPE_RLOAD||dmop0->mtype==MTYPE_DLOAD)?0:1;
	  int f  = id[dmop0->forceh].val||dmop0->forcev==T_VARIABLE; /* force=変数の場合もf=1として扱う 20240410 Nakashima */
	  int p  = 0; /* initial value */
	  int last_col_mex = last_col;
          if (dmop0->mtype == MTYPE_RLOAD && !(dmex->op0 == OP_CMPA_LE || dmex->op0 == OP_CMPA_GE)) {
            /* if ldx2_and_op0_is_empty==true, dmop0 can share lmmi with dmop1 */
          }
          else {
	    if (dmex->op0 == OP_CMPA_LE || dmex->op0 == OP_CMPA_GE) { /* dmop0->mtype == MTYPE_RLOAD && (dmex->op0 == OP_CMPA_LE || dmex->op0 == OP_CMPA_GE) */
	      //printf("MOP0 CMPA RLOAD\n");
	      if (last_col < 2) {
		printf("in %s: [%d][%d] mex0 should be located col>=2\n", id[current_prefix].name, last_row, last_col);
		exit(1);
	      }
	      if (id[dmop0->toph].name != id[dmop1->toph].name) { /* lmm領域が異なる場合(Sparse matrix) */
		last_col_mex = 1; //★★★ last_col=3,2の場合,mop0のlast_col_mexは1固定でよい
	        printf("dmex0.lmm moved from col%d to col%d ", last_col, last_col_mex);
	      }
	      else { /* lmm領域が同じ場合(Merge sort) */
	        printf("dmex0.lmm keep col%d ", last_col_mex);
	      }
	    }
	    else if (last_col == 1 && dmop0->mtype == MTYPE_RSTORE && (dec[last_row][2].dmex.op0 == OP_CMPA_LE || dec[last_row][2].dmex.op0 == OP_CMPA_GE)) { /* store in load-cfma-store */
	      //printf("MOP0 CMPA RSTORE(load-cfms-store)\n");
	      last_col_mex = 0; //★★★ last_col=1の場合,mop0のlast_col_mexは0固定でよい
	    }
	    switch ((rw<<2)|(f<<1)|p) { /* AXI->LMM write対象(lmr/lmf/lmp/lmxの場合1:rw_f_p=000,010,001,110) *//* AXI<-LMM read対象(lmw/lmx/lmdの場合1:rw_f_p=100,110,101) */
	    case 0: /* lmr */ case 1: /* lmp */ case 2: /* lmf */ conf[last_row][last_col_mex].cdw2.lmm_axiw = 1; conf[last_row][last_col_mex].cdw2.lmm_axir = 0; break;
	    case 4: /* lmw */ case 5: /* lmd */                   conf[last_row][last_col_mex].cdw2.lmm_axiw = 0; conf[last_row][last_col_mex].cdw2.lmm_axir = 1; break;
	    case 6: /* lmx */                                     conf[last_row][last_col_mex].cdw2.lmm_axiw = 1; conf[last_row][last_col_mex].cdw2.lmm_axir = 1; break;
	    case 3: /* lddmq */ case 7: /* tr */                  conf[last_row][last_col_mex].cdw2.lmm_axiw = 0; conf[last_row][last_col_mex].cdw2.lmm_axir = 0; break;
	    }
	    conf[last_row][last_col_mex].cdw2.lmm_mode = 3;
            lmmi[last_row][last_col_mex].v     = 1;
            lmmi[last_row][last_col_mex].rw    = rw;
            lmmi[last_row][last_col_mex].f     = f;
            lmmi[last_row][last_col_mex].p     = p;
            lmmi[last_row][last_col_mex].blk   = dmop0->blk;
	    lmmi[last_row][last_col_mex].cidx  = id[dmop0->toph].cidx;
            lmmi[last_row][last_col_mex].len   = id[dmop0->lenh].val-1;
            lmmi[last_row][last_col_mex].ofs   = 0; /* initial value */
            lmmi[last_row][last_col_mex].top   = (Ull)id[dmop0->toph].name;
            lmmx[last_row][last_col_mex].forcev= dmop0->forcev;
            lmmx[last_row][last_col_mex].forceh= dmop0->forceh;
            lmmx[last_row][last_col_mex].lenv  = dmop0->lenv;
            lmmx[last_row][last_col_mex].lenh  = dmop0->lenh;
          }
        }
        if (dmop0->ptopv == T_VARIABLE) { /* lmd */
          if (insn[i].imop.mtype == MTYPE_RLOAD) {
            /* if ldx2_and_op0_is_empty==true, dmop0 can share lmmi with dmop1 */
          }
          else { /* MTYPE_RSTORE */
            if (last_row-current_mapdist < 0) { /* copy前に検査 */
              printf("in %s: [%d][%d] drain exceeds EMAX_DEPTH\n", id[current_prefix].name, last_row, last_col);
              exit(1);
            }
            if (dec[last_row-current_mapdist][last_col].dmop1.op) {
              printf("in %s: [%d][%d] drain may conflict with other mop\n", id[current_prefix].name, last_row, last_col);
              exit(1);
            }
            if (current_mapdist==0) { /* old_STBF */
              if (id[dmop0->forceh].val) { /* reserved for TR */
                /* f=1を指定してもignored */
              }
              lmmi[last_row][last_col].p   = 1; /* optional postdrain *//* lmm_axiw/lmm_axirは，lmwとlmdとで同じ扱いなので設定済 */
              lmmi[last_row][last_col].ofs = (Uint)(id[dmop0->ptoph].name - id[dmop0->toph].name);
            }
            else {
              if (lmmi[last_row-current_mapdist][last_col].v) {
                printf("in %s: [%d][%d] drain may conflict with other lmm\n", id[current_prefix].name, last_row, last_col);
                exit(1);
              }
	      int rw = (dmop0->mtype==MTYPE_RLOAD||dmop0->mtype==MTYPE_DLOAD)?0:1;
	      int f  = 0;
	      int p  = 1;
	      switch ((rw<<2)|(f<<1)|p) { /* AXI->LMM write対象(lmr/lmf/lmp/lmxの場合1:rw_f_p=000,010,001,110) *//* AXI<-LMM read対象(lmw/lmx/lmdの場合1:rw_f_p=100,110,101) */
	      case 0: /* lmr */ case 1: /* lmp */ case 2: /* lmf */ conf[last_row-current_mapdist][last_col].cdw2.lmm_axiw = 1; conf[last_row-current_mapdist][last_col].cdw2.lmm_axir = 0; break;
	      case 4: /* lmw */ case 5: /* lmd */                   conf[last_row-current_mapdist][last_col].cdw2.lmm_axiw = 0; conf[last_row-current_mapdist][last_col].cdw2.lmm_axir = 1; break;
	      case 6: /* lmx */                                     conf[last_row-current_mapdist][last_col].cdw2.lmm_axiw = 1; conf[last_row-current_mapdist][last_col].cdw2.lmm_axir = 1; break;
	      case 3: /* lddmq */ case 7: /* tr */                  conf[last_row-current_mapdist][last_col].cdw2.lmm_axiw = 0; conf[last_row-current_mapdist][last_col].cdw2.lmm_axir = 0; break;
	      }
	      conf[last_row-current_mapdist][last_col].cdw2.lmm_mode = 3;
              lmmi[last_row-current_mapdist][last_col].v     = 1;
              lmmi[last_row-current_mapdist][last_col].rw    = rw;
              lmmi[last_row-current_mapdist][last_col].f     = f;
              lmmi[last_row-current_mapdist][last_col].p     = p;
              lmmi[last_row-current_mapdist][last_col].blk   = dmop0->blk;
	      lmmi[last_row-current_mapdist][last_col].cidx  = id[dmop0->ptoph].cidx;
              lmmi[last_row-current_mapdist][last_col].len   = id[dmop0->plenh].val-1;
              lmmi[last_row-current_mapdist][last_col].ofs   = 0;
              lmmi[last_row-current_mapdist][last_col].top   = (Ull)id[dmop0->ptoph].name;
              lmmx[last_row-current_mapdist][last_col].forcev= dmop0->forcev;
              lmmx[last_row-current_mapdist][last_col].forceh= dmop0->forceh;
              lmmx[last_row-current_mapdist][last_col].lenv  = dmop0->plenv;
              lmmx[last_row-current_mapdist][last_col].lenh  = dmop0->plenh;
            }
          }
        }
      }
      /* 1-6 *//* RLOAD,DLOAD:格納先busmapはBR止まり RSTORE,TRANS: 格納先busmapはTR止まり */
      if (insn[i].imop.mtype == MTYPE_RLOAD) {
        if (last_mop==1) { /* load */
          if (dmop1->topv  == T_VARIABLE) {
            for (j=0; j<UNIT_WIDTH; j++) {
              bus[last_row][last_col].lmwd[j].v = dmop1->topv;
              bus[last_row][last_col].lmwd[j].h = dmop1->toph;
              bus[last_row][last_col].lmwd[j].s = j;
              /* mwは実行時設定を優先(STATUS_LOADが実行時設定を無視) */
            }
          }
          if (dmop1->ptopv == T_VARIABLE) { /* lmp */
            dec[last_row+current_mapdist][last_col].dmop0.op = OP_IM_PREF; /* mapdist=0含む */
            for (j=0; j<UNIT_WIDTH; j++) {
              bus[last_row+current_mapdist][last_col].lmwd[j].v = dmop1->ptopv;
              bus[last_row+current_mapdist][last_col].lmwd[j].h = dmop1->ptoph;
              bus[last_row+current_mapdist][last_col].lmwd[j].s = j;
              bus[last_row+current_mapdist][last_col].mw[j].v   = dmop1->ptopv;
              bus[last_row+current_mapdist][last_col].mw[j].h   = dmop1->ptoph;
              bus[last_row+current_mapdist][last_col].mw[j].s   = j;
            }
          }
          if (dmop1->mopds == 0) {
            printf("in %s: [%d][%d] mop1 cannot connect to %s[%d]\n", id[current_prefix].name, last_row, last_col, id[dmop1->mopdh].name,dmop1->mopds);
            exit(1);
          }
	  /* if the first is dismissible(ex=const_0) for unaligned load, keep BR empty */
	  if (!(dmop1->exv == T_IMMEDIATE && id[dmop1->exh].val == 0)) {
	    bus[last_row][last_col].br[1].v = dmop1->mopdv;
	    bus[last_row][last_col].br[1].h = dmop1->mopdh;
	    bus[last_row][last_col].br[1].s = dmop1->mopds;
	    conf[last_row][last_col].cdw2.brs1 = 3; /* 3:mr1 */
	  }
        }
        else { /* store/load */
          if (dmop0->mopds == 1) {
            printf("in %s: [%d][%d] mop0 cannot connect to %s[%d]\n", id[current_prefix].name, last_row, last_col, id[dmop1->mopdh].name,dmop1->mopds);
            exit(1);
          }
          bus[last_row][last_col].br[0].v = dmop0->mopdv;
          bus[last_row][last_col].br[0].h = dmop0->mopdh;
          bus[last_row][last_col].br[0].s = dmop0->mopds;
          conf[last_row][last_col].cdw2.brs0 = 3; /* 3:mr0 */
        }
        id[insn[i].imop.mopdh].itype = ITYPE_MOP;
        id[insn[i].imop.mopdh].row   = last_row;
        id[insn[i].imop.mopdh].col   = last_col;
      }
      else { /* MTYPE_RSTORE */
        if (dmop0->ptopv == T_VARIABLE) { /* lmd */
          dec[last_row-current_mapdist][last_col].dmop1.op = OP_IM_DRAIN; /* mapdist=0含む */
          for (j=0; j<UNIT_WIDTH; j++) {
            bus[last_row-current_mapdist][last_col].lmrd[j].v = dmop0->ptopv;
            bus[last_row-current_mapdist][last_col].lmrd[j].h = dmop0->ptoph;
            bus[last_row-current_mapdist][last_col].lmrd[j].s = j;
          }
        }
        for (j=0; j<UNIT_WIDTH; j++) {
          bus[last_row][last_col].mw[j].v = dmop0->mopdv;
          bus[last_row][last_col].mw[j].h = dmop0->mopdh;
          bus[last_row][last_col].mw[j].s = dmop0->mopds;
        }
      }
      insn[i].iheader.fixed = 1;
      break;
    default: /* illegal */
      break;
    }
#if 1
    printf("dec[%d][%d]:type=%d\n", last_row, last_col, type);
#endif
    if (last_col == 0 && dec[last_row][0].dexu.op1 == OP_FOR && dec[last_row][1].dexu.op1 == OP_FOR)
      last_col++; /* max number of LOOP# */
  }
  /**********************************************************************************************************/
  /* Step 2 ... setup conf[][]                                                                              */
  /* 2-1. select EXE-in                                                                                     */
  /* 2-2. select CEX-in and EAG-in                                                                          */
  /**********************************************************************************************************/
  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      int ea0rs, ea1rs;
      /* 2-1 */
      /* ex[123] depends on busmap[i][j].br[prev][] and decode[i][j].dexu.ex[123] */
      conf[i][j].cdw0.v      = (i <= last_row) ? 1 : 0;
      conf[i][j].cdw0.op1    = dec[i][j].dexu.op1;
      conf[i][j].cdw0.op2    = dec[i][j].dexu.op2;
      conf[i][j].cdw0.op3    = dec[i][j].dexu.op3;
      conf[i][j].cdw0.ex1brs = search_prev_br0(i, dec[i][j].dexu.fold, dec[i][j].dexu.ex1v, dec[i][j].dexu.ex1h, dec[i][j].dexu.ex1s);
      conf[i][j].cdw0.ex1s   = dec[i][j].dexu.updt; /* 0->0 or 0->1 */
      conf[i][j].cdw0.ex1exp = dec[i][j].dexu.ex1e;
      conf[i][j].cdw0.ex2brs = search_prev_br0(i, dec[i][j].dexu.fold, dec[i][j].dexu.ex2v, dec[i][j].dexu.ex2h, dec[i][j].dexu.ex2s);
      conf[i][j].cdw0.ex2exp = dec[i][j].dexu.ex2e;
      conf[i][j].cdw0.ex3brs = search_prev_br0(i, dec[i][j].dexu.fold, dec[i][j].dexu.ex3v, dec[i][j].dexu.ex3h, dec[i][j].dexu.ex3s);
      conf[i][j].cdw0.ex3exp = dec[i][j].dexu.ex3e;
      if (dec[i][j].dexu.e2iv==T_IMMEDIATE)
        conf[i][j].cdw3.e2imm  = id[dec[i][j].dexu.e2ih].val;
      else
        conf[i][j].cdw3.e2imm  = 0;
      conf[i][j].cdw0.e2is   = dec[i][j].dexu.e2is;
      if (dec[i][j].dexu.e3iv==T_IMMEDIATE) {
        if ((Ull)id[dec[i][j].dexu.e3ih].val > (1<<E3IMMBITS)-1) {
          printf("in %s: [%d][%d] cannot fit to e3imm(unsigned %dbit) immediate=0x%08.8x%08.8x\n", id[current_prefix].name, i, j, E3IMMBITS, (Uint)(id[dec[i][j].dexu.e3ih].val>>32), (Uint)id[dec[i][j].dexu.e3ih].val);
          exit(1);
        }
        conf[i][j].cdw0.e3imm  = id[dec[i][j].dexu.e3ih].val;
      }
      else
        conf[i][j].cdw0.e3imm  = 0;
      conf[i][j].cdw0.e3is   = dec[i][j].dexu.e3is;
      conf[i][j].cdw0.init   = dec[i][j].dexu.init; /* case of updt=0: bit0:activate s2+INIT0 bit1:activate s3+INIT1 */
      conf[i][j].cdw0.fold   = dec[i][j].dexu.fold; /* for load-exe-store folding */
      //printf("conf[%d][%d]: init=%d fold=%d\n", i, j, conf[i][j].cdw0.init, conf[i][j].cdw0.fold);

      /* 2-2 */
      /* cs[0-3] depends on busmap[i][j].br[prev][] and decode[i][j].dcex.bit[0-3] */
      conf[i][j].cdw1.cs0    = search_prev_br0(i, dec[i][j].dexu.fold, dec[i][j].dcex.bit0v, dec[i][j].dcex.bit0h, -1);
      conf[i][j].cdw1.cs1    = search_prev_br0(i, dec[i][j].dexu.fold, dec[i][j].dcex.bit1v, dec[i][j].dcex.bit1h, -1);
      conf[i][j].cdw1.cs2    = search_prev_br0(i, dec[i][j].dexu.fold, dec[i][j].dcex.bit2v, dec[i][j].dcex.bit2h, -1);
      conf[i][j].cdw1.cs3    = search_prev_br0(i, dec[i][j].dexu.fold, dec[i][j].dcex.bit3v, dec[i][j].dcex.bit3h, -1);
      conf[i][j].cdw1.cex_tab= dec[i][j].dcex.op ? dec[i][j].dcex.table : 0xffff; /* always true */

      /* mex0/mop0 */
      conf[i][j].cdw0.mex0op   = dec[i][j].dmex.op0;
      conf[i][j].cdw0.mex0init = dec[i][j].dmex.init;
      switch (id[dec[i][j].dmex.dist1h].val) { /* distance 0:0, 1:1, 2:2, 3:4, 4:8, 5:16, 6:32, 7:64byte */
      case  0: conf[i][j].cdw0.mex0dist = 0; break;
      case  1: conf[i][j].cdw0.mex0dist = 1; break;
      case  2: conf[i][j].cdw0.mex0dist = 2; break;
      case  4: conf[i][j].cdw0.mex0dist = 3; break;
      case  8: conf[i][j].cdw0.mex0dist = 4; break;
      case 16: conf[i][j].cdw0.mex0dist = 5; break;
      case 32: conf[i][j].cdw0.mex0dist = 6; break;
      default: conf[i][j].cdw0.mex0dist = 7; break;
      }
      if (dec[i][j].dmex.op0) {
	switch (id[dec[i][j].dmex.limith].val) { /* limit 0:0, 1:8, 2:16, .... 10:4096, 11:8192, 12:16384, 13:32768 */
	case     0: conf[i][j].cdw0.mexlimit = 0; break;
	case     8: conf[i][j].cdw0.mexlimit = 1; break;
	case    16: conf[i][j].cdw0.mexlimit = 2; break;
	case    32: conf[i][j].cdw0.mexlimit = 3; break;
	case    64: conf[i][j].cdw0.mexlimit = 4; break;
	case   128: conf[i][j].cdw0.mexlimit = 5; break;
	case   256: conf[i][j].cdw0.mexlimit = 6; break;
	case   512: conf[i][j].cdw0.mexlimit = 7; break;
	case  1024: conf[i][j].cdw0.mexlimit = 8; break;
	case  2048: conf[i][j].cdw0.mexlimit = 9; break;
	case  4096: conf[i][j].cdw0.mexlimit =10; break;
	case  8192: conf[i][j].cdw0.mexlimit =11; break;
	case 16384: conf[i][j].cdw0.mexlimit =12; break;
	case 32768: conf[i][j].cdw0.mexlimit =13; break;
	case 65536: conf[i][j].cdw0.mexlimit =14; break;
	default:    conf[i][j].cdw0.mexlimit =15; break;
	}
      }
      /* mex0.mexdh and mop0.mopdh */
      if (dec[i][j].dmex.op0 && dec[i][j].dmex.mexd0h == dec[i][j].dmop0.baseh) {
	printf("[%d][%d] detected dmex0.dst==mop0.base.", i, j);
	                    dec[i][j].dmop0.updt  = 1;
	regv[i][j].ea0b_v = dec[i][j].dmop0.basev = dec[i][j].dmex.adr1v; /* replace mop0.base to base in mex(&base0, INIT0?base:base0) */
	regv[i][j].ea0b_h = dec[i][j].dmop0.baseh = dec[i][j].dmex.adr1h; /* replace mop0.base to base in mex(&base0, INIT0?base:base0) */
	regv[i][j].ea0b_s = dec[i][j].dmop0.bases = dec[i][j].dmex.adr1s; /* replace mop0.base to base in mex(&base0, INIT0?base:base0) */
	printf(" mop0 renamed from %s to %s\n", id[dec[i][j].dmex.mexd0h].name, id[dec[i][j].dmop0.baseh].name);
      }

      /* ea[bo] depends on busmap[i][j].br[prev][] and decode[i][j].dmop.ex[123] */
      /* mapdist=0の場合,dmop0.op=OP_IM_PREFの可能性あり,比較対象外 */
      conf[i][j].cdw1.ea0op  = dec[i][j].dmop0.op;
      conf[i][j].cdw1.ea0bs  = ((!dec[i][j].dmop0.op||dec[i][j].dmop0.op==OP_IM_PREF )||bus[i][j].ea0brv?0:2)|(dec[i][j].dmop0.updt?1:0); /* 0:ea0br, 1:ea0dr(ea0br+self-loop), 2:eabbrs, 3:ea0dr(eabbrs+self-loop) */
      conf[i][j].cdw1.ea0os  = ( !dec[i][j].dmop0.op||dec[i][j].dmop0.op==OP_IM_PREF )||bus[i][j].ea0orv?0:1; /* 0:ea0or, 1:eaobrs */
      conf[i][j].cdw1.ea0msk = dec[i][j].dmop0.offsm;

      /* mex1/mop1 */
      conf[i][j].cdw0.mex1op   = dec[i][j].dmex.op1;
      conf[i][j].cdw0.mex1init = dec[i][j].dmex.init;
      switch (id[dec[i][j].dmex.dist2h].val) { /* distance 0:0, 1:1, 2:2, 3:4, 4:8, 5:16, 6:32, 7:64byte */
      case  0: conf[i][j].cdw0.mex1dist = 0; break;
      case  1: conf[i][j].cdw0.mex1dist = 1; break;
      case  2: conf[i][j].cdw0.mex1dist = 2; break;
      case  4: conf[i][j].cdw0.mex1dist = 3; break;
      case  8: conf[i][j].cdw0.mex1dist = 4; break;
      case 16: conf[i][j].cdw0.mex1dist = 5; break;
      case 32: conf[i][j].cdw0.mex1dist = 6; break;
      default: conf[i][j].cdw0.mex1dist = 7; break;
      }
      if (dec[i][j].dmex.op1) {
	switch (id[dec[i][j].dmex.limith].val) { /* limit 0:0, 1:8, 2:16, .... 10:4096, 11:8192, 12:16384, 13:32768 */
	case     0: conf[i][j].cdw0.mexlimit = 0; break;
	case     8: conf[i][j].cdw0.mexlimit = 1; break;
	case    16: conf[i][j].cdw0.mexlimit = 2; break;
	case    32: conf[i][j].cdw0.mexlimit = 3; break;
	case    64: conf[i][j].cdw0.mexlimit = 4; break;
	case   128: conf[i][j].cdw0.mexlimit = 5; break;
	case   256: conf[i][j].cdw0.mexlimit = 6; break;
	case   512: conf[i][j].cdw0.mexlimit = 7; break;
	case  1024: conf[i][j].cdw0.mexlimit = 8; break;
	case  2048: conf[i][j].cdw0.mexlimit = 9; break;
	case  4096: conf[i][j].cdw0.mexlimit =10; break;
	case  8192: conf[i][j].cdw0.mexlimit =11; break;
	case 16384: conf[i][j].cdw0.mexlimit =12; break;
	case 32768: conf[i][j].cdw0.mexlimit =13; break;
	case 65536: conf[i][j].cdw0.mexlimit =14; break;
	default:    conf[i][j].cdw0.mexlimit =15; break;
	}
      }
      /* mex1.mexdh and mop1.mopdh */
      if (dec[i][j].dmex.op1 && dec[i][j].dmex.mexd1h == dec[i][j].dmop1.baseh) {
	printf("[%d][%d] detected dmex1.dst==mop1.base.", i, j);
	                    dec[i][j].dmop1.updt  = 1;
	regv[i][j].ea1b_v = dec[i][j].dmop1.basev = dec[i][j].dmex.adr3v; /* replace mop1.base to base in mex(&base0, INIT0?base:base0) */
	regv[i][j].ea1b_h = dec[i][j].dmop1.baseh = dec[i][j].dmex.adr3h; /* replace mop1.base to base in mex(&base0, INIT0?base:base0) */
	regv[i][j].ea1b_s = dec[i][j].dmop1.bases = dec[i][j].dmex.adr3s; /* replace mop1.base to base in mex(&base0, INIT0?base:base0) */
	printf(" mop1 renamed from %s to %s\n", id[dec[i][j].dmex.mexd1h].name, id[dec[i][j].dmop1.baseh].name);
      }

      /* ea[bo] depends on busmap[i][j].br[prev][] and decode[i][j].dmop.ex[123] */
      /* mapdist=0の場合,dmop1.op=OP_IM_DRAINの可能性あり,比較対象外 */
      if (dec[i][j].dmop0.op == OP_LDDMQ || dec[i][j].dmop0.op == OP_TR)
        conf[i][j].cdw1.ea1op = dec[i][j].dmop0.op;
      else
        conf[i][j].cdw1.ea1op = dec[i][j].dmop1.op;
      conf[i][j].cdw1.ea1bs  = ((!dec[i][j].dmop1.op||dec[i][j].dmop1.op==OP_IM_DRAIN)||bus[i][j].ea1brv?0:2)|(dec[i][j].dmop1.updt?1:0); /* 0:ea1br, 1:ea1dr(ea1br+self-loop), 2:eabbrs, 3:ea1dr(eabbrs+self-loop) */
      conf[i][j].cdw1.ea1os  = ( !dec[i][j].dmop1.op||dec[i][j].dmop1.op==OP_IM_DRAIN)||bus[i][j].ea1orv?0:1; /* 0:ea1or, 1:eaobrs */
      conf[i][j].cdw1.ea1msk = dec[i][j].dmop1.offsm;

      //printf("conf[%d][%d]: mex0=%d.%d.%d mex1=%d.%d.%d\n", i, j, conf[i][j].cdw0.mex0op, conf[i][j].cdw0.mex0init, conf[i][j].cdw0.mex0dist, conf[i][j].cdw0.mex1op, conf[i][j].cdw0.mex1init, conf[i][j].cdw0.mex1dist);

      if (conf[i][j].cdw1.ea0bs&2) /* find source of eabbrs */
        ea0rs = search_prev_br0(i, dec[i][j].dexu.fold, dec[i][j].dmop0.basev, dec[i][j].dmop0.baseh, dec[i][j].dmop0.bases);
      else
        ea0rs = -1;
      if (conf[i][j].cdw1.ea1bs&2) /* find source of eabbrs */
        ea1rs = search_prev_br0(i, 0, dec[i][j].dmop1.basev, dec[i][j].dmop1.baseh, dec[i][j].dmop1.bases);
      else
        ea1rs = -1;
      if ((conf[i][j].cdw1.ea0bs&2) && (conf[i][j].cdw1.ea1bs&2)) { /* eabbrs is shared */
        /* check conflict */
        if (ea0rs != ea1rs) { /* conflict */
          printf("in %s: [%d][%d] dmop1.base(%s.%d) and dmop0.base(%s.%d) conflict\n", id[current_prefix].name, i, j, id[dec[i][j].dmop1.baseh].name, ea1rs, id[dec[i][j].dmop0.baseh].name, ea0rs);
          exit(1);
        }
        conf[i][j].cdw1.eabbrs = ea0rs; /* dmop0.base takes eabbrs */
      }
      else if (conf[i][j].cdw1.ea0bs&2)
        conf[i][j].cdw1.eabbrs = ea0rs; /* dmop0.base takes eabbrs */
      else if (conf[i][j].cdw1.ea1bs&2)
        conf[i][j].cdw1.eabbrs = ea1rs; /* dmop1.base takes eabbrs */
      else
        conf[i][j].cdw1.eabbrs = 0; /* not used */
      if (conf[i][j].cdw1.ea0os)
        ea0rs = search_prev_br0(i, dec[i][j].dexu.fold, dec[i][j].dmop0.offsv, dec[i][j].dmop0.offsh, dec[i][j].dmop0.offss);
      if (conf[i][j].cdw1.ea1os)
        ea1rs = search_prev_br0(i, 0, dec[i][j].dmop1.offsv, dec[i][j].dmop1.offsh, dec[i][j].dmop1.offss);
      if (conf[i][j].cdw1.ea0os && conf[i][j].cdw1.ea1os) { /* eaobrs is selected */
        /* check conflict */
        if (ea0rs != ea1rs) { /* conflict */
          printf("in %s: [%d][%d] dmop1.offs(%s.%d) and dmop0.offs(%s.%d) conflict\n", id[current_prefix].name, i, j, id[dec[i][j].dmop1.offsh].name, ea1rs, id[dec[i][j].dmop0.offsh].name, ea0rs);
          exit(1);
        }
        conf[i][j].cdw1.eaobrs = ea0rs; /* dmop0.offs takes eaobrs */
      }
      else if (conf[i][j].cdw1.ea0os)
        conf[i][j].cdw1.eaobrs = ea0rs; /* dmop0.offs takes eaobrs */
      else if (conf[i][j].cdw1.ea1os)
        conf[i][j].cdw1.eaobrs = ea1rs; /* dmop1.offs takes eaobrs */
      else
        conf[i][j].cdw1.eaobrs = 0; /* not used */
    }
  }
  /**********************************************************************************************************/
  /* Step 3 ... setup conf[][]                                                                              */
  /* 3-1. select MW-in                                                                                      */
  /* 3-2. select BR-in                                                                                      */
  /* 3-3. set mapdist                                                                                       */
  /**********************************************************************************************************/
  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      /* 3-1 */
      /* tr[0],mw[0] */
      /* AR->TRのパスがあればEX4結果伝搬用 */
      /* BR->TRのパスがあれば伝搬用 */
      if (bus[i][j].tr[0].v) { /* tr[0] active */
        if ((k = search_prev_ar0_tr(i, j, 0, bus[i][j].tr[0].v, bus[i][j].tr[0].h, bus[i][j].tr[0].s)) >= 0) {
          conf[i][j].cdw2.trs0 = k;
        }
        else {
          k = search_prev_br0(i, 0, bus[i][j].tr[0].v, bus[i][j].tr[0].h, bus[i][j].tr[0].s);
          conf[i][j].cdw2.ts0  = k;
          conf[i][j].cdw2.trs0 = 2; /* ts0 */
        }
      }
      /* AR->MWのパスがあればset */
      /* BR->MWのパスがあればset */
      /* LMWD->MWのパスがあればset */
      if (bus[i][j].mw[0].v) { /* mw[0] active */
        if ((k = search_prev_ar0_mw(i, j, 0, bus[i][j].mw[0].v, bus[i][j].mw[0].h, bus[i][j].mw[0].s)) >= 0) {
          conf[i][j].cdw2.mws0 = k;
        }
        else {
          k = search_prev_br0(i, dec[i][j].dexu.fold, bus[i][j].mw[0].v, bus[i][j].mw[0].h, bus[i][j].mw[0].s);
          conf[i][j].cdw2.ts0  = k;
          conf[i][j].cdw2.mws0 = 2; /* ts0 */
        }
      }
      /* mws0... 0:lmwd0, 1:exdr, 2:ts0 */
      /* mwsa... 0:lmwa,  1:ea0d *//* for STATUS_EXEC+lmp */
      switch (conf[i][j].cdw2.mws0) {
      case 0: conf[i][j].cdw2.mwsa = 0; break;
      default:conf[i][j].cdw2.mwsa = 1; break;
      }

      /* tr[1],mw[1] */
      if (bus[i][j].tr[1].v) { /* tr[1] active */
        if ((k = search_prev_ar0_tr(i, j, 1, bus[i][j].tr[1].v, bus[i][j].tr[1].h, bus[i][j].tr[1].s)) >= 0) {
          conf[i][j].cdw2.trs1 = k;
        }
        else {
          k = search_prev_br0(i, 0, bus[i][j].tr[1].v, bus[i][j].tr[1].h, bus[i][j].tr[1].s);
          conf[i][j].cdw2.ts1  = k;
          conf[i][j].cdw2.trs1 = 2; /* ts1 */
        }
      }
      if (bus[i][j].mw[1].v) { /* mw[1] active */
        if ((k = search_prev_ar0_mw(i, j, 1, bus[i][j].mw[1].v, bus[i][j].mw[1].h, bus[i][j].mw[1].s)) >= 0) {
          conf[i][j].cdw2.mws1 = k;
        }
        else {
          k = search_prev_br0(i, dec[i][j].dexu.fold, bus[i][j].mw[1].v, bus[i][j].mw[1].h, bus[i][j].mw[1].s);
          conf[i][j].cdw2.ts1  = k;
          conf[i][j].cdw2.mws1 = 2; /* ts1 */
        }
      }

      /* tr[2],mw[2] */
      if (bus[i][j].tr[2].v) { /* tr[2] active */
        if ((k = search_prev_ar0_tr(i, j, 2, bus[i][j].tr[2].v, bus[i][j].tr[2].h, bus[i][j].tr[2].s)) >= 0) {
          conf[i][j].cdw2.trs2 = k;
        }
        else {
          k = search_prev_br0(i, 0, bus[i][j].tr[2].v, bus[i][j].tr[2].h, bus[i][j].tr[2].s);
          conf[i][j].cdw2.ts2  = k;
          conf[i][j].cdw2.trs2 = 2; /* ts2 */
        }
      }
      if (bus[i][j].mw[2].v) { /* mw[2] active */
        if ((k = search_prev_ar0_mw(i, j, 2, bus[i][j].mw[2].v, bus[i][j].mw[2].h, bus[i][j].mw[2].s)) >= 0) {
          conf[i][j].cdw2.mws2 = k;
        }
        else {
          k = search_prev_br0(i, dec[i][j].dexu.fold, bus[i][j].mw[2].v, bus[i][j].mw[2].h, bus[i][j].mw[2].s);
          conf[i][j].cdw2.ts2  = k;
          conf[i][j].cdw2.mws2 = 2; /* ts2 */
        }
      }

      /* tr[3],mw[3] */
      if (bus[i][j].tr[3].v) { /* tr[3] active */
        if ((k = search_prev_ar0_tr(i, j, 3, bus[i][j].tr[3].v, bus[i][j].tr[3].h, bus[i][j].tr[3].s)) >= 0) {
          conf[i][j].cdw2.trs3 = k;
        }
        else {
          k = search_prev_br0(i, 0, bus[i][j].tr[3].v, bus[i][j].tr[3].h, bus[i][j].tr[3].s);
          conf[i][j].cdw2.ts3  = k;
          conf[i][j].cdw2.trs3 = 2; /* ts3 */
        }
      }
      if (bus[i][j].mw[3].v) { /* mw[3] active */
        if ((k = search_prev_ar0_mw(i, j, 3, bus[i][j].mw[3].v, bus[i][j].mw[3].h, bus[i][j].mw[3].s)) >= 0) {
          conf[i][j].cdw2.mws3 = k;
        }
        else {
          k = search_prev_br0(i, dec[i][j].dexu.fold, bus[i][j].mw[3].v, bus[i][j].mw[3].h, bus[i][j].mw[3].s);
          conf[i][j].cdw2.ts3  = k;
          conf[i][j].cdw2.mws3 = 2; /* ts3 */
        }
      }

      /* 3-2 */
      /* conf[i][j].cdw2.brs0; *//* set with bus.tr[]/br[] */
      /* conf[i][j].cdw2.brs1; *//* set with bus.tr[]/br[] */
      /* conf[i][j].cdw2.brs2; *//* set with bus.tr[]/br[] */
      /* conf[i][j].cdw2.brs3; *//* set with bus.tr[]/br[] */
      /* exdr is connected by default (brs=0) */

      /* 3-3 */
      conf[i][j].cdw2.mapdist = current_mapdist;
    }
  }
  /**********************************************************************************************************/
  /* Step 4 ... Insert LMM-buffering for neighbor LDDMQ                                                     */
  /*            Multiple LDDMQ in the same row is not allowed                                               */
  /**********************************************************************************************************/
  for (i=0; i<EMAX_DEPTH; i++) {
    /* checking LDDMQ */
    int lddmq_loc = -1;
    for (j=0; j<EMAX_WIDTH; j++) {
      if (conf[i][j].cdw1.ea0op == OP_LDDMQ) {
        lddmq_loc = j;
        break;
      }
    }
    if (lddmq_loc < 0)
      continue;
    for (j=0; j<EMAX_WIDTH; j++) {
      if (j == lddmq_loc)
        continue;
      /* LMM-buffering replacement */
      if (conf[i][j].cdw2.brs0 || conf[i][j].cdw2.brs1 || conf[i][j].cdw2.brs2 || conf[i][j].cdw2.brs3) {
        if (conf[i][j].cdw1.ea0op || conf[i][j].cdw1.ea1op) { /* error if EAGs are occupied */
          printf("in %s: [%d][%d] cannot remap BR-buffering for neighbor lddmq (ea0op=%d ea1op=%d\n",
                 id[current_prefix].name, i, j, conf[i][j].cdw1.ea0op, conf[i][j].cdw1.ea1op);
          exit(1);
        }
        /* setup LMM as FIFO */
	conf[i][j].cdw2.lmm_mode = 3;
        conf[i][j].cdw1.ea0op  = OP_IM_BUFWR;
        conf[i][j].cdw1.ea0bs  = 1; /* ea0br+self-loop */
        conf[i][j].cdw1.ea0os  = 0; /* ea0or           */
        conf[i][j].cdw1.ea0msk = 15;/* 64bit           */
        conf[i][j].cdw1.ea1op  = OP_IM_BUFRD;
        conf[i][j].cdw1.ea1bs  = 1; /* ea1br+self-loop */
        conf[i][j].cdw1.ea1os  = 0; /* ea1or           */
        conf[i][j].cdw1.ea1msk = 15;/* 64bit           */
        conf[i][j].cdw1.eabbrs = 0; /* not used */
        conf[i][j].cdw1.eaobrs = 0; /* not used */
        regv[i][j].ea0b_v = T_IMMEDIATE;
        regv[i][j].ea0b_h = hash_reg_immediate(0LL);
        regv[i][j].ea0b_s = -1;
        regv[i][j].ea0o_v = T_IMMEDIATE;
        regv[i][j].ea0o_h = hash_reg_immediate(32LL);
        regv[i][j].ea0o_s = -1;
        regv[i][j].ea1b_v = T_IMMEDIATE;
        regv[i][j].ea1b_h = hash_reg_immediate(0LL);
        regv[i][j].ea1b_s = -1;
        regv[i][j].ea1o_v = T_IMMEDIATE;
        regv[i][j].ea1o_h = hash_reg_immediate(32LL);
        regv[i][j].ea1o_s = -1;

        /* ts0-3は維持 */
        /* brs0-3がTRかEXDRであることを確認し,mws0-3に反映 */
        /* brs0-3をmr10-13に切替え */
        printf("Lmm-buffering is inserted in conf[%d][%d]", i, j);
        switch (conf[i][j].cdw2.brs3) {
        case 2: conf[i][j].cdw2.mws3 = 2; conf[i][j].cdw2.brs3 = 1; printf(".t3"); break; /* tr3: ts3->mw3->mr13 */
        default:                          conf[i][j].cdw2.brs3 = 0; break; /* off */
        }
        switch (conf[i][j].cdw2.brs2) {
        case 2: conf[i][j].cdw2.mws2 = 2; conf[i][j].cdw2.brs2 = 1; printf(".t2"); break; /* tr2: ts2->mw2->mr12 */
        case 3: conf[i][j].cdw2.mws2 = 0; conf[i][j].cdw2.brs2 = 1; printf(".ex"); break; /* exd: exd->mw2->mr12 */
        default:                          conf[i][j].cdw2.brs2 = 0; break; /* off */
        }
        switch (conf[i][j].cdw2.brs1) {
        case 2: conf[i][j].cdw2.mws1 = 2; conf[i][j].cdw2.brs1 = 1; printf(".t1"); break; /* tr1: ts1->mw1->mr11 */
        default:                          conf[i][j].cdw2.brs1 = 0; break; /* off */
        }
        switch (conf[i][j].cdw2.brs0) {
        case 2: conf[i][j].cdw2.mws0 = 2; conf[i][j].cdw2.brs0 = 1; printf(".t0"); break; /* tr0: ts0->mw0->mr10 */
        default:                          conf[i][j].cdw2.brs0 = 0; break; /* off */
        }
        printf(" for [%d][%d].lddmq\n", i, lddmq_loc);
      }
    }
    /**/
  }
  /**********************************************************************************************************/
  /* Step 5 ... Merge LMM                                                                                   */
  /**********************************************************************************************************/
  /* merge lmmi pass1                                                                                       */
  /*  -.                            LDDMQがある場合,LMM-bufferingも存在 ... 必ずlmm_mode=3とする            */
  /*                                  並置IM-BUFFはよいとして                                               */
  /*                                  並置IM_PREFがlmm_mode=3であれば,前段LDRもlmm_mode=3が必要             */
  /*                                  つまり単独LDRをmode=0に拡張できるかは対応IM_PREF依存                  */
  /*  -.                            TRANSがある場合,隣接LMMは未使用     ... lmm_mode=0にできる              */
  /*  -.                            IM_PREF とRLOADは,セットで同じlmm_modeでなければならない                */
  /*  -.                            IM_DRAINとRSTOREはセットで同じlmm_modeでなければならない                */
  /*                                                                                                        */
  /* lmp/lmdの場合,ptop情報を使ってlmwd/lmrdに識別子がセットされている                                      */
  /* 水平方向に比較し,重複lmwdをマージする.                                                                 */
  /* LMM使用命令   (RLOAD,RSTORE,LDDMQ,TRANS) はFSMとのインタラクションがあるのでlmmi.v=1                   */
  /* LMM-buffering (conf.ea0op=OP_IM_BUFWR)   はFSMとのインタラクションがないのでlmmi.v=0                   */
  /*      col#3 col#2 col#1 col#0                                                                           */
  /* 5-0.                           imm_modeの初期値は,OP_IM_BUFRD/WRを含めて3                              */
  /* 5-0.                           stage毎にcolumn間(3-2,1-0)を検査し,上記機能の干渉に応じて               */
  /* 5-0.                           lmmを集約し,あとでIM_PREF/DRAINと整合させる                             */
  /* 5-1.           mode=0 | mode=0 1-0を検査し,#1/#0空  #0.mode=2/#1.mode=2(merge)                         */
  /* 5-1.                  =        1-0を検査し,同一なら #1.mode=2&#0.mode=2(merge)                         */
  /* 5-1.                 !=        1-0を検査し,その他   #1.mode=3,#0.mode=3                                */
  /* 5-2. mode=0 | mode=0           3-2を検査し,#3/#2空  #2.mode=2/#3.mode=2(merge)                         */
  /* 5-2.        =                  3-2を検査し,同一なら #3.mode=2&#2.mode=2(merge)                         */
  /* 5-2.       !=                  3-2を検査し,その他   #3.mode=3,#2.mode=3                                */
  /* 5-3.   mode=0,0 | mode=0,0     (3-2)と(1-0)を検査し,#2.mode=2/#0.mode=2&空有なら,mode=1(merge)         */
  /* 5-3.     mode=2 = mode=2       (3-2)と(1-0)を検査し,#2.mode=2&#0.mode=2&同一なら,mode=1(merge)         */
  /* 5-3.           !=              (3-2)と(1-0)を検査し,その他,                      そのまま              */
  /* 5-4.                           mapdist!=0とmop=IM_PREF/DRAINの検出により対応IMの調整が可能             */
  /*                                一通りmode拡張後,最後に,幅の狭いほうにあわせて再分割する                */
  for (i=0; i<EMAX_DEPTH; i++) {
    /* 5-1 */
    if (conf[i][1].cdw2.lmm_mode == 0 && conf[i][0].cdw2.lmm_mode == 0) { /* 0 0 */
      /* do nothing */
    }
    else if (conf[i][0].cdw2.lmm_mode == 0) /* 3 0 -> 2 0 */
      conf[i][1].cdw2.lmm_mode = 2; /* merge */
    else if (conf[i][1].cdw2.lmm_mode == 0) /* 0 3 -> 0 2 */
      conf[i][0].cdw2.lmm_mode = 2; /* merge */
    else if (lmmi[i][1].v     && lmmi[i][0].v
        /*&& lmmi[i][1].rw    == lmmi[i][0].rw*/
	  && lmmi[i][1].f     == lmmi[i][0].f
	  && lmmi[i][1].p     == lmmi[i][0].p
	  && lmmi[i][1].blk   == lmmi[i][0].blk
	  && lmmi[i][1].len   == lmmi[i][0].len
          && lmmi[i][1].top   == lmmi[i][0].top) { /* 3 3 -> 2 2 */
      conf[i][0].cdw2.lmm_mode = 2; /* merge */
      conf[i][1].cdw2.lmm_mode = 2; /* merge */
      lmmi[i][0].bcas |= (1<<1);    /* 1<-0 */
      lmmi[i][1].hcopy = 1;         /* mark as copy */
    }
    else { /* 3 3 */
      /* do nothing */
    }
    /* 5-2 */
    if (conf[i][3].cdw2.lmm_mode == 0 && conf[i][2].cdw2.lmm_mode == 0) { /* 0 0 */
      /* do nothing */
    }
    else if (conf[i][2].cdw2.lmm_mode == 0) /* 3 0 -> 2 0 */
      conf[i][3].cdw2.lmm_mode = 2; /* merge */
    else if (conf[i][3].cdw2.lmm_mode == 0) /* 0 3 -> 0 2 */
      conf[i][2].cdw2.lmm_mode = 2; /* merge */
    else if (lmmi[i][3].v     && lmmi[i][2].v
	/*&& lmmi[i][3].rw    == lmmi[i][2].rw*/
	  && lmmi[i][3].f     == lmmi[i][2].f
	  && lmmi[i][3].p     == lmmi[i][2].p
	  && lmmi[i][3].blk   == lmmi[i][2].blk
	  && lmmi[i][3].len   == lmmi[i][2].len
          && lmmi[i][3].top   == lmmi[i][2].top) { /* 3 3 -> 2 2 */
      conf[i][2].cdw2.lmm_mode = 2; /* merge */
      conf[i][3].cdw2.lmm_mode = 2; /* merge */
      lmmi[i][2].bcas |= (1<<3);    /* 3<-2 */
      lmmi[i][3].hcopy = 1;         /* mark as copy */
    }
    else { /* 3 3 */
      /* do nothing */
    }
    /* 5-3 */
    if (conf[i][3].cdw2.lmm_mode == 0 && conf[i][2].cdw2.lmm_mode == 0
     && conf[i][1].cdw2.lmm_mode == 0 && conf[i][0].cdw2.lmm_mode == 0) { /* 0 0 0 0 */
      /* do nothing */
    }
    else if (conf[i][3].cdw2.lmm_mode == 0 && conf[i][2].cdw2.lmm_mode == 0) { /* 0 0 - - */
      if (conf[i][1].cdw2.lmm_mode == 2 && conf[i][0].cdw2.lmm_mode == 2) { /* 0 0 2 2 */
	conf[i][0].cdw2.lmm_mode = 1; /* merge */
	conf[i][1].cdw2.lmm_mode = 1; /* merge */
      }
      else if (conf[i][0].cdw2.lmm_mode == 2) /* 0 0 0 2 */
	conf[i][0].cdw2.lmm_mode = 1; /* merge */
      else if (conf[i][1].cdw2.lmm_mode == 2) /* 0 0 2 0 */
	conf[i][1].cdw2.lmm_mode = 1; /* merge */
    }
    else if (conf[i][1].cdw2.lmm_mode == 0 && conf[i][0].cdw2.lmm_mode == 0) { /* - - 0 0 */
      if (conf[i][3].cdw2.lmm_mode == 2 && conf[i][2].cdw2.lmm_mode == 2) { /* 2 2 0 0 */
	conf[i][2].cdw2.lmm_mode = 1; /* merge */
	conf[i][3].cdw2.lmm_mode = 1; /* merge */
      }
      else if (conf[i][2].cdw2.lmm_mode == 2) /* 0 2 0 0 */
	conf[i][2].cdw2.lmm_mode = 1; /* merge */
      else if (conf[i][3].cdw2.lmm_mode == 2) /* 2 0 0 0 */
	conf[i][3].cdw2.lmm_mode = 1; /* merge */
    }
    else if (conf[i][1].cdw2.lmm_mode == 2 && conf[i][0].cdw2.lmm_mode == 2) { /* 0/2/3 0/2/3 2 2 */
      if (conf[i][2].cdw2.lmm_mode == 2) { /* 0/2 2 2 2 */
	if (lmmi[i][2].v     && lmmi[i][0].v
       /*&& lmmi[i][2].rw    == lmmi[i][0].rw*/
         && lmmi[i][2].f     == lmmi[i][0].f
         && lmmi[i][2].p     == lmmi[i][0].p
         && lmmi[i][2].blk   == lmmi[i][0].blk
         && lmmi[i][2].len   == lmmi[i][0].len
         && lmmi[i][2].top   == lmmi[i][0].top) {
	  conf[i][0].cdw2.lmm_mode = 1; /* merge */
	  conf[i][1].cdw2.lmm_mode = 1; /* merge */
	  conf[i][2].cdw2.lmm_mode = 1; /* merge */
	  lmmi[i][0].bcas |= (1<<2);    /* 2<-0 */
	  lmmi[i][2].bcas = 0;          /* reset bcas */
	  lmmi[i][2].hcopy = 1;         /* mark as copy */
	}
      }
      if (conf[i][3].cdw2.lmm_mode == 2) { /* 2 0/2 2 2 */
	if (lmmi[i][3].v     && lmmi[i][0].v
       /*&& lmmi[i][3].rw    == lmmi[i][0].rw*/
         && lmmi[i][3].f     == lmmi[i][0].f
         && lmmi[i][3].p     == lmmi[i][0].p
         && lmmi[i][3].blk   == lmmi[i][0].blk
         && lmmi[i][3].len   == lmmi[i][0].len
         && lmmi[i][3].top   == lmmi[i][0].top) {
	  conf[i][0].cdw2.lmm_mode = 1; /* merge */
	  conf[i][1].cdw2.lmm_mode = 1; /* merge */
	  conf[i][3].cdw2.lmm_mode = 1; /* merge */
	  lmmi[i][0].bcas |= (1<<3);    /* 3<-0 */
	  lmmi[i][3].bcas = 0;          /* reset bcas */
	  lmmi[i][3].hcopy = 1;         /* mark as copy */
	}
      }
    }
    else if (conf[i][0].cdw2.lmm_mode == 2) { /* 0/2/3 0/2/3 0 2 */
      if (conf[i][2].cdw2.lmm_mode == 2) { /* 0/2 2 0 2 */
	if (lmmi[i][2].v     && lmmi[i][0].v
       /*&& lmmi[i][2].rw    == lmmi[i][0].rw*/
         && lmmi[i][2].f     == lmmi[i][0].f
         && lmmi[i][2].p     == lmmi[i][0].p
         && lmmi[i][2].blk   == lmmi[i][0].blk
         && lmmi[i][2].len   == lmmi[i][0].len
         && lmmi[i][2].top   == lmmi[i][0].top) {
	  conf[i][0].cdw2.lmm_mode = 1; /* merge */
	  conf[i][2].cdw2.lmm_mode = 1; /* merge */
	  lmmi[i][0].bcas |= (1<<2);    /* 2<-0 */
	  lmmi[i][2].bcas = 0;          /* reset bcas */
	  lmmi[i][2].hcopy = 1;         /* mark as copy */
	}
      }
      if (conf[i][3].cdw2.lmm_mode == 2) { /* 2 0/2 0 2 */
	if (lmmi[i][3].v     && lmmi[i][0].v
       /*&& lmmi[i][3].rw    == lmmi[i][0].rw*/
         && lmmi[i][3].f     == lmmi[i][0].f
         && lmmi[i][3].p     == lmmi[i][0].p
         && lmmi[i][3].blk   == lmmi[i][0].blk
         && lmmi[i][3].len   == lmmi[i][0].len
         && lmmi[i][3].top   == lmmi[i][0].top) {
	  conf[i][0].cdw2.lmm_mode = 1; /* merge */
	  conf[i][3].cdw2.lmm_mode = 1; /* merge */
	  lmmi[i][0].bcas |= (1<<3);    /* 3<-0 */
	  lmmi[i][3].bcas = 0;          /* reset bcas */
	  lmmi[i][3].hcopy = 1;         /* mark as copy */
	}
      }
    }
    else if (conf[i][1].cdw2.lmm_mode == 2) { /* 0/2/3 0/2/3 2 0 */
      if (conf[i][2].cdw2.lmm_mode == 2) { /* 0/2 2 2 0 */
	if (lmmi[i][2].v     && lmmi[i][1].v
       /*&& lmmi[i][2].rw    == lmmi[i][1].rw*/
         && lmmi[i][2].f     == lmmi[i][1].f
         && lmmi[i][2].p     == lmmi[i][1].p
         && lmmi[i][2].blk   == lmmi[i][1].blk
         && lmmi[i][2].len   == lmmi[i][1].len
         && lmmi[i][2].top   == lmmi[i][1].top) {
	  conf[i][1].cdw2.lmm_mode = 1; /* merge */
	  conf[i][2].cdw2.lmm_mode = 1; /* merge */
	  lmmi[i][1].bcas |= (1<<2);    /* 2<-0 */
	  lmmi[i][2].bcas = 0;          /* reset bcas */
	  lmmi[i][2].hcopy = 1;         /* mark as copy */
	}
      }
      if (conf[i][3].cdw2.lmm_mode == 2) { /* 2 0/2 2 0 */
	if (lmmi[i][3].v     && lmmi[i][1].v
       /*&& lmmi[i][3].rw    == lmmi[i][1].rw*/
         && lmmi[i][3].f     == lmmi[i][1].f
         && lmmi[i][3].p     == lmmi[i][1].p
         && lmmi[i][3].blk   == lmmi[i][1].blk
         && lmmi[i][3].len   == lmmi[i][1].len
         && lmmi[i][3].top   == lmmi[i][1].top) {
	  conf[i][1].cdw2.lmm_mode = 1; /* merge */
	  conf[i][3].cdw2.lmm_mode = 1; /* merge */
	  lmmi[i][1].bcas |= (1<<3);    /* 3<-0 */
	  lmmi[i][3].bcas = 0;          /* reset bcas */
	  lmmi[i][3].hcopy = 1;         /* mark as copy */
	}
      }
    }
  }
  /* 5-4 */
  if (current_mapdist > 0) {
    for (i=0; i<EMAX_DEPTH; i++) {
      /* column-0 */
      if ((lmmi[i                ][0].v && lmmi[i                ][0].rw==0 && !lmmi[i                ][0].f && !lmmi[i                ][0].p
        && lmmi[i+current_mapdist][0].v && lmmi[i+current_mapdist][0].rw==0 && !lmmi[i+current_mapdist][0].f &&  lmmi[i+current_mapdist][0].p)    /* c+dist:lmp */
       || (lmmi[i                ][0].v && lmmi[i                ][0].rw==1 && !lmmi[i                ][0].f &&  lmmi[i                ][0].p  
        && lmmi[i+current_mapdist][0].v && lmmi[i+current_mapdist][0].rw==1 && !lmmi[i+current_mapdist][0].f && !lmmi[i+current_mapdist][0].p)) { /* c-dist:lmd */
	if      (conf[i][0].cdw2.lmm_mode < conf[i+current_mapdist][0].cdw2.lmm_mode) { /* [i]の集約を[i+c]に合わせて戻す */
	  if      (conf[i][0].cdw2.lmm_mode==1 && conf[i+current_mapdist][0].cdw2.lmm_mode==2) { /* -- -- 0/1 1 | -- -- 0/2 2 */
	    conf[i][0].cdw2.lmm_mode = 2;   /* split */
	    if (conf[i][1].cdw2.lmm_mode == 1)
	      conf[i][1].cdw2.lmm_mode = 2; /* split */
	    if (conf[i][3].cdw2.lmm_mode || conf[i][2].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=1,2\n", id[current_prefix].name, i, 0, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][0].cdw2.lmm_mode==1 && conf[i+current_mapdist][0].cdw2.lmm_mode==3) { /* -- -- --  1 | -- -- --  3 */
	    conf[i][0].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i][3].cdw2.lmm_mode || conf[i][2].cdw2.lmm_mode || conf[i][1].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=1,3\n", id[current_prefix].name, i, 0, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][0].cdw2.lmm_mode==2 && conf[i+current_mapdist][0].cdw2.lmm_mode==3) { /* -- -- --  2 | -- -- --  3 */
	    conf[i][0].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i][1].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=2,3\n", id[current_prefix].name, i, 0, current_mapdist);
	      exit(1);
	    }
	  }
	}
	else if (conf[i][0].cdw2.lmm_mode > conf[i+current_mapdist][0].cdw2.lmm_mode) { /* [i+c]の集約を[i]に合わせて戻す */
	  if      (conf[i][0].cdw2.lmm_mode==2 && conf[i+current_mapdist][0].cdw2.lmm_mode==1) { /* -- -- 0/2 2 | -- -- 0/1 1 */
	    conf[i+current_mapdist][0].cdw2.lmm_mode = 2;   /* split */
	    if (conf[i+current_mapdist][1].cdw2.lmm_mode == 1)
	      conf[i+current_mapdist][1].cdw2.lmm_mode = 2; /* split */
	    if (conf[i+current_mapdist][3].cdw2.lmm_mode || conf[i+current_mapdist][2].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=2,1\n", id[current_prefix].name, i, 0, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][0].cdw2.lmm_mode==3 && conf[i+current_mapdist][0].cdw2.lmm_mode==1) { /* -- -- --  3 | -- -- --  1 */
	    conf[i+current_mapdist][0].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i+current_mapdist][3].cdw2.lmm_mode || conf[i+current_mapdist][2].cdw2.lmm_mode || conf[i+current_mapdist][1].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=3,1\n", id[current_prefix].name, i, 0, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][0].cdw2.lmm_mode==3 && conf[i+current_mapdist][0].cdw2.lmm_mode==2) { /* -- -- --  3 | -- -- --  2 */
	    conf[i+current_mapdist][0].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i+current_mapdist][1].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=3,2\n", id[current_prefix].name, i, 0, current_mapdist);
	      exit(1);
	    }
	  }
	}
      }
      /* column-1 */
      if ((lmmi[i                ][1].v && lmmi[i                ][1].rw==0 && !lmmi[i                ][1].f && !lmmi[i                ][1].p
        && lmmi[i+current_mapdist][1].v && lmmi[i+current_mapdist][1].rw==0 && !lmmi[i+current_mapdist][1].f &&  lmmi[i+current_mapdist][1].p)    /* c+dist:lmp */
       || (lmmi[i                ][1].v && lmmi[i                ][1].rw==1 && !lmmi[i                ][1].f &&  lmmi[i                ][1].p  
        && lmmi[i+current_mapdist][1].v && lmmi[i+current_mapdist][1].rw==1 && !lmmi[i+current_mapdist][1].f && !lmmi[i+current_mapdist][1].p)) { /* c-dist:lmd */
	if      (conf[i][1].cdw2.lmm_mode < conf[i+current_mapdist][1].cdw2.lmm_mode) { /* [i]の集約を[i+c]に合わせて戻す */
	  if      (conf[i][1].cdw2.lmm_mode==1 && conf[i+current_mapdist][1].cdw2.lmm_mode==2) { /* -- -- 1 0/1 | -- -- 2 0/2 */
	    conf[i][1].cdw2.lmm_mode = 2;   /* split */
	    if (conf[i][0].cdw2.lmm_mode == 1)
	      conf[i][0].cdw2.lmm_mode = 2; /* split */
	    if (conf[i][3].cdw2.lmm_mode || conf[i][2].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=1,2\n", id[current_prefix].name, i, 1, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][1].cdw2.lmm_mode==1 && conf[i+current_mapdist][1].cdw2.lmm_mode==3) { /* -- --  1 -- | -- --  3 -- */
	    conf[i][1].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i][3].cdw2.lmm_mode || conf[i][2].cdw2.lmm_mode || conf[i][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=1,3\n", id[current_prefix].name, i, 1, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][1].cdw2.lmm_mode==2 && conf[i+current_mapdist][1].cdw2.lmm_mode==3) { /* -- --  2 -- | -- --  3 -- */
	    conf[i][1].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=2,3\n", id[current_prefix].name, i, 1, current_mapdist);
	      exit(1);
	    }
	  }
	}
	else if (conf[i][1].cdw2.lmm_mode > conf[i+current_mapdist][1].cdw2.lmm_mode) { /* [i+c]の集約を[i]に合わせて戻す */
	  if      (conf[i][1].cdw2.lmm_mode==2 && conf[i+current_mapdist][1].cdw2.lmm_mode==1) { /* -- -- 2 0/2 | -- -- 1 0/1 */
	    conf[i+current_mapdist][1].cdw2.lmm_mode = 2;   /* split */
	    if (conf[i+current_mapdist][0].cdw2.lmm_mode == 1)
	      conf[i+current_mapdist][0].cdw2.lmm_mode = 2; /* split */
	    if (conf[i+current_mapdist][3].cdw2.lmm_mode || conf[i+current_mapdist][2].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=2,1\n", id[current_prefix].name, i, 1, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][1].cdw2.lmm_mode==3 && conf[i+current_mapdist][1].cdw2.lmm_mode==1) { /* -- --  3 -- | -- --  1 -- */
	    conf[i+current_mapdist][1].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i+current_mapdist][3].cdw2.lmm_mode || conf[i+current_mapdist][2].cdw2.lmm_mode || conf[i+current_mapdist][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=3,1\n", id[current_prefix].name, i, 1, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][1].cdw2.lmm_mode==3 && conf[i+current_mapdist][1].cdw2.lmm_mode==2) { /* -- --  3 -- | -- --  2 -- */
	    conf[i+current_mapdist][1].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i+current_mapdist][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=3,2\n", id[current_prefix].name, i, 1, current_mapdist);
	      exit(1);
	    }
	  }
	}
      }
      /* column-2 */
      if ((lmmi[i                ][2].v && lmmi[i                ][2].rw==0 && !lmmi[i                ][2].f && !lmmi[i                ][2].p
        && lmmi[i+current_mapdist][2].v && lmmi[i+current_mapdist][2].rw==0 && !lmmi[i+current_mapdist][2].f &&  lmmi[i+current_mapdist][2].p)    /* c+dist:lmp */
       || (lmmi[i                ][2].v && lmmi[i                ][2].rw==1 && !lmmi[i                ][2].f &&  lmmi[i                ][2].p  
        && lmmi[i+current_mapdist][2].v && lmmi[i+current_mapdist][2].rw==1 && !lmmi[i+current_mapdist][2].f && !lmmi[i+current_mapdist][2].p)) { /* c-dist:lmd */
	if      (conf[i][2].cdw2.lmm_mode < conf[i+current_mapdist][2].cdw2.lmm_mode) { /* [i]の集約を[i+c]に合わせて戻す */
	  if      (conf[i][2].cdw2.lmm_mode==1 && conf[i+current_mapdist][2].cdw2.lmm_mode==2) { /* 0/1 1 -- -- | 0/2 2 -- -- */
	    conf[i][2].cdw2.lmm_mode = 2;   /* split */
	    if (conf[i][3].cdw2.lmm_mode == 1)
	      conf[i][3].cdw2.lmm_mode = 2; /* split */
	    if (conf[i][1].cdw2.lmm_mode || conf[i][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=1,2\n", id[current_prefix].name, i, 2, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][2].cdw2.lmm_mode==1 && conf[i+current_mapdist][2].cdw2.lmm_mode==3) { /* --  1 -- -- | --  3 -- -- */
	    conf[i][2].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i][3].cdw2.lmm_mode || conf[i][1].cdw2.lmm_mode || conf[i][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=1,3\n", id[current_prefix].name, i, 2, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][2].cdw2.lmm_mode==2 && conf[i+current_mapdist][2].cdw2.lmm_mode==3) { /* --  2 -- -- | --  3 -- -- */
	    conf[i][2].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i][3].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=2,3\n", id[current_prefix].name, i, 2, current_mapdist);
	      exit(1);
	    }
	  }
	}
	else if (conf[i][2].cdw2.lmm_mode > conf[i+current_mapdist][2].cdw2.lmm_mode) { /* [i+c]の集約を[i]に合わせて戻す */
	  if      (conf[i][2].cdw2.lmm_mode==2 && conf[i+current_mapdist][2].cdw2.lmm_mode==1) { /* 0/2 2 -- -- | 0/1 1 -- -- */
	    conf[i+current_mapdist][2].cdw2.lmm_mode = 2;   /* split */
	    if (conf[i+current_mapdist][3].cdw2.lmm_mode == 1)
	      conf[i+current_mapdist][3].cdw2.lmm_mode = 2; /* split */
	    if (conf[i+current_mapdist][1].cdw2.lmm_mode || conf[i+current_mapdist][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=2,1\n", id[current_prefix].name, i, 2, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][2].cdw2.lmm_mode==3 && conf[i+current_mapdist][2].cdw2.lmm_mode==1) { /* --  3 -- -- | --  1 -- -- */
	    conf[i+current_mapdist][2].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i+current_mapdist][3].cdw2.lmm_mode || conf[i+current_mapdist][1].cdw2.lmm_mode || conf[i+current_mapdist][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=3,1\n", id[current_prefix].name, i, 2, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][2].cdw2.lmm_mode==3 && conf[i+current_mapdist][2].cdw2.lmm_mode==2) { /* --  3 -- -- | --  2 -- -- */
	    conf[i+current_mapdist][2].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i+current_mapdist][3].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=3,2\n", id[current_prefix].name, i, 2, current_mapdist);
	      exit(1);
	    }
	  }
	}
      }
      /* column-3 */
      if ((lmmi[i                ][3].v && lmmi[i                ][3].rw==0 && !lmmi[i                ][3].f && !lmmi[i                ][3].p
        && lmmi[i+current_mapdist][3].v && lmmi[i+current_mapdist][3].rw==0 && !lmmi[i+current_mapdist][3].f &&  lmmi[i+current_mapdist][3].p)    /* c+dist:lmp */
       || (lmmi[i                ][3].v && lmmi[i                ][3].rw==1 && !lmmi[i                ][3].f &&  lmmi[i                ][3].p  
        && lmmi[i+current_mapdist][3].v && lmmi[i+current_mapdist][3].rw==1 && !lmmi[i+current_mapdist][3].f && !lmmi[i+current_mapdist][3].p)) { /* c-dist:lmd */
	if      (conf[i][3].cdw2.lmm_mode < conf[i+current_mapdist][3].cdw2.lmm_mode) { /* [i]の集約を[i+c]に合わせて戻す */
	  if      (conf[i][3].cdw2.lmm_mode==1 && conf[i+current_mapdist][3].cdw2.lmm_mode==2) { /* 1 0/1 -- -- | 2 0/2 -- -- */
	    conf[i][3].cdw2.lmm_mode = 2;   /* split */
	    if (conf[i][2].cdw2.lmm_mode == 1)
	      conf[i][2].cdw2.lmm_mode = 2; /* split */
	    if (conf[i][1].cdw2.lmm_mode || conf[i][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=1,2\n", id[current_prefix].name, i, 3, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][3].cdw2.lmm_mode==1 && conf[i+current_mapdist][3].cdw2.lmm_mode==3) { /*  1 -- -- -- |  3 -- -- -- */
	    conf[i][3].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i][2].cdw2.lmm_mode || conf[i][1].cdw2.lmm_mode || conf[i][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=1,3\n", id[current_prefix].name, i, 3, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][3].cdw2.lmm_mode==2 && conf[i+current_mapdist][3].cdw2.lmm_mode==3) { /*  2 -- -- -- |  3 -- -- -- */
	    conf[i][3].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i][2].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=2,3\n", id[current_prefix].name, i, 3, current_mapdist);
	      exit(1);
	    }
	  }
	}
	else if (conf[i][3].cdw2.lmm_mode > conf[i+current_mapdist][3].cdw2.lmm_mode) { /* [i+c]の集約を[i]に合わせて戻す */
	  if      (conf[i][3].cdw2.lmm_mode==2 && conf[i+current_mapdist][3].cdw2.lmm_mode==1) { /* 2 0/2 -- -- | 1 0/1 -- -- */
	    conf[i+current_mapdist][3].cdw2.lmm_mode = 2;   /* split */
	    if (conf[i+current_mapdist][2].cdw2.lmm_mode == 1)
	      conf[i+current_mapdist][2].cdw2.lmm_mode = 2; /* split */
	    if (conf[i+current_mapdist][1].cdw2.lmm_mode || conf[i+current_mapdist][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=2,1\n", id[current_prefix].name, i, 3, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][3].cdw2.lmm_mode==3 && conf[i+current_mapdist][3].cdw2.lmm_mode==1) { /*  3 -- -- -- |  1 -- -- -- */
	    conf[i+current_mapdist][3].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i+current_mapdist][2].cdw2.lmm_mode || conf[i+current_mapdist][1].cdw2.lmm_mode || conf[i+current_mapdist][0].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=3,1\n", id[current_prefix].name, i, 3, current_mapdist);
	      exit(1);
	    }
	  }
	  else if (conf[i][3].cdw2.lmm_mode==3 && conf[i+current_mapdist][3].cdw2.lmm_mode==2) { /*  3 -- -- -- |  2 -- -- -- */
	    conf[i+current_mapdist][3].cdw2.lmm_mode = 3;   /* split */
	    if (conf[i+current_mapdist][2].cdw2.lmm_mode) {
	      printf("in %s: [%d][%d] split_lmm found incomplete pair of lmr+lmp: mapdist=%d mode=3,2\n", id[current_prefix].name, i, 3, current_mapdist);
	      exit(1);
	    }
	  }
	}
      }
    }
  }
  /**********************************************************************************************************/
  /* Step 6 ... Set additional copy-flag for Vertical Broadcast(slave)                                      */
  /*            上から探して後段のvcopyを1にすると,lmf+lmxの場合,lmxがbitmapから消され,結果がDRAINされない  */
  /*            下から探して前段のvcopyを1にするのが正しい                                                  */
  /**********************************************************************************************************/
  for (j=0; j<EMAX_WIDTH; j++) {
    for (i=EMAX_DEPTH-1; i>0; i--) {
      for (k=i-1; k>=0; k--) {
	if (lmmi[i][j].v           && lmmi[k][j].v
         && lmmi[i][j].hcopy  == 0 && lmmi[k][j].hcopy  == 0
	 && lmmi[i][j].vcopy  == 0 && lmmi[k][j].vcopy  == 0
       /*&& lmmi[i][j].rw     == lmmi[k][j].rw*//* lmf and lmx can be merged */
         && lmmi[i][j].f      == lmmi[k][j].f
         && lmmi[i][j].p      == lmmi[k][j].p
         && lmmi[i][j].blk    == lmmi[k][j].blk
         && lmmi[i][j].len    == lmmi[k][j].len
	 && lmmi[i][j].top    == lmmi[k][j].top) {
	  if (lmmi[i][j].rw && lmmi[k][j].rw) { /* Check same addr-range in lmw/lmx */
	    printf("in %s: OP_ST with same addr-range in row[%d] and row[%d] will produce unpredictable result\n", id[current_prefix].name, k, i);
	    exit(1);
	  }
	  lmmi[k][j].vcopy = 1; /* mark as copy */
	  range_link[k][j] = i; /* for setting valid range[] from valid lmmi[] */
	                        /* share variables and reduce cache miss */
	}
      }
    }
  }
  /**********************************************************************************************************/
  /* Step 7 ... emit EMAX7 SC (soft-CGRA for manycore)                                                      */
  /**********************************************************************************************************/
  fprintf(s1fil, "/* EMAXSC start */\n");
  if (!s1fil_header_ready) {
    s1fil_header_ready = 1;
    fprintf(s1fil, "struct  sc_pth   {int dmy[16];} sc_pth[%d] __attribute__((aligned(64)));\n", EMAX_DEPTH);
    fprintf(s1fil, "struct  sc_param {int LOOP0; int LOOP1;} sc_param[%d];\n", EMAX_DEPTH);
    fprintf(s1fil, "struct  {unsigned long long b[%d][%d],o[%d][%d];} SCM0[%d] __attribute__((aligned(64)));\n", EMAX_NCHIP, EMAX_WIDTH, EMAX_NCHIP, EMAX_WIDTH, EMAX_DEPTH); /* eag0 */
    fprintf(s1fil, "struct  {unsigned long long b[%d][%d],o[%d][%d];} SCM1[%d] __attribute__((aligned(64)));\n", EMAX_NCHIP, EMAX_WIDTH, EMAX_NCHIP, EMAX_WIDTH, EMAX_DEPTH); /* eag1 */
    fprintf(s1fil, "volatile struct {unsigned long long r[%d][2][%d],enq[%d],d0[8-%d],deq[%d],d1[8-%d];} SCBR[%d] __attribute__((aligned(64)));\n", EMAX_NCHIP, EMAX_WIDTH*UNIT_WIDTH, EMAX_NCHIP, EMAX_NCHIP, EMAX_NCHIP, EMAX_NCHIP, EMAX_DEPTH); /* br   */
  }
  for (i=0; i<=last_row; i++)
    fprintf(s1fil, "void emax7sc_pth_%s_%02.2d(struct sc_param *);\n", id[current_prefix].name, i);
  fprintf(s1fil, "/* EMAXSC end */\n");

  /* multithreading IMAX2 */
  for (i=0; i<=last_row; i++) {
    /*********************************************************************************************************/
    fprintf(s2fil, "/* EMAXSC start */\n");
    fprintf(s2fil, "void emax7sc_pth_%s_%02.2d(struct sc_param *param) {\n", id[current_prefix].name, i);
    fprintf(s2fil, "Ull  CHIP, LOOP0=param->LOOP0, LOOP1=param->LOOP1;\n");
    fprintf(s2fil, "Ull  INIT1[%d], INIT0[%d];\n", EMAX_NCHIP, EMAX_NCHIP);
    fprintf(s2fil, "Uint uLOOP[%d], enq[%d];\n", EMAX_NCHIP, EMAX_NCHIP, EMAX_NCHIP);
    fprintf(s2fil, "Ull  awoo1[%d][%d], awoo0[%d][%d], mexd1[%d][%d], mexd0[%d][%d], alud[%d][%d];\n", EMAX_NCHIP, EMAX_WIDTH, EMAX_NCHIP, EMAX_WIDTH, EMAX_NCHIP, EMAX_WIDTH, EMAX_NCHIP, EMAX_WIDTH, EMAX_NCHIP, EMAX_WIDTH);
    fprintf(s2fil, "for (CHIP=0; CHIP<%d; CHIP++) { /* unit%d */\n", current_nchip, i);
    if (conf[0][0].cdw0.op1 == OP_WHILE)
      fprintf(s2fil, "LOOP1=1;uLOOP[CHIP]=LOOP0=%s;\n", id[dec[i][j].dexu.ex1h].name);
    else if (conf[0][1].cdw0.op1 == OP_FOR)
      fprintf(s2fil, "uLOOP[CHIP]=LOOP1*LOOP0;\n");
    else
      fprintf(s2fil, "LOOP1=1;uLOOP[CHIP]=LOOP0;\n");
    fprintf(s2fil, "}\n");
    fprintf(s2fil, "while (1) {\n");
    fprintf(s2fil, "for (CHIP=0; CHIP<%d; CHIP++)\n", current_nchip);
    fprintf(s2fil, "if (uLOOP[CHIP]) break;\n");
    fprintf(s2fil, "if (CHIP==%d) break;\n", current_nchip);
    fprintf(s2fil, "for (CHIP=0; CHIP<%d; CHIP++) {\n", current_nchip); /* 各unit内でsoft-multithreading */
    fprintf(s2fil, "if (uLOOP[CHIP]==0 || (%d && SCBR[%d].enq[CHIP]==SCBR[%d].deq[CHIP]) || (%d<%d && SCBR[%d].enq[CHIP]!=SCBR[%d].deq[CHIP])) continue;\n", i, (i+EMAX_DEPTH-1)%EMAX_DEPTH, (i+EMAX_DEPTH-1)%EMAX_DEPTH, i, last_row, i, i);
    fprintf(s2fil, "SCBR[%d].deq[CHIP] = 1-SCBR[%d].deq[CHIP];\n", (i+EMAX_DEPTH-1)%EMAX_DEPTH, (i+EMAX_DEPTH-1)%EMAX_DEPTH);
    fprintf(s2fil, "enq[CHIP] = SCBR[%d].enq[CHIP];\n", i);
    fprintf(s2fil, "INIT1[CHIP]=(uLOOP[CHIP]>LOOP1*LOOP0-LOOP0);\n");
    fprintf(s2fil, "INIT0[CHIP]=(uLOOP[CHIP]==uLOOP[CHIP]/LOOP0*LOOP0);\n");
    /* BR[i-1][0]             |  r  r     |           |  r  r     |        */
    /* BR[i-1][1]             |           |  r  r     |           |  r  r  */
    /* deq-          0  0  0  0->1  1  1  1->0  0  0  0->1  1  1  1->0  0  */
    /*                        V           V           V           V        */
    /* exe[i]                 *  *  * (E) *  *  * (E) *  *  * (E) *  *  *  ((先頭||enq-!=deq-) && enq0==deq0)なら,deq-=1-deq-,exe開始. exe後enq0=1-enq0 */
    /*                        A  |  |  V  A  |  |  V  A  |  |  V  A  |  |              */
    /* enq0          0  0  0  0  0  0  1  1  1  1  0  0  0  0  1  1  1  1  0  0  0  0  */
    /* BR[i][0]               |  w  w  |  r  r     |     w  w  |  r  r     |     w  w  */
    /* BR[i][1]               |        |     w  w  |  r  r     |     w  w  |  r  r     */
    /* deq0          0  0  0  0  0  0  0->1  1  1  1->0  0  0  0->1  1  1  1->0  0  0  */
    /*                                 V        A  V        A  V        A  V           */
    /* exe[i+1]                        *  *  * (E) *  *  * (E) *  *  * (E) *  *  * ((先頭||enq0!=deq0) && enq1==deq1)なら,deq0=1-deq0,exe開始. exe後enq1=1-enq1 */
    /*                                 A  |  |  V  A  |  |  V  A  |  |  V  A  |  |  
    /* enq1          0  0  0  0  0  0  0  0  0  1  1  1  1  0  0  0  0  1  1  1  1  0  0  0  0  */
    /* BR[i+1][0]                      |  w  w  |  r  r     |     w  w  |  r  r     |     w  w  */
    /* BR[i+1][1]                      |        |     w  w  |  r  r     |     w  w  |  r  r     */
    /* deq1          0  0  0  0  0  0  0  0  0  0->1  1  1  1->0  0  0  0->1  1  1  1->0  0  0  */

    for (j=0; j<EMAX_WIDTH; j++) {
      fprintf(s2fil, "{\n");
      /*********************************************************************************************************/
      if (conf[i][j].cdw2.brs0 == 2) { /* 0:off, 1:mr10, 2:tr0, 3:mr0  */
	int ts0   = conf[i][j].cdw2.ts0;    /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
	fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", i, j*UNIT_WIDTH+0, (i+EMAX_DEPTH-1)%EMAX_DEPTH, ts0);
      }
      if (conf[i][j].cdw2.brs1 == 2) { /* 0:off, 1:mr10, 2:tr0, 3:mr0  */
	int ts1   = conf[i][j].cdw2.ts1;    /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
	fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", i, j*UNIT_WIDTH+1, (i+EMAX_DEPTH-1)%EMAX_DEPTH, ts1);
      }
      if (conf[i][j].cdw2.brs2 == 2) { /* 0:off, 1:mr12, 2:tr2, 3:exdr */
	int ts2   = conf[i][j].cdw2.ts2;    /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
	fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", i, j*UNIT_WIDTH+2, (i+EMAX_DEPTH-1)%EMAX_DEPTH, ts2);
      }
      if (conf[i][j].cdw2.brs3 == 2) { /* 0:off, 1:mr13, 2:tr3         */
	int ts3   = conf[i][j].cdw2.ts3;    /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
	fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", i, j*UNIT_WIDTH+3, (i+EMAX_DEPTH-1)%EMAX_DEPTH, ts3);
      }
      fprintf(s2fil, "}\n");
    }

    for (j=0; j<EMAX_WIDTH; j++) {
      fprintf(s2fil, "{ Ull base, offs, adr, mexdist, mexlimit, load64;\n");
      fprintf(s2fil, "  static int emax7_unaligned_load_valid;\n");
      fprintf(s2fil, "  static Ull emax7_unaligned_load_high;\n");
      /*********************************************************************************************************/
      if (conf[i][j].cdw1.ea1op && conf[i][j].cdw1.ea1op < OP_IM_BUFRD) { /* LOAD1 */
	int eab   = conf[i][j].cdw1.eabbrs;
	int eao   = conf[i][j].cdw1.eaobrs;
	int ea1bs = conf[i][j].cdw1.ea1bs;  /* 0:ea1br, 1:ea1dr(ea1br+self-loop), 2:eabbrs, 3:ea1dr(eabbrs+self-loop) */
	int ea1os = conf[i][j].cdw1.ea1os;  /* 0:ea1or, 1:eaobrs */

	fprintf(s2fil, "base = (!(%d&1)||INIT0[CHIP]) ? ((%d&2)?SCBR[%d].r[CHIP][enq[CHIP]][%d]:SCM1[%d].b[CHIP][%d]) : awoo1[CHIP][%d];\n", ea1bs, ea1bs, (i+EMAX_DEPTH-1)%EMAX_DEPTH, eab, i, j, j); /*初回 or mexinitの毎INIT0*/
	fprintf(s2fil, "offs = eam(%d ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : SCM1[%d].o[CHIP][%d], %d);\n", ea1os, (i+EMAX_DEPTH-1)%EMAX_DEPTH, eao, i, j, (Uint)conf[i][j].cdw1.ea1msk);
	fprintf(s2fil, "mexdist = INIT0[CHIP] ? 0 : %d;\n", conf[i][j].cdw0.mex1dist==0? 0: conf[i][j].cdw0.mex1dist==1? 1: conf[i][j].cdw0.mex1dist==2? 2:
                                                            conf[i][j].cdw0.mex1dist==3? 4: conf[i][j].cdw0.mex1dist==4? 8: conf[i][j].cdw0.mex1dist==5?16:
                                                            conf[i][j].cdw0.mex1dist==6?32:64);
	fprintf(s2fil, "mexlimit = %d;\n", conf[i][j].cdw0.mexlimit== 0?    0: conf[i][j].cdw0.mexlimit== 1?    8: conf[i][j].cdw0.mexlimit== 2?    16:
                                           conf[i][j].cdw0.mexlimit== 3?   32: conf[i][j].cdw0.mexlimit== 4?   64: conf[i][j].cdw0.mexlimit== 5?   128:
                                           conf[i][j].cdw0.mexlimit== 6?  256: conf[i][j].cdw0.mexlimit== 7?  512: conf[i][j].cdw0.mexlimit== 8?  1024:
                                           conf[i][j].cdw0.mexlimit== 9? 2048: conf[i][j].cdw0.mexlimit==10? 4096: conf[i][j].cdw0.mexlimit==11?  8192:
                                           conf[i][j].cdw0.mexlimit==12?16384: conf[i][j].cdw0.mexlimit==13?32768: conf[i][j].cdw0.mexlimit==14? 65536:131072);
	switch (conf[i][j].cdw0.mex1op) {
	case OP_NOP:
	  fprintf(s2fil, "awoo1[CHIP][%d] = (Ull)(INIT0[CHIP]?base:awoo1[CHIP][%d]);\n", j, j);
	  break;
	case OP_ALWAYS: /* base++ 対応 */
	  fprintf(s2fil, "awoo1[CHIP][%d] = (Ull)(INIT0[CHIP]?base:awoo1[CHIP][%d])+(INIT0[CHIP]?0:mexdist);\n", j, j);
	  break;
	case OP_CMPA_LE:
	  fprintf(s2fil, "if (!mexlimit) awoo1[CHIP][%d] = (Ull)(INIT0[CHIP]?base:awoo1[CHIP][%d])+(INIT0[CHIP]?0:((mexd1[CHIP][%d]>>32)!=0xffffffff && (mexd1[CHIP][%d]>>32)<=(mexd0[CHIP][%d]>>32))?mexdist:0);\n", j, j, j, j, j);
	  break;
	case OP_CMPA_GE:
	  fprintf(s2fil, "if (!mexlimit) awoo1[CHIP][%d] = (Ull)(INIT0[CHIP]?base:awoo1[CHIP][%d])+(INIT0[CHIP]?0:((mexd0[CHIP][%d]>>32)!=0xffffffff && (mexd1[CHIP][%d]>>32)>=(mexd0[CHIP][%d]>>32))?mexdist:0);\n", j, j, j, j, j);
	  break;
	default:
	  printf("EMAXSC:undefined conf[%d][%d].mex1op=%d\n", i, j, conf[i][j].cdw0.mex1op);
	  break;
	}  

#if defined(__i386)
	fprintf(s2fil, "adr = (Uint)(awoo1[CHIP][%d] + offs);\n", j);
#else
	fprintf(s2fil, "adr = (Ull)(awoo1[CHIP][%d] + offs);\n", j);
#endif

	switch (conf[i][j].cdw1.ea1op) {
	case OP_LDR: /* 64bit lmm LMM is preloaded, random-access */
	  fprintf(s2fil, "load64 = *(Ull*)(adr&~7LL);\n");
	  fprintf(s2fil, "if ((adr&7) == 0)\n");
	  fprintf(s2fil, "  SCBR[%d].r[CHIP][enq[CHIP]][%d] = load64;\n", i, j*UNIT_WIDTH+1);
	  fprintf(s2fil, "else if (!emax7_unaligned_load_valid) { /* BR[][][1] */\n");
	  fprintf(s2fil, "  emax7_unaligned_load_valid = 1;\n");
	  fprintf(s2fil, "  emax7_unaligned_load_high = load64;\n");
	  fprintf(s2fil, "  SCBR[%d].r[CHIP][enq[CHIP]][%d] = load64 >> (adr&7)*8;\n", i, j*UNIT_WIDTH+1);
	  fprintf(s2fil, "}\n");
	  fprintf(s2fil, "else { /* BR[][][0] */\n");
	  fprintf(s2fil, "  emax7_unaligned_load_valid = 0;\n");
	  fprintf(s2fil, "  SCBR[%d].r[CHIP][enq[CHIP]][%d] = emax7_unaligned_load_high << (8-(adr&7))*8 | load64 >> (adr&7)*8;\n", i, j*UNIT_WIDTH+1);
	  fprintf(s2fil, "}\n");
	  break;
	case OP_LDWR: /* u32bit lmm LMM is preloaded, random-access */
	  fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = (Ull)*(Uint*)(adr&~3LL)<<32 | (Ull)*(Uint*)(adr&~3LL);\n", i, j*UNIT_WIDTH+1);
	  break;
	case OP_LDBR: /* u8bit lmm LMM is preloaded, random-access */
	  fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = (Ull)(Uint)*(Uchar*)adr<<32 | (Ull)(Uint)*(Uchar*)adr;\n", i, j*UNIT_WIDTH+1);
	  break;
	case OP_LDRQ: /* 64bit*4 lmm LMM is preloaded, random-access */
	  fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = *((Ull*)(adr&~31LL)+0);\n", i, j*UNIT_WIDTH+0);
          fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = *((Ull*)(adr&~31LL)+1);\n", i, j*UNIT_WIDTH+1);
	  fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = *((Ull*)(adr&~31LL)+2);\n", i, j*UNIT_WIDTH+2);
	  fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = *((Ull*)(adr&~31LL)+3);\n", i, j*UNIT_WIDTH+3);
	  break;
	default:
	  printf("EMAXSC:undefined conf[%d][%d].cdw1.ea1op=%d\n", i, j, conf[i][j].cdw1.ea1op);
	  break;
	}
      }

      /*********************************************************************************************************/
      if (conf[i][j].cdw1.ea0op && conf[i][j].cdw1.ea0op < OP_IM_BUFRD) { /* LOAD0 */
	int eab   = conf[i][j].cdw1.eabbrs;
	int eao   = conf[i][j].cdw1.eaobrs;
	int ea0bs = conf[i][j].cdw1.ea0bs;  /* 0:ea0br, 1:ea0dr(ea0br+self-loop), 2:eabbrs, 3:ea0dr(eabbrs+self-loop) */
	int ea0os = conf[i][j].cdw1.ea0os;  /* 0:ea0or, 1:eaobrs */
	
	fprintf(s2fil, "base = (!(%d&1)||INIT0[CHIP]) ? ((%d&2)?SCBR[%d].r[CHIP][enq[CHIP]][%d]:SCM0[%d].b[CHIP][%d]) : awoo0[CHIP][%d];\n", ea0bs, ea0bs, (i+EMAX_DEPTH-1)%EMAX_DEPTH, eab, i, j, j); /*初回 or mexinitの毎INIT0*/
	fprintf(s2fil, "offs = eam(%d ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : SCM0[%d].o[CHIP][%d], %d);\n", ea0os, (i+EMAX_DEPTH-1)%EMAX_DEPTH, eao, i, j, (Uint)conf[i][j].cdw1.ea0msk);
	fprintf(s2fil, "mexdist = INIT0[CHIP] ? 0 : %d;\n", conf[i][j].cdw0.mex0dist==0? 0: conf[i][j].cdw0.mex0dist==1? 1: conf[i][j].cdw0.mex0dist==2? 2:
                                                            conf[i][j].cdw0.mex0dist==3? 4: conf[i][j].cdw0.mex0dist==4? 8: conf[i][j].cdw0.mex0dist==5?16:
                                                            conf[i][j].cdw0.mex0dist==6?32:64);
	fprintf(s2fil, "mexlimit = %d;\n", conf[i][j].cdw0.mexlimit== 0?    0: conf[i][j].cdw0.mexlimit== 1?    8: conf[i][j].cdw0.mexlimit== 2?    16:
                                           conf[i][j].cdw0.mexlimit== 3?   32: conf[i][j].cdw0.mexlimit== 4?   64: conf[i][j].cdw0.mexlimit== 5?   128:
                                           conf[i][j].cdw0.mexlimit== 6?  256: conf[i][j].cdw0.mexlimit== 7?  512: conf[i][j].cdw0.mexlimit== 8?  1024:
                                           conf[i][j].cdw0.mexlimit== 9? 2048: conf[i][j].cdw0.mexlimit==10? 4096: conf[i][j].cdw0.mexlimit==11?  8192:
                                           conf[i][j].cdw0.mexlimit==12?16384: conf[i][j].cdw0.mexlimit==13?32768: conf[i][j].cdw0.mexlimit==14? 65536:131072);
	switch (conf[i][j].cdw0.mex0op) {
	case OP_NOP:
	  fprintf(s2fil, "awoo0[CHIP][%d] = (Ull)(INIT0[CHIP]?base:awoo0[CHIP][%d]);\n", j, j);
	  break;
	case OP_ALWAYS: /* base++ 対応 */
	  fprintf(s2fil, "awoo0[CHIP][%d] = (Ull)(INIT0[CHIP]?base:awoo0[CHIP][%d])+(INIT0[CHIP]?0:mexdist);\n", j, j);
	  break;
	case OP_CMPA_LE:
	  fprintf(s2fil, "if (!mexlimit) awoo0[CHIP][%d] = (Ull)(INIT0[CHIP]?base:awoo0[CHIP][%d])+(INIT0[CHIP]?0:((mexd1[CHIP][%d]>>32)!=0xffffffff && (mexd1[CHIP][%d]>>32)<=(mexd0[CHIP][%d]>>32))?mexdist:0);\n", j, j, j, j, j);
	  break;
	case OP_CMPA_GE:
	  fprintf(s2fil, "if (!mexlimit) awoo0[CHIP][%d] = (Ull)(INIT0[CHIP]?base:awoo0[CHIP][%d])+(INIT0[CHIP]?0:((mexd0[CHIP][%d]>>32)!=0xffffffff && (mexd1[CHIP][%d]>>32)>=(mexd0[CHIP][%d]>>32))?mexdist:0);\n", j, j, j, j, j);
	  break;
	default:
	  printf("EMAXSC:undefined conf[%d][%d].mex0op=%d\n", i, j, conf[i][j].cdw0.mex0op);
	  break;
	}  

#if defined(__i386)
	fprintf(s2fil, "adr = (Uint)(awoo0[CHIP][%d] + offs);\n", j);
#else
	fprintf(s2fil, "adr = (Ull)(awoo0[CHIP][%d] + offs);\n", j);
#endif

	switch (conf[i][j].cdw1.ea0op) {
	case OP_LDR: /* 64bit lmm LMM is preloaded, random-access */
	  fprintf(s2fil, "load64 = *(Ull*)(adr&~7LL);\n");
	  fprintf(s2fil, "if ((adr&7) == 0)\n");
	  fprintf(s2fil, "  SCBR[%d].r[CHIP][enq[CHIP]][%d] = load64;\n", i, j*UNIT_WIDTH+0);
	  fprintf(s2fil, "else if (!emax7_unaligned_load_valid) { /* BR[][][1] */\n");
	  fprintf(s2fil, "  emax7_unaligned_load_valid = 1;\n");
	  fprintf(s2fil, "  emax7_unaligned_load_high = load64;\n");
	  fprintf(s2fil, "  SCBR[%d].r[CHIP][enq[CHIP]][%d] = load64 >> (adr&7)*8;\n", i, j*UNIT_WIDTH+0);
	  fprintf(s2fil, "}\n");
	  fprintf(s2fil, "else { /* BR[][][0] */\n");
	  fprintf(s2fil, "  emax7_unaligned_load_valid = 0;\n");
	  fprintf(s2fil, "  SCBR[%d].r[CHIP][enq[CHIP]][%d] = emax7_unaligned_load_high << (8-(adr&7))*8 | load64 >> (adr&7)*8;\n", i, j*UNIT_WIDTH+0);
	  fprintf(s2fil, "}\n");
	  break;
	case OP_LDWR: /* u32bit lmm LMM is preloaded, random-access */
	  fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = (Ull)*(Uint*)(adr&~3LL)<<32 | (Ull)*(Uint*)(adr&~3LL);\n", i, j*UNIT_WIDTH+0);
	  break;
	case OP_LDBR: /* u8bit lmm LMM is preloaded, random-access */
	  fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = (Ull)(Uint)*(Uchar*)adr<<32 | (Ull)(Uint)*(Uchar*)adr;\n", i, j*UNIT_WIDTH+0);
	  break;
	case OP_LDRQ: /* 64bit*4 lmm LMM is preloaded, random-access */
	  fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = *((Ull*)(adr&~31LL)+0);\n", i, j*UNIT_WIDTH+0);
          fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = *((Ull*)(adr&~31LL)+1);\n", i, j*UNIT_WIDTH+1);
	  fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = *((Ull*)(adr&~31LL)+2);\n", i, j*UNIT_WIDTH+2);
	  fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = *((Ull*)(adr&~31LL)+3);\n", i, j*UNIT_WIDTH+3);
	  break;
	default:
	  printf("EMAXSC:undefined conf[%d][%d].cdw1.ea0op=%d\n", i, j, conf[i][j].cdw1.ea0op);
	  break;
	}
      }

      if (conf[i][j].cdw1.ea1op && conf[i][j].cdw1.ea1op < OP_IM_BUFRD) { /* LOAD1 */
	fprintf(s2fil, "mexd1[CHIP][%d] = SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", j, i, j*UNIT_WIDTH+1);
	
      }
      if (conf[i][j].cdw1.ea0op && conf[i][j].cdw1.ea0op < OP_IM_BUFRD) { /* LOAD0 */
	fprintf(s2fil, "mexd0[CHIP][%d] = SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", j, i, j*UNIT_WIDTH+0);
      }
      
      fprintf(s2fil, "}\n");
    }

    for (j=0; j<EMAX_WIDTH; j++) {
      fprintf(s2fil, "{ union { Uint i; float f; } f3, f2, f1, f0; Ull t3, t2, t1, t0, ex1, ex2, ex3, ex4, ex5, c1, c0, ex1_outd, ex2_outd;\n");
      /*********************************************************************************************************/
      if (conf[i][j].cdw0.op1 || conf[i][j].cdw0.op2 || conf[i][j].cdw0.op3) {
	int ex1brs = conf[i][j].cdw0.ex1brs; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
	int ex1s   = conf[i][j].cdw0.ex1s;   /* 0:ex1brs, 1:exdr(self-loop) */
	int ex1exp = conf[i][j].cdw0.ex1exp; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
	int ex2brs = conf[i][j].cdw0.ex2brs; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
	int ex2exp = conf[i][j].cdw0.ex2exp; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
	int ex3brs = conf[i][j].cdw0.ex3brs; /* 0:br0_0, 1:br0_1, ... 15:3_3 */
	int ex3exp = conf[i][j].cdw0.ex3exp; /* 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
	Ull e2imm  = conf[i][j].cdw3.e2imm;
	int e2is   = conf[i][j].cdw0.e2is;   /* 0:e2imm, 1:ex2, 2:ex3 */
	int e3imm  = conf[i][j].cdw0.e3imm;
	int e3is   = conf[i][j].cdw0.e3is;   /* 0:e3imm, 1:ex3 */
	int init   = conf[i][j].cdw0.init;   /* bit0:activate s1+INIT0 bit1:activate s2+INIT0 */
	int fold   = conf[i][j].cdw0.fold;   /* 0:normal, 1:load-exe-store folding */

	/* foldの場合も,1サイクル送らせる必要はなく,LDに続けてexe-stを実行すれば良い */
	switch (conf[i][j].cdw0.op1) {
	case OP_NOP:
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex1_outd = ex1;\n");
	  break;
	case OP_WHILE:
	case OP_FOR:
	  break;
	case OP_CFMA: /* [idx|32bit]*2 3in =(idx2==idx3)?r1+r2*r3:r1 */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "ex3 = exm(SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n",                                    !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex3brs, ex3exp);
	  fprintf(s2fil, "f1.i = (Uint)(ex1);\n");
	  fprintf(s2fil, "f2.i = (Uint)(ex2>>32);\n");
	  fprintf(s2fil, "f3.i = (Uint)(ex3>>32);\n");
	  fprintf(s2fil, "if (f2.i != -1 && f2.i == f3.i) {\n");
	  fprintf(s2fil, "  f2.i = (Uint)(ex2);\n");
	  fprintf(s2fil, "  f3.i = (Uint)(ex3);\n");
	  fprintf(s2fil, "  f0.f = f1.f + (f2.f * f3.f);\n");
	  fprintf(s2fil, "}\n");
	  fprintf(s2fil, "else {\n");
	  fprintf(s2fil, "  f0.f = f1.f;\n");
	  fprintf(s2fil, "}\n");
	  fprintf(s2fil, "t0 = f0.i;\n");
	  fprintf(s2fil, "ex1_outd = t0;\n");
	  break;
	case OP_FMA: /* 32bit*2 3in floating-point r1+r2*r3 */
	case OP_FMS: /* 32bit*2 3in floating-point r1-r2*r3 */
	  /* *(double*)&ex1_outd = *(double*)&r1 + (*(double*)&r2 * *(double*)&r3);*/
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "ex3 = exm(SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n",                                    !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex3brs, ex3exp);
	  fprintf(s2fil, "f1.i = (Uint)(ex1>>32);\n");
	  fprintf(s2fil, "f2.i = (Uint)(ex2>>32)^%08.8x;\n", conf[i][j].cdw0.op1==OP_FMA?0:0x80000000);
	  fprintf(s2fil, "f3.i = (Uint)(ex3>>32);\n");
	  fprintf(s2fil, "f0.f = f1.f + (f2.f * f3.f);\n");
	  fprintf(s2fil, "t2 = f0.i;\n");
	  fprintf(s2fil, "f1.i = (Uint)(ex1);\n");
	  fprintf(s2fil, "f2.i = (Uint)(ex2)^%08.8x;\n", conf[i][j].cdw0.op1==OP_FMA?0:0x80000000);
	  fprintf(s2fil, "f3.i = (Uint)(ex3);\n");
	  fprintf(s2fil, "f0.f = f1.f + (f2.f * f3.f);\n");
	  fprintf(s2fil, "t0 = f0.i;\n");
	  fprintf(s2fil, "ex1_outd = (t2<<32)|(t0);\n");
	  break;
	case OP_FAD: /* 32bit*2 3in floating-point r1+r2 */
	  /* *(double*)&ex1_outd = *(double*)&r1 + *(double*)&r2;*/
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "f1.i = (Uint)(ex1>>32);\n");
	  fprintf(s2fil, "f2.i = (Uint)(ex2>>32);\n");
	  fprintf(s2fil, "f0.f = f1.f + f2.f;\n");
	  fprintf(s2fil, "t2 = f0.i;\n");
	  fprintf(s2fil, "f1.i = (Uint)(ex1);\n");
	  fprintf(s2fil, "f2.i = (Uint)(ex2);\n");
	  fprintf(s2fil, "f0.f = f1.f + f2.f;\n");
	  fprintf(s2fil, "t0 = f0.i;\n");
	  fprintf(s2fil, "ex1_outd = (t2<<32)|(t0);\n");
	  break;
	case OP_FML: /* 32bit*2 3in floating-point r1*r2 */
	  /* *(double*)&ex1_outd = *(double*)&r1 * *(double*)&r2;*/
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "f1.i = (Uint)(ex1>>32);\n");
	  fprintf(s2fil, "f2.i = (Uint)(ex2>>32);\n");
	  fprintf(s2fil, "f0.f = f1.f * f2.f;\n");
	  fprintf(s2fil, "t2 = f0.i;\n");
	  fprintf(s2fil, "f1.i = (Uint)(ex1);\n");
	  fprintf(s2fil, "f2.i = (Uint)(ex2);\n");
	  fprintf(s2fil, "f0.f = f1.f * f2.f;\n");
	  fprintf(s2fil, "t0 = f0.i;\n");
	  fprintf(s2fil, "ex1_outd = (t2<<32)|(t0);\n");
	  break;
	case OP_ADD3: /* 32bit*2 3in integer add s1+(s2+s3) */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "ex3 = exm(SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n",                                    !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex3brs, ex3exp);
	  fprintf(s2fil, "t2 = (ex1>>32&0x00000000ffffffffLL)+((ex2>>32&0x00000000ffffffffLL)+(ex3>>32&0x00000000ffffffffLL));\n");
	  fprintf(s2fil, "t2 &= 0x00000000ffffffffLL;\n");
	  fprintf(s2fil, "t0 = (ex1    &0x00000000ffffffffLL)+((ex2    &0x00000000ffffffffLL)+(ex3    &0x00000000ffffffffLL));\n");
	  fprintf(s2fil, "t0 &= 0x00000000ffffffffLL;\n");
	  fprintf(s2fil, "ex1_outd = (t2<<32)|(t0);\n");
	  break;
	case OP_SUB3: /* 32bit*2 3in integer subtract s1-(s2+s3) */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "ex3 = exm(SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n",                                    !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex3brs, ex3exp);
	  fprintf(s2fil, "t2 = (ex1>>32&0x00000000ffffffffLL)-((ex2>>32&0x00000000ffffffffLL)+(ex3>>32&0x00000000ffffffffLL));\n");
	  fprintf(s2fil, "t2 &= 0x00000000ffffffffLL;\n");
	  fprintf(s2fil, "t0 = (ex1    &0x00000000ffffffffLL)-((ex2    &0x00000000ffffffffLL)+(ex3    &0x00000000ffffffffLL));\n");
	  fprintf(s2fil, "t0 &= 0x00000000ffffffffLL;\n");
	  fprintf(s2fil, "ex1_outd = (t2<<32)|(t0);\n");
	  break;
	case OP_ADD: /* 32bit*2 2in integer add s1+s2 */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "t2 = (ex1>>32&0x00000000ffffffffLL)+(ex2>>32&0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "t2 &= 0x00000000ffffffffLL;\n");
	  fprintf(s2fil, "t0 = (ex1    &0x00000000ffffffffLL)+(ex2    &0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "t0 &= 0x00000000ffffffffLL;\n");
	  fprintf(s2fil, "ex1_outd = (t2<<32)|(t0);\n");
	  break;
	case OP_SUB: /* 32bit*2 2in integer subtract s1-s2 */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "t2 = (ex1>>32&0x00000000ffffffffLL)-(ex2>>32&0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "t2 &= 0x00000000ffffffffLL;\n");
	  fprintf(s2fil, "t0 = (ex1    &0x00000000ffffffffLL)-(ex2    &0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "t0 &= 0x00000000ffffffffLL;\n");
	  fprintf(s2fil, "ex1_outd = (t2<<32)|(t0);\n");
	  break;
	case OP_CMP_EQ: /* 32bit*2 2in compare and set 1*2bit-CC */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "c1 = (ex1>>32&0x00000000ffffffffLL) == (ex2>>32&0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "c0 = (ex1    &0x00000000ffffffffLL) == (ex2    &0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "ex1_outd = (c1<<32)|c0;\n");
	  break;
	case OP_CMP_NE: /* 32bit*2 2in compare and set 1*2bit-CC */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "c1 = (ex1>>32&0x00000000ffffffffLL) != (ex2>>32&0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "c0 = (ex1    &0x00000000ffffffffLL) != (ex2    &0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "ex1_outd = (c1<<32)|c0;\n");
	  break;
	case OP_CMP_LT: /* 32bit*2 2in compare and set 1*2bit-CC */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "c1 = (ex1>>32&0x00000000ffffffffLL) < (ex2>>32&0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "c0 = (ex1    &0x00000000ffffffffLL) < (ex2    &0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "ex1_outd = (c1<<32)|c0;\n");
	  break;
	case OP_CMP_LE: /* 32bit*2 2in compare and set 1*2bit-CC */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "c1 = (ex1>>32&0x00000000ffffffffLL) <= (ex2>>32&0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "c0 = (ex1    &0x00000000ffffffffLL) <= (ex2    &0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "ex1_outd = (c1<<32)|c0;\n");
	  break;
	case OP_CMP_GT: /* 32bit*2 2in compare and set 1*2bit-CC */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "c1 = (ex1>>32&0x00000000ffffffffLL) > (ex2>>32&0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "c0 = (ex1    &0x00000000ffffffffLL) > (ex2    &0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "ex1_outd = (c1<<32)|c0;\n");
	  break;
	case OP_CMP_GE: /* 32bit*2 2in compare and set 1*2bit-CC */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "c1 = (ex1>>32&0x00000000ffffffffLL) >= (ex2>>32&0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "c0 = (ex1    &0x00000000ffffffffLL) >= (ex2    &0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "ex1_outd = (c1<<32)|c0;\n");
	  break;
	case OP_CMOV: /* 32bit*2 3in 2bit conditional move */
	  fprintf(s2fil, "ex1 = exm(!%d||(INIT1[CHIP]&&INIT0[CHIP])||((%d&1)&&INIT0[CHIP]) ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : alud[CHIP][%d], %d);\n", ex1s, init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex1brs, j, ex1exp);
	  fprintf(s2fil, "ex2 = exm(((%d&2)&&!INIT0[CHIP]) ? 0 : SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n", init, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, ex2exp);
	  fprintf(s2fil, "ex3 = exm(SCBR[%d].r[CHIP][enq[CHIP]][%d], %d);\n",                                    !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex3brs, ex3exp);
	  fprintf(s2fil, "c1 = ex1>>32&1;\n");
	  fprintf(s2fil, "c0 = ex1    &1;\n");
	  fprintf(s2fil, "t2 = c1 ? (ex2&0xffffffff00000000LL) : (ex3&0xffffffff00000000LL);\n");
	  fprintf(s2fil, "t0 = c0 ? (ex2&0x00000000ffffffffLL) : (ex3&0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "ex1_outd = t2 | t0;\n");
	  break;
	default:
	  printf("EMAXSC:undefined conf[%d][%d].cdw0.op1=%d\n", i, j, conf[i][j].cdw0.op1);
	  break;
	}

	switch (conf[i][j].cdw0.op2) {
	case OP_NOP:
	  if (conf[i][j].cdw0.op1 != OP_WHILE && conf[i][j].cdw0.op1 != OP_FOR)
	  fprintf(s2fil, "ex2_outd = ex1_outd;\n");
	  break;
	case OP_AND: /* 64bit 2in logical and s1&s2 */
	  fprintf(s2fil, "ex4 = %d==0 ? 0x%08.8x%08.8xLL : %d==1 ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", e2is, (Uint)(e2imm>>32), (Uint)e2imm, e2is, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex3brs);
	  fprintf(s2fil, "ex2_outd = ex1_outd & ex4;\n");
	  break;
	case OP_OR: /* 64bit 2in logical or s1|s2 */
	  fprintf(s2fil, "ex4 = %d==0 ? 0x%08.8x%08.8xLL : %d==1 ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", e2is, (Uint)(e2imm>>32), (Uint)e2imm, e2is, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex3brs);
	  fprintf(s2fil, "ex2_outd = ex1_outd | ex4;\n");
	  break;
	case OP_XOR: /* 64bit 2in logical xor s1^s2 */
	  fprintf(s2fil, "ex4 = %d==0 ? 0x%08.8x%08.8xLL : %d==1 ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", e2is, (Uint)(e2imm>>32), (Uint)e2imm, e2is, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex2brs, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex3brs);
	  fprintf(s2fil, "ex2_outd = ex1_outd ^ ex4;\n");
	  break;
	default:
	  printf("EMAXSC:undefined conf[%d][%d].cdw0.op2=%d\n", i, j, conf[i][j].cdw0.op2);
	  break;
	}
	
	switch (conf[i][j].cdw0.op3) {
	case OP_NOP:
	  if (conf[i][j].cdw0.op1 != OP_WHILE && conf[i][j].cdw0.op1 != OP_FOR)
	  fprintf(s2fil, "alud[CHIP][%d] = ex2_outd;\n", j);
	  break;
	case OP_SLL: /* 32bit*2 2in 32bit logical shift to left */
	  fprintf(s2fil, "ex5 = %d==0 ? 0x%08.8x : SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", e3is, e3imm, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex3brs);
	  fprintf(s2fil, "t1 = (Ull)(ex2_outd     &0xffffffff00000000LL)<<ex5;\n");
	  fprintf(s2fil, "t0 = (Ull)(ex2_outd<<ex5&0x00000000ffffffffLL);\n");
	  fprintf(s2fil, "alud[CHIP][%d] = t1 | t0;\n", j);
	  break;
	case OP_SRL: /* 32bit*2 2in 32bit logical shift to right */
	  fprintf(s2fil, "ex5 = %d==0 ? 0x%08.8x : SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", e3is, e3imm, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ex3brs);
	  fprintf(s2fil, "t1 = (Ull)(ex2_outd>>ex5&0xffffffff00000000LL);\n");
	  fprintf(s2fil, "t0 = (Ull)(ex2_outd     &0x00000000ffffffffLL)>>ex5;\n");
	  fprintf(s2fil, "alud[CHIP][%d] = t1 | t0;\n", j);
	  break;
	default:
	  printf("EMAXSC:undefined conf[%d][%d].cdw0.op3=%d\n", i, j, conf[i][j].cdw0.op3);
	  break;
	}

	if (conf[i][j].cdw2.brs2 == 3) { /* 0:off, 1:mr12, 2:tr2, 3:exdr */
	  fprintf(s2fil, "SCBR[%d].r[CHIP][enq[CHIP]][%d] = alud[CHIP][%d];\n", i, j*UNIT_WIDTH+2, j);
	}
      }
      fprintf(s2fil, "}\n");
    }

    for (j=0; j<EMAX_WIDTH; j++) {
      fprintf(s2fil, "{ Ull cs0, cs1, cs2, cs3, cex, base, offs, adr, mexdist;\n");
      /*********************************************************************************************************/
      if (dec[i][j].dcex.op) { /* confに存在しない */
	int fold    = conf[i][j].cdw0.fold;    /* 0:normal, 1:load-exe-store folding */
	int cs0     = conf[i][j].cdw1.cs0;     /* 0:br0_0, 1:br0_1, ... 15:3_3 */
	int cs1     = conf[i][j].cdw1.cs1;     /* 0:br0_0, 1:br0_1, ... 15:3_3 */
	int cs2     = conf[i][j].cdw1.cs2;     /* 0:br0_0, 1:br0_1, ... 15:3_3 */
	int cs3     = conf[i][j].cdw1.cs3;     /* 0:br0_0, 1:br0_1, ... 15:3_3 */
	int cex_tab = conf[i][j].cdw1.cex_tab; /* c3.c2.c1.c0の組合せ (cop=NOPの場合,ffff) */

	fprintf(s2fil, "cs0 = SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, cs0);
	fprintf(s2fil, "cs1 = SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, cs1);
	fprintf(s2fil, "cs2 = SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, cs2);
	fprintf(s2fil, "cs3 = SCBR[%d].r[CHIP][enq[CHIP]][%d];\n", !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, cs3);
	fprintf(s2fil, "cex = ((0x%04.4x>>(((cs3>>32&1)<<3)|((cs2>>32&1)<<2)|((cs1>>32&1)<<1)|(cs0>>32&1))&1)?2:0) | ((0x%04.4x>>(((cs3&1)<<3)|((cs2&1)<<2)|((cs1&1)<<1)|(cs0&1))&1)?1:0);\n", cex_tab, cex_tab);
      }

      /*********************************************************************************************************/
      if (conf[i][j].cdw1.ea0op > OP_IM_BUFRD) { /* STORE */
	int fold  = conf[i][j].cdw0.fold;   /* 0:normal, 1:load-exe-store folding */
	int eab   = conf[i][j].cdw1.eabbrs;
	int eao   = conf[i][j].cdw1.eaobrs;
	int ea0bs = conf[i][j].cdw1.ea0bs;  /* 0:ea0br, 1:ea0dr(ea0br+self-loop), 2:eabbrs, 3:ea0dr(eabbrs+self-loop) */
	int ea0os = conf[i][j].cdw1.ea0os;  /* 0:ea0or, 1:eaobrs */
	int ts0   = conf[i][j].cdw2.ts0;    /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
	int ts1   = conf[i][j].cdw2.ts1;    /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
	int ts2   = conf[i][j].cdw2.ts2;    /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
	int ts3   = conf[i][j].cdw2.ts3;    /* 0:br0_0, 1:br0_1, ... 15:br3_3 */
	int mws0  = conf[i][j].cdw2.mws0;   /* 0:lmwd0, 1:exdr, 2:ts0 */
	int mws1  = conf[i][j].cdw2.mws1;   /* 0:lmwd1, 1:exdr, 2:ts1 */
	int mws2  = conf[i][j].cdw2.mws2;   /* 0:lmwd2, 1:exdr, 2:ts2 */
	int mws3  = conf[i][j].cdw2.mws3;   /* 0:lmwd3, 1:exdr, 2:ts3 */

	if (!dec[i][j].dcex.op)
	  fprintf(s2fil, "cex = 3;\n");

	fprintf(s2fil, "base = (!(%d&1)||INIT0[CHIP]) ? ((%d&2)?SCBR[%d].r[CHIP][enq[CHIP]][%d]:SCM0[%d].b[CHIP][%d]) : awoo0[CHIP][%d];\n", ea0bs, ea0bs, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, eab, i, j, j); /*初回 or mexinitの毎INIT0*/
	fprintf(s2fil, "offs = eam(%d ? SCBR[%d].r[CHIP][enq[CHIP]][%d] : SCM0[%d].o[CHIP][%d], %d);\n", ea0os, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, eao, i, j, (Uint)conf[i][j].cdw1.ea0msk);
	fprintf(s2fil, "mexdist = INIT0[CHIP] ? 0 : %d;\n", conf[i][j].cdw0.mex0dist==0? 0: conf[i][j].cdw0.mex0dist==1? 1: conf[i][j].cdw0.mex0dist==2? 2:
                                                            conf[i][j].cdw0.mex0dist==3? 4: conf[i][j].cdw0.mex0dist==4? 8: conf[i][j].cdw0.mex0dist==5?16:
                                                            conf[i][j].cdw0.mex0dist==6?32:64);
	switch (conf[i][j].cdw0.mex0op) {
	case OP_NOP:
	  fprintf(s2fil, "awoo0[CHIP][%d] = (Ull)(INIT0[CHIP]?base:awoo0[CHIP][%d]);\n", j, j);
	  break;
	case OP_ALWAYS: /* base++ 対応 */
	  fprintf(s2fil, "awoo0[CHIP][%d] = (Ull)(INIT0[CHIP]?base:awoo0[CHIP][%d])+(INIT0[CHIP]?0:mexdist);\n", j, j);
	  break;
	default:
	  printf("EMAXSC:undefined conf[%d][%d].mex0op=%d\n", i, j, conf[i][j].cdw0.mex0op);
	  break;
	}  

#if defined(__i386)
	fprintf(s2fil, "adr = (Uint)(awoo0[CHIP][%d] + offs);\n", j);
#else
	fprintf(s2fil, "adr = (Ull)(awoo0[CHIP][%d] + offs);\n", j);
#endif
	  
	switch (conf[i][j].cdw1.ea0op) {
	case OP_STR: /* 64bit lmm LMM is drained. random-access */
	  fprintf(s2fil, "if (cex>>1&1) *((Uint*)(adr&~7LL)+1) = (%d==1? alud[CHIP][%d] : SCBR[%d].r[CHIP][enq[CHIP]][%d])>>32;\n", mws0, j, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ts0);
	  fprintf(s2fil, "if (cex   &1) *((Uint*)(adr&~7LL)  ) = (%d==1? alud[CHIP][%d] : SCBR[%d].r[CHIP][enq[CHIP]][%d]);\n",     mws0, j, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ts0);
	  break;
	case OP_STWR: /* 32bit lmm LMM is drained. random-access */
	  fprintf(s2fil, "if (cex   &1) *(Uint*)(adr&~3LL) = (%d==1? alud[CHIP][%d] : SCBR[%d].r[CHIP][enq[CHIP]][%d]);\n", mws0, j, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ts0);
	  break;
	case OP_STBR: /* 8bit lmm LMM is drained. random-access */
	  fprintf(s2fil, "if (cex   &1) *(Uchar*)adr = (%d==1? alud[CHIP][%d] : SCBR[%d].r[CHIP][enq[CHIP]][%d]);\n", mws0, j, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ts0);
	  break;
	case OP_STRQ: /* 64bit*4 lmm LMM is drained. random-access */
	  fprintf(s2fil, "*((Ull*)(adr&~31LL)+0) = (%d==1? alud[CHIP][0] : SCBR[%d].r[CHIP][enq[CHIP]][%d]);\n", mws0, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ts0);
	  fprintf(s2fil, "*((Ull*)(adr&~31LL)+1) = (%d==1? alud[CHIP][1] : SCBR[%d].r[CHIP][enq[CHIP]][%d]);\n", mws1, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ts1);
	  fprintf(s2fil, "*((Ull*)(adr&~31LL)+2) = (%d==1? alud[CHIP][2] : SCBR[%d].r[CHIP][enq[CHIP]][%d]);\n", mws2, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ts2);
	  fprintf(s2fil, "*((Ull*)(adr&~31LL)+3) = (%d==1? alud[CHIP][3] : SCBR[%d].r[CHIP][enq[CHIP]][%d]);\n", mws3, !fold?(i+EMAX_DEPTH-1)%EMAX_DEPTH:i, ts3);
	  break;
	default:
	  printf("EMAXSC:mmp:undefined op_mm=%d\n", dec[i][j].dmop0.op);
	  break;
	}
      }
      fprintf(s2fil, "}\n");
    }

    fprintf(s2fil, "SCBR[%d].enq[CHIP] = 1-SCBR[%d].enq[CHIP];\n", i, i);
    fprintf(s2fil, "uLOOP[CHIP]--;\n");
    fprintf(s2fil, "}\n"); /* for (CHIP) */
    fprintf(s2fil, "}\n"); /* while (1) */
    fprintf(s2fil, "}\n"); /* pth_func() */
    fprintf(s2fil, "/* EMAXSC end */\n");
  }

  /**********************************************************************************************************/
  fprintf(ofile, "#ifdef EMAXSC\n");
  fprintf(ofile, "/* EMAXSC start */\n");
    
  /* init REGV(breg) */
  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      for (k=0; k<UNIT_WIDTH; k++) {
	if (regv[i][j].br[k].v) {
	  if (id[regv[i][j].br[k].h].chip) { /* CHIP */
	    for (c=0; c<current_nchip; c++) {
	      fprintf(ofile, "SCBR[%d].r[%d][0][%d] = %d;\n", i, c, j*UNIT_WIDTH+k, c);
	      fprintf(ofile, "SCBR[%d].r[%d][1][%d] = %d;\n", i, c, j*UNIT_WIDTH+k, c);
	    }
	  }
	  else if (id[regv[i][j].br[k].h].cidx) { /* xxx[CHIP] */
	    for (c=0; c<current_nchip; c++) {
	      fprintf(ofile, "SCBR[%d].r[%d][0][%d] = %s[%d];\n", i, c, j*UNIT_WIDTH+k, id[regv[i][j].br[k].h].name, c);
	      fprintf(ofile, "SCBR[%d].r[%d][1][%d] = %s[%d];\n", i, c, j*UNIT_WIDTH+k, id[regv[i][j].br[k].h].name, c);
	    }
	  }
	  else if (regv[i][j].br[k].s < 0) {
	    for (c=0; c<current_nchip; c++) {
	      fprintf(ofile, "SCBR[%d].r[%d][0][%d] = %s;\n", i, c, j*UNIT_WIDTH+k, id[regv[i][j].br[k].h].name);
	      fprintf(ofile, "SCBR[%d].r[%d][1][%d] = %s;\n", i, c, j*UNIT_WIDTH+k, id[regv[i][j].br[k].h].name);
	    }
	  }
	  else {
	    for (c=0; c<current_nchip; c++) {
	      fprintf(ofile, "SCBR[%d].r[%d][0][%d] = %s[%d];\n", i, c, j*UNIT_WIDTH+k, id[regv[i][j].br[k].h].name, regv[i][j].br[k].s);
	      fprintf(ofile, "SCBR[%d].r[%d][1][%d] = %s[%d];\n", i, c, j*UNIT_WIDTH+k, id[regv[i][j].br[k].h].name, regv[i][j].br[k].s);
	    }
	  }
	}
      }
    }
  }

  /* init REGV(eag) */
  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      if (regv[i][j].ea0b_v) {
	if (id[regv[i][j].ea0b_h].cidx) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM0[%d].b[%d][%d] = (Ull)%s[%d];\n", i, c, j, id[regv[i][j].ea0b_h].name, c);
	}
	else if (regv[i][j].ea0b_s < 0) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM0[%d].b[%d][%d] = (Ull)%s;\n", i, c, j, id[regv[i][j].ea0b_h].name);
	}
	else {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM0[%d].b[%d][%d] = (Ull)%s[%d];\n", i, c, j, id[regv[i][j].ea0b_h].name, regv[i][j].ea0b_s);
	}
      }
      if (regv[i][j].ea0o_v) {
	if (id[regv[i][j].ea0o_h].cidx) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM0[%d].o[%d][%d] = (Ull)%s[%d];\n", i, c, j, id[regv[i][j].ea0o_h].name, c);
	}
	else if (regv[i][j].ea0o_s < 0) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM0[%d].o[%d][%d] = (Ull)%s;\n", i, c, j, id[regv[i][j].ea0o_h].name);
	}
	else {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM0[%d].o[%d][%d] = (Ull)%s[%d];\n", i, c, j, id[regv[i][j].ea0o_h].name, regv[i][j].ea0o_s);
	}
      }
      if (regv[i][j].ea1b_v) {
	if (id[regv[i][j].ea1b_h].cidx) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM1[%d].b[%d][%d] = (Ull)%s[%d];\n", i, c, j, id[regv[i][j].ea1b_h].name, c);
	}
	else if (regv[i][j].ea1b_s < 0) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM1[%d].b[%d][%d] = (Ull)%s;\n", i, c, j, id[regv[i][j].ea1b_h].name);
	}
	else {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM1[%d].b[%d][%d] = (Ull)%s[%d];\n", i, c, j, id[regv[i][j].ea1b_h].name, regv[i][j].ea1b_s);
	}
      }
      if (regv[i][j].ea1o_v) {
	if (id[regv[i][j].ea1o_h].cidx) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM1[%d].o[%d][%d] = (Ull)%s[%d];\n", i, c, j, id[regv[i][j].ea1o_h].name, c);
	}
	else if (regv[i][j].ea1o_s < 0) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM1[%d].o[%d][%d] = (Ull)%s;\n", i, c, j, id[regv[i][j].ea1o_h].name);
	}
	else {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "SCM1[%d].o[%d][%d] = (Ull)%s[%d];\n", i, c, j, id[regv[i][j].ea1o_h].name, regv[i][j].ea1o_s);
	}
      }
    }
  }

  for (i=0; i<=last_row; i++)
    fprintf(ofile, "sc_param[%d].LOOP0=LOOP0; sc_param[%d].LOOP1=LOOP1; pthread_create((pthread_t*)&sc_pth[%d], 0, emax7sc_pth_%s_%02.2d, &sc_param[%d]);\n", i, i, i, id[current_prefix].name, i, i);
  for (i=0; i<=last_row; i++)
    fprintf(ofile, "pthread_join(*(pthread_t*)&sc_pth[%d], 0);\n", i);

  fprintf(ofile, "/* EMAXSC end */\n");
  fprintf(ofile, "#endif\n");
  /**********************************************************************************************************/
  /* Step 8 ... emit EMAX7 CGRA                                                                             */
  /**********************************************************************************************************/
  /* resolve lmmi -> host-lmmi */
  fprintf(ofile, "#ifndef EMAXSC\n");
  fprintf(ofile, "\temax7[LANE].lmmio = emax7[LANE].lmmic;\n");
  fprintf(ofile, "\temax7[LANE].lmmic = 1-emax7[LANE].lmmic;\n");
  fprintf(ofile, "\temax7[LANE].mapdist = %d;\n", current_mapdist);
  lmmi_first_loc = -1;
  for (j=0; j<EMAX_WIDTH; j++) {
    lmmi_bitmap[j] = 0LL;
    range_bitmap[j] = 0LL;
  }
  /*                               lmmi-loc  v  top  blk  len  rw  f  p *//*   lmmi bitmap drain load ar pl/pd last_drain */
  /* lmr                               curr  1  top  blk  len   0  0  0 *//*  8  1      1     -   *1   -   -      -       */
  /* lmf                               curr  1  top  blk  len   0  1  0 *//* 10  1      1     -   *1   -   -      -       */
  /* lmr+lmp(mapdist!=0)               curr  1  top  blk  len   0  0  0 *//*  8  1      1     -   *1   -   -      -       */
  /*                                 c+dist  1 ptop  blk  len   0  0  1 *//*  9  1      1     -    -   -  *1      -       */
  /* lmr+lmp(mapdist==0) ofs=ptop-top  curr  1  top  blk  len   0  0  1 *//*  9  1      1     -   *1   1  *1      -       */
  /* lddmq                             curr  1  top  -    -     0  1  1 *//* 11  -      -     -    -   -   -      -       */
  /* lmw    (mapdist!=0)               curr  1  top  blk  len   1  0  0 *//* 12  1      1     -    -   -   -      -       */
  /*                                 c-dist                             *//*     1**    1    *1    -   -   -     *1       */
  /* lmw    (mapdist==0)               curr  1  top  blk  len   1  0  0 *//* 12  1      1    *1    -   -   -     *1       */
  /* lmx    (mapdist!=0)               curr  1  top  blk  len   1  1  0 *//* 14  1      1     -   *1   -   -      -       */
  /*                                 c-dist                             *//*     1**    1    *1    -   -   -     *1       */
  /* lmx    (mapdist==0)               curr  1  top  blk  len   1  1  0 *//* 14  1      1    *1   *1   -   -     *1       */
  /* lmw+lmd(mapdist!=0)               curr  1  top  blk  len   1  0  0 *//* 12  1      1     -    -   -   -      -       */
  /*                                 c-dist  1 ptop  blk  len   1  0  1 *//* 13  1      1     -    -   -  *1     *1       */
  /* lmw+lmd(mapdist==0) ofs=ptop-top  curr  1  top  blk  len   1  0  1 *//* 13  1      1     -    -   1  *1     *1       */
  /* tr                                curr  1  top  -    -     1  1  1 *//* 15  -      -     -    -   -   -      -       */
  /*                                                      v&~copy:  lmmi_bitmap[c     ]=1  rw[c]||(md&&rw[c+d]&&!p[c+d]   */
  /*                                                              **lmmi_bitmap[c-dist]=1       !wr||f                    */
  /*                                                      v      : range_bitmap[c     ]=1             ofs                 */
  /*                                                             **range_bitmap[c-dist]=1                  p              */
  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      if (lmmi[i][j].v && !(lmmx[i][j].lenv == T_IMMEDIATE && lmmi[i][j].len==0x7ffff) && !lmmi[i][j].hcopy && !lmmi[i][j].vcopy) {
        lmmi_bitmap[j] |= (1LL<<i);
	if (lmmi_first_loc == -1 && !lmmi[i][j].rw && !lmmi[i][j].p) /* lmrとlmfの場合,DYNAMIC_SCONの基準位置として記憶 */
	  lmmi_first_loc = i*EMAX_WIDTH+j;
	if (current_mapdist && lmmi[i][j].rw && !lmmi[i][j].p) /* lmwとlmxの場合,emax7_check_lmmi_and_dma()のためにmarkが必要 */
	  lmmi_bitmap[j] |= (1LL<<(i-current_mapdist));
	if (lmmi[i][j].cidx) { /* NCHIP間でlmm_topが異なる場合 */
	  for (c=0; c<current_nchip; c++) {
	    if (lmmx[i][j].lenv == T_IMMEDIATE) /* IMMEDIATE */
	      fprintf(ofile, "\t*(Uint*)&emax7[LANE].lmmi[%d][%d][%d][emax7[LANE].lmmic] = 0x%08.8x|(%s<<2);\n", c, i, j, *(Uint*)&lmmi[i][j]&0xfffffffb, id[lmmx[i][j].forceh].name);
	    else /* VARIABLE */
	      fprintf(ofile, "\t*(Uint*)&emax7[LANE].lmmi[%d][%d][%d][emax7[LANE].lmmic] = ((%s-1)<<13)|0x%04.4x|(%s<<2);\n", c, i, j, id[lmmx[i][j].lenh].name, *(Ushort*)&lmmi[i][j]&0x1ffb, id[lmmx[i][j].forceh].name);
	    if (lmmi[i][j].ofs) /* ptophが有効の場合,topが[CHIP]ならptopも[CHIP]でなければおかしい */
	      fprintf(ofile, "\temax7[LANE].lmmi[%d][%d][%d][emax7[LANE].lmmic].ofs = (Uchar*)%s[%d] - (Uchar*)%s[%d];\n", c, i, j, (char*)lmmi[i][j].top + lmmi[i][j].ofs, c, (char*)lmmi[i][j].top, c);
	    else /* ptophが無効 */
	      fprintf(ofile, "\temax7[LANE].lmmi[%d][%d][%d][emax7[LANE].lmmic].ofs = 0;\n", c, i, j);
	    fprintf(ofile, "\temax7[LANE].lmmi[%d][%d][%d][emax7[LANE].lmmic].top = %s[%d];\n", c, i, j, (char*)lmmi[i][j].top, c);
	  }
	}
	else {  /* NCHIP間でlmm_topが共通の場合 */
	  if (lmmx[i][j].lenv == T_IMMEDIATE) /* IMMEDIATE */
	    fprintf(ofile, "\t*(Uint*)&emax7[LANE].lmmi[0][%d][%d][emax7[LANE].lmmic] = 0x%08.8x|(%s<<2);\n", i, j, *(Uint*)&lmmi[i][j]&0xfffffffb, id[lmmx[i][j].forceh].name);
	  else /* VARIABLE */
	    fprintf(ofile, "\t*(Uint*)&emax7[LANE].lmmi[0][%d][%d][emax7[LANE].lmmic] = ((%s-1)<<13)|0x%04.4x|(%s<<2);\n", i, j, id[lmmx[i][j].lenh].name, *(Ushort*)&lmmi[i][j]&0x1ffb, id[lmmx[i][j].forceh].name);
	  if (lmmi[i][j].ofs) /* ptophが有効の場合,topが[CHIP]ならptopも[CHIP]でなければおかしい */
	    fprintf(ofile, "\temax7[LANE].lmmi[0][%d][%d][emax7[LANE].lmmic].ofs = (Uchar*)%s - (Uchar*)%s;\n", i, j, (char*)lmmi[i][j].top + lmmi[i][j].ofs, (char*)lmmi[i][j].top);
	  else /* ptophが無効 */
	    fprintf(ofile, "\temax7[LANE].lmmi[0][%d][%d][emax7[LANE].lmmic].ofs = 0;\n", i, j);
	  fprintf(ofile, "\temax7[LANE].lmmi[0][%d][%d][emax7[LANE].lmmic].top = %s;\n", i, j, (char*)lmmi[i][j].top);
	}
      }
      if (lmmi[i][j].v && !(lmmx[i][j].lenv == T_IMMEDIATE && lmmi[i][j].len==0x7ffff) && !lmmi[i][j].hcopy) {
        range_bitmap[j] |= (1LL<<i);
	if (current_mapdist && lmmi[i][j].rw && !lmmi[i][j].p) /* lmwとlmxの場合,emax7_check_lmmi_and_dma()のためにmarkが必要 */
	  range_bitmap[j] |= (1LL<<(i-current_mapdist));
      }
    }
  }
  /* lmmi_bitmap */
  for (j=0; j<EMAX_WIDTH; j++)
    fprintf(ofile, "\temax7[LANE].lmmi_bitmap[%d] = 0x%08.8x%08.8xLL;\n", j, (Uint)(lmmi_bitmap[j]>>32), (Uint)lmmi_bitmap[j]);

mode_drain_dirty_lmm:

  if (mode == 0 && !current_lmmwb) /* for transaction */
    fprintf(ofile, "\temax7_pre_with_keep_cache();\n");
  else /* for normal emax */
    fprintf(ofile, "\temax7_pre_with_drain_cache();\n");

  fprintf(ofile, "\tget_nanosec(LANE, NANOS_ARM);\n");/*=================================*/

  /* ■■■dma(drain) */
  /* mode=0,phase=1     (drain) :lmra=lmmio.top     */
  /* mode=1,phase=1     (drain) :lmra=lmmic.top     */
  if (mode == 0) { /* array */
    fprintf(ofile, "\tif (emax7[LANE].last_conf == emax7_conf_%s) {\n", id[current_prefix].name);
    fprintf(ofile, "\t  emax7[LANE].status = STATUS_DRAIN;\n");
    for (j=0; j<EMAX_WIDTH; j++) {
      for (i=0; i<EMAX_DEPTH; i++) {
	if (lmmi_bitmap[j] & (1LL<<i) && lmmi[i+current_mapdist][j].rw && !lmmi[i+current_mapdist][j].p) {
	  /* 同一conf使用の最後にemax5_drain_dirty_lmm()する前提なので,新lmmi_bitmapと,更新前RANGEの組合せでもOK */
	  /* 但し,SCON後はconf.lmm_modeがずれるので,drainの時にtag_matchしない.drainはlmmi更新後&SCON前 */
	  if (lmmi[i+current_mapdist][j].cidx) { /* NCHIP間でlmm_topが異なる場合 */
	    for (c=0; c<current_nchip; c++)
	      fprintf(ofile, "\t  emax7_check_lmmi_and_dma(LANE, 0, 1, %d, %d, %d, %d);/*drain*/\n", current_mapdist, c, i, j); /* phase=1 drain */
	  }
	  else /* NCHIP間でlmm_topが共通の場合 */
	    fprintf(ofile, "\t  emax7_check_lmmi_and_dma(LANE, 0, 1, %d, 0, %d, %d);/*drain*/\n", current_mapdist, i, j); /* phase=1 drain */
	}
      }
    }
    fprintf(ofile, "\t}\n");
  }
  else { /* drain */
    fprintf(ofile, "\t{\n");
    fprintf(ofile, "\t  struct reg_ctrl *reg_ctrl = emax7[LANE].reg_ctrl;\n");
    fprintf(ofile, "\t  Uint   lmmic              = emax7[LANE].lmmic;\n");
    fprintf(ofile, "\t  Uint   mapdist            = emax7[LANE].mapdist;\n");
    fprintf(ofile, "\t  int    c,i,j;\n");
    fprintf(ofile, "\t  emax7[LANE].status = STATUS_DRAIN;\n");
    fprintf(ofile, "\t  for (j=0; j<%d; j++) {\n", EMAX_WIDTH);
    fprintf(ofile, "\t    for (i=0; i<%d; i++) {\n", EMAX_DEPTH);
    fprintf(ofile, "\t      if (emax7[LANE].lmmi_bitmap[j] & (1LL<<i) && emax7[LANE].lmmi[0][i][j][lmmic].rw) {\n");
    fprintf(ofile, "\t        for (c=0; c<%d; c++) {\n", current_nchip);
    fprintf(ofile, "\t          if (emax7[LANE].lmmi[0][i][j][lmmic].ofs)\n");
    fprintf(ofile, "\t            *(Ull*)&(reg_ctrl->i[c].addr[i][j].top) = ((Ull)(emax7[LANE].lmmi[c][i][j][lmmic].top+emax7[LANE].lmmi[c][i][j][lmmic].len*sizeof(Uint)+(sizeof(Uint)-1))<<32) | (Ull)(Uint)emax7[LANE].lmmi[c][i][j][lmmic].top;\n");
    fprintf(ofile, "\t          emax7_check_lmmi_and_dma(LANE, 1, 1, mapdist, c, i, j);/*drain*/\n"); /* phase=1 drain */
    fprintf(ofile, "\t        }\n");
    fprintf(ofile, "\t      }\n");
    fprintf(ofile, "\t    }\n");
    fprintf(ofile, "\t  }\n");
    fprintf(ofile, "\t}\n");
  }

  fprintf(ofile, "\tget_nanosec(LANE, NANOS_DRAIN);\n");/*=================================*/

  if (mode == 1) { /* emit drain */
    fprintf(ofile, "#endif\n");
    return (0);
  }

  /* conf */
  fprintf(ofile, "\tif (emax7[LANE].last_conf != emax7_conf_%s) {\n", id[current_prefix].name);
  fprintf(ofile, "\t  Dll *dst, *src;\n");
  fprintf(ofile, "\t  int i,j,hard_stat,hard_depth;\n");
  fprintf(ofile, "\t  emax7[LANE].status = STATUS_CONF;\n");
#if 1
  fprintf(ofile, "\t  hard_stat  = ((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].stat>>8 & 0xffffff0f;\n");
  fprintf(ofile, "\t  hard_depth = (hard_stat==3)?64:(hard_stat==2)?32:(hard_stat==1)?16:8;\n");
  fprintf(ofile, "\t  if (hard_depth != %d) {\n", EMAX_DEPTH);
  fprintf(ofile, "\t    printf(\"EMAX_DEPTH mismatch in emax7_conf_%s. hard_depth=%%d code_depth=%d\\n\", hard_depth);\n", id[current_prefix].name, EMAX_DEPTH);
  fprintf(ofile, "\t    exit(1);\n");
  fprintf(ofile, "\t  }\n");
#endif
  fprintf(ofile, "\t  emax7[LANE].last_conf = emax7_conf_%s;\n", id[current_prefix].name);
  fprintf(ofile, "\t  emax7[LANE].lastdist = 0;\n");
  fprintf(ofile, "\t  dst = (Dll*)(((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].conf);\n");
  fprintf(ofile, "\t  src = (Dll*)emax7_conf_%s;\n", id[current_prefix].name);
  fprintf(ofile, "\t  for (i=0; i<(sizeof(struct conf)*%d*%d)/sizeof(Dll); i++)\n", EMAX_DEPTH, EMAX_WIDTH);
  fprintf(ofile, "\t    *dst++ = *src++;\n");

  fprintf(ofile, "\t  for (i=0; i<%d; i++) {\n", EMAX_DEPTH);
  fprintf(ofile, "\t    for (j=0; j<%d; j++)\n", EMAX_WIDTH);
  fprintf(ofile, "\t      emax7[LANE].lmmi[0][i][j][emax7[LANE].lmmio].v = 0;\n");
  fprintf(ofile, "\t  }\n");
    
  fprintf(ofile, "\t  while (((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].stat & 0xffff00f0); //LMRING_BUSY \n");
  fprintf(ofile, "\t}\n");

  /* scon */
  if (current_mapdist > 0 && lmmi_first_loc >= 0) {
    int i = lmmi_first_loc/EMAX_WIDTH;
    int j = lmmi_first_loc%EMAX_WIDTH;
    fprintf(ofile, "\telse if (emax7[LANE].lmmi[0][%d][%d][emax7[LANE].lmmic].top != emax7[LANE].lmmi[0][%d][%d][emax7[LANE].lmmio].top) {\n", i, j, i, j);
    fprintf(ofile, "\t  emax7[LANE].status = STATUS_SCON;\n");
    fprintf(ofile, "\t  emax7[LANE].lastdist = emax7[LANE].mapdist;\n");
    fprintf(ofile, "\t  ((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].cmd = 2LL; // SCON\n");
    fprintf(ofile, "\t  while (((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].stat & 0xffff000f); //EXRING_BUSY\n");
    fprintf(ofile, "\t}\n");
    fprintf(ofile, "\telse {\n");
    fprintf(ofile, "\t  emax7[LANE].lastdist = 0;\n");
    fprintf(ofile, "\t}\n");
  }

  fprintf(ofile, "\tget_nanosec(LANE, NANOS_CONF);\n");/*=================================*/

  fprintf(ofile, "\t//pthread_mutex_lock(&axi_dma_mutex);\n");/*=======MUTEX/VP180-step4800 workaround=======*/

  /* breg */
  fprintf(ofile, "\temax7[LANE].status = STATUS_REGV;\n");
  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      for (k=0; k<UNIT_WIDTH; k++) {
	if (regv[i][j].br[k].v) {
	  if (id[regv[i][j].br[k].h].chip) { /* CHIP */
	    for (c=0; c<current_nchip; c++)
	      fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[%d].breg[%d][%d].br[%d] = %d;\n", c, i, j, k, c);
	  }
	  else if (id[regv[i][j].br[k].h].cidx) { /* xxx[CHIP] */
	    for (c=0; c<current_nchip; c++)
	      fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[%d].breg[%d][%d].br[%d] = %s[%d];\n", c, i, j, k, id[regv[i][j].br[k].h].name, c);
	  }
	  else if (regv[i][j].br[k].s < 0)
	    fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].breg[%d][%d].br[%d] = %s;\n", i, j, k, id[regv[i][j].br[k].h].name);
	  else
	    fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].breg[%d][%d].br[%d] = %s[%d];\n", i, j, k, id[regv[i][j].br[k].h].name, regv[i][j].br[k].s);
	}
      }
    }
  }

  int statistics_br_tot = 0;
  int statistics_br_min = 99;
  int statistics_br_max = 0;
  int statistics_br_ave = 0;
  for (i=0; i<EMAX_DEPTH; i++) {
    int statistics_br_row = 0;
    for (j=0; j<EMAX_WIDTH; j++) {
      for (k=0; k<UNIT_WIDTH; k++) {
	if (bus[i][j].br[k].v)
	  statistics_br_row++;
      }
    }
    if (i < last_row || i == EMAX_DEPTH-1) {
      statistics_br_tot += statistics_br_row;
      if (statistics_br_row < statistics_br_min)
	statistics_br_min = statistics_br_row;
      if (statistics_br_row > statistics_br_max)
	statistics_br_max = statistics_br_row;
    }
  }
  statistics_br_ave = statistics_br_tot/(last_row<EMAX_DEPTH-1?last_row+2:last_row+1);

  int statistics_ap_tot = 0; /* emergency AR pass through */
  int statistics_ap_max = 0;
  for (i=0; i<EMAX_DEPTH; i++) {
    int statistics_ap_row = 0;
    for (j=0; j<EMAX_WIDTH; j++) {
      if (dec[i][j].dexu.op1  == OP_NOP && dec[i][j].dexu.op2  == OP_OR && dec[i][j].dexu.op3 == OP_NOP
	&&dec[i][j].dexu.ex2v == T_NONE && dec[i][j].dexu.ex3v == T_NONE
        &&dec[i][j].dexu.e2iv == T_IMMEDIATE && dec[i][j].dexu.e2ih == hash_reg_immediate(0LL)
        &&dec[i][j].dexu.e2is == 0 && dec[i][j].dexu.e2is == 0)
	statistics_ap_row++;
    }
    if (i <= last_row) {
      statistics_ap_tot += statistics_ap_row;
      if (statistics_ap_row > statistics_ap_max)
	statistics_ap_max = statistics_ap_row;
    }
  }

  /* eag */
#if 1
  fprintf(ofile, "\t{ union { Dll dll; struct {Uint ea0b; Uint ea0o; Uint ea1b; Uint ea1o;} ea4;} ea128;\n");
  for (c=0; c<current_nchip; c++) {
    for (i=0; i<EMAX_DEPTH; i++) {
      for (j=0; j<EMAX_WIDTH; j++) {
	if (c == 0) {
	  if (regv[i][j].ea0b_v) {
	    if (id[regv[i][j].ea0b_h].cidx)
	      fprintf(ofile, "\tea128.ea4.ea0b = (Ull)%s[%d];\n", id[regv[i][j].ea0b_h].name, c);
	    else if (regv[i][j].ea0b_s < 0)
	      fprintf(ofile, "\tea128.ea4.ea0b = (Ull)%s;\n", id[regv[i][j].ea0b_h].name);
	    else
	      fprintf(ofile, "\tea128.ea4.ea0b = (Ull)%s[%d];\n", id[regv[i][j].ea0b_h].name, regv[i][j].ea0b_s);
	  }
	  if (regv[i][j].ea0o_v) {
	    if (id[regv[i][j].ea0o_h].cidx)
	      fprintf(ofile, "\tea128.ea4.ea0o = (Ull)%s[%d];\n", id[regv[i][j].ea0o_h].name, c);
	    else if (regv[i][j].ea0o_s < 0)
	      fprintf(ofile, "\tea128.ea4.ea0o = (Ull)%s;\n", id[regv[i][j].ea0o_h].name);
	    else
	      fprintf(ofile, "\tea128.ea4.ea0o = (Ull)%s[%d];\n", id[regv[i][j].ea0o_h].name, regv[i][j].ea0o_s);
	  }
	  if (regv[i][j].ea1b_v) {
	    if (id[regv[i][j].ea1b_h].cidx)
	      fprintf(ofile, "\tea128.ea4.ea1b = (Ull)%s[%d];\n", id[regv[i][j].ea1b_h].name, c);
	    else if (regv[i][j].ea1b_s < 0)
	      fprintf(ofile, "\tea128.ea4.ea1b = (Ull)%s;\n", id[regv[i][j].ea1b_h].name);
	    else
	      fprintf(ofile, "\tea128.ea4.ea1b = (Ull)%s[%d];\n", id[regv[i][j].ea1b_h].name, regv[i][j].ea1b_s);
	  }
	  if (regv[i][j].ea1o_v) {
	    if (id[regv[i][j].ea1o_h].cidx)
	      fprintf(ofile, "\tea128.ea4.ea1o = (Ull)%s[%d];\n", id[regv[i][j].ea1o_h].name, c);
	    else if (regv[i][j].ea1o_s < 0)
	      fprintf(ofile, "\tea128.ea4.ea1o = (Ull)%s;\n", id[regv[i][j].ea1o_h].name);
	    else
	      fprintf(ofile, "\tea128.ea4.ea1o = (Ull)%s[%d];\n", id[regv[i][j].ea1o_h].name, regv[i][j].ea1o_s);
	  }
	  if (regv[i][j].ea0b_v || regv[i][j].ea0o_v || regv[i][j].ea1b_v || regv[i][j].ea1o_v)
	    fprintf(ofile, "\t*(Dll*)&(((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[%d].addr[%d][%d]) = ea128.dll;\n", c, i, j);
	}
	else {
	  if (regv[i][j].ea0b_v) {
	    if (id[regv[i][j].ea0b_h].cidx)
	      fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[%d].addr[%d][%d].ea0b = (Ull)%s[%d];\n", c, i, j, id[regv[i][j].ea0b_h].name, c);
	  }
	  if (regv[i][j].ea0o_v) {
	    if (id[regv[i][j].ea0o_h].cidx)
	      fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[%d].addr[%d][%d].ea0o = (Ull)%s[%d];\n", c, i, j, id[regv[i][j].ea0o_h].name, c);
	  }
	  if (regv[i][j].ea1b_v) {
	    if (id[regv[i][j].ea1b_h].cidx)
	      fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[%d].addr[%d][%d].ea1b = (Ull)%s[%d];\n", c, i, j, id[regv[i][j].ea1b_h].name, c);
	  }
	  if (regv[i][j].ea1o_v) {
	    if (id[regv[i][j].ea1o_h].cidx)
	      fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[%d].addr[%d][%d].ea1o = (Ull)%s[%d];\n", c, i, j, id[regv[i][j].ea1o_h].name, c);
	  }
	}
      }
    }
  }
  fprintf(ofile, "\t}\n");
#else
  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      if (regv[i][j].ea0b_v) {
	if (id[regv[i][j].ea0b_h].cidx) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[%d].addr[%d][%d].ea0b = (Ull)%s[%d];\n", c, i, j, id[regv[i][j].ea0b_h].name, c);
	}
	else if (regv[i][j].ea0b_s < 0)
	  fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].addr[%d][%d].ea0b = (Ull)%s;\n", i, j, id[regv[i][j].ea0b_h].name);
	else
	  fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].addr[%d][%d].ea0b = (Ull)%s[%d];\n", i, j, id[regv[i][j].ea0b_h].name, regv[i][j].ea0b_s);
      }
      if (regv[i][j].ea0o_v) {
	if (id[regv[i][j].ea0o_h].cidx) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[%d].addr[%d][%d].ea0o = (Ull)%s[%d];\n", c, i, j, id[regv[i][j].ea0o_h].name, c);
	}
	else if (regv[i][j].ea0o_s < 0)
	  fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].addr[%d][%d].ea0o = (Ull)%s;\n", i, j, id[regv[i][j].ea0o_h].name);
	else
	  fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].addr[%d][%d].ea0o = (Ull)%s[%d];\n", i, j, id[regv[i][j].ea0o_h].name, regv[i][j].ea0o_s);
      }
      if (regv[i][j].ea1b_v) {
	if (id[regv[i][j].ea1b_h].cidx) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[%d].addr[%d][%d].ea1b = (Ull)%s[%d];\n", c, i, j, id[regv[i][j].ea1b_h].name, c);
	}
	else if (regv[i][j].ea1b_s < 0)
	  fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].addr[%d][%d].ea1b = (Ull)%s;\n", i, j, id[regv[i][j].ea1b_h].name);
	else
	  fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].addr[%d][%d].ea1b = (Ull)%s[%d];\n", i, j, id[regv[i][j].ea1b_h].name, regv[i][j].ea1b_s);
      }
      if (regv[i][j].ea1o_v) {
	if (id[regv[i][j].ea1o_h].cidx) {
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[%d].addr[%d][%d].ea1o = (Ull)%s[%d];\n", c, i, j, id[regv[i][j].ea1o_h].name, c);
	}
	else if (regv[i][j].ea1o_s < 0)
	  fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].addr[%d][%d].ea1o = (Ull)%s;\n", i, j, id[regv[i][j].ea1o_h].name);
	else
	  fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].addr[%d][%d].ea1o = (Ull)%s[%d];\n", i, j, id[regv[i][j].ea1o_h].name, regv[i][j].ea1o_s);
      }
    }
  }
#endif

  int statistics_ea_tot = 0;
  int statistics_ea_min = 99;
  int statistics_ea_max = 0;
  int statistics_ea_ave = 0;
  for (i=0; i<EMAX_DEPTH; i++) {
    int statistics_ea_row = 0;
    for (j=0; j<EMAX_WIDTH; j++) {
      if (bus[i][j].ea0brv)
	statistics_ea_row++;
      if (bus[i][j].ea0orv)
	statistics_ea_row++;
      if (bus[i][j].ea1brv)
	statistics_ea_row++;
      if (bus[i][j].ea1orv)
	statistics_ea_row++;
    }
    if (i <= last_row) {
      statistics_ea_tot += statistics_ea_row;
      if (statistics_ea_row < statistics_ea_min)
	statistics_ea_min = statistics_ea_row;
      if (statistics_ea_row > statistics_ea_max)
	statistics_ea_max = statistics_ea_row;
    }
  }
  statistics_ea_ave = statistics_ea_tot/(last_row+1);

  fprintf(ofile, "\t//pthread_mutex_unlock(&axi_dma_mutex);\n");/*=======MUTEX/VP180-step4800 workaround=======*/

  fprintf(ofile, "\tget_nanosec(LANE, NANOS_REGV);\n");/*=================================*/

  /* EXEC前●lmmi指示ルール (copy from conv-c2d/emac5.c)                                                            lmmi-loc  v  top  blk  len  rw  f  p */
  /* LD with force-read=0 and ptop==NULL generates current(lmr) and reuse LMM. same as lmr in EMAX4                     curr  1  top  blk  len   0  0  0 *//* ★EXEC前にcheckし,なければDMA */
  /* LD with force-read=1 and ptop==NULL generates current(lmf) and !reuse LMM. same as lmf in EMAX4                    curr  1  top  blk  len   0  1  0 *//* ★EXEC前に,必ずDMA */
  /* LD with force-read=0 and ptop!=NULL generates current(lmr) and next(lmp). mapdist!=0                               curr  1  top  blk  len   0  0  0 *//* ★EXEC前にcheckし,なければDMA */
  /*                                                                                                                  c+dist  1 ptop  blk  len   0  0  1 *//* EXEC中に,必ずDMA(先頭はlmmi.top) */
  /* LD with force-read=0 and ptop!=NULL generates current(lmr) and next(lmp). mapdist==0                  ofs=ptop-top curr  1  top  blk  len   0  0  1 *//* ★EXEC前にcheckし,なければDMA */
  /*                                                                                                               p=1の場合,pref-addrは常にlmmi.top+ofs *//* EXEC中に,必ずDMA(先頭はlmmi.top+ofs) */
  /*******************************************************************************************************************************************************/
  /* ST with force-read=0 and ptop==NULL generates current(lmw) and reuse+wback LMM. same as lmw in EMAX4               curr  1  top  blk  len   1  0  0 *//* ★EXEC前にcheckし,なければDMA */
  /* ST with force-read=1 and ptop==NULL generates current(lmx) and !reuse+wback LMM. same as lmx in EMAX4              curr  1  top  blk  len   1  1  0 *//* ★EXEC前に,必ずDMA */
  /* ST with force-read=0 and ptop!=NULL generates current(lmw) and prev(lmd). mapdist!=0                               curr  1  top  blk  len   1  0  0 *//* ★EXEC前にcheckし,なければDMA */
  /*                                                                                                                  c-dist  1 ptop  blk  len   1  0  1 *//* EXEC中に,必ずDMA(先頭はlmmi.top) */
  /* ST with force-read=0 and ptop!=NULL generates current(lmw) and prev(lmd). mapdist==0                  ofs=ptop-top curr  1  top  blk  len   1  0  1 *//* ★EXEC前にcheckし,なければDMA */
  /*                                                                                                              p=1の場合,drain-addrは常にlmmi.top+ofs *//* EXEC中に,必ずDMA(先頭はlmmi.top+ofs) */

  fprintf(ofile, "\t//pthread_mutex_lock(&axi_dma_mutex);\n");/*=======MUTEX/VP180-step4800 workaround=======*/

  fprintf(ofile, "\temax7[LANE].status = STATUS_RANGE;\n");
  fprintf(ofile, "\t{struct reg_ctrl *reg_ctrl = emax7[LANE].reg_ctrl;\n");
  fprintf(ofile, "\t Uint            lmmic     = emax7[LANE].lmmic;\n");
  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      if (range_bitmap[j] & (1LL<<i)) {
	int source_i;
	if (lmmi_bitmap[j] & (1LL<<i))
	  source_i = i;
	else
	  source_i = range_link[i][j];
	if (lmmi[i][j].cidx) { /* NCHIP間でlmm_topが異なる場合 */
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "\t*(Ull*)&(reg_ctrl->i[%d].addr[%d][%d].top) = ((Ull)(emax7[LANE].lmmi[%d][%d][%d][lmmic].top+emax7[LANE].lmmi[%d][%d][%d][lmmic].len*sizeof(Uint)+(sizeof(Uint)-1))<<32) | (Ull)(Uint)emax7[LANE].lmmi[%d][%d][%d][lmmic].top;\n", c, i, j, c, source_i, j, c, source_i, j, c, source_i, j);
	}
	else /* NCHIP間でlmm_topが共通の場合 */
	  fprintf(ofile, "\t*(Ull*)&(reg_ctrl->i[0].addr[%d][%d].top) = ((Ull)(emax7[LANE].lmmi[0][%d][%d][lmmic].top+emax7[LANE].lmmi[0][%d][%d][lmmic].len*sizeof(Uint)+(sizeof(Uint)-1))<<32) | (Ull)(Uint)emax7[LANE].lmmi[0][%d][%d][lmmic].top;\n", i, j, source_i, j, source_i, j, source_i, j);
      }
    }
  }
  fprintf(ofile, "\t}\n");

  fprintf(ofile, "\t//pthread_mutex_unlock(&axi_dma_mutex);\n");/*=======MUTEX/VP180-step4800 workaround=======*/

  fprintf(ofile, "\tget_nanosec(LANE, NANOS_RANGE);\n");/*=================================*/

  /* ■■■dma(load) */
  /*        phase=2     (load)  :lmwa=lmmic.top     */
  fprintf(ofile, "\temax7[LANE].status = STATUS_LOAD;\n");
  for (j=0; j<EMAX_WIDTH; j++) {
    for (i=0; i<EMAX_DEPTH; i++) {
      if (lmmi_bitmap[j] & (1LL<<i) && ((!lmmi[i][j].rw&&(!lmmi[i][j].p||lmmi[i][j].ofs))||(lmmi[i][j].rw&&lmmi[i][j].f))) { /* lmr,lmf,lmx */
	if (lmmi[i][j].cidx) { /* NCHIP間でlmm_topが異なる場合 */
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "\temax7_check_lmmi_and_dma(LANE, %d, 2, emax7[LANE].lastdist, %d, %d, %d);/*load*/\n", mode, c, i, j); /* phase=2 load */
	}
	else /* NCHIP間でlmm_topが共通の場合 */
	  fprintf(ofile, "\temax7_check_lmmi_and_dma(LANE, %d, 2, emax7[LANE].lastdist, 0, %d, %d);/*load*/\n", mode, i, j); /* phase=2 load */
      }
    }
  }

  fprintf(ofile, "\tget_nanosec(LANE, NANOS_LOAD);\n");/*=================================*/

  /* confは1wayあたり,32B*16行=0.5KB,32B*64行=2KB(最大). 4way分連続にするには,16行で2KB,64行で8KBアラインが必要. ZYNQのpage-sizeが8KB以上であればOK */

  /* EXEC中●lmmi指示ルール (copy from conv-c2d/emac5.c)                                                            lmmi-loc  v  top  blk  len  rw  f  p */
  /* LD with force-read=0 and ptop==NULL generates current(lmr) and reuse LMM. same as lmr in EMAX4                     curr  1  top  blk  len   0  0  0 *//* EXEC前にcheckし,なければDMA */
  /* LD with force-read=1 and ptop==NULL generates current(lmf) and !reuse LMM. same as lmf in EMAX4                    curr  1  top  blk  len   0  1  0 *//* EXEC前に,必ずDMA */
  /* LD with force-read=0 and ptop!=NULL generates current(lmr) and next(lmp). mapdist!=0                               curr  1  top  blk  len   0  0  0 *//* EXEC前にcheckし,なければDMA */
  /*                                                                                                                  c+dist  1 ptop  blk  len   0  0  1 *//* ★EXEC中に,必ずDMA(先頭はlmmi.top) */
  /* LD with force-read=0 and ptop!=NULL generates current(lmr) and next(lmp). mapdist==0                  ofs=ptop-top curr  1  top  blk  len   0  0  1 *//* EXEC前にcheckし,なければDMA */
  /*                                                                                                               p=1の場合,pref-addrは常にlmmi.top+ofs *//* ★EXEC中に,必ずDMA(先頭はlmmi.top+ofs) */
  /*******************************************************************************************************************************************************/
  /* ST with force-read=0 and ptop==NULL generates current(lmw) and reuse+wback LMM. same as lmw in EMAX4               curr  1  top  blk  len   1  0  0 *//* EXEC前にcheckし,なければDMA */
  /* ST with force-read=1 and ptop==NULL generates current(lmx) and !reuse+wback LMM. same as lmx in EMAX4              curr  1  top  blk  len   1  1  0 *//* EXEC前に,必ずDMA */
  /* ST with force-read=0 and ptop!=NULL generates current(lmw) and prev(lmd). mapdist!=0                               curr  1  top  blk  len   1  0  0 *//* EXEC前にcheckし,なければDMA */
  /*                                                                                                                  c-dist  1 ptop  blk  len   1  0  1 *//* ★EXEC中に,必ずDMA(先頭はlmmi.top) */
  /* ST with force-read=0 and ptop!=NULL generates current(lmw) and prev(lmd). mapdist==0                  ofs=ptop-top curr  1  top  blk  len   1  0  1 *//* EXEC前にcheckし,なければDMA */
  /*                                                                                                              p=1の場合,drain-addrは常にlmmi.top+ofs *//* ★EXEC中に,必ずDMA(先頭はlmmi.top+ofs) */
  fprintf(ofile, "\t{struct reg_ctrl *reg_ctrl = emax7[LANE].reg_ctrl;\n");
  fprintf(ofile, "\t Uint            lmmic     = emax7[LANE].lmmic;\n");
  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      if (range_bitmap[j] & (1LL<<i) && lmmi[i][j].ofs) {
	int source_i;
	if (lmmi_bitmap[j] & (1LL<<i))
	  source_i = i;
	else
	  source_i = range_link[i][j];
	if (lmmi[i][j].cidx) { /* NCHIP間でlmm_topが異なる場合 */
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "\t*(Ull*)&(reg_ctrl->i[%d].addr[%d][%d].top) = ((Ull)(emax7[LANE].lmmi[%d][%d][%d][lmmic].top+emax7[LANE].lmmi[%d][%d][%d][lmmic].ofs+emax7[LANE].lmmi[%d][%d][%d][lmmic].len*sizeof(Uint)+(sizeof(Uint)-1))<<32) | (Ull)(Uint)(emax7[LANE].lmmi[%d][%d][%d][lmmic].top+emax7[LANE].lmmi[%d][%d][%d][lmmic].ofs);\n", c, i, j, c, source_i, j, c, source_i, j, c, source_i, j, c, source_i, j, c, source_i, j);
	}
	else /* NCHIP間でlmm_topが共通の場合 */
	  fprintf(ofile, "\t*(Ull*)&(reg_ctrl->i[0].addr[%d][%d].top) = ((Ull)(emax7[LANE].lmmi[0][%d][%d][lmmic].top+emax7[LANE].lmmi[0][%d][%d][lmmic].ofs+emax7[LANE].lmmi[0][%d][%d][lmmic].len*sizeof(Uint)+(sizeof(Uint)-1))<<32) | (Ull)(Uint)(emax7[LANE].lmmi[0][%d][%d][lmmic].top+emax7[LANE].lmmi[0][%d][%d][lmmic].ofs);\n", i, j, source_i, j, source_i, j, source_i, j, source_i, j, source_i, j);
      }
    }
  }
  fprintf(ofile, "\t}\n");

  /* exec */
  fprintf(ofile, "\t((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].cmd = 3LL; // EXEC\n");

  /* ■■■dma(pdrain) */
  /* ■■■dma(pload) */
  /*        phase=3,rw=1(pdrain):lmra=lmmic.top+ofs */
  /*        phase=3,rw=0(pload) :lmwa=lmmic.top+ofs */
  for (j=0; j<EMAX_WIDTH; j++) {
    for (i=0; i<EMAX_DEPTH; i++) {
      if (lmmi_bitmap[j] & (1LL<<i) && lmmi[i][j].p) {
	if (lmmi[i][j].cidx) { /* NCHIP間でlmm_topが異なる場合 */
	  for (c=0; c<current_nchip; c++)
	    fprintf(ofile, "\temax7_check_lmmi_and_dma(LANE, %d, 3, %d, %d, %d, %d);/*pdrain,pload*/\n", mode, current_mapdist, c, i, j); /* phase=3 exec */
	}
	else /* NCHIP間でlmm_topが共通の場合 */
	  fprintf(ofile, "\temax7_check_lmmi_and_dma(LANE, %d, 3, %d, 0, %d, %d);/*pdrain,pload*/\n", mode, current_mapdist, i, j); /* phase=3 exec */
      }
    }
  }

  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      if (lmmi_bitmap[j] & (1LL<<i) && lmmi[i][j].rw && !lmmi[i][j].p) /* ●6 lmw/lmx */
	fprintf(ofile, "\temax7[LANE].lmmd[%d][%d] = 0xff>>%d;\n", i, j, sizeof(Uchar)*8-current_nchip); /* set dirty */
    }
  }

#if 1
  fprintf(ofile, "\t{ int stat; static int step;\n");
  fprintf(ofile, "\tdo {\n");
  fprintf(ofile, "\t  if      (step <   0) step = 0;\n");              
  fprintf(ofile, "\t  else if (step > 100) step = 100;\n");              
  fprintf(ofile, "\t  if (step>1) sleep_nanosec(step*100);\n");              
  fprintf(ofile, "\t  stat = ((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].stat & 0xffff00ff;\n");
  fprintf(ofile, "\t  if (stat) step++; else step--;\n");
  fprintf(ofile, "\t} while (stat); }//LMRING_BUSY|EXRING_BUSY\n");
#elif 0
  fprintf(ofile, "\twhile (((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].stat & 0xffff00ff) emax7_sigwait(LANE); //LMRING_BUSY|EXRING_BUSY\n");
#else
  fprintf(ofile, "\t{ int stat;\n");
  fprintf(ofile, "\tdo {             //LMRING_BUSY|EXRING_BUSY\n");                                /*=======MUTEX/VP180-step4800 workaround=======*/
  fprintf(ofile, "\t  sleep_nanosec(100);\n");                                                     /*=======MUTEX/VP180-step4800 workaround=======*/
  fprintf(ofile, "\t  pthread_mutex_lock(&axi_dma_mutex);\n");                                     /*=======MUTEX/VP180-step4800 workaround=======*/
  fprintf(ofile, "\t  stat = ((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].stat & 0xffff00ff;\n");/*=======MUTEX/VP180-step4800 workaround=======*/
  fprintf(ofile, "\t  pthread_mutex_unlock(&axi_dma_mutex);\n");                                   /*=======MUTEX/VP180-step4800 workaround=======*/
  fprintf(ofile, "\t} while (stat); }//LMRING_BUSY|EXRING_BUSY\n");                                /*=======MUTEX/VP180-step4800 workaround=======*/
#endif

  fprintf(ofile, "\tget_nanosec(LANE, NANOS_EXEC);\n");/*=================================*/

  /* term */
  fprintf(ofile, "asm volatile(\"b emax7_conf_end_%s\\n\"\n\".align 5\\n\"\n\".global emax7_conf_%s\\n\"\n\"emax7_conf_%s:\\n\"\n", id[current_prefix].name, id[current_prefix].name, id[current_prefix].name, id[current_prefix].name);
  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      fprintf(ofile, "\"\t.word\t0x%08.8x, 0x%08.8x\\n\"\n", *(Ull*)&conf[i][j].cdw0, (Uint)((*(Ull*)&conf[i][j].cdw0)>>32));
      fprintf(ofile, "\"\t.word\t0x%08.8x, 0x%08.8x\\n\"\n", *(Ull*)&conf[i][j].cdw1, (Uint)((*(Ull*)&conf[i][j].cdw1)>>32));
      fprintf(ofile, "\"\t.word\t0x%08.8x, 0x%08.8x\\n\"\n", *(Ull*)&conf[i][j].cdw2, (Uint)((*(Ull*)&conf[i][j].cdw2)>>32));
      fprintf(ofile, "\"\t.word\t0x%08.8x, 0x%08.8x\\n\"\n", *(Ull*)&conf[i][j].cdw3, (Uint)((*(Ull*)&conf[i][j].cdw3)>>32));
    }
  }
  fprintf(ofile, "\".global emax7_conf_end_%s\\n\"\n\"emax7_conf_end_%s:\\n\"\n", id[current_prefix].name, id[current_prefix].name);
  fprintf(ofile, ");\n");
  fprintf(ofile, "#endif\n");
  /**********************************************************************************************************/
  /* Step 9 ... emit tgif                                                                                   */
  /**********************************************************************************************************/
  strncpy(figfile = (char*)malloc(strlen(srcprog)+1+strlen(id[current_prefix].name)+strlen(FIGSUFX)+1), srcprog, strlen(srcprog)+1); /* xxx.x -> xxx-func.obj */
  for (i=0; i<strlen(srcprog); i++) {
    if (figfile[i] == '.' || figfile[i] == '\0' ) {
      strncpy(figfile+i, "-", 2);
      strncat(figfile, id[current_prefix].name, strlen(id[current_prefix].name)+1);
      strncat(figfile, FIGSUFX, strlen(FIGSUFX)+1);
      break;
    }
  }

  /* open fig-file */
  if ((ffile = fopen(figfile, "w")) == NULL) {
    fprintf(stderr, "can't open object:\"%s\"\n", figfile);
    exit(1);
  }

  /* generate fig-file */
  fprintf(ffile, "%%TGIF 4.1.45-QPL\n");
  fprintf(ffile, "state(0,37,100.000,0,0,1,16,1,9,1,1,0,0,1,0,1,0,'Ryumin-Light-EUC-H',0,80640,0,0,1,5,0,0,1,1,0,16,0,0,1,1,1,1,8100,9500,1,0,19000,0).\n");
  fprintf(ffile, "%%\n");
  fprintf(ffile, "unit(\"1 pixel/pixel\").\n");
  fprintf(ffile, "color_info(11,65535,0,[\n");
  fprintf(ffile, "  \"magenta\", 65535, 0, 65535, 65535, 0, 65535, 1,\n");
  fprintf(ffile, "  \"red\", 65535, 0, 0, 65535, 0, 0, 1,\n");
  fprintf(ffile, "  \"green\", 0, 65535, 0, 0, 65535, 0, 1,\n");
  fprintf(ffile, "  \"blue\", 0, 0, 65535, 0, 0, 65535, 1,\n");
  fprintf(ffile, "  \"yellow\", 65535, 65535, 0, 65535, 65535, 0, 1,\n");
  fprintf(ffile, "  \"pink\", 65535, 49344, 52171, 65535, 49344, 52171, 1,\n");
  fprintf(ffile, "  \"cyan\", 0, 65535, 65535, 0, 65535, 65535, 1,\n");
  fprintf(ffile, "  \"CadetBlue\", 24415, 40606, 41120, 24415, 40606, 41120, 1,\n");
  fprintf(ffile, "  \"white\", 65535, 65535, 65535, 65535, 65535, 65535, 1,\n");
  fprintf(ffile, "  \"black\", 0, 0, 0, 0, 0, 0, 1,\n");
  fprintf(ffile, "  \"DarkSlateGray\", 12079, 20303, 20303, 12079, 20303, 20303, 1\n");
  fprintf(ffile, "]).\n");
  fprintf(ffile, "script_frac(\"0.6\").\n");
  fprintf(ffile, "fg_bg_colors('black','white').\n");
  fprintf(ffile, "dont_reencode(\"FFDingbests:ZapfDingbats\").\n");
  fprintf(ffile, "objshadow_info('#c0c0c0',2,2).\n");
  fprintf(ffile, "page(1,\"\",1,'').\n");
  draw_text(100, 100, figfile, 5, 0);

  char statistics[1024];
  snprintf(statistics, 1024, "BR/row: max=%d min=%d ave=%d", statistics_br_max, statistics_br_min, statistics_br_ave);
  draw_text(100, 200, statistics, 4, 0);
  snprintf(statistics, 1024, "EA/row: max=%d min=%d ave=%d", statistics_ea_max, statistics_ea_min, statistics_ea_ave);
  draw_text(1200, 200, statistics, 4, 0);
  snprintf(statistics, 1024, "ARpass/row: max=%d", statistics_ap_max);
  draw_text(2300, 200, statistics, 4, 0);

  for (i=0; i<EMAX_DEPTH; i++) {
    for (j=0; j<EMAX_WIDTH; j++) {
      emit_tgif(i, j);
    }
  }

  /* close fig-file */
  fclose(ffile);
  free(figfile);

  return (0);
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

get_mop_type(int op)
{
  int retval;
  switch (op) {
  case OP_LDR:
  case OP_LDWR:
//case OP_LDHR:
  case OP_LDBR:
  case OP_LDRQ:
    retval = MTYPE_RLOAD; /* random_load on mop1->BR (+implicit AXI->mop0->MW) */
    break;
  case OP_STR:
  case OP_STWR:
//case OP_STHR:
  case OP_STBR:
  case OP_STRQ:
    retval = MTYPE_RSTORE; /* random_store on mop0->MW (+implicit mop1->MR->AXI) */
    break;
  case OP_LDDMQ:
    retval = MTYPE_DLOAD; /* direct_load on mop0->MW,MR->AXI->TR->BR (+implicit mop1->MR->AXI) */
    break;
  case OP_TR:
    retval = MTYPE_TRANS; /* transaction on mop0->MW,MR->AXI (+implicit mop1->MR->AXI) */
    break;
  }

  return (retval);
}

get_valid_row(int insn_type, int mid, int src_type, int src_hash, char *rdep)
{
  /* mid:0 ... refer at the top    of UNIT (                 EXE->store_addr) */
  /* mid:1 ... refer at the middle of UNIT (CEX->store_cond, EXE->store_data) */
  int i, j;

  switch (src_type) {
  case T_NONE:
  case T_IMMEDIATE:
    /* do nothing */
    break;
  case T_EXRNO: /* for cex in same unit */
  case T_ALRNO:
  case T_BDRNO:    /* for boundary reg */
  case T_VARIABLE: /* for any variable */
    switch (id[src_hash].itype) { /* 当該srcを生成する先行命令タイプ */
    case ITYPE_CEX:
      if (*rdep < id[src_hash].row) /* loc of new_insn < loc of dst_reg */
        *rdep = id[src_hash].row; /* same row ★CEX+CMOV/MOP */
      break;
    case ITYPE_WHILE:
    case ITYPE_FOR:
    case ITYPE_EX4:
    case ITYPE_EXE:
      if (insn_type == ITYPE_MO4 ||  insn_type == ITYPE_MOP) { /* 後続命令タイプ MO4 || MOP */
        if (mid==0) { /* EXE->store_addr */
          if (id[src_hash].row >= 0) { /* if defined as dst in EMAX */
            if (*rdep <= id[src_hash].row) /* loc of new_insn < loc of dst_reg */
              *rdep = id[src_hash].row+1; /* next row */
          }
        }
        else { /* EXE->store_data */
          if (*rdep < id[src_hash].row) /* loc of new_insn < loc of dst_reg */
            *rdep = id[src_hash].row; /* same row ★EXE+MOPの組合せは同一UNIT収容可能 */
        }
      }
      else { /* 後続命令タイプ ITYPE_WHILE || ITYPE_FOR || ITYPE_CEX || ITYPE_EX4 || ITYPE_EXE */
        if (id[src_hash].row >= 0) { /* if defined as dst in EMAX */
          if (*rdep <= id[src_hash].row) /* loc of new_insn < loc of dst_reg */
            *rdep = id[src_hash].row+1; /* next row */
        }
      }
      break;
    case ITYPE_MEX:
      if (insn_type == ITYPE_MO4 ||  insn_type == ITYPE_MOP) { /* 後続命令タイプ MO4 || MOP */
	if (*rdep < id[src_hash].row) /* loc of new_insn < loc of dst_reg */
	  *rdep = id[src_hash].row; /* same row ★EXE+MOPの組合せは同一UNIT収容可能 */
      }
      else {
        if (id[src_hash].row >= 0) { /* if defined as dst in EMAX */
          if (*rdep <= id[src_hash].row) /* loc of new_insn < loc of dst_reg */
            *rdep = id[src_hash].row+1; /* next row */
        }
      }
      break;
    case ITYPE_MO4:
    case ITYPE_MOP:
      if (id[src_hash].row >= 0) { /* if defined as dst in EMAX */
        if (*rdep <= id[src_hash].row) /* loc of new_insn < loc of dst_reg */
          *rdep = id[src_hash].row+1; /* next row */
      }
      break;
    }
    break;
  }
  /* printf("in get_valid: rdep=%d\n", *rdep); */
}

set_reg_path(int last_row, int last_col, int folding, int insn_type, int reg_type, int reg_loc, int src_type, int src_hash, int src_sidx)
     /* last_row:  配置先行位置 */
     /* last_col:  配置先列位置 */
     /* insn_type: 命令種別     ITYPE_WHILE, ITYPE_FOR, ITYPE_CEX, ITYPE_EX4, ITYPE_EXE, ITYPE_MEX, ITYPE_MO4, ITYPE_MOP */
     /* reg_type:  ITYPE_MEX/ITYPE_MO4/ITYPE_MOPの場合のみ有効: レジスタ種別 RTYPE_DATA, RTYPE_BASE, RTYPE_OFFS */
     /* reg_loc:   ITYPE_MEX/ITYPE_MO4/ITYPE_MOPの場合のみ有効: MO4/MOPのbase/offsの位置 0:mop0, 1:mop1 */
     /* src_type:  T_NONE, T_IMMEDIATE, T_EXRNO, T_ALRNO, T_BDRNO, T_INITNO, T_LOOPNO, T_VARIABLE */
     /* src_hash:  id[src_hash] */
     /* src_sidx:  subindex of VAR[]/AR[]/BR[] */
     /* 固定値設定用ARMコード生成,および,busmap設定 */
{
  int i, j, k, h, w;

  if (src_type == T_NONE) /* T_NONE */
    return 0;
  if (src_type == T_EXRNO) /* T_EXRNOは同一UNITにて消費するため，busmap不要 */
    return 0;
  if (id[src_hash].row < 0) { /* initialized by ARM (T_IMMEDIATE, T_VARIABLE) */
    if (insn_type == ITYPE_MEX || insn_type == ITYPE_MO4 || insn_type == ITYPE_MOP) { /* store4/store1 */
      switch (reg_type) {
      case RTYPE_DATA:
        printf("in %s: [%d][%d] ITYPE_MEX/ITYPE_MO4/ITYPE_MOP cannot store constant variable %s\n", id[current_prefix].name, last_row, last_col, id[src_hash].name);
        exit(1);
      case RTYPE_BASE:
        if (reg_loc==0) {
          if (bus[last_row][last_col].ea0brv) {
            printf("in %s: [%d][%d] ITYPE_MEX/ITYPE_MO4/ITYPE_MOP cannot find empty reg for RTYPE_BASE %s (may conflict with prefetch)\n", id[current_prefix].name, last_row, last_col, id[src_hash].name);
            exit(1);
          }
          bus[last_row][last_col].ea0brv = src_type;
          bus[last_row][last_col].ea0brh = src_hash;
          bus[last_row][last_col].ea0drv = src_type;
          bus[last_row][last_col].ea0drh = src_hash;
	  regv[last_row][last_col].ea0b_v = src_type;
	  regv[last_row][last_col].ea0b_h = src_hash;
	  regv[last_row][last_col].ea0b_s = src_sidx;
        }
        else {
          if (bus[last_row][last_col].ea1brv) {
            printf("in %s: [%d][%d] ITYPE_MEX/ITYPE_MO4/ITYPE_MOP cannot find empty reg for RTYPE_BASE %s (may conflict with drain)\n", id[current_prefix].name, last_row, last_col, id[src_hash].name);
            exit(1);
          }
          bus[last_row][last_col].ea1brv = src_type;
          bus[last_row][last_col].ea1brh = src_hash;
          bus[last_row][last_col].ea1drv = src_type;
          bus[last_row][last_col].ea1drh = src_hash;
	  regv[last_row][last_col].ea1b_v = src_type;
	  regv[last_row][last_col].ea1b_h = src_hash;
	  regv[last_row][last_col].ea1b_s = src_sidx;
        }
        break;
      case RTYPE_OFFS: /* prefetch/drainの設定にも使用（値は,バイトアドレスで64bit*4が1単位,即ち32. N+1回参照し,alignされていない場合でもFSMが調整）*/
        if (reg_loc==0) {
          if (bus[last_row][last_col].ea0orv) {
            printf("in %s: [%d][%d] ITYPE_MEX/ITYPE_MO4/ITYPE_MOP cannot find empty reg for RTYPE_OFFS %s (may conflict with prefetch)\n", id[current_prefix].name, last_row, last_col, id[src_hash].name);
            exit(1);
          }
          bus[last_row][last_col].ea0orv = src_type;
          bus[last_row][last_col].ea0orh = src_hash;
	  regv[last_row][last_col].ea0o_v = src_type;
	  regv[last_row][last_col].ea0o_h = src_hash;
	  regv[last_row][last_col].ea0o_s = src_sidx;
        }
        else {
          if (bus[last_row][last_col].ea1orv) {
            printf("in %s: [%d][%d] ITYPE_MEX/ITYPE_MO4/ITYPE_MOP cannot find empty reg for RTYPE_OFFS %s (may conflict with drain)\n", id[current_prefix].name, last_row, last_col, id[src_hash].name);
            exit(1);
          }
          bus[last_row][last_col].ea1orv = src_type;
          bus[last_row][last_col].ea1orh = src_hash;
	  regv[last_row][last_col].ea1o_v = src_type;
	  regv[last_row][last_col].ea1o_h = src_hash;
	  regv[last_row][last_col].ea1o_s = src_sidx;
        }
        break;
      }
    }
    else { /* (insn_type == ITYPE_WHILE || insn_type == ITYPE_FOR || insn_type == ITYPE_CEX || insn_type == ITYPE_EX4 || insn_type == ITYPE_EXE) */
      if (folding)
	h = last_row;
      else
	h = (last_row+EMAX_DEPTH-1)%EMAX_DEPTH; /* 直前段出力BR[][][0-3]の空きを探し,ARMが直接セット */
      /* find same BR1 */
      for (j=0; j<EMAX_WIDTH; j++) {
        for (k=0; k<UNIT_WIDTH; k++) {
          if (bus[h][j].br[k].v==src_type && bus[h][j].br[k].h==src_hash && bus[h][j].br[k].s==src_sidx) /* already set */
            return 0; /* found & do nothing */
        }
      }
      /* find empty BR1 */
      for (j=0; j<EMAX_WIDTH; j++) {
        for (k=0; k<UNIT_WIDTH; k++) {
          if (k==2 && bus[h][j].exdrv && (bus[h][j].exdrh != src_hash))
            continue; /* ★★★ AR->EXDR->BR伝搬にBR[2]を使用する可能性があるので当面避ける ★★★ */
          if (!bus[h][j].br[k].v) { /* empty BR1 found */
            bus[h][j].br[k].v = src_type;
            bus[h][j].br[k].h = src_hash;
            bus[h][j].br[k].s = src_sidx;
            /* generate C-code to copy C-var to emax7_xx_regv[x][y].xxx before all EMAX7 code */
	    regv[h][j].br[k].v = src_type;
	    regv[h][j].br[k].h = src_hash;
	    regv[h][j].br[k].s = src_sidx;
            return 0; /* generate ARM->BR */
          }
        }
      }
      printf("in %s: [%d][%d] cannot find BR1 for %s\n", id[current_prefix].name, last_row, last_col, id[src_hash].name);
      exit(1);
    }
  }
  else { /* initialized by previous row (T_ALRNO, T_BDRNO, T_VARIABLE) */
    h = id[src_hash].row; /* 開始段のみ対象 */
    /* 開始段 < last_row かつ src=ARなら開始段のBRまで持ってくる */
    if (h < last_row) {
      if (id[src_hash].itype == ITYPE_EX4) { /* 同一UNITのTRに集約し直下のBRへ */
        /* find same TR4+BR4 */
        for (j=0; j<EMAX_WIDTH; j++) {
          for (k=0; k<UNIT_WIDTH; k++) {
            if (!(bus[h][j].tr[k].v==src_type && bus[h][j].tr[k].h==src_hash && bus[h][j].tr[k].s==k
               && bus[h][j].br[k].v==src_type && bus[h][j].br[k].h==src_hash && bus[h][j].br[k].s==k)) /* miss */
              break;
          }
          if (k >= UNIT_WIDTH)
            goto srp_ar_br_ready; /* found & proceed */
        }
        /* find empty TR4+BR4 */
        for (j=0; j<EMAX_WIDTH; j++) {
#if 1
          /* LMM-bufferingのためには，伝搬REGがLMP対象UNITに混在してはダメ */
          if (lmmi[h][j].v && !lmmi[h][j].rw && lmmi[h][j].p) /* conflict with lmm pload */
            continue;
#endif
          for (k=0; k<UNIT_WIDTH; k++) {
            if (bus[h][j].tr[k].v || bus[h][j].br[k].v) /* inuse */
              break;
          }
          if (k >= UNIT_WIDTH) { /* empty TR4+BR4 found */
            for (k=0; k<UNIT_WIDTH; k++) {
              bus[h][j].tr[k].v = src_type;
              bus[h][j].tr[k].h = src_hash;
              bus[h][j].tr[k].s = k;
              bus[h][j].br[k].v = src_type;
              bus[h][j].br[k].h = src_hash;
              bus[h][j].br[k].s = k;
            }
            conf[h][j].cdw2.brs0 = 2; /* 2:tr0 */
            conf[h][j].cdw2.brs1 = 2; /* 2:tr1 */
            conf[h][j].cdw2.brs2 = 2; /* 2:tr2 */
            conf[h][j].cdw2.brs3 = 2; /* 2:tr3 */
            goto srp_ar_br_ready; /* found & proceed */
          }
        }
        printf("in %s: [%d][%d] cannot find TR4+BR4 for %s\n", id[current_prefix].name, last_row, last_col, id[src_hash].name);
        exit(1);
      }
      else if (id[src_hash].itype == ITYPE_EXE) { /* 変数が個別EXE演算結果の場合，各EXEのEXDRを経由しBR[2]に接続 */
        /* find same BR[2] */
        j = id[src_hash].col; /* ★★★ id[src_hash].colの参照箇所はここのみ．ITYPE_EXEの場合,VAR[c],AR[r][c]に対して,j=-1 (同一行に複数) */
        if (j<0)        /* exe->&VARの場合,j=last_colがセットされている */
          j = src_sidx; /* exe->VAR[c],AR[r][c]をバラで使う場合,j=-1がセットされている */
        /* printf("h=%d j=%d name=%s | v=%d srctype=%d h=%d srchash=%d s=%d srcidx=%d\n", h, j, id[src_hash].name, bus[h][j].br[2].v, src_type, bus[h][j].br[2].h, src_hash, bus[h][j].br[2].s, src_sidx);*/
        if (bus[h][j].br[2].v==src_type && bus[h][j].br[2].h==src_hash && bus[h][j].br[2].s==src_sidx) /* already set */
          goto srp_ar_br_ready; /* found & proceed */
        /* find empty BR[2] */
        if (!bus[h][j].br[2].v) { /* empty BR[2] found */
          bus[h][j].br[2].v = src_type;
          bus[h][j].br[2].h = src_hash;
          bus[h][j].br[2].s = src_sidx;
          conf[h][j].cdw2.brs2 = 3; /* 3:exdr */
          goto srp_ar_br_ready; /* found & proceed */
        }
        printf("in %s: [%d][%d] cannot find BR[2] for %s (BR[2] is occupied by %s)\n", id[current_prefix].name, last_row, last_col, id[src_hash].name, id[bus[h][j].br[2].h].name);
        exit(1);
      }
      else if (id[src_hash].itype == ITYPE_MEX) { /* 変数がMEXのea0woofs/ea1woofs出力の場合，各々BR[3],BR[2]に接続 */
        j = id[src_hash].col;
        if (j<0)        /* exe->&VARの場合,j=last_colがセットされている */
          j = src_sidx; /* exe->VAR[c],AR[r][c]をバラで使う場合,j=-1がセットされている */
        if (bus[h][j].ea0woofsv==src_type && bus[h][j].ea0woofsh==src_hash) {
	  //printf("------ea0woofs h=%d j=%d name=%s | v=%d srctype=%d h=%d srchash=%d s=%d srcidx=%d\n", h, j, id[src_hash].name, bus[h][j].br[2].v, src_type, bus[h][j].br[2].h, src_hash, bus[h][j].br[2].s, src_sidx);
	  if (bus[h][j].br[2].v==src_type && bus[h][j].br[2].h==src_hash && bus[h][j].br[2].s==src_sidx) /* already set */
	    goto srp_ar_br_ready; /* found & proceed */
	  /* find empty BR[2] */
	  if (!bus[h][j].br[2].v) { /* empty BR[2] found */
	    bus[h][j].br[2].v = src_type;
	    bus[h][j].br[2].h = src_hash;
	    bus[h][j].br[2].s = src_sidx;
	    conf[h][j].cdw2.brs2 = 3; /* 3:exdr(brs3=3の場合,ea0woofsに接続) */
	    goto srp_ar_br_ready; /* found & proceed */
	  }
	  printf("in %s: [%d][%d] cannot find BR[2] for %s (BR[2] is occupied by %s)\n", id[current_prefix].name, last_row, last_col, id[src_hash].name, id[bus[h][j].br[2].h].name);
	  exit(1);
	}
        else if (bus[h][j].ea1woofsv==src_type && bus[h][j].ea1woofsh==src_hash) {
	  //printf("------ea1woofs h=%d j=%d name=%s | v=%d srctype=%d h=%d srchash=%d s=%d srcidx=%d\n", h, j, id[src_hash].name, bus[h][j].br[3].v, src_type, bus[h][j].br[3].h, src_hash, bus[h][j].br[3].s, src_sidx);
	  if (bus[h][j].br[3].v==src_type && bus[h][j].br[3].h==src_hash && bus[h][j].br[3].s==src_sidx) /* already set */
	    goto srp_ar_br_ready; /* found & proceed */
	  /* find empty BR[3] */
	  if (!bus[h][j].br[3].v) { /* empty BR[3] found */
	    bus[h][j].br[3].v = src_type;
	    bus[h][j].br[3].h = src_hash;
	    bus[h][j].br[3].s = src_sidx;
	    conf[h][j].cdw2.brs3 = 3; /* 3:ea1woofs */
	    goto srp_ar_br_ready; /* found & proceed */
	  }
	  printf("in %s: [%d][%d] cannot find BR[3] for %s (BR[3] is occupied by %s)\n", id[current_prefix].name, last_row, last_col, id[src_hash].name, id[bus[h][j].br[3].h].name);
	  exit(1);
	}
	else {
	  printf("in %s: [%d][%d] cannot find MEX ea0woofs/ea1woofs for %s)\n", id[current_prefix].name, last_row, last_col, id[src_hash].name);
	  exit(1);
	}
      }
    }
srp_ar_br_ready:

    /* その後,last_row前段まで必要なら,開始段BRをTR+BRにより伝搬 */
    for (h=id[src_hash].row+1; h<(folding?last_row+1:last_row); h++) { /* 途中まで,伝搬レジスタtr[][]とbr[][]を使用 */
      if ((insn_type == ITYPE_EX4 || insn_type == ITYPE_MO4) && reg_type == RTYPE_DATA) { /* multiple use (ex4/store4) */
	/* find same TR4+BR4 */
	for (j=0; j<EMAX_WIDTH; j++) {
	  for (k=0; k<UNIT_WIDTH; k++) {
	    if (!(bus[h][j].tr[k].v==src_type && bus[h][j].tr[k].h==src_hash && bus[h][j].tr[k].s==k
               && bus[h][j].br[k].v==src_type && bus[h][j].br[k].h==src_hash && bus[h][j].br[k].s==k)) /* miss */
	      break;
	  }
	  if (k >= UNIT_WIDTH)
	    goto srp_tr_br_ready; /* found & proceed */
	}
	/* find empty TR4+BR4 */
	for (j=0; j<EMAX_WIDTH; j++) {
#if 1
	  /* LMM-bufferingのためには，伝搬REGがLMP対象UNITに混在してはダメ */
	  if (lmmi[h][j].v && !lmmi[h][j].rw && lmmi[h][j].p) /* conflict with lmm pload */
	    continue;
#endif
	  for (k=0; k<UNIT_WIDTH; k++) {
	    if (bus[h][j].tr[k].v || bus[h][j].br[k].v) /* inuse */
	      break;
	  }
	  if (k >= UNIT_WIDTH) { /* empty TR4+BR4 found */
            for (k=0; k<UNIT_WIDTH; k++) {
	      bus[h][j].tr[k].v = src_type;
	      bus[h][j].tr[k].h = src_hash;
	      bus[h][j].tr[k].s = k;
	      bus[h][j].br[k].v = src_type;
	      bus[h][j].br[k].h = src_hash;
	      bus[h][j].br[k].s = k;
	    }
            conf[h][j].cdw2.brs0 = 2; /* 2:tr0 */
	    conf[h][j].cdw2.brs1 = 2; /* 2:tr1 */
	    conf[h][j].cdw2.brs2 = 2; /* 2:tr2 */
	    conf[h][j].cdw2.brs3 = 2; /* 2:tr3 */
	    goto srp_tr_br_ready; /* found & proceed */
	  }
	}
	printf("in %s: [%d][%d] cannot find TR4+BR4 for %s\n", id[current_prefix].name, last_row, last_col, id[src_hash].name);
	exit(1);
      }
      else { /* (insn_type == ITYPE_WHILE || insn_type == ITYPE_FOR || insn_type == ITYPE_CEX || insn_type == ITYPE_EXE || insn_type == ITYPE_MOP) *//* single use */
        /* find same TR1+BR1 */
        for (j=0; j<EMAX_WIDTH; j++) {
          for (k=0; k<UNIT_WIDTH; k++) {
            if (bus[h][j].tr[k].v==src_type && bus[h][j].tr[k].h==src_hash && bus[h][j].tr[k].s==src_sidx
             && bus[h][j].br[k].v==src_type && bus[h][j].br[k].h==src_hash && bus[h][j].br[k].s==src_sidx) /* hit */
              goto srp_tr_br_ready; /* found & proceed */
          }
        }
        /* find empty TR1+BR1 */
        for (j=0; j<EMAX_WIDTH; j++) {
#if 1
          /* LMM-bufferingのためには，伝搬REGがLMP対象UNITに混在してはダメ */
          if (lmmi[h][j].v && !lmmi[h][j].rw && lmmi[h][j].p) /* conflict with lmm pload */
            continue;
#endif
          for (k=0; k<UNIT_WIDTH; k++) {
            if (k==2 && bus[h][j].exdrv && (bus[h][j].exdrh != src_hash))
              continue; /* ★★★ AR->EXDR->BR伝搬にBR[2]を使用する可能性があるので当面避ける ★★★ */
            if (!bus[h][j].tr[k].v && !bus[h][j].br[k].v) { /* empty */
	      if (!bus[h][j].mw[k].v || search_prev_ar0_mw(h, j, k, bus[h][j].mw[k].v, bus[h][j].mw[k].h, bus[h][j].mw[k].s) >= 0) { /* 自unit内にsrcの場合はtr競合無し */
		bus[h][j].tr[k].v = src_type;
		bus[h][j].tr[k].h = src_hash;
		bus[h][j].tr[k].s = src_sidx;
		bus[h][j].br[k].v = src_type;
		bus[h][j].br[k].h = src_hash;
		bus[h][j].br[k].s = src_sidx;
		switch (k) {
		case 0: conf[h][j].cdw2.brs0 = 2; /* 2:tr0 */ break;
		case 1: conf[h][j].cdw2.brs1 = 2; /* 2:tr1 */ break;
		case 2: conf[h][j].cdw2.brs2 = 2; /* 2:tr2 */ break;
		case 3: conf[h][j].cdw2.brs3 = 2; /* 2:tr3 */ break;
		}
		goto srp_tr_br_ready; /* found & proceed */
	      }
	      else { /* 前unitにsrcがあると，tr競合有りのため回避必要 */
		continue;
	      }
            }
          }
        }
        /* find same emergency bypass through EXE+EXDR+BR[2] */
        for (j=0; j<EMAX_WIDTH; j++) {
          if (!dec[h][j].dexu.op1 && dec[h][j].dexu.op2 == OP_OR && !dec[h][j].dexu.op3
           && bus[h][j].exdrv   == src_type && bus[h][j].exdrh   == src_hash && bus[h][j].exdrs   == src_sidx
           && bus[h][j].br[2].v == src_type && bus[h][j].br[2].h == src_hash && bus[h][j].br[2].s == src_sidx) {
            goto srp_tr_br_ready;
          }
        }
        /* find empty emergency bypass through EXE+EXDR+BR[2] */
        for (j=0; j<EMAX_WIDTH; j++) {
          if (!dec[h][j].dexu.op1 && !dec[h][j].dexu.op2 && !dec[h][j].dexu.op3
           && !bus[h][j].exdrv && !bus[h][j].br[2].v) {
            dec[h][j].dexu.op1  = OP_NOP;
            dec[h][j].dexu.op2  = OP_OR;
            dec[h][j].dexu.op3  = OP_NOP;
            dec[h][j].dexu.updt = 0;
            dec[h][j].dexu.init = 0;
            dec[h][j].dexu.ex1v = src_type;
            dec[h][j].dexu.ex1h = src_hash;
            dec[h][j].dexu.ex1s = src_sidx;
            dec[h][j].dexu.ex1e = EXP_H3210;
            dec[h][j].dexu.ex2v = T_NONE;
            dec[h][j].dexu.ex2h = -1;
            dec[h][j].dexu.ex2s = -1;
            dec[h][j].dexu.ex2e = 0;
            dec[h][j].dexu.ex3v = T_NONE;
            dec[h][j].dexu.ex3h = -1;
            dec[h][j].dexu.ex3s = -1;
            dec[h][j].dexu.ex3e = 0;
            dec[h][j].dexu.e2iv = T_IMMEDIATE;
            dec[h][j].dexu.e2ih = hash_reg_immediate(0LL);
            dec[h][j].dexu.e2is = 0; /* e2imm */
            dec[h][j].dexu.e3iv = T_NONE;
            dec[h][j].dexu.e3ih = -1;
            dec[h][j].dexu.e3is = 0; /* e3imm */
            dec[h][j].dexu.exdv = src_type;
            dec[h][j].dexu.exdh = src_hash;
            dec[h][j].dexu.exds = src_sidx;
            bus[h][j].exdrv     = src_type;
            bus[h][j].exdrh     = src_hash;
            bus[h][j].exdrs     = src_sidx;
            bus[h][j].br[2].v   = src_type;
            bus[h][j].br[2].h   = src_hash;
            bus[h][j].br[2].s   = src_sidx;
            conf[h][j].cdw2.brs2 = 3; /* 3:exdr */
            goto srp_tr_br_ready;
          }
        }
        /* find empty TR1+BR1 */
	/* ★★★ どうしてもない場合,BR[2]を使用 ★★★ */
        for (j=0; j<EMAX_WIDTH; j++) {
#if 1
          /* LMM-bufferingのためには，伝搬REGがLMP対象UNITに混在してはダメ */
          if (lmmi[h][j].v && !lmmi[h][j].rw && lmmi[h][j].p) /* conflict with lmm pload */
            continue;
#endif
          for (k=0; k<UNIT_WIDTH; k++) {
            if (!bus[h][j].tr[k].v && !bus[h][j].br[k].v) { /* empty */
	      if (!bus[h][j].mw[k].v || search_prev_ar0_mw(h, j, k, bus[h][j].mw[k].v, bus[h][j].mw[k].h, bus[h][j].mw[k].s) >= 0) { /* 自unit内にsrcの場合はtr競合無し */
		bus[h][j].tr[k].v = src_type;
		bus[h][j].tr[k].h = src_hash;
		bus[h][j].tr[k].s = src_sidx;
		bus[h][j].br[k].v = src_type;
		bus[h][j].br[k].h = src_hash;
		bus[h][j].br[k].s = src_sidx;
		switch (k) {
		case 0: conf[h][j].cdw2.brs0 = 2; /* 2:tr0 */ break;
		case 1: conf[h][j].cdw2.brs1 = 2; /* 2:tr1 */ break;
		case 2: conf[h][j].cdw2.brs2 = 2; /* 2:tr2 */ break;
		case 3: conf[h][j].cdw2.brs3 = 2; /* 2:tr3 */ break;
		}
		goto srp_tr_br_ready; /* found & proceed */
	      }
	      else { /* 前unitにsrcがあると，tr競合有りのため回避必要 */
		continue;
	      }
            }
          }
        }
        printf("in %s: [%d][%d] cannot find TR1+BR1 for %s\n", id[current_prefix].name, last_row, last_col, id[src_hash].name);
        exit(1);
      }
srp_tr_br_ready:
      continue;
    }
  }
}

search_prev_br0(int row, int folding, int src_type, int src_hash, int src_sidx)
{
  /* return 0-3:br[][0].br[0-3], 4-7:br[][1].br[0-3], 8-11:br[][2].br[0-3], 12-15:br[][3].br[0-3] */
  /* return 0 and warn if not found */
  int i, j;

  if (!folding)
    row = (row+EMAX_DEPTH-1)%EMAX_DEPTH;
  if (src_type) {
    for (i=0; i<EMAX_WIDTH; i++) {
      for (j=0; j<UNIT_WIDTH; j++) {
        if (bus[row][i].br[j].v == src_type && bus[row][i].br[j].h == src_hash && bus[row][i].br[j].s == src_sidx)
          return (i*UNIT_WIDTH + j);
      }
    }
    printf("in %s: [%d][] (folding=%d) cannot find source bus[%d][].br[] for %s sidx=%d (malfunction)\n", id[current_prefix].name, row, folding, row, id[src_hash].name, src_sidx);
    exit(1);
  }
  return (0);
}

search_prev_ar0_tr(int row, int col, int pos, int src_type, int src_hash, int src_sidx)
{
  /* col=0 ... return 0:lmwd0, 1:exdr, 2:ts0 */
  /* col=1 ... return 0:lmwd1, 1:exdr, 2:ts1 */
  /* col=2 ... return 0:lmwd2. 1:exdr, 2:ts2 */
  /* col=3 ... return 0:lmwd3, 1:exdr, 2:ts3 */
  /* return -1 if not found */
  int i, j;

  /* tsX使用の伝搬設定(retval=2)はsearch_prev_br0の後に設定 */
  if (src_type) {
    if (bus[row][pos].exdrv       == src_type && bus[row][pos].exdrh       == src_hash && bus[row][pos].exdrs       == src_sidx) return (1); /* EX4->TR4->BR4 */
    if (bus[row][col].lmwd[pos].v == src_type && bus[row][col].lmwd[pos].h == src_hash && bus[row][col].lmwd[pos].s == src_sidx) return (0); /* LDDMQ */
    return (-1);
  }
  else
    return (0);
}

search_prev_ar0_mw(int row, int col, int pos, int src_type, int src_hash, int src_sidx)
{
  /* col=0 ... return 0:lmwd0, 1:exdr, 2:ts0 */
  /* col=1 ... return 0:lmwd1, 1:exdr, 2:ts1 */
  /* col=2 ... return 0:lmwd2, 1:exdr, 2:ts2 */
  /* col=3 ... return 0:lmwd3, 1:exdr, 2:ts3 */
  /* return -1 if not found */
  int i, j;

  /* tsX使用の伝搬設定(retval=2)はsearch_prev_br0の後に設定 */
  if (src_type) {
    if (bus[row][col].exdrv       == src_type && bus[row][col].exdrh       == src_hash && bus[row][col].exdrs       == src_sidx) return (1);
    if (bus[row][pos].exdrv       == src_type && bus[row][pos].exdrh       == src_hash && bus[row][pos].exdrs       == src_sidx) return (1);
    if (bus[row][col].lmwd[pos].v == src_type && bus[row][col].lmwd[pos].h == src_hash && bus[row][col].lmwd[pos].s == src_sidx) return (0);
    return (-1);
  }
  else
    return (0);
}

emit_emax7t(int type) /* 0:transaction */
{
  int i, j, l;

  for (i=0; i<=trans_pc; i++) {
    tconf[i].rw            = trans[i].rw;
    tconf[i].base_type     = trans[i].base_type;
    tconf[i].offset_type   = trans[i].offset_type;
    tconf[i].offset        = trans[i].offset;
    tconf[i].offset_suffix = trans[i].offset_suffix;
    tconf[i].offset_sll    = trans[i].offset_sll;
    tconf[i].op_type       = trans[i].op_type;
    tconf[i].op_val_type   = trans[i].op_val_type > 0;
    tconf[i].t_action_type = trans[i].t_action_type;
    tconf[i].t_action      = trans[i].t_action;
    tconf[i].f_action_type = trans[i].f_action_type;
    tconf[i].f_action      = trans[i].f_action;
    tconf[i].reg_type      = trans[i].reg_type;
    fprintf(ofile, "\t.word\t0x%08.8x /* tconf[%d].word0 */\n", *(Uint*)&tconf[i], i);
    if (trans[i].base_type == 2)
      fprintf(ofile, "%s\t.word\t0x%08.8x /* tconf[%d].base */\n", trans[i].base_symbol, trans[i].base_num, i);
    else
      fprintf(ofile, "\t.word\t0x%08.8x /* tconf[%d].base */\n", trans[i].base_num, i);
    if (trans[i].op_val_type == 2)
      fprintf(ofile, "%s\t.word\t0x%08.8x /* tconf[%d].op_val */\n", trans[i].op_val_symbol, trans[i].op_val_num, i);
    else
      fprintf(ofile, "\t.word\t0x%08.8x /* tconf[%d].op_val */\n", trans[i].op_val_num, i);
    if (trans[i].reg_type == 1)
      fprintf(ofile, "%s\t.word\t0x%08.8x /* tconf[%d].reg */\n", trans[i].reg_symbol, trans[i].reg_num, i);
    else
      fprintf(ofile, "\t.word\t0x%08.8x /* tconf[%d].reg */\n", trans[i].reg_num, i);
  }
}

emit_tgif(int i, int j)
{
  int k;
  int base_row = (i%16) * 540  + 300;
  int base_col = (i/16) * 2020 + ((EMAX_WIDTH-1)-j) * 500;
  int bro_x = base_col,     bro_y = base_row+10;
  int aro_x = base_col,     aro_y = base_row+210;
  int lmi_x = base_col,     lmi_y = base_row+280;
  int exb_x = base_col+70,  exb_y = base_row+160;
  int cxb_x = base_col+20,  cxb_y = base_row+160;
  int e0b_x = base_col+320, e0b_y = base_row+170;
  int e1b_x = base_col+200, e1b_y = base_row+170;
  int trb_x = base_col+60,  trb_y = base_row+350;
  int lmb_x = base_col+40,  lmb_y = base_row+380;
  int bri_x = base_col+40,  bri_y = base_row+530;

  for (k=0; k<EMAX_WIDTH*UNIT_WIDTH; k++)
    draw_bro(i, j, bro_x, bro_y, k);

  draw_aro(i, j, aro_x, aro_y);

  for (k=0; k<UNIT_WIDTH+1; k++)
    draw_lmi(i, j, lmi_x, lmi_y, k);

  draw_exe(exb_x, exb_y,
           conf[i][j].cdw0.op1,    /*:  6; alu_opcd */
           conf[i][j].cdw0.op2,    /*:  3; logical_opcd */
           conf[i][j].cdw0.op3,    /*:  3; sft_opcd */
           conf[i][j].cdw0.init,   /*:  2; conditional exec on INIT1/INI10 */
           conf[i][j].cdw0.fold,   /*:  1; 0:normal, 1:load-exe-store folding */
           conf[i][j].cdw0.ex1brs, /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
           conf[i][j].cdw0.ex1s,   /*:  1; 0:ex1brs, 1:exdr(self-loop) */
           conf[i][j].cdw0.ex1exp, /*:  3; 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
           conf[i][j].cdw0.ex2brs, /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
           conf[i][j].cdw0.ex2exp, /*:  3; 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
           conf[i][j].cdw0.ex3brs, /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
           conf[i][j].cdw0.ex3exp, /*:  3; 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
           conf[i][j].cdw3.e2imm,  /*: 64; */
           conf[i][j].cdw0.e2is,   /*:  2; 0:e2imm, 1:ex2, 2:ex3 */
           conf[i][j].cdw0.e3imm,  /*:  E3IMMBITS; */
           conf[i][j].cdw0.e3is    /*:  1; 0:e3imm, 1:ex3 */
           );

  draw_cex(cxb_x, cxb_y,
           conf[i][j].cdw1.cs0,    /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
           conf[i][j].cdw1.cs1,    /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
           conf[i][j].cdw1.cs2,    /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
           conf[i][j].cdw1.cs3,    /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
           conf[i][j].cdw1.cex_tab /*: 16; c3.c2.c1.c0の組合せ (cop=NOPの場合,ffff) */
                                   /* 1111,1110,1101,1100,....,0001,0000 の各々に0/1を割り当てた16bitを指定 */
           );

  draw_ea0(e0b_x, e0b_y,
           conf[i][j].cdw1.ea0op,   /*:  5; mem_opcd */
           conf[i][j].cdw1.ea0bs,   /*:  2; 0:ea0br, 1:ea0dr(ea0br+self-loop), 2:eabbrs, 3:ea0dr(eabbrs+self-loop) */
           conf[i][j].cdw1.ea0os,   /*:  1; 0:ea0or, 1:eaobrs */
           conf[i][j].cdw1.ea0msk,  /*:  4; 14:64bit, 13:word1, 12:word0, 11-8:half3-0, 7-0:byte7-0 of offset */
           conf[i][j].cdw1.eabbrs,  /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
           conf[i][j].cdw1.eaobrs,  /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
	   conf[i][j].cdw0.mex0op,  /* mex(sparse matrix) conditional 0:NOP, 1:AL, 2:OP_CMPA_LE, 3:OP_CMPA_GE */
	   conf[i][j].cdw0.mex0init,/* mex(sparse matrix) 0:none, 1:INIT0? */
	   conf[i][j].cdw0.mex0dist,/* distance 0:0, 1:1, 2:2, 3:4, 4:8, 5:16, 6:32, 7:64byte */
	   conf[i][j].cdw0.mexlimit /* limit 0:0, 1:8, 2:16, .... 10:4096, 11:8192, 12:16384, 13:32768 */
           );

  draw_ea1(e1b_x, e1b_y,
           conf[i][j].cdw1.ea1op,   /*:  5; mem_opcd */
           conf[i][j].cdw1.ea1bs,   /*:  2; 0:ea1br, 1:ea1dr(ea1br+self-loop), 2:eabbrs, 3:ea1dr(eabbrs+self-loop) */
           conf[i][j].cdw1.ea1os,   /*:  1; 0:ea1or, 1:eaobrs */
           conf[i][j].cdw1.ea1msk,  /*:  4; 14:64bit, 13:word1, 12:word0, 11-8:half3-0, 7-0:byte7-0 of offset */
           conf[i][j].cdw1.eabbrs,  /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
           conf[i][j].cdw1.eaobrs,  /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
	   conf[i][j].cdw0.mex1op,  /* mex(sparse matrix) conditional 0:NOP, 1:AL, 2:OP_CMPA_LE, 3:OP_CMPA_GE */
	   conf[i][j].cdw0.mex1init,/* mex(sparse matrix) 0:none, 1:INIT0? */
	   conf[i][j].cdw0.mex1dist,/* distance 0:0, 1:1, 2:2, 3:4, 4:8, 5:16, 6:32, 7:64byte */
	   conf[i][j].cdw0.mexlimit /* limit 0:0, 1:8, 2:16, .... 10:4096, 11:8192, 12:16384, 13:32768 */
           );

  draw_trx(trb_x, trb_y,
           bus[i][j].tr[0].v,
           bus[i][j].tr[1].v,
           bus[i][j].tr[2].v,
           bus[i][j].tr[3].v,
           conf[i][j].cdw2.ts0,    /*:  4; 0:br0_0, 1:br0_1, ... 15:br3_3 */
           conf[i][j].cdw2.ts1,    /*:  4; 0:br0_0, 1:br0_1, ... 15:br3_3 */
           conf[i][j].cdw2.ts2,    /*:  4; 0:br0_0, 1:br0_1, ... 15:br3_3 */
           conf[i][j].cdw2.ts3,    /*:  4; 0:br0_0, 1:br0_1, ... 15:br3_3 */
           conf[i][j].cdw2.trs0,   /*:  2; 0:lmwd0, 1:exdr, 2:ts0 */
           conf[i][j].cdw2.trs1,   /*:  2; 0:lmwd1, 1:exdr, 2:ts1 */
           conf[i][j].cdw2.trs2,   /*:  2; 0:lmwd2. 1:exdr, 2:ts2 */
           conf[i][j].cdw2.trs3    /*:  2; 0:lmwd3, 1:exdr, 2:ts3 */
           );

  draw_lmx(lmb_x, lmb_y,
	   lmmi[i][j].v,           /*:  1; valid */
           conf[i][j].cdw1.ea0op,  /*:  5; mem_opcd */
           conf[i][j].cdw2.mwsa,   /*:  1; 0:lmwa,  1:ea0d        */
           conf[i][j].cdw2.mws0,   /*:  2; 0:lmwd0, 1:exdr, 2:ts0 */
           conf[i][j].cdw2.mws1,   /*:  2; 0:lmwd1, 1:exdr, 2:ts1 */
           conf[i][j].cdw2.mws2,   /*:  2; 0:lmwd2, 1:exdr, 2:ts2 */
           conf[i][j].cdw2.mws3,   /*:  2; 0:lmwd3, 1:exdr, 2:ts3 */
           conf[i][j].cdw2.ts0,
           conf[i][j].cdw2.ts1,
           conf[i][j].cdw2.ts2,
           conf[i][j].cdw2.ts3,
           conf[i][j].cdw2.lmm_mode,/*:  2; 0:無効, 1:分割無, 2:2分割, 3:4分割 */
           lmmi[i][j].top
           );

  draw_bri(bri_x, bri_y,
           conf[i][j].cdw1.ea0op,  /*:  5; mem_opcd */
           conf[i][j].cdw1.ea1op,  /*:  5; mem_opcd */
           conf[i][j].cdw2.brs0,   /*:  2; 0:off, 1:mr10, 2:tr0, 3:mr0  */
           conf[i][j].cdw2.brs1,   /*:  2; 0:off, 1:mr11, 2:tr1, 3:mr1  */
           conf[i][j].cdw2.brs2,   /*:  2; 0:off, 1:mr12, 2:tr2, 3:exdr */
           conf[i][j].cdw2.brs3,   /*:  2; 0:off, 1:mr13, 2:tr3         */
           bus[i][j].br[0].v,
           bus[i][j].br[0].h,
           bus[i][j].br[0].s,
           bus[i][j].br[1].v,
           bus[i][j].br[1].h,
           bus[i][j].br[1].s,
           bus[i][j].br[2].v,
           bus[i][j].br[2].h,
           bus[i][j].br[2].s,
           bus[i][j].br[3].v,
           bus[i][j].br[3].h,
           bus[i][j].br[3].s
           );

  draw_lmr(bri_x, bri_y,
           bus[i][j].lmrd[0].v,
           bus[i][j].lmrd[1].v,
           bus[i][j].lmrd[2].v,
           bus[i][j].lmrd[3].v
           );
}

/*    0   50   100  150  200  250  300  350  400  450  500 */
/*   0+----+----+----+----+----+----+----+----+----+----+  */
/*  10  |||| :  |  |  |  :---------| :         | :   BOBASE*/
/*  15  |||| :  |  |  |  :---------| :         | :         */
/*  20  |||| :  |  |  |  :---------| :         | :         */
/*  25  |||| :  |  |  |  :---------| :         | :         */
/*  30  |||| :  |  |  |  :---------| :         | :         */
/*  35  |||| :  |  |  |  :---------| :         | :         */
/*  40  |||| :  |  |  |  :---------| :         | :         */
/*  45  |||| :  |  |  |  :---------| :         | :         */
/*  50  |||| :  |  |  |  :---------| :         | :         */
/*  55  |||| :  |  |  |  :---------| :         | :         */
/*  60  |||| :  |  |  |  :---------| :         | :         */
/*  65  |||| :  |  |  |  :---------| :         | :         */
/*  70  |||| :  |  |  |  :---------| :         | :         */
/*  75  |||| :  |  |  |  :---------| :         | :         */
/*  80  |||| :  |  |  |  :---------| :         | :         */
/*  85  |||| :  |  |  |  :---------| :         | :         */
/*  90  |||| :  |  |  |  :         | :         | :         */
/* 100  |||| :  |  |  |  :   +-----*-----+     | :         */
/* 110  |||| :  |  |  |  :   |     +-----------+ :         */
/* 120  |||| :  |  |  |  :   |     | :   |     | :         */
/* 130  |||| :  |  |  |  :+---+ +---+:+---+ +---+:         */
/* 140  |||| :  |  |  |  :+-*-+ +-*-+:+---+ +---+:         */
/* 150  |||| :  -  -  -  :  ||    || :  ||    || :         */
/* 160  +--+ +-----------+  --    -- :  --    -- :   EXBASE*/
/* 170  |  | :\ A       /:+---------+:+---------+:         */
/* 180  |  | : \|      / : \A      / : \A      / :         */
/* 190  +--+ |  +--*--+  |  +--*--+  |  +--*--+  |         */
/* 200       |  |  |     |  |  |     |  |  |     |         */
/* 210       |  |  |     |  |  |     |  |  |    *|-  AOBASE*/
/* 215       |  |  |     |  |  |    *|  |  |    ||-        */
/* 220       |  |  |    *|  |  |    ||  |  |    ||-        */
/* 225      *|  |  |    ||  |  |    ||  |  |    ||-        */
/* 230   --*||--|--*---*||---------*||---------*||-        */
/* 260     |||  |  |   |||  |  |   |||  |  |   |||         */
/* 270     |||+<|  |   |||+<|  |+  |||+<|  |+  |||+<       */
/* 280   --||||--------||||--------||||------**|||*  LIBASE*/
/* 285   --||||--------||||------**|||*------||||||        */
/* 290   --||||------**|||*------||||||------||||||        */
/* 295   **|||*------||||||------||||||------||||||        */
/* 300   ||||||------||||||------||||||-----*||||||        */
/* 310   |||||| |  | |||||| |  |||||||| |  ||||||||        */
/* 320   ****** |  | ****** |  ||****** |  ||******        */
/* 330     | || |  |   | || |  ||  | || |  ||  | ||        */
/* 340     | ** |  |   | ** |  ||  | ** |  ||  | **        */
/* 350     |+*+ | +*+  |+*+ | +*+  |+*+ | +*+  |+*+  TRBASE*/
/* 360     |+*+ | +*+  |+*+ | +*+  |+*+ | +*+  |+*+        */
/* 370     | |  +--*   | |  +--*   | |  +--*   | |         */
/* 380  : +*----+  |  +*----+ +*+ +*----+ +*+ +*----+LMBASE*/
/* 390  : +-----+  |  +-----+ +-+ +-----+ +-+ +-----+      */
/* 400       |     |     |     |     |     |     |         */
/* 410    +-----+  |  +-----+  |  +-----+  |  +-----+      */
/* 420    |-----|  |  |-----|  |  |-----|  |  |-----|      */
/* 430    |-----|  |  |-----|  |  |-----|  |  |-----|      */
/* 440    +---**+  |  +---**+  |  +---**+  |  +---**+      */
/* 450       |||   +-+   |||   |     ||+-------++||+       */
/* 460       |||     |   ||+---------||-------+||||        */
/* 470       ||+-----|-- ||----------||------+|||||        */
/* 480       ||      |   |*-------++-|*    | ||||||        */
/* 500       |*------|---||------+||+||------|||||*        */
/* 510       ||      *--*||    +>*--*||    +>*--*||        */
/* 520       |+>        ||+>        ||+>   |    ||+>       */
/* 530    +--**-+     +-***-+     +-***-+  V  +-***-+BIBASE*/
/* 540    +-----+     +-----+     +-----+     +-----+      */

draw_bro(int i, int j, int bro_x, int bro_y, int num)
{
  int col, thi;
  i = (i+EMAX_DEPTH-1)%EMAX_DEPTH;

  /* BR0-0..3-3 */
  switch (bus[i][num/UNIT_WIDTH].br[num%UNIT_WIDTH].v) {
  case 0: /* T_NONE */
    col=0; /*black*/
    thi=1;
    break;
  case 1: /* T_IMMEDIATE */
    col=4; /*blue*/
    thi=3;
    break;
  case 2: /* T_EXRNO */
    col=5; /*magenta*/
    thi=3;
    break;
  case 3: /* T_ALRNO */
    col=3; /*yellow*/
    thi=3;
    break;
  case 4: /* T_BDRNO */
    col=1; /*red*/
    thi=3;
    break;
  case 7: /* T_VARIABLE */
    col=2; /*green*/
    thi=3;
    break;
  default:
    col=6; /*cyan*/
    thi=3;
    break;
  }
  draw_line(bro_x,  bro_y+num*5, bro_x+500,  bro_y+num*5, thi, col);
}

draw_aro(int i, int j, int aro_x, int aro_y)
{
  int col, thi;

  /* AR0..EXD */
  switch (bus[i][j].exdrv) {
  case 0: /* T_NONE */
    col=0; /*black*/
    thi=1;
    break;
  case 1: /* T_IMMEDIATE */
    col=4; /*blue*/
    thi=3;
    break;
  case 2: /* T_EXRNO */
    col=5; /*magenta*/
    thi=3;
    break;
  case 3: /* T_ALRNO */
    col=3; /*yellow*/
    thi=3;
    break;
  case 4: /* T_BDRNO */
    col=1; /*red*/
    thi=3;
    break;
  case 7: /* T_VARIABLE */
    col=2; /*green*/
    thi=3;
    break;
  default:
    col=6; /*cyan*/
    thi=3;
    break;
  }
  draw_line(aro_x,  aro_y+20, aro_x+500,  aro_y+20, thi, col);
}

draw_lmi(int i, int j, int lmi_x, int lmi_y, int num)
{
  int col, thi;
  int lmiall;

  /* LMI */
  if (num < UNIT_WIDTH) { /* lmwd */
    lmiall = bus[i][j].lmwd[num].v;
    switch (lmiall) {
    case 0: /* T_NONE */
      col=0; /*black*/
      thi=1;
      break;
    case 1: /* T_IMMEDIATE */
      col=4; /*blue*/
      thi=3;
      break;
    case 2: /* T_EXRNO */
      col=5; /*magenta*/
      thi=3;
      break;
    case 3: /* T_ALRNO */
      col=3; /*yellow*/
      thi=3;
      break;
    case 4: /* T_BDRNO */
      col=1; /*red*/
      thi=3;
      break;
    case 7: /* T_VARIABLE */
      col=2; /*green*/
      thi=3;
      break;
    default:
      col=6; /*cyan*/
      thi=3;
      break;
    }
    draw_line(lmi_x,  lmi_y+num*5, lmi_x+500,  lmi_y+num*5, thi, col);
  }
  else { /* lmwa */
    lmiall = bus[i][j].lmwd[0].v;
    switch (lmiall) {
    case 0: /* T_NONE */
      col=0; /*black*/
      thi=1;
      break;
    case 1: /* T_IMMEDIATE */
      col=4; /*blue*/
      thi=3;
      break;
    case 2: /* T_EXRNO */
      col=5; /*magenta*/
      thi=3;
      break;
    case 3: /* T_ALRNO */
      col=3; /*yellow*/
      thi=3;
      break;
    case 4: /* T_BDRNO */
      col=1; /*red*/
      thi=3;
      break;
    case 7: /* T_VARIABLE */
      col=2; /*green*/
      thi=3;
      break;
    default:
      col=6; /*cyan*/
      thi=3;
      break;
    }
    draw_line(lmi_x,  lmi_y+num*5, lmi_x+500,  lmi_y+num*5, thi, col);
  }
}

draw_exe(int exb_x, int exb_y,
         int op1,    /*:  6; alu_opcd */
         int op2,    /*:  3; logical_opcd */
         int op3,    /*:  3; sft_opcd */
	 int init,   /*:  2; conditional exec on INIT1/INI10 */
         int fold,   /*:  1; 0:normal, 1:load-exe-store folding */
         int ex1brs, /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
         int ex1s,   /*:  1; 0:ex1brs, 1:exdr(self-loop) */
         int ex1exp, /*:  3; 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
         int ex2brs, /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
         int ex2exp, /*:  3; 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
         int ex3brs, /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
         int ex3exp, /*:  3; 0:H3210, 1:H1010, 2:H3232, 3:B5410, 4:B7632 */
         Ull e2imm,  /*: 64; */
         int e2is,   /*:  2; 0:e2imm, 1:ex2, 2:ex3 */
         int e3imm,  /*:  E3IMMBITS; */
         int e3is    /*:  1; 0:e3imm, 1:ex3 */
         )
{
#define EXLABELMAX 30
  char opcd[EXLABELMAX];
  int col, thi;

  /* ALU */
  if (op1 || op2 || op3) {
    col=5; /*magenta*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line(exb_x+20,  exb_y-10, exb_x+40,  exb_y-10, thi, col);
  draw_line(exb_x+50,  exb_y-10, exb_x+70,  exb_y-10, thi, col);
  draw_line(exb_x+80,  exb_y-10, exb_x+100, exb_y-10, thi, col);
  draw_line(exb_x,     exb_y,    exb_x+120, exb_y,    thi, col);
  draw_line(exb_x+120, exb_y,    exb_x+90,  exb_y+30, thi, col);
  draw_line(exb_x+90,  exb_y+30, exb_x+30,  exb_y+30, thi, col);
  draw_line(exb_x+30,  exb_y+30, exb_x,     exb_y,    thi, col);
  snprintf(opcd, EXLABELMAX, "%02.2x%01.1x%01.1x-%01.1x-%01.1x", op1, op2, op3, init, fold);
  draw_text(exb_x+35, exb_y+15, opcd, 1, col);
  if (e2is == 0) {
    snprintf(opcd, EXLABELMAX, "%08.8x", (Uint)(e2imm>>32));
    draw_text(exb_x-50, exb_y+45, opcd, 1, col);
    snprintf(opcd, EXLABELMAX, "%08.8x", (Uint)e2imm);
    draw_text(exb_x+35, exb_y+45, opcd, 1, col);
  }
  if (e3is == 0) {
    snprintf(opcd, EXLABELMAX, "%08.8x", e3imm);
    draw_text(exb_x+35, exb_y+30, opcd, 1, col);
  }
  draw_line (exb_x+60, exb_y+30,              exb_x+60, exb_y+190,     thi, col);
  draw_box  (exb_x+50, exb_y+190,             exb_x+70, exb_y+200,     thi, col);

  /* SRC1 */
  if (op1 || op2 || op3) {
    col=5; /*magenta*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_arrow(exb_x+30, exb_y-160+ex1brs*5+10, exb_x+30, exb_y-160+150, thi, col);
  if (ex1s) { /* feedback-loop */
    col=5; /*magenta*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line (exb_x+60, exb_y+200,             exb_x+60, exb_y+210,     thi, col);
  draw_line (exb_x+60, exb_y+210,             exb_x+30, exb_y+210,     thi, col);
  draw_arrow(exb_x+30, exb_y+210,             exb_x+30, exb_y-10,      thi, col);
  /* SRC2 */
  if (op1 || op2 || op3) {
    col=5; /*magenta*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_arrow(exb_x+60, exb_y-160+ex2brs*5+10, exb_x+60, exb_y-160+150, thi, col);
  /* SRC3 */
  if (op1 || op2 || op3) {
    col=5; /*magenta*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_arrow(exb_x+90, exb_y-160+ex3brs*5+10, exb_x+90, exb_y-160+150, thi, col);
}

draw_cex(int cxb_x, int cxb_y,
         int cs0,    /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
         int cs1,    /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
         int cs2,    /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
         int cs3,    /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
         int cex_tab /*: 16; c3.c2.c1.c0の組合せ (cop=NOPの場合,ffff) */
                     /* 1111,1110,1101,1100,....,0001,0000 の各々に0/1を割り当てた16bitを指定 */
         )
{
#define CXLABELMAX 30
  char opcd[CXLABELMAX];
  int col, thi;

  /* CEX */
  if (cex_tab != 0xffff) {
    col=2; /*green*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (cxb_x-10, cxb_y, cxb_x+40, cxb_y+30, thi, col);
  snprintf(opcd, CXLABELMAX, "%04.4x", cex_tab);
  draw_text(cxb_x, cxb_y+20, opcd, 1, 0);

  /* SRC1 */
  if (cex_tab != 0xffff) {
    col=2; /*green*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_arrow(cxb_x+30, cxb_y-160+cs0*5+10, cxb_x+30, cxb_y-5,  thi, col);
  /* SRC2 */
  if (cex_tab != 0xffff) {
    col=2; /*green*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_arrow(cxb_x+20, cxb_y-160+cs1*5+10, cxb_x+20, cxb_y-5, thi, col);
  /* SRC3 */
  if (cex_tab != 0xffff) {
    col=2; /*green*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_arrow(cxb_x+10, cxb_y-160+cs2*5+10, cxb_x+10, cxb_y-5, thi, col);
  /* SRC4 */
  if (cex_tab != 0xffff) {
    col=2; /*green*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_arrow(cxb_x+0,  cxb_y-160+cs3*5+10, cxb_x+0,  cxb_y-5, thi, col);
}

draw_ea0(int e0b_x, int e0b_y,
         int ea0op,  /*:  5; mem_opcd */
         int ea0bs,  /*:  2; 0:ea0br, 1:ea0dr(ea0br+self-loop), 2:eabbrs, 3:ea0dr(eabbrs+self-loop) */
         int ea0os,  /*:  1; 0:ea0or, 1:eaobrs */
         int ea0msk, /*:  4; 14:64bit, 13:word1, 12:word0, 11-8:half3-0, 7-0:byte7-0 of offset */
         int eabbrs, /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
         int eaobrs, /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
	 int mex0op,
	 int mex0init,
	 int mex0dist,
	 int mexlimit
         )
{
#define E0LABELMAX 80
  char opcd[E0LABELMAX];
  int col, thi;
  int ea0singleload = (ea0op && (ea0op <= OP_LDBR));

  /* EA0 */
  if (ea0op&0x10) {
    col=1; /*red*/
    thi=3;
  }
  else if (ea0op) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line(e0b_x+10,  e0b_y-10, e0b_x+30,  e0b_y-10, thi, col);
  draw_line(e0b_x+70,  e0b_y-10, e0b_x+90,  e0b_y-10, thi, col);
  draw_line(e0b_x,     e0b_y,    e0b_x+100, e0b_y,    thi, col);
  draw_line(e0b_x+100, e0b_y,    e0b_x+80,  e0b_y+20, thi, col);
  draw_line(e0b_x+80,  e0b_y+20, e0b_x+20,  e0b_y+20, thi, col);
  draw_line(e0b_x+20,  e0b_y+20, e0b_x,     e0b_y,    thi, col);
  snprintf(opcd, E0LABELMAX, "%02.2x", ea0op);
  draw_text(e0b_x+35, e0b_y+15, opcd, 1, col);
  draw_line (e0b_x+50, e0b_y+20,              e0b_x+50, e0b_y+180,     thi, col);
  draw_box  (e0b_x+40, e0b_y+180,             e0b_x+60, e0b_y+190,     thi, col);
  draw_line (e0b_x+50, e0b_y+190,             e0b_x+50, e0b_y+210,     thi, col);
  draw_box  (e0b_x+40, e0b_y+210,             e0b_x+60, e0b_y+220,     thi, col);
  if (ea0singleload) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line (e0b_x+55, e0b_y+190,             e0b_x+55, e0b_y+340,     thi, col);
  draw_arrow(e0b_x+55, e0b_y+340,             e0b_x+65, e0b_y+340,     thi, col);
  draw_line (e0b_x+65, e0b_y+340,             e0b_x+100,e0b_y+340,     thi, col);

  /* SRC1 */
  if (ea0op && !(ea0bs & 2)) { /* ea0br */
    col=1; /*red*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (e0b_x,    e0b_y-40, e0b_x+40, e0b_y-30, thi, col);
  draw_arrow(e0b_x+20, e0b_y-30, e0b_x+20, e0b_y-10, thi, col);
  if (ea0op && (ea0bs & 2)) { /* eabbrs */
    col=1; /*red*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line (e0b_x-30, e0b_y-170+eabbrs*5+10, e0b_x-30, e0b_y-70, thi, col);
  draw_line (e0b_x-30, e0b_y-70,              e0b_x+30, e0b_y-70, thi, col);
  draw_arrow(e0b_x+30, e0b_y-70,              e0b_x+30, e0b_y-10, thi, col);

  /* feedback-loop */
  if (ea0bs & 1) {
    col=1; /*red*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line (e0b_x+50, e0b_y+190,             e0b_x+50, e0b_y+200,     thi, col);
  draw_line (e0b_x+50, e0b_y+200,             e0b_x+20, e0b_y+200,     thi, col);
  draw_arrow(e0b_x+20, e0b_y+200,             e0b_x+20, e0b_y-10,      thi, col);

  /* mex-loop */
  if (mex0op==OP_CMPA_LE || mex0op==OP_CMPA_GE) {
    col=2; /*green*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line (e0b_x+65, e0b_y+360,             e0b_x+80, e0b_y+360,     thi, col);
  draw_arrow(e0b_x+65, e0b_y+360,             e0b_x+65, e0b_y-40,      thi, col);
  snprintf(opcd, E0LABELMAX, "%s-%d-%d-%d", mex0op==OP_ALWAYS?"AL":mex0op==OP_CMPA_LE?"LE":mex0op==OP_CMPA_GE?"GE":"NA", mex0init, mex0dist, mexlimit);
  draw_text(e0b_x+30, e0b_y-40, opcd, 1, 0);

  /* SRC2 */
  if (ea0op && !(ea0os & 1)) { /* ea0or */
    col=1; /*red*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (e0b_x+60, e0b_y-40, e0b_x+100, e0b_y-30, thi, col);
  draw_arrow(e0b_x+80, e0b_y-30, e0b_x+80,  e0b_y-10, thi, col);
  if (ea0op && (ea0os & 1)) { /* eaobrs */
    col=1; /*red*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line (e0b_x+90, e0b_y-170+eaobrs*5+10, e0b_x+90, e0b_y-70, thi, col);
  draw_arrow(e0b_x+90, e0b_y-70,              e0b_x+90, e0b_y-10, thi, col);
}

draw_ea1(int e1b_x, int e1b_y,
         int ea1op,  /*:  5; mem_opcd */
         int ea1bs,  /*:  2; 0:ea1br, 1:ea1dr(ea1br+self-loop), 2:eabbrs, 3:ea1dr(self-loop) */
         int ea1os,  /*:  1; 0:ea1or, 1:eaobrs */
         int ea1msk, /*:  4; 14:64bit, 13:word1, 12:word0, 11-8:half3-0, 7-0:byte7-0 of offset */
         int eabbrs, /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
         int eaobrs, /*:  4; 0:br0_0, 1:br0_1, ... 15:3_3 */
	 int mex1op,
	 int mex1init,
	 int mex1dist,
	 int mexlimit
         )
{
#define E1LABELMAX 80
  char opcd[E1LABELMAX];
  int col, thi;
  int ea1singleload = (ea1op && (ea1op <= OP_LDBR));

  /* EA1 */
  if (ea1op) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line(e1b_x+10,  e1b_y-10, e1b_x+30,  e1b_y-10, thi, col);
  draw_line(e1b_x+70,  e1b_y-10, e1b_x+90,  e1b_y-10, thi, col);
  draw_line(e1b_x,     e1b_y,    e1b_x+100, e1b_y,    thi, col);
  draw_line(e1b_x+100, e1b_y,    e1b_x+80,  e1b_y+20, thi, col);
  draw_line(e1b_x+80,  e1b_y+20, e1b_x+20,  e1b_y+20, thi, col);
  draw_line(e1b_x+20,  e1b_y+20, e1b_x,     e1b_y,    thi, col);
  snprintf(opcd, E1LABELMAX, "%02.2x", ea1op);
  draw_text(e1b_x+35, e1b_y+15, opcd, 1, col);
  draw_line (e1b_x+50, e1b_y+20,              e1b_x+50, e1b_y+180,     thi, col);
  draw_box  (e1b_x+40, e1b_y+180,             e1b_x+60, e1b_y+190,     thi, col);
  draw_line (e1b_x+50, e1b_y+190,             e1b_x+50, e1b_y+210,     thi, col);
  draw_box  (e1b_x+40, e1b_y+210,             e1b_x+60, e1b_y+220,     thi, col);
  if (ea1singleload) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line (e1b_x+55, e1b_y+190,             e1b_x+55, e1b_y+340,     thi, col);
  draw_arrow(e1b_x+55, e1b_y+340,             e1b_x+65, e1b_y+340,     thi, col);
  draw_line (e1b_x+65, e1b_y+340,             e1b_x+100,e1b_y+340,     thi, col);

  /* SRC1 */
  if (ea1op && !(ea1bs & 2)) { /* ea1br */
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (e1b_x,    e1b_y-40, e1b_x+40, e1b_y-30, thi, col);
  draw_arrow(e1b_x+20, e1b_y-30, e1b_x+20, e1b_y-10, thi, col);
  if (ea1op && (ea1bs & 2)) { /* eabbrs */
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line (e1b_x+90, e1b_y-170+eabbrs*5+10, e1b_x+90, e1b_y-70, thi, col);
  draw_line (e1b_x+90, e1b_y-70,              e1b_x+30, e1b_y-70, thi, col);
  draw_arrow(e1b_x+30, e1b_y-70,              e1b_x+30, e1b_y-10, thi, col);

  /* feedback-loop */
  if (ea1bs & 1) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line (e1b_x+50, e1b_y+190,             e1b_x+50, e1b_y+200,     thi, col);
  draw_line (e1b_x+50, e1b_y+200,             e1b_x+20, e1b_y+200,     thi, col);
  draw_arrow(e1b_x+20, e1b_y+200,             e1b_x+20, e1b_y-10,      thi, col);

  /* mex-loop */
  if (mex1op==OP_CMPA_LE || mex1op==OP_CMPA_GE) {
    col=2; /*green*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line (e1b_x+65, e1b_y+360,             e1b_x+80, e1b_y+360,     thi, col);
  draw_arrow(e1b_x+65, e1b_y+360,             e1b_x+65, e1b_y-40,      thi, col);
  snprintf(opcd, E1LABELMAX, "%s-%d-%d-%d", mex1op==OP_ALWAYS?"AL":mex1op==OP_CMPA_LE?"LE":mex1op==OP_CMPA_GE?"GE":"NA", mex1init, mex1dist, mexlimit);
  draw_text(e1b_x+30, e1b_y-40, opcd, 1, 0);

  /* SRC2 */
  if (ea1op && !(ea1os & 1)) { /* ea1or */
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (e1b_x+60, e1b_y-40, e1b_x+100, e1b_y-30, thi, col);
  draw_arrow(e1b_x+80, e1b_y-30, e1b_x+80,  e1b_y-10, thi, col);
  if (ea1op && (ea1os & 1)) { /* eaobrs */
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_line (e1b_x+210, e1b_y-170+eaobrs*5+10, e1b_x+210, e1b_y-60, thi, col);
  draw_line (e1b_x+210, e1b_y-60,              e1b_x+90,  e1b_y-60, thi, col);
  draw_arrow(e1b_x+90,  e1b_y-60,              e1b_x+90,  e1b_y-10, thi, col);
}

draw_trx(int trb_x, int trb_y,
         int tr0v,
         int tr1v,
         int tr2v,
         int tr3v,
         int ts0,    /*:  4; 0:br0_0, 1:br0_1, ... 15:br3_3 */
         int ts1,    /*:  4; 0:br0_0, 1:br0_1, ... 15:br3_3 */
         int ts2,    /*:  4; 0:br0_0, 1:br0_1, ... 15:br3_3 */
         int ts3,    /*:  4; 0:br0_0, 1:br0_1, ... 15:br3_3 */
         int trs0,   /*:  2; 0:lmwd0, 1:exdr, 2:ts0 */
         int trs1,   /*:  2; 0:lmwd1, 1:exdr, 2:ts1 */
         int trs2,   /*:  2; 0:lmwd2, 1:exdr, 2:ts2 */
         int trs3    /*:  2; 0:lmwd3, 1:exdr, 2:ts3 */
         )
{
#define TRLABELMAX 30
  char opcd[TRLABELMAX];
  int col, thi;

  /* TR0 */
  if (tr0v) { col=2; /*green*/ thi=3; }
  else      { col=0; /*black*/ thi=1; }
  draw_box  (trb_x+360,   trb_y,              trb_x+380, trb_y+10,  thi, col);
  if (tr0v && trs0==0) { col=2; /*green*/ thi=3; } /* lmwd */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x+380,   trb_y-80,           trb_x+380, trb_y,     thi, col);
  if (tr0v && trs0==1) { col=2; /*green*/ thi=3; } /* exdr */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x+360,   trb_y-120,          trb_x+360, trb_y,     thi, col);
  if (tr0v && trs0==2) { col=2; /*green*/ thi=3; } /* ts */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x+370,   trb_y-350+ts0*5+10, trb_x+370, trb_y,     thi, col);

  /* TR1 */
  if (tr1v) { col=2; /*green*/ thi=3; }
  else      { col=0; /*black*/ thi=1; }
  draw_box  (trb_x+240,   trb_y,              trb_x+260, trb_y+10,  thi, col);
  if (tr1v && trs1==0) { col=2; /*green*/ thi=3; } /* lmwd */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x+260,   trb_y-80,           trb_x+260, trb_y,     thi, col);
  if (tr1v && trs1==1) { col=2; /*green*/ thi=3; } /* exdr */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x+240,   trb_y-120,          trb_x+240, trb_y,     thi, col);
  if (tr1v && trs1==2) { col=2; /*green*/ thi=3; } /* ts */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x+250,   trb_y-350+ts1*5+10, trb_x+250, trb_y,     thi, col);

  /* TR2 */
  if (tr2v) { col=2; /*green*/ thi=3; }
  else      { col=0; /*black*/ thi=1; }
  draw_box  (trb_x+120,   trb_y,              trb_x+140, trb_y+10,  thi, col);
  if (tr2v && trs2==0) { col=2; /*green*/ thi=3; } /* lmwd */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x+140,   trb_y-80,           trb_x+140, trb_y,     thi, col);
  if (tr2v && trs2==1) { col=2; /*green*/ thi=3; } /* exdr */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x+120,   trb_y-120,          trb_x+120, trb_y,     thi, col);
  if (tr2v && trs2==2) { col=2; /*green*/ thi=3; } /* ts */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x+130,   trb_y-350+ts2*5+10, trb_x+130, trb_y,     thi, col);

  /* TR3 */
  if (tr3v) { col=2; /*green*/ thi=3; }
  else      { col=0; /*black*/ thi=1; }
  draw_box  (trb_x,       trb_y,              trb_x+20,  trb_y+10,  thi, col);
  if (tr3v && trs3==0) { col=2; /*green*/ thi=3; } /* lmwd */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x+20,    trb_y-80,           trb_x+20,  trb_y,     thi, col);
  if (tr3v && trs3==1) { col=2; /*green*/ thi=3; } /* exdr */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x,       trb_y-120,          trb_x,     trb_y,     thi, col);
  if (tr3v && trs3==2) { col=2; /*green*/ thi=3; } /* ts */
  else                 { col=0; /*black*/ thi=1; }
  draw_arrow(trb_x+10,    trb_y-350+ts3*5+10, trb_x+10,  trb_y,     thi, col);
}

draw_lmx(int lmb_x, int lmb_y,
	 int lmmi_v,
         int ea0op,   /* MWSAに加え,LDDMQおよびSTOREの場合にもMW有効 */
         int mwsa,    /*:  1; 0:lmwa,  1:ea0d        */
         int mws0,    /*:  2; 0:lmwd0, 1:exdr, 2:ts0 */
         int mws1,    /*:  2; 0:lmwd1, 1:exdr, 2:ts1 */
         int mws2,    /*:  2; 0:lmwd2, 1:exdr, 2:ts2 */
         int mws3,    /*:  2; 0:lmwd3, 1:exdr, 2:ts3 */
	 int ts0,
	 int ts1,
	 int ts2,
	 int ts3,
	 int lmm_mode,/*:  2; 0:無効, 1:分割無, 2:2分割, 3:4分割 */
	 Ull top
         )
{
#define LMLABELMAX 30
  char opcd[LMLABELMAX];
  int col, thi;
  int ea0store = lmmi_v && ((ea0op&0x10)||mwsa==0);

  /* MWA */
  if (ea0store) {
    col=1; /*red*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_arrow(lmb_x+340,   lmb_y-110, lmb_x+340, lmb_y-30,  thi, col);

  /* MW0 */
  if (ea0store) {
    col=1; /*red*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (lmb_x+360,   lmb_y,     lmb_x+420, lmb_y+10,  thi, col);
  draw_box  (lmb_x+360,   lmb_y+20,  lmb_x+420, lmb_y+30,  thi, col);
  draw_box  (lmb_x+360,   lmb_y+30,  lmb_x+420, lmb_y+40,  thi, col);
  draw_box  (lmb_x+360,   lmb_y+40,  lmb_x+420, lmb_y+50,  1, 0);
  draw_box  (lmb_x+360,   lmb_y+50,  lmb_x+420, lmb_y+60,  1, 0);
  draw_line (lmb_x+365,   lmb_y-60,  lmb_x+405, lmb_y-60,  thi, col);
  draw_line (lmb_x+370,   lmb_y-60,  lmb_x+370, lmb_y,     thi, col);
  if (ea0store && mws0==0) { col=1; /*red*/   thi=3; } /* lmwd0 */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+400,   lmb_y-110, lmb_x+400, lmb_y-60,  thi, col);
  if (ea0store && mws0==1) { col=1; /*red*/   thi=3; } /* exdr */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+380,   lmb_y-150, lmb_x+380, lmb_y-60,  thi, col);
  if (ea0store && mws0==2) { col=1; /*red*/   thi=3; } /* ts0 */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+390,   lmb_y-370+ts0*5, lmb_x+390, lmb_y-60,  thi, col);

  /* MW1 */
  if (ea0store) {
    col=1; /*red*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (lmb_x+240,   lmb_y,     lmb_x+300, lmb_y+10,  thi, col);
  draw_box  (lmb_x+240,   lmb_y+20,  lmb_x+300, lmb_y+30,  thi, col);
  draw_box  (lmb_x+240,   lmb_y+30,  lmb_x+300, lmb_y+40,  thi, col);
  draw_box  (lmb_x+240,   lmb_y+40,  lmb_x+300, lmb_y+50,  1, 0);
  draw_box  (lmb_x+240,   lmb_y+50,  lmb_x+300, lmb_y+60,  1, 0);
  draw_line (lmb_x+245,   lmb_y-60,  lmb_x+285, lmb_y-60,  thi, col);
  draw_line (lmb_x+250,   lmb_y-60,  lmb_x+250, lmb_y,     thi, col);
  if (ea0store && mws1==0) { col=1; /*red*/   thi=3; } /* lmwd1 */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+280,   lmb_y-110, lmb_x+280, lmb_y-60,  thi, col);
  if (ea0store && mws1==1) { col=1; /*red*/   thi=3; } /* exdr */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+260,   lmb_y-150, lmb_x+260, lmb_y-60,  thi, col);
  if (ea0store && mws1==2) { col=1; /*red*/   thi=3; } /* ts1 */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+270,   lmb_y-370+ts1*5, lmb_x+270, lmb_y-60,  thi, col);

  /* MW2 */
  if (ea0store) {
    col=1; /*red*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (lmb_x+120,   lmb_y,     lmb_x+180, lmb_y+10,  thi, col);
  draw_box  (lmb_x+120,   lmb_y+20,  lmb_x+180, lmb_y+30,  thi, col);
  draw_box  (lmb_x+120,   lmb_y+30,  lmb_x+180, lmb_y+40,  thi, col);
  draw_box  (lmb_x+120,   lmb_y+40,  lmb_x+180, lmb_y+50,  1, 0);
  draw_box  (lmb_x+120,   lmb_y+50,  lmb_x+180, lmb_y+60,  1, 0);
  draw_line (lmb_x+125,   lmb_y-60,  lmb_x+165, lmb_y-60,  thi, col);
  draw_line (lmb_x+130,   lmb_y-60,  lmb_x+130, lmb_y,     thi, col);
  if (ea0store && mws2==0) { col=1; /*red*/   thi=3; } /* lmwd2 */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+160,   lmb_y-110, lmb_x+160, lmb_y-60,  thi, col);
  if (ea0store && mws2==1) { col=1; /*red*/   thi=3; } /* exdr */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+140,   lmb_y-150, lmb_x+140, lmb_y-60,  thi, col);
  if (ea0store && mws2==2) { col=1; /*red*/   thi=3; } /* ts2 */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+150,   lmb_y-370+ts2*5, lmb_x+150, lmb_y-60,  thi, col);

  /* MW3 */
  if (ea0store) {
    col=1; /*red*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (lmb_x,       lmb_y,     lmb_x+60,  lmb_y+10,  thi, col);
  draw_box  (lmb_x,       lmb_y+20,  lmb_x+60,  lmb_y+30,  thi, col);
  draw_box  (lmb_x,       lmb_y+30,  lmb_x+60,  lmb_y+40,  thi, col);
  draw_box  (lmb_x,       lmb_y+40,  lmb_x+60,  lmb_y+50,  1, 0);
  draw_box  (lmb_x,       lmb_y+50,  lmb_x+60,  lmb_y+60,  1, 0);
  draw_line (lmb_x+5,     lmb_y-60,  lmb_x+45,  lmb_y-60,  thi, col);
  draw_line (lmb_x+10,    lmb_y-60,  lmb_x+10,  lmb_y,     thi, col);
  if (ea0store && mws3==0) { col=1; /*red*/   thi=3; } /* lmwd3 */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+40,    lmb_y-110, lmb_x+40 , lmb_y-60,  thi, col);
  if (ea0store && mws3==1) { col=1; /*red*/   thi=3; } /* exdr */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+20,    lmb_y-150, lmb_x+20,  lmb_y-60,  thi, col);
  if (ea0store && mws3==2) { col=1; /*red*/   thi=3; } /* ts3 */
  else                     { col=0; /*black*/ thi=1; }
  draw_arrow(lmb_x+30,    lmb_y-370+ts3*5, lmb_x+30,  lmb_y-60,  thi, col);

  switch (lmm_mode) {
  case 0:
    draw_text(lmb_x+405, lmb_y-25, "M0(N/A)", 1, 0);
    break;
  case 1:
    draw_text(lmb_x+405, lmb_y-25, "M1(512KB)", 1, 0);
    break;
  case 2:
    draw_text(lmb_x+405, lmb_y-25, "M2(256KB)", 1, 0);
    break;
  case 3:
    draw_text(lmb_x+405, lmb_y-25, "M3(128KB)", 1, 0);
    break;
  }
  draw_text(lmb_x+405, lmb_y-5, (char*)top, 1, 0);
}

draw_bri(int bri_x, int bri_y,
         int ea0op,  /*:  5; mem_opcd */
         int ea1op,  /*:  5; mem_opcd */
         int brs0,   /*:  2; 0:off, 1:mr10, 2:tr0, 3:mr0  */
         int brs1,   /*:  2; 0:off, 1:mr11, 2:tr1, 3:mr1  */
         int brs2,   /*:  2; 0:off, 1:mr12, 2:tr2, 3:exd(brs3=3の場合,ea0woofsに接続) */
         int brs3,   /*:  2; 0:off, 1:mr13, 2:tr3  3:ea1woofs */
         int br0v,
         int br0h,
         int br0s,
         int br1v,
         int br1h,
         int br1s,
         int br2v,
         int br2h,
         int br2s,
         int br3v,
         int br3h,
         int br3s
         )
{
#define BRLABELMAX 30
  char opcd[BRLABELMAX];
  int col, thi;
  int ea0singleload = (ea0op && (ea0op <= OP_LDBR));
  int ea1singleload = (ea1op && (ea1op <= OP_LDBR));

  /* BR0 */
  if (brs0) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (bri_x+360,   bri_y,     bri_x+420, bri_y+15,  thi, col);
  if (brs0==1) { col=4; thi=3; } /* mr10 */
  else         { col=0; thi=1; }
  if (col) {
    draw_box(bri_x+360,   bri_y-110, bri_x+420, bri_y-100, thi, col);
    draw_box(bri_x+360,   bri_y-100, bri_x+420, bri_y-90,  thi, col);
  }
  draw_arrow(bri_x+400,   bri_y-90,  bri_x+400, bri_y,     thi, col);
  if (brs0==2) { col=4; thi=3; } /* tr0 */
  else         { col=0; thi=1; }
  draw_arrow(bri_x+390,   bri_y-170, bri_x+390, bri_y,     thi, col);
  if (brs0==3) { col=4; thi=3; } /* mr0 */
  else         { col=0; thi=1; }
  if (col) {
    draw_box(bri_x+360,   bri_y-110, bri_x+420, bri_y-100, thi, col);
    draw_box(bri_x+360,   bri_y-100, bri_x+420, bri_y-90,  thi, col);
    draw_box(bri_x+240,   bri_y-110, bri_x+300, bri_y-100, thi, col);
    draw_box(bri_x+240,   bri_y-100, bri_x+300, bri_y-90,  thi, col);
    draw_box(bri_x+120,   bri_y-110, bri_x+180, bri_y-100, thi, col);
    draw_box(bri_x+120,   bri_y-100, bri_x+180, bri_y-90,  thi, col);
    draw_box(bri_x,       bri_y-110, bri_x+60,  bri_y-100, thi, col);
    draw_box(bri_x,       bri_y-100, bri_x+60,  bri_y-90,  thi, col);
  }
  draw_line (bri_x+410,   bri_y-90,  bri_x+410, bri_y-80,  thi, col);
  draw_line (bri_x+410,   bri_y-80,  bri_x+380, bri_y-80,  thi, col);
  draw_arrow(bri_x+380,   bri_y-80,  bri_x+380, bri_y-20,  thi, col);
  draw_line (bri_x+290,   bri_y-90,  bri_x+290, bri_y-80,  thi, col);
  draw_line (bri_x+290,   bri_y-80,  bri_x+370, bri_y-80,  thi, col);
  draw_arrow(bri_x+370,   bri_y-80,  bri_x+370, bri_y-20,  thi, col);
  draw_line (bri_x+170,   bri_y-90,  bri_x+170, bri_y-70,  thi, col);
  draw_line (bri_x+170,   bri_y-70,  bri_x+360, bri_y-70,  thi, col);
  draw_arrow(bri_x+360,   bri_y-70,  bri_x+360, bri_y-20,  thi, col);
  draw_line (bri_x+50,    bri_y-90,  bri_x+50,  bri_y-60,  thi, col);
  draw_line (bri_x+50,    bri_y-60,  bri_x+350, bri_y-60,  thi, col);
  draw_arrow(bri_x+350,   bri_y-60,  bri_x+350, bri_y-20,  thi, col);
  draw_arrow(bri_x+380,   bri_y-20,  bri_x+380, bri_y,     thi, col);
  switch (br0v) {
  case 0: /* T_NONE */
    col=0; /*black*/
    thi=1;
    break;
  case 1: /* T_IMMEDIATE */
    col=4; /*blue*/
    thi=3;
    break;
  case 2: /* T_EXRNO */
    col=5; /*magenta*/
    thi=3;
    break;
  case 3: /* T_ALRNO */
    col=3; /*yellow*/
    thi=3;
    break;
  case 4: /* T_BDRNO */
    col=1; /*red*/
    thi=3;
    break;
  case 7: /* T_VARIABLE */
    col=2; /*green*/
    thi=3;
    break;
  default:
    col=6; /*cyan*/
    thi=3;
    break;
  }
  if (col) {
    draw_box (bri_x+360,   bri_y,    bri_x+420, bri_y+15,  thi, col);
    if (br0s == -1)
      snprintf(opcd, BRLABELMAX, "%s", id[br0h].name);
    else
      snprintf(opcd, BRLABELMAX, "%s[%d]", id[br0h].name, br0s);
    draw_text(bri_x+365,   bri_y+14, opcd, 1, 0);
  }

  /* BR1 */
  if (brs1) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (bri_x+240,   bri_y,     bri_x+300, bri_y+15,  thi, col);
  if (brs1==1) { col=4; thi=3; } /* mr11 */
  else         { col=0; thi=1; }
  if (col) {
    draw_box(bri_x+240,   bri_y-110, bri_x+300, bri_y-100, thi, col);
    draw_box(bri_x+240,   bri_y-100, bri_x+300, bri_y-90,  thi, col);
  }
  draw_arrow(bri_x+280,   bri_y-90,  bri_x+280, bri_y,     thi, col);
  if (brs1==2) { col=4; thi=3; } /* tr1 */
  else         { col=0; thi=1; }
  draw_arrow(bri_x+270,   bri_y-170, bri_x+270, bri_y,     thi, col);
  if (brs1==3) { col=4; thi=3; } /* mr1 */
  else         { col=0; thi=1; }
  if (col) {
    draw_box(bri_x+360,   bri_y-110, bri_x+420, bri_y-100, thi, col);
    draw_box(bri_x+360,   bri_y-100, bri_x+420, bri_y-90,  thi, col);
    draw_box(bri_x+240,   bri_y-110, bri_x+300, bri_y-100, thi, col);
    draw_box(bri_x+240,   bri_y-100, bri_x+300, bri_y-90,  thi, col);
    draw_box(bri_x+120,   bri_y-110, bri_x+180, bri_y-100, thi, col);
    draw_box(bri_x+120,   bri_y-100, bri_x+180, bri_y-90,  thi, col);
    draw_box(bri_x,       bri_y-110, bri_x+60,  bri_y-100, thi, col);
    draw_box(bri_x,       bri_y-100, bri_x+60,  bri_y-90,  thi, col);
  }
  draw_line (bri_x+400,   bri_y-40,  bri_x+260, bri_y-40,  thi, col);
  draw_arrow(bri_x+260,   bri_y-40,  bri_x+260, bri_y-20,  thi, col);
  draw_line (bri_x+280,   bri_y-50,  bri_x+250, bri_y-50,  thi, col);
  draw_arrow(bri_x+250,   bri_y-50,  bri_x+250, bri_y-20,  thi, col);
  draw_line (bri_x+160,   bri_y-50,  bri_x+240, bri_y-50,  thi, col);
  draw_arrow(bri_x+240,   bri_y-50,  bri_x+240, bri_y-20,  thi, col);
  draw_line (bri_x+40,    bri_y-40,  bri_x+230, bri_y-40,  thi, col);
  draw_arrow(bri_x+230,   bri_y-40,  bri_x+230, bri_y-20,  thi, col);
  draw_arrow(bri_x+260,   bri_y-20,  bri_x+260, bri_y,     thi, col);
  switch (br1v) {
  case 0: /* T_NONE */
    col=0; /*black*/
    thi=1;
    break;
  case 1: /* T_IMMEDIATE */
    col=4; /*blue*/
    thi=3;
    break;
  case 2: /* T_EXRNO */
    col=5; /*magenta*/
    thi=3;
    break;
  case 3: /* T_ALRNO */
    col=3; /*yellow*/
    thi=3;
    break;
  case 4: /* T_BDRNO */
    col=1; /*red*/
    thi=3;
    break;
  case 7: /* T_VARIABLE */
    col=2; /*green*/
    thi=3;
    break;
  default:
    col=6; /*cyan*/
    thi=3;
    break;
  }
  if (col) {
    draw_box (bri_x+240,   bri_y,    bri_x+300, bri_y+15,  thi, col);
    if (br1s == -1)
      snprintf(opcd, BRLABELMAX, "%s", id[br1h].name);
    else
      snprintf(opcd, BRLABELMAX, "%s[%d]", id[br1h].name, br1s);
    draw_text(bri_x+245,   bri_y+14, opcd, 1, 0);
  }

  /* BR2 */
  if (brs2) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (bri_x+120,   bri_y,     bri_x+180, bri_y+15,  thi, col);
  if (brs2==1) { col=4; thi=3; } /* mr12 */
  else         { col=0; thi=1; }
  if (col) {
    draw_box(bri_x+120,   bri_y-110, bri_x+180, bri_y-100, thi, col);
    draw_box(bri_x+120,   bri_y-100, bri_x+180, bri_y-90,  thi, col);
  }
  draw_arrow(bri_x+160,   bri_y-90,  bri_x+160, bri_y,     thi, col);
  if (brs2==2) { col=4; thi=3; } /* tr2 */
  else         { col=0; thi=1; }
  draw_arrow(bri_x+150,   bri_y-170, bri_x+150, bri_y,     thi, col);
  if (brs2==3 && brs3!=3) { col=5; thi=3; } /* exdr */
  else                    { col=0; thi=1; }
  draw_line (bri_x+90,    bri_y-170, bri_x+90,  bri_y-20,  thi, col);
  draw_line (bri_x+90,    bri_y-20,  bri_x+140, bri_y-20,  thi, col);
  draw_arrow(bri_x+140,   bri_y-20,  bri_x+140, bri_y,     thi, col);
  if (brs2==3 && brs3==3) { col=2; thi=3; /* ea0woofs */
  draw_arrow(bri_x+330,   bri_y-140,  bri_x+170, bri_y,     thi, col);
  }
  switch (br2v) {
  case 0: /* T_NONE */
    col=0; /*black*/
    thi=1;
    break;
  case 1: /* T_IMMEDIATE */
    col=4; /*blue*/
    thi=3;
    break;
  case 2: /* T_EXRNO */
    col=5; /*magenta*/
    thi=3;
    break;
  case 3: /* T_ALRNO */
    col=3; /*yellow*/
    thi=3;
    break;
  case 4: /* T_BDRNO */
    col=1; /*red*/
    thi=3;
    break;
  case 7: /* T_VARIABLE */
    col=2; /*green*/
    thi=3;
    break;
  default:
    col=6; /*cyan*/
    thi=3;
    break;
  }
  if (col) {
    draw_box (bri_x+120,   bri_y,    bri_x+180, bri_y+15,  thi, col);
    if (br2s == -1)
      snprintf(opcd, BRLABELMAX, "%s", id[br2h].name);
    else
      snprintf(opcd, BRLABELMAX, "%s[%d]", id[br2h].name, br2s);
    draw_text(bri_x+125,   bri_y+14, opcd, 1, 0);
  }

  /* BR3 */
  if (brs3) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (bri_x,       bri_y,     bri_x+60,  bri_y+15,  thi, col);
  if (brs3==1) { col=4; thi=3; } /* mr13 */
  else         { col=0; thi=1; }
  if (col) {
    draw_box(bri_x,       bri_y-110, bri_x+60,  bri_y-100, thi, col);
    draw_box(bri_x,       bri_y-100, bri_x+60,  bri_y-90,  thi, col);
  }
  draw_arrow(bri_x+40,    bri_y-90,  bri_x+40,  bri_y,     thi, col);
  if (brs3==2) { col=4; thi=3; } /* tr3 */
  else         { col=0; thi=1; }
  draw_arrow(bri_x+30,    bri_y-170, bri_x+30,  bri_y,     thi, col);
  if (brs3==3) { col=2; thi=3; /* ea1woofs */
  draw_arrow(bri_x+210,   bri_y-140, bri_x+50,  bri_y,     thi, col);
  }
  switch (br3v) {
  case 0: /* T_NONE */
    col=0; /*black*/
    thi=1;
    break;
  case 1: /* T_IMMEDIATE */
    col=4; /*blue*/
    thi=3;
    break;
  case 2: /* T_EXRNO */
    col=5; /*magenta*/
    thi=3;
    break;
  case 3: /* T_ALRNO */
    col=3; /*yellow*/
    thi=3;
    break;
  case 4: /* T_BDRNO */
    col=1; /*red*/
    thi=3;
    break;
  case 7: /* T_VARIABLE */
    col=2; /*green*/
    thi=3;
    break;
  default:
    col=6; /*cyan*/
    thi=3;
    break;
  }
  if (col) {
    draw_box (bri_x,       bri_y,    bri_x+60,  bri_y+15,  thi, col);
    if (br3s == -1)
      snprintf(opcd, BRLABELMAX, "%s", id[br3h].name);
    else
      snprintf(opcd, BRLABELMAX, "%s[%d]", id[br3h].name, br3s);
    draw_text(bri_x+5,     bri_y+14, opcd, 1, 0);
  }
}

draw_lmr(int bri_x, int bri_y,
         int lmrd0,
         int lmrd1,
         int lmrd2,
         int lmrd3
         )
{
  int col, thi;

  /* LMRA */
  if (lmrd0 || lmrd1 || lmrd2 || lmrd3) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_arrow(bri_x+220,  bri_y-260, bri_x+220, bri_y-180,  thi, col);
  draw_box  (bri_x+200,  bri_y-180, bri_x+220, bri_y-170,  thi, col);
  draw_line (bri_x+210,  bri_y-170, bri_x+210, bri_y-150,  thi, col);
  draw_box  (bri_x+200,  bri_y-150, bri_x+220, bri_y-140,  thi, col);

  /* LMRD0 */
  if (lmrd0) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (bri_x+360,   bri_y-110, bri_x+420, bri_y-100, thi, col);
  draw_box  (bri_x+360,   bri_y-100, bri_x+420, bri_y-90,  thi, col);
  draw_line (bri_x+400,   bri_y-90,  bri_x+400, bri_y-20,  thi, col);
  draw_arrow(bri_x+400,   bri_y-20,  bri_x+420, bri_y-20,  thi, col);

  /* LMRD1 */
  if (lmrd1) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (bri_x+240,   bri_y-110, bri_x+300, bri_y-100, thi, col);
  draw_box  (bri_x+240,   bri_y-100, bri_x+300, bri_y-90,  thi, col);
  draw_line (bri_x+280,   bri_y-90,  bri_x+280, bri_y-20,  thi, col);
  draw_arrow(bri_x+280,   bri_y-20,  bri_x+300, bri_y-20,  thi, col);

  /* LMRD2 */
  if (lmrd2) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (bri_x+120,   bri_y-110, bri_x+180, bri_y-100, thi, col);
  draw_box  (bri_x+120,   bri_y-100, bri_x+180, bri_y-90,  thi, col);
  draw_line (bri_x+160,   bri_y-90,  bri_x+160, bri_y-20,  thi, col);
  draw_arrow(bri_x+160,   bri_y-20,  bri_x+180, bri_y-20,  thi, col);

  /* LMRD3 */
  if (lmrd3) {
    col=4; /*blue*/
    thi=3;
  }
  else {
    col=0; /*black*/
    thi=1;
  }
  draw_box  (bri_x,       bri_y-110, bri_x+60,  bri_y-100, thi, col);
  draw_box  (bri_x,       bri_y-100, bri_x+60,  bri_y-90,  thi, col);
  draw_line (bri_x+40,    bri_y-90,  bri_x+40,  bri_y-20,  thi, col);
  draw_arrow(bri_x+40,    bri_y-20,  bri_x+60,  bri_y-20,  thi, col);
}

draw_box(int x0, int y0, int x1, int y1, int thick, int color)
{
  char *cstr[] = { "'black'", "'red'", "'green'", "'yellow'", "'blue'", "'magenta'", "'cyan'", "'white'"};

  if (thick>7) thick=7;
  if (color>7) color=7;

  fprintf(ffile, "box(%s,'',%d,%d,%d,%d,0,%d,1,0,0,0,0,0,0,'%d',0,[\n", cstr[color], x0, y0, x1, y1, thick, thick);
  fprintf(ffile, "]).\n");
}

draw_line(int x0, int y0, int x1, int y1, int thick, int color)
{
  char *cstr[] = { "'black'", "'red'", "'green'", "'yellow'", "'blue'", "'magenta'", "'cyan'", "'white'"};

  if (thick>7) thick=7;
  if (color>7) color=7;

  fprintf(ffile, "poly(%s,'',2,[\n", cstr[color]);
  fprintf(ffile, "%d,%d,%d,%d],0,%d,1,1,0,0,0,0,0,0,0,'%d',0,0,\n", x0, y0, x1, y1, thick, thick);
  fprintf(ffile, "\"0\",\"\",[\n");
  fprintf(ffile, "0,12,5,0,'12','5','0'],[0,12,5,0,'12','5','0'],[\n");
  fprintf(ffile, "]).\n");
}

draw_arrow(int x0, int y0, int x1, int y1, int thick, int color)
{
  char *cstr[] = { "'black'", "'red'", "'green'", "'yellow'", "'blue'", "'magenta'", "'cyan'", "'white'"};

  if (thick>7) thick=7;
  if (color>7) color=7;

  fprintf(ffile, "poly(%s,'',2,[\n", cstr[color]);
  fprintf(ffile, "%d,%d,%d,%d],1,%d,1,1,0,0,0,0,0,0,0,'%d',0,0,\n", x0, y0, x1, y1, thick, thick);
  fprintf(ffile, "\"0\",\"\",[\n");
  fprintf(ffile, "0,12,5,0,'12','5','0'],[0,12,5,0,'12','5','0'],[\n");
  fprintf(ffile, "]).\n");
}

draw_text(int x0, int y0, char *str, int size, int color)
{
  char *cstr[] = { "'black'", "'red'", "'green'", "'yellow'", "'blue'", "'magenta'", "'cyan'", "'white'"};

  if (color>7) color=7;

  fprintf(ffile, "text(%s,%d,%d,1,0,1,40,20,10,10,5,0,0,0,0,2,40,20,0,0,\"\",0,0,0,0,%d,'',[\n", cstr[color], x0, y0, y0);
  fprintf(ffile, "minilines(40,20,0,0,0,0,0,[\n");
  fprintf(ffile, "mini_line(40,20,10,0,0,0,[\n");
  fprintf(ffile, "str_block(0,40,20,10,0,0,0,0,0,[\n");
  fprintf(ffile, "str_seg(%s,'Times-Roman',0,%d,40,20,10,0,0,0,0,1,0,0,\n", cstr[color], size*115200);
  fprintf(ffile, "        \"%s\")])\n", str);
  fprintf(ffile, "])\n");
  fprintf(ffile, "])]).\n");
}
