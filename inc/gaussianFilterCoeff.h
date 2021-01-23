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
 * @file <gaussianFilterCoeff.h>
 *
 * @brief Contains the filter coefficients.
 *
 ********************************************************************************
 */
#ifndef __GAUSSIAN_FILTERCOEFF__H
#define __GAUSSIAN_FILTERCOEFF__H


/****************************************************************************************
 * Gaussian 3x3, 5x5 filter coefficients (for sigma/spread = 0.85 and 1.0 respectively) *
 ****************************************************************************************/
#define GAUSSIAN_FILT_3_1    0.062500f, 0.125000f, 0.062500f
#define GAUSSIAN_FILT_3_2    0.125000f, 0.250000f, 0.125000f

#define GAUSSIAN_FILT_5_1    0.003502f, 0.012259f, 0.021015f, 0.012259f, 0.003502f
#define GAUSSIAN_FILT_5_2    0.012259f, 0.054290f, 0.091068f, 0.054290f, 0.012259f
#define GAUSSIAN_FILT_5_3    0.021015f, 0.091068f, 0.222416f, 0.091068f, 0.021015f

#define GAUSSIAN_FILT_7_1    0.023511f, 0.023511f, 0.023511f, 0.023512f, 0.023511f, 0.023511f, 0.023511f
#define GAUSSIAN_FILT_7_2    0.023511f, 0.023519f, 0.025592f, 0.035470f, 0.025592f, 0.023519f, 0.023511f
#define GAUSSIAN_FILT_7_3    0.023511f, 0.025592f, 0.303353f, 0.712768f, 0.303353f, 0.025592f, 0.023511f
#define GAUSSIAN_FILT_7_4    0.023512f, 0.035470f, 0.712768f,-5.069447f, 0.712768f, 0.035470f, 0.023512f

#define GAUSSIAN_FILT_9_1    0.014223f, 0.014223f, 0.014223f, 0.014223f, 0.014223f, 0.014223f, 0.014223f, 0.014223f, 0.014223f
#define GAUSSIAN_FILT_9_2    0.014223f, 0.014223f, 0.014223f, 0.014223f, 0.014224f, 0.014223f, 0.014223f, 0.014223f, 0.014223f
#define GAUSSIAN_FILT_9_3    0.014223f, 0.014223f, 0.014231f, 0.016304f, 0.026182f, 0.016304f, 0.014231f, 0.014223f, 0.014223f
#define GAUSSIAN_FILT_9_4    0.014223f, 0.014223f, 0.016304f, 0.294065f, 0.703480f, 0.294065f, 0.016304f, 0.014223f, 0.014223f
#define GAUSSIAN_FILT_9_5    0.014223f, 0.014224f, 0.026182f, 0.703480f,-5.078736f, 0.703480f, 0.026182f, 0.014224f, 0.014223f

float gaussianFilterCoeff_3x3[] = { 
    GAUSSIAN_FILT_3_1,
    GAUSSIAN_FILT_3_2,
    GAUSSIAN_FILT_3_1
};
    
float gaussianFilterCoeff_5x5[] = { 
    GAUSSIAN_FILT_5_1,
    GAUSSIAN_FILT_5_2,  
    GAUSSIAN_FILT_5_3,  
    GAUSSIAN_FILT_5_2,  
    GAUSSIAN_FILT_5_1
};


float gaussianFilterCoeff_7x7[] = { 
    GAUSSIAN_FILT_7_1,
    GAUSSIAN_FILT_7_2,  
    GAUSSIAN_FILT_7_3,  
    GAUSSIAN_FILT_7_4,  
    GAUSSIAN_FILT_7_3,  
    GAUSSIAN_FILT_7_2,  
    GAUSSIAN_FILT_7_1};

float gaussianFilterCoeff_9x9[] = { 
    GAUSSIAN_FILT_9_1,
    GAUSSIAN_FILT_9_2,  
    GAUSSIAN_FILT_9_3,  
    GAUSSIAN_FILT_9_4,  
    GAUSSIAN_FILT_9_5,  
    GAUSSIAN_FILT_9_4,  
    GAUSSIAN_FILT_9_3,  
    GAUSSIAN_FILT_9_2,  
    GAUSSIAN_FILT_9_1};

#endif                                                                                   
