// Adafruit GFX's font structures (Adafruit_GFX_Library/gfxfont.h, BSD
// licence), for building the Books app's text code on a computer without the
// library. The same layout and the same include guard as the original.
#ifndef _GFXFONT_H_
#define _GFXFONT_H_
#include <stdint.h>

typedef struct {
  uint16_t bitmapOffset;  // into GFXfont->bitmap
  uint8_t width;          // the bitmap, in pixels
  uint8_t height;
  uint8_t xAdvance;       // how far the cursor moves
  int8_t xOffset;         // from the cursor to the bitmap's top left corner
  int8_t yOffset;
} GFXglyph;

typedef struct {
  uint8_t *bitmap;   // the glyphs' bitmaps, one after another
  GFXglyph *glyph;
  uint16_t first;    // the first and the last character
  uint16_t last;
  uint8_t yAdvance;  // the line distance
} GFXfont;

#endif
