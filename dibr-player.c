/* This file is part of dibr-player.
 *
 * dibr-player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dibr-player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
/*************** DIBR Overview *************************************************
 * Depth-image-based rendering is the process of genereting virtual views from
 * a set of original view and associated depth frame. In special, this project
 * is intended to generate a stereo-pair from a reference texture. The following
 * 'image' shows schematically the process.
 *   ___________ ___________
 *  |           |           |
 *  |  Texture  |   Depth   |------------ INPUT
 *  |___________|___________|
 *        |           |
 *        |      _____|_____
 *        |     |           |
 *        |     |   Depth   |
 *        |     | Filtering |
 *        |     |___________|
 *        |___________|
 *              |
 *        _____\|/______
 *       |              |
 *       |  3D Warping  |
 *       |______________|
 *              |
 *   __________\|/__________
 *  |           |           |
 *  |    Left   |   Right   |------------ OUTPUT
 *  |___________|___________|
 ******************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm> //min,max

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#include "SDL/SDL_image.h"

#include <vlc/vlc.h>

#include "sdl_aux.h"

#define PI 3.141592653589793
#define GAUSSIAN_KERNEL_SIZE 9
#define RADDEG  57.29577951f

struct user_params
{
  char *file_path;

  /* Screen related params */
  int screen_width;
  int screen_height;
  int screen_bpp;
  bool fullscreen;

  /* DIBR related params */
  bool hole_filling;
  bool paused;
  bool depth_filter;
  bool show_all;

  /* Gaussian filter parameters */
  double sigmax;
  double sigmay;
};

double gaussian_kernel[GAUSSIAN_KERNEL_SIZE][GAUSSIAN_KERNEL_SIZE];

/***************** Depth filtering related functions **************************/
double gaussian (double x, double mu, double sigma)
{
  return exp ( -(((x-mu)/(sigma))*((x-mu)/(sigma)))/2.0 );
}

double oriented_gaussian (int x, int y,
                          double sigma1, double sigma2)
{
  return exp ( - ((x)*(x))/(2.0*sigma1) - ((y)*(y))/(2.0*sigma2));
}

void calc_gaussian_kernel(double sigmax, double sigmay)
{
  double sum = 0;
  for (int x = 0; x < GAUSSIAN_KERNEL_SIZE; x++)
  {
    for (int y = 0; y < GAUSSIAN_KERNEL_SIZE; y++)
    {
      int x1 = x - GAUSSIAN_KERNEL_SIZE/2;
      int y1 = y - GAUSSIAN_KERNEL_SIZE/2;
      gaussian_kernel[x][y] = oriented_gaussian(x1, y1, sigmax, sigmay);
      sum += gaussian_kernel[x][y];
    }
  }

  for (int x = 0; x < GAUSSIAN_KERNEL_SIZE; x++)
    for (int y = 0; y < GAUSSIAN_KERNEL_SIZE; y++)
      gaussian_kernel[x][y] /= sum;
}

SDL_Surface* filter_depth( SDL_Surface* depth_frame,
                           SDL_Surface* depth_frame_filtered,
                           int x, int y,
                           unsigned int width, unsigned int height)
{
  int cols = width;
  int rows = height;

  int neighborX, neighborY;
  Uint8 r, g, b;
  Uint8 r_new, g_new, b_new;
  float r_sum, g_sum, b_sum;

  for (int y = GAUSSIAN_KERNEL_SIZE-1; y < rows - GAUSSIAN_KERNEL_SIZE-1; y++)
  {
    for (int x = GAUSSIAN_KERNEL_SIZE-1; x < cols-GAUSSIAN_KERNEL_SIZE-1; x++)
    {
      r_sum = 0;
      g_sum = 0;
      b_sum = 0;

      for (int filterX = 0; filterX < GAUSSIAN_KERNEL_SIZE; filterX++)
      {
        for (int filterY = 0; filterY < GAUSSIAN_KERNEL_SIZE; filterY++)
        {
          // get original depth around point being calculated
          neighborX = x - 1 + filterX;
          neighborY = y - 1 + filterY;

          // get neighbor pixels
          Uint32 pixel = sdl_get_pixel(depth_frame, neighborX, neighborY);
          SDL_GetRGB (pixel, depth_frame->format, &r, &g, &b);

          r_sum += r * gaussian_kernel[filterX][filterY];
          g_sum += g * gaussian_kernel[filterX][filterY];
          b_sum += b * gaussian_kernel[filterX][filterY];
          // OK to apply Guassian filter to RGB values instead of Y value of YUV?
          // testing Ybefore and Yafter suggests its ok
        }

        r_new = std::min(std::max(int( r_sum ), 0), 255); // not using bias or factor
        g_new = std::min(std::max(int( g_sum ), 0), 255);
        b_new = std::min(std::max(int( b_sum ), 0), 255);

        // store result in original position in depth_frame_filtered
        Uint32 pixel_new = SDL_MapRGB( depth_frame_filtered->format,
                                       r_new, g_new, b_new );
        sdl_put_pixel (depth_frame_filtered, x, y, pixel_new );
      }
    }
  }
  return depth_frame_filtered;
}

/****************** 3D Warping related functions ******************************/
int find_shiftMC3(int depth, int Ny)
{
  int h;
  int nkfar = 128, nknear = 128, kfar = 0, knear = 0;
  int n_depth = 256;  // Number of depth planes
  int eye_sep = 6;    // eye separation 6cm

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

bool shift_surface ( user_params &p,
                     SDL_Surface *image_color,
                     SDL_Surface *depth_frame,
                     SDL_Surface *depth_frame_filtered,
                     SDL_Surface *left_image,
                     SDL_Surface *right_image,
                     int S = 58)
{

  SDL_FillRect(left_image, NULL, 0xFFFFFF);
  SDL_FillRect(right_image, NULL, 0xFFFFFF);

  // This value is half od the maximun shift
  // Maximun shift comes at depth == 0
  int N = 256; // Number of depth-planes
  // int S = 58;
  int depth_shift_table_lookup[N];
  int cols = image_color->w, rows = image_color->h;
  bool mask [rows][cols];
  memset (mask, false, rows*cols*sizeof(bool));

  // \fixme remove from here
  for(int i = 0; i < N; i++)
    depth_shift_table_lookup[i] = find_shiftMC3(i, N);

  if (p.depth_filter)
  {
    filter_depth( depth_frame,
                  depth_frame_filtered,
                  cols, rows,
                  depth_frame->w, depth_frame->h);
  };

  // Calculate left image
  for (int y = 0; y < rows; y++)
  {
    for (int x = cols-1; x >= 0; --x)
    {
      // get depth
      // depth_frame_filtered after filter applied FCI
      Uint32 pixel = sdl_get_pixel(depth_frame_filtered, x, y);
      Uint8 r, g, b;
      SDL_GetRGB (pixel, depth_frame_filtered->format, &r, &g, &b);
      int Y, U, V;
      get_YUV(r, g, b, Y, U, V);
      int D = Y;
      int shift = depth_shift_table_lookup [D];

      if( x + S - shift < cols)
      {
        sdl_put_pixel( left_image, x + S-shift, y,
                       sdl_get_pixel (image_color, x, y) );
        mask [y][x+S-shift] = 1;
      }
    }

    if(p.hole_filling)
    {
      for (int x = 1; x < cols; x++)
      {
        if ( mask[y][x] == 0 )
        {
          if ( x - 7 < 0)
          {
            sdl_put_pixel (left_image, x, y, sdl_get_pixel(image_color, x, y));
          }
          else
          {
            Uint32 r_sum = 0, g_sum = 0, b_sum = 0;
            for (int x1 = x-7; x1 <= x-4; x1++)
            {
              Uint32 pixel = sdl_get_pixel(left_image, x1, y);
              Uint8 r, g, b;
              SDL_GetRGB (pixel, left_image->format, &r, &g, &b);
              r_sum += r;
              g_sum += g;
              b_sum += b;
            }

            Uint8 r_new, g_new, b_new;
            r_new = (Uint8)(r_sum / 4);
            g_new = (Uint8)(g_sum / 4);
            b_new = (Uint8)(b_sum / 4);
            Uint32 pixel_new = SDL_MapRGB(left_image->format,
                                          r_new,
                                          g_new,
                                          b_new);
            sdl_put_pixel (left_image, x, y, pixel_new );
          }
        }
      }
    }
  }

  // Calculate right image
  memset (mask, false, rows*cols*sizeof(bool));
  for (int y = 0; y < rows; y++)
  {
    for (int x = 0; x < cols; x++)
    {
      // get depth
      // depth_frame_filtered after filter FCI
      Uint32 pixel = sdl_get_pixel(depth_frame_filtered, x, y);
      Uint8 r, g, b;
      SDL_GetRGB (pixel, depth_frame_filtered->format, &r, &g, &b);
      int Y, U, V;
      get_YUV(r, g, b, Y, U, V);
      int D = Y;
      int shift = depth_shift_table_lookup [D];

      if( x + shift - S >= 0 )
      {
        sdl_put_pixel ( right_image, x + shift - S, y,
                        sdl_get_pixel (image_color, x, y) );
        mask [y][x+shift-S] = 1;
      }
    }

    if(p.hole_filling)
    {
      for (int x = cols-1 ; x >= 0; --x)
      {
        if ( mask[y][x] == 0 )
        {
          if ( x + 7 > cols - 1)
          {
            sdl_put_pixel (right_image, x, y, sdl_get_pixel(image_color, x, y));
          }
          else
          {
            Uint32 r_sum = 0, g_sum = 0, b_sum = 0;
            for (int x1 = x+4; x1 <= x+7; x1++)
            {
              Uint32 pixel = sdl_get_pixel(right_image, x1, y);
              Uint8 r, g, b;
              SDL_GetRGB (pixel, right_image->format, &r, &g, &b);
              r_sum += r;
              g_sum += g;
              b_sum += b;
            }

            Uint8 r_new, g_new, b_new;
            r_new = (Uint8)(r_sum / 4);
            g_new = (Uint8)(g_sum / 4);
            b_new = (Uint8)(b_sum / 4);
            Uint32 pixel_new = SDL_MapRGB(  right_image->format,
                                            r_new,
                                            g_new,
                                            b_new );
            sdl_put_pixel (right_image, x, y, pixel_new );
          }
        }
      }
    }

  }
  return true;
}

/********************* VLC related functions **********************************/
struct vlc_sdl_ctx
{
  SDL_Surface *surf;
  SDL_mutex *mutex;

  libvlc_instance_t *libvlc;
  libvlc_media_t *m;
  libvlc_media_player_t *mp;

  int frame_start_time;
  int frame_current_time;
  int frame_count ;
};

static void *lock(void *data, void **p_pixels)
{
  struct vlc_sdl_ctx *ctx = (struct vlc_sdl_ctx*)data;

  SDL_LockMutex(ctx->mutex);
  SDL_LockSurface(ctx->surf);
  *p_pixels = ctx->surf->pixels;
  return NULL; /* picture identifier, not needed here */
}

static void unlock(void *data, void *id, void *const *p_pixels)
{
  struct vlc_sdl_ctx *ctx = (struct vlc_sdl_ctx*)data;

  SDL_UnlockSurface(ctx->surf);
  SDL_UnlockMutex(ctx->mutex);

  assert(id == NULL); /* picture identifier, not needed here */
}

static void display(void *data, void *id)
{
  /* VLC wants to display the video */
  (void) data;
  assert(id == NULL);
}
/* end libVLC */

/*
 * Initializes Core OpenGL Features.
 */
bool opengl_init(user_params &p)
{
  glEnable( GL_TEXTURE_2D );

  glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
  glViewport( 0, 0, p.screen_width, p.screen_height);

  glClear( GL_COLOR_BUFFER_BIT );

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();

  glOrtho(0.0f, p.screen_width, p.screen_height, 0.0f, -1.0f, 1.0f);

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  return true;
}

/*
 * Initializes SDL, OpenGL and video window.
 */
bool init(user_params &p, vlc_sdl_ctx &ctx)
{
  Uint32 rmask, gmask, bmask, amask;

  /* initialize SDL */
  if( SDL_Init(SDL_INIT_EVERYTHING) < 0 )
    return false;

  if (SDL_SetVideoMode( p.screen_width,
                        p.screen_height,
                        p.screen_bpp,
                        /*SDL_FULLSCREEN |*/ SDL_OPENGL) == NULL )
    return false;

  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 ); // *new*

  if( opengl_init(p) == false )
    return false;

  SDL_WM_SetCaption("DIBR player -- Guassian Filter", NULL);

  /* ctx initialization */
  ctx.mutex = SDL_CreateMutex();

  sdl_get_pixel_mask(rmask, gmask, bmask, amask);
  ctx.surf  = SDL_CreateRGBSurface( SDL_SWSURFACE,
                                    p.screen_width,
                                    p.screen_height,
                                    32,
                                    rmask,
                                    gmask,
                                    bmask,
                                    0 );

  /* Initialise libVLC    */
  char const *vlc_argv[] =
  {
    "--no-audio", /* skip any audio track */
    "--no-xlib"
  };
  int vlc_argc = sizeof(vlc_argv) / sizeof(*vlc_argv);
  ctx.libvlc = libvlc_new(vlc_argc, vlc_argv);
  printf ("%s\n", p.file_path);
  ctx.m = libvlc_media_new_path(ctx.libvlc, p.file_path);
  ctx.mp = libvlc_media_player_new_from_media(ctx.m);
  libvlc_media_release(ctx.m);

  libvlc_video_set_callbacks(ctx.mp, lock, unlock, display, &ctx);
  libvlc_video_set_format( ctx.mp,
                           "RGBA",
                           p.screen_width,
                           p.screen_height,
                           p.screen_width*4 );

  return true;
}

/* Removes objects before closes */
void clean_up(vlc_sdl_ctx &ctx)
{
  /* Stop stream and clean up libVLC */
  libvlc_media_player_stop(ctx.mp);
  libvlc_media_player_release(ctx.mp);
  libvlc_release(ctx.libvlc);

  /* Close window and clean up libSDL */
  SDL_DestroyMutex(ctx.mutex);
  SDL_FreeSurface(ctx.surf);

  /* Stop SDL */
  SDL_Quit();
}

void set_default_params(user_params &p)
{
  p.screen_width = 1024;
  p.screen_height = 768;
  p.screen_bpp = 32;
  p.fullscreen = false;

  p.hole_filling = false;
  p.paused = true;
  p.depth_filter = true;
  p.show_all = false;

  /* Gaussian filter parameters */
  p.sigmax = 500.0;
  p.sigmay = 10; /* assymetric gaussian filter */
}

int main(int argc, char* argv[])
{
  user_params p;
  struct vlc_sdl_ctx ctx;

  bool quit = false;
  SDL_Event event;

  int S = 20; //58;

  // Handling parameters
  set_default_params(p);
  //TODO: parse_opts(argc, argv);

  if(argc < 2)
  {
      printf("Usage: %s <filename>\n", argv[0]);
      return EXIT_FAILURE;
  }

  p.file_path = argv[1]; // path to video/image.

  if(init(p, ctx) == false)
    return 1;

  SDL_Surface *image = ctx.surf;
  SDL_Surface *image_all = sdl_create_RGB_surface(image, image->w, image->h*2 );
  SDL_Surface *image_color = sdl_create_RGB_surface(image, image->w/2, image->h);
  sdl_crop_surface( image, image_color, 0, 0, image->w/2, image->h );

  SDL_Surface *depth_frame = sdl_create_RGB_surface(image, image->w/2, image->h);
  sdl_crop_surface( image, depth_frame, image->w/2, 0, image->w/2, image->h);

  SDL_Surface *depth_frame_filtered = sdl_create_RGB_surface( image, image->w/2, image->h);
  sdl_crop_surface( image, depth_frame_filtered, image->w/2, 0, image->w/2, image->h);


  /* Create a 32-bit surface with the bytes of each pixel in R,G,B,A order, as
   * expected by OpenGL for textures */
  SDL_Surface *left_color = sdl_create_RGB_surface(image_color, image_color->w, image_color->h);
  SDL_Surface *right_color = sdl_create_RGB_surface( image_color, image_color->w, image_color->h);
  SDL_Surface *stereo_color = sdl_create_RGB_surface( image_color, image->w, image->h);

  GLuint TextureID = 0;
  /* Generate the openGL textures */
  glEnable( GL_TEXTURE_2D );
  glGenTextures(1, &TextureID);
  glBindTexture(GL_TEXTURE_2D, TextureID);

  int Mode = GL_RGB;
  if(image->format->BytesPerPixel == 4)
    Mode = GL_RGBA;

  calc_gaussian_kernel(p.sigmax, p.sigmay);

  libvlc_media_player_play(ctx.mp);
  ctx.frame_count = 0;

  /****** Main Loop ******/
  while(quit == false)
  {
    SDL_LockMutex(ctx.mutex);
    sdl_crop_surface( image, image_color, 0, 0, image->w/2, image->h);
    sdl_crop_surface( image, depth_frame, image->w/2, 0, image->w/2, image->h);

    //FCI just have a copy to work with
    sdl_crop_surface( image,
                      depth_frame_filtered,
                      image->w/2,
                      0,
                      image->w/2,
                      image->h );
    SDL_UnlockMutex(ctx.mutex);

    // Generate stereo image
    // FCI depth_frame_filtered
    shift_surface( p, image_color, depth_frame,
                   depth_frame_filtered,
                   left_color,
                   right_color,
                   S );

    SDL_FillRect(stereo_color, NULL, 0x000000);
    SDL_BlitSurface (left_color, NULL, stereo_color, NULL);

    SDL_Rect dest;
    dest.x = left_color->w;
    dest.y = 0;
    dest.w = right_color->w;
    dest.h = right_color->h;
    SDL_BlitSurface (right_color, NULL, stereo_color, &dest);

    //Compose original and depth
    SDL_BlitSurface (stereo_color, NULL, image_all, NULL);
    dest.x = 0;
    dest.y = stereo_color->h;
    SDL_BlitSurface (image_color, NULL, image_all, &dest);
    dest.x = image_color->w;
    SDL_BlitSurface (depth_frame_filtered, NULL, image_all, &dest);

    SDL_Surface *surface_to_display = stereo_color;
    if (p.show_all)
      surface_to_display = image_all;

    glTexImage2D( GL_TEXTURE_2D,
                  0, Mode,
                  surface_to_display->w, surface_to_display->h, 0,
                  Mode, GL_UNSIGNED_BYTE, surface_to_display->pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /****** Get Most Current Time ******/
    ctx.frame_start_time = SDL_GetTicks();

    /****** Draw Rectangle ******/
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    // Bind the texture to which subsequent calls refer to
    glBindTexture( GL_TEXTURE_2D, TextureID);
      glBegin( GL_QUADS );
      //Bottom-left vertex (corner)
      glTexCoord2i( 0, 0 );
      glVertex3f( 0.f, 0.f, 0.0f );
      //Bottom-right vertex (corner)
      glTexCoord2i( 1, 0 );
      glVertex3f( p.screen_width, 0.f, 0.f );
      //Top-right vertex (corner)
      glTexCoord2i( 1, 1 );
      glVertex3f( p.screen_width, p.screen_height, 0.f );
      //Top-left vertex (corner)
      glTexCoord2i( 0, 1 );
      glVertex3f( 0.f, p.screen_height, 0.f );
    glEnd();

    /****** Check for Key & System input ******/
    while(SDL_PollEvent(&event))
    {
      /******  Application Quit Event ******/
      switch (event.type)
      {
        case SDL_KEYDOWN:
          if(event.key.keysym.sym == 27)
            quit = true;
          else if(event.key.keysym.sym == '=')
            S++;
          else if(event.key.keysym.sym == '-')
            S--;
          else if(event.key.keysym.sym == 'h')
          {
            p.hole_filling = !p.hole_filling;
            printf ("Hole filling: %d.\n", p.hole_filling);
          }
          else if(event.key.keysym.sym == 'f')
          {
            p.depth_filter = !p.depth_filter;     // filter toggle FCI****
            printf ("Depth filter: %d.\n", p.depth_filter);
          }
          else if(event.key.keysym.sym == SDLK_SPACE)
          {
            p.paused = !p.paused;
            libvlc_media_player_set_pause(ctx.mp, p.paused);
          }
          else if(event.key.keysym.sym == 'a')
          {
            p.show_all = !p.show_all;
            printf ("Show reference frames: %d.\n", p.show_all);
          }
          break;
      }
    }

    /****** Update Screen And Frame Counts ******/
    SDL_GL_SwapBuffers();
    ctx.frame_count++;
    ctx.frame_current_time = SDL_GetTicks();

    /****** Frame Rate Handle ******/
    if((ctx.frame_current_time - ctx.frame_start_time) < (1000/60))
    {
      ctx.frame_count = 0;
      SDL_Delay((1000/60) - (ctx.frame_current_time - ctx.frame_start_time));
    }
  }

  // Clean up everything
  clean_up(ctx);

  return 0;
}

