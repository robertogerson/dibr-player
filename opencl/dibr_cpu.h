//OpenCV header files
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"

#define N 256
double  eye_sep = 0.25;
double  BORDER_THRESHOLD = 0.009;
int     depth_shift_table_lookup[N];

/* DIBR STARTS HERE */
int find_shiftMC3(int depth, int Ny, double eye_sep = 6) // eye separation 6cm
{
  (void) Ny;

  int h;
  int nkfar = 128, nknear = 128, kfar = 0, knear = 0;
  int n_depth = 256;  // Number of depth planes

  // This is a display dependant parameter and the maximum shift depends
  // on this value. However, the maximum disparity should not exceed
  // particular threshold, defined by phisiology of human vision.
  int Npix = 1920/2; // 300 TODO: Make this adjustable in the interface.
  int h1 = 0;
  int A = 0;
  int h2 = 0;

  // Viewing distance. Usually 300 cm
  int D = 100;

  // According to N8038
  knear = nknear / 64;
  kfar = nkfar / 16;

  // Assumption 1: For visual purposes
  // This can be let as it is. However, then we are not able to have a
  // clear understanding of how the rendered views look like.
  // knear = 0 means everything is displayed behind the screen
  // which is practically not the case.
  //
  // Interested user can remove this part to see what happens.
  // knear = 0;
  A  = depth * (knear + kfar)/(n_depth-1);
  h1 = - eye_sep * Npix * ( A - kfar ) / D;

  // return h1; // comment this for the original formulation

  h2 = (h1/2) % 1; //  Warning: Previously this was rem(h1/2,1)

  if (h2>=0.5)
    h = ceil(h1/2);
  else
    h = floor(h1/2);
  if (h<0)
    // It will never come here due to Assumption 1
    h = 0 - h;
  return h;
}

/*
 * Calls find_shiftMC3 for each possible depth value
 * (e.g. [0-255] and stores its value in depth_shift_table_lookup
 * */
void update_depth_shift_lookup_table ()
{
  for(int i = 0; i < N; i++)
  {
    depth_shift_table_lookup[i] = find_shiftMC3(i, N, eye_sep);
    // printf ("%d ", depth_shift_table_lookup[i]);
  }
}

void get_YUV(char r, char g, char b, int &Y, int &U, int &V)
{
  Y = 0.299*r + 0.587*g + 0.114*b;
  U = (b-Y)*0.565;
  V = (r-Y)*0.713;
}

bool shift_surface ( Mat &image_color,
                     Mat &image_depth,
                     Mat &image_border,
                     Mat &output,
                     int S = 58,
                     bool hole_filling = true,
                     bool enable_dist = true,
                     bool is_stereo = false)
{
  bool mask [output.rows][output.cols];
  memset (mask, false, output.rows * output.cols * sizeof(bool));

  // Calculate left image
  // for every pixel change its value
  for (int y = 0; y < image_color.rows; y++)
  {
    for (int x = image_color.cols-1; x >= 0; --x)
    {
      if(enable_dist && image_border.at<float>(y, x) <= (float)BORDER_THRESHOLD)
        continue;

      // get depth
      int idx = y * image_depth.step + x * image_depth.channels();
      int D = image_depth.data[idx];

      char b, g, r;
      idx = y * image_color.step + x * image_color.channels();
      b = image_color.data[idx];
      g = image_color.data[idx + 1];
      r = image_color.data[idx + 2];

      int shift = depth_shift_table_lookup [D];
      if( (x + S - shift) < image_color.cols && (x + S - shift >= 0))
      {
        int newidx = y * output.step + (x + S - shift) * output.channels();
        output.data[newidx] = b;
        output.data[newidx + 1] = g;
        output.data[newidx + 2] = r;

        mask [y][x + S - shift] = 1;
      }
    }

    if(hole_filling)
    {
      for (int x = 1; x < image_color.cols; x++)
      {
        if ( mask[y][x] == 0 )
        {
          if ( x - 7 < 0)
          {
            int oldidx = y * image_color.step + x * image_color.channels();
            int newidx = y * output.step + x * output.channels();

            output.data[newidx] = image_color.data[oldidx];
            output.data[newidx + 1] = image_color.data[oldidx + 1];
            output.data[newidx + 2] = image_color.data[oldidx + 2];
          }
          else
          {
            // interpolation between neighbord pixels
            int r_sum = 0, g_sum = 0, b_sum = 0;
            for (int x1 = x-7; x1 <= x-4; x1++)
            {
              int oldidx = y * output.step + x1 * output.channels();
              b_sum += output.data[oldidx];
              g_sum += output.data[oldidx+1];
              r_sum += output.data[oldidx+2];
            }

            int newidx = y * output.step + x * output.channels();
            output.data[newidx] = (b_sum / 4);
            output.data[newidx+1] = (g_sum / 4);
            output.data[newidx+2] = (r_sum / 4);
          }
        }
      }
    }
  }

  if(is_stereo)
  {
    // Calculate right image
    // for every pixel change its value
    for (int y = 0; y < image_color.rows; y++)
    {
      for (int x = image_color.cols-1; x >= 0; --x)
      {
        // get depth
        int idx = y * image_depth.step + x * image_depth.channels();
        int D = image_depth.data[idx];

        if(enable_dist && image_border.at<float>(y, x) < BORDER_THRESHOLD)
          continue;

        char b, g, r;
        idx = y * image_color.step + x * image_color.channels();
        b = image_color.data[idx];
        g = image_color.data[idx + 1];
        r = image_color.data[idx + 2];

        int shift = depth_shift_table_lookup [D];

        if( (x + shift - S >= 0)  && (x + shift - S < image_color.cols) &&
            !(mask [y][(x + shift - S) + image_color.cols]))
        {
          int newidx = y * output.step + ((x + shift - S) + image_color.cols) * output.channels();
          output.data[newidx] = b;
          output.data[newidx + 1] = g;
          output.data[newidx + 2] = r;

          mask [y][(x + shift - S) + image_color.cols] = 1;
        }
      }

      if(hole_filling)
      {
        for (int x = image_color.cols-1 ; x >= 0; --x)
        {
          if ( mask[y][x + image_color.cols] == 0 )
          {
            if ( x + 7 > image_color.cols - 1)
            {
              int oldidx = y * image_color.step + x * image_color.channels();
              int newidx = y * output.step + (x + image_color.cols) * output.channels();

              output.data[newidx] = image_color.data[oldidx];
              output.data[newidx + 1] = image_color.data[oldidx + 1];
              output.data[newidx + 2] = image_color.data[oldidx + 2];
            }
            else
            {
              // interpolation between neighbord pixels
              int r_sum = 0, g_sum = 0, b_sum = 0;
              for (int x1 = x+4; x1 <= x+7; x1++)
              {
                int oldidx = y * output.step + (x1 + image_color.cols) * output.channels();
                b_sum += output.data[oldidx];
                g_sum += output.data[oldidx+1];
                r_sum += output.data[oldidx+2];
              }

              int newidx = y * output.step + (x + image_color.cols) * output.channels();
              output.data[newidx] = (b_sum / 4);
              output.data[newidx+1] = (g_sum / 4);
              output.data[newidx+2] = (r_sum / 4);
            }
          }
        }
      }
    }
  }
  return true;
}
