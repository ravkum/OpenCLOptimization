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
#ifndef __UTILS__H
#define __UTILS__H
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>
#include "CL/cl.h"
#include "macros.h"

#ifdef _WIN32
#include <windows.h>
#elif defined __MACH__
#include <mach/mach_time.h>
#else
#include <sys/time.h>
#include <linux/limits.h>
#include <unistd.h>
#endif

/******************************************************************************
* Timer structure                                                             *
******************************************************************************/
typedef struct timer
{
#ifdef _WIN32
    LARGE_INTEGER start;
#else
    long long start;
#endif
} timer;

/******************************************************************************
* Structure to hold opencl device information                                 *
******************************************************************************/
typedef struct DeviceInfo
{
    cl_platform_id mPlatform;
    cl_device_id mDevice;
    cl_context mCtx;
    cl_command_queue mQueue;
} DeviceInfo;

void timerStart(timer* mytimer);
double timerCurrent(timer* mytimer);
bool initOpenCl(DeviceInfo *infoDeviceOcl, cl_uint deviceNum);

#endif