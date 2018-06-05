
#include "LSHReservoirSampler.h"
#include "misc.h"
#include "indexing.h"
#include "LSHReservoirSampler_config.h"
#include "FrequentItems.h"
#include <algorithm>

//#define DEBUG_SAMPLING_SEGFAULT_CPU
//#define DEBUG_TALLY

void LSHReservoirSampler::reservoir_sampling_cpu_openmp(unsigned int *allprobsHash, unsigned int *allprobsIdx,
	unsigned int *storelog, int numProbePerTb) {
#ifdef PROFILE_READ_DETAILED
	auto begin = Clock::now();
#endif
	unsigned int counter, allocIdx, reservoirRandNum, TB, hashIdx, inputIdx, ct, reservoir_full, location;
#pragma omp parallel private(TB, hashIdx, inputIdx, ct, allocIdx, counter, reservoir_full, reservoirRandNum, location)
#pragma	omp for
	for (int probeIdx = 0; probeIdx < numProbePerTb; probeIdx++) {
		for (unsigned int tb = 0; tb < _numTables; tb++) {

			TB = numProbePerTb * tb;

#if defined DEBUG_SAMPLING_SEGFAULT_CPU
			unsigned int allprobsIdxCheck = allprobsHashSimpleIdx(numProbePerTb, tb, probeIdx);
			if (allprobsIdxCheck > _numTables * numProbePerTb-1 || allprobsIdxCheck < 0) {
				printf("allprobsHashSimpleIdx %u >= %u or < 0\n", allprobsIdxCheck, _numTables * numProbePerTb);
			}
#endif

			hashIdx = allprobsHash[allprobsHashSimpleIdx(numProbePerTb, tb, probeIdx)];
			inputIdx = allprobsIdx[allprobsHashSimpleIdx(numProbePerTb, tb, probeIdx)];
			ct = 0;

#if defined DEBUG_SAMPLING_SEGFAULT_CPU
			if (hashIdx > _numReservoirs - 1 || hashIdx < 0) {
				printf("hashIdx %u >= %u\n", hashIdx, _numReservoirs);
			}

			unsigned long long tablePointerAccess = tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b);
			if (tablePointerAccess > _tablePointerMax - 1 || tablePointerAccess < 0) {
				printf("tablePointerAccess %llu >= %llu or < 0\n", tablePointerAccess, _tablePointerMax);
				printf("_tablePointers[tablePointersIdx(_numReservoirsHashed(%u), hashIdx(%u), tb(%u), _sechash_a(%u), _sechash_b(%u))]\n", 
					_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b);
			}
#endif
			/* Allocate the reservoir if non-existent. */
			omp_set_lock(_tablePointersLock + tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b));
			allocIdx = _tablePointers[tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b)];
			if (allocIdx == TABLENULL) {
				allocIdx = _tableMemAllocator[tableMemAllocatorIdx(tb)];
				_tableMemAllocator[tableMemAllocatorIdx(tb)] ++;
				_tablePointers[tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b)] = allocIdx;
			}
			omp_unset_lock(_tablePointersLock + tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b));

#if defined DEBUG_SAMPLING_SEGFAULT_CPU
			unsigned long long tableCounterLockIdxVerify = tableCountersLockIdx(tb, allocIdx, _aggNumReservoirs);
			if (tableCounterLockIdxVerify > _tableMemReservoirMax - 1 || tableCounterLockIdxVerify < 0) {
				printf("tableCountersLockIdx %llu >= %llu or < 0\n", tableCounterLockIdxVerify, _tableMemReservoirMax);
				printf("tableCountersLockIdx(tb(%u), allocIdx(%u), _aggNumReservoirs(%u))\n", tb, allocIdx, _aggNumReservoirs);
			}
#endif
			// ATOMIC: Obtain the counter, and increment the counter. (Counter initialized to 0 automatically). 
			// Counter counts from 0 to currentCount-1. 
			omp_set_lock(_tableCountersLock + tableCountersLockIdx(tb, allocIdx, _aggNumReservoirs));

#if defined DEBUG_SAMPLING_SEGFAULT_CPU
			unsigned long long tableAccess = tableMemCtIdx(tb, allocIdx, _aggNumReservoirs);
			if (tableAccess > _tableMemMax - 1 || tableAccess < 0) {
				printf("tableAccess %llu >= %llu\n", tableAccess, _tableMemMax);
				printf("tableMemCtIdx(tb(%u), allocIdx(%u), _aggNumReservoirs(%u))\n",
					tb, allocIdx, _aggNumReservoirs);
			}
#endif
			counter = _tableMem[tableMemCtIdx(tb, allocIdx, _aggNumReservoirs)]; // Potentially overflowable.
			_tableMem[tableMemCtIdx(tb, allocIdx, _aggNumReservoirs)] ++;
			omp_unset_lock(_tableCountersLock + tableCountersLockIdx(tb, allocIdx, _aggNumReservoirs));

			// The counter here is the old counter. Current count is already counter + 1. 
			// If current count is larger than _reservoirSize, current item needs to be sampled. 
			//reservoir_full = (counter + 1) > _reservoirSize;

			reservoirRandNum = _global_rand[std::min((unsigned int)(_maxReservoirRand-1), counter)]; // Overflow prevention. 

			if ((counter + 1) > _reservoirSize) { // Reservoir full. 
				location = reservoirRandNum;
			}
			else {
				location = counter;
			}

			//location = reservoir_full * (reservoirRandNum)+(1 - reservoir_full) * counter;

#if defined DEBUG_SAMPLING_SEGFAULT_CPU
			unsigned int storelogALimit = _numTables * 4 * numProbePerTb;
			unsigned int storelogAccess = storelogHashIdxIdx(numProbePerTb, probeIdx, tb);
			if (storelogAccess > storelogALimit - 1 || storelogAccess < 0) {
				printf("storelogAccess %llu >= %llu\n", storelogAccess, storelogALimit);
				printf("storelogHashIdxIdx(numProbePerTb(%d), probeIdx(%d), tb(%d))\n", numProbePerTb, probeIdx, tb);
			}
#endif		

			storelog[storelogIdIdx(numProbePerTb, probeIdx, tb)] = inputIdx;
			storelog[storelogCounterIdx(numProbePerTb, probeIdx, tb)] = counter;
			storelog[storelogLocationIdx(numProbePerTb, probeIdx, tb)] = location;
			storelog[storelogHashIdxIdx(numProbePerTb, probeIdx, tb)] = hashIdx;

		}
	}

#ifdef PROFILE_READ_DETAILED
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[Timer] reservoir_sampling_cpu_openmp took %5.3f ms\n", etime);
#endif
}

void LSHReservoirSampler::add_table_cpu_openmp(unsigned int *storelog, int numProbePerTb) {

#ifdef PROFILE_READ_DETAILED
	auto begin = Clock::now();
#endif
	unsigned int id, hashIdx, allocIdx;
	unsigned locCapped;
//#pragma omp parallel private(allocIdx, id, hashIdx, locCapped)
//#pragma	omp for
	for (int probeIdx = 0; probeIdx < numProbePerTb; probeIdx++) {
		for (unsigned int tb = 0; tb < _numTables; tb++) {

#if defined DEBUG_SAMPLING_SEGFAULT_CPU
			unsigned int storelogALimit = _numTables * 4 * numProbePerTb;
			unsigned int storelogAccess = storelogHashIdxIdx(numProbePerTb, probeIdx, tb);
			if (storelogAccess > storelogALimit - 1 || storelogAccess < 0) {
				printf("storelogAccess %llu >= %llu\n", storelogAccess, storelogALimit);
				printf("storelogHashIdxIdx(numProbePerTb(%d), probeIdx(%d), tb(%d))\n", numProbePerTb, probeIdx, tb);
			}
#endif
			id = storelog[storelogIdIdx(numProbePerTb, probeIdx, tb)];
			hashIdx = storelog[storelogHashIdxIdx(numProbePerTb, probeIdx, tb)];
			allocIdx = _tablePointers[tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b)];
			// If item_i spills out of the reservoir, it is capped to the dummy location at _reservoirSize. 
			locCapped = storelog[storelogLocationIdx(numProbePerTb, probeIdx, tb)];

#if defined DEBUG_SAMPLING_SEGFAULT_CPU
			unsigned long long tableAccess = tableMemResIdx(tb, allocIdx, _aggNumReservoirs);
			if (tableAccess > _tableMemMax - 1 || tableAccess < 0) {
				printf("tableAccess %llu >= %llu\n", tableAccess, _tableMemMax);
				printf("tableMemResIdx(tb(%u), allocIdx(%u), _aggNumReservoirs(%u))\n",
					tb, allocIdx, _aggNumReservoirs);
			}
#endif
			if (locCapped < _reservoirSize) {
				_tableMem[tableMemResIdx(tb, allocIdx, _aggNumReservoirs) + locCapped] = id + _sequentialIDCounter_kernel;
			}
		}
	}

#ifdef PROFILE_READ_DETAILED
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[Timer] add_table_cpu_openmp took %5.3f ms\n", etime);
#endif
}

#ifdef OPENCL_2XX
void LSHReservoirSampler::reservoir_sampling_gpu(cl_mem *allprobsHash_obj, cl_mem *allprobsIdx_obj,
	cl_mem *storelog_obj, int numProbePerTb) {
#ifdef PROFILE_READ_DETAILED
	auto begin = Clock::now();
#endif
	_err = clSetKernelArg(kernel_reservoir, 0, sizeof(cl_mem), (void *)&_tableMem_obj);
	_err |= clSetKernelArg(kernel_reservoir, 1, sizeof(cl_mem), (void *)&_tablePointers_obj);
	_err |= clSetKernelArg(kernel_reservoir, 2, sizeof(cl_mem), (void *)&_tableMemAllocator_obj);
	_err |= clSetKernelArg(kernel_reservoir, 3, sizeof(cl_mem), (void *)allprobsHash_obj);
	_err |= clSetKernelArg(kernel_reservoir, 4, sizeof(cl_mem), (void *)allprobsIdx_obj);
	_err |= clSetKernelArg(kernel_reservoir, 5, sizeof(cl_mem), (void *)storelog_obj);
	_err |= clSetKernelArg(kernel_reservoir, 6, sizeof(cl_mem), (void *)&_globalRand_obj);
	_err |= clSetKernelArg(kernel_reservoir, 7, sizeof(unsigned int), (void *)&_numReservoirsHashed);
	_err |= clSetKernelArg(kernel_reservoir, 8, sizeof(unsigned int), (void *)&numProbePerTb);
	_err |= clSetKernelArg(kernel_reservoir, 9, sizeof(unsigned int), (void *)&_aggNumReservoirs);
	_err |= clSetKernelArg(kernel_reservoir, 10, sizeof(unsigned int), (void *)&_maxReservoirRand);
	_err |= clSetKernelArg(kernel_reservoir, 11, sizeof(unsigned int), (void *)&_sechash_a);
	_err |= clSetKernelArg(kernel_reservoir, 12, sizeof(unsigned int), (void *)&_sechash_b);
	_err |= clSetKernelArg(kernel_reservoir, 13, sizeof(unsigned int), (void *)&_reservoirSize);
	_err |= clSetKernelArg(kernel_reservoir, 14, sizeof(unsigned int), (void *)&_numSecHash);
	clCheckError(_err, "Failed to set kernel_reservoir arguments!");

	size_t gsize_kernel_reservoir[2] = { numProbePerTb, _numTables };
	// size_t lsize_kernel_reservoir[2] = { 32, _numTables }; // TODO. 
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_reservoir, 2, NULL,
		gsize_kernel_reservoir, NULL, 0, NULL, NULL);
	clFinish(command_queue_gpu);
	clCheckError(_err, "kernel_reservoir failed!");

#ifdef PROFILE_READ_DETAILED
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[Timer] kernel_reservoir took %5.3f ms\n", etime);
	kernelBandWidth("kernel_reservoir",
		(float)2 * numProbePerTb * _numTables * sizeof(unsigned int) + // allProbesHash / Idx
		(float)numProbePerTb * _numTables * sizeof(unsigned int) +  // tableCounter accesses
		(float)numProbePerTb * _numTables * sizeof(unsigned int), // reservoirRand accesses
		(float)_numTables * 4 * numProbePerTb * sizeof(unsigned int), // Storelog
		etime);
#endif
}

void LSHReservoirSampler::add_table_gpu(cl_mem *storelog_obj, int numProbePerTb) {

#ifdef PROFILE_READ_DETAILED
	auto begin = Clock::now();
#endif
	_err = clSetKernelArg(kernel_addtable, 0, sizeof(cl_mem), (void *)&_tablePointers_obj);
	_err |= clSetKernelArg(kernel_addtable, 1, sizeof(cl_mem), (void *)&_tableMem_obj);
	_err |= clSetKernelArg(kernel_addtable, 2, sizeof(cl_mem), (void *)storelog_obj);
	_err |= clSetKernelArg(kernel_addtable, 3, sizeof(unsigned int), (void *)&numProbePerTb);
	_err |= clSetKernelArg(kernel_addtable, 4, sizeof(unsigned int), (void *)&_numReservoirsHashed);
	_err |= clSetKernelArg(kernel_addtable, 5, sizeof(unsigned int), (void *)&_aggNumReservoirs);
	_err |= clSetKernelArg(kernel_addtable, 6, sizeof(unsigned int), (void *)&_sequentialIDCounter_kernel);
	_err |= clSetKernelArg(kernel_addtable, 7, sizeof(unsigned int), (void *)&_sechash_a);
	_err |= clSetKernelArg(kernel_addtable, 8, sizeof(unsigned int), (void *)&_sechash_b);
	_err |= clSetKernelArg(kernel_addtable, 9, sizeof(unsigned int), (void *)&_reservoirSize);
	_err |= clSetKernelArg(kernel_addtable, 10, sizeof(unsigned int), (void *)&_numSecHash);
	clCheckError(_err, "Failed to set kernel_addtable arguments!");

	size_t gsize_kernel_addtable[2] = { _numTables, numProbePerTb };
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_addtable, 2, NULL,
		gsize_kernel_addtable, NULL, 0, NULL, NULL);
	clFinish(command_queue_gpu);
	clCheckError(_err, "kernel_addtable failed!");

#ifdef PROFILE_READ_DETAILED
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[Timer] kernel_addtable took %5.3f ms\n", etime);

	kernelBandWidth("kernel_addtable",
		(float)_numTables * 1 * numProbePerTb * sizeof(unsigned int) * 4, // All of the storelog. 
		(float)_numTables * 1 * numProbePerTb * sizeof(unsigned int) * 0.5, // Probabilistic store, p = 0.5
		etime);
#endif
}


void LSHReservoirSampler::query_taketopk(int numQueryEntries, int segmentSizePow2, int topk,
	cl_mem *talley_obj, cl_mem *talleyCount_obj, unsigned int *topItems) {

	int topkplus1 = topk + 1;

#ifdef PROFILE_READ_DETAILED
	float etime;
	auto begin = Clock::now();
#endif

	_err = clSetKernelArg(kernel_taketopk, 0, sizeof(cl_mem), (void *)talley_obj);
	_err |= clSetKernelArg(kernel_taketopk, 1, sizeof(cl_mem), (void *)talleyCount_obj);
	_err |= clSetKernelArg(kernel_taketopk, 2, sizeof(int), (void *)&segmentSizePow2);
	_err |= clSetKernelArg(kernel_taketopk, 3, sizeof(int), (void *)&topkplus1);

	clCheckError(_err, "Failed to set kernel_taketopk arguments!");

	size_t gsize_kernel_taketopk[1] = { topkplus1 * numQueryEntries };
	size_t lsize_kernel_taketopk[1] = { topkplus1 };
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_taketopk, 1, NULL,
		gsize_kernel_taketopk, lsize_kernel_taketopk, 0, NULL, NULL);
	clFinish(command_queue_gpu);
	clCheckError(_err, "kernel_taketopk failed!");

#ifdef PROFILE_READ_DETAILED
	clFinish(command_queue_gpu);
	auto end = Clock::now();
	etime = (end - begin).count() / (float)1000000;
	printf("[Timer] kernel_taketopk took %5.3f ms\n", etime);
	kernelBandWidth("kernel_taketopk",
		(float)topk * numQueryEntries * sizeof(int),
		(float)topk * numQueryEntries * sizeof(int),
		etime);
#endif

	unsigned int *outBuffer = new unsigned int[numQueryEntries * topkplus1];

#ifdef PROFILE_READ_DETAILED
	begin = Clock::now();
#endif
	_err = clEnqueueReadBuffer(command_queue_gpu, *talleyCount_obj, CL_TRUE, 0,
		numQueryEntries * topkplus1 * sizeof(unsigned int), outBuffer, 0, NULL, NULL);

#ifdef PROFILE_READ_DETAILED
	clFinish(command_queue_gpu);
	end = Clock::now();
	etime = (end - begin).count() / (float)1000000;
	printf("[Timer] Reading outputs_obj(topkplus1 only) took %5.3f ms\n", etime);
#endif

	int ss;
	int ct = 0;
	int dirr = 0;
	for (int i = 0; i < numQueryEntries; i++) {
		if (dirr) {
			if (outBuffer[i * topkplus1] == 0) { // The first item is spurious. 
				ss = 1;
			}
			else {
				ss = 0;
			}
			ct = 0;
			for (int k = ss; k < topk + ss; k++) {
				topItems[i * topk + ct] = outBuffer[i * topkplus1 + k];
				ct++;
			}
		}
		else {
			if (outBuffer[i * topkplus1 + topkplus1 - 1] == 0) {
				ss = 1;
			}
			else {
				ss = 0;
			}
			ct = 0;
			for (int k = topkplus1 - 1 - ss; k > topkplus1 - 1 - topk - ss; k--) {
				topItems[i * topk + ct] = outBuffer[i * topkplus1 + k];
				ct++;
			}
		}
		dirr = !dirr;
	}

}

void LSHReservoirSampler::query_tallyNaive(int segmentSize, int numQueryEntries, cl_mem *talley_obj, cl_mem *talleyCount_obj, cl_mem *queue_obj) {
	_err = clSetKernelArg(kernel_tally_naive, 0, sizeof(cl_mem), (void *)talley_obj);
	_err |= clSetKernelArg(kernel_tally_naive, 1, sizeof(cl_mem), (void *)talleyCount_obj);
	_err |= clSetKernelArg(kernel_tally_naive, 2, sizeof(cl_mem), (void *)queue_obj);
	_err |= clSetKernelArg(kernel_tally_naive, 3, sizeof(int), (void *)&segmentSize);
	clCheckError(_err, "Failed to set kernel_tally_naive arguments!");

	size_t gsize_kernel_tally_naive[1] = { numQueryEntries };
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_tally_naive, 1, NULL,
		gsize_kernel_tally_naive, NULL, 0, NULL, NULL);
	clFinish(command_queue_gpu);
	clCheckError(_err, "kernel_tally_naive failed!");
}
#endif

void LSHReservoirSampler::mock_markdiff(unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSizePow2) {

	/* For each and every element. */
	for (int gIdx = 0; gIdx < numQueryEntries * segmentSizePow2; gIdx++) {

		/* Index inside each query segment, gIdx % segmentSizePow2. */
		unsigned int localQueueIdx = gIdx & _segmentSizeModulor; 

		/* Record differences, except for the first element in the queue. */
		if (localQueueIdx != 0) {
			tallyCnt[gIdx] = (tally[gIdx] != tally[gIdx - 1]) ? gIdx : -1;
		}
		else { // The first element, no spot of comparison. 
			tallyCnt[gIdx] = gIdx;
		}
	}
}

void LSHReservoirSampler::mock_agg(unsigned int *g_queryCt, unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSizePow2) {

	for (int i = 0; i < numQueryEntries; i++) {

		int counter = 0;	// To record the number of valid elements.
		int gIdx;			// Temporary global index.  
		for (int k = 0; k < segmentSizePow2; k++) {
			gIdx = i * segmentSizePow2 + k;

			if (tallyCnt[gIdx] != -1) { // If difference marked. 
				tallyCnt[i * segmentSizePow2 + counter] = tallyCnt[gIdx];
				tally[i * segmentSizePow2 + counter] = tally[gIdx];
				counter++;
			}
		}

		// Record the number of compacted elements. 
		g_queryCt[i] = counter;

		// Mark all following as zeros. 
		for (; counter < segmentSizePow2; counter++) {
			tallyCnt[i * segmentSizePow2 + counter] = 0;
			tally[i * segmentSizePow2 + counter] = 0;
		}
	}

}

void LSHReservoirSampler::mock_sub(unsigned int *g_queryCt, unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSize, int segmentSizePow2) {
	
	for (int i = 0; i < numQueryEntries; i++) {
		for (int k = 0; k < segmentSizePow2; k++) {

			int gIdx = i * segmentSizePow2 + k;	

			if (k < (g_queryCt[i] - 1)) { // If is in the valid range. 
				tallyCnt[gIdx] = tallyCnt[gIdx + 1] - tallyCnt[gIdx];
			}
			else if (k != (g_queryCt[i] - 1)) { // At k >= queryCt[i], fill with zero to prevent interfering with sorting. 
				tallyCnt[gIdx] = 0;
				tally[gIdx] = 0;
			}
			else { // At k == (g_queryCt[i] - 1). 
				tallyCnt[gIdx] = (i + 1) * segmentSizePow2 - tallyCnt[gIdx]; // Very important - *(i + 1)
			}
		}
	}
}

//#define DEBUG_TALLY
#ifdef OPENCL_2XX
void LSHReservoirSampler::query_tallyReduction(int numQueryEntries, int segmentSize, int segmentSizePow2,
	cl_mem *talley_obj, cl_mem *talleyCount_obj) {

	/* CPU Version, for debugging. */
	///* Copy CPU result to GPU. */
	//unsigned int* tallyMock = new unsigned int[segmentSizePow2 * numQueryEntries];
	//unsigned int* tallyCntMock = new unsigned int[segmentSizePow2 * numQueryEntries];
	//unsigned int* queryPosition = new unsigned int[numQueryEntries]();
	//memCpy_uint_g2c(tallyMock, talley_obj, segmentSizePow2 * numQueryEntries);
	//memCpy_uint_g2c(tallyCntMock, talleyCount_obj, segmentSizePow2 * numQueryEntries);
	//mock_markdiff(tallyCntMock, tallyMock, numQueryEntries, segmentSizePow2);
	//mock_agg(queryPosition, tallyCntMock, tallyMock, numQueryEntries, segmentSizePow2);
	//mock_sub(queryPosition, tallyCntMock, tallyMock, numQueryEntries, segmentSize, segmentSizePow2);
	//memCpy_uint_c2g(talleyCount_obj, tallyCntMock, numQueryEntries * segmentSizePow2);
	//memCpy_uint_c2g(talley_obj, tallyMock, numQueryEntries * segmentSizePow2);
	//return;

	cl_mem queryCt_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueryEntries * sizeof(unsigned int), NULL, &_err);

	cl_mem tallyBuffer_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), NULL, &_err);

	/* Get difference. */
	int numWiPerWg = wg_segSize / l_segSize;
#ifdef PROFILE_READ_DETAILED
	float etime;
#endif

#ifdef DEBUG_TALLY
	/* Compute markdiff result using CPU. */
	unsigned int* queuePreMarkdiffExtract = new unsigned int[segmentSizePow2 * numQueryEntries];
	unsigned int* queuePreMarkdiffCntExtract = new unsigned int[segmentSizePow2 * numQueryEntries];
	memCpy_uint_g2c(queuePreMarkdiffExtract, talley_obj, segmentSizePow2 * numQueryEntries);
	memCpy_uint_g2c(queuePreMarkdiffCntExtract, &tallyBuffer_obj, segmentSizePow2 * numQueryEntries);
	mock_markdiff(queuePreMarkdiffCntExtract, queuePreMarkdiffExtract, numQueryEntries, segmentSizePow2);
#endif

#ifdef PROFILE_READ_DETAILED
	auto begin = Clock::now();
#endif
	_err = clSetKernelArg(kernel_markdiff, 0, sizeof(cl_mem), (void *)talley_obj);
	_err |= clSetKernelArg(kernel_markdiff, 1, sizeof(cl_mem), (void *)&tallyBuffer_obj);
	_err |= clSetKernelArg(kernel_markdiff, 2, sizeof(int), (void *)&segmentSizePow2);
	_err |= clSetKernelArg(kernel_markdiff, 3, sizeof(int), (void *)&_segmentSizeModulor);

	clCheckError(_err, "Failed to set kernel_markdiff arguments!");
	size_t gsize_kernel_markdiff[1] = { segmentSizePow2 * numQueryEntries };
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_markdiff, 1, NULL,
		gsize_kernel_markdiff, NULL, 0, NULL, NULL);
	clCheckError(_err, "kernel_markdiff failed!");
	clFinish(command_queue_gpu);
#ifdef PROFILE_READ_DETAILED
	auto end = Clock::now();
	etime = (end - begin).count() / (float)1000000;
	printf("[Timer] kernel_markdiff took %5.3f ms\n", etime);
	kernelBandWidth("kernel_markdiff",
		(float)2 * segmentSizePow2 * numQueryEntries * sizeof(unsigned int), // queueSorted * 2
		(float)segmentSizePow2 * numQueryEntries * sizeof(unsigned int), // talleyCount
		etime);
#endif

#ifdef DEBUG_TALLY
	/* visualize marked diff. */
	printf("\n");
	printf("<<< Marked Diffs >>>\n");
	unsigned int *queueMarkDiffExtract = new unsigned int[numQueryEntries * segmentSizePow2];
	unsigned int *queueMarkDiffCntExtract = new unsigned int[numQueryEntries * segmentSizePow2];
	_err = clEnqueueReadBuffer(command_queue_gpu, *talley_obj, CL_TRUE, 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), queueMarkDiffExtract, 0, NULL, NULL);
	_err = clEnqueueReadBuffer(command_queue_gpu, tallyBuffer_obj, CL_TRUE, 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), queueMarkDiffCntExtract, 0, NULL, NULL);

	/* Verify with CPU. */
	for (int gIdx = 0; gIdx < numQueryEntries * segmentSizePow2; gIdx++) {
		if (queuePreMarkdiffExtract[gIdx] != queueMarkDiffExtract[gIdx]) {
			std::cout << "Markdiff error at " << gIdx <<
				": cpu " << queuePreMarkdiffExtract[gIdx] << "-" << queuePreMarkdiffCntExtract[gIdx] <<
				", gpu " << queueMarkDiffExtract[gIdx] << "-" << queueMarkDiffCntExtract[gIdx] <<
				std::endl;
		}
	}
	delete[] queuePreMarkdiffExtract;
	delete[] queuePreMarkdiffCntExtract;

	delete[] queueMarkDiffExtract;
	delete[] queueMarkDiffCntExtract;
	pause();
#endif

	/* Compact difference. */
#ifdef DEBUG_TALLY
	/* Aggdiff precomputation using CPU. */
	unsigned int *queuePreAggExtract = new unsigned int[numQueryEntries * segmentSizePow2];
	unsigned int *countPreAggExtract = new unsigned int[numQueryEntries * segmentSizePow2];
	_err = clEnqueueReadBuffer(command_queue_gpu, *talley_obj, CL_TRUE, 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), queuePreAggExtract, 0, NULL, NULL);
	_err = clEnqueueReadBuffer(command_queue_gpu, tallyBuffer_obj, CL_TRUE, 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), countPreAggExtract, 0, NULL, NULL);

	unsigned int *tallyPreAgg = new unsigned int[numQueryEntries * segmentSizePow2]();

	unsigned int *gQueryEntry = new unsigned int[numQueryEntries];
	mock_agg(gQueryEntry, countPreAggExtract, queuePreAggExtract, numQueryEntries, segmentSizePow2);
#endif

#ifdef PROFILE_READ_DETAILED
	begin = Clock::now();
#endif

	_err = clSetKernelArg(kernel_aggdiff, 0, sizeof(cl_mem), (void *)talley_obj);
	_err |= clSetKernelArg(kernel_aggdiff, 1, sizeof(cl_mem), (void *)&tallyBuffer_obj);
	_err |= clSetKernelArg(kernel_aggdiff, 2, sizeof(cl_mem), (void *)&queryCt_obj);
	_err |= clSetKernelArg(kernel_aggdiff, 3, wg_segSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_aggdiff, 4, wg_segSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_aggdiff, 5, wg_segSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_aggdiff, 6, wg_segSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_aggdiff, 7, wg_segSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_aggdiff, 8, numWiPerWg * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_aggdiff, 9, 2 * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_aggdiff, 10, sizeof(unsigned int), (void *)&segmentSizePow2);
	clCheckError(_err, "Failed to set kernel_aggdiff arguments!");

	size_t gsize_kernel_aggdiff_0[1] = { numWiPerWg * numQueryEntries };
	size_t lsize_kernel_aggdiff_0[1] = { numWiPerWg }; // The number of workitems in each workgroup. 
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_aggdiff, 1, NULL,
		gsize_kernel_aggdiff_0, lsize_kernel_aggdiff_0, 0, NULL, NULL);
	clFinish(command_queue_gpu);
	clCheckError(_err, "kernel_aggdiff failed!");
#ifdef PROFILE_READ_DETAILED
	end = Clock::now();
	etime = (end - begin).count() / (float)1000000;
	printf("[Timer] kernel_aggdiff took %5.3f ms\n", etime);
	kernelBandWidth("kernel_aggdiff",
		(float)2 * segmentSizePow2 * numQueryEntries * sizeof(int), // queueSorted and talleyCount
		(float)numQueryEntries * sizeof(int) + // queryCt_obj
		(float)4 * segmentSizePow2 * numQueryEntries * sizeof(int), // talleyCount and tally, init and write
		etime);
#endif 

#ifdef DEBUG_TALLY
	/* visualize compact. */
	printf("\n");
	printf("<<< Aggregated Diffs >>>\n");
	int *g_CompactQueryCt = new int[numQueryEntries];
	_err = clEnqueueReadBuffer(command_queue_gpu, queryCt_obj, CL_TRUE, 0,
		numQueryEntries * sizeof(int), g_CompactQueryCt, 0, NULL, NULL);

	unsigned int *queueAggDiffExtract = new unsigned int[numQueryEntries * segmentSizePow2];
	unsigned int *queueAggDiffCntExtract = new unsigned int[numQueryEntries * segmentSizePow2];
	_err = clEnqueueReadBuffer(command_queue_gpu, *talley_obj, CL_TRUE, 0,
		numQueryEntries * segmentSizePow2 * sizeof(int), queueAggDiffExtract, 0, NULL, NULL);
	_err = clEnqueueReadBuffer(command_queue_gpu, tallyBuffer_obj, CL_TRUE, 0,
		numQueryEntries * segmentSizePow2 * sizeof(int), queueAggDiffCntExtract, 0, NULL, NULL);

	/* verification. */
	for (int i = 0; i < numQueryEntries; i++) {
		for (int k = 0; k < segmentSizePow2; k++) {
			int idx = i * segmentSizePow2 + k;
			if (queueAggDiffExtract[idx] != queuePreAggExtract[idx] || queueAggDiffCntExtract[idx] != countPreAggExtract[idx]) {
				printf("Error at query %u position %u; %u(%u) vs %u(%u) CPU vs GPU\n", i, k,
					queuePreAggExtract[idx], countPreAggExtract[idx], queueAggDiffExtract[idx], queueAggDiffCntExtract[idx]);
			}
		}
	}

	printf("\n");
	delete[] tallyPreAgg;
	delete[] countPreAggExtract;
	delete[] g_CompactQueryCt;

	delete[] queueAggDiffExtract;
	delete[] queueAggDiffCntExtract;

	pause();

#endif

#ifdef DEBUG_TALLY
	/* CPU computation of subtract diff. */
	unsigned int *g_queryCt = new unsigned int[numQueryEntries];
	_err = clEnqueueReadBuffer(command_queue_gpu, queryCt_obj, CL_TRUE, 0,
		numQueryEntries * sizeof(unsigned int), g_queryCt, 0, NULL, NULL);
	unsigned int *preTallyCnt = new unsigned int[numQueryEntries * segmentSizePow2];
	_err = clEnqueueReadBuffer(command_queue_gpu, tallyBuffer_obj, CL_TRUE, 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), preTallyCnt, 0, NULL, NULL);
	unsigned int *preTally = new unsigned int[numQueryEntries * segmentSizePow2];
	_err = clEnqueueReadBuffer(command_queue_gpu, *talley_obj, CL_TRUE, 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), preTally, 0, NULL, NULL);

	mock_sub(g_queryCt, preTallyCnt, preTally, numQueryEntries, segmentSize, segmentSizePow2);

#endif

#ifdef PROFILE_READ_DETAILED
	begin = Clock::now();
#endif

	_err = clSetKernelArg(kernel_subtractdiff, 0, sizeof(cl_mem), (void *)talley_obj);
	_err |= clSetKernelArg(kernel_subtractdiff, 1, sizeof(cl_mem), (void *)talleyCount_obj);
	_err |= clSetKernelArg(kernel_subtractdiff, 2, sizeof(cl_mem), (void *)&tallyBuffer_obj);
	_err |= clSetKernelArg(kernel_subtractdiff, 3, sizeof(cl_mem), (void *)&queryCt_obj);
	_err |= clSetKernelArg(kernel_subtractdiff, 4, sizeof(int), (void *)&segmentSize);
	_err |= clSetKernelArg(kernel_subtractdiff, 5, sizeof(int), (void *)&segmentSizePow2);
	_err |= clSetKernelArg(kernel_subtractdiff, 6, sizeof(int), (void *)&_segmentSizeModulor);
	_err |= clSetKernelArg(kernel_subtractdiff, 7, sizeof(int), (void *)&_segmentSizeBitShiftDivisor);
	clCheckError(_err, "Failed to set kernel_subtractdiff arguments!");

	size_t gsize_kernel_subtractdiff[1] = { segmentSizePow2 * numQueryEntries };
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_subtractdiff, 1, NULL,
		gsize_kernel_subtractdiff, NULL, 0, NULL, NULL);
	clFinish(command_queue_gpu);
	clCheckError(_err, "kernel_subtractdiff failed!");
#ifdef PROFILE_READ_DETAILED
	end = Clock::now();
	etime = (end - begin).count() / (float)1000000;
	printf("[Timer] kernel_subtractdiff took %5.3f ms\n", etime);
	kernelBandWidth("kernel_subtractdiff",
		(float)0.8 * 2 * segmentSizePow2 * numQueryEntries * sizeof(unsigned int), // talleyCount * 2
		(float)0.8 * segmentSizePow2 * numQueryEntries * sizeof(unsigned int), // talleyCount
		etime); // Both with assumption that occupied reservoir spots is about 0.8
#endif

#ifdef DEBUG_TALLY
							   /* Verification. */
	printf("\n");
	printf("<<< Subtract Diffs >>>\n");

	unsigned int *postTally = new unsigned int[numQueryEntries * segmentSizePow2];
	unsigned int *postTallyCnt = new unsigned int[numQueryEntries * segmentSizePow2];
	_err = clEnqueueReadBuffer(command_queue_gpu, *talley_obj, CL_TRUE, 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), postTally, 0, NULL, NULL);
	_err = clEnqueueReadBuffer(command_queue_gpu, *talleyCount_obj, CL_TRUE, 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), postTallyCnt, 0, NULL, NULL);

	/* Verification. Uncomment to enable. */
	for (int i = 0; i < numQueryEntries; i++) {
		for (int k = 0; k < segmentSizePow2; k++) {
			int idx = i * segmentSizePow2 + k;
			if (preTallyCnt[idx] != postTallyCnt[idx] || preTally[idx] != postTally[idx]) {
				printf("Error at query %d position %d: ", i, k);
				printf("%u(%u) vs %u(%u)\n", preTally[idx], preTallyCnt[idx], postTally[idx], postTallyCnt[idx]);
			}
		}
	}
	
	pause();

	printf("\n");
	delete[] preTallyCnt;
	delete[] preTally;
	delete[] postTallyCnt;
	delete[] postTally;
	delete[] g_queryCt;

#endif
	clReleaseMemObject(queryCt_obj);
	clReleaseMemObject(tallyBuffer_obj);
}
#endif

void LSHReservoirSampler::query_extractRows_cpu_openmp(int numQueryEntries, int segmentSize, unsigned int *queue, 
	unsigned int *hashIndices) {

#ifdef PROFILE_READ_DETAILED
	auto begin = Clock::now();
#endif
	unsigned int hashIdx, allocIdx;
#pragma omp parallel private(hashIdx, allocIdx)
#pragma	omp for
	for (int tb = 0; tb < _numTables; tb++) {
		for (int queryIdx = 0; queryIdx < numQueryEntries; queryIdx++) {
			for (int elemIdx = 0; elemIdx < _reservoirSize; elemIdx++) {

				for (unsigned int k = 0; k < _queryProbes; k++) {
					hashIdx = hashIndices[allprobsHashIdx(_queryProbes, numQueryEntries, tb, queryIdx, k)];
					allocIdx = _tablePointers[tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b)];
					if (allocIdx != TABLENULL) {
						queue[queueElemIdx(segmentSize, tb, queryIdx, k, elemIdx)] =
							_tableMem[tableMemResIdx(tb, allocIdx, _aggNumReservoirs) + elemIdx];
					}
				}

			}
		}
	}

#ifdef PROFILE_READ_DETAILED
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[Timer] query_extractRows_cpu_openmp took %5.3f ms\n", etime);
#endif
}

void LSHReservoirSampler::query_frequentitem_cpu_openmp(int numQueryEntries, unsigned int *outputs,
	unsigned int *hashIndices, int topk) {

#ifdef PROFILE_READ_DETAILED
	auto begin = Clock::now();
#endif
	unsigned int hashIdx, allocIdx;
#pragma omp parallel private(hashIdx, allocIdx)
#pragma	omp for
	for (int queryIdx = 0; queryIdx < numQueryEntries; queryIdx++) {
		
		FrequentItems * items = new FrequentItems(topk);
		for (int tb = 0; tb < _numTables; tb++) {
			for (int elemIdx = 0; elemIdx < _reservoirSize; elemIdx++) {
				for (unsigned int k = 0; k < _queryProbes; k++) {
					hashIdx = hashIndices[allprobsHashIdx(_queryProbes, numQueryEntries, tb, queryIdx, k)];
					allocIdx = _tablePointers[tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b)];
					if (allocIdx != TABLENULL) {						
						/* Instead of inserting into the queue, insert directly to the lossy counter. */
						items->increment(_tableMem[tableMemResIdx(tb, allocIdx, _aggNumReservoirs) + elemIdx]);
					}
				}
			}
		}
		items->getTopk(outputs + queryIdx * topk);
		delete items;
	}

#ifdef PROFILE_READ_DETAILED
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[Timer] query_extractRows_cpu_openmp took %5.3f ms\n", etime);
#endif
}

#ifdef OPENCL_2XX
void LSHReservoirSampler::query_extractRows_gpu(int numQueryEntries, int segmentSizePow2, cl_mem *queue_obj, 
	cl_mem *hashIndices_obj) {

#ifdef PROFILE_READ_DETAILED
	auto begin = Clock::now();
#endif
	_err = clSetKernelArg(kernel_extract_rows, 0, sizeof(cl_mem), (void *)&_tablePointers_obj);
	_err |= clSetKernelArg(kernel_extract_rows, 1, sizeof(cl_mem), (void *)&_tableMem_obj);
	_err |= clSetKernelArg(kernel_extract_rows, 2, sizeof(cl_mem), (void *)hashIndices_obj);
	_err |= clSetKernelArg(kernel_extract_rows, 3, sizeof(cl_mem), (void *)queue_obj);
	_err |= clSetKernelArg(kernel_extract_rows, 4, sizeof(unsigned int), (void *)&_numReservoirsHashed);
	_err |= clSetKernelArg(kernel_extract_rows, 5, sizeof(unsigned int), (void *)&_aggNumReservoirs);
	_err |= clSetKernelArg(kernel_extract_rows, 6, sizeof(unsigned int), (void *)&numQueryEntries);
	_err |= clSetKernelArg(kernel_extract_rows, 7, sizeof(unsigned int), (void *)&segmentSizePow2);
	_err |= clSetKernelArg(kernel_extract_rows, 8, sizeof(unsigned int), (void *)&_sechash_a);
	_err |= clSetKernelArg(kernel_extract_rows, 9, sizeof(unsigned int), (void *)&_sechash_b);
	_err |= clSetKernelArg(kernel_extract_rows, 10, sizeof(unsigned int), (void *)&_reservoirSize);
	_err |= clSetKernelArg(kernel_extract_rows, 11, sizeof(unsigned int), (void *)&_numSecHash);
	_err |= clSetKernelArg(kernel_extract_rows, 12, sizeof(unsigned int), (void *)&_queryProbes);
	clCheckError(_err, "Failed to set kernel_extract_rows arguments!");

	size_t gsize_kernel_extract_rows[3] = { numQueryEntries, _numTables, _reservoirSize };
	size_t lsize_kernel_extract_rows[3] = { 1, 1, _reservoirSize };
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_extract_rows, 3, NULL,
		gsize_kernel_extract_rows, lsize_kernel_extract_rows, 0, NULL, NULL);
	clFinish(command_queue_gpu);
	clCheckError(_err, "kernel_extract_rows failed!");
#ifdef PROFILE_READ_DETAILED
	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[Timer] kernel_extract_rows took %5.3f ms\n", etime);
	kernelBandWidth("kernel_extract_rows",
		(float)numQueryEntries * _numTables * sizeof(unsigned int) + // Read hashIndices_obj
		(float)numQueryEntries * _numTables * sizeof(unsigned int) +  // Access table pointers
		(float)numQueryEntries * _numTables * _reservoirSize * sizeof(unsigned int), // Access reservoirs
		(float)numQueryEntries * _numTables * _reservoirSize * sizeof(unsigned int), // Write to queue
		etime);
#endif
}
#endif
