#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#ifdef OPENCL_2XX
#include <CL/cl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <algorithm>
#include <inttypes.h>
#include <math.h>

#include "omp.h"
#include "LSH.h"

/* Outputs and verbose amount. */
//#define PROFILE_READ
#define DEBUGTB 3
#define DEBUGENTRIES 20

//#define DEBUG

#define NUM_FILES 2
#define PROGRAM_FILE_1 "LSHReservoirSampler.cl"
#define PROGRAM_FILE_2 "LSHReservoirSampler_segsort.cl"

/** LSHReservoirSampler Class. 

	Providing hashtable data-structure and k-select algorithm. 
	An LSH class instantiation is pre-required. 
*/
class LSHReservoirSampler {
private:

	LSH *_hashFamily;
	unsigned int _rangePow, _numTables, _reservoirSize, _dimension, _numSecHash, _maxSamples,
		_maxReservoirRand, _queryProbes, _hashingProbes, _segmentSizeModulor, _segmentSizeBitShiftDivisor;
	float _tableAllocFraction;
#ifdef OPENCL_2XX
	cl_mem _globalRand_obj, _tableMem_obj, _tableMemAllocator_obj, _tablePointers_obj;
#endif

	/* OpenCL. */
	char *_program_log;
#ifdef OPENCL_2XX
	cl_int _err;
	cl_kernel kernel_reservoir, kernel_addtable, kernel_extract_rows, kernel_taketopk,
		kernel_markdiff, kernel_aggdiff, kernel_subtractdiff, kernel_tally_naive;
	cl_kernel kernel_bsort_preprocess, kernel_bsort_postprocess, kernel_bsort_init_manning, 
		kernel_bsort_stage_0_manning, kernel_bsort_stage_n_manning, kernel_bsort_stage_0_manning_kv, 
		kernel_bsort_stage_n_manning_kv, kernel_bsort_init_manning_kv;
#endif

	/* For OpenMP only, but declare anyways. */
	unsigned int* _tableMem;
	unsigned int* _tableMemAllocator; // Special value MAX - 1. 
	unsigned int* _tablePointers;
	omp_lock_t* _tablePointersLock;
	omp_lock_t* _tableCountersLock;

	unsigned int *_global_rand;
	unsigned int _numReservoirs, _sequentialIDCounter_kernel, _numReservoirsHashed, _aggNumReservoirs;
	unsigned long long _tableMemMax, _tableMemReservoirMax, _tablePointerMax;
	float _zerof;
	unsigned int _sechash_a, _sechash_b, _tableNull, _zero;

	/* Init. */
	void initVariables(unsigned int numHashPerFamily, unsigned int numHashFamilies, unsigned int reservoirSize, 
		unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples, unsigned int queryProbes, 
		unsigned int hashingProbes, float tableAllocFraction);
	void initHelper(int numTablesIn, int numHashPerFamilyIn, int reservoriSizeIn);
	void unInit();

#ifdef OPENCL_2XX
	void clPlatformDevices();
	void clContext();
	void clProgram();
	void clKernels();
	void clCommandQueue();
	void clTestAlloc(long long numInts, cl_context* testContext, cl_command_queue* testQueue);
#endif

	/* Buildingblocks. */
	void query_extractRows_cpu_openmp(int numQueryEntries, int segmentSizePow2, unsigned int *queue, unsigned int *hashIndices);
	void query_frequentitem_cpu_openmp(int numQueryEntries, unsigned int *outputs, unsigned int *hashIndices, int topk);
	void reservoir_sampling_cpu_openmp(unsigned int *allprobsHash, unsigned int *allprobsIdx, unsigned int *storelog, int numProbePerTb);
	void add_table_cpu_openmp(unsigned int *storelog, int numProbePerTb);

#ifdef OPENCL_2XX
	void reservoir_sampling_gpu(cl_mem *allprobsHash_obj, cl_mem *allprobsIdx_obj, cl_mem *storelog_obj, int numProbePerTb);

	void add_table_gpu(cl_mem *storelog_obj, int numProbePerTb);

	void query_extractRows_gpu(int numQueryEntries, int segmentSizePow2, cl_mem *queue_obj, cl_mem *hashIndices_obj);

	void query_tallyReduction(int numQueryEntries, int segmentSize, int segmentSizePow2, cl_mem *talley_obj, cl_mem *talleyCount_obj);
	void query_tallyNaive(int segmentSize, int numQueryEntries, cl_mem *talley_obj, cl_mem *talleyCount_obj, cl_mem *queue_obj);
	void query_taketopk(int numQueryEntries, int segmentSizePow2, int topk, cl_mem *talley_obj, cl_mem *talleyCount_obj, unsigned int *topItems);
	void segmentedSortKV(cl_mem *key_in, cl_mem *val_in, int segmentSize, int numSegments, unsigned int valMax);
	void segmentedSort(cl_mem *in, int segmentSize, int numSegments);
#endif

	/* Routines. */
	void HashAddCPUTB(unsigned int *allprobsHash, unsigned int* allprobsIdx, int numProbePerTb, int numInputEntries);
#ifdef OPENCL_2XX
	void RowsAggregationCPUTB(unsigned int *hashIndices, cl_mem *tally_gpuobj, int segmentSizePow2, int numQueryEntries);
#endif
	void kSelect(unsigned int *tally, unsigned int *outputs, int segmentSize, int numQueryEntries, int topk);

#ifdef OPENCL_2XX
	void HashAddGPUTB(cl_mem *allprobsHash_gpuobj, cl_mem* allprobsIdx_gpuobj, int numProbePerTb, int numInputEntries);
	void RowsAggregationGPUTB(cl_mem *hashIndices_gpuobj, cl_mem *tally_gpuobj, int segmentSizePow2, int numQueryEntries);
	void kSelect(cl_mem *tally_gpuobj, unsigned int *outputs, int segmentSize, int segmentSizePow2, int numQueryEntries, int topk);
	void kSelect_debug(cl_mem *tally_gpuobj, unsigned int *tally, int segmentSize, int segmentSizePow2, int numQueryEntries, int topk);

	/* Aux. */
	void clCheckError(cl_int code, const char* msg);
	void clCheckErrorNoExit(cl_int code, const char* msg);
	//void clMemCpy_uint_g2c(cl_mem *dst, cl_mem *src, unsigned int size);
	//void clMemCpy_uint_c2g(cl_mem *dst, cl_mem *src, unsigned int size);
	void memCpy_uint_g2c(unsigned int *dst, cl_mem *src, unsigned int size);
	void memCpy_uint_c2g(cl_mem *dst, unsigned int *src, unsigned int size);

#endif
    void kernelBandWidth(const std::string&, float br, float bw, float time);
    void pause();

	/* Debug. */
	void mock_markdiff(unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSizePow2);
	void mock_agg(unsigned int *g_queryCt, unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSizePow2);
	void mock_sub(unsigned int *g_queryCt, unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSize, int segmentSizePow2);
#ifdef OPENCL_2XX
	void ann_debug(int numQueryEntries, int* dataIdx, float* dataVal, int* dataMarker, int topk);
#endif
	void viewTables();
	int benchCounting(int segmentSize, int* dataIdx, float* dataVal, int* dataMarker, float *timings);

	/* Experimental. */
	void lossy_ann(int numQueryEntries, int* dataIdx, float* dataVal, int* dataMarker, unsigned int* outputs, int k);

public:
#ifdef OPENCL_2XX
	cl_platform_id *platforms;

	cl_device_id *devices_gpu;
	cl_context context_gpu;
	cl_program program_gpu;
	cl_command_queue command_queue_gpu;
#endif

	//cl_device_id *devices_cpu;
	//cl_context context_cpu;
	//cl_program program_cpu;
	//cl_command_queue command_queue_cpu;

	void restart(LSH *hashFamIn, unsigned int numHashPerFamily, unsigned int numHashFamilies,
		unsigned int reservoirSize, unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples,
		unsigned int queryProbes, unsigned int hashingProbes, float tableAllocFraction);

	/** Constructor. 

	Creates an instance of LSHReservoirSampler. 

	@param hashFam An LSH class, a family of hash functions. 
	@param numHashPerFamily Number of hashes (bits) per hash table, have to be the same as that of the hashFam. 
	@param numHashFamilies Number of hash families (tables), have to be the same as that of the hashFam. 
	@param reservoirSize Size of each hash rows (reservoir). 
	@param dimension For dense vectors, this is the dimensionality of each vector. 
		For sparse format data, this number is not used. (TBD)
	@param numSecHash The number of secondary hash bits. A secondary (universal) hashing is used to shrink the
		original range of the LSH for better table occupancy. Only a number <= numHashPerFamily makes sense. 
	@param maxSamples The maximum number incoming data points to be hashed and added. 
	@param queryProbes Number of probes per query per table. 
	@param hashingProbes Number of probes per data point per table.
	@param tableAllocFraction Fraction of reservoirs to allocate for each table, will share with other table if overflows.
	*/
	LSHReservoirSampler(LSH *hashFam, unsigned int numHashPerFamily, unsigned int numHashFamilies, 
		unsigned int reservoirSize, unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples, 
		unsigned int queryProbes, unsigned int hashingProbes, float tableAllocFraction);

	/** Adds input vectors (in sparse format) to the hash table.  

	Each vector is assigned ascending identification starting 0.
	For numInputEntries > 1, simply concatenate data vectors.
		
	@param numInputEntries Number of input vectors. 
	@param dataIdx Non-zero indice of the sparse format. 
	@param dataVal Non-zero values of the sparse format. 
	@param dataMarker Marks the start index of each vector in dataIdx and dataVal. 
		Have an additional marker at the end to mark the (end+1) index.
	*/
	void add(int numInputEntries, int* dataIdx, float* dataVal, int* dataMarker);

	/** Query vectors (in sparse format) and return top k neighbors for each.

	Near-neighbors for each query will be returned in descending similarity. 
	For numQueryEntries > 1, simply concatenate data vectors.
	
	@param numQueryEntries Number of query vectors.
	@param dataIdx Non-zero indice of the sparse format.
	@param dataVal Non-zero values of the sparse format.
	@param dataMarker Marks the start index of each vector in dataIdx and dataVal.
		Have an additional marker at the end to mark the (end+1) index.
	@param outputs Near-neighbor identifications. The i_th neighbor of the q_th query is outputs[q * k + i]
	@param k number of near-neighbors to query for each query vector. 
	*/
	void ann(int numQueryEntries, int* dataIdx, float* dataVal, int* dataMarker, unsigned int* outputs, int k);

	

	/** Adds input vectors (in dense format) to the hash table.

	Each vector is assigned ascending identification starting 0. 
	For numInputEntries > 1, simply concatenate data vectors.

	@param numInputEntries Number of input vectors.
	@param input Concatenated data vectors (fixed dimension). 
	*/
	void add(int numInputEntries, float* input);

	/** Query vectors (in dense format) and return top k neighbors for each.

	Near-neighbors for each query will be returned in descending similarity.
	For numQueryEntries > 1, simply concatenate data vectors.

	@param numQueryEntries Number of query vectors.
	@param queries Concatenated data vectors (fixed dimension).
	@param outputs Near-neighbor identifications. The i_th neighbor of the q_th query is outputs[q * k + i]
	@param k number of near-neighbors to query for each query vector.
	*/
	void ann(int numQueryEntries, float* queries, unsigned int* outputs, int k);	

	/** Print current parameter settings to the console. 
	*/
	void showParams();

	/** Check the memory load of the hash table. 
	*/
	void checkTableMemLoad();

	/** Destructor. Frees memory allocations and OpenCL environments. 
	*/
	~LSHReservoirSampler();
};
