
#include "LSH.h"
//#define PROFILE_READ

#ifdef OPENCL_2XX
typedef std::chrono::high_resolution_clock Clock;
/* OpenCL - Sparse. */
void LSH::getHash(cl_mem *hashIndices_obj, cl_mem *probeDataIdx_obj, cl_mem *dataIdx_obj, cl_mem *dataVal_obj, 
	cl_mem *dataMarker_obj, int numInputEntries, int numProbes) {
#ifdef PROFILE_READ
	auto begin = Clock::now();
#endif
	if (_clEnabled == 0) {
		std::cout << "[LSH::getHash] OpenCL is not initialized" << std::endl;
		exit(1);
	}
	cl_mem hashes_obj = clCreateBuffer(_context_lsh, CL_MEM_READ_WRITE,
		_numTables * numInputEntries * _rangePow * sizeof(int), NULL, &_err);
	ClCheckError(_err, "[LSH::getHash] Failed to write hashes_obj!");

	switch (_hashType)
	{
	case 1:
		srp_opencl_sparse(&hashes_obj, dataIdx_obj, dataVal_obj, dataMarker_obj, numInputEntries);
		getHashIdx(hashIndices_obj, probeDataIdx_obj, &hashes_obj, numInputEntries, numProbes);
		break;
	case 2:
		break;
	default:
		break;
	}
	clReleaseMemObject(hashes_obj);
#ifdef PROFILE_READ
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[LSH::getHash] Computation %5.3f ms\n", etime);
#endif
}
#endif

/* OpenMP - Sparse. */
void LSH::getHash(unsigned int *hashIndices, unsigned int *probeDataIdx, int *dataIdx, float *dataVal, int *dataMarker, int numInputEntries, int numProbes) {
#if defined DEBUG
	std::cout << "[LSH::getHash]" << std::endl;
#endif
#ifdef PROFILE_READ
	auto begin = Clock::now();
#endif

	if (_hashType == 1) {
		unsigned int *hashes = new unsigned int[_numTables * numInputEntries * _rangePow];
		srp_openmp_sparse(hashes, dataIdx, dataVal, dataMarker, numInputEntries);
		getHashIdx(hashIndices, probeDataIdx, hashes, numInputEntries, numProbes);
		delete[] hashes;
	}
	else if (_hashType == 2) {
		getOptimalMinhash(hashIndices, probeDataIdx, dataIdx, dataMarker, numInputEntries, numProbes);
	}


#ifdef PROFILE_READ
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[LSH::getHash] Computation %5.3f ms\n", etime);
#endif
#if defined DEBUG
	std::cout << "[LSH::getHash] Exit. " << std::endl;
#endif
}

/* OpenCL - Dense. */
#ifdef OPENCL_2XX
void LSH::getHash(cl_mem *hashIndices_obj, cl_mem *probeDataIdx_obj, cl_mem *input_obj, int numInputEntries, int numProbes) {
	if (_clEnabled == 0) {
		std::cout << "[LSH::getHash] OpenCL is not initialized" << std::endl;
		exit(1);
	}
#ifdef PROFILE_READ
	auto begin = Clock::now();
#endif
	cl_mem hashes_obj = clCreateBuffer(_context_lsh, CL_MEM_READ_WRITE,
		_numTables * numInputEntries * _rangePow * sizeof(int), NULL, &_err);
	ClCheckError(_err, "[LSH::getHash] Failed to write hashes_obj!");

	switch (_hashType)
	{
	case 1:
		srp_opencl_dense(&hashes_obj, input_obj, numInputEntries);
		getHashIdx(hashIndices_obj, probeDataIdx_obj, &hashes_obj, numInputEntries, numProbes);
		break;
	case 2:
		break;
	default:
		break;
	}
	clReleaseMemObject(hashes_obj);

#ifdef PROFILE_READ
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[LSH::getHash] Computation %5.3f ms\n", etime);
#endif
}
#endif

/* OpenMP - Dense. */
void LSH::getHash(unsigned int *hashIndices, unsigned int *probeDataIdx, float *input, int numInputEntries, int numProbes) {
#ifdef PROFILE_READ
	auto begin = Clock::now();
#endif
	switch (_hashType)
	{
	case 1:
		std::cout << "[LSH::getHash] No OpenMP implementation: Dense Data Signed Random Projection. " << std::endl;
		exit(1);
		break;
	case 2:
		break;
	default:
		break;
	}
#ifdef PROFILE_READ
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[LSH::getHash] Computation %5.3f ms\n", etime);
#endif
}



/* Aux functions. */

void LSH::getHashIdx(unsigned int *hashIndices, unsigned int *hashes, int numInputEntries, int numProbes) {
	if (numProbes > _rangePow) {
		std::cout << "[LSH::getHashIdx] More probes than hashes. " << std::endl;
		exit(1);
	}
#ifdef PROFILE_READ
	auto begin = Clock::now();
#endif
	unsigned int hashIdx;
#pragma omp parallel private(hashIdx)
#pragma omp parallel for
	for (int inputIdx = 0; inputIdx < numInputEntries; inputIdx ++) {
		for (int tb = 0; tb < _numTables; tb++) {
			hashIdx = 0;
			for (int k = 0; k < _rangePow; k++) {
				hashIdx |= (unsigned)hashes[hashesOutputIdx(_rangePow, numInputEntries, inputIdx, tb, k)] << k;
			}
			for (int k = 0; k < numProbes; k++) {
				hashIndices[hashIndicesOutputIdx(_numTables, numProbes, numInputEntries, inputIdx, k, tb)] = 
					hashIdx ^ (1 << (k - 1));
			}
		}
	}
#ifdef PROFILE_READ
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[LSH::getHashIdx] Computation %5.3f ms\n", etime);
#endif
}

void LSH::getHashIdx(unsigned int *hashIndices, unsigned int *dataIdx, unsigned int *hashes, int numInputEntries, int numProbes) {
	if (numProbes > _rangePow) {
		std::cout << "[LSH::getHashIdx] More probes than hashes. " << std::endl;
		exit(1);
	}
#ifdef PROFILE_READ
	auto begin = Clock::now();
#endif
	unsigned int hashIdx;
#pragma omp parallel private(hashIdx)
#pragma omp parallel for
	for (int inputIdx = 0; inputIdx < numInputEntries; inputIdx++) {
		for (int tb = 0; tb < _numTables; tb++) {
			hashIdx = 0;
			for (int k = 0; k < _rangePow; k++) {
				hashIdx |= (unsigned)hashes[hashesOutputIdx(_rangePow, numInputEntries, inputIdx, tb, k)] << k;
			}
			for (int k = 0; k < numProbes; k++) {
				hashIndices[hashIndicesOutputIdx(_numTables, numProbes, numInputEntries, inputIdx, k, tb)] = 
					hashIdx ^ (1 << (k - 1));
				dataIdx[hashIndicesOutputIdx(_numTables, numProbes, numInputEntries, inputIdx, k, tb)] = 
					inputIdx;
			}
		}
	}
#ifdef PROFILE_READ
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[LSH::getHashIdx] Computation %5.3f ms\n", etime);
#endif
}

#ifdef OPENCL_2XX
void LSH::getHashIdx(cl_mem *hashIndices_obj, cl_mem *dataIdx_obj, cl_mem *hashes_obj, int numInputEntries, int numProbes) {
	if (_clEnabled == 0) {
		std::cout << "[LSH::getHashIdx] OpenCL is not initialized. " << std::endl;
		exit(1);
	}
#ifdef PROFILE_READ
	auto begin = Clock::now();
#endif
	_err = clSetKernelArg(kernel_mult_probes_storeid, 0, sizeof(cl_mem), (void *)hashIndices_obj);
	_err |= clSetKernelArg(kernel_mult_probes_storeid, 1, sizeof(cl_mem), (void *)dataIdx_obj);
	_err |= clSetKernelArg(kernel_mult_probes_storeid, 2, sizeof(cl_mem), (void *)hashes_obj);
	_err |= clSetKernelArg(kernel_mult_probes_storeid, 3, sizeof(int), (void *)&numInputEntries);
	_err |= clSetKernelArg(kernel_mult_probes_storeid, 6, sizeof(int), (void *)&numProbes);
	ClCheckError(_err, "[LSH::getHashIdx] Failed to set kernel_mult_probes_storeid arguments!");
	size_t gsize_kernel_mprobs[2] = { numInputEntries, _numTables };
	_err = clEnqueueNDRangeKernel(_command_queue_lsh, kernel_mult_probes_storeid, 2, NULL,
		gsize_kernel_mprobs, NULL, 0, NULL, NULL);
	clFinish(_command_queue_lsh);
	ClCheckError(_err, "[LSH::getHashIdx] kernel_mult_probes_storeids failed!");
#ifdef PROFILE_READ
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[LSH::getHashIdx] Computation %5.3f ms\n", etime);
#endif
}
#endif

#ifdef OPENCL_2XX
void LSH::getHashIdx(cl_mem *hashIndices_obj, cl_mem *hashes_obj, int numInputEntries, int numProbes) {
	if (_clEnabled == 0) {
		std::cout << "[LSH::getHashIdx] OpenCL is not initialized. " << std::endl;
		exit(1);
	}
#ifdef PROFILE_READ
	auto begin = Clock::now();
#endif
	_err = clSetKernelArg(kernel_mult_probes, 0, sizeof(cl_mem), (void *)hashIndices_obj);
	_err |= clSetKernelArg(kernel_mult_probes, 1, sizeof(cl_mem), (void *)hashes_obj);
	_err |= clSetKernelArg(kernel_mult_probes, 2, sizeof(int), (void *)&numInputEntries);
	_err |= clSetKernelArg(kernel_mult_probes, 5, sizeof(int), (void *)&numProbes);
	ClCheckError(_err, "[LSH::getHashIdx] Failed to set kernel_mult_probes arguments!");
	size_t gsize_kernel_mprobs[2] = { numInputEntries, _numTables };
	_err = clEnqueueNDRangeKernel(_command_queue_lsh, kernel_mult_probes, 2, NULL,
		gsize_kernel_mprobs, NULL, 0, NULL, NULL);
	clFinish(_command_queue_lsh);
	ClCheckError(_err, "[LSH::getHashIdx] kernel_mult_probess failed!");
#ifdef PROFILE_READ
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[LSH::getHashIdx] Computation %5.3f ms\n", etime);
#endif
}
#endif
