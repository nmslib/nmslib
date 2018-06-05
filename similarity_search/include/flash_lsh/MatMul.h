//@file
//@brief This file contains vector multiplications codes.

#pragma once
#include "LSHReservoirSampler.h"
#include "LSH.h"

float SparseVecMul(int *indicesA, float *valuesA, unsigned int sizeA,
	int *indicesB, float *valuesB, unsigned int sizeB);
float SparseVecMul(int *indicesA, float *valuesA, unsigned int sizeA, float *B);