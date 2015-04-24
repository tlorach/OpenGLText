/* -*- Mode: C++; tab-width: 3; indent-tabs-mode: nil c-basic-offset: 3 -*- */
// vim:cindent:ts=3:sw=3:et:tw=80:sta:
/*************************************************************** gltext-cpr beg
 *
 * GLText - OpenGL TrueType Font Renderer
 * GLText is (C) Copyright 2002 by Ben Scott
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * -----------------------------------------------------------------
 * File:          $RCSfile: GLTextureGlyph.cpp,v $
 * Date modified: $Date: 2003/04/09 23:43:54 $
 * Version:       $Revision: 1.7 $
 * -----------------------------------------------------------------
 *
 ************************************************************ gltext-cpr-end */
#include <algorithm>
#include <string.h>
#include "GLTextureGlyph.h"

namespace gltext
{
   int nextPowerOf2(int n)
   {
      int i = 1;
      while (i < n)
      {
         i *= 2;
      }
      return i;
   }

   GLTextureGlyph::GLTextureGlyph(int offx, int offy,
                                  int width, int height, u8* data,
                                  bool mipmap)
      : mOffsetX(offx)
      , mOffsetY(offy)
      , mWidth(width)
      , mHeight(height)
   {
      // Make a texture sized at a power of 2.  Require them to be 8x8
      // to prevent some funky texture flickering rendering glitches.
#undef max
#undef min
      mTexWidth  = std::max(8, nextPowerOf2(mWidth));
      mTexHeight = std::max(8, nextPowerOf2(mHeight));

      // Get a unique texture name for our new texture
      glGenTextures(1, &mTexName);

      const int size = mTexWidth * mTexHeight;
      u8* pixels = new u8[size];
      memset(pixels, 0, size);
      for (int row = 0; row < mHeight; ++row)
      {
         memcpy(pixels + mTexWidth * row, data + mWidth * row, mWidth);
      }
      delete[] data;
      data = 0;

      // This is basically an expansion of pixels into Luminance/Alpha.
      u8* la = new u8[size * 2];
      for (int i = 0; i < size; ++i)
      {
         la[i * 2 + 0] = (pixels[i] ? 255 : 0);
         la[i * 2 + 1] = pixels[i];
      }
      delete[] pixels;
      pixels = 0;

      // Generate a 2D texture with our image data
      glBindTexture(GL_TEXTURE_2D, mTexName);
      if (mipmap)
      {
         gluBuild2DMipmaps(
            GL_TEXTURE_2D,
            2,
            mTexWidth,
            mTexHeight,
            GL_LUMINANCE_ALPHA,
            GL_UNSIGNED_BYTE,
            la);
      }
      else
      {
         glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_LUMINANCE_ALPHA,
            mTexWidth,
            mTexHeight,
            0,
            GL_LUMINANCE_ALPHA,
            GL_UNSIGNED_BYTE,
            la);
      }

      delete[] la;
      la = 0;

      // Setup clamping and our min/mag filters
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   }

   GLTextureGlyph::~GLTextureGlyph()
   {
      // Free our texture resource
      glDeleteTextures(1, &mTexName);
   }

   void GLTextureGlyph::render(int penX, int penY) const
   {
      // Enable texturing and bind our texture
      glPushAttrib(GL_TEXTURE_BIT);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, mTexName);

      // Draw the texture-mapped quad
      glPushMatrix();

      // Translate over to the font position
      glTranslatef(GLfloat(penX + mOffsetX), GLfloat(penY + mOffsetY), 0);

      float realWidth = float(mWidth) / mTexWidth;
      float realHeight = float(mHeight) / mTexHeight;

      glBegin(GL_QUADS);
      glTexCoord2f(0,         0);            glVertex2i(0,      0);
      glTexCoord2f(realWidth, 0);            glVertex2i(mWidth, 0);
      glTexCoord2f(realWidth, realHeight);   glVertex2i(mWidth, mHeight);
      glTexCoord2f(0,         realHeight);   glVertex2i(0,      mHeight);
      glEnd();

      glPopMatrix();

      glPopAttrib();
   }
}
