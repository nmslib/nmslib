#include "MatMul.h"

float SparseVecMul(int *indicesA, float *valuesA, unsigned int sizeA, float *B) {

	float result = 0;
	unsigned int ctA = 0;
	unsigned int iA;

	while (ctA < sizeA) {
		iA = indicesA[ctA];
		result += valuesA[ctA] * B[iA];
		ctA++;
	}
	return result;
}

float SparseVecMul(int *indicesA, float *valuesA, unsigned int sizeA,
	int *indicesB, float *valuesB, unsigned int sizeB) {
	
	float result = 0;
	unsigned int ctA = 0;
	unsigned int ctB = 0;
	unsigned int iA, iB;

	/* Maximum iteration: nonzerosA + nonzerosB.*/
	while (ctA < sizeA && ctB < sizeB) {
		iA = indicesA[ctA];
		iB = indicesB[ctB];
		
		if (iA == iB) {
			result += valuesA[ctA] * valuesB[ctB];
			ctA++;
			ctB++;
		} else if (iA < iB) {
			ctA ++;
		} else if (iA > iB) {
			ctB++;
		}
	}
	return result;
}