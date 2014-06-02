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
#include <sstream>
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
#include "opencv2/imgproc/imgproc_c.h"

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
    // printf ("%d ", depth_shift_table_lookup[i]);
  }
}

void get_YUV(char r, char g, char b, int &Y, int &U, int &V)
{
  Y = 0.299*r + 0.587*g + 0.114*b;
  U = (b-Y)*0.565;
  V = (r-Y)*0.713;
}

#define BORDER_THRESHOLD 0.01

bool shift_surface ( Mat &image_color,
                     Mat &image_depth,
                     Mat &image_border,
                     Mat &output,
                     int S = 20,
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

        if(image_border.at<float>(y, x) < BORDER_THRESHOLD)
          continue;

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

void detect_border(Mat &src_gray, Mat &out)
{
#if  0
  int scale = 1;
  int delta = 0;
  int ddepth = CV_16S;

  /// Generate grad_x and grad_y
  Mat grad_x, grad_y;
  Mat abs_grad_x, abs_grad_y;

  /// Gradient X
  //Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );
  Sobel( src_gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT );
  convertScaleAbs( grad_x, abs_grad_x );

  /// Gradient Y
  //Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
  Sobel( src_gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT );
  convertScaleAbs( grad_y, abs_grad_y );

  /// Total Gradient (approximate)
  addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, out );

  GaussianBlur( out, out, Size(3, 3), 0, 0, BORDER_DEFAULT );
#else
  int X = 3;
  int aperature_size = X;
  int lowThresh = 70;
  cv::Canny( src_gray, out, lowThresh, lowThresh*X, aperature_size );
#endif
}

int is_input_video = 0;

int main(int argc,char *argv[])
{
  int width = 1920, height = 1080;
  parse_opts(argc, argv); // First, parse the user options

  bool must_update = false, paused = false;
  bool enable_hole_filling = true;
  bool enable_dist = true;
  int is_stereo = opts['s'] == "1";

  int use_opencl = (opts['o'] == "1");
  if (opts.count('w'))
  {
    std::istringstream ss(opts['w']);
    ss >> width;
  }
  if (opts.count('h'))
  {
    std::istringstream ss(opts['h']);
    ss >> height;
  }

  cout << width << "x" << height << endl;

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

  input.create(height, width, CV_8UC(3));
  resize(image, input, input.size(), 0, 0, CV_INTER_CUBIC);

  // Creating image objects
  Mat color, depth, depth_filtered, depth_out, isHole, border, dist;

  int input_rows = input.rows;
  int input_cols = input.rows;

  if(is_stereo)
    input_cols /= 2;

  color.create(input_rows, input_cols, CV_8UC(3));
  depth.create(input_rows, input_cols, CV_8UC(1));
  depth_filtered.create(input_rows, input_cols, CV_8UC(3));
  border.create(input_rows, input_cols, CV_8UC(1));
  dist.create(input_rows, input_cols, CV_32F);

  //Output info
  int out_rows = input.rows;
  int out_cols = input.cols;

  depth_out.create(out_rows, out_cols, CV_8UC(1));
  isHole.create(out_rows, out_cols, CV_8UC1);
  output.create(out_rows, out_cols, CV_8UC(3));

  int *pixelMutex = (int*) malloc (input.rows * input.cols * sizeof (int));

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
      if(is_input_video && !paused)
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
      cvtColor(cropped, depth, CV_RGB2GRAY);
      imshow("depth", depth);
      detect_border(depth, border);
      imshow("border", border);
      Mat bw;
      cv::threshold(border, bw, 40, 255, CV_THRESH_BINARY);
      bw =  cv::Scalar::all(255) - bw;
      imshow("bw", bw);
      cv::distanceTransform(bw, dist, CV_DIST_L12, CV_DIST_MASK_PRECISE);
      cv::normalize(dist, dist, 1., 0, cv::NORM_MINMAX);
      imshow("dist", dist);
      gettimeofday(&end, NULL);
      diff = timeval_subtract(&result, &end, &now);
      cout << (float)diff << "\t";

      gettimeofday(&now, NULL);
      // bilateralFilter ( cropped, depth, 15, 100, 100, BORDER_ISOLATED );
      // GaussianBlur(cropped, depth, Size (101, 101), 10, 90);
      gettimeofday(&end, NULL);
      diff = timeval_subtract(&result, &end, &now);
      cout << (float)diff << "\t";

      memset(pixelMutex, 0, input.cols * input.rows * sizeof(int));
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
          o.dibr ( &kernel[0], program,
                    color,
                    depth,
                    depth_filtered,
                    output,
                    depth_out,
                    isHole,
                   &depth_shift_table_lookup[0],
                    pixelMutex);
        }
        else
          shift_surface(color,
                        depth,
                        dist,
                        output, 20,
                        enable_hole_filling,
                        enable_dist,
                        is_stereo);

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
    else if(key == 'h' || key == 1048680)
    {
      enable_hole_filling = !enable_hole_filling;
    }
    else if(key == 'd' || key == 1048676)
    {
      enable_dist = !enable_dist;
    }
    else if (is_input_video)
    {
      if(key == ' ' || key == 1048608)
        paused = !paused;
      must_update = true;
    }
  }

  delete pixelMutex;
  o.destroy();
}
