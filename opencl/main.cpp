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
#include <dibr_cpu.h>
#include "dibr_ocl.h"

// Options -  We need a better way to do that
int  width = 1920/2, height = 1080/2;
bool depth_filter = true;
bool must_update = false, paused = false;
bool enable_hole_filling = true;
bool enable_dist = true;
bool output_video = false;
int  is_stereo = true; //(opts['s'] == "1");
int  use_opencl = false;
int  is_input_video = 0;
//end options

// Some global variables
Mat image, input, output;

// Some configuration sets
#define EYE_SEP_STEP 0.25
#define CONV_KERNEL_SIZE 9

double sigmax = 10.0, sigmay = 90;  // assymetric gaussian filter
double conv_kernel [CONV_KERNEL_SIZE];
// end

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

bool handle_key(int key)
{
  if (key == 27 || key == 1048603)
    return false;
  else if (key == 'j' || key == 1048682)
  {
    eye_sep -= EYE_SEP_STEP;
    update_depth_shift_lookup_table();
    must_update = true;
  }
  else if (key == 'k' || key == 1048683)
  {
    eye_sep += EYE_SEP_STEP;
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

  return true;
}

int main(int argc,char *argv[])
{
  parse_opts(argc, argv); // First, parse the user options

  use_opencl = (opts['o'] == "1");
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

#ifdef ENABLE_EYE_TRACKING
  bool enable_head_tracking = ( opts.count('t') ) && ( opts['t'] == "1");
  // begin eye tracking
  // Open webcam
  VideoCapture cap(0);
  cv::Mat frame, eye_tpl;
  cv::Rect eye_bb;
  if(enable_head_tracking)
  {
    face_cascade.load("/usr/share/opencv/haarcascades/haarcascade_frontalface_alt2.xml");
    // Check if everything is ok
    if (face_cascade.empty() || !cap.isOpened())
      return 1;

    // Set video to 320x240
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
  }
  // end eye tracking
#endif

  VideoCapture inputVideo(opts['i']);              // Open input
  inputVideo.set(CV_CAP_PROP_FPS, 10);

  if (!inputVideo.isOpened())
  {
    cout  << "Could not open the input video: " << opts['i'] << endl;
    return -1;
  }
  inputVideo >> image;

  int ex = static_cast<int>(inputVideo.get(CV_CAP_PROP_FOURCC));
  VideoWriter outputVideo;
  if(opts.count('u'))
  {
    output_video = true;
    outputVideo.open( opts['u'].c_str(),
                      CV_FOURCC('P', 'I', 'M', '1'),
                      inputVideo.get(CV_CAP_PROP_FPS),
                      Size(width, height), true);

    cout << "create output video " << opts['u'].c_str() << endl;
  }

  // Get mime-type of input
  magic_t magic;
  const char *mime;
  magic = magic_open(MAGIC_MIME_TYPE);
  magic_load(magic, NULL);
  magic_compile(magic, NULL);
  mime = magic_file(magic, opts['i'].c_str());
  is_input_video = (strstr(mime, "image") == NULL);
  magic_close(magic);

  // Creating image objects
  Mat color, depth, depth_filtered, depth_out, isHole, border, dist;

  input.create(height, width, CV_8UC(3));
  resize(image, input, input.size(), 0, 0, CV_INTER_CUBIC);

  int input_rows = input.rows;
  int input_cols = input.cols / 2;

  // if(is_stereo)
  //   input_cols /= 2;

  color.create(input_rows, input_cols, CV_8UC(3));
  depth.create(input_rows, input_cols, CV_8UC(1));
  depth_filtered.create(input_rows, input_cols, CV_8UC(3));
  border.create(input_rows, input_cols, CV_8UC(1));
  dist.create(input_rows, input_cols, CV_32F);

  //Output info
  int out_rows = input.rows;
  int out_cols = input.cols;

  depth_out.create(out_rows, out_cols, CV_8UC(1));
  output.create(out_rows, out_cols, CV_8UC(3));
  isHole.create(out_rows, out_cols, CV_8UC1);

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

      // gettimeofday(&now, NULL);
      // bilateralFilter ( cropped, depth, 15, 100, 100, BORDER_ISOLATED );
      // GaussianBlur(cropped, depth, Size (101, 101), 10, 90);
      // gettimeofday(&end, NULL);
      // diff = timeval_subtract(&result, &end, &now);
      // cout << (float)diff << "\t";

      imshow("depth", depth);
      detect_border(depth, border);
      imshow("border", border);

      Mat bw;
      cv::threshold(border, bw, 40, 255, CV_THRESH_BINARY);
      bw =  cv::Scalar::all(255) - bw;
      imshow("bw", bw);
      cv::distanceTransform(bw, dist, CV_DIST_C, CV_DIST_MASK_PRECISE);
      cv::normalize(dist, dist, 1., 0, cv::NORM_MINMAX);
      imshow("dist", dist);
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
                    depth,
                    isHole,
                    &depth_shift_table_lookup[0],
                    pixelMutex,
                    true,
                    enable_hole_filling );
        }
        else
          shift_surface ( color,
                         depth,
                         dist,
                         output,
                         20,
                         enable_hole_filling,
                         enable_dist,
                         is_stereo );

      gettimeofday(&end, NULL);
      diff = timeval_subtract(&result, &end, &now);
      cout << (float)diff << "\t";

      if(fullscreen)
        cvSetWindowProperty("Output", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
      imshow("Output", output);

      must_update = false;

      if (output_video)
      {
        outputVideo.write(output);
      }
    }

    gettimeofday(&now, NULL);
    int key = cvWaitKey(1);
    gettimeofday(&end, NULL);
    diff = timeval_subtract(&result, &end, &now);
    cout << (float)diff << endl;

    cout << "eye_sep:" << (float) eye_sep << endl;

    if(!handle_key(key)) // It will return false if key is ESC
        break;

#ifdef ENABLE_EYE_TRACKING
    cap >> frame;
    if (frame.empty())
      break;

    if(enable_head_tracking)
    {
      // Flip the frame horizontally, Windows users might need this
      cv::flip(frame, frame, 1);

      // Convert to grayscale and
      // adjust the image contrast using histogram equalization
      cv::Mat gray;
      cv::cvtColor(frame, gray, CV_BGR2GRAY);


      if (eye_bb.width == 0 && eye_bb.height == 0)
      {
        // Detection stage
        // Try to detect the face and the eye of the user
        detectEye(gray, eye_tpl, eye_bb);
      }
      else
      {
        // Tracking stage with template matching
        trackEye(gray, eye_tpl, eye_bb);

        // Draw bounding rectangle for the eye
        cv::rectangle(frame, eye_bb, CV_RGB(0,255,0));

        eye_sep = 6 - (eye_bb.x)/ 5;
        update_depth_shift_lookup_table();
        must_update;
      }
    }

    // Display video
    cv::imshow("video", frame);
#endif
  }

  delete pixelMutex;
  o.destroy();
}
