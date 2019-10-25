# OpenGLText

This pair of cpp and h file provides a very easy way to draw text in OpenGL, *using fonts generated from Truetype fonts*.

There are many tools and libraries available. But :
* I needed to have a really simple approach
* I certainly didn't want to link against any huge monster, such as Freetype library
* I never got satisfied by any existing tool on their way to batch primitives through OpenGL driver : most are rendering characters one after another (even using immediate mode). Some other might use the deprecated Display List...
* I did *not* need the whole flexibility that freetype offers
* I might want to use this implementation on tablets (Android and iOS)

This file proposes a very simple approach based on a texture Atlas containing all the necessary Fonts for a specific type and size. If more than one type or more than one size is needed, the application will have to work with many instances of the class and related resource.

The fonts are in a tga file; a binary file is associated, containing all the necessary offsets to work correctly with the texture-font from tga file.

## Example on how to use it.

init time:
````
        OpenGLText             oglText;
        ...
        if(!oglText.init(font_name, canvas_width, canvas_height))
            return false;
````

render time:
````
        oglText.beginString();
        float bbStr[2];
        char *tmpStr = "Hello world";
        oglText.stringSize(tmpStr, bbStr);
        oglText.drawString(posX - bbStr[0]*0.5, posY - bbStr[1], tmpStr, 0,0xF0F0F0F0);
        ...
        oglText.endString(); // will render the whole at once
````

# Baking Fonts

textBaker folder contains few details about this.

 For now, I got a bit 'hacky', here: baking Fonts is achieved with patching GLText 0.3.1 ( http://gltext.sourceforge.net/home.php )

This folder contains the changes I made to finally save the data I needed.
Download freetype (I took 2.3.5-1) and download GLText. Then modify it with the data in this folder.

I have put a compiled version of it for Windows. Note this application would require to be better and therefore could crash or not work properly.

bakeFonts.exe < .ttf file > < size >

Example from the demo :
![Example](https://github.com/tlorach/OpenGLText/raw/master/example/example.png)

__Few comments:__

* I found this tool online, but I could have used anything that would use freetype library.
* One might argue that gltext is exactly doing what I want : display text onscreen with truetype fonts... **but**:
    * This project is rather big for what it does (isn't it ? :-)
    * the C++ design is overkill... at least to me
    * we must link against Freetype library to have it working. This is flexible, of course (becaude we can directly read any .ttf file...), but this is not appropriate for my goal : one might not want to link against freetype library if he targeted Android or iOS...
    * The rendering loop... is not the efficient way to do. Each font is rendered separately. Our approach is to render almost everything in one single drawcall.

The basic idea behind this patch is to allow me to save a tga file made of the character I need with the right size. a binary file will be created, in which structures of Glyphs are stored.
