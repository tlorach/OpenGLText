Baking Fonts is aschieved for now with patching GLText 0.3.1 ( http://gltext.sourceforge.net/home.php ) from Ben Scott

This folder contains the changes I made to finally save the data I needed.
Download freetype (I took 2.3.5-1) and download GLText. Then modify it with the data in this folder.

I have put a compiled version of it for Windows. Note this application would require to be better and therefore could crash or not work properly.

bakeFonts.exe < .ttf file > < size >

this tool will generate 2 possible ways of including the font:
* bin + tga : those 2 files can be used to setup the font
* headers font_xx.h and font_xx_bitmap.h : in this case, everything is generated as include files. this is for the case where one wants to have the font data part of the exe file without any reference to external files.

Few comments:
------------
* I found this tool online, but I could have used anything that would use freetype library.
* One might argue that gltext is exactly doing what I want : display text onscreen with truetype fonts... **but**:
** This project is rather big for what it does (isn't it ? :-)
** the C++ design is overkill... at least to me
** we must link against Freetype library to have it working. This is flexible, of course (becaude we can directly read any .ttf file...), but this is not appropriate for my goal : one might not want to link against freetype library if he targeted Android or iOS...
** The rendering loop... is not the efficient way to do. Each font is rendered separately. Our approach is to render almost everything in one single drawcall.

The basic idea behing this patch is to allow me to save a tga file made of the character I need with the right size. a binary file will be created, in which structures of Glyphs are stored.
