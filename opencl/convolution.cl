#define DATA_TYPE unsigned char

float rgb2Y(DATA_TYPE r, DATA_TYPE g, DATA_TYPE b)
{
    float Y = (float)(0.299)*(float)r + (float)(0.587)*(float)g + (float)(0.114)*(float)b;
    return Y;
}

#define GHOST_THRESHOLD -100
bool isGhost (
    int x, int y,
    __constant DATA_TYPE *depth,
    int depth_step, int depth_channels,
    int width, int height)
{
    if (x - 1 < 0 || x + 1 >= width) return 0;
    if (y - 1 < 0 || y + 1 >= height) return 0;

    float SUM = 0;
    for (int i = -1; i <= 1; i++)
    {
        for(int j = -1; j <= 1; j++ )
        {
            int neighbor_idx = (y + j)* depth_step + (x + i) * depth_channels;
            DATA_TYPE D = depth[neighbor_idx];
            SUM += (int)D;
        }
    }

    int idx = y * depth_step + x * depth_channels;
    int D = (int)depth[idx];

    if (SUM - (9.0 * D) > GHOST_THRESHOLD)
        return 0;

    return 1;
}

#define WITH_PRECOMPILED_PARAMS 0
#define WITH_LOCK 0
#define PER_LINE 1

__kernel void dibr (
       __constant DATA_TYPE *src, /* Can we change that for a uchar3 ? */
       __constant DATA_TYPE  *depth,
       __global DATA_TYPE *out,
       __global DATA_TYPE *depth_out,
       volatile __global int *depth_mutex,
       __global DATA_TYPE *mask,
#if !WITH_PRECOMPILED_PARAMS
       int rows, int cols,                     // We can pre-compile this values
       int src_step, int out_step, int channel, // We can pre-compile this values
       int mask_step,                           // We can pre-compile this values
#endif
       __constant int *depth_shift_table_lookup,
       int S)
{

#if PER_LINE
    const int y = get_global_id(0);
#else
        const int x = get_global_id(0);
        const int y = get_global_id(1);
#endif

#if PER_LINE
for (int x = 0; x < cols; x ++)
{
#endif
        int idx = (y*src_step) + (x*channel);

#if !PER_LINE
        if ( isGhost (x, y, depth, mask_step/2, 1, cols, rows) ) // Should not project ghost pixels
            return;
#endif

        int D = depth[ y * (mask_step/2) + x];
        int shift = depth_shift_table_lookup [D];

        DATA_TYPE b, g, r;
        b = src [idx];
        g = src [idx+1];
        r = src [idx+2];
        S = 20;

#if 1
            if( (x + S - shift) < cols && (x + S - shift) >= 0)
            {
                int newidx = (y  * out_step) + (x + S - shift) * channel;
                int newidx_mask =  y * mask_step + (x + S - shift);
#if WITH_LOCK
                int processed = 0;
                while (!processed)
                {
                    if (atomic_cmpxchg (depth_mutex + (y * 2 * cols + (x + S - shift)), 0, 1) == 0) // got the lock
                    {
#endif
                        int previousDepthOut = depth_out[newidx];
                        if ( mask [newidx_mask] != '1' ||
                             D > previousDepthOut) // I need to process
                        {
                            out [newidx]   = b;
                            out [newidx+1] = g;
                            out [newidx+2] = r;

                            mask [newidx_mask] = '1';
                            depth_out[newidx] = D;
                        }
#if WITH_LOCK
                        processed = 1;
                        // free the lock
                        atomic_xchg (depth_mutex + (y * 2 * cols + (x + S - shift)), 0);
                    }
                    barrier(CLK_GLOBAL_MEM_FENCE);
                }
#endif

            }

            if( (x + shift - S >= 0) && (x + shift - S) < cols )
            {
                int newidx = (y * out_step) + ((x + shift - S) + cols) * channel;
                int newidx_mask =  y * mask_step + ((x + shift - S) + cols);
#if WITH_LOCK
                int processed = 0;
                while (!processed)
                {
                    if (atomic_cmpxchg (depth_mutex + (y * 2 * cols + (x + shift - S)) + cols, 0, 1) == 0) // got the lock
                    {
#endif
                        int previousDepthOut = depth_out[newidx];
                        if ( mask [newidx_mask] != '1' ||
                             D > previousDepthOut) // I need to process
                        {
                            out [newidx]   = b;
                            out [newidx+1] = g;
                            out [newidx+2] = r;

                            mask [newidx_mask] = '1';
                            depth_out[newidx]  = D;
                         }
#if WITH_LOCK
                        processed = 1;
                        // free the lock
                        atomic_xchg (depth_mutex + (y * 2 * cols + (x + shift - S) + cols), 0);
                    }
                    barrier(CLK_GLOBAL_MEM_FENCE);
                }
#endif
            }
#else
            //shift *= 2;
            if( x + S - shift < cols)
            {
                int newidx_mask =  y * mask_step + (x + S - shift);
                int newidx = (y  * out_step) + (x + S - shift) * channel;

                if(depth_out [newidx] <= D)
                {
                    out [newidx] = b;
                    out [newidx + 1] = g;
                    out [newidx + 2] = r;

                    depth_out[newidx] = (int)D;
                }

                mask [newidx_mask] = '1';
            }

            int newidx = (y * out_step) + (x + cols ) * channel;
            out [newidx] = b;
            out [newidx+1] = g;
            out [newidx+2] = r;

            newidx = y * mask_step + x + cols;
            mask [newidx] = '1';
#endif

#if PER_LINE
}
#endif

}

__kernel void hole_filling (
        __global DATA_TYPE *src,
        __global DATA_TYPE *out,
        __global DATA_TYPE *depthOut,
       __global DATA_TYPE *mask,
#if !WITH_PRECOMPILED_PARAMS
        int rows, int cols,
        int src_step, int out_step, int channel, int mask_step,
        int INTERPOLATION_HALF_SIZE_WINDOW
#endif
        )
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    int background = 255;

    int idxMask = y * mask_step + x;
    if(mask[idxMask] != '1') // is hole
    {
        float sumB = 0, sumG = 0, sumR = 0, Y = 0;
        float total = 0;

        // search for background depth
        for (int i = -INTERPOLATION_HALF_SIZE_WINDOW; i <= INTERPOLATION_HALF_SIZE_WINDOW; i++)
        {
            int j = 0;
            // for (int j = -INTERPOLATION_HALF_SIZE_WINDOW; j <= INTERPOLATION_HALF_SIZE_WINDOW; j++)
            // {
                if (x + i >= 0 && x + i < 2 * cols
                    && y + j >= 0 && y + j <= rows)
                {
                    int idxDepthOut = (y + j) * out_step + (x + i) * channel;
                    int idxMask1    = (y + j) * mask_step + (x + i);

                    if(mask[idxMask1] == '1') // its not a hole
                        if (depthOut[idxDepthOut] < background)
                            background = depthOut[idxDepthOut];
                }
             // }
        }

        // Do interpolation only with background objects
        for (int i = -INTERPOLATION_HALF_SIZE_WINDOW; i <= INTERPOLATION_HALF_SIZE_WINDOW; i++)
        {
            int j = 0;
            // for (int j = -INTERPOLATION_HALF_SIZE_WINDOW; j <= INTERPOLATION_HALF_SIZE_WINDOW; j++)
            // {
                if (x + i >= 0 && x + i < 2 * cols
                    && y + j >= 0 && y + j <= rows)
                {
                    int idxOut1 = (y + j) * out_step + (x + i) * channel;
                    int idxMask1 = (y + j) * mask_step + (x + i);

                    if(mask[idxMask1] == '1' && depthOut[idxOut1] == background) // it is not a hole and it is background
                    {
                        DATA_TYPE r, g, b;
                        b = out [idxOut1];
                        g = out [idxOut1 + 1];
                        r = out [idxOut1 + 2];

                        sumB += (int) b;
                        sumG += (int) g;
                        sumR += (int) r;

                        total += 1.0;
                    }
                }
            // }
        }

        int idxOut = y * out_step + x * channel;
        out[idxOut]   = (DATA_TYPE)(sumB/total);
        out[idxOut+1] = (DATA_TYPE)(sumG/total);
        out[idxOut+2] = (DATA_TYPE)(sumR/total);
    }
}

__kernel void convolute (
       __global DATA_TYPE *src,
       __global DATA_TYPE *out,
#if !WITH_PRECOMPILED_PARAMS
       int rows, int cols,
       int src_step, int channel,
#endif
       int FILTER_HALF_SIZE,
       __constant float *filter)
{
        const int x = get_global_id(1);
        const int y = get_global_id(0);

        if( x >= rows || y >= cols)
            return;

        int idx = (y*src_step)+(x*channel);

        float sumR = 0.0;
        float sumG = 0.0;
        float sumB = 0.0;

#if 1
        if ( (x - FILTER_HALF_SIZE) < 0 || (y - FILTER_HALF_SIZE < 0 ))
        {
            out[idx] = src[idx];
            out[idx+1] = src[idx+1];
            out[idx+2] = src[idx+2];
        }
        else
        {
            for (int i = -FILTER_HALF_SIZE; i <= FILTER_HALF_SIZE; i++)
            {
                for(int j = -FILTER_HALF_SIZE; j <= FILTER_HALF_SIZE; j++)
                {
                    int idxTmp = (y + j) * src_step + (x + i) * channel;
                    int idxFilter = (i + FILTER_HALF_SIZE) * 3 + j + FILTER_HALF_SIZE;

                    sumB += src[idxTmp] * filter[idxFilter];
                    sumG += src[idxTmp+1] * filter[idxFilter];
                    sumR += src[idxTmp+2] * filter[idxFilter];
                }
            }
        }

        if (sumB > 255) sumB = 255;
        if (sumG > 255) sumG = 255;
        if (sumR > 255) sumR = 255;

        out[idx] = sumB;
        out[idx+1] = sumG;
        out[idx+2] = sumR;
#else
        out[idx] = src[idx];
        out[idx+1] = src[idx+1];
        out[idx+2] = src[idx+2];
#endif
}
