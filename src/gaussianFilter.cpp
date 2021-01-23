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
/**
 ********************************************************************************
 * @file <gaussianFilter.cpp>
 *
 * @brief Contains functions to compile and run the gaussian filter kernel
 *
 ********************************************************************************
 */
#include "gaussianFilter.h"

/**
 *******************************************************************************
 *  @fn     buildGaussianFilterKernel
 *  @brief  This function builds the necessary kernels for gaussian filter
 *
 *  @param[in] oclContext       : pointer to the OCL context
 *  @param[in] oclDevice        : pointer to the OCL device
 *  @param[out] gaussianFilterKernel : pointer to the kernel
 *  @param[in] filtSize         : Filter size if set to = 3 - 3x3 gaussianarable filter
 *                                                      = 5 - 5x5 gaussianarable filter
 *                                                      
 *  @param[in] bitWidth         : Bits per pixel
 *  @param[in] useLds           : Should Lds memory be used by the OpenCL kernel for input
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool buildKernels(cl_context oclContext, cl_device_id oclDevice,
                cl_kernel *gaussianFilterKernel, cl_kernel *enhancedKernel, cl_kernel *cominedKernel, 
                cl_uint filtSize, cl_uint bitWidth,
                cl_int useLds, cl_int useIntrinsics)
{
    cl_int err = CL_SUCCESS;

    /**************************************************************************
     * Read kernel source file into buffer.                                    *
     **************************************************************************/
    const char *filename = GAUSSIANFILTER_KERNEL_SOURCE;
    char *source = NULL;
    size_t sourceSize = 0;
    err = convertToString(filename, &source, &sourceSize);
    CHECK_RESULT(err != CL_SUCCESS, "Error reading file %s ", filename);

    /**************************************************************************
     * Create kernel program.                                                  *
     **************************************************************************/
    cl_program programNonSeparableFilter =
                    clCreateProgramWithSource(oclContext, 1,
                                    (const char **) &source,
                                    (const size_t *) &sourceSize, &err);
    CHECK_RESULT(err != CL_SUCCESS,
                    "clCreateProgramWithSource failed with Error code = %d",
                    err);

    /**************************************************************************
     * Build the kernel and check for errors. If errors are found, it will be  *
     * printed to console                                                      *
     **************************************************************************/
    char option[256];
    sprintf(option, "-DTAP_SIZE=%d -DPIX_WIDTH=%d -DUSE_LDS=%d -DLOCAL_XRES=%d -DLOCAL_YRES=%d -DUSE_INTRINSICS=%d",
                    filtSize, bitWidth, useLds, LOCAL_XRES, LOCAL_YRES, useIntrinsics);

    err = clBuildProgram(programNonSeparableFilter, 1, &(oclDevice), option, NULL, NULL);
    free(source);
    if (err != CL_SUCCESS)
    {
        char *buildLog = NULL;
        size_t buildLogSize = 0;
        clGetProgramBuildInfo(programNonSeparableFilter, oclDevice,
                        CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog,
                        &buildLogSize);
        buildLog = (char *) malloc(buildLogSize);
        clGetProgramBuildInfo(programNonSeparableFilter, oclDevice,
                        CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, NULL);
        printf("%s\n", buildLog);
        free(buildLog);
        clReleaseProgram(programNonSeparableFilter);
        CHECK_RESULT(true, "clCreateProgram failed with Error code = %d", err);
    }

    /**************************************************************************
     * Create kernel                                                           *
     **************************************************************************/
    *gaussianFilterKernel = clCreateKernel(programNonSeparableFilter, GAUSSIANFILTER_KERNEL,
                    &err);
    CHECK_RESULT(err != CL_SUCCESS, 
                    "clCreateKernel failed with Error code = %d", err);

    *enhancedKernel = clCreateKernel(programNonSeparableFilter, ENHANCED_KERNEL,
                    &err);
    CHECK_RESULT(err != CL_SUCCESS, 
                    "clCreateKernel failed with Error code = %d", err);

    *cominedKernel = clCreateKernel(programNonSeparableFilter, COMBINED_KERNEL,
                    &err);
    CHECK_RESULT(err != CL_SUCCESS, 
                    "clCreateKernel failed with Error code = %d", err);

    clReleaseProgram(programNonSeparableFilter);
    return true;
}

/**
 *******************************************************************************
 *  @fn     setGaussianFilterKernelArgs
 *  @brief  This function sets the arguments for the filter kernel
 *
 *  @param[in] gaussianFilterKernel : pointer to Gaussian filter kernel
 *  @param[in] input           : OCL memory containing input
 *  @param[out] output         : OCL memory to hold output
 *  @param[in] filterCoeff     : OCL memory containing filter coefficients
 *  @param[in] width           : Image width
 *  @param[in] height          : Image height
 *  @param[in] filtSize        : Filter size
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool setKernelArgs(cl_kernel gaussianFilterKernel, cl_kernel enhancedKernel, cl_kernel combinedKernel, 
                   cl_mem input, cl_mem output1, cl_mem output2, cl_mem filterCoeff, cl_uint width,
                cl_uint height, cl_uint filtSize)
{
    int cnt = 0;
    cl_uint extWidth = width + filtSize - 1;
    cl_int err = CL_SUCCESS;

    err  = clSetKernelArg(gaussianFilterKernel, cnt++, sizeof(cl_mem), &(input));
    err |= clSetKernelArg(gaussianFilterKernel, cnt++, sizeof(cl_mem), &(output1));
    err |= clSetKernelArg(gaussianFilterKernel, cnt++, sizeof(cl_uint), &(width));
    err |= clSetKernelArg(gaussianFilterKernel, cnt++, sizeof(cl_uint), &(height));
    err |= clSetKernelArg(gaussianFilterKernel, cnt++, sizeof(cl_uint), &(extWidth));
    err |= clSetKernelArg(gaussianFilterKernel, cnt++, sizeof(cl_mem),
                    &(filterCoeff));

    CHECK_RESULT(err != CL_SUCCESS,
                    "clSetKernelArg failed with Error code = %d", err);

    cnt = 0;
    err  = clSetKernelArg(enhancedKernel, cnt++, sizeof(cl_mem), &(input));
    err |= clSetKernelArg(enhancedKernel, cnt++, sizeof(cl_mem), &(output1));
    err |= clSetKernelArg(enhancedKernel, cnt++, sizeof(cl_mem), &(output2));
    err |= clSetKernelArg(enhancedKernel, cnt++, sizeof(cl_uint), &(width));
    err |= clSetKernelArg(enhancedKernel, cnt++, sizeof(cl_uint), &(height));
    err |= clSetKernelArg(enhancedKernel, cnt++, sizeof(cl_uint), &(extWidth));
    
    CHECK_RESULT(err != CL_SUCCESS,
                    "clSetKernelArg failed with Error code = %d", err);

    cnt = 0;
    err  = clSetKernelArg(combinedKernel, cnt++, sizeof(cl_mem), &(input));
    err |= clSetKernelArg(combinedKernel, cnt++, sizeof(cl_mem), &(output1));
    err |= clSetKernelArg(combinedKernel, cnt++, sizeof(cl_mem), &(output2));
    err |= clSetKernelArg(combinedKernel, cnt++, sizeof(cl_uint), &(width));
    err |= clSetKernelArg(combinedKernel, cnt++, sizeof(cl_uint), &(height));
    err |= clSetKernelArg(combinedKernel, cnt++, sizeof(cl_uint), &(extWidth));
    err |= clSetKernelArg(combinedKernel, cnt++, sizeof(cl_mem),
                    &(filterCoeff));

    CHECK_RESULT(err != CL_SUCCESS,
                    "clSetKernelArg failed with Error code = %d", err);

    return true;
}

/**
 *******************************************************************************
 *  @fn     runKernels
 *  @brief  This function runs the filter kenrel kernels
 *
 *  @param[in] oclQueue        : pointer to the ocl command queue
 *  @param[in] gaussianFilterKernel : pointer to the gaussian filter kernel
 *  @param[in] enhancedKernel : pointer to the enhanced filter kernel
 *  @param[in] combinedKernel : pointer to the enhanced combined kernel
 *  @param[in] width           : X dimension
 *  @param[in] height          : Y dimension
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool runKernels(cl_command_queue oclQueue, cl_kernel gaussianFilterKernel,
                cl_kernel enhancedKernel, cl_kernel combinedKernel, cl_int runCombinedKernel, 
                cl_uint width, cl_uint height)
{
    cl_int err;
    size_t localWorkSize[2] = { LOCAL_XRES, LOCAL_YRES };
    size_t globalWorkSize[2];

    globalWorkSize[0] = (width + localWorkSize[0] - 1) / localWorkSize[0];
    globalWorkSize[0] *= localWorkSize[0];
    globalWorkSize[1] = (height + localWorkSize[1] - 1) / localWorkSize[1];
    globalWorkSize[1] *= localWorkSize[1];

    if (runCombinedKernel)
    {
        err = clEnqueueNDRangeKernel(oclQueue, combinedKernel, 2, NULL,
                        globalWorkSize, localWorkSize, 0, NULL, NULL);
        CHECK_RESULT(err != CL_SUCCESS,
                        "clEnqueueNDRangeKernel failed with Error code = %d", err);
    }
    else
    {
        err = clEnqueueNDRangeKernel(oclQueue, gaussianFilterKernel, 2, NULL,
                        globalWorkSize, localWorkSize, 0, NULL, NULL);
        CHECK_RESULT(err != CL_SUCCESS,
                        "clEnqueueNDRangeKernel failed with Error code = %d", err);

        err = clEnqueueNDRangeKernel(oclQueue, enhancedKernel, 2, NULL,
                        globalWorkSize, localWorkSize, 0, NULL, NULL);
        CHECK_RESULT(err != CL_SUCCESS,
                        "clEnqueueNDRangeKernel failed with Error code = %d", err);
    }

    return true;
}
