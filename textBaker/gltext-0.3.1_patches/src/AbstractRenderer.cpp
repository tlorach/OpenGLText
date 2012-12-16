#include "tga.h"
   struct GlyphInfo
   {
       struct Pix // pixel oriented data
       {
           int u, v;
           int width, height;
           int advance;
           int offX, offY;
       };
       struct Norm // normalized data
       {
           float u, v; // position in the map in normalized coords
           float width, height;
           float advance;
           float offX, offY;
       };
       Pix  pix;
       Norm norm;
   };
   struct FileHeader
   {
       int texwidth, texheight;
       struct Pix
       {
           int ascent;
           int descent;
           int linegap;
       };
       struct Norm
       {
           float ascent;
           float descent;
           float linegap;
       };
       Pix  pix;
       Norm norm;
       GlyphInfo glyphs[256];
   };

   void GLTEXT_CALL AbstractRenderer::saveFonts(const char* file)
   {
      float fW = 128.0;
      float fH = 512.0;
      char fileName[100];
      int penX = 1;
      int penY = 1;
      int maxheight = 0;
      static char *text = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_\\`~=+[]{};':\"<>/?,. ";
      const int H  = mFont->getAscent() + mFont->getDescent() + mFont->getLineGap();

      // optimize the size of the map
      int texW = 64;
      do {
          texW <<= 1;
          penX = 1;
          penY = 1;
          maxheight = 0;
          for (const char* itr = text; *itr; ++itr)
          {
             Glyph* glyph = mFont->getGlyph(*itr);
             //GLGlyph* drawGlyph = makeGlyph(glyph);
              int width  = glyph->getWidth();
              int height = glyph->getHeight();
              if(height > maxheight)
                  maxheight = height;
              int offX   = glyph->getXOffset();
              int offY   = glyph->getYOffset();
              if(penX + (width+2) >= texW)
              {
                  penX = 0;
                  penY += maxheight+2;
                  maxheight = 0;
              }
              penX += width+2;
          }
      } while((penY > texW)&&(penY > (texW<<1)));
      fH = (float)(penY + maxheight*2);
      fW = (float)texW;

      // now write things
      strcpy(fileName, file);
      int i=(int)strlen(fileName)-1;
      while((i > 0)&&(file[i] != '.'))
          i--;
      fileName[i] = '\0';
      int fileSize = sizeof(FileHeader);
      FileHeader *fileHeader = (FileHeader *)malloc( fileSize );
      memset(fileHeader, 0, fileSize);

      fileHeader->pix.ascent = mFont->getAscent();
      fileHeader->norm.ascent = (float)mFont->getAscent()/fH;

      fileHeader->pix.descent = mFont->getDescent();
      fileHeader->norm.descent = (float)mFont->getDescent()/fH;

      fileHeader->pix.linegap = mFont->getLineGap();
      fileHeader->norm.linegap = (float)mFont->getLineGap()/fH;

      fileHeader->texwidth = (int)fW;
      fileHeader->texheight = (int)fH;

      FILE *fh;
      char fileNameBin[100];
      sprintf(fileNameBin, "%s_%d.bin", fileName, mFont->getSize());
      if( !(fh = fopen( fileNameBin, "wb" )) )
        return;

      penX = 1;
      penY = 1;
      maxheight = 0;
      u8* data = new u8[fileHeader->texwidth * fileHeader->texheight];
      memset(data, 0, fileHeader->texwidth * fileHeader->texheight);
      for (const char* itr = text; *itr; ++itr)
      {
         Glyph* glyph = mFont->getGlyph(*itr);
         //GLGlyph* drawGlyph = makeGlyph(glyph);
          int width  = glyph->getWidth();
          int height = glyph->getHeight();
          if(height > maxheight)
              maxheight = height;
          int offX   = glyph->getXOffset();
          int offY   = glyph->getYOffset();
          if(penX + (width+2) >= fileHeader->texwidth)
          {
              penX = 0;
              penY += maxheight+2;
              maxheight = 0;
          }
          GlyphInfo &g = fileHeader->glyphs[*itr];

          g.pix.advance = glyph->getAdvance();
          g.norm.advance = (float)glyph->getAdvance()/fW;
          g.pix.u = penX;
          g.pix.v = penY;
          g.norm.u = (float)penX/fW;
          g.norm.v = (float)penY/fH;
          g.pix.width = width;
          g.pix.height = height;
          g.norm.width = (float)width/fW;
          g.norm.height = (float)height/fH;
          g.pix.offX = offX;
          g.pix.offY = offY;
          g.norm.offX = (float)offX/fW;
          g.norm.offY = (float)offY/fH;

          glyph->render(data, penX, penY, fileHeader->texwidth);
          penX += width+2;
      }
      fwrite( fileHeader, sizeof (unsigned char), fileSize, fh );
      fclose( fh );
      TGA tga;
      char fileNameTGA[100];
      sprintf(fileNameTGA, "%s_%d.tga", fileName, mFont->getSize());
      TGA::TGAError err = tga.saveFromExternalData( fileNameTGA, fileHeader->texwidth, fileHeader->texheight, TGA::RGB, data );

   }
