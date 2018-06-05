
#include "LSHReservoirSampler.h"
#include "misc.h"
#include "indexing.h"

#include <algorithm>

#define NAIVE_COUNTING
//#defing SINGLETHREAD_COUNTING
//#define PRINT_TOPK

#ifdef OPENCL_2XX
void LSHReservoirSampler::HashAddGPUTB(cl_mem *allprobsHash_gpuobj, cl_mem* allprobsIdx_gpuobj, int numProbePerTb, int numInputEntries) {
#ifdef PROFILE_READ
	float compute_time = 0;
	float transfer_time = 0;
	auto transfer_begin = Clock::now();
#endif

	cl_mem storelog_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_numTables * 4 * numProbePerTb * sizeof(unsigned int), NULL, &_err);
	_err = clEnqueueFillBuffer(command_queue_gpu, storelog_obj, &_zero, sizeof(const int), 0,
		_numTables * 4 * numProbePerTb * sizeof(unsigned int), 0, NULL, NULL);
	clCheckError(_err, "[LSHReservoirSampler::HashAddGPUTB] Failed to create kernel_reservoir buffer!");

#ifdef PROFILE_READ
	auto transfer_end = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin, transfer_end);
	auto compute_begin = Clock::now();
#endif

	reservoir_sampling_gpu(allprobsHash_gpuobj, allprobsIdx_gpuobj, &storelog_obj, numProbePerTb);
	add_table_gpu(&storelog_obj, numProbePerTb);

	clReleaseMemObject(storelog_obj);
	
#ifdef PROFILE_READ
	auto compute_end = Clock::now();
	compute_time += GETTIME_MS(compute_begin, compute_end);
	printf("[LSHReservoirSampler::HashAddGPUTB] Computation %5.3f ms, MemTransfer %5.3f ms\n", compute_time, transfer_time);
#endif
}
#endif

void LSHReservoirSampler::HashAddCPUTB(unsigned int *allprobsHash, unsigned int* allprobsIdx, int numProbePerTb, int numInputEntries) {
#ifdef PROFILE_READ
	float compute_time = 0;
	float transfer_time = 0;
	auto transfer_begin = Clock::now();
#endif
	unsigned int* storelog = new unsigned int[_numTables * 4 * numProbePerTb]();
#ifdef PROFILE_READ
	auto transfer_end = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin, transfer_end);
	auto compute_begin = Clock::now();
#endif
	reservoir_sampling_cpu_openmp(allprobsHash, allprobsIdx, storelog, numProbePerTb);
	add_table_cpu_openmp(storelog, numProbePerTb);
	delete[] storelog;
#ifdef PROFILE_READ
	auto compute_end = Clock::now();
	compute_time += GETTIME_MS(compute_begin, compute_end);
	printf("[LSHReservoirSampler::HashAddCPUTB] Computation %5.3f ms, MemTransfer %5.3f ms\n", compute_time, transfer_time);
#endif
}

#ifdef OPENCL_2XX
void LSHReservoirSampler::RowsAggregationGPUTB(cl_mem *hashIndices_gpuobj, cl_mem *tally_gpuobj, int segmentSizePow2, int numQueryEntries) {
#ifdef PROFILE_READ
	float compute_time = 0;
	float transfer_time = 0;
	auto transfer_begin = Clock::now();
#endif
	_err |= clEnqueueFillBuffer(command_queue_gpu, *tally_gpuobj, &_zero, sizeof(const int), 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), 0, NULL, NULL);
	clCheckError(_err, "[LSHReservoirSampler::RowsAggregationGPUTB] Failed to clear tally_gpuobj buffer!");

#ifdef PROFILE_READ
	auto transfer_end = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin, transfer_end);
	auto compute_begin = Clock::now();
#endif
	query_extractRows_gpu(numQueryEntries, segmentSizePow2, tally_gpuobj, hashIndices_gpuobj);
#ifdef PROFILE_READ
	auto compute_end = Clock::now();
	compute_time += GETTIME_MS(compute_begin, compute_end);
	printf("[LSHReservoirSampler::RowsAggregationGPUTB] Computation %5.3f ms, MemTransfer %5.3f ms\n", compute_time, transfer_time);
#endif
}


void LSHReservoirSampler::RowsAggregationCPUTB(unsigned int *hashIndices, cl_mem *tally_gpuobj, int segmentSizePow2, int numQueryEntries) {
#ifdef PROFILE_READ
	float compute_time = 0;
	float transfer_time = 0;
	auto transfer_begin = Clock::now();
#endif
	unsigned int *tally = new unsigned int[numQueryEntries * segmentSizePow2](); // Init (slow). 
#ifdef PROFILE_READ
	auto transfer_end = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin, transfer_end);
	auto compute_begin = Clock::now();
#endif
	query_extractRows_cpu_openmp(numQueryEntries, segmentSizePow2, tally, hashIndices);
#ifdef PROFILE_READ
	auto compute_end = Clock::now();
	compute_time += GETTIME_MS(compute_begin, compute_end);
	transfer_begin = Clock::now();
#endif
	memCpy_uint_c2g(tally_gpuobj, tally, numQueryEntries * segmentSizePow2);
	delete[] tally;
#ifdef PROFILE_READ
	transfer_end = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin, transfer_end);
	printf("[LSHReservoirSampler::RowsAggregationCPUTB] Computation %5.3f ms, MemTransfer %5.3f ms\n", compute_time, transfer_time);
#endif
}

void LSHReservoirSampler::kSelect(cl_mem *tally_gpuobj, unsigned int *outputs, int segmentSize, int segmentSizePow2, int numQueryEntries, int topk) {
#ifdef PROFILE_READ
	float compute_time = 0;
	float transfer_time = 0;
	auto compute_begin = Clock::now();
#endif
	segmentedSort(tally_gpuobj, segmentSizePow2, numQueryEntries);
#ifdef PROFILE_READ
	auto compute_end = Clock::now();
	compute_time += GETTIME_MS(compute_begin, compute_end);
	auto transfer_begin = Clock::now();
#endif
	cl_mem talleyCount_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), NULL, &_err);
	_err |= clEnqueueFillBuffer(command_queue_gpu, talleyCount_obj, &_zero, sizeof(const int), 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), 0, NULL, NULL);
	clCheckError(_err, "[LSHReservoirSampler::kSelect] Failed to create talleyCount_obj buffer!");
#ifdef PROFILE_READ
	auto transfer_end = Clock::now();
	transfer_time += GETTIME_MS(transfer_begin, transfer_end);
	compute_begin = Clock::now();
#endif
#if !defined NAIVE_COUNTING
	query_tallyReduction(numQueryEntries, segmentSize, segmentSizePow2, tally_gpuobj, &talleyCount_obj); 
	segmentedSortKV(&talleyCount_obj, tally_gpuobj, segmentSizePow2, numQueryEntries, _maxSamples);
	query_taketopk(numQueryEntries, segmentSizePow2, topk, tally_gpuobj, &talleyCount_obj, outputs);
#else
	cl_mem tallied_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), NULL, &_err);
	_err |= clEnqueueFillBuffer(command_queue_gpu, tallied_obj, &_zero, sizeof(const int), 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), 0, NULL, NULL);
	clCheckError(_err, "[LSHReservoirSampler::kSelect (NaiveGPU)] Failed to create tallied_obj buffer!");
	query_tallyNaive(segmentSizePow2, numQueryEntries, &tallied_obj, &talleyCount_obj, tally_gpuobj);
	segmentedSortKV(&talleyCount_obj, &tallied_obj, segmentSizePow2, numQueryEntries, _maxSamples);
	query_taketopk(numQueryEntries, segmentSizePow2, topk, &tallied_obj, &talleyCount_obj, outputs);
	clReleaseMemObject(tallied_obj);
#endif

#ifdef PROFILE_READ
	compute_end = Clock::now();
	compute_time += GETTIME_MS(compute_begin, compute_end);
	printf("[LSHReservoirSampler::kSelect] Computation %5.3f ms, MemTransfer %5.3f ms\n", compute_time, transfer_time);
#endif

#ifdef PRINT_TOPK

	/* Print out extracted topk. */
	printf("\n");
	int howmany = std::min(numQueryEntries, DEBUGENTRIES);
	printf("<<< Top K Of First %d Queries >>>\n", howmany);
	for (int i = 0; i < std::min(numQueryEntries, DEBUGENTRIES); i++) {
		printf("Query %d: \n", i);
		for (int k = 0; k < topk; k++) {
			printf("%u ", outputs[i * topk + k]);
		}
		printf("\n");
	}
	printf("\n");
	pause();
#endif

	clReleaseMemObject(talleyCount_obj);
}
#endif

void LSHReservoirSampler::kSelect(unsigned int *tally, unsigned int *outputs, int segmentSize, int numQueryEntries, int topk) {

#ifdef PROFILE_READ
	float compute_time = 0;
	auto compute_begin = Clock::now();
#endif

	// SegmentedSort. 
#pragma omp parallel for
	for (int i = 0; i < numQueryEntries; i++) {
		std::sort(tally + i * segmentSize, tally + i * segmentSize + segmentSize);
	}

	// Reduction. 
	unsigned int *tallyCnt = new unsigned int[segmentSize * numQueryEntries]();
#if !defined SINGLETHREAD_COUNTING
#pragma omp parallel for
#endif
	for (int i = 0; i < numQueryEntries; i++) {
		unsigned int *vec = tally + i * segmentSize;
		unsigned int *cntvec = tallyCnt + i * segmentSize;
		int prev = vec[0];
		int ct = 0;
		int counter = 0;
		for (int j = 1; j < segmentSize; j++) {
			counter++;
			if (prev != vec[j]) {
				vec[ct] = prev;
				cntvec[ct] = counter;
				prev = vec[j];
				counter = 0;
				ct++;
			}
		}
		//vec[ct] = prev;
		//cntvec[ct] = counter;
		//ct++;
		for (; ct < segmentSize; ct++) {
			vec[ct] = 0;
		}
	}
	// KV SegmentedSort. 
#pragma omp parallel for
	for (int i = 0; i < numQueryEntries; i++) {
		unsigned int *vec = tally + i * segmentSize;
		unsigned int *cntvec = tallyCnt + i * segmentSize;
		unsigned int *idx = new unsigned int[segmentSize];
		for (int j = 0; j < segmentSize; j++) {
			idx[j] = j;
		}
		std::sort(idx, idx + segmentSize, 
			[&cntvec](unsigned int i1, unsigned int i2) { return cntvec[i1] > cntvec[i2]; });

		int ss;
		int ct = 0;
		if (vec[idx[0]] == 0) { // The first item is spurious. 
			ss = 1;
		}
		else {
			ss = 0;
		}
		ct = 0;
		for (int k = ss; k < topk + ss; k++) {
			outputs[i * topk + ct] = vec[idx[k]];
			ct++;
		}
		delete[] idx;
	}
	delete[] tallyCnt;
#ifdef PROFILE_READ
	compute_end = Clock::now();
	compute_time += GETTIME_MS(compute_begin, compute_end);
	printf("[LSHReservoirSampler::kSelect (CPU)] Computation %5.3f ms. \n", compute_time);
#endif

}

void reverse_array(unsigned int *array, int arraylength)
{
	for (int i = 0; i < (arraylength / 2); i++) {
		float temporary = array[i];
		array[i] = array[(arraylength - 1) - i];
		array[(arraylength - 1) - i] = temporary;
	}
}

void segmentedReverse(int dir, int segmentSize, int numSegments, unsigned int *a) {
	for (int sidx = 0; sidx < numSegments; sidx++) {
		// Reverse the current segment?
		if (sidx % 2 == dir) {
			reverse_array(a + segmentSize * sidx, segmentSize);
		}
	}
}

#ifdef OPENCL_2XX
void LSHReservoirSampler::kSelect_debug(cl_mem *tally_gpuobj,  unsigned int *tally, 
	int segmentSize, int segmentSizePow2, int numQueryEntries, int topk) {

	unsigned int *outputs_gpu = new unsigned int[numQueryEntries * topk];
	unsigned int *outputs_cpu = new unsigned int[numQueryEntries * topk];
	unsigned int* tally_gpu = new unsigned int[numQueryEntries * segmentSizePow2]();
	unsigned int* tallyCnt_gpu = new unsigned int[numQueryEntries * segmentSizePow2]();
	unsigned int* tally_cpu = new unsigned int[numQueryEntries * segmentSizePow2]();
	unsigned int* tallyCnt_cpu = new unsigned int[numQueryEntries * segmentSizePow2]();
	unsigned int *tallyCnt = new unsigned int[segmentSizePow2 * numQueryEntries]();

	// SegmentedSort - CPU. 
#pragma omp parallel for
	for (int i = 0; i < numQueryEntries; i++) {
		std::sort(tally + i * segmentSizePow2, tally + i * segmentSizePow2 + segmentSizePow2);
	}

	// SegmentedSort - GPU. 
	segmentedSort(tally_gpuobj, segmentSizePow2, numQueryEntries);
	memCpy_uint_g2c(tally_gpu, tally_gpuobj, numQueryEntries * segmentSizePow2);

	// Compare. 
	segmentedReverse(1, segmentSizePow2, numQueryEntries, tally_gpu);
	memCpy_uint_c2g(tally_gpuobj, tally_gpu, numQueryEntries * segmentSizePow2);
	for (int i = 0; i < numQueryEntries * segmentSizePow2; i++) {
		if (tally_gpu[i] != tally[i]) {
			std::cout << "Seg1 diff at " << i << ": cpu " << tally[i] << ", gpu " << tally_gpu[i] << std::endl;
		}
	}
	
	// Reduction - CPU. 
	#pragma omp parallel for
	for (int i = 0; i < numQueryEntries; i++) {
		unsigned int *vec = tally + i * segmentSizePow2;
		unsigned int *cntvec = tallyCnt + i * segmentSizePow2;
		int prev = vec[0];
		int ct = 0;
		int counter = 0;
		for (int j = 1; j < segmentSizePow2; j++) {
			counter++;
			if (prev != vec[j]) {
				vec[ct] = prev;
				cntvec[ct] = counter;
				prev = vec[j];
				counter = 0;
				ct++;
			}
		}
		vec[ct] = prev;
		cntvec[ct] = counter;
		ct++;
		for (; ct < segmentSizePow2; ct++) {
			vec[ct] = 0;
		}
	}

	// Reduction - GPU. 
	cl_mem talleyCount_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), NULL, &_err);
	_err |= clEnqueueFillBuffer(command_queue_gpu, talleyCount_obj, &_zero, sizeof(const int), 0,
		numQueryEntries * segmentSizePow2 * sizeof(unsigned int), 0, NULL, NULL);
	query_tallyReduction(numQueryEntries, segmentSize, segmentSizePow2, tally_gpuobj, &talleyCount_obj);
	memCpy_uint_g2c(tally_gpu, tally_gpuobj, numQueryEntries * segmentSizePow2);
	memCpy_uint_g2c(tallyCnt_gpu, &talleyCount_obj, numQueryEntries * segmentSizePow2);

	// Compare. 
	/* There exists reduction difference, the KV sort has no problem. */
	for (int i = 0; i < numQueryEntries * segmentSizePow2; i++) {
		if (tally_gpu[i] != tally[i]) {
			std::cout << "Reduction diff at " << i << 
				": cpu " << tally[i] << "-" << tallyCnt[i] << 
				", gpu " << tally_gpu[i] << "-" << tallyCnt_gpu[i] <<
				std::endl;
		}
	}

	/* Copy CPU reduction results for the last step. */
	memCpy_uint_c2g(tally_gpuobj, tally, numQueryEntries * segmentSizePow2);
	memCpy_uint_c2g(&talleyCount_obj, tallyCnt, numQueryEntries * segmentSizePow2);

	// KV SegmentedSort - CPU. 
#pragma omp parallel for
	for (int i = 0; i < numQueryEntries; i++) {
		unsigned int *vec = tally + i * segmentSizePow2;
		unsigned int *cntvec = tallyCnt + i * segmentSizePow2;
		unsigned int *idx = new unsigned int[segmentSizePow2];
		for (int j = 0; j < segmentSizePow2; j++) {
			idx[j] = j;
		}
		std::sort(idx, idx + segmentSizePow2,
			[&cntvec](unsigned int i1, unsigned int i2) { return cntvec[i1] > cntvec[i2]; });

		for (int j = 0; j < segmentSizePow2; j++) {
			tallyCnt_cpu[i * segmentSizePow2 + j] = cntvec[idx[j]];
			tally_cpu[i * segmentSizePow2 + j] = vec[idx[j]];
		}

		//int ss;
		//int ct = 0;
		//if (vec[idx[0]] == 0) { // The first item is spurious. 
		//	ss = 1;
		//}
		//else {
		//	ss = 0;
		//}
		//ct = 0;
		//for (int k = ss; k < topk + ss; k++) {
		//	outputs_cpu[i * topk + ct] = vec[idx[k]];
		//	ct++;
		//}
		//delete[] idx;
	}
	
	// KV SegmentedSort - GPU. 
	segmentedSortKV(&talleyCount_obj, tally_gpuobj, segmentSizePow2, numQueryEntries, _maxSamples);
	memCpy_uint_g2c(tally_gpu, tally_gpuobj, numQueryEntries * segmentSizePow2);
	memCpy_uint_g2c(tallyCnt_gpu, &talleyCount_obj, numQueryEntries * segmentSizePow2);
	segmentedReverse(0, segmentSizePow2, numQueryEntries, tally_gpu);
	segmentedReverse(0, segmentSizePow2, numQueryEntries, tallyCnt_gpu);

	for (int i = 0; i < numQueryEntries * segmentSizePow2; i++) {
		if (tally_gpu[i] != tally_cpu[i] && tallyCnt_gpu[i] != tallyCnt_cpu[i]) {
			std::cout << "Seg2 diff at " << i <<
				": cpu " << tally_cpu[i] << "-" << tallyCnt_cpu[i] <<
				", gpu " << tally_gpu[i] << "-" << tallyCnt_gpu[i] <<
				std::endl;
		}
	}

	delete[] tallyCnt;

	//query_taketopk(numQueryEntries, segmentSizePow2, topk, tally_gpuobj, &talleyCount_obj, outputs_gpu);
	//segmentedReverse(1, segmentSizePow2, numQueryEntries, tally_gpu);
	//segmentedReverse(1, segmentSizePow2, numQueryEntries, tallyCnt_gpu);

	// Compare. 
	//for (int i = 0; i < numQueryEntries * segmentSizePow2; i++) {
	//	if (outputs_gpu[i] != outputs_cpu[i]) {
	//		std::cout << "Output diff at " << i <<
	//			": cpu " << outputs_cpu[i] << 
	//			", gpu " << outputs_gpu[i] <<
	//			std::endl;
	//	}
	//}

	pause();
}

/* Benchmarks the counting step of the count-based k-selection. */
int LSHReservoirSampler::benchCounting(int numQueries, int* dataIdx, float* dataVal, int* dataMarker, float *timings) {
#if !defined USE_OPENCL
	std::cout << "Enable USE_OPENCL to perform benchmarks on counting. " << std::endl;
	exit(1);
#endif
	float mytime = 0;
	auto begin = Clock::now();
	auto end = Clock::now();
	timings[0] = 0; timings[1] = 0; timings[2] = 0; timings[3] = 0;

	std::cout << "Preparing benchmarking counting ..." << std::endl;
	unsigned int* allprobsHash = new unsigned int[_numTables * numQueries * _queryProbes];
	unsigned int* allprobsIdx = new unsigned int[_numTables * numQueries * _queryProbes];
	int segmentSize = _numTables * _queryProbes * _reservoirSize;
	int segmentSizePow2 = smallestPow2(segmentSize); // Pow2 required by sorting.
	if (segmentSize != segmentSizePow2) {
		std::cout << "For benchCounting purpose, L * R needs to be power of 2. " << std::endl;
	}

	cl_mem tally_gpuobj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueries * segmentSizePow2 * sizeof(unsigned int), NULL, &_err);
	cl_mem talleyCount_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueries * segmentSizePow2 * sizeof(unsigned int), NULL, &_err);
	clFinish(command_queue_gpu);

	/* Extract the rows, segmented sort. */
	_hashFamily->getHash(allprobsHash, allprobsIdx,
		dataIdx, dataVal, dataMarker, numQueries, _queryProbes);
	unsigned int* _A = new unsigned int[numQueries * segmentSizePow2];
	query_extractRows_cpu_openmp(numQueries, segmentSizePow2, _A, allprobsHash);

	//_err = clEnqueueWriteBuffer(command_queue_gpu, tally_gpuobj, CL_TRUE, 0,
	//	numQueries * segmentSizePow2 * sizeof(unsigned int), _A, 0, NULL, NULL);
	//clFinish(command_queue_gpu);

	unsigned int* A = new unsigned int[numQueries * segmentSizePow2];

	#pragma omp parallel for
		for (int i = 0; i < numQueries; i++) std::sort(_A + i * segmentSizePow2, _A + i * segmentSizePow2 + segmentSizePow2);


	/* Benchmark sorting. */

//	begin = Clock::now();
//	segmentedSort(&tally_gpuobj, segmentSizePow2, numQueries);	
//	end = Clock::now();
//	float gpusort_time = GETTIME_MS(begin, end);
//	timings[0] += gpusort_time; 
//	timings[1] += gpusort_time;
//
//	for (int i = 0; i < numQueries * segmentSizePow2; i++) A[i] = _A[i];
//	begin = Clock::now();
//#pragma omp parallel for
//	for (int i = 0; i < numQueries; i++) std::sort(A + i * segmentSizePow2, A + i * segmentSizePow2 + segmentSizePow2);
//	end = Clock::now();
//	float cpusort_time = GETTIME_MS(begin, end);
//	timings[2] += cpusort_time;
//
//	for (int i = 0; i < numQueries * segmentSizePow2; i++) A[i] = _A[i];
//	begin = Clock::now();
//	for (int i = 0; i < numQueries; i++) std::sort(A + i * segmentSizePow2, A + i * segmentSizePow2 + segmentSizePow2);
//	end = Clock::now();
//	float single_sorttime = GETTIME_MS(begin, end);
//	timings[3] += single_sorttime;

	std::cout << "Benchmarking ... Segment size " << segmentSizePow2 << " queries count " << numQueries <<  std::endl;
	
	std::cout << "GPU Clever (ms)" << std::endl;
	_err = clEnqueueWriteBuffer(command_queue_gpu, tally_gpuobj, CL_TRUE, 0,
		numQueries * segmentSizePow2 * sizeof(unsigned int), _A, 0, NULL, NULL);
	_err |= clEnqueueFillBuffer(command_queue_gpu, talleyCount_obj, &_zero, sizeof(const int), 0,
		numQueries * segmentSizePow2 * sizeof(unsigned int), 0, NULL, NULL);
	clFinish(command_queue_gpu);
	begin = Clock::now();
	query_tallyReduction(numQueries, segmentSize, segmentSizePow2, &tally_gpuobj, &talleyCount_obj);
	clFinish(command_queue_gpu);
	end = Clock::now();
	mytime = GETTIME_MS(begin, end);
	std::cout << mytime << std::endl;
	timings[0] += mytime;

	//begin = Clock::now();
	//segmentedSortKV(&talleyCount_obj, &tally_gpuobj, segmentSizePow2, numQueries, _maxSamples);
	//end = Clock::now();
	//float gpukvsort_time = GETTIME_MS(begin, end);
	//timings[0] += gpukvsort_time;

	std::cout << "GPU Naive (ms)" << std::endl;
	_err = clEnqueueWriteBuffer(command_queue_gpu, tally_gpuobj, CL_TRUE, 0,
		numQueries * segmentSizePow2 * sizeof(unsigned int), _A, 0, NULL, NULL);
	_err |= clEnqueueFillBuffer(command_queue_gpu, talleyCount_obj, &_zero, sizeof(const int), 0,
		numQueries * segmentSizePow2 * sizeof(unsigned int), 0, NULL, NULL);
	cl_mem tallied_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		numQueries * segmentSizePow2 * sizeof(unsigned int), NULL, &_err);
	_err |= clEnqueueFillBuffer(command_queue_gpu, tallied_obj, &_zero, sizeof(const int), 0,
		numQueries * segmentSizePow2 * sizeof(unsigned int), 0, NULL, NULL);
	clFinish(command_queue_gpu);
	begin = Clock::now();
	query_tallyNaive(segmentSizePow2, numQueries, &tallied_obj, &talleyCount_obj, &tally_gpuobj);
	clFinish(command_queue_gpu);
	end = Clock::now();
	mytime = GETTIME_MS(begin, end);
	std::cout << mytime << std::endl;
	timings[1] += mytime; 
	//timings[1] += gpukvsort_time;
	
	/* CPU Parallel Counting. */
	std::cout << "CPU Parallel (ms)" << std::endl;
	for (int i = 0; i < numQueries * segmentSizePow2; i++) A[i] = _A[i];
	unsigned int *tallyCnt = new unsigned int[segmentSizePow2 * numQueries]();
	begin = Clock::now();
#pragma omp parallel for
	for (int i = 0; i < numQueries; i++) {
		unsigned int *vec = A + i * segmentSizePow2;
		unsigned int *cntvec = tallyCnt + i * segmentSizePow2;
		int prev = vec[0];
		int ct = 0;
		int counter = 0;
		for (int j = 1; j < segmentSizePow2; j++) {
			counter++;
			if (prev != vec[j]) {
				vec[ct] = prev;
				cntvec[ct] = counter;
				prev = vec[j];
				counter = 0;
				ct++;
			}
		}
		for (; ct < segmentSizePow2; ct++) {
			vec[ct] = 0;
		}
	}
	end = Clock::now();
	mytime = GETTIME_MS(begin, end);
	std::cout << mytime << std::endl;
	timings[2] += mytime;

//	begin = Clock::now();
//#pragma omp parallel for
//	for (int i = 0; i < numQueries; i++) {
//		unsigned int *vec = A + i * segmentSizePow2;
//		unsigned int *cntvec = tallyCnt + i * segmentSizePow2;
//		unsigned int *idx = new unsigned int[segmentSizePow2];
//		for (int j = 0; j < segmentSizePow2; j++) {
//			idx[j] = j;
//		}
//		std::sort(idx, idx + segmentSizePow2,
//			[&cntvec](unsigned int i1, unsigned int i2) { return cntvec[i1] > cntvec[i2]; });
//	}
//	end = Clock::now();
//	mytime = GETTIME_MS(begin, end);
//	timings[2] += mytime;
	
	std::cout << "CPU OneCore (ms)" << std::endl;
	for (int i = 0; i < numQueries * segmentSizePow2; i++) A[i] = _A[i];
	begin = Clock::now();
	for (int i = 0; i < numQueries; i++) {
		unsigned int *vec = A + i * segmentSizePow2;
		unsigned int *cntvec = tallyCnt + i * segmentSizePow2;
		int prev = vec[0];
		int ct = 0;
		int counter = 0;
		for (int j = 1; j < segmentSizePow2; j++) {
			counter++;
			if (prev != vec[j]) {
				vec[ct] = prev;
				cntvec[ct] = counter;
				prev = vec[j];
				counter = 0;
				ct++;
			}
		}
		for (; ct < segmentSizePow2; ct++) {
			vec[ct] = 0;
		}
	}
	end = Clock::now();
	mytime = GETTIME_MS(begin, end);
	std::cout << mytime << std::endl;
	timings[3] += mytime;

	//begin = Clock::now();
	//for (int i = 0; i < numQueries; i++) {
	//	unsigned int *vec = A + i * segmentSizePow2;
	//	unsigned int *cntvec = tallyCnt + i * segmentSizePow2;
	//	unsigned int *idx = new unsigned int[segmentSizePow2];
	//	for (int j = 0; j < segmentSizePow2; j++) {
	//		idx[j] = j;
	//	}
	//	std::sort(idx, idx + segmentSizePow2,
	//		[&cntvec](unsigned int i1, unsigned int i2) { return cntvec[i1] > cntvec[i2]; });
	//}
	//end = Clock::now();
	//mytime = GETTIME_MS(begin, end);
	//timings[3] += mytime;
	
	delete[] tallyCnt;
	delete[] A;
	delete[] _A;
	delete[] allprobsHash;
	delete[] allprobsIdx;
	clReleaseMemObject(tally_gpuobj);
	clReleaseMemObject(talleyCount_obj);
	clReleaseMemObject(tallied_obj);

	return segmentSizePow2;
}
#endif