#define DATA_TYPE unsigned char

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

/*      cconst int idx00 = (y-1) * src_step + (x-1)*channel;
        const int idx01 = (y) * src_step + (x-1)*channel;
        const int idx02 = (y+1) * src_step + (x-1)*channel;
        const int idx10 = (y-1) * src_step + (x)*channel;
        const int idx11 = (y) * src_step + (x)*channel;
        const int idx12 = (y+1) * src_step + (x)*channel;
        const int idx20 = (y-1) * src_step + (x+1)*channel;
        const int idx21 = (y) * src_step + (x+1)*channel;
        const int idx22 = (y+1) * src_step + (x+1)*channel;

        out[idx] = (DATA_TYPE)(src[idx00] * -1.0 + src[idx01] * 0.0 + src[idx02] * 1.0
                             + src[idx10] * -2.0 + src[idx11] * 0.0 + src[idx12] * 2.0
                             + src[idx20] * -1.0 + src[idx21] * 0.0 + src[idx22] * 1.0);

        out[idx+1] = (DATA_TYPE)(src[idx00+1] * -1.0 + src[idx01+1] * 0.0 + src[idx02+1] * 1.0
                                + src[idx10+1] * -2.0 + src[idx11+1] * 0.0 + src[idx12+1] * 2.0
                                + src[idx20+1] * -1.0 + src[idx21+1] * 0.0 + src[idx22+1] * 1.0);

        out[idx+2] = (DATA_TYPE)(src[idx00+2] * -1.0 + src[idx01+2] * 0.0 + src[idx02+2] * 1.0
                                + src[idx10+2] * -2.0 + src[idx11+2] * 0.0 + src[idx12+2] * 2.0
                                + src[idx20+2] * -1.0 + src[idx21+2] * 0.0 + src[idx22+2] * 1.0);
                                */
}
