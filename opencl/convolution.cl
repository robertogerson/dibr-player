#define DATA_TYPE unsigned char

// Converts an rgb value to YUV
uint3 rgb2YUV(int r, int g, int b)
{
    uint3 YUV = (0.299*r + 0.587*g + 0.114*b,
                 (b-YUV.x)*0.565,
                 (r-YUV.x)*0.713);
    return YUV;
}

float rgb2Y(DATA_TYPE r, DATA_TYPE g, DATA_TYPE b)
{
    float Y = (float)(0.299)*(float)r + (float)(0.587)*(float)g + (float)(0.114)*(float)b;
    return Y;
}

// Put pixel in a BGR image type
void putPixel ( __global DATA_TYPE *image,
                int src_step, int channel,
                int x, int y,
                unsigned char r, unsigned char g, unsigned char b)
{
    int idx = (y * src_step) + (x*channel);
    image [idx] = b;
    image [idx+1] = g;
    image [idx+2] = r;
}

uint3 getPixel ( __constant DATA_TYPE *image,
                int src_step, int channel,
                int x, int y)
{
    int idx = (y * src_step) + (x * channel);
    return ( /*b = */ image [idx],
              /*g = */ image [idx+1],
              /*r = */ image [idx+2]);
}

__kernel void dibr (
       __constant DATA_TYPE *src,
       __constant DATA_TYPE  *depth,
       __global DATA_TYPE *out,
       __global DATA_TYPE *mask,
       int rows, int cols,
       int src_step, int out_step, int channel,
       int mask_step,
       __constant int *depth_shift_table_lookup,
       int S)
{
        const int x = get_global_id(0);
        const int y = get_global_id(1);

        int idx = (y*src_step) + (x*channel);
        DATA_TYPE b = depth[idx];
        DATA_TYPE g = depth[idx+1];
        DATA_TYPE r = depth[idx+2];
        int Y, U, V;

        float D = rgb2Y(r, g, b);
        int shift = depth_shift_table_lookup [ (int)D];

        b = src [idx];
        g = src [idx+1];
        r = src [idx+2];
        S = 20;

        if(1)
        {
            if( x + S - shift < cols)
            {
                int newidx = (y  * out_step) + (x + S - shift) * channel;
                out [newidx] = b;
                out [newidx+1] = g;
                out [newidx+2] = r;

                newidx =  y * mask_step + (x + S - shift);
                mask [newidx] = '1';
            }

            if( x + cols + shift - S >= 0 )
            {
                int newidx = (y  * out_step) + (x + cols + shift - S) * channel;
                out [newidx] = b;
                out [newidx+1] = g;
                out [newidx+2] = r;

                newidx =  y * mask_step + (x + shift - S) + cols;
                mask [newidx] = '1';
            }
        }
        else
        {
            shift *= 2;
            if( x + S - shift < cols)
            {
                int newidx = (y  * out_step) + (x + S - shift) * channel;
                out [newidx] = b;
                out [newidx+1] = g;
                out [newidx+2] = r;

                newidx =  y * mask_step + (x + S - shift);
                mask [newidx] = '1';
            }

            int newidx = (y * out_step) + (x + cols )* channel;
            out [newidx] = b;
            out [newidx+1] = g;
            out [newidx+2] = r;

            newidx =  y * mask_step + x;
            mask [newidx] = '1';
        }

}

__kernel void hole_filling (
        __global DATA_TYPE *src,
        __global DATA_TYPE *out,
       __global DATA_TYPE *mask,
        int rows, int cols,
        int src_step, int out_step, int channel, int mask_step,
        int INTERPOLATION_HALF_SIZE_WINDOW)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);


    int idxMask = y * mask_step + x;
    if(mask[idxMask] != '1') // is hole
    {
        float sumB = 0, sumG = 0, sumR = 0, Y = 0;
        float total = 0;
        for (int i = -INTERPOLATION_HALF_SIZE_WINDOW; i <= INTERPOLATION_HALF_SIZE_WINDOW; i++)
        {
            if (x + i >= 0 && x + i < 2 * rows)
            {
                int idxOut1 = y * out_step + (x + i) * channel;
                int idxMask1 = y * mask_step + x + i;

                if(mask[idxMask1] == '1') // non-hole
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
        }

        int idxOut = y * out_step + x * channel;
        out[idxOut] = (DATA_TYPE)(sumB/total);
        out[idxOut+1] = (DATA_TYPE)(sumG/total);
        out[idxOut+2] = (DATA_TYPE)(sumR/total);
    }
}

__kernel void convolute (
       __global DATA_TYPE *src,
       __global DATA_TYPE *out,
       int rows, int cols,
       int src_step, int channel,
       int FILTER_HALF_SIZE,
       __constant float *filter)
{
        const int x = get_global_id(0);
        const int y = get_global_id(1);

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
