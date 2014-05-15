#include <math.h>

#define PI 3.141592653589793

double gaussian ( int x, int y,
                  double sigma1, double sigma2 )
{
  return exp ( - ((x)*(x))/(2.0*sigma1) - ((y)*(y))/(2.0*sigma2));
}

void gaussian_kernel( double *gauss_kernel,
                      int kernel_size,
                      double sigmaX, double sigmaY)
{
    double sum = 0;
    for (int x = 0; x < kernel_size; x++)
    {
      for (int y = 0; y < kernel_size; y++)
      {
        int x1 = x - kernel_size/2;
        int y1 = y - kernel_size/2;
        gauss_kernel[x][y] = oriented_gaussian(x1, y1, sigmaX, sigmaY);
        printf ("(%d, %d) ", x1, y1);
        printf ("%.5f ", gauss_kernel[x][y]);
        sum += gauss_kernel[x][y];
      }
      // printf ("\n");
    }

    for (int x = 0; x < kernel_size; x++)
      for (int y = 0; y < kernel_size; y++)
        gauss_kernel[x][y] /= sum;
}
