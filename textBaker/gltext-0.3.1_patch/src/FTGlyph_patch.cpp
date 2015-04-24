35,46d34
<    void FTGlyph::render(u8* pixels, int x, int y, int pitch)
<    {
<        u8 * p = mPixmap;
<        u8 * pdst = pixels + x + pitch*(y+mHeight-1);
<        for(int i = 0; i<mHeight; i++)
<        {
<          memcpy(pdst , p, mWidth);
<          p += mWidth;
<          pdst -= pitch;
<        }
<    }
< 
