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

#if PIX_WIDTH == 8
#define T1 uchar
#define ROUND(x) ((x > 0) ? convert_uchar_rte(x) : 0)
#else
#define T1 ushort
#define ROUND(x) ((x > 0) ? convert_ushort_rte(x) : 0)
#endif

// gaussianFilterKernel implements a non-separable convolution filter.
__kernel 
__attribute__((reqd_work_group_size(LOCAL_XRES, LOCAL_YRES, 1)))
void gaussianFilterKernel(
    __global T1 *pIBuf,      // 0: Input buffer of type T1
    __global T1 *pFilterOBuf,      // 1: Output buffer of type T1
    uint nWidth,             // 2: Image width in pixels
    uint nHeight,            // 3: Image height in pixels
    uint nExWidth,           // 4: Padded image width in pixels
    __constant float *pFilter// 5: Filter coefficients of type float
    )
{
    uint ix = get_global_id(0);
    uint iy = get_global_id(1);

    // Process only if pIBuf[ix,iy] is within valid bounds.
    if (ix >= nWidth || iy >= nHeight) return;

    uint Pos = iy * nExWidth + ix;
    T1 pix_val;
    uint filter_offset = 0;

    float nSum = 0.0f;
    float tVal = 0.0f;
    
    /***************************************************************************************
    * If using LDS, get the data to local memory. Else, get the global memory indices ready 
    ***************************************************************************************/

#if USE_LDS == 1

    __local T1 local_input[(LOCAL_XRES + TAP_SIZE - 1) * (LOCAL_YRES + TAP_SIZE - 1)];

    int lid_x = get_local_id(0);
    int lid_y = get_local_id(1);
    
    int tile_xres = (LOCAL_XRES + TAP_SIZE - 1);
    int tile_yres = (LOCAL_YRES + TAP_SIZE - 1);

    int start_col = get_group_id(0) * LOCAL_XRES;// image is padded
    int start_row = get_group_id(1) * LOCAL_YRES; 
    Pos = (lid_y * tile_xres) + lid_x;
    
    /* Get input for a tile in local memory */
    //First 16X16
    local_input[lid_y * (LOCAL_XRES + TAP_SIZE - 1) + lid_x] = pIBuf[(start_row + lid_y) * nExWidth + (start_col + lid_x)];

    //16X8 at the y-end of the tile
    if (lid_y < (TAP_SIZE - 1))
    {
        local_input[(LOCAL_YRES + lid_y) * (LOCAL_XRES + TAP_SIZE - 1) + lid_x] = pIBuf[(start_row + lid_y + LOCAL_YRES) * nExWidth + (start_col + lid_x)];
    }

    //8X16 at the x-end of the tile
    if (lid_x < (TAP_SIZE - 1))
    {
        local_input[lid_y * (LOCAL_XRES + TAP_SIZE - 1) + (lid_x + LOCAL_XRES)] = pIBuf[(start_row + lid_y) * nExWidth + (start_col + lid_x + LOCAL_XRES)];
    }

    //Right-most 8X8
    if (lid_x < (TAP_SIZE - 1) && lid_y < (TAP_SIZE - 1))
    {
        local_input[(LOCAL_YRES + lid_y) * (LOCAL_XRES + TAP_SIZE - 1) + (lid_x + LOCAL_XRES)] = pIBuf[(start_row + lid_y + LOCAL_YRES) * nExWidth + (start_col + lid_x + LOCAL_XRES)];
    }

    barrier(CLK_LOCAL_MEM_FENCE);
#endif

    for (uint i=0; i<TAP_SIZE; i++)
    {
        #pragma unroll TAP_SIZE
        for (uint j=0; j<TAP_SIZE; j++,Pos++)
        {
#if USE_LDS == 1 
            pix_val = local_input[Pos];
#else
            pix_val = pIBuf[Pos];
#endif
            filter_offset = i*TAP_SIZE+j;
#if USE_INTRINSICS == 1
            nSum = mad(convert_float(pix_val), pFilter[filter_offset++], nSum);                        
#else            
            tVal = pix_val * pFilter[filter_offset++];
            nSum = tVal + nSum;
#endif
        }
#if USE_LDS == 1
        Pos += tile_xres - TAP_SIZE;
#else
        Pos += nExWidth - TAP_SIZE;
#endif
    }

    //Save output
    pFilterOBuf[iy * nWidth + ix] = ROUND(nSum);   
}

__kernel 
__attribute__((reqd_work_group_size(LOCAL_XRES, LOCAL_YRES, 1)))
void enhanceFilterKernel(
    __global T1 *pIBuf,      // 0: Input buffer of type T1
    __global T1 *pGaussianFilterBuf,      // 1: Output buffer of type T1
    __global T1 *pEnhanceOBuf,      // 1: Output buffer of type T1
    uint nWidth,             // 2: Image width in pixels
    uint nHeight,            // 3: Image height in pixels
    uint nExWidth            // 4: Padded image width in pixels   
    )
{
    uint ix = get_global_id(0);
    uint iy = get_global_id(1);

    // Process only if pIBuf[ix,iy] is within valid bounds.
    if (ix >= nWidth || iy >= nHeight) return;

    T1 input_val = pIBuf[(iy + (TAP_SIZE/2)) * nExWidth + (ix + (TAP_SIZE/2))];
    T1 filtered_val = pGaussianFilterBuf[iy * nWidth + ix];;
    
    //Enhance image
    int enhanced_val = input_val + (input_val - filtered_val);
    
#if PIX_WIDTH == 8
    enhanced_val = enhanced_val > 255 ? 255 : enhanced_val;
    enhanced_val = enhanced_val < 0   ?   0 : enhanced_val;
#else    
    enhanced_val = enhanced_val > 65535 ? 65535 : enhanced_val;
    enhanced_val = enhanced_val <     0 ?     0 : enhanced_val;
#endif

    pEnhanceOBuf[iy * nWidth + ix] = enhanced_val;

}



__kernel 
__attribute__((reqd_work_group_size(LOCAL_XRES, LOCAL_YRES, 1)))
void combinedFilterKernel(
    __global T1 *pIBuf,      // 0: Input buffer of type T1
    __global T1 *pFilterOBuf,      // 1: Output buffer of type T1
    __global T1 *pEnhanceOBuf,      // 1: Output buffer of type T1
    uint nWidth,             // 2: Image width in pixels
    uint nHeight,            // 3: Image height in pixels
    uint nExWidth,           // 4: Padded image width in pixels
    __constant float *pFilter// 5: Filter coefficients of type float
    )
{
    uint ix = get_global_id(0);
    uint iy = get_global_id(1);

    // Process only if pIBuf[ix,iy] is within valid bounds.
    if (ix >= nWidth || iy >= nHeight) return;

    uint Pos = iy * nExWidth + ix;
    T1 pix_val;
    uint filter_offset = 0;

    float nSum = 0.0f;
    float tVal = 0.0f;

    T1 input_val = pIBuf[(iy + (TAP_SIZE/2)) * nExWidth + (ix + (TAP_SIZE/2))];
    T1 filtered_val;
    
    /***************************************************************************************
    * If using LDS, get the data to local memory. Else, get the global memory indices ready 
    ***************************************************************************************/
#if USE_LDS == 1

    __local T1 local_input[(LOCAL_XRES + TAP_SIZE - 1) * (LOCAL_YRES + TAP_SIZE - 1)];

    int lid_x = get_local_id(0);
    int lid_y = get_local_id(1);
    
    int tile_xres = (LOCAL_XRES + TAP_SIZE - 1);
    int tile_yres = (LOCAL_YRES + TAP_SIZE - 1);

    int start_col = get_group_id(0) * LOCAL_XRES;// image is padded
    int start_row = get_group_id(1) * LOCAL_YRES; 
    Pos = (lid_y * tile_xres) + lid_x;
    
    /* Get input for a tile in local memory */
    //First 16X16
    local_input[lid_y * (LOCAL_XRES + TAP_SIZE - 1) + lid_x] = pIBuf[(start_row + lid_y) * nExWidth + (start_col + lid_x)];

    //16X8 at the y-end of the tile
    if (lid_y < (TAP_SIZE - 1))
    {
        local_input[(LOCAL_YRES + lid_y) * (LOCAL_XRES + TAP_SIZE - 1) + lid_x] = pIBuf[(start_row + lid_y + LOCAL_YRES) * nExWidth + (start_col + lid_x)];
    }

    //8X16 at the x-end of the tile
    if (lid_x < (TAP_SIZE - 1))
    {
        local_input[lid_y * (LOCAL_XRES + TAP_SIZE - 1) + (lid_x + LOCAL_XRES)] = pIBuf[(start_row + lid_y) * nExWidth + (start_col + lid_x + LOCAL_XRES)];
    }

    //Right-most 8X8
    if (lid_x < (TAP_SIZE - 1) && lid_y < (TAP_SIZE - 1))
    {
        local_input[(LOCAL_YRES + lid_y) * (LOCAL_XRES + TAP_SIZE - 1) + (lid_x + LOCAL_XRES)] = pIBuf[(start_row + lid_y + LOCAL_YRES) * nExWidth + (start_col + lid_x + LOCAL_XRES)];
    }

    barrier(CLK_LOCAL_MEM_FENCE);
#endif

    for (uint i=0; i<TAP_SIZE; i++)
    {
        #pragma unroll TAP_SIZE
        for (uint j=0; j<TAP_SIZE; j++,Pos++)
        {
#if USE_LDS == 1 
            pix_val = local_input[Pos];
#else
            pix_val = pIBuf[Pos];
#endif
            filter_offset = i*TAP_SIZE+j;
#if USE_INTRINSICS == 1
            nSum = mad(convert_float(pix_val), pFilter[filter_offset++], nSum);                        
#else            
            tVal = pix_val * pFilter[filter_offset++];
            nSum = tVal + nSum;
#endif
        }
#if USE_LDS == 1
        Pos += tile_xres - TAP_SIZE;
#else
        Pos += nExWidth - TAP_SIZE;
#endif
    }

    //Save output
    //pOBuf[iy * nWidth + ix] = ROUND(nSum);    
    filtered_val = ROUND(nSum);
    
    //Enhance image
    int enhanced_val = input_val + (input_val - filtered_val);
    
#if PIX_WIDTH == 8
    enhanced_val = enhanced_val > 255 ? 255 : enhanced_val;
    enhanced_val = enhanced_val < 0   ?   0 : enhanced_val;
#else    
    enhanced_val = enhanced_val > 65535 ? 65535 : enhanced_val;
    enhanced_val = enhanced_val <     0 ?     0 : enhanced_val;
#endif

    pFilterOBuf[iy * nWidth + ix] = filtered_val;
    pEnhanceOBuf[iy * nWidth + ix] = enhanced_val;

}
