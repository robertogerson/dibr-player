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
#ifndef __SDL_AUX_H__
#define __SDL_AUX_H__
#include <stdio.h>
#include <stdlib.h>

#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
#include "SDL/SDL_image.h"

// need to create get_RGB(oposite to get_YUV)  if going to put back in
// image_depth2 FCI****
// need to store values of UV if goint to put pixel back 
// get precise constants FCI***
void get_RGB( int Y, int U, int V, Uint8 &r, Uint8 &g, Uint8 &b)
{
  r = 1.0*Y +     0*U + 1.140*V;
  g = 1.0*Y - 0.395*U - 0.581*V;
  b = 1.0*Y + 2.032*U +     0*V;
}

void get_YUV(Uint8 r, Uint8 g, Uint8 b, int &Y, int &U, int &V)
{
  Y = 0.299*r + 0.587*g + 0.114*b;
  U = (b-Y)*0.565;
  V = (r-Y)*0.713;
} 

/*
 * SDL interprets each pixel as a 32-bit number, so our masks must depend on the
 * endianness (byte order) of the machine.
 */
void sdl_get_pixel_mask( Uint32 &rmask,
                         Uint32 &gmask,
                         Uint32 &bmask,
                         Uint32 &amask )
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  rmask = 0xff000000;
  gmask = 0x00ff0000;
  bmask = 0x0000ff00;
  amask = 0x000000ff;
#else
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
  amask = 0xff000000;
#endif
}

SDL_Surface* sdl_crop_surface( SDL_Surface* orig,
                           SDL_Surface* dest,
                           int x, int y,
                           unsigned int width, unsigned int height )
{
  SDL_Rect rect = {x, y, width, height};
  SDL_BlitSurface(orig, &rect, dest, 0);
  return dest;
}

Uint32 sdl_get_pixel(SDL_Surface *surface, int x, int y)
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

void sdl_put_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
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

/*
 * Creates an RGB surface using the same parameters as surf. Width and height,
 * however, can be different.
 */
SDL_Surface *sdl_create_RGB_surface(SDL_Surface *surf, int w, int h)
{
  return SDL_CreateRGBSurface( surf->flags, w, h,
                               surf->format->BitsPerPixel,
                               surf->format->Rmask,
                               surf->format->Gmask,
                               surf->format->Bmask,
                               surf->format->Amask );

}
#endif
