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
#include <iostream>
#include <sstream>
#include <ctime>
#include <sys/time.h>
using namespace std;

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

//Boost::program_options
#include <boost/program_options.hpp>
namespace po = boost::program_options;

//OpenCL Header files
#include <CL/opencl.h>

// My Header files
#include "dibr_cpu.h"
#include "dibr_ocl.h"
#include "yuv.h"

// Options -  We need a better way to do that
int  width = 1920, height = 1080;
bool depth_filter = true;
bool must_update = false, paused = false;
bool enable_hole_filling = true;
bool enable_dist = true;
bool output_video = false;
int  is_stereo = true;
int  use_opencl = false;
int  is_input_video = 0;
//end options

// Some global variables
Mat image, input, output;

// Some configuration sets
#define EYE_SEP_STEP 0.25
#define CONV_KERNEL_SIZE 9
#define YUV_INPUT 0

double sigmax = 10.0, sigmay = 90;  // assymetric gaussian filter
double conv_kernel [CONV_KERNEL_SIZE];
// end

using namespace std;

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
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help",          "produce this help message")
      ("fullscreen,f",  "use fullscreen mode")
      ("opencl,o",      "use opencl")
      ("stereo,s",      "generate stereo pair")
      ("width,w",       po::value<int>(), "window width")
      ("height,h",      po::value<int>(), "window height")
      ("input,i",       po::value<string>(), "input texture file")
      ("depth,d",       po::value<string>(), "input depth file")
      ;

  po::positional_options_description p;
  p.add("input", 1);
  p.add("depth", 2);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    cout << desc << "\n";
    return 1;
  }

  if (vm.count("width"))
    width = vm["width"].as<int>();

  if(vm.count("height"))
    height = vm["height"].as<int>();

  if(vm.count("opencl"))
    use_opencl = true;

  cout << width << "x" << height << endl;

#if YUV_INPUT
  FILE  *fin = NULL, *fin_depth = NULL;
  struct YUV_Capture cap, cap_depth;
  enum YUV_ReturnValue ret;
  IplImage *bgr, *bgr_depth;
  string filename_tex = vm["input"].as<string>();
  string filename_depth = vm["depth"].as<string>();

  fin = fopen (filename_tex.c_str(), "rb");
  if (!fin)
    fprintf (stderr, "error: unable to open file: %s\n", filename_tex.c_str());

  fin_depth = fopen (filename_depth.c_str(), "rb");

  if (!fin)
    fprintf (stderr, "error: unable to open file: %s\n", filename_depth.c_str());

  ret = YUV_init(fin, 1920, 1088, &cap);
  assert (ret == YUV_OK);

  ret = YUV_init(fin_depth, 1920, 1088, &cap_depth);
  assert (ret == YUV_OK);

  bgr = cvCreateImage(cvSize(1920, 1088), IPL_DEPTH_8U, 3);
  assert(bgr);

  bgr_depth = cvCreateImage(cvSize(1920, 1088), IPL_DEPTH_8U, 3);
  assert(bgr);

#else
  VideoCapture inputVideo (vm["input"].as<string> ()); // Open input
  inputVideo.set (CV_CAP_PROP_FPS, 10);

  if (!inputVideo.isOpened())
  {
    cout  << "Could not open the input video: " << vm["input"].as<string> ()
          << endl;
    return -1;
  }

  inputVideo >> image;

//  int ex = static_cast<int>(inputVideo.get(CV_CAP_PROP_FOURCC));

//  VideoWriter outputVideo;
//  if(vm.count('u'))
//  {
//    output_video = true;
//    outputVideo.open( vm['u'].as<string> (),
//                      CV_FOURCC('P', 'I', 'M', '1'),
//                      inputVideo.get(CV_CAP_PROP_FPS),
//                      Size(width, height), true);

//    cout << "create output video " << vm['u'].as<string> () << endl;
//  }
#endif

  // Get mime-type of input
  /* magic_t magic;
  const char *mime;
  magic = magic_open(MAGIC_MIME_TYPE);

  magic_load(magic, NULL);
  magic_compile(magic, NULL);
  mime = magic_file(magic, opts['i'].c_str()); */

  is_input_video = true;

  /* (strstr(mime, "image") == NULL);
  magic_close(magic); */

  // Creating image objects
  Mat color, depth, depth_filtered, depth_out, isHole, border, dist;

  input.create(height, width, CV_8UC(3));
  image.create(height, width, CV_8UC(3));
  // resize(image, input, input.size(), 0, 0, CV_INTER_CUBIC);

  int input_rows = input.rows;
  int input_cols = input.cols / 2;

  // if(is_stereo)
  //   input_cols /= 2;

  color.create (input_rows, input_cols, CV_8UC(3));
  depth.create (input_rows, input_cols, CV_8UC(1));
  depth_filtered.create (input_rows, input_cols, CV_8UC(3));
  border.create (input_rows, input_cols, CV_8UC(1));
  dist.create (input_rows, input_cols, CV_32F);

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

  // Output Window
  bool fullscreen = vm.count("fullscreen");
  cvNamedWindow("Output", CV_WINDOW_NORMAL);

  update_depth_shift_lookup_table ();

  //initialising opencl structures
  o.init();
  //loading the program and kernel source
  o.load_demo(&program, &kernel[0]);

  cout << "#Decode\tResize\tCrop\tFilter\tDIBR\tShow" << endl;

  for(;;)
  {
    // gettimeofday(&now, NULL);

    memset(pixelMutex, 0, input.cols * input.rows * sizeof(int));
    isHole.setTo(cv::Scalar(0));
    depth_out.setTo(cv::Scalar(0, 0, 0));
    output.setTo(cv::Scalar(255, 255, 255));

    if (must_update)
    {
      if(is_input_video && !paused)
      {
#if YUV_INPUT
        ret = YUV_read(&cap);
        if (ret == YUV_EOF)
        {
          cvWaitKey(0);
          break;
        }
        else if(ret == YUV_IO_ERROR)
        {
          fprintf(stderr, "I/O error\n");
          break;
        }
        cvCvtColor(cap.ycrcb, bgr, CV_YCrCb2BGR);

        ret = YUV_read(&cap_depth);
        if (ret == YUV_EOF)
        {
          cvWaitKey(0);
          break;
        }
        else if(ret == YUV_IO_ERROR)
        {
          fprintf(stderr, "I/O error\n");
          break;
        }

        cvCvtColor(cap_depth.ycrcb, bgr_depth, CV_YCrCb2RGB);

        Mat tmp_bgr (bgr);
        Mat tmp_bgr_depth (bgr_depth);
        Mat tmp_bgr_depth_resized;
        tmp_bgr_depth_resized.create(depth.cols, depth.rows, CV_8UC(3));

        resize(tmp_bgr, color, color.size(), 0, 0, CV_INTER_CUBIC);
        resize(tmp_bgr_depth, tmp_bgr_depth_resized, depth.size(), 0, 0, CV_INTER_CUBIC);
        cvtColor(tmp_bgr_depth_resized, depth, CV_RGB2GRAY);

        // resize(image, tmp_image, cvSize(width, height), 0, 0, CV_INTER_CUBIC);
#else
        inputVideo >> image;
#endif
      }


//      gettimeofday(&end, NULL);
//      diff = timeval_subtract(&result, &end, &now);
//      cout << (float)diff << "\t";

//      gettimeofday(&now, NULL);
      resize(image, input, input.size(), 0, 0, INTER_NEAREST);
//      gettimeofday(&end, NULL);
//      diff = timeval_subtract(&result, &end, &now);
//      cout << (float)diff << "\t";

//      gettimeofday(&now, NULL);
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

      /* detect_border(depth, border);
      imshow("border", border); */

      /*
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
      */

//      imshow("image", color);
//      imshow("depth", depth);

      //running parallel program
      // gettimeofday(&now, NULL);

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

      // gettimeofday(&end, NULL);
      // diff = timeval_subtract(&result, &end, &now);
      // cout << (float)diff << "\t";

      if(fullscreen)
        cvSetWindowProperty("Output", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
      imshow("Output", output);

      must_update = false;

      /* if (output_video)
      {
        outputVideo.write(output);
      } */
    }

    //gettimeofday(&now, NULL);
    int key = cvWaitKey(1);
    /*gettimeofday(&end, NULL);
    diff = timeval_subtract(&result, &end, &now);
    cout << (float)diff << endl;
    */
    // cout << "eye_sep:" << (float) eye_sep << endl;

    if(!handle_key(key)) // It will return false if key is ESC
        break;
  }

  printf ("Mean: %.3f fps\n", (total_frames / total_time) * 1000.0);
#if YUV_INPUT
  fclose(fin);
#endif

  delete pixelMutex;
  o.destroy();
}
