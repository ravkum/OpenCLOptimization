/*******************************************************************************
 Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1   Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 2   Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************/
#ifndef _GAUSSIANFILTER_H_
#define _GAUSSIANFILTER_H_

/*****************************************************************************
 * Include files                                                              *
 ******************************************************************************/
#include "macros.h"
#include "CL/cl.h"

#define LOCAL_XRES  16
#define LOCAL_YRES  16

#define GAUSSIANFILTER_KERNEL_SOURCE      "gaussianFilter.cl"
#define GAUSSIANFILTER_KERNEL             "gaussianFilterKernel"
#define ENHANCED_KERNEL                   "enhanceFilterKernel"
#define COMBINED_KERNEL                   "combinedFilterKernel"

bool buildKernels(cl_context oclContext, cl_device_id oclDevice,
                cl_kernel *gaussianFilterKernel, cl_kernel *enhancedKernel, cl_kernel *cominedKernel, 
                cl_uint filtSize, cl_uint bitWidth,
                cl_int useLds, cl_int useIntrinsics);
bool setKernelArgs(cl_kernel gaussianFilterKernel, cl_kernel enhancedKernel, cl_kernel combinedKernel, 
                   cl_mem input, cl_mem output1, cl_mem output2, cl_mem filterCoeff, cl_uint width,
                cl_uint height, cl_uint filtSize);
bool runKernels(cl_command_queue oclQueue, cl_kernel gaussianFilterKernel,
                cl_kernel enhancedKernel, cl_kernel combinedKernel, cl_int runCombinedKernel, 
                cl_uint width, cl_uint height);

#endif
