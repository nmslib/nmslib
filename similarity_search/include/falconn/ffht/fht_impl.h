#ifdef __AVX__
#include <immintrin.h>
#endif

#include <math.h>

int FHTFloatCombined(float *buffer, int len, int chunk);
int FHTDoubleCombined(double *buffer, int len, int chunk);

#ifdef __AVX__
int FHTFloatCombinedAVX(float *buffer, int len, int chunk);
int FHTDoubleCombinedAVX(double *buffer, int len, int chunk);
#endif

void FHTFloatCombinedHelper(float *buffer, int len, int chunk);
void FHTFloatIterativeHelper(float *buffer, int len, int logLen);

void FHTDoubleCombinedHelper(double *buffer, int len, int chunk);
void FHTDoubleIterativeHelper(double *buffer, int len, int logLen);

#ifdef __AVX__
void FHTFloatNormalizeAVX(float *buffer, int len);
void FHTFloatCombinedHelperAVX(float *buffer, int len, int chunk);
void FHTFloatIterativeHelperAVX(float *buffer, int len, int logLen);
void FHTFloatIterative8HelperAVX(float *buffer);
void FHTFloatIterative16HelperAVX(float *buffer);
void FHTFloatIterative32HelperAVX(float *buffer);
void FHTFloatIterativeLongHelperAVX(float *buffer, int len, int logLen);

void FHTDoubleNormalizeAVX(double *buffer, int len);
void FHTDoubleCombinedHelperAVX(double *buffer, int len, int chunk);
void FHTDoubleIterativeHelperAVX(double *buffer, int len, int logLen);
#endif

inline int FHTFloat(float *buffer, int len, int chunk) {
#ifdef __AVX__
  return FHTFloatCombinedAVX(buffer, len, chunk);
#else
  return FHTFloatCombined(buffer, len, chunk);
#endif
}

inline int FHTDouble(double *buffer, int len, int chunk) {
#ifdef __AVX__
  return FHTDoubleCombinedAVX(buffer, len, chunk);
#else
  return FHTDoubleCombined(buffer, len, chunk);
#endif
}

#define GEN_ITERATIVE_HELPER(NAME, TYPE)         \
  inline void NAME(TYPE *buffer, int len, int logLen) { \
    int i, j, k, step, step2;                    \
    TYPE u, v;                                   \
                                                 \
    for (i = 0; i < logLen; ++i) {               \
      step = 1 << i;                             \
      step2 = step << 1;                         \
      for (j = 0; j < len; j += step2) {         \
        for (k = j; k < j + step; ++k) {         \
          u = buffer[k];                         \
          v = buffer[k + step];                  \
          buffer[k] = u + v;                     \
          buffer[k + step] = u - v;              \
        }                                        \
      }                                          \
    }                                            \
  }

GEN_ITERATIVE_HELPER(FHTFloatIterativeHelper, float)
GEN_ITERATIVE_HELPER(FHTDoubleIterativeHelper, double)

#define GEN_COMBINED_HELPER(NAME, SUBNAME, TYPE) \
  inline void NAME(TYPE *buffer, int len, int chunk) {  \
    int logLen, hl, i;                           \
    TYPE u, v;                                   \
                                                 \
    if (len == 1) {                              \
      return;                                    \
    }                                            \
    if (len <= chunk) {                          \
      logLen = 0;                                \
      while (1 << logLen < len) {                \
        ++logLen;                                \
      }                                          \
      SUBNAME(buffer, len, logLen);              \
      return;                                    \
    }                                            \
    hl = len / 2;                                \
    NAME(buffer, hl, chunk);                     \
    NAME(buffer + hl, hl, chunk);                \
    for (i = 0; i < hl; ++i) {                   \
      u = buffer[i];                             \
      v = buffer[i + hl];                        \
      buffer[i] = u + v;                         \
      buffer[i + hl] = u - v;                    \
    }                                            \
  }

GEN_COMBINED_HELPER(FHTFloatCombinedHelper, FHTFloatIterativeHelper, float)
GEN_COMBINED_HELPER(FHTDoubleCombinedHelper, FHTDoubleIterativeHelper, double)

#ifdef __AVX__

#define GEN_COMBINED_HELPER_AVX(NAME, SUBNAME, TYPE, BATCH_TYPE, STEP, LOAD, \
                                SAVE, ADD, SUB)                              \
  inline void NAME(TYPE *buffer, int len, int chunk) {                              \
    int hl, logLen, j;                                                       \
    TYPE *uu, *vv;                                                           \
    BATCH_TYPE A, B;                                                         \
                                                                             \
    if (len == 1) {                                                          \
      return;                                                                \
    }                                                                        \
    if (len <= chunk) {                                                      \
      logLen = 0;                                                            \
      while (1 << logLen < len) {                                            \
        ++logLen;                                                            \
      }                                                                      \
      SUBNAME(buffer, len, logLen);                                          \
      return;                                                                \
    }                                                                        \
    hl = len / 2;                                                            \
    NAME(buffer, hl, chunk);                                                 \
    NAME(buffer + hl, hl, chunk);                                            \
    for (j = 0; j < hl; j += STEP) {                                         \
      uu = buffer + j;                                                       \
      vv = uu + hl;                                                          \
      A = LOAD(uu);                                                          \
      B = LOAD(vv);                                                          \
      SAVE(uu, ADD(A, B));                                                   \
      SAVE(vv, SUB(A, B));                                                   \
    }                                                                        \
  }

GEN_COMBINED_HELPER_AVX(FHTFloatCombinedHelperAVX, FHTFloatIterativeHelperAVX,
                        float, __m256, 8, _mm256_load_ps, _mm256_store_ps,
                        _mm256_add_ps, _mm256_sub_ps)
GEN_COMBINED_HELPER_AVX(FHTDoubleCombinedHelperAVX, FHTDoubleIterativeHelperAVX,
                        double, __m256d, 4, _mm256_load_pd, _mm256_store_pd,
                        _mm256_add_pd, _mm256_sub_pd)

inline void FHTFloatIterative8HelperAVX(float *buffer) {
  __m256 A, B, C, D, E, ZERO;
  ZERO = _mm256_set_ps(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  // Iteration #0
  A = _mm256_load_ps(buffer);
  B = _mm256_permute_ps(A, (2 << 4) | (2 << 6));
  C = _mm256_permute_ps(A, 1 | (1 << 2) | (3 << 4) | (3 << 6));
  D = _mm256_sub_ps(ZERO, C);
  E = _mm256_addsub_ps(B, D);

  // Iteration #1
  A = _mm256_permute_ps(E, (1 << 2) | (1 << 6));
  B = _mm256_permute_ps(E, 2 | (3 << 2) | (2 << 4) | (3 << 6));
  C = _mm256_sub_ps(ZERO, B);
  // SLOW!!!!
  D = _mm256_blend_ps(B, C, (1 << 2) | (1 << 3) | (1 << 6) | (1 << 7));
  E = _mm256_add_ps(A, D);

  // Iteration #2
  B = _mm256_sub_ps(ZERO, E);
  C = _mm256_permute2f128_ps(E, E, 0);
  D = _mm256_permute2f128_ps(E, B, 1 | (3 << 4));
  _mm256_store_ps(buffer, _mm256_add_ps(C, D));
}

#define BUTTERFLY_FLOAT(A, B, T) \
  T = _mm256_sub_ps(A, B);       \
  A = _mm256_add_ps(A, B);       \
  B = T

#define BUTTERFLY_DOUBLE(A, B, T) \
  T = _mm256_sub_pd(A, B);        \
  A = _mm256_add_pd(A, B);        \
  B = T

inline void FHTFloatIterative16HelperAVX(float *buffer) {
  __m256 A, B, C, D, E, ZERO, A0, A1;
  int i;

  ZERO = _mm256_set_ps(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  for (i = 0; i < 16; i += 8) {
    // Iteration #0
    A = _mm256_load_ps(buffer + i);
    B = _mm256_permute_ps(A, (2 << 4) | (2 << 6));
    C = _mm256_permute_ps(A, 1 | (1 << 2) | (3 << 4) | (3 << 6));
    D = _mm256_sub_ps(ZERO, C);
    E = _mm256_addsub_ps(B, D);

    // Iteration #1
    A = _mm256_permute_ps(E, (1 << 2) | (1 << 6));
    B = _mm256_permute_ps(E, 2 | (3 << 2) | (2 << 4) | (3 << 6));
    C = _mm256_sub_ps(ZERO, B);
    // SLOW!!!!
    D = _mm256_blend_ps(B, C, (1 << 2) | (1 << 3) | (1 << 6) | (1 << 7));
    E = _mm256_add_ps(A, D);

    // Iteration #2
    B = _mm256_sub_ps(ZERO, E);
    C = _mm256_permute2f128_ps(E, E, 0);
    D = _mm256_permute2f128_ps(E, B, 1 | (3 << 4));
    _mm256_store_ps(buffer + i, _mm256_add_ps(C, D));
  }
  A0 = _mm256_load_ps(buffer);
  A1 = _mm256_load_ps(buffer + 8);
  _mm256_store_ps(buffer, _mm256_add_ps(A0, A1));
  _mm256_store_ps(buffer + 8, _mm256_sub_ps(A0, A1));
}

inline void FHTFloatIterative32HelperAVX(float *buffer) {
  __m256 A, B, C, D, E, ZERO, A0, A1, A2, A3, T;
  int i;

  ZERO = _mm256_set_ps(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  for (i = 0; i < 32; i += 8) {
    // Iteration #0
    A = _mm256_load_ps(buffer + i);
    B = _mm256_permute_ps(A, (2 << 4) | (2 << 6));
    C = _mm256_permute_ps(A, 1 | (1 << 2) | (3 << 4) | (3 << 6));
    D = _mm256_sub_ps(ZERO, C);
    E = _mm256_addsub_ps(B, D);

    // Iteration #1
    A = _mm256_permute_ps(E, (1 << 2) | (1 << 6));
    B = _mm256_permute_ps(E, 2 | (3 << 2) | (2 << 4) | (3 << 6));
    C = _mm256_sub_ps(ZERO, B);
    // SLOW!!!!
    D = _mm256_blend_ps(B, C, (1 << 2) | (1 << 3) | (1 << 6) | (1 << 7));
    E = _mm256_add_ps(A, D);

    // Iteration #2
    B = _mm256_sub_ps(ZERO, E);
    C = _mm256_permute2f128_ps(E, E, 0);
    D = _mm256_permute2f128_ps(E, B, 1 | (3 << 4));
    _mm256_store_ps(buffer + i, _mm256_add_ps(C, D));
  }
  A0 = _mm256_load_ps(buffer);
  A1 = _mm256_load_ps(buffer + 8);
  A2 = _mm256_load_ps(buffer + 16);
  A3 = _mm256_load_ps(buffer + 24);
  BUTTERFLY_FLOAT(A0, A1, T);
  BUTTERFLY_FLOAT(A2, A3, T);
  _mm256_store_ps(buffer, _mm256_add_ps(A0, A2));
  _mm256_store_ps(buffer + 8, _mm256_add_ps(A1, A3));
  _mm256_store_ps(buffer + 16, _mm256_sub_ps(A0, A2));
  _mm256_store_ps(buffer + 24, _mm256_sub_ps(A1, A3));
}

inline void FHTFloatIterativeHelperAVX(float *buffer, int len, int logLen) {
  float u, v, w, x;
  if (len == 2) {
    u = buffer[0];
    buffer[0] += buffer[1];
    buffer[1] = u - buffer[1];
  } else if (len == 4) {
    u = buffer[0];
    v = buffer[1];
    w = buffer[2];
    x = buffer[3];
    buffer[0] = u + v + w + x;
    buffer[1] = u - v + w - x;
    buffer[2] = u + v - w - x;
    buffer[3] = u - v - w + x;
  } else if (len == 8) {
    FHTFloatIterative8HelperAVX(buffer);
  } else if (len == 16) {
    FHTFloatIterative16HelperAVX(buffer);
  } else if (len == 32) {
    FHTFloatIterative32HelperAVX(buffer);
  } else {
    FHTFloatIterativeLongHelperAVX(buffer, len, logLen);
  }
}

inline void FHTDoubleIterativeHelperAVX(double *buffer, int len, int logLen) {
  // This can and needs to be optimized!
  double tmp;
  int i, step, step2, level, j, startLevel;
  __m256d ZERO, A, B, C, D, A0, A1, A2, A3, A4, A5, A6, A7, T;
  double *u, *v, *aux;

  if (len == 2) {
    tmp = buffer[0];
    buffer[0] += buffer[1];
    buffer[1] = tmp - buffer[1];
  } else {
    ZERO = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
    for (i = 0; i < len; i += 4) {
      // Iteration #0
      A = _mm256_load_pd(buffer + i);
      B = _mm256_permute_pd(A, 0);
      C = _mm256_permute_pd(A, 15);
      D = _mm256_sub_pd(ZERO, C);
      A = _mm256_addsub_pd(B, D);

      // Iteration #1
      B = _mm256_permute2f128_pd(A, A, 0);
      C = _mm256_sub_pd(ZERO, A);
      D = _mm256_permute2f128_pd(A, C, 1 | (3 << 4));
      _mm256_store_pd(buffer + i, _mm256_add_pd(B, D));
    }
    startLevel = 2;
    if (len == 4) {
      return;
    }
    if (len == 8) {
    } else if (len == 16) {
      A0 = _mm256_load_pd(buffer);
      A1 = _mm256_load_pd(buffer + 4);
      A2 = _mm256_load_pd(buffer + 8);
      A3 = _mm256_load_pd(buffer + 12);
      BUTTERFLY_DOUBLE(A0, A1, T);
      BUTTERFLY_DOUBLE(A2, A3, T);
      BUTTERFLY_DOUBLE(A0, A2, T);
      BUTTERFLY_DOUBLE(A1, A3, T);
      _mm256_store_pd(buffer, A0);
      _mm256_store_pd(buffer + 4, A1);
      _mm256_store_pd(buffer + 8, A2);
      _mm256_store_pd(buffer + 12, A3);
      return;
    } else {
      for (i = 0; i < len; i += 32) {
        aux = buffer + i;
        A0 = _mm256_load_pd(aux);
        A1 = _mm256_load_pd(aux + 4);
        A2 = _mm256_load_pd(aux + 8);
        A3 = _mm256_load_pd(aux + 12);
        A4 = _mm256_load_pd(aux + 16);
        A5 = _mm256_load_pd(aux + 20);
        A6 = _mm256_load_pd(aux + 24);
        A7 = _mm256_load_pd(aux + 28);
        BUTTERFLY_DOUBLE(A0, A1, T);
        BUTTERFLY_DOUBLE(A2, A3, T);
        BUTTERFLY_DOUBLE(A4, A5, T);
        BUTTERFLY_DOUBLE(A6, A7, T);
        BUTTERFLY_DOUBLE(A0, A2, T);
        BUTTERFLY_DOUBLE(A1, A3, T);
        BUTTERFLY_DOUBLE(A4, A6, T);
        BUTTERFLY_DOUBLE(A5, A7, T);
        BUTTERFLY_DOUBLE(A0, A4, T);
        BUTTERFLY_DOUBLE(A1, A5, T);
        BUTTERFLY_DOUBLE(A2, A6, T);
        BUTTERFLY_DOUBLE(A3, A7, T);
        _mm256_store_pd(aux, A0);
        _mm256_store_pd(aux + 4, A1);
        _mm256_store_pd(aux + 8, A2);
        _mm256_store_pd(aux + 12, A3);
        _mm256_store_pd(aux + 16, A4);
        _mm256_store_pd(aux + 20, A5);
        _mm256_store_pd(aux + 24, A6);
        _mm256_store_pd(aux + 28, A7);
      }
      startLevel = 5;
    }
    for (level = startLevel; level < logLen; ++level) {
      step = 1 << level;
      step2 = step << 1;
      for (i = 0; i < len; i += step2) {
        for (j = 0; j < step; j += 4) {
          u = buffer + i + j;
          v = u + step;
          A = _mm256_load_pd(u);
          B = _mm256_load_pd(v);
          _mm256_store_pd(u, _mm256_add_pd(A, B));
          _mm256_store_pd(v, _mm256_sub_pd(A, B));
        }
      }
    }
  }
}

inline void FHTFloatNormalizeAVX(float *buffer, int len) {
  float s;
  int i;
  __m256 S, A;

  s = 1 / sqrtf(len + 0.0);

  if (len < 8) {
    for (i = 0; i < len; ++i) {
      buffer[i] *= s;
    }
  } else {
    S = _mm256_set_ps(s, s, s, s, s, s, s, s);
    for (i = 0; i < len; i += 8) {
      A = _mm256_load_ps(buffer + i);
      _mm256_store_ps(buffer + i, _mm256_mul_ps(A, S));
    }
  }
}

inline void FHTDoubleNormalizeAVX(double *buffer, int len) {
  double s;
  int i;
  __m256d S, A;

  s = 1 / sqrt(len + 0.0);

  if (len < 4) {
    for (i = 0; i < len; ++i) {
      buffer[i] *= s;
    }
  } else {
    S = _mm256_set_pd(s, s, s, s);
    for (i = 0; i < len; i += 4) {
      A = _mm256_load_pd(buffer + i);
      _mm256_store_pd(buffer + i, _mm256_mul_pd(A, S));
    }
  }
}

#define GEN_COMBINED_AVX(NAME, SUBNAME1, SUBNAME2, TYPE) \
  inline int NAME(TYPE *buffer, int len, int chunk) {           \
    if (chunk < 8) {                                     \
      return -1;                                         \
    }                                                    \
                                                         \
    if (len == 1) {                                      \
      return 0;                                          \
    }                                                    \
                                                         \
    if (len < 1 || len & (len - 1)) {                    \
      return -1;                                         \
    }                                                    \
                                                         \
    SUBNAME1(buffer, len, chunk);                        \
    SUBNAME2(buffer, len);                               \
                                                         \
    return 0;                                            \
  }

GEN_COMBINED_AVX(FHTFloatCombinedAVX, FHTFloatCombinedHelperAVX,
                 FHTFloatNormalizeAVX, float)

GEN_COMBINED_AVX(FHTDoubleCombinedAVX, FHTDoubleCombinedHelperAVX,
                 FHTDoubleNormalizeAVX, double)

#endif

#define GEN_COMBINED(NAME, SUBNAME, TYPE, SQRT_FUNC) \
  inline int NAME(TYPE *buffer, int len, int chunk) {       \
    int i;                                           \
    TYPE s;                                          \
                                                     \
    if (chunk < 8) {                                 \
      return -1;                                     \
    }                                                \
                                                     \
    if (len < 1 || len & (len - 1)) {                \
      return -1;                                     \
    }                                                \
                                                     \
    SUBNAME(buffer, len, chunk);                     \
                                                     \
    s = 1.0 / SQRT_FUNC(len + 0.0);                  \
    for (i = 0; i < len; ++i) {                      \
      buffer[i] *= s;                                \
    }                                                \
    return 0;                                        \
  }

GEN_COMBINED(FHTFloatCombined, FHTFloatCombinedHelper, float, sqrtf)

GEN_COMBINED(FHTDoubleCombined, FHTDoubleCombinedHelper, double, sqrt)

#ifdef __AVX__

inline void FHTFloatIterativeLongHelperAVX(float *buffer, int len, int logLen) {
  int i, level, j, step, step2;
  __m256 ZERO, A, B, C, D, E;
  __m256 A0, A1, A2, A3, A4, A5, A6, A7, T;
  float *u, *v;

  ZERO = _mm256_set_ps(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  for (i = 0; i < len; i += 8) {
    // Iteration #0
    A = _mm256_load_ps(buffer + i);
    B = _mm256_permute_ps(A, (2 << 4) | (2 << 6));
    C = _mm256_permute_ps(A, 1 | (1 << 2) | (3 << 4) | (3 << 6));
    D = _mm256_sub_ps(ZERO, C);
    E = _mm256_addsub_ps(B, D);

    // Iteration #1
    A = _mm256_permute_ps(E, (1 << 2) | (1 << 6));
    B = _mm256_permute_ps(E, 2 | (3 << 2) | (2 << 4) | (3 << 6));
    C = _mm256_sub_ps(ZERO, B);
    // SLOW!!!!
    D = _mm256_blend_ps(B, C, (1 << 2) | (1 << 3) | (1 << 6) | (1 << 7));
    E = _mm256_add_ps(A, D);

    // Iteration #2
    B = _mm256_sub_ps(ZERO, E);
    C = _mm256_permute2f128_ps(E, E, 0);
    D = _mm256_permute2f128_ps(E, B, 1 | (3 << 4));
    _mm256_store_ps(buffer + i, _mm256_add_ps(C, D));
  }
  // Iterations #3, #4 and #5
  for (i = 0; i < len; i += 64) {
    A0 = _mm256_load_ps(buffer + i);
    A1 = _mm256_load_ps(buffer + i + 8);
    A2 = _mm256_load_ps(buffer + i + 16);
    A3 = _mm256_load_ps(buffer + i + 24);
    A4 = _mm256_load_ps(buffer + i + 32);
    A5 = _mm256_load_ps(buffer + i + 40);
    A6 = _mm256_load_ps(buffer + i + 48);
    A7 = _mm256_load_ps(buffer + i + 56);
    BUTTERFLY_FLOAT(A0, A1, T);
    BUTTERFLY_FLOAT(A2, A3, T);
    BUTTERFLY_FLOAT(A4, A5, T);
    BUTTERFLY_FLOAT(A6, A7, T);
    BUTTERFLY_FLOAT(A0, A2, T);
    BUTTERFLY_FLOAT(A1, A3, T);
    BUTTERFLY_FLOAT(A4, A6, T);
    BUTTERFLY_FLOAT(A5, A7, T);
    BUTTERFLY_FLOAT(A0, A4, T);
    BUTTERFLY_FLOAT(A1, A5, T);
    BUTTERFLY_FLOAT(A2, A6, T);
    BUTTERFLY_FLOAT(A3, A7, T);
    _mm256_store_ps(buffer + i, A0);
    _mm256_store_ps(buffer + i + 8, A1);
    _mm256_store_ps(buffer + i + 16, A2);
    _mm256_store_ps(buffer + i + 24, A3);
    _mm256_store_ps(buffer + i + 32, A4);
    _mm256_store_ps(buffer + i + 40, A5);
    _mm256_store_ps(buffer + i + 48, A6);
    _mm256_store_ps(buffer + i + 56, A7);
  }
  // Iterations starting from #6
  for (level = 6; level < logLen; ++level) {
    step = 1 << level;
    step2 = step << 1;
    for (i = 0; i < len; i += step2) {
      for (j = 0; j < step; j += 8) {
        u = buffer + i + j;
        v = u + step;
        A = _mm256_load_ps(u);
        B = _mm256_load_ps(v);
        _mm256_store_ps(u, _mm256_add_ps(A, B));
        _mm256_store_ps(v, _mm256_sub_ps(A, B));
      }
    }
  }
}

#endif
