/****** Initilization Header - Setup & Prepares Key Game Items ******/

#include <stdio.h>
#include <math.h>

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#include "SDL/SDL_image.h"

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;
const int SCREEN_BPP = 32;

SDL_Event event;
#define  RADDEG  57.29577951f

/* Crop a SDL_Surface */
SDL_Surface* crop_surface( SDL_Surface* sprite_sheet,
                           int x, int y,
                           int width, int height);

/* Warp left and right image and holle-filling */
bool shift_surface ( SDL_Surface *image_color,
                     SDL_Surface *image_depth,
                     SDL_Surface *left_color,
                     SDL_Surface *right_color );

namespace var
{
  int frame_start_time = 0;
  int frame_current_time = 0;
  int frame_count = 0;
}

/* Initializes Core OpenGL Features */
bool opengl_init()
{
  glEnable( GL_TEXTURE_2D );

  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
  glViewport( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  glClear( GL_COLOR_BUFFER_BIT );

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();

  glOrtho(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, -1.0f, 1.0f);

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  return true;
}

/* Initializes SDL, OpenGL and video and window */
bool init()
{
  if( SDL_Init(SDL_INIT_EVERYTHING) < 0 )
  {
    return false;
  }

  if (SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP,
                        SDL_FULLSCREEN | SDL_OPENGL) == NULL )
  {
    return false;
  }

  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 ); // *new*

  if( opengl_init() == false )
  {
    return false;
  }

  SDL_WM_SetCaption("SDL-OpenGL Graphics : Part IV - 3D Shapes", NULL);

  return true;
}

/* Removes objects before game closes */
void clean_up()
{
  SDL_Quit();
}


int main(int argc, char* argv[])
{
  GLfloat rotation = 0.0f;

  bool quit = false;

  if(init() == false)
  {
    return 1;
  }

  GLuint TextureID = 0;
  SDL_Surface *image = IMG_Load(argv[1]);
  SDL_Surface *image_color = crop_surface( image,
                                           0, 0,
                                           image->w/2, image->h);

  SDL_Surface *image_depth = crop_surface( image,
                                           image->w/2, 0,
                                           image->w/2, image->h);

  SDL_Surface *left_color, *right_color, *stereo_color;

  /* Create a 32-bit surface with the bytes of each pixel in R,G,B,A order,
     as expected by OpenGL for textures */
  left_color = SDL_CreateRGBSurface( image_color->flags,
                                     image_color->w, image_color->h,
                                     image_color->format->BitsPerPixel,
                                     image_color->format->Rmask,
                                     image_color->format->Gmask,
                                     image_color->format->Bmask,
                                     image_color->format->Amask );

  /* Create a 32-bit surface with the bytes of each pixel in R,G,B,A order,
     as expected by OpenGL for textures */
  right_color = SDL_CreateRGBSurface( image_color->flags,
                                      image_color->w, image_color->h,
                                      image_color->format->BitsPerPixel,
                                      image_color->format->Rmask,
                                      image_color->format->Gmask,
                                      image_color->format->Bmask,
                                      image_color->format->Amask );

  /* Create a 32-bit surface with the bytes of each pixel in R,G,B,A order,
        as expected by OpenGL for textures */
  stereo_color = SDL_CreateRGBSurface( image_color->flags,
                                       image->w, image->h,
                                       image_color->format->BitsPerPixel,
                                       image_color->format->Rmask,
                                       image_color->format->Gmask,
                                       image_color->format->Bmask,
                                       image_color->format->Amask );

  shift_surface(image_color, image_depth, left_color, right_color);
  SDL_BlitSurface (left_color, NULL, stereo_color, NULL);

  SDL_Rect dest;
  dest.x = left_color->w;
  dest.y = 0;
  dest.w = right_color->w;
  dest.h = right_color->h;
  SDL_BlitSurface (right_color, NULL, stereo_color, &dest);

  glEnable( GL_TEXTURE_2D );
  glGenTextures(1, &TextureID);
  glBindTexture(GL_TEXTURE_2D, TextureID);

  int Mode = GL_RGB;

  if(image->format->BytesPerPixel == 4) {
    Mode = GL_RGBA;
  }

  glTexImage2D( GL_TEXTURE_2D,
                0, Mode,
                image->w, image->h, 0,
                Mode, GL_UNSIGNED_BYTE, stereo_color->pixels);
  // glTexImage2D(GL_TEXTURE_2D, 0, Mode, image->w, image->h, 0, Mode, GL_UNSIGNED_BYTE, image->pixels);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


  /****** Main Game Loop ******/
  while(quit == false)
  {

    /****** Get Most Current Time ******/
    var::frame_start_time = SDL_GetTicks();

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
    glVertex3f( SCREEN_WIDTH, 0.f, 0.f );

    //Top-right vertex (corner)
    glTexCoord2i( 1, 1 );
    glVertex3f( SCREEN_WIDTH, SCREEN_HEIGHT, 0.f );

    //Top-left vertex (corner)
    glTexCoord2i( 0, 1 );
    glVertex3f( 0.f, SCREEN_HEIGHT, 0.f );
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
          break;
      }
    }

    /****** Update Screen And Frame Counts ******/
    SDL_GL_SwapBuffers();
    var::frame_count++;
    var::frame_current_time = SDL_GetTicks();

    /****** Frame Rate Handle ******/
    if((var::frame_current_time - var::frame_start_time) < (1000/60))
    {
      var::frame_count = 0;
      SDL_Delay((1000/60) - (var::frame_current_time - var::frame_start_time));
    }
  }

  clean_up();

  return 0;
}

SDL_Surface* crop_surface( SDL_Surface* sprite_sheet,
                           int x, int y,
                           int width, int height )
{
  SDL_Surface* surface = SDL_CreateRGBSurface( sprite_sheet->flags,
                                               width, height,
                                               sprite_sheet->format->BitsPerPixel,
                                               sprite_sheet->format->Rmask,
                                               sprite_sheet->format->Gmask,
                                               sprite_sheet->format->Bmask,
                                               sprite_sheet->format->Amask );
  SDL_Rect rect = {x, y, width, height};
  SDL_BlitSurface(sprite_sheet, &rect, surface, 0);
  return surface;
}

Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

  switch(bpp) {
    case 1:
      return *p;
      break;

    case 2:
      return *(Uint16 *)p;
      break;

    case 3:
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;
      break;

    case 4:
      return *(Uint32 *)p;
      break;

    default:
      return 0;       /* shouldn't happen, but avoids warnings */
  }
}

void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to set */
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

  switch(bpp) {
    case 1:
      *p = pixel;
      break;

    case 2:
      *(Uint16 *)p = pixel;
      break;

    case 3:
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
      {
        p[0] = (pixel >> 16) & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = pixel & 0xff;
      }
      else
      {
        p[0] = pixel & 0xff;
        p[1] = (pixel >> 8) & 0xff;
        p[2] = (pixel >> 16) & 0xff;
      }
      break;

    case 4:
      *(Uint32 *)p = pixel;
      break;
  }
}

void get_YUV(Uint8 r, Uint8 g, Uint8 b, int &Y, int &U, int &V)
{
  Y = 0.299*r + 0.587*g + 0.114*b;
  U = (b-Y)*0.565;
  V = (r-Y)*0.713;
}

/* DIBR STARTS HERE */
int find_shiftMC3(int depth, int Ny)
{
  int h;
  int nkfar = 128, nknear = 128, kfar = 0, knear = 0;
  int n_depth = 256;  // Number of depth planes
  int eye_sep = 6;    // eye separation 6cm

  // This is a display dependant parameter and the maximum shift depends
  // on this value. However, the maximum disparity should not exceed
  // particular threshold, defined by phisiology of human vision.
  int Npix = 300; // TODO: Make this adjustable in the interface.
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


bool shift_surface ( SDL_Surface *image_color, SDL_Surface *image_depth,
                     SDL_Surface *left_image, SDL_Surface *right_image )
{

  SDL_FillRect(left_image, NULL, 0x000000);
  if(left_image == NULL) {
    fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
    exit(1);
  }

  // This value is half od the maximun shift
  // Maximun shift comes at depth == 0
  int S = 20; // 58
  int N = 256; // Number of depth-planes
  int depth = 0;
  int depth_shift_table_lookup[N];
  int cols = image_color->w, rows = image_color->h;
  bool mask [rows][cols];
  memset (mask, false, rows*cols*sizeof(bool));

  for(int i = 0; i < N; i++)
  {
    depth_shift_table_lookup[i] = find_shiftMC3(i, N);
  }

  // Calculate left image
  // for every pixel change its value
  for (int y = 0; y < rows; y++)
  {
    for (int x = cols-1; x >= 0; --x)
    {
      // get depth
      Uint32 pixel = getpixel(image_depth, x, y);
      Uint8 r, g, b;
      SDL_GetRGB (pixel, image_depth->format, &r, &g, &b);
      int Y, U, V;
      get_YUV(r, g, b, Y, U, V);
      int D = Y;
      int shift = depth_shift_table_lookup [D];

      if( x + S - shift < cols)
      {
        putpixel (left_image, x + S-shift, y, getpixel (image_color, x, y) );
        mask [y][x+S-shift] = 1;
      }

      // putpixel (left_image, x, y, getpixel (image_color, x, y) );
    }

    for (int x = 1; x < cols; x++)
    {
      if ( mask[y][x] == 0 )
      {
        if ( x - 7 < 0)
        {
          putpixel (left_image, x, y, getpixel(image_color, x, y));
        }
        else
        {
          Uint32 r_sum = 0, g_sum = 0, b_sum = 0;
          for (int x1 = x-7; x1 <= x-4; x1++)
          {
            Uint32 pixel = getpixel(left_image, x1, y);
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
          Uint32 pixel_new = SDL_MapRGB(left_image->format, r_new, g_new, b_new);
          putpixel (left_image, x, y, pixel_new );
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
      Uint32 pixel = getpixel(image_depth, x, y);
      Uint8 r, g, b;
      SDL_GetRGB (pixel, image_depth->format, &r, &g, &b);
      int Y, U, V;
      get_YUV(r, g, b, Y, U, V);
      int D = Y;
      int shift = depth_shift_table_lookup [D];

      if( x + shift - S >= 0 )
      {
        putpixel (right_image, x + shift - S, y, getpixel (image_color, x, y) );
        mask [y][x+shift-S] = 1;
      }

      // putpixel (left_image, x, y, getpixel (image_color, x, y) );
    }

    for (int x = cols-1 ; x >= 0; --x)
    {
      if ( mask[y][x] == 0 )
      {
        if ( x + 7 > cols - 1)
        {
          putpixel (right_image, x, y, getpixel(image_color, x, y));
        }
        else
        {
          Uint32 r_sum = 0, g_sum = 0, b_sum = 0;
          for (int x1 = x+4; x1 <= x+7; x1++)
          {
            Uint32 pixel = getpixel(right_image, x1, y);
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
          Uint32 pixel_new = SDL_MapRGB(right_image->format, r_new, g_new, b_new);
          putpixel (right_image, x, y, pixel_new );
        }

      }

    }
  }

  return true;
}

