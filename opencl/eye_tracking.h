//OpenCV header files
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include <opencv2/objdetect/objdetect.hpp>

#define ENABLE_EYE_TRACKING 0

#if ENABLE_EYE_TRACKING
cv::CascadeClassifier face_cascade;
// cv::CascadeClassifier eye_cascade;
/**
* Function to detect human face and the eyes from an image.
*
* @param im The source image
* @param tpl Will be filled with the eye template, if detection success.
* @param rect Will be filled with the bounding box of the eye
* @return zero=failed, nonzero=success
*/
int detectEye(cv::Mat& im, cv::Mat& tpl, cv::Rect& rect)
{
  std::vector<cv::Rect> faces, eyes;
  face_cascade.detectMultiScale(im, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(30,30));

  if (faces.size())
  {
    rect = faces[0];
    tpl = im(rect);
  }
/*
  for (int i = 0; i < faces.size(); i++)
  {
    cv::Mat face = im(faces[i]);
    eye_cascade.detectMultiScale(face, eyes, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(20,20));

    if (eyes.size())
    {
      rect = eyes[0] + cv::Point(faces[i].x, faces[i].y);
      tpl = im(rect);
    }
  }
  return eyes.size();
*/
  return faces.size();
}

/**
* Perform template matching to search the user's eye in the given image.
*
* @param im The source image
* @param tpl The eye template
* @param rect The eye bounding box, will be updated with the new location of the eye
*/
void trackEye(cv::Mat& im, cv::Mat& tpl, cv::Rect& rect)
{
  cv::Size size(rect.width * 2, rect.height * 2);
  cv::Rect window(rect + size - cv::Point(size.width/2, size.height/2));

  window &= cv::Rect(0, 0, im.cols, im.rows);

  cv::Mat dst(window.width - tpl.rows + 1, window.height - tpl.cols + 1, CV_32FC1);
  cv::matchTemplate(im(window), tpl, dst, CV_TM_SQDIFF_NORMED);

  double minval, maxval;
  cv::Point minloc, maxloc;
  cv::minMaxLoc(dst, &minval, &maxval, &minloc, &maxloc);

  if (minval <= 0.2)
  {
    rect.x = window.x + minloc.x;
    rect.y = window.y + minloc.y;
  }
  else
    rect.x = rect.y = rect.width = rect.height = 0;
}
#endif
