#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#ifdef OPENCL_2XX
#include <CL/cl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <iostream>
#include <random>
#include <algorithm>
#include <omp.h>

//#define DEBUG
#define MAGIC_NUMBER 100 // For debugging purpose, ignore. 

#define UNIVERSAL_HASH(x,M,a,b) ((unsigned) (a * x + b) >> (32 - M))
#define BINARY_HASH(x,a,b) ((unsigned) (a * x + b) >> 31)

#define NUM_CL_KERNEL 1
#define CL_KERNEL_FILE_1 "LSH.cl"

// hashIndicesOutputIdx HAS to be the same as that of the LSHReservoirSampler class !!!
#define hashIndicesOutputIdx(numHashFamilies, numProbes, numInputs, dataIdx, probeIdx, tb) (unsigned long long)(numInputs * numProbes * tb + dataIdx * numProbes + probeIdx)
#define hashesOutputIdx(numHashPerFamily, numInputs, dataIdx, tb, hashInFamIdx) (unsigned long long)(tb * (numInputs * numHashPerFamily) + dataIdx * numHashPerFamily + hashInFamIdx)

class LSH {
private:

	/* Core parameters. */
	int _rangePow;

	// Here _rangePow always means the power2 range of the hash table. 
	int _clEnabled, _hashType;
#ifdef OPENCL_2XX
	cl_kernel kernel_mult_probes_storeid;
	cl_kernel kernel_mult_probes;

	/* OpenCL. */
	cl_int _err;
	cl_platform_id *_platforms_lsh;
	cl_device_id *_devices_lsh;
	cl_context _context_lsh;
	cl_program _program_lsh;
	cl_command_queue _command_queue_lsh;
#endif
	char *_program_log_lsh;
	
	/* Signed random projection. */
	int _numTables;
	int _dimension, _samSize, _samFactor, _groupHashingSize;
#ifdef OPENCL_2XX
	cl_kernel kernel_randproj_dense;
	cl_kernel kernel_randproj_sparse;
	cl_mem _randBits_obj, _indices_obj, _hash_a_obj, _hash_b_obj, _binhash_a_obj, _binhash_b_obj;
#endif
	unsigned int *_binhash_a;
	unsigned int *_binhash_b;
	unsigned int *_hash_a;
	unsigned int *_hash_b;
	short *_randBits;
	int *_indices;

	/* Optimal Densified Minhash. */
	int *_randHash, _randa, _numhashes, _lognumhash, _K, _L;
	int *rand1;

	/* Function definitions. */
#ifdef OPENCL_2XX
	void clProgram_LSH();
	void ClCheckError(cl_int code, const char* msg);
#endif

	/* Signed random projection implementations. */
	void srp_openmp_sparse(unsigned int *hashes, int *dataIdx, float *dataVal, int *dataMarker, int numInputEntries);
#ifdef OPENCL_2XX
	void srp_opencl_sparse(cl_mem *hashes_obj, cl_mem *dataIdx, cl_mem *dataVal, cl_mem *dataMarker, int numInputEntries);
	void srp_opencl_dense(cl_mem *hashes_obj, cl_mem *input_obj, int numInputEntries);
#endif

	/* Optimal Densified Minhash. - by Anshumali */
	unsigned int getRandDoubleHash(int binid, int count);
	void optimalMinHash(unsigned int *hashArray, unsigned int *nonZeros, int sizenonzeros);
	void getOptimalMinhash(unsigned int *hashIndices, unsigned int *probeDataIdx, int *dataIdx, int *dataMarker, int numInputEntries, int numProbes);

#ifdef OPENCL_2XX
	void getHashIdx(cl_mem *hashIndices_obj, cl_mem *probeDataIdx_obj, cl_mem *hashes_obj, int numInputEntries, int numProbes);
	void getHashIdx(cl_mem *hashIndices_obj, cl_mem *hashes_obj, int numInputEntries, int numProbes);
#endif

	void getHashIdx(unsigned int *hashIndices, unsigned int *probeDataIdx, unsigned int *hashes, int numInputEntries, int numProbes);
	void getHashIdx(unsigned int *hashIndices, unsigned int *hashes, int numInputEntries, int numProbes);
	
public:

	/** Obtain hash indice given the (sparse) input vector, using OpenCL. 

	Hash indice refer to the corresponding "row number" in a hash table, in the form of unsigned integer. 
	This function will only be valid when an OpenCL implementation exists for that type of hashing, 
	and when OpenCL is initialized (clLSH) for that hashing. 
	The outputs indexing is defined as hashIndicesOutputIdx(numHashFamilies, numProbes, numInputs, inputIdx, probeIdx, tb) (unsigned)(numInputs * numProbes * tb + inputIdx * numProbes + probeIdx). 
	
	@param hashIndices_obj Hash indice for the batch of input vectors. 
	@param identity_obj Hash indice's corresponding identifications (sequential number starting 0) for this batch of inputs. 
	@param dataIdx_obj Non-zero indice of the sparse format.
	@param dataVal_obj Non-zero values of the sparse format.
	@param dataMarker_obj Marks the start index of each vector in dataIdx and dataVal.
	@param numInputEntries Number of input vectors. 
	@param numProbes Number of probes per input. 
	*/
#ifdef OPENCL_2XX
	void getHash(cl_mem *hashIndices_obj, cl_mem *identity_obj, 
		cl_mem *dataIdx_obj, cl_mem *dataVal_obj, cl_mem *dataMarker_obj, int numInputEntries, int numProbes);
#endif
	/** Obtain hash indice given the (sparse) input vector, using CPU.

	Hash indice refer to the corresponding "row number" in a hash table, in the form of unsigned integer.
	This function will only be valid when an CPU implementation exists for that type of hashing.
	The outputs indexing is defined as hashIndicesOutputIdx(numHashFamilies, numProbes, numInputs, inputIdx, probeIdx, tb) (unsigned)(numInputs * numProbes * tb + inputIdx * numProbes + probeIdx).

	@param hashIndices Hash indice for the batch of input vectors.
	@param identity Hash indice's corresponding identifications (sequential number starting 0) for this batch of inputs.
	@param dataIdx Non-zero indice of the sparse format.
	@param dataVal Non-zero values of the sparse format.
	@param dataMarker Marks the start index of each vector in dataIdx and dataVal.
	@param numInputEntries Number of input vectors.
	@param numProbes Number of probes per input.
	*/
	void getHash(unsigned int *hashIndices, unsigned int *identity,
		int *dataIdx, float *dataVal, int *dataMarker, int numInputEntries, int numProbes);
	
	/** Obtain hash indice given the (dense) input vector, using OpenCL.

	Hash indice refer to the corresponding "row number" in a hash table, in the form of unsigned integer.
	This function will only be valid when an CPU implementation exists for that type of hashing.
	The outputs indexing is defined as hashIndicesOutputIdx(numHashFamilies, numProbes, numInputs, inputIdx, probeIdx, tb) (unsigned)(numInputs * numProbes * tb + inputIdx * numProbes + probeIdx).

	@param hashIndices_obj Hash indice for the batch of input vectors.
	@param identity_obj Hash indice's corresponding identifications (sequential number starting 0) for this batch of inputs.
	@param input_obj Input vectors concatenated.
	@param numInputEntries Number of input vectors.
	@param numProbes Number of probes per input.
	*/
#ifdef OPENCL_2XX
	void getHash(cl_mem *hashIndices_obj, cl_mem *identity_obj, cl_mem *input_obj, int numInputEntries, int numProbes);
#endif
	
	/** Obtain hash indice given the (dense) input vector, using OpenCL.

	Hash indice refer to the corresponding "row number" in a hash table, in the form of unsigned integer.
	This function will only be valid when an OpenCL implementation exists for that type of hashing,
	and when OpenCL is initialized (clLSH) for that hashing.
	The outputs indexing is defined as hashIndicesOutputIdx(numHashFamilies, numProbes, numInputs, inputIdx, probeIdx, tb) (unsigned)(numInputs * numProbes * tb + inputIdx * numProbes + probeIdx).

	@param hashIndices Hash indice for the batch of input vectors.
	@param identity Hash indice's corresponding identifications (sequential number starting 0) for this batch of inputs.
	@param input Input vectors concatenated.
	@param numInputEntries Number of input vectors.
	@param numProbes Number of probes per input.
	*/
	void getHash(unsigned int *hashIndices, unsigned int *identity, float *input, int numInputEntries, int numProbes);

	/** Constructor. 

	Construct an LSH class for signed random projection. 

	@param hashType For SRP, use 1. 
	@param numHashPerFamily Number of hash (bits) per hashfamily (hash table). 
	@param numHashFamilies Number of hash families (hash tables). 
	@param dimension Dimensionality of input data. 
	@param samFactor samFactor = dimension / samSize, have to be an integer. 
	*/
	LSH(int hashType, int numHashPerFamily, int numHashFamilies, int dimension, int samFactor);  

	/** Constructor.  
	
	Construct an LSH class for optimal densified min-hash (for more details refer to Anshumali Shrivastava, anshu@rice.edu). 
	This hashing scheme is for very sparse and high dimensional data stored in sparse format. 
	
	@param hashType For optimal densified min-hash, use 2. 
	*/
	LSH(int hashType, int _K_in, int _L_in, int _rangePow_in);
	
	/** Initializes LSH hashing with OpenCl environment. 
	
	Takes in initialized OpenCL objects to support OpenCL hash functions, 
	given that the particular type of hashing instantiated have an OpenCL implementation.
	Otherwise an error will be triggered during hashing. 

	@param platforms_lsh Reference to OpenCL cl_platform_id. 
	@param devices_lsh Reference to OpenCL cl_device_id. 
	@param contect_lsh Reference to OpenCl context_lsh. 
	@param program_lsh Reference to OpenCL program_lsh. 
	@param command_queue_lsh Reference to OpenCL cl_command_queue. 
	*/
#ifdef OPENCL_2XX
	void clLSH(cl_platform_id *platforms_lsh, cl_device_id *devices_lsh, cl_context context_lsh, 
		cl_program program_lsh, cl_command_queue command_queue_lsh);
#endif
	
	/** Destructor. 
	
	Does not uninitialize the OpenCL environment (if applicable). 
	*/
	~LSH();
};
