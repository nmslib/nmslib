
/* EMAX7 Compiler                      */
/*        Copyright (C) 2012 by NAIST. */
/*         Primary writer: Y.Nakashima */
/*                nakashim@is.naist.jp */

/* conv-c2d.l   2012/6/15 */

%e      300
%p      700
%n      100
%k      60
%a      300
%o      500

%{
double      atof();
void        *malloc();
static int  indx;
%}

%%
\n                   { ++y_lineno; return(EOL); }
^[^\/\n].*           { return(ARMV8); }
^\/[^-\n].*          { return(ARMV8); }
^\/-EMAX5AB-\/       { return(EMAX7ABEGIN); }
^\/-EMAX5AS-\/       { return(EMAX7ASTATEM); }
^\/-EMAX5AE-\/       { return(EMAX7AEND); }
^\/-EMAX5AD-\/       { return(EMAX7ADRAIN); }
^\/-EMAX5TB-\/       { return(EMAX7TBEGIN); }
^\/-EMAX5TS-\/       { return(EMAX7TSTATEM); }
^\/-EMAX5TE-\/       { return(EMAX7TEND); }
\(Ull\)              { return(CGRA_ULL); }
\(Uint\)             { return(CGRA_UINT); }
"<<"                 { return(CGRA_SLL); }
">>"                 { return(CGRA_SRL); }
while                { return(CGRA_WHILE); }
for                  { return(CGRA_FOR); }
cex                  { return(CGRA_CEX); }
ex4                  { return(CGRA_EX4); }
exe                  { return(CGRA_EXE); }
mex                  { return(CGRA_MEX); }
mo4                  { return(CGRA_MO4); }
mop                  { return(CGRA_MOP); }
\-\-                 {
    /* copy "-1LL" --> buf[] */
    yylval = hash_reg_immediate(-1LL);
    return(CGRA_DECR);
}
\+\+                 { return(CGRA_INCR); }
read                 { return(TRAN_READ); }
write                { return(TRAN_WRITE); }
[\(\)\[\]\{\}\<\>,\;\*\/\%\+\-\&\|\^\~\=\?\:] { return(*yytext); }
[ \t]+ ;

\(\(void[ ]*\*\)0\)  {
    /* copy yytext[] --> buf[] */
    for (indx=0; indx<yyleng && indx<BUF_MAXLEN; indx++)
        buf[indx] = yytext[indx];
    buf[indx] = 0;
    if (!hash_search(buf, &yylval)) { /* not found */
      id[yylval].type = T_IMMEDIATE;
      id[yylval].chip = 0;
      id[yylval].cidx = 0;
      id[yylval].val = 0;
    }
    return(IMMEDIATE);
}

0x[0-9A-Fa-f]+   |
0x[0-9A-Fa-f]+LL |
[0-9]+           |
[0-9]+LL         {
    /* copy yytext[] --> buf[] */
    for (indx=0; indx<yyleng && indx<BUF_MAXLEN; indx++)
        buf[indx] = yytext[indx];
    buf[indx] = 0;
    if (!hash_search(buf, &yylval)) { /* not found */
      id[yylval].type = T_IMMEDIATE;
      id[yylval].chip = 0;
      id[yylval].cidx = 0;
      id[yylval].val = strtoull(buf, NULL, 0);
    }
    return(IMMEDIATE);
}

ex[0-9] {
    /* copy yytext[] --> buf[] */
    for (indx=0; indx<yyleng && indx<BUF_MAXLEN; indx++)
        buf[indx] = yytext[indx];
    buf[indx] = 0;
    if (!hash_search(buf, &yylval)) { /* not found */
      id[yylval].type = T_EXRNO;
      id[yylval].chip = 0;
      id[yylval].cidx = 0;
      id[yylval].val = strtoull(buf+3, NULL, 0);
    }
    return(EXRNO);
}

AR\[[0-9][0-9]*\] { /* row only */
    /* copy yytext[] --> buf[] */
    for (indx=0; indx<yyleng && indx<BUF_MAXLEN; indx++)
        buf[indx] = yytext[indx];
    buf[indx] = 0;
    if (!hash_search(buf, &yylval)) { /* not found */
      id[yylval].type = T_ALRNO;
      id[yylval].chip = 0;
      id[yylval].cidx = 0;
      id[yylval].val = strtoull(buf+3, NULL, 0);
    }
    return(ALRNO);
}

BR\[[0-9][0-9]*\]\[[0-3]\] { /* row+column only */
    /* copy yytext[] --> buf[] */
    for (indx=0; indx<yyleng && indx<BUF_MAXLEN; indx++)
        buf[indx] = yytext[indx];
    buf[indx] = 0;
    if (!hash_search(buf, &yylval)) { /* not found */
      id[yylval].type = T_BDRNO;
      id[yylval].chip = 0;
      id[yylval].cidx = 0;
      id[yylval].val = strtoull(strchr(buf, '\[')+1, NULL, 0)*EMAX_WIDTH+strtoull(strrchr(buf, '\[')+1, NULL, 0);
    }
    return(BDRNO);
}

CHIP {
    /* copy yytext[] --> buf[] */
    for (indx=0; indx<yyleng && indx<BUF_MAXLEN; indx++)
        buf[indx] = yytext[indx];
    buf[indx] = 0;
    if (!hash_search(buf, &yylval)) { /* not found */
      id[yylval].type = T_VARIABLE;
      id[yylval].chip = 1;
      id[yylval].cidx = 0;
      id[yylval].val = 0;
    }
    return(CHIP);
}

LOOP[0-3] { /* LOOP[23] not available */
    /* copy yytext[] --> buf[] */
    for (indx=0; indx<yyleng && indx<BUF_MAXLEN; indx++)
        buf[indx] = yytext[indx];
    buf[indx] = 0;
    if (!hash_search(buf, &yylval)) { /* not found */
      id[yylval].type = T_LOOPNO;
      id[yylval].chip = 0;
      id[yylval].cidx = 0;
      id[yylval].val = strtoull(buf+4, NULL, 0);
    }
    return(LOOPNO);
}

INIT[0-3] { /* INIT[23] not available */
    /* copy yytext[] --> buf[] */
    for (indx=0; indx<yyleng && indx<BUF_MAXLEN; indx++)
        buf[indx] = yytext[indx];
    buf[indx] = 0;
    if (!hash_search(buf, &yylval)) { /* not found */
      id[yylval].type = T_INITNO;
      id[yylval].chip = 0;
      id[yylval].cidx = 0;
      id[yylval].val = strtoull(buf+4, NULL, 0);
    }
    return(INITNO);
}

\.*[A-Za-z_][A-Za-z0-9_]* |
[A-Za-z_][A-Za-z0-9_]* |
[A-Za-z_][A-Za-z0-9_]*\[[0-9][0-9]*\] |
[A-Za-z_][A-Za-z0-9_]*\[LANE\] |
[A-Za-z_][A-Za-z0-9_]*\[[0-9][0-9]*\]\[LANE\] |
[a-z_][a-z0-9_]*\[[0-9][0-9]*\]\[[0-9][0-9]*\] |
[a-z_][a-z0-9_]*\[[0-9][0-9]*\]\[[0-9][0-9]*\]\[[0-9][0-9]*\] {
    /* copy yytext[] --> buf[] */
    for (indx=0; indx<yyleng && indx<BUF_MAXLEN; indx++)
        buf[indx] = yytext[indx];
    buf[indx] = 0;
    if (!hash_search(buf, &yylval)) { /* not found */
      id[yylval].type = T_VARIABLE;
      id[yylval].chip = 0;
      id[yylval].cidx = 0;
      id[yylval].val  = 0;
    }
    return(VARIABLE);
}

\"\"[^\"]*\"\" {
    /* copy yytext[] --> buf[] */
    for (indx=0; indx<yyleng && indx<BUF_MAXLEN; indx++)
      buf[indx] = yytext[indx];
    buf[     0] = '(';
    buf[     1] = '(';
    buf[indx-2] = ')';
    buf[indx-1] = ')';
    buf[indx  ] = 0;
    if (!hash_search(buf, &yylval)) { /* not found */
      id[yylval].type = T_ASIS;
      id[yylval].chip = 0;
      id[yylval].cidx = 0;
      id[yylval].val  = 0;
    }
    return(ASIS);
}
