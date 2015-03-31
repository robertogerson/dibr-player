#include "yuv.h"

enum YUV_ReturnValue
YUV_init(FILE *fin, size_t w, size_t h, struct YUV_Capture *out)
{
    if (!fin || w % 2 == 1 || h % 2 == 1)
        return YUV_PARAMETER_ERROR;

    out->fin = fin;
    out->width = w;
    out->height = h;

    out->ycrcb = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 3);
    out->y = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 1);
    out->cb = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 1);
    out->cr = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 1);
    out->cb_half = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_8U, 1);
    out->cr_half = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_8U, 1);

    if
    (
       out->ycrcb == NULL
       ||
       out->y == NULL
       ||
       out->cb == NULL
       ||
       out->cr == NULL
       ||
       out->cb_half == NULL
       ||
       out->cr_half == NULL
    )
    {
        YUV_cleanup(out);
        return YUV_OUT_OF_MEMORY;
    }

    return YUV_OK;
}

enum YUV_ReturnValue
YUV_read(struct YUV_Capture *cap)
{
    size_t bytes_read;
    size_t npixels;

    npixels = cap->width*cap->height;
    bytes_read = fread(cap->y->imageData, sizeof(uint8_t), npixels, cap->fin);
    if (bytes_read == 0)
        return YUV_EOF;
    else if (bytes_read != npixels)
        return YUV_IO_ERROR;
    bytes_read = fread(cap->cb_half->imageData, sizeof(uint8_t), npixels/4, cap->fin);
    if (bytes_read != npixels/4)
        return YUV_IO_ERROR;

    bytes_read = fread(cap->cr_half->imageData, sizeof(uint8_t), npixels/4, cap->fin);
    if (bytes_read != npixels/4)
        return YUV_IO_ERROR;

    cvResize(cap->cb_half, cap->cb, CV_INTER_CUBIC);
    cvResize(cap->cr_half, cap->cr, CV_INTER_CUBIC);
    cvMerge(cap->y, cap->cr, cap->cb, NULL, cap->ycrcb);

    return YUV_OK;
}

void
YUV_cleanup(struct YUV_Capture *cap)
{
    if (!cap)
        return;

    if (cap->ycrcb)
        free(cap->ycrcb);
    if (cap->y)
        free(cap->y);
    if (cap->cb)
        free(cap->cb);
    if (cap->cr)
        free(cap->cr);
    if (cap->cb_half)
        free(cap->cb_half);
    if (cap->cr_half)
        free(cap->cr_half);
}
