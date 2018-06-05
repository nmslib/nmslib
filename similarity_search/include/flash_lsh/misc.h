#pragma once

#include <chrono>
#include <string>

#define GETTIME_MS(begin, end) ((end - begin).count() / (float)1000000)

typedef std::chrono::high_resolution_clock Clock;
int smallestPow2(int x);
void zCentering(float *values, int n);
unsigned int getLog2(unsigned int x);
float cosineDist(float *A, float *B, unsigned int n);
float cosineDist(int *indiceA, float *valA, int nonzerosA, int *indiceB, float *valB, int nonzerosB);
int readGraph(std::string fileName, int *A, int *B, unsigned int bufferlen);
float smartrp(int *indiceA, float *valA, int nonzerosA, short *randBits);
void smartrp_batch(int numRP, int dimension, int *indiceA, float *valA, int nonzerosA, short *randBits, float *outputs);
