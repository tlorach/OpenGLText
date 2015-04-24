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
 * File:          $RCSfile: FTGlyph.cpp,v $
 * Date modified: $Date: 2003/02/04 03:31:03 $
 * Version:       $Revision: 1.6 $
 * -----------------------------------------------------------------
 *
 ************************************************************ gltext-cpr-end */
#include <algorithm>
#include "FTGlyph.h"

namespace gltext
{
   void FTGlyph::render(u8* pixels, int x, int y, int pitch)
   {
       u8 * p = mPixmap;
       u8 * pdst = pixels + x + pitch*(y+mHeight-1);
       for(int i = 0; i<mHeight; i++)
       {
         memcpy(pdst , p, mWidth);
         p += mWidth;
         pdst -= pitch;
       }
   }

   FTGlyph* FTGlyph::create(FT_Face face, char c)
   {
      // just load the glyph into the face's glyph slot
      FT_Error error = FT_Load_Char(face, c, FT_LOAD_DEFAULT);
      if (error)
      {
         return 0;
      }

      // get a glyph object from the glyph slot to be rendered as a pixmap
      FT_Glyph pxm_glyph;
      error = FT_Get_Glyph(face->glyph, &pxm_glyph);
      if (error)
      {
         return 0;
      }

      // copy the unmodified pixmap glyph into a glyph fated to be
      // rendered as a bitmap
      FT_Glyph btm_glyph;
      error = FT_Glyph_Copy(pxm_glyph, &btm_glyph);
      if (error)
      {
         FT_Done_Glyph(pxm_glyph);
         return 0;
      }

      error = FT_Glyph_To_Bitmap(&pxm_glyph, ft_render_mode_normal, 0, true);
      if (error)
      {
         FT_Done_Glyph(pxm_glyph);
         FT_Done_Glyph(btm_glyph);
         return 0;
      }

      error = FT_Glyph_To_Bitmap(&btm_glyph, ft_render_mode_mono, 0, true);
      if (error)
      {
         FT_Done_Glyph(pxm_glyph);
         FT_Done_Glyph(btm_glyph);
         return 0;
      }

      FT_BitmapGlyph pxm_glyph_bitmap = (FT_BitmapGlyph)pxm_glyph;
      FT_BitmapGlyph btm_glyph_bitmap = (FT_BitmapGlyph)btm_glyph;

      int advance = face->glyph->metrics.horiAdvance / 64;

      // use the largest bitmap rendering...
      int width  = std::max(pxm_glyph_bitmap->bitmap.width,
                            btm_glyph_bitmap->bitmap.width);
      int height = std::max(pxm_glyph_bitmap->bitmap.rows,
                            btm_glyph_bitmap->bitmap.rows);

      // allocate pixmap and bitmap buffers
      u8* pixmap = new u8[width * height];
      u8* bitmap = new u8[width * height];

      // copy the pixmap bitmap glyph buffer into our local buffer
      int pxm_pitch  = pxm_glyph_bitmap->bitmap.pitch;
      u8* pxm_source = pxm_glyph_bitmap->bitmap.buffer;
      for (int i = 0; i < height; ++i)
      {
         memcpy(pixmap + i * width, pxm_source, width);
         pxm_source += pxm_pitch;
      }

      // copy the bitmap bitmap glyph buffer into our local buffer
      int btm_pitch  = btm_glyph_bitmap->bitmap.pitch;
      u8* btm_source = btm_glyph_bitmap->bitmap.buffer;
      for (int i = 0; i < height; ++i)
      {
         for (int c = 0; c < width; ++c)
         {
            int bytePos = i * btm_pitch + (c / 8);
            int bit = c % 8;
            unsigned char byte = btm_source[bytePos];
            int value = ((byte & (0x80 >> bit)) != 0) * 255;
            bitmap[i * width + c] = value;
         }
      }
      
      int offx = pxm_glyph_bitmap->left;
      int offy = -pxm_glyph_bitmap->top;

      FT_Done_Glyph(pxm_glyph);
      FT_Done_Glyph(btm_glyph);

      // FINALLY, create a new glyph object with the decoded pixel buffers
      return new FTGlyph(width, height, offx, offy, advance, pixmap, bitmap);
   }

   FTGlyph::FTGlyph(int width, int height, int offx, int offy, int advance,
                    u8* pixmap, u8* bitmap)
   {
      mWidth   = width;
      mHeight  = height;
      mXOffset = offx;
      mYOffset = offy;
      mAdvance = advance;
      mPixmap  = pixmap;
      mBitmap  = bitmap;
   }

   FTGlyph::~FTGlyph()
   {
      delete[] mPixmap;
      mPixmap = 0;
      delete[] mBitmap;
      mBitmap = 0;
   }

   int
   FTGlyph::getWidth()
   {
      return mWidth;
   }

   int
   FTGlyph::getHeight()
   {
      return mHeight;
   }
   
   int
   FTGlyph::getXOffset()
   {
      return mXOffset;
   }
   
   int
   FTGlyph::getYOffset()
   {
      return mYOffset;
   }

   int
   FTGlyph::getAdvance()
   {
      return mAdvance;
   }

   void
   FTGlyph::render(u8* pixels)
   {
      memcpy(pixels, mPixmap, mWidth * mHeight);
   }

   void
   FTGlyph::renderBitmap(u8* pixels)
   {
      memcpy(pixels, mBitmap, mWidth * mHeight);
   }
}
