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
#include <iostream>
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
  long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
  //result->tv_sec = diff / 1000000;
  //result->tv_usec = diff % 1000000;
  return (diff);
}

Mat image, input, output;

#define N 256
int depth_shift_table_lookup[N];
int eye_sep = 100;

/* DIBR STARTS HERE */
int find_shiftMC3(int depth, int Ny, int eye_sep = 6) // eye separation 6cm
{
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
  h1 = -eye_sep*Npix*( A - kfar )/D;
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
  // Remove from here
  for(int i = 0; i < N; i++)
  {
    depth_shift_table_lookup[i] = find_shiftMC3(i, N, eye_sep);
    printf ("%d ", depth_shift_table_lookup[i]);
  }
}

int is_video = 0;

int main(int argc,char *argv[])
{  
  parse_opts(argc, argv); // First, parse the user options

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
  is_video = (strstr(mime, "image") == NULL);
  magic_close(magic);

  input.create(1080, 1920, CV_8UC(3));
  resize(image, input, input.size(), 0, 0, INTER_NEAREST);

  // Creating image objects
  Mat color, depth, depth_filtered, depth_out, isHole;
  color.create(input.rows, input.cols/2, CV_8UC(3));
  depth.create(input.rows, input.cols/2, CV_8UC(3));
  depth_out.create(input.rows, input.cols/2, CV_8UC(3));
  depth_filtered.create(input.rows, input.cols/2, CV_8UC(3));
  isHole.create(input.rows, input.cols, CV_8UC1);
  output.create(input.rows, input.cols, CV_8UC(3));

  //creating OCLX object
  OCLX o;
  //structures to hold kernel and program
  cl_kernel kernel[10];
  cl_program program;

  struct timeval end,result,now;
  long int diff;

  //initialising opencl structures
  o.init();

  //loading the program and kernel source
  o.load_demo(&program, &kernel[0]);
  cout << "#\tDecode\tResize\tCrop\tFilter\tShow\tDIBR\tShow" << endl;
  for(;;)
  {
    gettimeofday(&now, NULL);
    if(is_video)
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
    cropped.copyTo(depth);
    gettimeofday(&end, NULL);
    diff = timeval_subtract(&result, &end, &now);
    cout << (float)diff << "\t";
    // imshow("depth1", cropped);

    gettimeofday(&now, NULL);
    GaussianBlur(cropped, depth, Size (31, 31), 10, 100);
    gettimeofday(&end, NULL);
    diff = timeval_subtract(&result, &end, &now);
    cout << (float)diff << "\t";

    isHole.setTo(cv::Scalar(0));
    depth_out.setTo(cv::Scalar(0, 0, 0));
    output.setTo(cv::Scalar(255, 255, 255));

    gettimeofday(&now, NULL);
    imshow("image", color);
    imshow("depth2", depth);
    gettimeofday(&end, NULL);
    diff = timeval_subtract(&result, &end, &now);
    cout << (float)diff << "\t";

    //running parallel program
    gettimeofday(&now, NULL);
    // o.conv( color, output, &kernel[0], program );
    o.dibr ( color,
             depth,
             depth_filtered,
             output,
             depth_out,
             isHole,
             &kernel[0], program,
        &depth_shift_table_lookup[0] );

    gettimeofday(&end, NULL);
    diff = timeval_subtract(&result, &end, &now);
    cout << (float)diff << "\t";

    gettimeofday(&now, NULL);
    cvNamedWindow("Name", CV_WINDOW_NORMAL);
    cvSetWindowProperty("Name", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    imshow("Name", output);
    gettimeofday(&end, NULL);
    diff = timeval_subtract(&result, &end, &now);
    cout << (float)diff << endl;

    int key = cvWaitKey(1);
    if (key == 1113937) // LEFT_KEY
    {
      eye_sep -= 10;
      update_depth_shift_lookup_table();
    }
    else if (key == 1113939) //RIGHT_KEY
    {
      eye_sep += 10;
      update_depth_shift_lookup_table();
    }
  }

  o.destroy();
}
