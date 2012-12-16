// in class Glyph : public RefCounted
// after      virtual void GLTEXT_CALL render(u8* pixels) = 0;
// add:
      virtual void GLTEXT_CALL render(u8* pixels, int x, int y, int pitch) = 0;
// in    class FontRenderer : public RefCounted
// add:
