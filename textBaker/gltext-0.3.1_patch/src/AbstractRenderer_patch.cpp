33d32
< #include "tga.h"
41,290d39
<    //////////////////////////////////////////////////////////////////
<    // ADDED CODE:
<    
<    struct GlyphInfo
<    {
<        struct Pix // pixel oriented data
<        {
<            int u, v;
<            int width, height;
<            int advance;
<            int offX, offY;
<        };
<        struct Norm // normalized data
<        {
<            float u, v; // position in the map in normalized coords
<            float width, height;
<            float advance;
<            float offX, offY;
<        };
<        Pix  pix;
<        Norm norm;
<    };
<    struct FileHeader
<    {
<        int texwidth, texheight;
<        struct Pix
<        {
<            int ascent;
<            int descent;
<            int linegap;
<        };
<        struct Norm
<        {
<            float ascent;
<            float descent;
<            float linegap;
<        };
<        Pix  pix;
<        Norm norm;
<        GlyphInfo glyphs[256];
<    };
< 
< static char headerStr[] = "\
<    struct GlyphInfo\n\
<    {\n\
<        struct Pix // pixel oriented data\n\
<        {\n\
<            int u, v;\n\
<            int width, height;\n\
<            int advance;\n\
<            int offX, offY;\n\
<        };\n\
<        struct Norm // normalized data\n\
<        {\n\
<            float u, v; // position in the map in normalized coords\n\
<            float width, height;\n\
<            float advance;\n\
<            float offX, offY;\n\
<        };\n\
<        Pix  pix;\n\
<        Norm norm;\n\
<    };\n\
<    struct FileHeader\n\
<    {\n\
<        int texwidth, texheight;\n\
<        struct Pix\n\
<        {\n\
<            int ascent;\n\
<            int descent;\n\
<            int linegap;\n\
<        };\n\
<        struct Norm\n\
<        {\n\
<            float ascent;\n\
<            float descent;\n\
<            float linegap;\n\
<        };\n\
<        Pix  pix;\n\
<        Norm norm;\n\
<        GlyphInfo glyphs[256];\n\
<    };\n\
<    static FileHeader font = \n\
< ";
< 
<    void GLTEXT_CALL AbstractRenderer::saveFonts(const char* file)
<    {
<       float fW = 128.0;
<       float fH = 512.0;
<       char fileName[100];
<       int penX = 1;
<       int penY = 1;
<       int maxheight = 0;
<       static char *text = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_\\`~=+[]{};':\"<>/?,. ";
<       const int H  = mFont->getAscent() + mFont->getDescent() + mFont->getLineGap();
< 
<       // optimize the size of the map
<       int texW = 64;
<       do {
<           texW <<= 1;
<           penX = 1;
<           penY = 1;
<           maxheight = 0;
<           for (const char* itr = text; *itr; ++itr)
<           {
<              Glyph* glyph = mFont->getGlyph(*itr);
<              //GLGlyph* drawGlyph = makeGlyph(glyph);
<               int width  = glyph->getWidth();
<               int height = glyph->getHeight();
<               if(height > maxheight)
<                   maxheight = height;
<               int offX   = glyph->getXOffset();
<               int offY   = glyph->getYOffset();
<               if(penX + (width+2) >= texW)
<               {
<                   penX = 0;
<                   penY += maxheight+2;
<                   maxheight = 0;
<               }
<               penX += width+2;
<           }
<       } while((penY > texW)&&(penY > (texW<<1)));
<       fH = (float)(penY + maxheight*2);
<       fW = (float)texW;
< 
<       // now write things
<       strcpy(fileName, file);
<       int i=(int)strlen(fileName)-1;
<       while((i > 0)&&(file[i] != '.'))
<           i--;
<       fileName[i] = '\0';
<       int fileSize = sizeof(FileHeader);
<       FileHeader *fileHeader = (FileHeader *)malloc( fileSize );
<       memset(fileHeader, 0, fileSize);
< 
<       fileHeader->pix.ascent = mFont->getAscent();
<       fileHeader->norm.ascent = (float)mFont->getAscent()/fH;
< 
<       fileHeader->pix.descent = mFont->getDescent();
<       fileHeader->norm.descent = (float)mFont->getDescent()/fH;
< 
<       fileHeader->pix.linegap = mFont->getLineGap();
<       fileHeader->norm.linegap = (float)mFont->getLineGap()/fH;
< 
<       fileHeader->texwidth = (int)fW;
<       fileHeader->texheight = (int)fH;
< 
<       FILE *fh;
<       char fileNameBin[100];
<       sprintf(fileNameBin, "%s_%d.bin", fileName, mFont->getSize());
<       if( !(fh = fopen( fileNameBin, "wb" )) )
<         return;
< 
<       penX = 1;
<       penY = 1;
<       maxheight = 0;
<       u8* data = new u8[fileHeader->texwidth * fileHeader->texheight];
<       memset(data, 0, fileHeader->texwidth * fileHeader->texheight);
<       for (const char* itr = text; *itr; ++itr)
<       {
<          Glyph* glyph = mFont->getGlyph(*itr);
<          //GLGlyph* drawGlyph = makeGlyph(glyph);
<           int width  = glyph->getWidth();
<           int height = glyph->getHeight();
<           if(height > maxheight)
<               maxheight = height;
<           int offX   = glyph->getXOffset();
<           int offY   = glyph->getYOffset();
<           if(penX + (width+2) >= fileHeader->texwidth)
<           {
<               penX = 0;
<               penY += maxheight+2;
<               maxheight = 0;
<           }
<           GlyphInfo &g = fileHeader->glyphs[*itr];
< 
<           g.pix.advance = glyph->getAdvance();
<           g.norm.advance = (float)glyph->getAdvance()/fW;
<           g.pix.u = penX;
<           g.pix.v = penY;
<           g.norm.u = (float)penX/fW;
<           g.norm.v = (float)penY/fH;
<           g.pix.width = width;
<           g.pix.height = height;
<           g.norm.width = (float)width/fW;
<           g.norm.height = (float)height/fH;
<           g.pix.offX = offX;
<           g.pix.offY = offY;
<           g.norm.offX = (float)offX/fW;
<           g.norm.offY = (float)offY/fH;
< 
<           glyph->render(data, penX, penY, fileHeader->texwidth);
<           penX += width+2;
<       }
<       fwrite( fileHeader, sizeof (unsigned char), fileSize, fh );
<       fclose( fh );
<       //
<       // Write the C version
<       //
<       sprintf(fileNameBin, "%s_%d.h", fileName, mFont->getSize());
<       if( !(fh = fopen( fileNameBin, "w" )) )
<         return;
<       fprintf(fh, "namespace font_%d {\n", /*fileName, */mFont->getSize());
<       fprintf(fh, headerStr);
<       fprintf(fh, "{ %d, %d,\n", fileHeader->texwidth, fileHeader->texheight);
<       fprintf(fh, "{ %d, %d, %d },\n", fileHeader->pix.ascent, fileHeader->pix.descent, fileHeader->pix.linegap);
<       fprintf(fh, "{ %.3ff, %.3ff, %.3ff },\n", fileHeader->norm.ascent, fileHeader->norm.descent, fileHeader->norm.linegap);
<       fprintf(fh, "{\n");
<       for (int itr = 0; itr<256; itr++)
<       {
<           GlyphInfo &g = fileHeader->glyphs[itr];
<           fprintf(fh, "/*%c*/{ { %d, %d, %d, %d, %d, %d, %d },{%.3ff, %.3ff, %.3ff, %.3ff, %.3ff, %.3ff, %.3ff} },\n",isprint(itr) ? itr:' ',
<            g.pix.u, g.pix.v,
<            g.pix.width, g.pix.height,
<            g.pix.advance,
<            g.pix.offX, g.pix.offY,
<            g.norm.u, g.norm.v,
<            g.norm.width, g.norm.height,
<            g.norm.advance,
<            g.norm.offX, g.norm.offY);
<       }
<       fprintf(fh, "} };\n}\n");
<       fclose( fh );
< 
<       TGA tga;
<       char fileNameTGA[100];
<       sprintf(fileNameTGA, "%s_%d.tga", fileName, mFont->getSize());
<       TGA::TGAError err = tga.saveFromExternalData( fileNameTGA, fileHeader->texwidth, fileHeader->texheight, TGA::RGB, data );
<       //
<       //
<       sprintf(fileNameBin, "%s_%d_bitmap.h", fileName, mFont->getSize());
<       if( !(fh = fopen( fileNameBin, "w" )) )
<         return;
<       fprintf(fh, "namespace font_%d {\n", /*fileName, */mFont->getSize());
<       fprintf(fh, "  static int imageW = %d;\nstatic int imageH = %d;\n", fileHeader->texwidth, fileHeader->texheight);
<       fprintf(fh, "  static unsigned char image[] = { //gray-scale image\n");
<       u8 *l = data;
<       for(int y=0; y<fileHeader->texheight;y++)
<       {
<           for(int x=0; x<fileHeader->texwidth;x++)
<           {
<             fprintf(fh, "%d,", *l);
<             l++;
<           }
<           fprintf(fh, "\n");
<       }
<       fprintf(fh, "};\n}\n");
<       fclose( fh );
<    }
<    // END OF ADDED CODE
<    //////////////////////////////////////////////////////////////////
