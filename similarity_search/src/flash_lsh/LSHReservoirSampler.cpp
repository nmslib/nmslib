
#include "LSHReservoirSampler.h"
#include "misc.h"
#include "LSHReservoirSampler_config.h"

void LSHReservoirSampler::add(int numInputEntries, int* dataIdx, float* dataVal, int* dataMarker) {
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add]" << std::endl;
#endif
#ifdef PROFILE_READ
	float transfer_time = 0;
#endif

	const int numProbePerTb = numInputEntries * _hashingProbes;

	if ((unsigned) numInputEntries > _maxSamples) {
		printf("[LSHReservoirSampler::add] Input length %d is too large! \n", numInputEntries);
		pause();
		return;
	}

#if defined OPENCL_HASHING || defined OPENCL_HASHTABLE
	cl_mem allprobsHash_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_numTables * numInputEntries * _hashingProbes * sizeof(unsigned int), NULL, &_err);
	cl_mem allprobsIdx_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_numTables * numInputEntries *  _hashingProbes * sizeof(unsigned int), NULL, &_err);
#endif
#if defined CPU_HASHING || defined CPU_TB
	unsigned int* allprobsHash = new unsigned int[_numTables * numInputEntries * _hashingProbes];
	unsigned int* allprobsIdx = new unsigned int[_numTables * numInputEntries * _hashingProbes];
#endif

#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add] Started hashing. " << std::endl;
#endif
#if defined OPENCL_HASHING
#ifdef PROFILE_READ
	auto transfer_begin = Clock::now();
#endif
	cl_mem dataIdx_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numInputEntries * _dimension * sizeof(int), NULL, &_err);
	_err = clEnqueueWriteBuffer(command_queue_gpu, dataIdx_obj, CL_TRUE, 0,
		numInputEntries * _dimension * sizeof(int), dataIdx, 0, NULL, NULL);
	cl_mem dataVal_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numInputEntries * _dimension * sizeof(float), NULL, &_err);
	_err |= clEnqueueWriteBuffer(command_queue_gpu, dataVal_obj, CL_TRUE, 0,
		numInputEntries * _dimension * sizeof(float), dataVal, 0, NULL, NULL);
	cl_mem dataMarker_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		(numInputEntries + 1) * sizeof(int), NULL, &_err);
	_err |= clEnqueueWriteBuffer(command_queue_gpu, dataMarker_obj, CL_TRUE, 0,
		(numInputEntries + 1) * sizeof(int), dataMarker, 0, NULL, NULL);
	clCheckError(_err, "Failed to write sparse input data to memobj!");

#ifdef PROFILE_READ
	auto transfer_end = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin, transfer_end);
#endif

	_hashFamily->getHash(&allprobsHash_gpuobj, &allprobsIdx_gpuobj, 
		&dataIdx_obj, &dataVal_obj, &dataMarker_obj, numInputEntries, _hashingProbes);

	clReleaseMemObject(dataIdx_obj);
	clReleaseMemObject(dataVal_obj);
	clReleaseMemObject(dataMarker_obj);
#elif defined CPU_HASHING
	_hashFamily->getHash(allprobsHash, allprobsIdx, 
		dataIdx, dataVal, dataMarker, numInputEntries, _hashingProbes);
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add] Completed hashing. " << std::endl;
#endif

	/* Extra memory transfer for cross device processing. */
#ifdef PROFILE_READ
	auto transfer_begin_1 = Clock::now();
#endif
#if defined OPENCL_HASHING && defined CPU_TB
	memCpy_uint_g2c(allprobsHash, &allprobsHash_gpuobj, _numTables * numInputEntries * _hashingProbes);
	memCpy_uint_g2c(allprobsIdx, &allprobsIdx_gpuobj, _numTables * numInputEntries * _hashingProbes);
#endif
#if defined CPU_HASHING && defined OPENCL_HASHTABLE
	memCpy_uint_c2g(&allprobsHash_gpuobj, allprobsHash, _numTables * numInputEntries * _hashingProbes);
	memCpy_uint_c2g(&allprobsIdx_gpuobj, allprobsIdx, _numTables * numInputEntries * _hashingProbes);
#endif
#ifdef PROFILE_READ
	auto transfer_end_1 = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin_1, transfer_end_1);
#endif

#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add] Adding to table. " << std::endl;
#endif
#ifdef OPENCL_HASHTABLE
	HashAddGPUTB(&allprobsHash_gpuobj, &allprobsIdx_gpuobj, numProbePerTb, numInputEntries);
#elif defined CPU_TB
	HashAddCPUTB(allprobsHash, allprobsIdx, numProbePerTb, numInputEntries);
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add] Added to table. " << std::endl;
#endif

#if defined OPENCL_HASHING || defined OPENCL_HASHTABLE
	clReleaseMemObject(allprobsHash_gpuobj);
	clReleaseMemObject(allprobsIdx_gpuobj);
#endif
#if defined CPU_HASHING || defined CPU_TB
	delete[] allprobsHash;
	delete[] allprobsIdx;
#endif
	
	_sequentialIDCounter_kernel += numInputEntries;
#ifdef PROFILE_READ
	printf("[LSHReservoirSampler::add] MemTransfer %5.3f ms\n", transfer_time);
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add] Exit. " << std::endl;
#endif
}

void LSHReservoirSampler::add(int numInputEntries, float* input) {
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add]" << std::endl;
#endif
	const int numProbePerTb = numInputEntries * _hashingProbes;

	if ((unsigned) numInputEntries > _maxSamples) { // TODO: Better limit available? 
		printf("[LSHReservoirSampler::add] Input length %d is too large! \n", numInputEntries);
		pause();
		return;
	}

#ifdef PROFILE_READ
	float transfer_time = 0;
#endif

#if defined OPENCL_HASHING || defined OPENCL_HASHTABLE
	cl_mem allprobsHash_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_numTables * numInputEntries * _hashingProbes * sizeof(unsigned int), NULL, &_err);
	cl_mem allprobsIdx_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_numTables * numInputEntries *  _hashingProbes * sizeof(unsigned int), NULL, &_err);
#endif
#if defined CPU_HASHING || defined CPU_TB
	unsigned int* allprobsHash = new unsigned int[_numTables * numInputEntries * _hashingProbes];
	unsigned int* allprobsIdx = new unsigned int[_numTables * numInputEntries * _hashingProbes];
#endif

#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add] Started hashing. " << std::endl;
#endif
#if defined OPENCL_HASHING
#ifdef PROFILE_READ
	auto transfer_begin = Clock::now();
#endif

	cl_mem input_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numInputEntries * _dimension * sizeof(float), NULL, &_err);
	_err = clEnqueueWriteBuffer(command_queue_gpu, input_obj, CL_TRUE, 0,
		numInputEntries * _dimension * sizeof(float), input, 0, NULL, NULL);	
	clCheckError(_err, "Failed to write input data to memobj!");

#ifdef PROFILE_READ
	auto transfer_end = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin, transfer_end);
#endif

	_hashFamily->getHash(&allprobsHash_gpuobj, &allprobsIdx_gpuobj,
		&input_obj, numInputEntries, _hashingProbes);
	clReleaseMemObject(input_obj);

#elif defined CPU_HASHING
	_hashFamily->getHash(allprobsHash, allprobsIdx, input, numInputEntries, _hashingProbes);
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add] Completed hashing. " << std::endl;
#endif

	/* Extra memory transfer for cross device processing. */
#ifdef PROFILE_READ
	auto transfer_begin_1 = Clock::now();
#endif
#if defined OPENCL_HASHING && defined CPU_TB
	memCpy_uint_g2c(allprobsHash, &allprobsHash_gpuobj, _numTables * numInputEntries * _hashingProbes);
	memCpy_uint_g2c(allprobsIdx, &allprobsIdx_gpuobj, _numTables * numInputEntries * _hashingProbes);
#endif
#if defined CPU_HASHING && defined OPENCL_HASHTABLE
	memCpy_uint_c2g(&allprobsHash_gpuobj, allprobsHash, _numTables * numInputEntries * _hashingProbes);
	memCpy_uint_c2g(&allprobsIdx_gpuobj, allprobsIdx, _numTables * numInputEntries * _hashingProbes);
#endif
#ifdef PROFILE_READ
	auto transfer_end_1 = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin_1, transfer_end_1);
#endif

#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add] Adding to table. " << std::endl;
#endif
#ifdef OPENCL_HASHTABLE
	HashAddGPUTB(&allprobsHash_gpuobj, &allprobsIdx_gpuobj, numProbePerTb, numInputEntries);
#elif defined CPU_TB
	HashAddCPUTB(allprobsHash, allprobsIdx, numProbePerTb, numInputEntries);
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add] Added to table. " << std::endl;
#endif

#if defined OPENCL_HASHING || defined OPENCL_HASHTABLE
	clReleaseMemObject(allprobsHash_gpuobj);
	clReleaseMemObject(allprobsIdx_gpuobj);
#endif
#if defined CPU_HASHING || defined CPU_TB
	delete[] allprobsHash;
	delete[] allprobsIdx;
#endif

	_sequentialIDCounter_kernel += numInputEntries;
#ifdef PROFILE_READ
	printf("[LSHReservoirSampler::add] MemTransfer %5.3f ms\n", transfer_time);
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::add] Exit. " << std::endl;
#endif
}

void LSHReservoirSampler::lossy_ann(int numQueryEntries, int* dataIdx, float* dataVal, int* dataMarker, unsigned int* outputs, int k) {
#if defined USE_OPENCL
	std::cout << "[LSHReservoirSampler::lossy_ann] Does not have GPU implementation. " << std::endl;
	return;
#endif

	unsigned int* allprobsHash = new unsigned int[_numTables * numQueryEntries * _queryProbes];
	unsigned int* allprobsIdx = new unsigned int[_numTables * numQueryEntries * _queryProbes];
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::lossy_ann] Started hashing. " << std::endl;
#endif
	_hashFamily->getHash(allprobsHash, allprobsIdx,
		dataIdx, dataVal, dataMarker, numQueryEntries, _queryProbes);
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::lossy_ann] Completed hashing. " << std::endl;
	std::cout << "[LSHReservoirSampler::lossy_ann] Lossy K-selection. " << std::endl;
#endif
	query_frequentitem_cpu_openmp(numQueryEntries, outputs, allprobsHash, k);
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::lossy_ann] Lossy K-selection completes. " << std::endl;
#endif
	delete[] allprobsHash;
	delete[] allprobsIdx;
}

void LSHReservoirSampler::ann(int numQueryEntries, float* queries, unsigned int* outputs, int topk) {
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann]" << std::endl;
#endif
	if ((unsigned) topk > _reservoirSize * _numTables) {
		printf("[LSHReservoirSampler::ann] Maximum k exceeded! %d\n", topk);
		pause();
		return;
	}

#if !defined USE_OPENCL
	unsigned int* allprobsHash = new unsigned int[_numTables * numQueryEntries * _queryProbes];
	unsigned int* allprobsIdx = new unsigned int[_numTables * numQueryEntries * _queryProbes];
	int segmentSize = _numTables * _queryProbes * _reservoirSize;
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Started hashing. " << std::endl;
#endif
	_hashFamily->getHash(allprobsHash, allprobsIdx, queries, numQueryEntries, _queryProbes);
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Completed hashing. " << std::endl;
#endif
	unsigned int* tally = new unsigned int[numQueryEntries * segmentSize];
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Extracting rows. " << std::endl;
#endif
	query_extractRows_cpu_openmp(numQueryEntries, segmentSize, tally, allprobsHash);
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Extracted rows. " << std::endl;
	std::cout << "[LSHReservoirSampler::ann] Started k-selection. " << std::endl;
#endif
	kSelect(tally, outputs, segmentSize, numQueryEntries, topk);
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] K-selection completed. " << std::endl;
#endif
	delete[] allprobsHash;
	delete[] allprobsIdx;
	delete[] tally;
#else

#ifdef PROFILE_READ
	float transfer_time = 0;
#endif

	int segmentSize = _numTables * _queryProbes * _reservoirSize;
	int segmentSizePow2 = smallestPow2(segmentSize); // Pow2 required by sorting. 

#if defined OPENCL_HASHING || defined OPENCL_HASHTABLE
	cl_mem allprobsHash_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_numTables * numQueryEntries * _queryProbes * sizeof(unsigned int), NULL, &_err);
	cl_mem allprobsIdx_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_numTables * numQueryEntries *  _queryProbes * sizeof(unsigned int), NULL, &_err);
#endif
#if defined CPU_HASHING || defined CPU_TB
	unsigned int* allprobsHash = new unsigned int[_numTables * numQueryEntries * _queryProbes];
	unsigned int* allprobsIdx = new unsigned int[_numTables * numQueryEntries * _queryProbes];
#endif

#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Started hashing. " << std::endl;
#endif
#if defined OPENCL_HASHING
#ifdef PROFILE_READ
	auto transfer_begin = Clock::now();
#endif
	cl_mem queries_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueryEntries * _dimension * sizeof(float), NULL, &_err);
	_err = clEnqueueWriteBuffer(command_queue_gpu, queries_obj, CL_TRUE, 0,
		numQueryEntries * _dimension * sizeof(float), queries, 0, NULL, NULL);
	clCheckError(_err, "Failed to write queries data to memobj!");
#ifdef PROFILE_READ
	auto transfer_end = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin, transfer_end);
#endif
	_hashFamily->getHash(&allprobsHash_gpuobj, &allprobsIdx_gpuobj,
		&queries_obj, numQueryEntries, _queryProbes);
	clReleaseMemObject(queries_obj);
#elif defined CPU_HASHING
	_hashFamily->getHash(allprobsHash, allprobsIdx, queries, numQueryEntries, _queryProbes);
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Completed hashing. " << std::endl;
#endif

	/* Extra memory transfer for cross device processing. */
#ifdef PROFILE_READ
	auto transfer_begin_1 = Clock::now();
#endif
#if defined OPENCL_HASHING && defined CPU_TB
	memCpy_uint_g2c(allprobsHash, &allprobsHash_gpuobj, _numTables * numQueryEntries * _queryProbes);
#endif
#if defined CPU_HASHING && defined OPENCL_HASHTABLE
	memCpy_uint_c2g(&allprobsHash_gpuobj, allprobsHash, _numTables * numQueryEntries * _queryProbes);
#endif
#ifdef PROFILE_READ
	auto transfer_end_1 = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin_1, transfer_end_1);
#endif
	
	cl_mem tally_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), NULL, &_err);

#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Extracting rows. " << std::endl;
#endif
#if defined OPENCL_HASHTABLE
	RowsAggregationGPUTB(&allprobsHash_gpuobj, &tally_gpuobj, segmentSizePow2, numQueryEntries);
#elif defined CPU_TB
	RowsAggregationCPUTB(allprobsHash, &tally_gpuobj, segmentSizePow2, numQueryEntries);
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Extracted rows. " << std::endl;
	std::cout << "[LSHReservoirSampler::ann] Started k-selection. " << std::endl;
#endif
#if defined OPENCL_KSELECT
	kSelect(&tally_gpuobj, outputs, segmentSize, segmentSizePow2, numQueryEntries, topk);
#elif defined CPU_KSELECT
	unsigned int *tally = new unsigned int[numQueryEntries * segmentSizePow2];
	memCpy_uint_g2c(tally, &tally_gpuobj, numQueryEntries * segmentSizePow2);
	kSelect(tally, outputs, segmentSizePow2, numQueryEntries, topk);
	delete[] tally;
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] K-selection completed. " << std::endl;
#endif
	clReleaseMemObject(tally_gpuobj);

#if defined OPENCL_HASHTABLE || defined OPENCL_HASHING
	clReleaseMemObject(allprobsHash_gpuobj);
	clReleaseMemObject(allprobsIdx_gpuobj);
#endif
#if defined CPU_TB || defined CPU_HASHING
	delete[] allprobsHash;
	delete[] allprobsIdx;
#endif

#ifdef PROFILE_READ
	printf("[LSHReservoirSampler::ann] MemTransfer %5.3f ms\n", transfer_time);
#endif

#endif // NO_GPU
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Exit. " << std::endl;
#endif
}

void LSHReservoirSampler::ann(int numQueryEntries, int* dataIdx, float* dataVal, int* dataMarker, unsigned int* outputs, int topk) {
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann]" << std::endl;
#endif
	if ((unsigned) topk > _reservoirSize * _numTables) {
		printf("Error: Maximum k exceeded! %d\n", topk);
		pause();
		return;
	}

#if !defined USE_OPENCL
	unsigned int* allprobsHash = new unsigned int[_numTables * numQueryEntries * _queryProbes];
	unsigned int* allprobsIdx = new unsigned int[_numTables * numQueryEntries * _queryProbes];
	int segmentSize = _numTables * _queryProbes * _reservoirSize;
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Started hashing. " << std::endl;
#endif
	_hashFamily->getHash(allprobsHash, allprobsIdx,
		dataIdx, dataVal, dataMarker, numQueryEntries, _queryProbes);
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Completed hashing. " << std::endl;
#endif
	unsigned int* tally = new unsigned int[numQueryEntries * segmentSize];
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Extracting rows. " << std::endl;
#endif
	query_extractRows_cpu_openmp(numQueryEntries, segmentSize, tally, allprobsHash);
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Extracted rows. " << std::endl;
	std::cout << "[LSHReservoirSampler::ann] Started k-selection. " << std::endl;
#endif
	kSelect(tally, outputs, segmentSize, numQueryEntries, topk);
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Completed k-selection. " << std::endl;
#endif
	delete[] allprobsHash;
	delete[] allprobsIdx;
	delete[] tally;
#else

#ifdef PROFILE_READ
	float transfer_time = 0;
#endif

	int segmentSize = _numTables * _queryProbes * _reservoirSize;
	int segmentSizePow2 = smallestPow2(segmentSize); // Pow2 required by sorting. 

#if defined OPENCL_HASHING || defined OPENCL_HASHTABLE
	cl_mem allprobsHash_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_numTables * numQueryEntries * _queryProbes * sizeof(unsigned int), NULL, &_err);
	cl_mem allprobsIdx_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_numTables * numQueryEntries *  _queryProbes * sizeof(unsigned int), NULL, &_err);
#endif
#if defined CPU_HASHING || defined CPU_TB
	unsigned int* allprobsHash = new unsigned int[_numTables * numQueryEntries * _queryProbes];
	unsigned int* allprobsIdx = new unsigned int[_numTables * numQueryEntries * _queryProbes];
#endif
	
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Started hashing. " << std::endl;
#endif
#if defined OPENCL_HASHING
#ifdef PROFILE_READ
	auto transfer_begin = Clock::now();
#endif
	cl_mem dataIdx_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueryEntries * _dimension * sizeof(int), NULL, &_err);
	_err = clEnqueueWriteBuffer(command_queue_gpu, dataIdx_obj, CL_TRUE, 0,
		numQueryEntries * _dimension * sizeof(int), dataIdx, 0, NULL, NULL);
	cl_mem dataVal_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueryEntries * _dimension * sizeof(float), NULL, &_err);
	_err |= clEnqueueWriteBuffer(command_queue_gpu, dataVal_obj, CL_TRUE, 0,
		numQueryEntries * _dimension * sizeof(float), dataVal, 0, NULL, NULL);
	cl_mem dataMarker_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		(numQueryEntries + 1) * sizeof(int), NULL, &_err);
	_err |= clEnqueueWriteBuffer(command_queue_gpu, dataMarker_obj, CL_TRUE, 0,
		(numQueryEntries + 1) * sizeof(int), dataMarker, 0, NULL, NULL);
	clCheckError(_err, "Failed to write sparse input data to memobj!");
#ifdef PROFILE_READ
	auto transfer_end = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin, transfer_end);
#endif
	_hashFamily->getHash(&allprobsHash_gpuobj, &allprobsIdx_gpuobj,
		&dataIdx_obj, &dataVal_obj, &dataMarker_obj, numQueryEntries, _queryProbes);
	clReleaseMemObject(dataIdx_obj);
	clReleaseMemObject(dataVal_obj);
	clReleaseMemObject(dataMarker_obj);
#elif defined CPU_HASHING
	_hashFamily->getHash(allprobsHash, allprobsIdx,
		dataIdx, dataVal, dataMarker, numQueryEntries, _queryProbes);
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Completed hashing. " << std::endl;
#endif

	/* Extra memory transfer for cross device processing. */
#ifdef PROFILE_READ
	auto transfer_begin_1 = Clock::now();
#endif
#if defined OPENCL_HASHING && defined CPU_TB
	memCpy_uint_g2c(allprobsHash, &allprobsHash_gpuobj, _numTables * numQueryEntries * _queryProbes);
#endif
#if defined CPU_HASHING && defined OPENCL_HASHTABLE
	memCpy_uint_c2g(&allprobsHash_gpuobj, allprobsHash, _numTables * numQueryEntries * _queryProbes);
#endif
#ifdef PROFILE_READ
	auto transfer_end_1 = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin_1, transfer_end_1);
#endif

	cl_mem tally_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), NULL, &_err);

#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Extracting rows. " << std::endl;
#endif
#if defined OPENCL_HASHTABLE
	RowsAggregationGPUTB(&allprobsHash_gpuobj, &tally_gpuobj, segmentSizePow2, numQueryEntries);
#elif defined CPU_TB
	RowsAggregationCPUTB(allprobsHash, &tally_gpuobj, segmentSizePow2, numQueryEntries);
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Extracted rows. " << std::endl;
	std::cout << "[LSHReservoirSampler::ann] Started k-selection. " << std::endl;
#endif
#if defined OPENCL_KSELECT
	kSelect(&tally_gpuobj, outputs, segmentSize, segmentSizePow2, numQueryEntries, topk);	
#elif defined CPU_KSELECT
	unsigned int *tally = new unsigned int[numQueryEntries * segmentSizePow2];
	memCpy_uint_g2c(tally, &tally_gpuobj, numQueryEntries * segmentSizePow2);
	kSelect(tally, outputs, segmentSizePow2, numQueryEntries, topk);
	delete[] tally;
#endif
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Completed k-selection. " << std::endl;
#endif
	clReleaseMemObject(tally_gpuobj);

#if defined OPENCL_HASHTABLE || defined OPENCL_HASHING
	clReleaseMemObject(allprobsHash_gpuobj);
	clReleaseMemObject(allprobsIdx_gpuobj);
#endif
#if defined CPU_TB || defined CPU_HASHING
	delete[] allprobsHash;
	delete[] allprobsIdx;
#endif

#ifdef PROFILE_READ
	printf("[LSHReservoirSampler::ann] MemTransfer %5.3f ms\n", transfer_time);
#endif

#endif // NO_GPU
#if defined DEBUG
	std::cout << "[LSHReservoirSampler::ann] Exit" << std::endl;
#endif
}

#ifdef OPENCL_2XX
void LSHReservoirSampler::ann_debug(int numQueryEntries, int* dataIdx, float* dataVal, int* dataMarker, int topk) {


	int segmentSize = _numTables * _queryProbes * _reservoirSize;
	int segmentSizePow2 = smallestPow2(segmentSize); // Pow2 required by sorting. 

	unsigned int* allprobsHash = new unsigned int[_numTables * numQueryEntries * _queryProbes];
	unsigned int* allprobsIdx = new unsigned int[_numTables * numQueryEntries * _queryProbes];

	_hashFamily->getHash(allprobsHash, allprobsIdx,
		dataIdx, dataVal, dataMarker, numQueryEntries, _queryProbes);

	cl_mem tally_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), NULL, &_err);

	RowsAggregationCPUTB(allprobsHash, &tally_gpuobj, segmentSizePow2, numQueryEntries);
	unsigned int* tally = new unsigned int[numQueryEntries * segmentSizePow2];
	memCpy_uint_g2c(tally, &tally_gpuobj, numQueryEntries * segmentSizePow2);

	kSelect_debug(&tally_gpuobj, tally, segmentSize, segmentSizePow2, numQueryEntries, topk);

	delete[] tally;
	delete[] allprobsHash;
	delete[] allprobsIdx;

}
#endif
