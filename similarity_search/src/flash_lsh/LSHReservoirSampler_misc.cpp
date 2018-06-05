
#include "LSHReservoirSampler.h"
#include "indexing.h"
#include "misc.h"
#include "LSHReservoirSampler_config.h"

void LSHReservoirSampler::kernelBandWidth(const std::string& kernelName, float br, float bw, float time) {
	float actual = ((br + bw) / 1000000000) / (time / 1000);
	std::cout << "[Bandwidth] " << kernelName;
	printf(" %3.2f GBps. \n", actual);
	return;
}

//void LSHReservoirSampler::clMemCpy_uint_g2c(cl_mem *dst, cl_mem *src, unsigned int size) {
//	unsigned int *buffer = new unsigned int[size];
//	_err = clEnqueueReadBuffer(command_queue_gpu, *src, CL_TRUE, 0, size * sizeof(unsigned int), buffer, 0, NULL, NULL);
//	clCheckError(_err, "[clMemCpy_uint_g2c] Failed to read from gpu.");
//	clFinish(command_queue_gpu);
//	_err = clEnqueueWriteBuffer(command_queue_cpu, *dst, CL_TRUE, 0, size * sizeof(unsigned int), buffer, 0, NULL, NULL);
//	clCheckError(_err, "[clMemCpy_uint_g2c] Failed to write to cpu.");
//	clFinish(command_queue_cpu);
//	delete[] buffer;
//}

//void LSHReservoirSampler::clMemCpy_uint_c2g(cl_mem *dst, cl_mem *src, unsigned int size) {
//	unsigned int *buffer = new unsigned int[size];
//	_err = clEnqueueReadBuffer(command_queue_cpu, *src, CL_TRUE, 0, size * sizeof(unsigned int), buffer, 0, NULL, NULL);
//	clCheckError(_err, "[clMemCpy_uint_c2g] Failed to read from cpu.");
//	clFinish(command_queue_cpu);
//	_err |= clEnqueueWriteBuffer(command_queue_gpu, *dst, CL_TRUE, 0, size * sizeof(unsigned int), buffer, 0, NULL, NULL);
//	clCheckError(_err, "[clMemCpy_uint_c2g] Failed to write to gpu.");
//	clFinish(command_queue_gpu);
//	delete[] buffer;
//}

#ifdef OPENCL_2XX
void LSHReservoirSampler::memCpy_uint_g2c(unsigned int *dst, cl_mem *src, unsigned int size) {
	_err = clEnqueueReadBuffer(command_queue_gpu, *src, CL_TRUE, 0, size * sizeof(unsigned int), dst, 0, NULL, NULL);
	clCheckError(_err, "[memCpy_uint_g2c] Failed to read from gpu.");
	clFinish(command_queue_gpu);
}

void LSHReservoirSampler::memCpy_uint_c2g(cl_mem *dst, unsigned int *src, unsigned int size) {
	_err |= clEnqueueWriteBuffer(command_queue_gpu, *dst, CL_TRUE, 0, size * sizeof(unsigned int), src, 0, NULL, NULL);
	clCheckError(_err, "[memCpy_uint_c2g] Failed to write to gpu.");
	clFinish(command_queue_gpu);
}
#endif

void LSHReservoirSampler::checkTableMemLoad() {
	
#if defined OPENCL_HASHTABLE
	_tableMemAllocator = new unsigned int[_numTables * sizeof(unsigned int)];
	_err = clEnqueueReadBuffer(command_queue_gpu, _tableMemAllocator_obj, CL_TRUE, 0,
		_numTables * sizeof(unsigned int), _tableMemAllocator, 0, NULL, NULL);
#endif
	unsigned int maxx = 0;
	unsigned int minn = _numReservoirs;
	unsigned int tt = 0;
	for (unsigned int i = 0; i < _numTables; i++) {
		if (_tableMemAllocator[i] < minn) {
			minn = _tableMemAllocator[i];
		}
		if (_tableMemAllocator[i] > maxx) {
			maxx = _tableMemAllocator[i];
		}
		tt += _tableMemAllocator[i];
	}

	printf("Table Mem Usage ranges from %f to %f, average %f\n",
		((float)minn) / (float)_aggNumReservoirs,
		((float)maxx) / (float)_aggNumReservoirs,
		((float)tt) / (float)(_numTables * _aggNumReservoirs));
#if defined OPENCL_HASHTABLE
	delete[] _tableMemAllocator;
#endif
}

#ifdef OPENCL_2XX
void LSHReservoirSampler::clCheckError(cl_int code, const char* msg) {
	if (code != CL_SUCCESS) {
		printf(msg);
		printf("\nError Code: %d\n", code);
		pause();
		exit(1);
	}
}

void LSHReservoirSampler::clCheckErrorNoExit(cl_int code, const char* msg) {
	if (code != CL_SUCCESS) {
		printf(msg);
		printf("\nError Code: %d\n", code);
		pause();
	}
}

void LSHReservoirSampler::clTestAlloc(long long numInts, cl_context* testContext, cl_command_queue* testQueue) {
	int *bufferArray = new int[numInts];

	/* Allocation. */

	auto begin = Clock::now();

	cl_mem buffer = clCreateBuffer(*testContext, CL_MEM_READ_WRITE, numInts * sizeof(int), NULL, &_err);
	clCheckErrorNoExit(_err, "[clTestAlloc] Failed to declare buffer.");
	clFinish(*testQueue);

	auto end = Clock::now();
	float etime = (end - begin).count() / (float)1000000;
	printf("[clTestAlloc] Allocation took %5.3f ms\n", etime);

	begin = Clock::now();

	_err = clEnqueueWriteBuffer(*testQueue, buffer, CL_TRUE, 0, numInts * sizeof(int), bufferArray, 0, NULL, NULL);
	clCheckErrorNoExit(_err, "[clTestAlloc] Failed to Write-init.");
	clFinish(*testQueue);

	end = Clock::now();
	etime = (end - begin).count() / (float)1000000;
	float io = (float)numInts * (float)sizeof(int);
	printf("[clTestAlloc] Write-init took %5.3f ms. Avg Bandwidth %5.3f Gib/s \n", 
		etime, (io / (float)1000000000) / (etime / 1000));

	clReleaseMemObject(buffer);
	delete[] bufferArray;
	pause();
}
#endif

void LSHReservoirSampler::showParams() {
	printf("\n");
	printf("<<< LSHR Parameters >>>\n");
	std::cout << "_rangePow " << _rangePow << "\n";
	std::cout << "_rangePow_Rehashed " << _numSecHash << "\n";
	std::cout << "_numTables " << _numTables << "\n";
	std::cout << "_reservoirSize " << _reservoirSize << "\n";
	std::cout << "_queryProbes " << _queryProbes << "\n";
	std::cout << "_hashingProbes " << _hashingProbes << "\n";

	std::cout << "_dimension " << _dimension << "\n";
	std::cout << "_maxSamples " << _maxSamples << "\n";
	std::cout << "_tableAllocFraction " << _tableAllocFraction << "\n";
	std::cout << "_segmentSizeModulor " << _segmentSizeModulor << "\n";
	std::cout << "_segmentSizeBitShiftDivisor " << _segmentSizeBitShiftDivisor << "\n";
	std::cout << "_numReservoirs " << _numReservoirs << "\n";
	std::cout << "_numReservoirsHashed " << _numReservoirsHashed << "\n";
	std::cout << "_aggNumReservoirs " << _aggNumReservoirs << "\n";
	std::cout << "_maxReservoirRand " << _maxReservoirRand << "\n";
	printf("\n");
}

void LSHReservoirSampler::viewTables() {
#if defined OPENCL_HASHTABLE
	_tablePointers = new unsigned int[_numReservoirsHashed * _numTables];
	_tableMem = new unsigned int[_tableMemMax];
	_err = clEnqueueReadBuffer(command_queue_gpu, _tablePointers_obj, CL_TRUE, 0,
		_numReservoirsHashed * _numTables * sizeof(unsigned int), _tablePointers, 0, NULL, NULL);
	_err = clEnqueueReadBuffer(command_queue_gpu, _tableMem_obj, CL_TRUE, 0,
		_tableMemMax * sizeof(unsigned int), _tableMem, 0, NULL, NULL);
#endif
	for (unsigned int which = 0; which < std::min(_numTables, (unsigned int) DEBUGTB); which++) {
		printf("\n");
		printf("<<< Table %d Content >>>\n", which);
		unsigned int maxResShow = 0; 
		for (unsigned int t = 0; t < _numReservoirs; t++) {
			if (_tablePointers[tablePointersIdx(_numReservoirsHashed, t, which, _sechash_a, _sechash_b)] != _tableNull && maxResShow < DEBUGENTRIES) {
				unsigned int allocIdx = _tablePointers[tablePointersIdx(_numReservoirsHashed, t, which, _sechash_a, _sechash_b)];
				printf("Reservoir %d (%u): ", t, _tableMem[tableMemCtIdx(which, allocIdx, _aggNumReservoirs)]);
				for (unsigned int h = 0; h < std::min(_reservoirSize, (unsigned)DEBUGENTRIES); h++) {
					printf("%u ", _tableMem[tableMemResIdx(which, allocIdx, _aggNumReservoirs) + h]);
				}
				printf("\n");
				maxResShow++;
			}
		}
		printf("\n");
	}

	pause();
#if defined OPENCL_HASHTABLE
	delete[] _tableMem;
#endif
}

void LSHReservoirSampler::pause() {
#ifdef VISUAL_STUDIO
	system("pause");
#endif
}