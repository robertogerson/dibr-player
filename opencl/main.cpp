#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
//System header files
#include <iostream>
#include <cstdlib>
#include "string.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctime>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include <iostream>
#include <ctime>
#include <sys/time.h>

//Options parse
#include <options.h>

//File mime-type
#include <magic.h>

//OpenCL heaer file
#include <CL/cl.h>

//OpenCV header files
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

//OpenCL Header Files
#include <CL/opencl.h>
#include "ocl_common.h"

bool depth_filter = true;
#define CONV_KERNEL_SIZE 9
double sigmax = 10.0, sigmay = 90;  // assymetric gaussian filter
double conv_kernel [CONV_KERNEL_SIZE];

using namespace std;

//function to get the time difference
long int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
  (void) result;

  long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
  //result->tv_sec = diff / 1000000;
  //result->tv_usec = diff % 1000000;
  return (diff);
}

Mat image, input, output;

#define N 256
int depth_shift_table_lookup[N];
int eye_sep = 6;

/* DIBR STARTS HERE */
int find_shiftMC3(int depth, int Ny, int eye_sep = 6) // eye separation 6cm
{
  (void) Ny;

  int h;
  int nkfar = 128, nknear = 128, kfar = 0, knear = 0;
  int n_depth = 256;  // Number of depth planes

  // This is a display dependant parameter and the maximum shift depends
  // on this value. However, the maximum disparity should not exceed
  // particular threshold, defined by phisiology of human vision.
  int Npix = 100; // 300 TODO: Make this adjustable in the interface.
  int h1 = 0;
  int A = 0;
  int h2 = 0;

  // Viewing distance. Usually 300 cm
  int D = 300;

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
  knear = 0;
  A  = depth*(knear + kfar)/(n_depth-1);
  h1 = - eye_sep * Npix * ( A - kfar ) / D;
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
    printf ("%d ", depth_shift_table_lookup[i]);
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
                     Mat &output,
                     int S = 20,
                     bool hole_filling = true)
{
  bool mask [output.rows][output.cols];
  memset (mask, false, output.rows * output.cols * sizeof(bool));

  // Calculate left image
  // for every pixel change its value
  for (int y = 0; y < image_color.rows; y++)
  {
    for (int x = image_color.cols-1; x >= 0; --x)
    {
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
              int oldidx = y * image_color.step + x1 * image_color.channels();
              b_sum += image_color.data[oldidx];
              g_sum += image_color.data[oldidx+1];
              r_sum += image_color.data[oldidx+2];
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

  // Calculate right image
  // for every pixel change its value
  for (int y = 0; y < image_color.rows; y++)
  {
    for (int x = image_color.cols-1; x >= 0; --x)
    {
      // get depth
      int idx = y * image_depth.step + x * image_depth.channels();
      int D = image_depth.data[idx];

      char b, g, r;
      idx = y * image_color.step + x * image_color.channels();
      b = image_color.data[idx];
      g = image_color.data[idx + 1];
      r = image_color.data[idx + 2];

      int shift = depth_shift_table_lookup [D];

      if( (x + shift - S >= 0)  && (x + shift - S < image_color.cols))
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
              int oldidx = y * image_color.step + x1 * image_color.channels();
              b_sum += image_color.data[oldidx];
              g_sum += image_color.data[oldidx+1];
              r_sum += image_color.data[oldidx+2];
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
  return true;
}

int is_input_video = 0;

int main(int argc,char *argv[])
{
  parse_opts(argc, argv); // First, parse the user options

  int must_update = false, paused = false;  
  int use_opencl = (opts['o'] == "1");

  VideoCapture inputVideo(opts['i']);              // Open input
  // inputVideo.set(CV_CAP_PROP_FPS, 10);
  if (!inputVideo.isOpened())
  {
    cout  << "Could not open the input video: " << opts['i'] << endl;
    return -1;
  }
  inputVideo >> image;

  // Get mime-type of input
  magic_t magic;
  const char *mime;
  magic = magic_open(MAGIC_MIME_TYPE);
  magic_load(magic, NULL);
  magic_compile(magic, NULL);
  mime = magic_file(magic, opts['i'].c_str());
  is_input_video = (strstr(mime, "image") == NULL);
  magic_close(magic);

  input.create(1080, 1920 * 2, CV_8UC(3));
  resize(image, input, input.size(), 0, 0, CV_INTER_CUBIC);

  // Creating image objects
  Mat color, depth, depth_filtered, depth_out, isHole;
  color.create(input.rows, input.cols/2, CV_8UC(3));
  // depth.create(input.rows, input.cols/2, CV_8UC(3));
  depth_out.create(input.rows, input.cols, CV_8UC(3));
  depth_filtered.create(input.rows, input.cols/2, CV_8UC(3));
  isHole.create(input.rows, input.cols, CV_8UC1);
  output.create(input.rows, input.cols, CV_8UC(3));

  int *pixelMutex = (int*)malloc (input.rows * input.cols * sizeof (int));

  //creating OCLX object
  OCLX o;
  //structures to hold kernel and program
  cl_kernel kernel[10];
  cl_program program;

  struct timeval end,result,now;
  long int diff;

  // Output Window
  bool fullscreen = opts.count('f');
  cvNamedWindow("Output", CV_WINDOW_NORMAL);

  update_depth_shift_lookup_table();

  //initialising opencl structures
  o.init();
  //loading the program and kernel source
  o.load_demo(&program, &kernel[0]);
  cout << "#Decode\tResize\tCrop\tFilter\tDIBR\tShow" << endl;
  for(;;)
  {
    if (must_update)
    {
      gettimeofday(&now, NULL);
      if(is_input_video)
        inputVideo >> image;
      gettimeofday(&end, NULL);
      diff = timeval_subtract(&result, &end, &now);
      cout << (float)diff << "\t";

      gettimeofday(&now, NULL);
      resize(image, input, input.size(), 0, 0, INTER_NEAREST);
      gettimeofday(&end, NULL);
      diff = timeval_subtract(&result, &end, &now);
      cout << (float)diff << "\t";

      gettimeofday(&now, NULL);
      Mat cropped = input(Rect(0, 0, (input.cols / 2), input.rows));
      cropped.copyTo(color);
      cropped = input(Rect((input.cols / 2), 0, (input.cols / 2), input.rows));
      // cropped.copyTo(depth);
      cvtColor(cropped, depth, CV_RGB2GRAY);
      imshow("depth", depth);
      gettimeofday(&end, NULL);
      diff = timeval_subtract(&result, &end, &now);
      cout << (float)diff << "\t";

      gettimeofday(&now, NULL);
      /* bilateralFilter ( cropped, depth, 15, 100, 100, BORDER_ISOLATED ); */
      // GaussianBlur(cropped, depth, Size (101, 101), 10, 90);
      gettimeofday(&end, NULL);
      diff = timeval_subtract(&result, &end, &now);
      cout << (float)diff << "\t";

      memset(pixelMutex, 0, output.cols * output.rows * sizeof(int));
      isHole.setTo(cv::Scalar(0));
      depth_out.setTo(cv::Scalar(0, 0, 0));
      output.setTo(cv::Scalar(255, 255, 255));

      imshow("image", color);
      imshow("depth2", depth);

      //running parallel program
      gettimeofday(&now, NULL);
      // o.conv( color, output, &kernel[0], program );
        if(use_opencl)
        {
          o.dibr ( color,
                 depth,
                 depth_filtered,
                 output,
                 depth_out,
                 pixelMutex,
                 isHole,
                 &kernel[0], program,
                 &depth_shift_table_lookup[0] );
        }
        else
          shift_surface(color, depth, output);

      gettimeofday(&end, NULL);
      diff = timeval_subtract(&result, &end, &now);
      cout << (float)diff << "\t";

      if(fullscreen)
        cvSetWindowProperty("Output", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
      imshow("Output", output);

      must_update = false;
    }

    gettimeofday(&now, NULL);
    int key = cvWaitKey(1);
    gettimeofday(&end, NULL);
    diff = timeval_subtract(&result, &end, &now);
    cout << (float)diff << endl;

    if (key == 27)
      break;
    else if (key == 'j' || key == 1048682)
    {
      eye_sep -= 2;
      update_depth_shift_lookup_table();
      must_update = true;
    }
    else if (key == 'k' || key == 1048683)
    {
      eye_sep += 2;
      update_depth_shift_lookup_table();
      must_update = true;
    }
    else if (is_input_video)
    {
      must_update = true;
    }
  }

  delete pixelMutex;
  o.destroy();
}
