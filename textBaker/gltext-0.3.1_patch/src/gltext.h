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
 * File:          $RCSfile: gltext.h,v $
 * Date modified: $Date: 2003/06/03 22:21:23 $
 * Version:       $Revision: 1.25 $
 * -----------------------------------------------------------------
 *
 ************************************************************ gltext-cpr-end */
#ifndef GLTEXT_H
#define GLTEXT_H

#ifndef __cplusplus
#  error GLText requires C++
#endif

#include <sstream>

// The calling convention for cross-DLL calls in win32
#ifndef GLTEXT_CALL
#  ifdef WIN32
#    define GLTEXT_CALL __stdcall
#  else
#    define GLTEXT_CALL
#  endif
#endif

// Export functions from the DLL
#ifndef GLTEXT_DECL
#  if defined(WIN32) || defined(_WIN32)
#    ifdef GLTEXT_EXPORTS
#      define GLTEXT_DECL __declspec(dllexport)
#    else
#      define GLTEXT_DECL __declspec(dllimport)
#    endif
#  else
#    define GLTEXT_DECL
#  endif
#endif

#define GLTEXT_FUNC(ret) extern "C" GLTEXT_DECL ret GLTEXT_CALL

namespace gltext
{
   typedef unsigned char u8;


   /**
    * Base class for classes the manage their own memory using reference
    * counting.
    *
    * This class was originally written by Chad Austin for the audiere project
    * and released under the LGPL.
    */
   class RefCounted
   {
   protected:
      /**
       * Protected so users of recounted classes don't use std::auto_ptr or the
       * delete operator.
       *
       * Interfaces that derive from RefCounted should define an inline, empty,
       * protected destructor as well.
       */
      ~RefCounted() {}

   public:
      /**
       * Add a reference to this object. This will increment the internal
       * reference count.
       */
      virtual void GLTEXT_CALL ref() = 0;

      /**
       * Remove a reference from this object. This will decrement the internal
       * reference count. When this count reaches 0, the object is destroyed.
       */
      virtual void GLTEXT_CALL unref() = 0;
   };

   /**
    * This defines a smart pointer to some type T that implements the RefCounted
    * interface. This object will do all the nasty reference counting for you.
    *
    * This class was originally written by Chad Austin for the audiere project
    * and released under the LGPL.
    */
   template< typename T >
   class RefPtr
   {
   public:
      RefPtr(T* ptr = 0)
      {
         mPtr = 0;
         *this = ptr;
      }

      RefPtr(const RefPtr<T>& ptr)
      {
         mPtr = 0;
         *this = ptr;
      }

      ~RefPtr()
      {
         if (mPtr)
         {
            mPtr->unref();
            mPtr = 0;
         }
      }

      RefPtr<T>& operator=(T* ptr)
      {
         if (ptr != mPtr)
         {
            if (mPtr)
            {
               mPtr->unref();
            }
            mPtr = ptr;
            if (mPtr)
            {
               mPtr->ref();
            }
         }
         return *this;
      }

      RefPtr<T>& operator=(const RefPtr<T>& ptr)
      {
         *this = ptr.mPtr;
         return *this;
      }

      T* operator->() const
      {
         return mPtr;
      }

      T& operator*() const
      {
         return *mPtr;
      }

      operator bool() const
      {
         return (mPtr != 0);
      }

      T* get() const
      {
         return mPtr;
      }

   private:
      T* mPtr;
   };


   /**
    * A basic implementation of the RefCounted interface.  Derive your
    * implementations from RefImpl<YourInterface>.
    *
    * This class was originally written by Chad Austin for the audiere project
    * and released under the LGPL.
    */
   template< class Interface >
   class RefImpl : public Interface
   {
   protected:
      RefImpl()
         : mRefCount(0)
      {}

      /**
       * So the implementation can put its destruction logic in the destructor,
       * as natural C++ code does.
       */
      virtual ~RefImpl() {}

   public:
      void GLTEXT_CALL ref()
      {
         ++mRefCount;
      }

      void GLTEXT_CALL unref()
      {
         if (--mRefCount == 0)
         {
            delete this;
         }
      }

   private:
      int mRefCount;
   };


   /// Renderer types supported
   enum FontRendererType
   {
      BITMAP,
      PIXMAP,
      TEXTURE,
      MIPMAP,
   };


   /// Represents a particular shape in a font used to represent one
   /// or more characters.
   class Glyph : public RefCounted
   {
   protected:
      ~Glyph() {}

   public:
      /// Returns width in pixels
      virtual int GLTEXT_CALL getWidth() = 0;

      /// Returns height in pixels.
      virtual int GLTEXT_CALL getHeight() = 0;
      
      /// Returns the X offset of this glyph when it is drawn.
      virtual int GLTEXT_CALL getXOffset() = 0;
      
      /// Returns the Y offset of this glyph when it is drawn.
      virtual int GLTEXT_CALL getYOffset() = 0;

      /// Returns the number of pixels the pen should move to the
      /// right to draw the next character
      virtual int GLTEXT_CALL getAdvance() = 0;

      /// Renders the glyph into a buffer of width * height bytes.  A
      /// value of 0 means the pixel is transparent; likewise, 255
      /// means opaque.
      virtual void GLTEXT_CALL render(u8* pixels) = 0;
      virtual void GLTEXT_CALL render(u8* pixels, int x, int y, int pitch) = 0;

      /// renderBitmap() is the same as render, except that it only
      /// uses values 0 and 255.  These bitmaps may look better on
      /// two-color displays.
      virtual void GLTEXT_CALL renderBitmap(u8* pixels) = 0;
   };


   /**
    * Represents a particular font face.
    */
   class Font : public RefCounted
   {
   protected:
      ~Font() {}

   public:
      /// Gets the name of this font.
      virtual const char* GLTEXT_CALL getName() = 0;

      /**
       * Returns the glyph object associated with the character c in
       * the font.
       */
      virtual Glyph* GLTEXT_CALL getGlyph(unsigned char c) = 0;

      /// Gets the point size of this font.
      virtual int GLTEXT_CALL getSize() = 0;

      /**
       * Returns the DPI value passed to OpenFont() when this font
       * was created.
       */
      virtual int GLTEXT_CALL getDPI() = 0;

      /**
       * Gets the ascent of this font. This is the distance from the
       * baseline to the top of the tallest glyph of this font.
       */
      virtual int GLTEXT_CALL getAscent() = 0;

      /**
       * Gets the descent of this font. This is the distance from the
       * baseline to the bottom of the glyph that descends the most
       * from the baseline.
       */
      virtual int GLTEXT_CALL getDescent() = 0;

      /**
       * Gets the distance that must be placed between two lines of
       * text. Thus the baseline to baseline distance can be computed
       * as ascent + descent + linegap.
       */
      virtual int GLTEXT_CALL getLineGap() = 0;

      /**
       * Returns the kerning distance between character c1 and
       * character c2.  The kerning distance is added to the previous
       * character's advance distance.
       */
      virtual int GLTEXT_CALL getKerning(unsigned char l, unsigned char r) = 0;
   };
   typedef RefPtr<Font> FontPtr;


   /**
    * Interface to an object that knows how to render a font.
    */
   class FontRenderer : public RefCounted
   {
   protected:
      ~FontRenderer() {}

   public:
      /**
       * Renders the text in the given string as glyphs using this font
       * renderer's current font.
       */
      virtual void GLTEXT_CALL render(const char* text) = 0;

      /**
       * Computes the width of the given text string if it were to be rendered
       * with this renderer.
       */
      virtual int GLTEXT_CALL getWidth(const char* text) = 0;

      /**
       * Computes the height of the given text string if it were to be
       * rendered with this renderer.
       */
      virtual int GLTEXT_CALL getHeight(const char* text) = 0;

      /**
       * Returns the font used to create this renderer.
       */
      virtual Font* GLTEXT_CALL getFont() = 0;
      virtual void GLTEXT_CALL saveFonts(const char* file) = 0;
   };
   typedef RefPtr<FontRenderer> FontRendererPtr;


   /**
    * Provides an iostream-like interface to a font renderer.  Use as follows:
    * Stream(renderer).get() << blah blah blah;
    *
    * The GLTEXT_STREAM macro may be simpler:
    * GLTEXT_STREAM(renderer) << blah blah blah;
    */
   class Stream : public std::ostringstream
   {
   public:
      Stream(FontRenderer* r)
         : mRenderer(r)
      {
      }
      
      Stream(const FontRendererPtr& p)
         : mRenderer(p.get())
      {
      }

      ~Stream()
      {
         flush();
      }

      /**
       * This is a little trick to convert an rvalue to an lvalue.
       * The problem is this: class temporaries created with
       * FontStream(r) are rvalues, which means they cannot be
       * assigned to FontStream& references.  Therefore, temporaries
       * cannot be used as arguments to the standard operator<<
       * overloads.
       *
       * However!  You CAN call methods on object rvalues and methods
       * CAN return lvalues!  This method effectively converts *this
       * from an rvalue to an lvalue using the syntax:
       * FontStream(r).get().
       */
      Stream& get()
      {
         return *this;
      }

      FontRenderer* getRenderer()
      {
         return mRenderer.get();
      }

      /**
       * Calls render() on the associated FontRenderer using the
       * string that has been created so far.  Then it sets the string
       * to empty.  flush() is called before any GLText manipulator
       * and when the font stream is destroyed.
       */
      void flush()
      {
         mRenderer->render(str().c_str());
         str("");
      }

   private:
      FontRendererPtr mRenderer;
   };

   /**
    * This generic insertion operator can take any type (but fails if
    * the type can't be inserted into a std::ostream) and returns a
    * FontStream&.  Otherwise, something like |FontStream(...) <<
    * "blah" << gltextmanip;| would not work, as 'gltextmanip' cannot
    * be inserted into a std::ostream.
    */
   template<typename T>
   Stream& operator<<(Stream& fs, T t)
   {
      static_cast<std::ostream&>(fs) << t;
      return fs;
   }

   /**
    * Flush the font stream and apply the gltext stream manipulator to it.
    */
   inline Stream& operator<<(Stream& fs, Stream& (*manip)(Stream&))
   {
      fs.flush();
      return manip(fs);
   }

   /**
    * A convenience macro around gltext::Stream().  Used as follows:
    * GLTEXT_STREAM(renderer) << blah blah blah;
    *
    * This is the preferred interface for streaming text to a
    * renderer.
    */
   #define GLTEXT_STREAM(renderer) gltext::Stream(renderer).get()


   /**
    * PRIVATE API - for internal use only
    * Anonymous namespace containing our exported functions. They are extern "C"
    * so we don't mangle the names and they export nicely as shared libraries.
    */
   namespace internal
   {
      /// Gets version information
      GLTEXT_FUNC(const char*) GLTextGetVersion();

      /// Creates a new Font by name.
      GLTEXT_FUNC(Font*) GLTextOpenFont(const char* name, int size, int dpi);

      /// Creates a new FontRenderer.
      GLTEXT_FUNC(FontRenderer*) GLTextCreateRenderer(
         FontRendererType type,
         Font* font);
   }

   /**
    * Gets the version string for GLText.
    *
    * @return  GLText version information
    */
   inline const char* GetVersion()
   {
      return internal::GLTextGetVersion();
   }

   /**
    * Creates a new font from the given name, style and point size.
    *
    * @param name    the name of a particular font face.
    * @param size    the point size of the font, in 1/72nds of an inch.
    * @param dpi     number of pixels per inch on the display
    */
   inline Font* OpenFont(const char* name, int size, int dpi = 72)
   {
      return internal::GLTextOpenFont(name, size, dpi);
   }

   /**
    * Creates a new font renderer of the given type.
    *
    * @param type    the type of renderer to create.
    * @param font    the font to render
    */
   inline FontRenderer* CreateRenderer(FontRendererType type,
                                       const FontPtr& font)
   {
      return internal::GLTextCreateRenderer(type, font.get());
   }
}


#endif
