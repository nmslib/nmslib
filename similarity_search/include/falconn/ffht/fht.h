#ifndef _FHT_H_
#define _FHT_H_

#ifdef __cplusplus
extern "C" {
#endif

int FHTFloat(float *buffer, int len, int chunk);
int FHTDouble(double *buffer, int len, int chunk);

#ifdef __cplusplus
}
#endif

#endif
