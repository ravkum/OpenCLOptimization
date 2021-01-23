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
 * @file <main.cpp>
 *
 * @brief Main file
 *
 ********************************************************************************
 */
#include "gaussianFilterCoeff.h"
#include "gaussianFilter.h"
#include "CL/cl.h"
#include "utils.h"
#include "CLUtil.hpp"
#include "SDKBitMap.hpp"
using namespace appsdk;

/******************************************************************************
 * Default parameter values                                                    *
 ******************************************************************************/
#define DEFAULT_FILTER_SIZE             5
#define DEFAULT_INPUT_IMAGE             "Nature_1600x1200.bmp"
#define DEFAULT_OPENCL_OUTPUT_IMAGE     "gaussianOutput.bmp"
#define DEFAULT_ENH_OUTPUT_IMAGE        "enhancedOutput.bmp"
#define DEFAULT_BITWIDTH                8

/******************************************************************************
 * Structure to hold the parameters for the sample                             *
 ******************************************************************************/
typedef struct filters
{
    cl_uint rows;
    cl_uint cols;

    cl_uint paddedRows;
    cl_uint paddedCols;

    cl_uint filterSize;
    cl_float *gaussianFilterCpu;

    cl_uchar *inputImg;
    cl_uchar *gaussianOutputImg;
    cl_uchar *enhancedOutputImg;

    cl_mem input;
    cl_mem gaussianFilter;
    cl_mem gaussianOutput;
    cl_mem enhancedOutput;    

    cl_kernel gaussianKernel;
    cl_kernel enhancedKernel;
    cl_kernel combinedKernel;

    SDKBitMap inputBitmap;   /**< Bitmap class object */

} filters;

/******************************************************************************
 * Number of runs for performance measurement                                  *
 ******************************************************************************/
#define LOOP_COUNT 10

/******************************************************************************
 * Function declaration                                                        *
 ******************************************************************************/
bool readInput(filters *paramFF, const char *inputImage,
                cl_uint bitWidth);
bool createMemory(filters* paramFF, DeviceInfo *infoDeviceOcl,
                cl_uint bitWidth, cl_int zeroCopy);
void destroyMemory(filters *paramFF, DeviceInfo *infoDeviceOcl);
bool saveOutputs(filters *paramFF, const char *filename1, const char *filename2,
                cl_uint bitWidth);
bool run(DeviceInfo *infoDeviceOcl, filters *paramFF, cl_uint bitWidth, cl_uint runCombinedKernel, cl_uint dataTransfer);
bool init(DeviceInfo *infoDeviceOcl, filters *paramFF,
                const char *inputImage, cl_int filterSize,
                cl_uint bitWidth, cl_uint deviceNum, cl_int useLds, cl_int zeroCopy, cl_int useIntrinsics);

/**
 *******************************************************************************
 *  @fn     usage
 *  @brief  This function prints the program usage
 *
 *  @param[in] prog : name of the binary to print in the message
 *
 *  @return void
 *******************************************************************************
 */
void usage(const char *prog)
{
    printf("Usage: %s \n\t[-i (input image path)]", prog);
    printf("\n\t[-combinedKernel (0 | 1)] \n\t[-zeroCopy (0 | 1)] //0 (default) - Device buffer, 1 - zero copy buffer\n\t[-filtSize (filterSize 3 | 5)]\n\t[-useLds (0 | 1)]");                    
    printf("\n\t[-reduceOverhead (0 | 1)]\n\t[-useIntrinsics (0 | 1)]\n\t[-h (help)]\n\n");                    
    printf("Example: To run 5X5 filter on 8 bit/channel input image, run");
    printf("\n\t %s -i Nature_2048x1024.bmp -filtSize 5 -useLds 0 -zeroCopy 0 -reduceOverhead 1\n", prog);    
}

int main(int argc, char **argv)
{

    DeviceInfo infoDeviceOcl;
    filters paramFF;

    cl_uint status = 0;
    cl_int loopCnt = LOOP_COUNT;
    cl_uint bitWidth = DEFAULT_BITWIDTH;
    cl_uint deviceNum = 0;
    cl_int useLds = 0;
    cl_uint zeroCopy = 0;
    cl_uint optimizedPipeline = 1;
    cl_uint verify = 1;
    cl_uint useIntrinsics = 1;
    cl_uint runCombinedKernel = 0;
    cl_uint dataTransfer = 1;
    
    const char *inputImage = DEFAULT_INPUT_IMAGE;
    const char *gaussianOutputImage = DEFAULT_OPENCL_OUTPUT_IMAGE;
    const char *enhancedOutputImage = DEFAULT_ENH_OUTPUT_IMAGE;

    cl_int filterSize = DEFAULT_FILTER_SIZE;

    /***************************************************************************
     * Processing the command line arguments                                   *
     **************************************************************************/

    int tmpArgc = argc;
    char **tmpArgv = argv;

    while (tmpArgv[1] && tmpArgv[1][0] == '-')
    {
        if (strcmp(tmpArgv[1], "-i") == 0)
        {
            tmpArgv++;
            tmpArgc--;
            inputImage = tmpArgv[1];
        }
        else if (strncmp(tmpArgv[1], "-filtSize", 9) == 0)
        {
            tmpArgv++;
            tmpArgc--;
            filterSize = atoi(tmpArgv[1]);
            if (!(filterSize == 3 || filterSize == 5 || filterSize == 7 || filterSize == 9))
            {
                printf("Only filter sizes 3, 5, 7 and 9 are supported.\n");
                exit(1);
            }
        }
        else if (strncmp(tmpArgv[1], "-bitWidth", 9) == 0)
        {
            tmpArgv++;
            tmpArgc--;
            bitWidth = atoi(tmpArgv[1]);
            if (!(bitWidth == 8 || bitWidth == 16))
            {
                printf("Only 8 and 16 are supported bitWidth.\n");
                exit(1);
            }
        }
        else if (strncmp(tmpArgv[1], "-useLds", 8) == 0)
        {
            tmpArgv++;
            tmpArgc--;
            useLds = atoi(tmpArgv[1]);
        }
        else if (strncmp(tmpArgv[1], "-device", 7) == 0)
        {
            tmpArgv++;
            tmpArgc--;
            deviceNum = atoi(tmpArgv[1]);
        }
        else if (strncmp(tmpArgv[1], "-verify", 7) == 0)
        {
            tmpArgv++;
            tmpArgc--;
            verify = atoi(tmpArgv[1]);
        }
        else if (strncmp(tmpArgv[1], "-zeroCopy", 9) == 0)
        {
            tmpArgv++;
            tmpArgc--;
            zeroCopy = atoi(tmpArgv[1]);
        }
        else if (strncmp(tmpArgv[1], "-reduceOverhead", 15) == 0)
        {
            tmpArgv++;
            tmpArgc--;
            optimizedPipeline = atoi(tmpArgv[1]);
        }
        else if (strncmp(tmpArgv[1], "-useIntrinsics", 14) == 0)
        {
            tmpArgv++;
            tmpArgc--;
            useIntrinsics = atoi(tmpArgv[1]);
        }
        else if (strncmp(tmpArgv[1], "-combinedKernel", 15) == 0)
        {
            tmpArgv++;
            tmpArgc--;
            runCombinedKernel = atoi(tmpArgv[1]);
        }
        else if (strncmp(tmpArgv[1], "-h", 2) == 0)
        {
            usage(argv[0]);
            exit(1);
        }
        else
        {
            printf("Illegal option %s ignored\n", tmpArgv[1]);
            usage(argv[0]);
            exit(1);
        }
        tmpArgv++;
        tmpArgc--;
    }

    if (tmpArgc > 1)
    {
        usage(argv[0]);
        exit(1);
    }

    if (zeroCopy)
    {
        dataTransfer = 0;
    }
    
    /***************************************************************************
     * Read input, initialize OpenCL runtime, create memory and OpenCL kernels
     **************************************************************************/
    if (init(&infoDeviceOcl, &paramFF, inputImage, filterSize,
                    bitWidth, deviceNum, useLds, zeroCopy, useIntrinsics) != true)
    {
        printf("Error in init.\n");
        return -1;
    }


    
    /***************************************************************************
    * Print information about input and other improtant details              
    **************************************************************************/
    if (!runCombinedKernel)
        printf("Executing Gaussian filter and Enhance kernel one after other.");
    else
        printf("Executing one combined filter containing Gaussian and Enhance filters.");
    printf("\n\tFilter size: %dx%d\n\tInput Image: %d bit single channel\n\tInput Image resolution: %dx%d", 
                    filterSize, filterSize, bitWidth, paramFF.cols, paramFF.rows);
     
    if (zeroCopy)
        printf("\n\tKernels are using zero copy buffers.");
    else 
        printf("\n\tKernels are using device buffers.");

    if (useLds)
        printf("\n\tKernels are using Lds memory for input.");
    else 
        printf("\n\tKernels are not using Lds memory for input.");

    printf("\n\nRunning for %d iterations\n\n", loopCnt);

    /***************************************************************************
    * Warm-up run of OpenCL Gaussian filters                            
    **************************************************************************/
    if (run(&infoDeviceOcl, &paramFF, bitWidth, runCombinedKernel, dataTransfer) != true)
    {
        printf("Error in run.\n");
        return -1;
    }
    clFinish(infoDeviceOcl.mQueue);

    /*******************************************************************************
    * Get Performance data
    *******************************************************************************/
    timer t_timer;
    timerStart(&t_timer);

    for (int i = 0; i < loopCnt; i++)
    {
        if (run(&infoDeviceOcl, &paramFF, bitWidth, runCombinedKernel, dataTransfer) != true)
        {
            printf("Error in run.\n");
            return -1;
        }

        if (!optimizedPipeline)
        {
            clFinish(infoDeviceOcl.mQueue);
        }
    }
    clFinish(infoDeviceOcl.mQueue);

    double time_ms = timerCurrent(&t_timer);
    time_ms = 1000 * (time_ms / loopCnt);

    if (zeroCopy)
        printf("Average time taken per iteration using zero-copy buffer: %f msec\n", time_ms);
    else
        printf("Average time taken per iteration using device-memory with data-transfer: %f msec\n", time_ms);
    
    /*******************************************************************************
    * Get Performance data without data transfer in case of using device-memory
    *******************************************************************************/
    if (!zeroCopy)
    {
        
        timerStart(&t_timer);

        for (int i = 0; i < loopCnt; i++)
        {
            if (run(&infoDeviceOcl, &paramFF, bitWidth, runCombinedKernel, 0) != true)
            {
                printf("Error in run.\n");
                return -1;
            }

            if (!optimizedPipeline)
            {
                clFinish(infoDeviceOcl.mQueue);
            }
        }
        clFinish(infoDeviceOcl.mQueue);

        double time_ms = timerCurrent(&t_timer);
        time_ms = 1000 * (time_ms / loopCnt);

        printf("Average time taken per iteration using device-memory without any data-transfer: %f msec\n", time_ms);
    }


    /***************************************************************************
    * Save separable and non-separable filter output images                  
    **************************************************************************/
    if (!saveOutputs(&paramFF, gaussianOutputImage, enhancedOutputImage, bitWidth))
    {
        printf("Error in saveOutputs.\n");
        return -1;
    }
    
    /***************************************************************************
    * Destpry memory and cleanup OpenCL runtime                              
    **************************************************************************/
    destroyMemory(&paramFF, &infoDeviceOcl);
    
    return 0;
}

/**
 *******************************************************************************
 *  @fn     init
 *  @brief  This function parses creates the memory needed by the pipeline, 
 *          initializes OpenCL, builds kernel and sets the kernel arguments
 *
 *  @param[in/out] infoDeviceOcl : Structure which holds openCL related params
 *  @param[in/out] paramFF      : Structure holds all parameters required 
 *                                 by the sample
 *  @param[in] inputImage       : input imaage name
 *  @param[in] filterSize       : filter size (only 3 and 5 are currently supported)
 *  @param[in] bitWidth         : 8 bit or 16 bit input
 *  @param[in] deviceNum        : device on which to run OpenCL kernels
 *  @param[in] useLds           : Should the OpenCL kernel use LDS memory for input
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool init(DeviceInfo *infoDeviceOcl, filters *paramFF,
                const char *inputImage, cl_int filterSize, 
                cl_uint bitWidth, cl_uint deviceNum, cl_int useLds, cl_int zeroCopy, cl_int useIntrinsics)
{
    paramFF->filterSize = filterSize;
    
    /***************************************************************************
     * read the input image                                                   
     ***************************************************************************/
    if (readInput(paramFF, inputImage, bitWidth) == false)
    {
        printf("Error reading input.\n");
        return false;
    }

    /**************************************************************************
    * Initialize the openCL device and create context and command queue      
    ***************************************************************************/
    if (initOpenCl(infoDeviceOcl, deviceNum) == false)
    {
        printf("Error in initOpenCl.\n");
        return false;
    }

    /**************************************************************************
    * Create the memory needed by the pipeline                               
    ***************************************************************************/
    if (createMemory(paramFF, infoDeviceOcl, bitWidth, zeroCopy) == false)
    {
        printf("Error in createMemory.\n");
        return false;
    }

    /***************************************************************************
    * Build the Gaussin Filter OpenCL kernel                         
    ***************************************************************************/
    if (buildKernels(infoDeviceOcl->mCtx, infoDeviceOcl->mDevice,
                    &(paramFF->gaussianKernel), &(paramFF->enhancedKernel), &(paramFF->combinedKernel), 
                    filterSize, bitWidth, useLds, useIntrinsics)
                    == false)
    {
        printf("Error in buildGaussianFilterKernel.\n");
        return false;
    }

    /**************************************************************************
    * Sets the Gaussian Filter OpenCL kernel arguments                     
    **************************************************************************/
    if (setKernelArgs(paramFF->gaussianKernel,
                    paramFF->enhancedKernel,
                    paramFF->combinedKernel,
                    paramFF->input, paramFF->gaussianOutput, paramFF->enhancedOutput,
                    paramFF->gaussianFilter, paramFF->cols, 
                    paramFF->rows, paramFF->filterSize) == false)
    {
        printf("Error in setGaussianFilterKernelArgs.\n");
        return false;
    }
    
    return true;
}

/**
 *******************************************************************************
 *  @fn     run
 *  @brief  This function runs the entire pipeline
 *
 *  @param[in/out] infoDeviceOcl : Structure which holds openCL related params
 *  @param[in/out] paramFF      : Structure holds all parameters required
 *                                 by the sample
 *  @param[in] bitWidth         : 8 bit or 16 bit input
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool run(DeviceInfo *infoDeviceOcl, filters *paramFF, cl_uint bitWidth, cl_uint runCombinedKernel, cl_uint dataTransfer)
{
    cl_int status;

    /**************************************************************************
    * Transfer the data to device if zero-copy is not being used
    ***************************************************************************/
    if (dataTransfer)
    {
        /**************************************************************************
        * Send the input image data to the device
        ***************************************************************************/
        status = clEnqueueWriteBuffer(infoDeviceOcl->mQueue, paramFF->input,
                        CL_FALSE, 0, paramFF->paddedRows * paramFF->paddedCols * sizeof(cl_uchar)
                                        * (bitWidth / 8), paramFF->inputImg, 0,
                        NULL, NULL);
        CHECK_RESULT(status != CL_SUCCESS,
                        "Error in clEnqueueWriteBuffer. Status: %d\n", status);

        /**************************************************************************
        * Send the filters to the device
        ***************************************************************************/
        status = clEnqueueWriteBuffer(infoDeviceOcl->mQueue,
                        paramFF->gaussianFilter, CL_FALSE, 0, paramFF->filterSize
                                        * paramFF->filterSize * sizeof(cl_float),
                        paramFF->gaussianFilterCpu, 0, NULL, NULL);
        CHECK_RESULT(status != CL_SUCCESS,
                        "Error in clEnqueueWriteBuffer. Status: %d\n", status);
    }
    /**************************************************************************
     * Run the gaussianFilter OpenCL kernel.
     ***************************************************************************/
    runKernels(infoDeviceOcl->mQueue, paramFF->gaussianKernel, paramFF->enhancedKernel, 
        paramFF->combinedKernel, runCombinedKernel, paramFF->cols, paramFF->rows);

        /**************************************************************************
    * Transfer the data to host if zero-copy is not being used
    ***************************************************************************/
    if (dataTransfer) 
    {
        /**************************************************************************
         * Get the results back to host
         ***************************************************************************/
        status = clEnqueueReadBuffer(infoDeviceOcl->mQueue, paramFF->gaussianOutput,
                        CL_FALSE, 0, paramFF->cols * paramFF->rows
                                        * sizeof(cl_uchar) * (bitWidth / 8),
                        paramFF->gaussianOutputImg, 0, NULL, NULL);
        CHECK_RESULT(status != CL_SUCCESS,
                        "Error in clEnqueueReadBuffer. Status: %d\n", status);

        status = clEnqueueReadBuffer(infoDeviceOcl->mQueue, paramFF->enhancedOutput,
                        CL_FALSE, 0, paramFF->cols * paramFF->rows
                                        * sizeof(cl_uchar) * (bitWidth / 8),
                        paramFF->enhancedOutputImg, 0, NULL, NULL);
        CHECK_RESULT(status != CL_SUCCESS,
                        "Error in clEnqueueReadBuffer. Status: %d\n", status);
    }
    
        return true;
}

/**
 *******************************************************************************
 *  @fn     readInput
 *  @brief  This functons reads an input image content and also pads the filter
 *          based on input image size
 *
 *  @param[in] paramFF     : Pointer to structure
 *  @param[in] inputImage : input image file name
 *  @param[in] bitWidth         : 8 bit or 16 bit input
 *
 *  @return bool : true if successful; otherwise false.
 *******************************************************************************
 */
bool readInput(filters *paramFF, const char *inputImage, cl_uint bitWidth)
{
    uchar4* pixelData;

    // load input bitmap image
    paramFF->inputBitmap.load(inputImage);

    // error if image did not load
    if(!paramFF->inputBitmap.isLoaded())
    {
        printf("Failed to load input image!");
        return false;
    }

    // get width and height of input image
    paramFF->rows = paramFF->inputBitmap.getHeight();
    paramFF->cols = paramFF->inputBitmap.getWidth();

    paramFF->paddedRows = paramFF->rows + paramFF->filterSize - 1;
    paramFF->paddedCols = paramFF->cols + paramFF->filterSize - 1;

    cl_int filterRadius = paramFF->filterSize / 2;

    paramFF->inputImg = (cl_uchar *) malloc(paramFF->paddedCols
                    * paramFF->paddedRows * sizeof(cl_uchar) * bitWidth / 8);
    memset(paramFF->inputImg, 0, paramFF->paddedCols * paramFF->paddedRows
                    * sizeof(cl_uchar) * bitWidth / 8);


    // get the pointer to pixel data
    pixelData = paramFF->inputBitmap.getPixels();
    if(pixelData == NULL)
    {
        printf("Failed to read pixel Data!");
        return false;
    }

    /**************************************************************************
     * Using only r channel of the image. 
     * Pad the input image.
     **************************************************************************/
    if (bitWidth == 16)
    {
        for (cl_uint i = filterRadius; i < paramFF->rows + filterRadius; i++)
        {
            for (cl_uint j = filterRadius; j < paramFF->cols + filterRadius; j++)
            {
                ((cl_ushort *) paramFF->inputImg)[i * paramFF->paddedCols + j]
                                = (cl_ushort)(pixelData[(i - filterRadius) * paramFF->cols + (j - filterRadius)].x);
            }
        }

    }
    else if (bitWidth == 8)
    {
        for (cl_uint i = filterRadius; i < paramFF->rows + filterRadius; i++)
        {
            for (cl_uint j = filterRadius; j < paramFF->cols + filterRadius; j++)
            {
                ((cl_uchar *) paramFF->inputImg)[i * paramFF->paddedCols + j]
                                = (cl_uchar)(pixelData[(i - filterRadius) * paramFF->cols + (j - filterRadius)].x);
            }
        }
    }
    else
    {
        CHECK_RESULT(false, "Un-supported bitWidth, only 8 and 16 bits are supported");
    }

    /***********************************************************************
     * get filter 
     ***********************************************************************/
    if (paramFF->filterSize == 3)
    {
        paramFF->gaussianFilterCpu = gaussianFilterCoeff_3x3;
    }
    else if (paramFF->filterSize == 5)
    {
        paramFF->gaussianFilterCpu = gaussianFilterCoeff_5x5;
    }
    else if (paramFF->filterSize == 7)
    {
        paramFF->gaussianFilterCpu = gaussianFilterCoeff_7x7;
    }
    else
    {
        paramFF->gaussianFilterCpu = gaussianFilterCoeff_9x9;
    }
    
    return true;
}

/**
 *******************************************************************************
 *  @fn     saveOutput
 *  @brief  This functons crops the output image to the size of input image and 
 *          saves it in the given format. Format is identified based on image name
 *
 *  @param[in] paramFF     : Pointer to structure
 *  @param[in] gaussianOutputImage  : output file name
 *  @param[in] sepOutputImage    : output file name
 *  @param[in] bitWidth         : 8 bit or 16 bit input
 *
 *  @return void
 *******************************************************************************
 */
bool saveOutputs(filters *paramFF, const char *gaussianOutputImage,
                const char *enhancedOutputImage, cl_uint bitWidth)
{
    uchar4 *data = (uchar4 *)malloc(paramFF->cols * paramFF->rows * sizeof(uchar4));
    if (!data) 
    {
        printf("Error mallocing.");
        return false;
    }

    //Save gaussian filter output
    memset(data, 0, paramFF->rows * paramFF->cols * sizeof(uchar4));
    if (bitWidth == 8)
    {      
        for (cl_uint i = 0; i < paramFF->rows; i++)
        {
            for (cl_uint j = 0; j < paramFF->cols; j++)
            {
                data[i * paramFF->cols + j].x = (cl_uchar)paramFF->gaussianOutputImg[i * paramFF->cols + j];
                data[i * paramFF->cols + j].y = (cl_uchar)paramFF->gaussianOutputImg[i * paramFF->cols + j];
                data[i * paramFF->cols + j].z = (cl_uchar)paramFF->gaussianOutputImg[i * paramFF->cols + j];
            }
        }
    }
    else
    {
        for (cl_uint i = 0; i < paramFF->rows; i++)
        {
            for (cl_uint j = 0; j < paramFF->cols; j++)
            {
                data[i * paramFF->cols + j].x = (cl_uchar)((cl_ushort *)paramFF->gaussianOutputImg)[i * paramFF->cols + j];
                data[i * paramFF->cols + j].y = (cl_uchar)((cl_ushort *)paramFF->gaussianOutputImg)[i * paramFF->cols + j];
                data[i * paramFF->cols + j].z = (cl_uchar)((cl_ushort *)paramFF->gaussianOutputImg)[i * paramFF->cols + j];
            }
        }
    }
    // write the output bmp file
    paramFF->inputBitmap.write(gaussianOutputImage, paramFF->cols, paramFF->rows, (unsigned int *)data);

    //Save enhanced image output
    memset(data, 0, paramFF->rows * paramFF->cols * sizeof(uchar4));
    if (bitWidth == 8)
    {      
        for (cl_uint i = 0; i < paramFF->rows; i++)
        {
            for (cl_uint j = 0; j < paramFF->cols; j++)
            {
                data[i * paramFF->cols + j].x = (cl_uchar)paramFF->enhancedOutputImg[i * paramFF->cols + j];
                data[i * paramFF->cols + j].y = (cl_uchar)paramFF->enhancedOutputImg[i * paramFF->cols + j];
                data[i * paramFF->cols + j].z = (cl_uchar)paramFF->enhancedOutputImg[i * paramFF->cols + j];
            }
        }
    }
    else
    {
        for (cl_uint i = 0; i < paramFF->rows; i++)
        {
            for (cl_uint j = 0; j < paramFF->cols; j++)
            {
                data[i * paramFF->cols + j].x = (cl_uchar)((cl_ushort *)paramFF->enhancedOutputImg)[i * paramFF->cols + j];
                data[i * paramFF->cols + j].y = (cl_uchar)((cl_ushort *)paramFF->enhancedOutputImg)[i * paramFF->cols + j];
                data[i * paramFF->cols + j].z = (cl_uchar)((cl_ushort *)paramFF->enhancedOutputImg)[i * paramFF->cols + j];
            }
        }
    }
    // write the output bmp file
    paramFF->inputBitmap.write(enhancedOutputImage, paramFF->cols, paramFF->rows, (unsigned int *)data);

    free(data);

    printf("\nGaussian Filter output written to %s\n", gaussianOutputImage);
    printf("Enhanced Filter output written to %s\n\n", enhancedOutputImage);
    
    return true;
}

/**
 *******************************************************************************
 *  @fn     createMemory
 *  @brief  This function creates memory required by the pipeline 
 *
 *  @param[in/out] paramFF  : pointer to filters structure
 *  @param[in] infoDeviceOcl   : pointer to the structure containing opencl 
 *                               device information
 *  @param[in] bitWidth         : 8 bit or 16 bit input
 *
 *  @return bool : true if successful; otherwise false.
 ******************************************************************************
 */
bool createMemory(filters* paramFF, DeviceInfo *infoDeviceOcl,
                cl_uint bitWidth, cl_int zeroCopy)
{
    cl_int err = 0;

    int paddedRows = paramFF->paddedRows;
    int paddedCols = paramFF->paddedCols;

    paramFF->gaussianOutputImg = (cl_uchar *) malloc(paramFF->rows * paramFF->cols
                    * sizeof(cl_uchar) * (bitWidth / 8));
    CHECK_RESULT(paramFF->gaussianOutputImg == NULL, "Malloc failed.\n");

    paramFF->enhancedOutputImg = (cl_uchar *) malloc(paramFF->rows * paramFF->cols
                    * sizeof(cl_uchar) * (bitWidth / 8));
    CHECK_RESULT(paramFF->enhancedOutputImg == NULL, "Malloc failed.\n");

    if (zeroCopy)
    {
        paramFF->input = clCreateBuffer(infoDeviceOcl->mCtx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                            paddedRows * paddedCols * sizeof(cl_uchar) * (bitWidth / 8), 
                            paramFF->inputImg, &err);
        CHECK_RESULT(err != CL_SUCCESS, "clCreateBuffer failed with %d\n", err);

        paramFF->gaussianFilter = clCreateBuffer(infoDeviceOcl->mCtx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
            paramFF->filterSize * paramFF->filterSize * sizeof(cl_float), paramFF->gaussianFilterCpu, &err);
        CHECK_RESULT(err != CL_SUCCESS, "clCreateBuffer failed with %d\n", err);

        paramFF->gaussianOutput = clCreateBuffer(infoDeviceOcl->mCtx, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                        paramFF->rows * paramFF->cols * sizeof(cl_uchar)
                                        * (bitWidth / 8), paramFF->gaussianOutputImg, &err);
        CHECK_RESULT(err != CL_SUCCESS, "clCreateBuffer failed with %d\n", err);

        paramFF->enhancedOutput = clCreateBuffer(infoDeviceOcl->mCtx, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                        paramFF->rows * paramFF->cols * sizeof(cl_uchar)
                                        * (bitWidth / 8), paramFF->enhancedOutputImg, &err);
        CHECK_RESULT(err != CL_SUCCESS, "clCreateBuffer failed with %d\n", err);
    }
    else
    {
        paramFF->input = clCreateBuffer(infoDeviceOcl->mCtx, CL_MEM_READ_ONLY,
                            paddedRows * paddedCols * sizeof(cl_uchar) * (bitWidth / 8), 
                            NULL, &err);
        CHECK_RESULT(err != CL_SUCCESS, "clCreateBuffer failed with %d\n", err);

        paramFF->gaussianFilter = clCreateBuffer(infoDeviceOcl->mCtx, CL_MEM_READ_ONLY,
                        paramFF->filterSize * paramFF->filterSize * sizeof(cl_float), NULL, &err);
        CHECK_RESULT(err != CL_SUCCESS, "clCreateBuffer failed with %d\n", err);

        paramFF->gaussianOutput = clCreateBuffer(infoDeviceOcl->mCtx, CL_MEM_WRITE_ONLY,
                        paramFF->rows * paramFF->cols * sizeof(cl_uchar)
                                        * (bitWidth / 8), NULL, &err);
        CHECK_RESULT(err != CL_SUCCESS, "clCreateBuffer failed with %d\n", err);

        paramFF->enhancedOutput = clCreateBuffer(infoDeviceOcl->mCtx, CL_MEM_WRITE_ONLY,
                        paramFF->rows * paramFF->cols * sizeof(cl_uchar)
                                        * (bitWidth / 8), NULL, &err);
        CHECK_RESULT(err != CL_SUCCESS, "clCreateBuffer failed with %d\n", err);
    }

    return true;
}

/**
 *******************************************************************************
 *  @fn     destroyMemory
 *  @brief  This function destroys the memory created by the pipeline
 *
 *  @param[in/out] paramFF  : pointer to structure
 *  @param[in] infoDeviceOcl   : pointer to the structure containing opencl
 *                               device information
 *
 *  @return void
 *******************************************************************************
 */
void destroyMemory(filters* paramFF, DeviceInfo *infoDeviceOcl)
{
    free(paramFF->inputImg);
    free(paramFF->gaussianOutputImg);
    free(paramFF->enhancedOutputImg);
    
    clReleaseMemObject(paramFF->input);
    clReleaseMemObject(paramFF->gaussianFilter);
    clReleaseMemObject(paramFF->gaussianOutput);
    clReleaseMemObject(paramFF->enhancedOutput);
    clReleaseKernel(paramFF->gaussianKernel);
}