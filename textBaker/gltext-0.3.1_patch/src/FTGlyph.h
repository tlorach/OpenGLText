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
 * File:          $RCSfile: FTGlyph.h,v $
 * Date modified: $Date: 2003/02/03 19:40:41 $
 * Version:       $Revision: 1.4 $
 * -----------------------------------------------------------------
 *
 ************************************************************ gltext-cpr-end */
#ifndef GLTEXT_FTGLYPH_H
#define GLTEXT_FTGLYPH_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include "FTFont.h"

namespace gltext
{
   /**
    * C++ wrapper for a FreeType2 glyph handle.
    */
   class FTGlyph : public RefImpl<Glyph>
   {
   public:
      void GLTEXT_CALL render(u8* pixels, int x, int y, int pitch);
      /// Creates a new glyph from the given font face for the given
      /// character.
      static FTGlyph* create(FT_Face face, char c);

      FTGlyph(int width, int height, int offx, int offy,
              int advance, u8* pixmap, u8* bitmap);

      /// Release the FT2 resources.
      ~FTGlyph();

      int  GLTEXT_CALL getWidth();
      int  GLTEXT_CALL getHeight();
      int  GLTEXT_CALL getXOffset();
      int  GLTEXT_CALL getYOffset();
      int  GLTEXT_CALL getAdvance();
      void GLTEXT_CALL render(u8* pixels);
      void GLTEXT_CALL renderBitmap(u8* pixels);

   private:
      int mWidth;
      int mHeight;
      int mXOffset;
      int mYOffset;
      int mAdvance;
      u8* mPixmap;
      u8* mBitmap;
   };
}

#endif
