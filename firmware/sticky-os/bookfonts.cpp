// The reading fonts' data and their table (bookfonts.h). This is the one
// file that includes the generated headers: they define static arrays, so a
// second file including them would put a second copy in flash.
//
// The host tests compile every book*.cpp; without the shim's include path
// (firmware/test/ebook/shim) there is no <Arduino.h> for glyphs.h and this
// file is then empty, so tests that don't draw text still link.
#if defined(ARDUINO) || __has_include(<Arduino.h>)
#include "bookfonts.h"

#include "src/fonts/BookSerif10.h"
#include "src/fonts/BookSerif11.h"
#include "src/fonts/BookSerif12.h"
#include "src/fonts/BookSerif14.h"
#include "src/fonts/BookSerif16.h"
#include "src/fonts/BookSerif18.h"
#include "src/fonts/BookSerif21.h"
#include "src/fonts/BookSerifItalic10.h"
#include "src/fonts/BookSerifItalic11.h"
#include "src/fonts/BookSerifItalic12.h"
#include "src/fonts/BookSerifItalic14.h"
#include "src/fonts/BookSerifItalic16.h"
#include "src/fonts/BookSerifItalic18.h"
#include "src/fonts/BookSerifItalic21.h"
#include "src/fonts/BookSerifBold10.h"
#include "src/fonts/BookSerifBold11.h"
#include "src/fonts/BookSerifBold12.h"
#include "src/fonts/BookSerifBold14.h"
#include "src/fonts/BookSerifBold16.h"
#include "src/fonts/BookSerifBold18.h"
#include "src/fonts/BookSerifBold21.h"
#include "src/fonts/BookSerifBold24.h"
#include "src/fonts/BookSerifBold28.h"
#include "src/fonts/BookSerifBoldItalic10.h"
#include "src/fonts/BookSerifBoldItalic11.h"
#include "src/fonts/BookSerifBoldItalic12.h"
#include "src/fonts/BookSerifBoldItalic14.h"
#include "src/fonts/BookSerifBoldItalic16.h"
#include "src/fonts/BookSerifBoldItalic18.h"
#include "src/fonts/BookSerifBoldItalic21.h"

// [text size][style]
static const BookFont *const BODY[BOOK_SIZES][4] = {
    {&BookSerif10Font, &BookSerifItalic10Font, &BookSerifBold10Font, &BookSerifBoldItalic10Font},
    {&BookSerif11Font, &BookSerifItalic11Font, &BookSerifBold11Font, &BookSerifBoldItalic11Font},
    {&BookSerif12Font, &BookSerifItalic12Font, &BookSerifBold12Font, &BookSerifBoldItalic12Font},
    {&BookSerif14Font, &BookSerifItalic14Font, &BookSerifBold14Font, &BookSerifBoldItalic14Font},
    {&BookSerif16Font, &BookSerifItalic16Font, &BookSerifBold16Font, &BookSerifBoldItalic16Font},
    {&BookSerif18Font, &BookSerifItalic18Font, &BookSerifBold18Font, &BookSerifBoldItalic18Font},
    {&BookSerif21Font, &BookSerifItalic21Font, &BookSerifBold21Font, &BookSerifBoldItalic21Font},
};
// The bolds, small to large: a heading is the body's bold a few steps up
// (about 1.5, 1.3 and 1.15 times the body). The largest text size has no room
// left above it, and a title in a bigger type wouldn't fit the 480 px page
// anyway, so its steps are shorter; the two smallest take their own bold for
// heading 3, as there are no bolds between them and 12.
static const BookFont *const BOLD[] = {&BookSerifBold10Font, &BookSerifBold11Font, &BookSerifBold12Font, &BookSerifBold14Font, &BookSerifBold16Font, &BookSerifBold18Font,
                                       &BookSerifBold21Font, &BookSerifBold24Font, &BookSerifBold28Font};
// [level - 1][text size]: an index into BOLD.
static const uint8_t HEADING[3][BOOK_SIZES] = {{3, 4, 5, 6, 7, 8, 8}, {2, 3, 4, 5, 6, 7, 7}, {0, 1, 3, 4, 5, 6, 6}};

static int clampSize(int size) { return size < 0 ? 0 : size >= BOOK_SIZES ? BOOK_SIZES - 1 : size; }

const BookFont *bookFont(int size, int style) { return BODY[clampSize(size)][style & 3]; }

const BookFont *bookHeadingFont(int size, int level) {
  int row = level < 1 ? 0 : level > 3 ? 2 : level - 1;
  return BOLD[HEADING[row][clampSize(size)]];
}

const BookFont *bookFontOf(const GFXfont *base) {
  for (auto &row : BODY)
    for (const BookFont *f : row)
      if (f->base == base) return f;
  for (const BookFont *f : BOLD)
    if (f->base == base) return f;
  return nullptr;
}

const ExtFont *bookExtFor(const GFXfont *base) {
  const BookFont *f = bookFontOf(base);
  return f ? f->ext : nullptr;
}

// Adafruit_GFX::write() for a custom font: a character outside the font's
// run doesn't move the cursor.
static int asciiAdvance(const GFXfont *base, uint8_t c) { return c < base->first || c > base->last ? 0 : base->glyph[c - base->first].xAdvance; }

int bookAdvance(const GFXfont *base, const ExtFont *ext, uint32_t cp) {
  if (cp < 0x80) return asciiAdvance(base, (uint8_t)cp);
  int at = ext ? extIndex(cp) : -1;
  if (at >= 0) return ext->glyph[at].xAdvance;
  char two[2];
  int w = 0;
  for (const char *f = foldCodePoint(cp, two); f && *f; f++) w += asciiAdvance(base, (uint8_t)*f);
  return w;
}

int bookTextAdvance(const GFXfont *base, const ExtFont *ext, const char *utf8, int len) {
  int w = 0;
  for (int i = 0; (len < 0 || i < len) && utf8[i];) w += bookAdvance(base, ext, utf8Next(utf8, i));
  return w;
}
#endif
