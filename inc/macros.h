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

#ifndef __MACROS__H
#define __MACROS__H
#include <stdio.h>
#include <stdlib.h>

#ifdef USE_CL_EVENTS
#define CL_EVENT(x)	x
#else
#define CL_EVENT(x)	NULL
#endif

/******************************************************************************
* Error handling macro.                                                       *
******************************************************************************/
#ifdef _MSC_VER
#define snprintf sprintf_s
#endif
#define CHECK_RESULT(test, msg,...)                                     \
    if ((test))                                                         \
    {                                                                   \
        char *buf = (char*)malloc(4096);                                \
        int rc = snprintf(buf, 4096, msg,  ##__VA_ARGS__);              \
        printf("%s:%d - %s\n", __FILE__, __LINE__, buf);                \
        free(buf);                                                      \
        return false;                                                   \
    }

/**
*******************************************************************************
*  @fn     convertToString
*  @brief  convert the kernel file into a string
*
*  @param[in] filename          : Kernel file name to read
*  @param[out] kernelSource     : Buffer containing the contents of kernel file
*  @param[out] kernelSourceSize : Size of the buffer containing the source
*
*  @return int : 0 if successful; otherwise 1.
*******************************************************************************
*/
inline int convertToString(const char *filename, char **kernelSource, size_t *kernelSourceSize)
{
    FILE *fp = NULL;
    *kernelSourceSize = 0;
    *kernelSource = NULL;
    fp = fopen(filename, "r");
    if (fp != NULL)
    {
        fseek(fp, 0L, SEEK_END);
        *kernelSourceSize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        *kernelSource = (char*)malloc(*kernelSourceSize);
        if (*kernelSource != NULL)
            *kernelSourceSize = fread(*kernelSource, 1, *kernelSourceSize, fp);
        fclose(fp);
        return 0;
    }
    return 1;
}
#endif
