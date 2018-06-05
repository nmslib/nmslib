#pragma once

// Uncomment if there's OPENCL version 2 present
//#define OPENCL_2XX


// Customize processing architecture.
#ifdef OPENCL_2XX
#define OPENCL_HASHTABLE // Placing the hashtable in the OpenCL device.
#define OPENCL_HASHING   // Perform hashing in the OpenCL device.
#define OPENCL_KSELECT	 // Perform k-selection in the OpenCL device.

// Select the id of the desired platform and device, only relevant when using OpenCl. 
// An overview of the platforms and devices can be queried through the OpenCL framework. 
// On Linux, a package "clinfo" is also capable of outputing the platform and device information. 
#define CL_PLATFORM_ID 1
#define CL_DEVICE_ID 0

#endif
















/* Performance tuning params, Do not touch. */
#define wg_segSize 512			// Number of workgroup element, an integral factor of the segmentSize. 
#define l_segSize 64			// Number of elements each thread will tally. 
//#define SECONDARY_HASHING

#if !defined(OPENCL_HASHTABLE)
#define CPU_TB
#endif

#if !defined(OPENCL_HASHING)
#define CPU_HASHING
#endif

#if !defined(OPENCL_KSELECT)
#define CPU_KSELECT
#endif
