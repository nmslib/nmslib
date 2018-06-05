
#include "LSH.h"
#ifndef UINT_MAX
#define UINT_MAX 0xffffffff
#endif
#define RANDPROJGROUPSIZE 100

/* Constructor - Optimal Densified Minhash - Type 2. */
LSH::LSH(int hashType, int _K_in, int _L_in, int _rangePow_in)	{

	_clEnabled = 0;
	_hashType = hashType;

	_K = _K_in;
	_L = _L_in;
	_numTables = _L_in; // In densified minhash, _numTables is equivalan to _L. Initialized for general usage just in case. 
	_rangePow = _rangePow_in;

	printf("<<< LSH Parameters >>>\n");
	std::cout << "_K " << _K << std::endl;
	std::cout << "_L " << _L << std::endl;
	std::cout << "_rangePow " << _rangePow_in << std::endl;
	std::cout << "_hashType " << _hashType << std::endl;

	rand1 = new int[_K * _L];

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> dis(1, UINT_MAX);

	// Generate rand1 - odd random numbers. 
	for (int i = 0; i < _K * _L; i++)
	{
		rand1[i] = dis(gen);
		if (rand1[i] % 2 == 0)
			rand1[i]++;
	}
	
	_numhashes = _K * _L;
	_lognumhash = log2(_numhashes);
	std::cout << "_lognumhash " << _lognumhash << std::endl;

	// _randa and _randHash* are random odd numbers. 
	_randa = dis(gen);
	if (_randa % 2 == 0)
		_randa++;
	_randHash = new int[2];
	_randHash[0] = dis(gen);
	if (_randHash[0] % 2 == 0)
		_randHash[0]++;
	_randHash[1] = dis(gen);
	if (_randHash[1] % 2 == 0)
		_randHash[1]++;
	std::cout << "Optimal Densified Hashing intialized ...  \n";
}

/* Constructor - SRP - Type 1. */
LSH::LSH(int hashType, int numHashPerFamily, int numHashFamilies, int dimension, int samFactor) {

	_rangePow = numHashPerFamily,
	_numTables = numHashFamilies;
	_dimension = dimension;
	_samSize = (int) floor(dimension / samFactor);;
	_samFactor = samFactor;
	_clEnabled = 0;
	_hashType = hashType;
	_groupHashingSize = RANDPROJGROUPSIZE;

	printf("<<< LSH Parameters >>>\n");
	std::cout << "_rangePow " << _rangePow << std::endl;
	std::cout << "_numTables " << _numTables << std::endl;
	std::cout << "_dimension " << _dimension << std::endl;
	std::cout << "_samSize " << _samSize << std::endl;
	std::cout << "_hashType " << _hashType << std::endl;

	/* Signed random projection. */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	std::cout << "Generating random number of srp hashes of dense data ...  \n";
	// Random number generation for hashing purpose - declarations. 
	// Random number generation for fast random projection. 
	// Reference: Anshumali Shrivastava, Rice CS
	// randBits - random bits deciding to add or subtract, contain randbits for numTable * _rangePow * samSize. 
	_randBits = new short[_numTables * _rangePow * _samSize];
	// indices - selected indices to perform subtraction. Dimension same as randbits. 
	_indices = new int[_numTables * _rangePow * _samSize];

	int *a = new int[_dimension];
	for (int i = 0; i < _dimension; i++) {
		a[i] = i;
	}
	for (int tb = 0; tb < _numTables; tb++) {
		srand(time(0));
		for (int i = 0; i < _rangePow; i++) {
			std::random_shuffle(&a[0], &a[_dimension]);
			for (int j = 0; j < _samSize; j++) {
				_indices[tb * _rangePow * _samSize + i * _samSize + j] = a[j];
				// For 1/2 chance, assign random bit 1, or -1 to randBits. 
				if (rand() % 2 == 0)
					_randBits[tb * _rangePow * _samSize + i * _samSize + j] = 1;
				else
					_randBits[tb * _rangePow * _samSize + i * _samSize + j] = -1;
			}
		}
	}
	delete[] a;

	std::cout << "Generating random number of universal hashes of sparse data ...  \n";
	_hash_a = new unsigned int[_rangePow * _numTables];
	_hash_b = new unsigned int[_rangePow * _numTables];
	_binhash_a = new unsigned int[_rangePow * _numTables];
	_binhash_b = new unsigned int[_rangePow * _numTables];

	std::default_random_engine generator0;
	for (int i = 0; i < _rangePow * _numTables; i++) {
		std::uniform_int_distribution<unsigned int> distribution(0, 0x7FFFFFFF);
		_binhash_a[i] = (distribution(generator0)) * 2 + 1;
	}
	for (int i = 0; i < _rangePow * _numTables; i++) {
		std::uniform_int_distribution<unsigned int> distribution(0, 0x7FFFFFFF);
		_binhash_b[i] = distribution(generator0);
	}
	for (int i = 0; i < _rangePow * _numTables; i++) {
		std::uniform_int_distribution<unsigned int> distribution(0, 0x7FFFFFFF);
		_hash_a[i] = (distribution(generator0)) * 2 + 1;
	}
	for (int i = 0; i < _rangePow * _numTables; i++) {
		std::uniform_int_distribution<unsigned int> distribution(0, 0xFFFFFFFF >> _samFactor);
		_hash_b[i] = distribution(generator0);
	}

}

LSH::~LSH() {

	if (_clEnabled) {

		switch (_hashType)
		{
		case 1:
#ifdef OPENCL_2XX
			clReleaseKernel(kernel_randproj_dense);
			clReleaseKernel(kernel_randproj_sparse);
			clReleaseMemObject(_randBits_obj);
			clReleaseMemObject(_indices_obj);
			clReleaseMemObject(_hash_a_obj);
			clReleaseMemObject(_hash_b_obj);
			clReleaseMemObject(_binhash_a_obj);
			clReleaseMemObject(_binhash_b_obj);
#endif
			break;
		case 2:
			break;
		default:
			break;
		}

	}

	switch (_hashType)
	{
	case 1:
		delete[] _binhash_a;
		delete[] _binhash_b;
		delete[] _hash_a;
		delete[] _hash_b;
		delete[] _randBits;
		delete[] _indices;
		break;
	case 2:
		delete[] _randHash;
		delete[] rand1;
		break;
	default:
		break;
	}
	
}

#ifdef OPENCL_2XX
void LSH::clLSH(cl_platform_id *platforms_lsh, cl_device_id *devices_lsh, cl_context context_lsh,
	cl_program program_lsh, cl_command_queue command_queue_lsh) {

	_platforms_lsh = platforms_lsh;
	_devices_lsh = devices_lsh;
	_context_lsh = context_lsh;
	_program_lsh = program_lsh;
	_command_queue_lsh = command_queue_lsh;

	clProgram_LSH(); // Build .cl source files. 

	kernel_mult_probes_storeid = clCreateKernel(_program_lsh, "mult_probes_storeid", NULL);
	kernel_mult_probes = clCreateKernel(_program_lsh, "mult_probes", NULL);

	_err = clSetKernelArg(kernel_mult_probes_storeid, 4, sizeof(int), (void *)&_rangePow);
	_err |= clSetKernelArg(kernel_mult_probes_storeid, 5, sizeof(int), (void *)&_numTables);
	_err |= clSetKernelArg(kernel_mult_probes, 3, sizeof(int), (void *)&_rangePow);
	_err |= clSetKernelArg(kernel_mult_probes, 4, sizeof(int), (void *)&_numTables);
	ClCheckError(_err, "[LSH::clLSH] Failed to set kernel_mult_probes_storeid & kernel_mult_probes arguments!");

	switch (_hashType)
	{
	case 1:
		_clEnabled = 1;

		_randBits_obj = clCreateBuffer(_context_lsh, CL_MEM_READ_WRITE,
			_numTables * _rangePow * _samSize * sizeof(short), NULL, &_err);
		_indices_obj = clCreateBuffer(_context_lsh, CL_MEM_READ_WRITE,
			_numTables * _rangePow * _samSize * sizeof(int), NULL, &_err);

		_err = clEnqueueWriteBuffer(_command_queue_lsh, _randBits_obj, CL_TRUE, 0,
			_numTables * _rangePow * _samSize * sizeof(short), _randBits, 0, NULL, NULL);
		_err |= clEnqueueWriteBuffer(_command_queue_lsh, _indices_obj, CL_TRUE, 0,
			_numTables * _rangePow * _samSize * sizeof(int), _indices, 0, NULL, NULL);

		kernel_randproj_dense = clCreateKernel(_program_lsh, "dense_rand_proj", NULL);
		kernel_randproj_sparse = clCreateKernel(_program_lsh, "sparse_rand_proj", NULL);

		if (kernel_randproj_dense == NULL || kernel_randproj_sparse == NULL) {
			printf("[LSH::clLSH] One or more CPU kernels failed to be created. \n");
		}

		_hash_a_obj = clCreateBuffer(_context_lsh, CL_MEM_READ_WRITE,
			_rangePow * _numTables * sizeof(unsigned int), NULL, &_err);
		_hash_b_obj = clCreateBuffer(_context_lsh, CL_MEM_READ_WRITE,
			_rangePow * _numTables * sizeof(unsigned int), NULL, &_err);
		_err = clEnqueueWriteBuffer(_command_queue_lsh, _hash_a_obj, CL_TRUE, 0,
			_rangePow * _numTables * sizeof(unsigned int), _hash_a, 0, NULL, NULL);
		_err |= clEnqueueWriteBuffer(_command_queue_lsh, _hash_b_obj, CL_TRUE, 0,
			_rangePow * _numTables * sizeof(unsigned int), _hash_b, 0, NULL, NULL);
		_binhash_a_obj = clCreateBuffer(_context_lsh, CL_MEM_READ_WRITE,
			_rangePow * _numTables * sizeof(unsigned int), NULL, &_err);
		_binhash_b_obj = clCreateBuffer(_context_lsh, CL_MEM_READ_WRITE,
			_rangePow * _numTables * sizeof(unsigned int), NULL, &_err);
		_err |= clEnqueueWriteBuffer(_command_queue_lsh, _binhash_a_obj, CL_TRUE, 0,
			_rangePow * _numTables * sizeof(unsigned int), _binhash_a, 0, NULL, NULL);
		_err |= clEnqueueWriteBuffer(_command_queue_lsh, _binhash_b_obj, CL_TRUE, 0,
			_rangePow * _numTables * sizeof(unsigned int), _binhash_b, 0, NULL, NULL);

		_err = clSetKernelArg(kernel_randproj_dense, 2, sizeof(cl_mem), (void *)&_randBits_obj);
		_err |= clSetKernelArg(kernel_randproj_dense, 3, sizeof(cl_mem), (void *)&_indices_obj);
		_err |= clSetKernelArg(kernel_randproj_dense, 5, sizeof(int), (void *)&_samSize);
		_err |= clSetKernelArg(kernel_randproj_dense, 6, sizeof(int), (void *)&_dimension);
		_err |= clSetKernelArg(kernel_randproj_dense, 7, sizeof(int), (void *)&_rangePow);
		_err |= clSetKernelArg(kernel_randproj_dense, 8, sizeof(int), (void *)&_groupHashingSize);
		_err |= clSetKernelArg(kernel_randproj_dense, 9, _samSize * _rangePow * sizeof(int), NULL);
		_err |= clSetKernelArg(kernel_randproj_dense, 10, _samSize * _rangePow * sizeof(short), NULL);
		_err |= clSetKernelArg(kernel_randproj_dense, 11, _groupHashingSize * _rangePow * sizeof(int), NULL);
		ClCheckError(_err, "[LSH::clLSH] Failed to set kernel_randproj_dense arguments!");
		_err = clSetKernelArg(kernel_randproj_sparse, 4, sizeof(cl_mem), (void *)&_hash_a_obj);
		_err |= clSetKernelArg(kernel_randproj_sparse, 5, sizeof(cl_mem), (void *)&_hash_b_obj);
		_err |= clSetKernelArg(kernel_randproj_sparse, 6, sizeof(cl_mem), (void *)&_binhash_a_obj);
		_err |= clSetKernelArg(kernel_randproj_sparse, 7, sizeof(cl_mem), (void *)&_binhash_b_obj);
		_err |= clSetKernelArg(kernel_randproj_sparse, 9, sizeof(int), (void *)&_rangePow);
		_err |= clSetKernelArg(kernel_randproj_sparse, 10, sizeof(int), (void *)&_samFactor);
		_err |= clSetKernelArg(kernel_randproj_sparse, 11, sizeof(int), (void *)&_groupHashingSize);
		_err |= clSetKernelArg(kernel_randproj_sparse, 12, _groupHashingSize * _rangePow * sizeof(int), NULL);
		ClCheckError(_err, "[LSH::clLSH] Failed to set kernel_randproj_sparse arguments!");
		break;

	case 2:
		std::cout << "[LSH::clLSH] No OpenCL implementation: Optimal Densified MinHash. ";
		break;
	default:
		break;
	}

}

void LSH::clProgram_LSH() {

	// Load and creat program. 
	FILE *program_handle;
	const char *file_name[] = { CL_KERNEL_FILE_1 };
	const char options[] = "-cl-finite-math-only -cl-no-signed-zeros -w -cl-mad-enable -cl-fast-relaxed-math -I ./";
	size_t program_size[NUM_CL_KERNEL];
	size_t log_size;

	char *program_buffer[NUM_CL_KERNEL];
	for (int i = 0; i < NUM_CL_KERNEL; i++) {
		program_handle = fopen(file_name[i], "r");
		if (program_handle == NULL) {
			perror("[OpenCL] Couldn't find the program file");
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

	_program_lsh = clCreateProgramWithSource(_context_lsh, NUM_CL_KERNEL,
		(const char**)program_buffer, program_size, &_err);

	if (_err != 0) {
		printf("[LSH] Couldn't create CL program for lsh.");
		printf("\nError Code: %d\n", _err);
	}

	// Build lsh program. 
	_err = clBuildProgram(_program_lsh, 1, _devices_lsh, options, NULL, NULL);
	if (_err < 0) {
		clGetProgramBuildInfo(_program_lsh, _devices_lsh[0],
			CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		_program_log_lsh = (char*)malloc(log_size + 1);
		_program_log_lsh[log_size] = '\0';
		clGetProgramBuildInfo(_program_lsh, _devices_lsh[0],
			CL_PROGRAM_BUILD_LOG,
			log_size + 1, _program_log_lsh, NULL);
		printf("%s\n", _program_log_lsh);
		free(_program_log_lsh);
		system("pause");
		exit(1);
	}

	for (int i = 0; i < NUM_CL_KERNEL; i++) {
		free(program_buffer[i]);
	}
}


void LSH::ClCheckError(cl_int code, const char* msg) {
	if (code != 0) {
		printf(msg);
		printf("\nError Code: %d\n", code);
		exit(1);
	}
}
#endif