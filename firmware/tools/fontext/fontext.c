/*
fontext: the letters beyond ASCII for an Adafruit GFX font, as a second,
sparse glyph table (see firmware/sticky-os/glyphs.h).

Made from Adafruit GFX's fontconvert (BSD licence, Adafruit Industries),
with the same rendering (FreeType, monochrome, 141 dpi, the v35 TrueType
interpreter), so the new letters match the library's FreeSans*pt7b and
FreeSerifItalic*pt7b ones pixel for pixel in style and size.

  fontext <font.ttf> <points> <name> <codes.txt> > <name>Ext.h
  fontext book <font.ttf> <points> <name> <codes.txt> > <name>.h

codes.txt: the code points, one hex number a line, sorted, the same file for
every font (the board keeps one list of codes and an index into each
font's table). make.sh builds this and writes every font.

"book" writes a whole font for the Books app in one header: printable ASCII
as an Adafruit GFX font (what fontconvert writes: the library ships FreeSerif
at 9, 12, 18 and 24 pt only), the same extra letters, and the line metrics
measured from the glyphs (bookfonts.h).
*/
#include <ctype.h>
#include <ft2build.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include FT_GLYPH_H
#include FT_MODULE_H
#include FT_TRUETYPE_DRIVER_H

#define DPI 141  // as Adafruit's fonts

static int column;
static uint8_t sum, bitMask = 0x80;
static long bytes;

static void flushByte(void) {
  printf("%s0x%02X", bytes ? (column ? ", " : ",\n  ") : "  ", sum);
  column = (column + 1) % 12;
  bytes++;
  sum = 0;
  bitMask = 0x80;
}

static void enbit(int on) {
  if (on) sum |= bitMask;
  if (!(bitMask >>= 1)) flushByte();
}

typedef struct { int offset, w, h, adv, xo, yo; } Glyph;

// One character, rendered and appended to the bitmap being printed.
// `tidy`: a glyph without ink (a space) gets no bitmap at all, as in the
// library's own fonts; FreeType since 2.10 or so hands back one blank pixel
// for it, which the older tables here keep.
static int raster(FT_Face face, unsigned code, Glyph *g, const char *name, int tidy) {
  if (FT_Get_Char_Index(face, code) == 0) fprintf(stderr, "%s: no glyph for U+%04X\n", name, code);
  if (FT_Load_Char(face, code, FT_LOAD_TARGET_MONO) || FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO)) {
    fprintf(stderr, "%s: U+%04X failed\n", name, code);
    return 0;
  }
  FT_Glyph glyph;
  FT_Get_Glyph(face->glyph, &glyph);
  FT_Bitmap *b = &face->glyph->bitmap;
  FT_BitmapGlyphRec *bg = (FT_BitmapGlyphRec *)glyph;
  g->offset = (int)bytes;
  g->w = b->width;
  g->h = b->rows;
  g->adv = face->glyph->advance.x >> 6;
  g->xo = bg->left;
  g->yo = 1 - bg->top;
  int ink = 0;
  for (unsigned y = 0; y < b->rows; y++)
    for (unsigned x = 0; x < b->width; x++) ink |= b->buffer[y * b->pitch + x / 8] & (0x80 >> (x & 7));
  if (tidy && !ink) {
    g->w = g->h = g->xo = 0, g->yo = 1;
    FT_Done_Glyph(glyph);
    return 1;
  }
  for (unsigned y = 0; y < b->rows; y++)
    for (unsigned x = 0; x < b->width; x++) enbit(b->buffer[y * b->pitch + x / 8] & (0x80 >> (x & 7)));
  if (bitMask != 0x80) flushByte();  // each glyph starts on a byte, as fontconvert's
  FT_Done_Glyph(glyph);
  return 1;
}

// The glyph tables keep sizes in a byte and offsets in a signed one.
static int fits(const Glyph *g) {
  return g->w < 256 && g->h < 256 && g->adv >= 0 && g->adv < 256 && g->xo >= -128 && g->xo < 128 && g->yo >= -128 && g->yo < 128;
}

// How far the tallest of `chars` rises above the baseline (below: how far the deepest hangs under it).
static int reach(const Glyph *ascii, const char *chars, int below) {
  int most = 0;
  for (; *chars; chars++) {
    const Glyph *g = &ascii[*chars - 0x20];
    int r = below ? g->yo + g->h : -g->yo;
    if (g->h && r > most) most = r;
  }
  return most;
}

int main(int argc, char *argv[]) {
  int book = argc == 6 && !strcmp(argv[1], "book");
  if (argc != 5 && !book) {
    fprintf(stderr, "usage: %s [book] font.ttf points name codes.txt\n", argv[0]);
    return 1;
  }
  argv += book;
  int size = atoi(argv[2]);
  const char *name = argv[3];
  FILE *f = fopen(argv[4], "r");
  if (!f) return perror(argv[4]), 1;
  unsigned codes[2048];
  int n = 0;
  char line[64];
  while (n < 2048 && fgets(line, sizeof line, f))
    if (isxdigit((unsigned char)line[0])) codes[n++] = (unsigned)strtoul(line, NULL, 16);
  fclose(f);

  FT_Library lib;
  FT_Face face;
  if (FT_Init_FreeType(&lib)) return fprintf(stderr, "FreeType init failed\n"), 1;
  FT_UInt interpreter = TT_INTERPRETER_VERSION_35;
  FT_Property_Set(lib, "truetype", "interpreter-version", &interpreter);
  if (FT_New_Face(lib, argv[1], 0, &face)) return fprintf(stderr, "can't load %s\n", argv[1]), 1;
  FT_Set_Char_Size(face, size << 6, 0, DPI, 0);
  const char *ttf = strrchr(argv[1], '/') ? strrchr(argv[1], '/') + 1 : argv[1];

  Glyph *g = calloc(n, sizeof *g);
  Glyph ascii[0x7F - 0x20];
  long asciiBytes = 0;
  if (book) {
    printf("// %s: GNU FreeFont 20120503's %s at %d pt, 141 dpi (a %d px em), for the Books\n", name, ttf, size, (size * DPI + 36) / 72);
    printf("// app (bookfonts.h): printable ASCII as an Adafruit GFX font, the %d letters beyond it\n", n);
    printf("// (glyphs.h) and the line's measures. Made by firmware/tools/fontext, rendered as\n");
    printf("// Adafruit GFX's fontconvert does. GNU FreeFont is GPLv3 with the font exception.\n");
    printf("// Generated: don't edit.\n#pragma once\n#include \"../../bookfonts.h\"\n\n");
    printf("static const uint8_t %sBitmaps[] PROGMEM = {\n", name);
    for (int c = 0x20; c < 0x7F; c++) {
      Glyph *a = &ascii[c - 0x20];
      if (!raster(face, c, a, name, 1)) return 1;
      if (!fits(a) || a->offset > 0xFFFF) return fprintf(stderr, "%s: '%c' doesn't fit a GFXglyph\n", name, c), 1;
    }
    printf(" };\n\nstatic const GFXglyph %sGlyphs[] PROGMEM = {\n", name);
    for (int c = 0x20; c < 0x7F; c++) {
      Glyph *a = &ascii[c - 0x20];
      printf("  { %5d, %3d, %3d, %3d, %4d, %4d }%s  // '%c'\n", a->offset, a->w, a->h, a->adv, a->xo, a->yo, c < 0x7E ? "," : " ", c);
    }
    printf("};\n\nstatic const GFXfont %s PROGMEM = {(uint8_t *)%sBitmaps, (GFXglyph *)%sGlyphs, 0x20, 0x7E, %d};\n\n", name, name, name,
           (int)(face->size->metrics.height >> 6));
    asciiBytes = bytes + 8L * (0x7F - 0x20);
    bytes = 0, column = 0;
    printf("static const uint8_t %sExtBitmaps[] PROGMEM = {\n", name);
  } else {
    printf("// %s: %d letters beyond ASCII (glyphs.h), made by firmware/tools/fontext from\n", name, n);
    printf("// GNU FreeFont 20120503's %s at %d pt, 141 dpi, as Adafruit GFX's own fonts.\n", ttf, size);
    printf("// GNU FreeFont is GPLv3 with the font exception. Generated: don't edit.\n#pragma once\n#include \"../../glyphs.h\"\n\n");
    printf("static const uint8_t %sBitmaps[] PROGMEM = {\n", name);
  }
  for (int i = 0; i < n; i++) {
    if (!raster(face, codes[i], &g[i], name, book)) return 1;
    if (!fits(&g[i])) {
      fprintf(stderr, "%s: U+%04X doesn't fit an ExtGlyph\n", name, codes[i]);
      if (book) return 1;
    }
  }
  if (!bytes) printf("  0x00");
  printf(" };\n\nstatic const ExtGlyph %s%sGlyphs[] PROGMEM = {\n", name, book ? "Ext" : "");
  for (int i = 0; i < n; i++)
    printf("  { %6d, %3d, %3d, %3d, %4d, %4d }%s  // U+%04X\n", g[i].offset, g[i].w, g[i].h, g[i].adv, g[i].xo, g[i].yo,
           i + 1 < n ? "," : " ", codes[i]);
  if (!book) {
    printf("};\n\nstatic const ExtFont %s = {%sBitmaps, %sGlyphs};\n\n", name, name, name);
    printf("// About %ld bytes of flash\n", bytes + 12L * n);
  } else {
    printf("};\n\nstatic const ExtFont %sExt = {%sExtBitmaps, %sExtGlyphs};\n\n", name, name, name);
    // The line's measures, in pixels from the baseline: the tall and the hanging
    // lower-case letters, x and capital height, and the ink of every glyph
    // (an accented capital rises above a 'd').
    int top = 0, bottom = 0;
    for (int i = 0; i < 0x7F - 0x20 + n; i++) {
      const Glyph *q = i < 0x7F - 0x20 ? &ascii[i] : &g[i - (0x7F - 0x20)];
      if (!q->h) continue;
      if (-q->yo > top) top = -q->yo;
      if (q->yo + q->h > bottom) bottom = q->yo + q->h;
    }
    printf("static const BookFont %sFont PROGMEM = {&%s, &%sExt, %d, %d, %d, %d, %d, %d, %d, %d, %d};\n\n", name, name, name, size,
           (int)(face->size->metrics.height >> 6), reach(ascii, "bdfhkl", 0), reach(ascii, "gjpqy", 1), reach(ascii, "xvwz", 0),
           reach(ascii, "HIEX", 0), top, bottom, ascii[0].adv);
    printf("// About %ld bytes of flash\n", asciiBytes + bytes + 12L * n + 16 + 8 + 20);
  }
  FT_Done_FreeType(lib);
  return 0;
}
