
#include "indexing.h"
#include "misc.h"
#include "LSHReservoirSampler.h"
#include "LSHReservoirSampler_config.h"

//#define PRINT_CLINFO

LSHReservoirSampler::LSHReservoirSampler(LSH *hashFamIn, unsigned int numHashPerFamily, unsigned int numHashFamilies, 
	unsigned int reservoirSize, unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples, 
	unsigned int queryProbes, unsigned int hashingProbes, float tableAllocFraction) {

#if !defined SECONDARY_HASHING
	if (numHashPerFamily != numSecHash) {
		std::cout << "[LSHReservoirSampler::LSHReservoirSampler] Fatal, secondary hashing disabled. " << std::endl;
	}
#endif

	initVariables(numHashPerFamily, numHashFamilies, reservoirSize, dimension, numSecHash, maxSamples, queryProbes, 
		hashingProbes, tableAllocFraction);

#if defined USE_OPENCL
	clPlatformDevices();
	clContext();
	clProgram();
	clKernels();
	clCommandQueue();
#endif

	_hashFamily = hashFamIn;

#if defined CL_TEST_CPU
	float cpu_test_size = (float)CL_TEST_CPU*(float)sizeof(int) / (float)1000000000;
	printf("Testing CPU Device %d Allocation (%3.1f GiB) Bandwidth.\n", CL_CPU_DEVICE, cpu_test_size);
	clTestAlloc(CL_TEST_CPU, &context_cpu, &command_queue_cpu);
#endif
#if defined CL_TEST_GPU
	float gpu_test_size = (float)CL_TEST_GPU*(float)sizeof(int) / (float)1000000000;
	printf("Testing GPU Device %d Allocation (%3.1f GiB) Bandwidth.\n", CL_DEVICE_ID, gpu_test_size);
	clTestAlloc(CL_TEST_GPU, &context_gpu, &command_queue_gpu);
#endif

	initHelper(_numTables, _rangePow, _reservoirSize);
}


void LSHReservoirSampler::restart(LSH *hashFamIn, unsigned int numHashPerFamily, unsigned int numHashFamilies,
	unsigned int reservoirSize, unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples,
	unsigned int queryProbes, unsigned int hashingProbes, float tableAllocFraction) {
	unInit();
	initVariables(numHashPerFamily, numHashFamilies, reservoirSize, dimension, numSecHash, maxSamples, queryProbes,
		hashingProbes, tableAllocFraction);
	_hashFamily = hashFamIn;
	initHelper(_numTables, _rangePow, _reservoirSize);
}

void LSHReservoirSampler::initVariables(unsigned int numHashPerFamily, unsigned int numHashFamilies, 
	unsigned int reservoirSize, unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples, 
	unsigned int queryProbes, unsigned int hashingProbes, float tableAllocFraction) {
	_rangePow = numHashPerFamily;
	_numTables = numHashFamilies;
	_reservoirSize = reservoirSize;
	_dimension = dimension;
	_numSecHash = numSecHash;
	_maxSamples = maxSamples;
	_queryProbes = queryProbes;
	_hashingProbes = hashingProbes;
	_tableAllocFraction = tableAllocFraction;
	_segmentSizeModulor = numHashFamilies * reservoirSize * queryProbes - 1;
	_segmentSizeBitShiftDivisor = getLog2(_segmentSizeModulor);

	_numReservoirs = (unsigned int) pow(2, _rangePow);		// Number of rows in each hashTable. 
	_numReservoirsHashed = (unsigned int) pow(2, _numSecHash);		// Number of rows in each hashTable. 
	_aggNumReservoirs = (unsigned int) _numReservoirsHashed * _tableAllocFraction;
	_maxReservoirRand = (unsigned int) ceil(maxSamples / 10); // TBD. 

	_zero = 0;
	_zerof = 0.0;
	_tableNull = TABLENULL;
}

void LSHReservoirSampler::initHelper(int numTablesIn, int numHashPerFamilyIn, int reservoriSizeIn) {

	/* Reservoir Random Number. */
	std::cout << "[LSHReservoirSampler::initHelper] Generating random number for reservoir sampling ..." << std::endl;
	std::default_random_engine generator1;
	std::uniform_int_distribution<unsigned int> distribution_a(0, 0x7FFFFFFF);
	_sechash_a = distribution_a(generator1) * 2 + 1;
	std::uniform_int_distribution<unsigned int> distribution_b(0, 0xFFFFFFFF >> _numSecHash);
	_sechash_b = distribution_b(generator1);

	_global_rand = new unsigned int[_maxReservoirRand];
	for (unsigned int i = 0; i < _maxReservoirRand; i++) {
		std::uniform_int_distribution<unsigned int> distribution(0, i);
		_global_rand[i] = distribution(generator1);
	}
#if defined OPENCL_HASHTABLE
	_globalRand_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_maxReservoirRand * sizeof(unsigned int), NULL, &_err);
	_err = clEnqueueWriteBuffer(command_queue_gpu, _globalRand_obj, CL_TRUE, 0,
		_maxReservoirRand * sizeof(unsigned int), _global_rand, 0, NULL, NULL);
#endif
	std::cout << "Completed. " << std::endl;
	
	/* Hash tables. */
	_tableMemReservoirMax = (_numTables - 1) * _aggNumReservoirs + _numReservoirsHashed;
	_tableMemMax = _tableMemReservoirMax * (1 + _reservoirSize);
	_tablePointerMax = _numTables * _numReservoirsHashed;
#if defined OPENCL_HASHTABLE
	std::cout << "Initializing GPU-OpenCL tables and pointers ...  " << std::endl;
	_tableMem_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_tableMemMax * sizeof(unsigned int), NULL, &_err);
	clCheckError(_err, "[initHelper] Failed to alloc GPU _tableMem_obj.");
	_err = clEnqueueFillBuffer(command_queue_gpu, _tableMem_obj, &_zero, sizeof(const int), 0,
		_tableMemMax * sizeof(unsigned int), 0, NULL, NULL);
	clCheckError(_err, "[initHelper] Failed to init GPU _tableMem_obj.");

	_tableMemAllocator_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_numTables * sizeof(unsigned int), NULL, &_err);
	clCheckError(_err, "[initHelper] Failed to alloc GPU _tableMemAllocator_obj.");
	_err = clEnqueueFillBuffer(command_queue_gpu, _tableMemAllocator_obj, &_zero, sizeof(const int), 0,
		_numTables * sizeof(unsigned int), 0, NULL, NULL);
	clCheckError(_err, "[initHelper] Failed to init GPU _tableMemAllocator_obj.");

	_tablePointers_obj = clCreateBuffer(context_gpu, CL_MEM_READ_WRITE,
		_tablePointerMax * sizeof(unsigned int), NULL, &_err);
	clCheckError(_err, "[initHelper] Failed to alloc GPU _tablePointers_obj.");
	_err = clEnqueueFillBuffer(command_queue_gpu, _tablePointers_obj, &_tableNull, sizeof(const int), 0,
		_tablePointerMax * sizeof(unsigned int), 0, NULL, NULL);
	clCheckError(_err, "[initHelper] Failed to init GPU _tablePointers_obj.");

	clFinish(command_queue_gpu);
	std::cout << "Completed. \n";
#elif defined CPU_TB
	std::cout << "Initializing CPU tables and pointers ... " << std::endl;
	_tableMem = new unsigned int[_tableMemMax]();
	_tableMemAllocator = new unsigned int[_numTables]();
	_tablePointers = new unsigned int[_tablePointerMax];
	_tablePointersLock = new omp_lock_t[_tablePointerMax];
	std::cout << "Completed. " << std::endl;
	std::cout << "Initializing CPU tablePointers/Locks ... " << std::endl;
	for (unsigned long long i = 0; i < _tablePointerMax; i++) {
		_tablePointers[i] = TABLENULL;
		omp_init_lock(_tablePointersLock + i);
	}
	std::cout << "Completed. " << std::endl;
	std::cout << "Initializing CPU tableCountersLocks ... " << std::endl;
	_tableCountersLock = new omp_lock_t[_tableMemReservoirMax]; 
	for (unsigned long long i = 0; i < _tableMemReservoirMax; i++) {
		omp_init_lock(_tableCountersLock + i);
	}
	std::cout << "Completed. " << std::endl;
#endif

	/* Hashing counter. */
	_sequentialIDCounter_kernel = 0;
}

LSHReservoirSampler::~LSHReservoirSampler() {
#ifdef OPENCL_2XX
	clFlush(command_queue_gpu);
	clFinish(command_queue_gpu);
	clReleaseProgram(program_gpu);
	clReleaseCommandQueue(command_queue_gpu);
	clReleaseContext(context_gpu);
	free(devices_gpu);

	//clFlush(command_queue_cpu);
	//clFinish(command_queue_cpu);
	//clReleaseProgram(program_cpu);
	//clReleaseCommandQueue(command_queue_cpu);
	//clReleaseContext(context_cpu);
	//free(devices_cpu);

	free(platforms);

	
	clReleaseKernel(kernel_reservoir);
	clReleaseKernel(kernel_addtable);
	clReleaseKernel(kernel_extract_rows);
	clReleaseKernel(kernel_taketopk);
	clReleaseKernel(kernel_bsort_preprocess);
	clReleaseKernel(kernel_bsort_postprocess);
	clReleaseKernel(kernel_bsort_init_manning);
	clReleaseKernel(kernel_bsort_stage_0_manning);
	clReleaseKernel(kernel_bsort_stage_n_manning);
	clReleaseKernel(kernel_bsort_stage_0_manning_kv);
	clReleaseKernel(kernel_bsort_stage_n_manning_kv);
	clReleaseKernel(kernel_bsort_init_manning_kv);

	clReleaseKernel(kernel_markdiff);
	clReleaseKernel(kernel_aggdiff);
	clReleaseKernel(kernel_subtractdiff);
	clReleaseKernel(kernel_tally_naive);
#endif

	unInit();
}

void LSHReservoirSampler::unInit() {
#if defined OPENCL_HASHTABLE
	clReleaseMemObject(_tableMem_obj);
	clReleaseMemObject(_tableMemAllocator_obj);
	clReleaseMemObject(_tablePointers_obj);
	clReleaseMemObject(_globalRand_obj);
#elif defined CPU_TB
	delete[] _tableMem;
	delete[] _tablePointers;
	delete[] _tableMemAllocator;
	for (unsigned long long i = 0; i < _tablePointerMax; i++) {
		omp_destroy_lock(_tablePointersLock + i);
	}
	for (unsigned long long i = 0; i < _tableMemReservoirMax; i++) {
		omp_destroy_lock(_tableCountersLock + i);
	}
	delete[] _tablePointersLock;
	delete[] _tableCountersLock;
#endif
	delete[] _global_rand;
}

#ifdef OPENCL_2XX
void LSHReservoirSampler::clPlatformDevices() {

	// Platforms. 
	cl_uint num_platforms;
	cl_int platform_index = -1;
	_err = clGetPlatformIDs(1, NULL, &num_platforms);
	printf("[OpenCL] %d platform found. \n", num_platforms);
	clCheckError(_err, "[OpenCL] Couldn't find any platforms.");

	platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
	clGetPlatformIDs(num_platforms, platforms, NULL);
	cl_uint num_devices;
	
	// GPU Platform. 
	_err = clGetDeviceIDs(platforms[CL_PLATFORM_ID], CL_DEVICE_TYPE_ALL, 1, NULL, &num_devices); 
	printf("[OpenCL] %d GPU device found. \n", num_devices); 
	clCheckError(_err, "[OpenCL] Couldn't find any GPU devices."); 
	devices_gpu = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices); 
	clGetDeviceIDs(platforms[CL_PLATFORM_ID], CL_DEVICE_TYPE_ALL, num_devices, devices_gpu, NULL); 
	
	// CPU Platform. 
	//_err = clGetDeviceIDs(platforms[CL_CPU_PLATFORM], CL_DEVICE_TYPE_ALL, 1, NULL, &num_devices);
	//printf("[OpenCL] %d CPU device found. \n", num_devices);
	//clCheckError(_err, "[OpenCL] Couldn't find any CPU devices.");
	//devices_cpu = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices);
	//clGetDeviceIDs(platforms[CL_CPU_PLATFORM], CL_DEVICE_TYPE_ALL, num_devices, devices_cpu, NULL);
		
#ifdef PRINT_CLINFO
	cl_uint q0;
	size_t q1;
	cl_ulong q2;
	char name_data[48], ext_data[4096];

	for (int d = 0; d < num_devices; d++) {
		printf("\n");
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_NAME, sizeof(name_data), name_data, NULL);
		printf("<<< Platform %d Device Info: %s >>> \n", d, name_data);
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_ADDRESS_BITS, sizeof(q0), &q0, NULL);
		printf("CL_DEVICE_ADDRESS_BITS: %u\n", q0);
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(q0), &q0, NULL);
		printf("CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: %u\n", q0);
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(q2), &q2, NULL);
		printf("CL_DEVICE_GLOBAL_MEM_SIZE: %" PRIu64 "\n", q2);
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(q2), &q2, NULL);
		printf("CL_DEVICE_LOCAL_MEM_SIZE: %" PRIu64 "\n", q2);
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(q0), &q0, NULL);
		printf("CL_DEVICE_MAX_COMPUTE_UNITS: %u\n", q0);
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(q1), &q1, NULL);
		printf("CL_DEVICE_MAX_WORK_GROUP_SIZE: %zu\n", q1);
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(q0), &q0, NULL);
		printf("CL_DEVICE_MAX_MEM_ALLOC_SIZE: %u\n", q0);
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_MAX_PARAMETER_SIZE, sizeof(q0), &q0, NULL);
		printf("CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT: %u\n", q0);
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, sizeof(q0), &q0, NULL);
		printf("CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT: %u\n", q0);
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof(q0), &q0, NULL);
		printf("CL_DEVICE_MAX_PARAMETER_SIZE: %u\n", q0);
		clGetDeviceInfo(devices_gpu[d], CL_DEVICE_EXTENSIONS, sizeof(ext_data), ext_data, NULL);
		printf("CL_DEVICE_EXTENSIONS: %s\n", ext_data);
		printf("\n");
	}
#endif
}

void LSHReservoirSampler::clContext() {
	cl_uint num_context_devices; // TODO, currenly use 1 device for each platform. 
	
	// GPU context. 
	context_gpu = clCreateContext(NULL, 1, devices_gpu + CL_DEVICE_ID, NULL, NULL, &_err); 
	clCheckError(_err, "[OpenCL] Couldn't create a context."); 
	_err = clGetContextInfo(context_gpu, CL_CONTEXT_NUM_DEVICES,
		sizeof(cl_uint), &num_context_devices, NULL); 
	printf("[OpenCL] Created GPU Context with %d device. \n", num_context_devices); 

	// CPU context. 
	//context_cpu = clCreateContext(NULL, 1, devices_cpu + CL_CPU_DEVICE, NULL, NULL, &_err);
	//clCheckError(_err, "[OpenCL] Couldn't create a context.");
	//_err = clGetContextInfo(context_cpu, CL_CONTEXT_NUM_DEVICES,
	//	sizeof(cl_uint), &num_context_devices, NULL);
	//printf("[OpenCL] Created CPU Context with %d device. \n", num_context_devices);
}

void LSHReservoirSampler::clProgram() {

	// Load and creat program. 
	FILE *program_handle;
	const char *file_name[] = { PROGRAM_FILE_1, PROGRAM_FILE_2 };
	const char options[] = "-cl-finite-math-only -cl-no-signed-zeros -w -cl-mad-enable -cl-fast-relaxed-math -I ./";
	size_t program_size[NUM_FILES];
	size_t log_size;

	char *program_buffer[NUM_FILES];
	for (int i = 0; i < NUM_FILES; i++) {
		program_handle = fopen(file_name[i], "r");
		if (program_handle == NULL) {
			perror("[OpenCL] Couldn't find the program file");
			pause();
			exit(1);
		}
		fseek(program_handle, 0, SEEK_END);
		program_size[i] = ftell(program_handle);
		rewind(program_handle);
		
		program_buffer[i] = (char*)malloc(program_size[i] + 1);
		program_buffer[i][program_size[i]] = '\0';
		fread(program_buffer[i], sizeof(char),
			program_size[i], program_handle);
		fclose(program_handle);
		printf("[OpenCL] Program %d loaded, %d characters. \n", i, (int) program_size[i]);
	}

	program_gpu = clCreateProgramWithSource(context_gpu, NUM_FILES,
		(const char**)program_buffer, program_size, &_err);
	clCheckError(_err, "[OpenCL] Couldn't create CL program for GPU.");

	//program_cpu = clCreateProgramWithSource(context_cpu, NUM_FILES,
	//	(const char**)program_buffer, program_size, &_err);
	//clCheckError(_err, "[OpenCL] Couldn't create CL program for CPU.");

	// Build GPU program. 
	_err = clBuildProgram(program_gpu, 1, devices_gpu + CL_DEVICE_ID, options, NULL, NULL);
	if (_err < 0) {
		clGetProgramBuildInfo(program_gpu, devices_gpu[CL_DEVICE_ID],
			CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		_program_log = (char*)malloc(log_size + 1);
		_program_log[log_size] = '\0';
		clGetProgramBuildInfo(program_gpu, devices_gpu[CL_DEVICE_ID],
			CL_PROGRAM_BUILD_LOG,
			log_size + 1, _program_log, NULL);
		printf("%s\n", _program_log);
		free(_program_log);
		system("pause");
		exit(1);
	}

	// Build CPU program. 
	//_err = clBuildProgram(program_cpu, 1, devices_cpu + CL_CPU_DEVICE, options, NULL, NULL);
	//if (_err < 0) {
	//	clGetProgramBuildInfo(program_cpu, devices_cpu[CL_CPU_DEVICE],
	//		CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
	//	_program_log = (char*)malloc(log_size + 1);
	//	_program_log[log_size] = '\0';
	//	clGetProgramBuildInfo(program_cpu, devices_cpu[CL_CPU_DEVICE],
	//		CL_PROGRAM_BUILD_LOG,
	//		log_size + 1, _program_log, NULL);
	//	printf("%s\n", _program_log);
	//	free(_program_log);
	//	system("pause");
	//	exit(1);
	//}

	for (int i = 0; i < NUM_FILES; i++) {
		free(program_buffer[i]);
	}
}

void LSHReservoirSampler::clKernels() {

	kernel_reservoir = clCreateKernel(program_gpu, "reservoir_sampling_recur", NULL);
	kernel_addtable = clCreateKernel(program_gpu, "add_table", NULL);
	kernel_extract_rows = clCreateKernel(program_gpu, "extract_rows", NULL);

	kernel_markdiff = clCreateKernel(program_gpu, "mark_diff", NULL);
	kernel_aggdiff = clCreateKernel(program_gpu, "agg_diff", NULL);
	kernel_subtractdiff = clCreateKernel(program_gpu, "subtract_diff", NULL);
	kernel_tally_naive = clCreateKernel(program_gpu, "talley_count", NULL);
	kernel_taketopk = clCreateKernel(program_gpu, "take_topk", NULL);

	kernel_bsort_preprocess = clCreateKernel(program_gpu, "bsort_preprocess_kv", NULL);
	kernel_bsort_postprocess = clCreateKernel(program_gpu, "bsort_postprocess_kv", NULL);
	kernel_bsort_init_manning = clCreateKernel(program_gpu, "bsort_init_manning", NULL);
	kernel_bsort_stage_0_manning = clCreateKernel(program_gpu, "bsort_stage_0_manning", NULL);
	kernel_bsort_stage_n_manning = clCreateKernel(program_gpu, "bsort_stage_n_manning", NULL);
	kernel_bsort_stage_0_manning_kv = clCreateKernel(program_gpu, "bsort_stage_0_manning_kv", NULL);
	kernel_bsort_stage_n_manning_kv = clCreateKernel(program_gpu, "bsort_stage_n_manning_kv", NULL);
	kernel_bsort_init_manning_kv = clCreateKernel(program_gpu, "bsort_init_manning_kv", NULL);

	if (kernel_reservoir == NULL ||
		kernel_addtable == NULL || kernel_taketopk == NULL ||
		kernel_extract_rows == NULL || 
		kernel_bsort_init_manning == NULL || 
		kernel_bsort_preprocess == NULL || kernel_bsort_postprocess == NULL ||
		kernel_bsort_stage_0_manning == NULL || kernel_bsort_stage_n_manning == NULL ||
		kernel_bsort_stage_0_manning_kv == NULL || kernel_bsort_stage_n_manning_kv == NULL ||
		kernel_bsort_init_manning_kv == NULL || kernel_tally_naive == NULL || 
		kernel_markdiff == NULL || kernel_aggdiff == NULL || kernel_subtractdiff == NULL) {
		printf("[OpenCL] One or more GPU kernels failed to be created. \n");
		pause();
		exit(1);
	} else {
		printf("[OpenCL] GPU Kernels successfully created. \n");
	}

}

void LSHReservoirSampler::clCommandQueue() {
	// Create command queue.Properties(2): CL_QUEUE_PROFILING_ENABLE, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE. 
#ifdef OPENCL_2XX
	command_queue_gpu = clCreateCommandQueueWithProperties(context_gpu, devices_gpu[CL_DEVICE_ID], NULL, &_err);
	clCheckError(_err, "[OpenCL] Couldn't create command queue for GPU.");
	//command_queue_cpu = clCreateCommandQueueWithProperties(context_cpu, devices_cpu[CL_CPU_DEVICE], NULL, &_err);
	//clCheckError(_err, "[OpenCL] Couldn't create command queue for CPU.");
#else
	command_queue_gpu = clCreateCommandQueue(context_gpu, devices_gpu[CL_DEVICE_ID], NULL, &_err);
	clCheckError(_err, "[OpenCL] Couldn't create command queue for GPU.");
	//command_queue_cpu = clCreateCommandQueue(context_cpu, devices_cpu[CL_CPU_DEVICE], NULL, &_err);
	//clCheckError(_err, "[OpenCL] Couldn't create command queue for CPU.");
#endif
}

#endif