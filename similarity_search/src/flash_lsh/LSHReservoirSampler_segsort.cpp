#include "LSHReservoirSampler.h"

/*
* Function:  segmentedSortKV
* --------------------
* Segmented sorting of KV pairs, based on the value of the key. Sorted order is bitonic across segments. 
*
*  key_in (in/out): input keys, as cl_mem (on which the sort's value is based), length required to be a power of two
*
*  val_in (in/out): input values, as cl_mem, length required to be a power of two
*
*  segmentSize: length of each segment to be sorted, required to be a power of two
*
*  numSegments: number of segments to be sorted
*
*  returns: nothing
*/
#ifdef OPENCL_2XX
void LSHReservoirSampler::segmentedSortKV(cl_mem* key_in, cl_mem* val_in, int segmentSize, int numSegments, unsigned int valMax) {
	int stage, high_stage, num_stages;
	size_t localSize, globalSize;
	localSize = std::min(512, segmentSize / 8);
	globalSize = segmentSize * numSegments / 8;
	num_stages = (int) 2 * (segmentSize / 8) / localSize;

	/* Preprocess. */
	_err = clSetKernelArg(kernel_bsort_preprocess, 0, sizeof(cl_mem), key_in);
	_err = clSetKernelArg(kernel_bsort_preprocess, 1, sizeof(cl_mem), val_in);
	_err = clSetKernelArg(kernel_bsort_preprocess, 2, sizeof(unsigned int), &valMax);
	clCheckError(_err, "kernel_bsort_preprocess set argument failed!");
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_bsort_preprocess, 1, NULL, &globalSize,
		&localSize, 0, NULL, NULL);
	clCheckError(_err, "kernel_bsort_preprocess failed!");

	/* Create kernel argument */
	_err = clSetKernelArg(kernel_bsort_init_manning_kv, 0, sizeof(cl_mem), key_in);
	_err |= clSetKernelArg(kernel_bsort_stage_0_manning_kv, 0, sizeof(cl_mem), key_in);
	_err |= clSetKernelArg(kernel_bsort_stage_n_manning_kv, 0, sizeof(cl_mem), key_in);
	_err |= clSetKernelArg(kernel_bsort_init_manning_kv, 1, 8 * localSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_bsort_stage_0_manning_kv, 1, 8 * localSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_bsort_stage_n_manning_kv, 1, 8 * localSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_bsort_init_manning_kv, 2, sizeof(cl_mem), val_in);
	_err |= clSetKernelArg(kernel_bsort_stage_0_manning_kv, 2, sizeof(cl_mem), val_in);
	_err |= clSetKernelArg(kernel_bsort_stage_n_manning_kv, 2, sizeof(cl_mem), val_in);
	_err |= clSetKernelArg(kernel_bsort_init_manning_kv, 3, 8 * localSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_bsort_stage_0_manning_kv, 3, 8 * localSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_bsort_stage_n_manning_kv, 3, 8 * localSize * sizeof(unsigned int), NULL);

	clCheckError(_err, "kernel_bsort_kv set argument failed!");

	/* Enqueue initial sorting kernel */
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_bsort_init_manning_kv, 1, NULL, &globalSize,
		&localSize, 0, NULL, NULL);
	clCheckError(_err, "kernel_bsort_init_manning_kv failed!");

	/* Execute further stages */
	for (high_stage = 2; high_stage < num_stages; high_stage <<= 1) {

		_err = clSetKernelArg(kernel_bsort_stage_0_manning_kv, 4, sizeof(int), &high_stage);
		_err |= clSetKernelArg(kernel_bsort_stage_n_manning_kv, 5, sizeof(int), &high_stage);
		clCheckError(_err, "kernel_bsort_kv set argument failed!");

		for (stage = high_stage; stage > 1; stage >>= 1) {

			_err = clSetKernelArg(kernel_bsort_stage_n_manning_kv, 4, sizeof(int), &stage);
			clCheckError(_err, "kernel_bsort_kv set argument failed!");

			_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_bsort_stage_n_manning_kv, 1, NULL,
				&globalSize, &localSize, 0, NULL, NULL);
			clCheckError(_err, "kernel_bsort_stage_n_manning_kv failed!");
		}

		_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_bsort_stage_0_manning_kv, 1, NULL,
			&globalSize, &localSize, 0, NULL, NULL);
		clCheckError(_err, "kernel_bsort_stage_0_manning_kv failed!");
	}

	/* Postprocess. */
	_err = clSetKernelArg(kernel_bsort_postprocess, 0, sizeof(cl_mem), key_in);
	_err = clSetKernelArg(kernel_bsort_postprocess, 1, sizeof(cl_mem), val_in);
	_err = clSetKernelArg(kernel_bsort_postprocess, 2, sizeof(unsigned int), &valMax);
	clCheckError(_err, "kernel_bsort_postprocess set argument failed!");
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_bsort_postprocess, 1, NULL, &globalSize,
		&localSize, 0, NULL, NULL);
	clCheckError(_err, "kernel_bsort_postprocess failed!");
	clFinish(command_queue_gpu);

}

/*
* Function:  segmentedSort
* --------------------
* Segmented sorting. Sorted order is bitonic across segments.
*
*  in (in/out): input values, as cl_mem (on which the sort's value is based), length required to be a power of two
*
*  segmentSize: length of each segment to be sorted, required to be a power of two
*
*  numSegments: number of segments to be sorted
*
*  returns: nothing
*/
void LSHReservoirSampler::segmentedSort(cl_mem* in, int segmentSize, int numSegments) {
	int stage, high_stage, num_stages;
	size_t localSize, globalSize;
	localSize = std::min(1024, segmentSize / 8);
	globalSize = segmentSize * numSegments / 8;
	num_stages = (int) 2 * (segmentSize / 8) / localSize;

	/* Create kernel argument */
	_err = clSetKernelArg(kernel_bsort_init_manning, 0, sizeof(cl_mem), in);
	_err |= clSetKernelArg(kernel_bsort_stage_0_manning, 0, sizeof(cl_mem), in);
	_err |= clSetKernelArg(kernel_bsort_stage_n_manning, 0, sizeof(cl_mem), in);
	_err |= clSetKernelArg(kernel_bsort_init_manning, 1, 8 * localSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_bsort_stage_0_manning, 1, 8 * localSize * sizeof(unsigned int), NULL);
	_err |= clSetKernelArg(kernel_bsort_stage_n_manning, 1, 8 * localSize * sizeof(unsigned int), NULL);
	clCheckError(_err, "kernel_bsort set argument failed!");

	/* Enqueue initial sorting kernel */
	_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_bsort_init_manning, 1, NULL, &globalSize,
		&localSize, 0, NULL, NULL);
	clCheckError(_err, "kernel_bsort_init_manning failed!");

	/* Execute further stages */
	for (high_stage = 2; high_stage < num_stages; high_stage <<= 1) {

		_err = clSetKernelArg(kernel_bsort_stage_0_manning, 2, sizeof(int), &high_stage);
		_err |= clSetKernelArg(kernel_bsort_stage_n_manning, 3, sizeof(int), &high_stage);
		clCheckError(_err, "kernel_bsort set argument failed!");

		for (stage = high_stage; stage > 1; stage >>= 1) {

			_err = clSetKernelArg(kernel_bsort_stage_n_manning, 2, sizeof(int), &stage);
			clCheckError(_err, "kernel_bsort set argument failed!");

			_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_bsort_stage_n_manning, 1, NULL,
				&globalSize, &localSize, 0, NULL, NULL);
			clCheckError(_err, "kernel_bsort_stage_n_manning failed!");
		}

		_err = clEnqueueNDRangeKernel(command_queue_gpu, kernel_bsort_stage_0_manning, 1, NULL,
			&globalSize, &localSize, 0, NULL, NULL);
		clCheckError(_err, "kernel_bsort_stage_0_manning failed!");
	}
	clFinish(command_queue_gpu);
}
#endif
