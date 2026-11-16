// PDF, for the Books app: the text of a PDF, reflowed like any other book.
// The screen is too small to show a PDF's pages as they were laid out, so
// this reads what the pages say and works out the rest: which words make a
// line, which lines a paragraph, what is a heading, a running header or a
// page number, and where the chapters start.
//
// The parts, in the order of this file:
//   - a lexer over bytes in memory, and objects kept as the bytes they were
//     written in (a dictionary is looked through when a key is wanted: no
//     trees are built, nothing recurses);
//   - the file, read in blocks; the stream filters (Flate, LZW, ASCII85,
//     ASCIIHex, RunLength, the PNG and TIFF predictors), each a small
//     decoder pulling from the one before it;
//   - the document: the cross-reference table (classic, streams, hybrids,
//     /Prev chains; rebuilt by scanning the file when it is missing or
//     wrong), objects and object streams, the page tree, the outline;
//   - fonts: character codes to Unicode (/ToUnicode maps, the standard
//     encodings, /Differences by glyph name) and widths;
//   - the content streams' text operators, giving runs of text with a place
//     and a size on the page;
//   - the layout: runs to lines, columns, paragraphs, headings; headers,
//     footers and page numbers left out; words broken at a line's end joined;
//   - the job (a page a step) and the quick look for the library's list.
//
// Encrypted PDFs are refused (BE_ENCRYPTED); a PDF of pictures (a scan with
// no text layer) ends as BE_NO_TEXT; a scan with an OCR text layer reads.
// Not read: encodings only a font program knows (TeX's math fonts), fonts with CIDs
// and no /ToUnicode (their text is dropped), right-to-left and vertical text.
//
// Memory: everything big is in PSRAM, nothing static. The cross-reference
// table takes 8 bytes an object (at most MAX_OBJECTS), unpacked object
// streams at most OBJSTM_CACHE, fonts at most FONT_CACHE, a content stream
// WIN (one more for each form being read), a page's runs and text at most
// MAX_RUNS and PAGE_TEXT; the two pages after the one being written wait
// laid out, the two before it as their edge lines only (running headers are
// told by the pages around). A book of a few hundred pages: 100 to 500 KB.
//
// Plain C++ (no Arduino): tested on a computer by firmware/test/ebook/pdf_test.cpp.
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <string>

#include "bookcharset.h"
#include "bookformats.h"
#include "bookinflate.h"
#include "bookpdf.h"
#include "psalloc.h"

namespace {

const uint32_t MAX_OBJECTS = 262144;       // objects in the cross-reference table (8 bytes each)
const uint32_t MAX_OBJ_BYTES = 512 * 1024; // one object's own bytes (its stream apart)
const uint32_t MAX_OBJSTM = 768 * 1024;    // one object stream, unpacked
const uint32_t OBJSTM_CACHE = 384 * 1024;  // unpacked object streams kept (the newest even if bigger)
const uint32_t FONT_CACHE = 256 * 1024;   // fonts kept, MAX_FONTS at most
const int MAX_FONTS = 32;
const uint32_t MAX_CMAP = 20000;           // mappings of one font's /ToUnicode
const uint32_t MAX_PAGES = 20000;
const uint32_t MAX_RUNS = 2048;            // runs of text on a page before it is laid out in parts
const uint32_t PAGE_TEXT = 64 * 1024;      // a page's text before it is laid out in parts
const uint32_t MAX_OUTLINE = 2000;
const uint32_t WIN = 32 * 1024;            // a content stream's window
const uint32_t LOOK = 8 * 1024;            // what the lexer may count on being there (a longer string is cut)
const uint32_t STEP_BYTES = 16 * 1024;     // content read in one step
const uint32_t REBUILD_BYTES = 256 * 1024; // file scanned in one step when the table must be rebuilt
const uint32_t PAGE_BYTES = 24u << 20;     // content of one page: past it the page is left
const int MAX_FORM_DEPTH = 5;
const size_t MAX_Q = 120;                  // graphics states saved (q) at once

// ---------------------------------------------------------------- small things

uint32_t clampU32(double d) { return d != d || d < 0 ? 0 : d > 4e9 ? 4000000000u : (uint32_t)d; }
bool finite6(const float *m) {
  for (int i = 0; i < 6; i++)
    if (!(fabsf(m[i]) < 1e9f)) return false;
  return true;
}

bool isWs(int c) { return c == 0 || c == 9 || c == 10 || c == 12 || c == 13 || c == 32; }
bool isDelim(int c) { return c == '(' || c == ')' || c == '<' || c == '>' || c == '[' || c == ']' || c == '{' || c == '}' || c == '/' || c == '%'; }
bool isReg(int c) { return !isWs(c) && !isDelim(c); }
bool isDigit(int c) { return c >= '0' && c <= '9'; }
int hexVal(int c) { return isDigit(c) ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; }

void putUtf8(PsVec<char> &out, uint32_t cp) {
  if (cp < 0x80) out.push_back((char)cp);
  else if (cp < 0x800) out.push_back((char)(0xC0 | cp >> 6)), out.push_back((char)(0x80 | (cp & 0x3F)));
  else if (cp < 0x10000) out.push_back((char)(0xE0 | cp >> 12)), out.push_back((char)(0x80 | (cp >> 6 & 0x3F))), out.push_back((char)(0x80 | (cp & 0x3F)));
  else out.push_back((char)(0xF0 | cp >> 18)), out.push_back((char)(0x80 | (cp >> 12 & 0x3F))), out.push_back((char)(0x80 | (cp >> 6 & 0x3F))), out.push_back((char)(0x80 | (cp & 0x3F)));
}
void putUtf8(std::string &out, uint32_t cp) {
  if (cp < 0x80) out += (char)cp;
  else if (cp < 0x800) out += (char)(0xC0 | cp >> 6), out += (char)(0x80 | (cp & 0x3F));
  else if (cp < 0x10000) out += (char)(0xE0 | cp >> 12), out += (char)(0x80 | (cp >> 6 & 0x3F)), out += (char)(0x80 | (cp & 0x3F));
  else out += (char)(0xF0 | cp >> 18), out += (char)(0x80 | (cp >> 12 & 0x3F)), out += (char)(0x80 | (cp >> 6 & 0x3F)), out += (char)(0x80 | (cp & 0x3F));
}
// The character at p (valid UTF-8 of our own making); len gets its bytes.
uint32_t getUtf8(const char *p, const char *e, int &len) {
  uint8_t c = *p;
  int n = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
  if (n > e - p) n = 1;
  len = n;
  if (n == 1) return c;
  uint32_t cp = c & (0xFF >> (n + 1));
  for (int i = 1; i < n; i++) cp = cp << 6 | ((uint8_t)p[i] & 0x3F);
  return cp;
}
bool isLower(uint32_t c) { return (c >= 'a' && c <= 'z') || (c >= 0xDF && c <= 0xFF && c != 0xF7) || (c >= 0x100 && c < 0x180 && (c & 1)) || (c >= 0x3B1 && c <= 0x3C9) || (c >= 0x430 && c <= 0x45F); }
bool isUpper(uint32_t c) { return (c >= 'A' && c <= 'Z') || (c >= 0xC0 && c <= 0xDE && c != 0xD7) || (c >= 0x100 && c < 0x180 && !(c & 1)) || (c >= 0x391 && c <= 0x3A9) || (c >= 0x400 && c <= 0x42F); }
bool isLetter(uint32_t c) { return isLower(c) || isUpper(c) || c >= 0x3040; }

// ---------------------------------------------------------------- the lexer

// A piece of an object's bytes: a value (a name, a number, a whole dictionary...).
struct Sl {
  const uint8_t *p = nullptr;
  uint32_t n = 0;
};

enum Tk : uint8_t { T_EOF, T_NUM, T_NAME, T_STR, T_HEX, T_AO, T_AC, T_DO, T_DC, T_KW };

struct Lex {
  const uint8_t *p, *e;
  const uint8_t *t0 = nullptr;  // where the token started
  const uint8_t *ts = nullptr;  // its text: a name after the '/', a string inside its brackets, a keyword
  uint32_t tn = 0;
  double num = 0;
  bool isInt = false;
  Lex(const uint8_t *b, size_t n) : p(b), e(b + n) {}
  explicit Lex(Sl s) : p(s.p), e(s.p + s.n) {}
  bool kw(const char *s) const { return tn == strlen(s) && !memcmp(ts, s, tn); }

  void skipWs() {
    for (;;) {
      while (p < e && isWs(*p)) p++;
      if (p < e && *p == '%') {
        while (p < e && *p != '\n' && *p != '\r') p++;
        continue;
      }
      return;
    }
  }

  Tk next() {
    skipWs();
    t0 = ts = p, tn = 0;
    if (p >= e) return T_EOF;
    uint8_t c = *p;
    if (c == '/') {
      ts = ++p;
      while (p < e && isReg(*p)) p++;
      tn = p - ts;
      return T_NAME;
    }
    if (c == '(') {
      ts = ++p;
      int depth = 1;
      while (p < e) {
        if (*p == '\\') p++;
        else if (*p == '(') depth++;
        else if (*p == ')' && --depth == 0) break;
        p++;
      }
      if (p > e) p = e;
      tn = p - ts;
      if (p < e) p++;
      return T_STR;
    }
    if (c == '<') {
      if (p + 1 < e && p[1] == '<') return p += 2, T_DO;
      ts = ++p;
      while (p < e && *p != '>') p++;
      tn = p - ts;
      if (p < e) p++;
      return T_HEX;
    }
    if (c == '>' && p + 1 < e && p[1] == '>') return p += 2, T_DC;
    if (c == '[') return p++, T_AO;
    if (c == ']') return p++, T_AC;
    if (isDelim(c)) return tn = 1, p++, T_KW;  // a stray bracket
    while (p < e && isReg(*p)) p++;
    tn = p - ts;
    // A number? [+-] digits [. digits]
    const uint8_t *q = ts;
    bool neg = false, digits = false, dot = false;
    if (q < p && (*q == '+' || *q == '-')) neg = *q++ == '-';
    // In integers, one division at the end: the board's doubles are done in software.
    uint32_t whole = 0, frac = 0;
    int fd = 0;
    bool big = false;
    for (; q < p; q++) {
      if (isDigit(*q)) {
        digits = true;
        if (dot) {
          if (fd < 9) frac = frac * 10 + (*q - '0'), fd++;
        } else if (whole < 400000000u) whole = whole * 10 + (*q - '0');
        else big = true;
      } else if (*q == '.' && !dot) dot = true;
      else break;
    }
    if (q == p && (digits || dot)) {
      static const uint32_t POW10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
      double v = big ? 4e9 : whole;
      if (fd) v += (double)frac / POW10[fd];
      num = neg ? -v : v;
      isInt = !dot;
      return T_NUM;
    }
    return T_KW;
  }
};

// A name as it is meant ("#20" is a space), into out. False if it doesn't fit.
bool nameText(const uint8_t *s, uint32_t n, char *out, size_t cap) {
  size_t o = 0;
  for (uint32_t i = 0; i < n; i++) {
    int c = s[i];
    if (c == '#' && i + 2 < n + 0u && hexVal(s[i + 1]) >= 0 && hexVal(s[i + 2]) >= 0) c = hexVal(s[i + 1]) * 16 + hexVal(s[i + 2]), i += 2;
    if (o + 1 >= cap) return false;
    out[o++] = (char)c;
  }
  out[o] = 0;
  return true;
}
bool nameEq(const uint8_t *s, uint32_t n, const char *key) {
  size_t k = strlen(key);
  if (n == k && !memcmp(s, key, k)) return true;
  if (!memchr(s, '#', n)) return false;
  char buf[128];
  return nameText(s, n, buf, sizeof buf) && !strcmp(buf, key);
}

// The value at the lexer's place, whole: "12 0 R", a dictionary with all in
// it, a string. False at a closing bracket or the end. Brackets are counted,
// not followed: nothing recurses.
bool readValue(Lex &lx, Sl &v) {
  Tk t = lx.next();
  const uint8_t *start = lx.t0;
  if (t == T_EOF || t == T_AC || t == T_DC) return false;
  if (t == T_AO || t == T_DO) {
    int depth = 1;
    while (depth > 0) {
      t = lx.next();
      if (t == T_EOF) break;
      if (t == T_AO || t == T_DO) depth++;
      else if (t == T_AC || t == T_DC) depth--;
    }
  } else if (t == T_NUM && lx.isInt && lx.num >= 0) {
    Lex ahead = lx;  // "12 0 R"?
    if (ahead.next() == T_NUM && ahead.isInt && ahead.next() == T_KW && ahead.kw("R")) lx = ahead;
  }
  v.p = start, v.n = lx.p - start;
  return true;
}

bool dictGet(Sl d, const char *key, Sl &v) {
  Lex lx(d);
  if (lx.next() != T_DO) return false;
  for (;;) {
    Tk t = lx.next();
    if (t == T_EOF || t == T_DC) return false;
    if (t != T_NAME) continue;
    bool hit = nameEq(lx.ts, lx.tn, key);
    if (!readValue(lx, v)) return false;
    if (hit) return true;
  }
}

struct ArrIt {
  Lex lx;
  bool ok;
  explicit ArrIt(Sl a) : lx(a) { ok = lx.next() == T_AO; }
  bool next(Sl &v) {
    if (ok && readValue(lx, v)) return true;
    return ok = false;
  }
};

char kindOf(Sl v) {  // 'd' dictionary, 'a' array, 'n' name, 's' string, '0' number or reference, 'k' keyword, 0 nothing
  Lex lx(v);
  switch (lx.next()) {
    case T_DO: return 'd';
    case T_AO: return 'a';
    case T_NAME: return 'n';
    case T_STR:
    case T_HEX: return 's';
    case T_NUM: return '0';
    case T_KW: return 'k';
    default: return 0;
  }
}
bool vNum(Sl v, double &d) {
  Lex lx(v);
  if (lx.next() != T_NUM) return false;
  d = lx.num;
  return lx.next() == T_EOF;
}
bool vRef(Sl v, uint32_t &num) {
  Lex lx(v);
  if (lx.next() != T_NUM || !lx.isInt || lx.num < 0) return false;
  double n = lx.num;
  if (lx.next() != T_NUM || !lx.isInt || lx.next() != T_KW || !lx.kw("R")) return false;
  num = clampU32(n);
  return true;
}
bool vName(Sl v, char *out, size_t cap) {
  Lex lx(v);
  out[0] = 0;
  return lx.next() == T_NAME && nameText(lx.ts, lx.tn, out, cap);
}
bool vNameIs(Sl v, const char *name) {
  Lex lx(v);
  return lx.next() == T_NAME && nameEq(lx.ts, lx.tn, name);
}

// A string's bytes: the escapes of a literal one, the digits of a hex one. At most cap.
template <class Out>
void stringBytes(const uint8_t *s, uint32_t n, bool hex, Out &out, size_t cap) {
  if (hex) {
    int hi = -1;
    for (uint32_t i = 0; i < n && out.size() < cap; i++) {
      int h = hexVal(s[i]);
      if (h < 0) continue;
      if (hi < 0) hi = h;
      else out.push_back((char)(hi << 4 | h)), hi = -1;
    }
    if (hi >= 0 && out.size() < cap) out.push_back((char)(hi << 4));
    return;
  }
  for (uint32_t i = 0; i < n && out.size() < cap; i++) {
    int c = s[i];
    if (c == '\r') {  // a line end in a string is a line feed
      if (i + 1 < n && s[i + 1] == '\n') i++;
      c = '\n';
    } else if (c == '\\') {
      if (++i >= n) break;
      c = s[i];
      if (c >= '0' && c <= '7') {
        int v = c - '0';
        for (int k = 0; k < 2 && i + 1 < n && s[i + 1] >= '0' && s[i + 1] <= '7'; k++) v = v * 8 + (s[++i] - '0');
        c = v & 0xFF;
      } else if (c == 'n') c = '\n';
      else if (c == 'r') c = '\r';
      else if (c == 't') c = '\t';
      else if (c == 'b') c = '\b';
      else if (c == 'f') c = '\f';
      else if (c == '\r') {  // a backslash at a line's end: the string goes on
        if (i + 1 < n && s[i + 1] == '\n') i++;
        continue;
      } else if (c == '\n') continue;
    }
    out.push_back((char)c);
  }
}
template <class Out>
bool vString(Sl v, Out &out, size_t cap) {
  Lex lx(v);
  Tk t = lx.next();
  if (t != T_STR && t != T_HEX) return false;
  stringBytes(lx.ts, lx.tn, t == T_HEX, out, cap);
  return true;
}

// A text string (a title, a bookmark) as UTF-8: UTF-16 with a byte order
// mark, UTF-8 with one, or PDFDocEncoding.
std::string textString(const std::string &b) {
  std::string out;
  size_t n = b.size();
  const uint8_t *s = (const uint8_t *)b.data();
  if (n >= 2 && ((s[0] == 0xFE && s[1] == 0xFF) || (s[0] == 0xFF && s[1] == 0xFE))) {
    bool be = s[0] == 0xFE;
    for (size_t i = 2; i + 1 < n; i += 2) {
      uint32_t u = be ? s[i] << 8 | s[i + 1] : s[i + 1] << 8 | s[i];
      if (u >= 0xD800 && u < 0xDC00 && i + 3 < n) {
        uint32_t lo = be ? s[i + 2] << 8 | s[i + 3] : s[i + 3] << 8 | s[i + 2];
        if (lo >= 0xDC00 && lo < 0xE000) u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00), i += 2;
      }
      if (u == 0x1B) {  // a language tag: ESC ... ESC
        for (i += 2; i + 1 < n && !((be ? s[i] << 8 | s[i + 1] : s[i + 1] << 8 | s[i]) == 0x1B); i += 2) {}
        continue;
      }
      if (u >= 0xD800 && u < 0xE000) u = 0xFFFD;
      putUtf8(out, u < 0x20 ? 0x20 : u);
    }
    return out;
  }
  if (n >= 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) return b.substr(3);  // the flow writer mends what is broken
  for (size_t i = 0; i < n; i++) {
    uint32_t u = PDF_ENC_DOC[s[i]];
    if (u) putUtf8(out, u);
  }
  return out;
}

// ---------------------------------------------------------------- the file

// The book's file, read through a few cached blocks: objects are small and
// scattered, and what is read around one is often the next one wanted.
struct PdfFile {
  static const uint32_t BLK = 32768;
  static const int SLOTS = 4;
  FILE *f = nullptr;
  uint32_t size = 0;
  uint8_t *buf = nullptr;
  static const uint32_t NONE_POS = 0xFFFFFFFF;
  uint32_t tag[SLOTS], len[SLOTS], age[SLOTS], clock = 0;
  uint32_t pos = NONE_POS;  // where the next fread starts
  bool failed = false;

  ~PdfFile() {
    if (f) fclose(f);
    free(buf);
  }
  bool open(const char *path) {
    f = fopen(path, "rb");
    if (!f) return false;
    if (fseek(f, 0, SEEK_END) != 0) return false;
    long n = ftell(f);
    pos = NONE_POS;
    if (n <= 0 || (unsigned long)n >= 0x7FFFFFF0ul) return false;
    size = (uint32_t)n;
    buf = (uint8_t *)psMalloc(BLK * SLOTS);
    for (int i = 0; i < SLOTS; i++) tag[i] = 0xFFFFFFFF, len[i] = age[i] = 0;
    return buf != nullptr;
  }
  // The block that holds off: its bytes from off on, n of them (0 at the end or on an error).
  const uint8_t *at(uint32_t off, uint32_t &n) {
    n = 0;
    if (off >= size) return nullptr;
    uint32_t b = off / BLK;
    int slot = -1, old = 0;
    for (int i = 0; i < SLOTS; i++) {
      if (tag[i] == b) slot = i;
      if (age[i] < age[old]) old = i;
    }
    if (slot < 0) {
      slot = old;
      tag[slot] = 0xFFFFFFFF;
      // A seek costs on the card: none when the file is there already (reading on).
      if (pos != b * BLK && fseek(f, (long)(b * BLK), SEEK_SET) != 0) return failed = true, pos = NONE_POS, nullptr;
      len[slot] = fread(buf + slot * BLK, 1, BLK, f);
      pos = len[slot] == BLK ? b * BLK + BLK : NONE_POS;
      if (!len[slot]) return failed = true, nullptr;
      tag[slot] = b;
    }
    age[slot] = ++clock;
    uint32_t in = off - b * BLK;
    if (in >= len[slot]) return nullptr;
    n = len[slot] - in;
    return buf + slot * BLK + in;
  }
  size_t read(uint32_t off, uint8_t *dst, size_t n) {
    size_t got = 0;
    while (got < n) {
      uint32_t have;
      const uint8_t *p = at(off + got, have);
      if (!p) break;
      if (have > n - got) have = n - got;
      memcpy(dst + got, p, have);
      got += have;
    }
    return got;
  }
  int byte(uint32_t off) {
    uint32_t n;
    const uint8_t *p = at(off, n);
    return p ? *p : -1;
  }
  // Where needle first is in [from, to), or 0xFFFFFFFF.
  uint32_t find(const char *needle, uint32_t from, uint32_t to) {
    size_t k = strlen(needle);
    if (to > size) to = size;
    uint8_t tmp[16];
    while (from + k <= to) {
      uint32_t n;
      const uint8_t *p = at(from, n);
      if (!p) break;
      if (n > to - from) n = to - from;
      const uint8_t *hit = (const uint8_t *)memchr(p, needle[0], n);
      if (!hit) {
        from += n;
        continue;
      }
      from += hit - p;
      if (from + k > to) break;
      if (k <= sizeof tmp && read(from, tmp, k) == k && !memcmp(tmp, needle, k)) return from;
      from++;
    }
    return 0xFFFFFFFF;
  }
};

// ---------------------------------------------------------------- stream filters

// A decoder gives the bytes of a stream as they are asked for; each filter
// pulls from the one before it, the first from the file.
struct Dec {
  virtual ~Dec() {}
  virtual size_t read(uint8_t *out, size_t cap) = 0;
  // All of it, or at most cap.
  size_t readAll(uint8_t *out, size_t cap) {
    size_t got = 0, n;
    while (got < cap && (n = read(out + got, cap - got)) > 0) got += n;
    return got;
  }
};

struct FileDec : Dec {
  PdfFile &f;
  uint32_t off, left;
  FileDec(PdfFile &file, uint32_t o, uint32_t n) : f(file), off(o), left(n) {}
  size_t read(uint8_t *out, size_t cap) override {
    if (cap > left) cap = left;
    size_t n = cap ? f.read(off, out, cap) : 0;
    off += n, left -= n;
    if (n < cap) left = 0;
    return n;
  }
};

// What the filters share: the decoder before them and a buffer of its bytes.
struct Filter : Dec {
  static const size_t IN = 4096;
  Dec *up;
  uint8_t *in;
  size_t pos = 0, len = 0;
  bool eof = false;
  explicit Filter(Dec *u) : up(u) { in = (uint8_t *)psMalloc(IN); }
  ~Filter() override {
    delete up;
    free(in);
  }
  bool fill() {
    if (eof || !in) return false;
    pos = 0;
    len = up->read(in, IN);
    if (!len) eof = true;
    return len > 0;
  }
  int get() { return pos < len || fill() ? in[pos++] : -1; }
};

struct FlateDec : Filter {
  Inflater inf;
  bool done;
  explicit FlateDec(Dec *u) : Filter(u) { done = !inf.begin(true); }
  size_t read(uint8_t *out, size_t cap) override {
    size_t made = 0;
    while (!done && made < cap) {
      if (pos >= len && !fill()) {
        // Nothing more comes in: what the decoder still holds, then the end.
        size_t used = 0, n = 0;
        Inflater::Result r = inf.run(in, 0, used, out + made, cap - made, n);
        made += n;
        if (r != Inflater::MORE_OUTPUT || !n) done = true;
        continue;
      }
      size_t used = 0, n = 0;
      Inflater::Result r = inf.run(in + pos, len - pos, used, out + made, cap - made, n);
      pos += used, made += n;
      if (r == Inflater::DONE || r == Inflater::BAD) done = true;
      else if (!used && !n && r == Inflater::MORE_INPUT && pos < len) done = true;  // no way forward
    }
    return made;
  }
};

struct LzwDec : Filter {
  uint16_t *prefix;
  uint8_t *suffix, *stack;
  int early, bits = 9, next = 258, prev = -1, sp = 0;
  uint32_t acc = 0;
  int have = 0;
  bool done = false;
  LzwDec(Dec *u, int earlyChange) : Filter(u), early(earlyChange ? 1 : 0) {
    prefix = (uint16_t *)psMalloc(4096 * 2 + 4096 * 2);
    suffix = prefix ? (uint8_t *)(prefix + 4096) : nullptr;
    stack = suffix ? suffix + 4096 : nullptr;
    if (!prefix) done = true;
  }
  ~LzwDec() override { free(prefix); }
  size_t read(uint8_t *out, size_t cap) override {
    size_t made = 0;
    while (made < cap) {
      if (sp > 0) {
        out[made++] = stack[--sp];
        continue;
      }
      if (done) break;
      while (have < bits) {
        int c = get();
        if (c < 0) break;
        acc = acc << 8 | (uint32_t)c, have += 8;
      }
      if (have < bits) {
        done = true;
        break;
      }
      int code = (int)(acc >> (have - bits)) & ((1 << bits) - 1);
      have -= bits;
      if (code == 256) {
        bits = 9, next = 258, prev = -1;
        continue;
      }
      if (code == 257 || code > next || (code == next && prev < 0)) {
        done = true;
        break;
      }
      // The string of this code, backwards onto the stack.
      int c = code == next ? prev : code;
      int at = code == next ? 1 : 0;  // room for the repeated first byte
      sp = at;
      while (c >= 258 && sp < 4096) stack[sp++] = suffix[c], c = prefix[c];
      if (sp >= 4096) {
        sp = 0, done = true;
        break;
      }
      stack[sp++] = (uint8_t)c;
      if (at) stack[0] = (uint8_t)c;
      if (prev >= 0 && next < 4096) prefix[next] = (uint16_t)prev, suffix[next] = (uint8_t)c, next++;
      if (next + early >= (1 << bits) && bits < 12) bits++;
      prev = code;
    }
    return made;
  }
};

struct A85Dec : Filter {
  uint8_t pend[4];
  int np = 0, ip = 0;
  bool done = false;
  explicit A85Dec(Dec *u) : Filter(u) {}
  size_t read(uint8_t *out, size_t cap) override {
    size_t made = 0;
    while (made < cap) {
      if (ip < np) {
        out[made++] = pend[ip++];
        continue;
      }
      if (done) break;
      uint32_t v = 0;
      int k = 0, c;
      while (k < 5) {
        c = get();
        if (c < 0 || c == '~') {
          done = true;
          break;
        }
        if (isWs(c)) continue;
        if (c == 'z' && k == 0) {
          k = -1;
          break;
        }
        if (c < '!' || c > 'u') continue;
        v = v * 85 + (uint32_t)(c - '!'), k++;
      }
      ip = np = 0;
      if (k == -1) np = 4, v = 0;
      else if (k == 5) np = 4;
      else if (k >= 2) {
        for (int i = k; i < 5; i++) v = v * 85 + 84;
        np = k - 1;
      }
      for (int i = 0; i < 4; i++) pend[i] = (uint8_t)(v >> (24 - 8 * i));
    }
    return made;
  }
};

struct AHxDec : Filter {
  bool done = false;
  explicit AHxDec(Dec *u) : Filter(u) {}
  size_t read(uint8_t *out, size_t cap) override {
    size_t made = 0;
    while (made < cap && !done) {
      int hi = -1, c;
      while ((c = get()) >= 0 && c != '>') {
        int h = hexVal(c);
        if (h < 0) continue;
        if (hi < 0) hi = h;
        else {
          out[made++] = (uint8_t)(hi << 4 | h), hi = -1;
          break;
        }
      }
      if (c < 0 || c == '>') {
        done = true;
        if (hi >= 0) out[made++] = (uint8_t)(hi << 4);
      }
    }
    return made;
  }
};

struct RlDec : Filter {
  int lit = 0, rep = 0, val = 0;
  bool done = false;
  explicit RlDec(Dec *u) : Filter(u) {}
  size_t read(uint8_t *out, size_t cap) override {
    size_t made = 0;
    while (made < cap && !done) {
      if (rep > 0) out[made++] = (uint8_t)val, rep--;
      else if (lit > 0) {
        int c = get();
        if (c < 0) done = true;
        else out[made++] = (uint8_t)c, lit--;
      } else {
        int n = get();
        if (n < 0 || n == 128) done = true;
        else if (n < 128) lit = n + 1;
        else if ((val = get()) < 0) done = true;
        else rep = 257 - n;
      }
    }
    return made;
  }
};

// The PNG predictors (10 to 15: a tag before every row says which) and TIFF's (2).
struct PredDec : Filter {
  int predictor, bpp;
  uint32_t row;
  uint8_t *cur = nullptr, *prv = nullptr;
  uint32_t rp = 0, rn = 0;
  bool done = false;
  PredDec(Dec *u, int pred, int colors, int bpc, uint32_t columns) : Filter(u), predictor(pred) {
    bpp = (colors * bpc + 7) / 8;
    row = (uint32_t)(((uint64_t)colors * bpc * columns + 7) / 8);
    if (bpp < 1 || row < 1 || row > 65536 || (pred == 2 && bpc != 8)) done = true;
    else {
      cur = (uint8_t *)psMalloc(row * 2);
      if (cur) prv = cur + row, memset(cur, 0, row * 2);
      else done = true;
    }
  }
  ~PredDec() override { free(cur < prv ? cur : prv); }
  bool nextRow() {
    int tag = 0;
    if (predictor >= 10 && (tag = get()) < 0) return false;
    std::swap(cur, prv);
    uint32_t got = 0;
    while (got < row) {
      if (pos >= len && !fill()) break;
      uint32_t n = std::min<uint32_t>(row - got, len - pos);
      memcpy(cur + got, in + pos, n);
      got += n, pos += n;
    }
    if (!got) return false;
    memset(cur + got, 0, row - got);
    if (predictor == 2) tag = 1;  // TIFF's is PNG's "Sub"
    for (uint32_t i = 0; i < row; i++) {
      int a = i >= (uint32_t)bpp ? cur[i - bpp] : 0, b = prv[i], c = i >= (uint32_t)bpp ? prv[i - bpp] : 0, p = 0;
      if (tag == 1) p = a;
      else if (tag == 2) p = b;
      else if (tag == 3) p = (a + b) / 2;
      else if (tag == 4) {
        int pa = abs(b - c), pb = abs(a - c), pc = abs(a + b - 2 * c);
        p = pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
      }
      cur[i] = (uint8_t)(cur[i] + p);
    }
    rp = 0, rn = got;
    return true;
  }
  size_t read(uint8_t *out, size_t cap) override {
    size_t made = 0;
    while (made < cap && !done) {
      if (rp >= rn && !nextRow()) {
        done = true;
        break;
      }
      size_t n = std::min<size_t>(cap - made, rn - rp);
      memcpy(out + made, cur + rp, n);
      made += n, rp += n;
    }
    return made;
  }
};

// ---------------------------------------------------------------- the document

// An object, as the bytes of its value (a copy: the file's blocks move on),
// and where its stream's data is, if it has one.
struct Obj {
  PsVec<uint8_t> b;
  bool stream = false;
  uint32_t sOff = 0, sLen = 0;
  Sl v() const {
    Sl s;
    s.p = b.data(), s.n = (uint32_t)b.size();
    return s;
  }
  void set(const uint8_t *p, uint32_t n) {
    b.assign(p, p + n);
    stream = false;
    sOff = sLen = 0;
  }
  void clear() { b.clear(), stream = false; }
};

// Where an object is: in the file (type 1, at off), in an object stream
// (type 2: stream number off, its idx-th), free (0) or not known (0xFF).
struct XEnt {
  uint32_t off;
  uint16_t idx;
  uint8_t type, pad;
};

struct ObjStm {
  uint32_t num = 0, first = 0, age = 0;
  PsVec<uint8_t> data;
  PsVec<uint32_t> offs;  // the objects' numbers and offsets, in pairs
};

const uint32_t NONE = 0xFFFFFFFF;

struct Doc {
  PdfFile f;
  PsVec<XEnt> xref;
  uint32_t base = 0;            // bytes of junk before "%PDF-": the offsets count from there
  uint32_t root = 0, info = 0;  // object numbers, 0: none
  bool encrypted = false, rebuilt = false, broken = false;
  ObjStm *stm[4] = {nullptr, nullptr, nullptr, nullptr};
  struct {
    int phase = 0;  // 0 not begun, 1 looking for objects, 2 for the catalog, 3 in object streams, 4 done
    uint32_t at = 0, num = 0;
  } rb;
  PsVec<uint32_t> stms;   // object streams found while rebuilding
  PsVec<uint32_t> loose;  // and pages: a file cut short may have lost its page tree
  bool needRebuild = false;
  uint32_t clock = 0;
  bool loading = false;          // reading the tables: no rebuilding then
  // The quick look (probe) wants an object or two, not the table: their entries end up in wantEnt.
  uint32_t wantNum = NONE, want[2] = {NONE, NONE};
  XEnt wantEnt[2] = {{0, 0, 0xFF, 0}, {0, 0, 0xFF, 0}};

  ~Doc() {
    for (ObjStm *s : stm) delete s;
  }

  XEnt entry(uint32_t num) const {
    if (wantNum != NONE) {
      for (int i = 0; i < 2; i++)
        if (num == want[i]) return wantEnt[i];
      return XEnt{0, 0, 0xFF, 0};
    }
    return num < xref.size() ? xref[num] : XEnt{0, 0, 0xFF, 0};
  }
  // First seen wins: the table read first is the newest.
  void setEntry(uint32_t num, uint8_t type, uint32_t off, uint32_t idx) {
    if (wantNum != NONE) {
      for (int i = 0; i < 2; i++)
        if (num == want[i] && wantEnt[i].type == 0xFF) wantEnt[i] = XEnt{off, (uint16_t)idx, type, 0};
      return;
    }
    if (num >= MAX_OBJECTS) return;
    if (num >= xref.size()) xref.resize(num + 1, XEnt{0, 0, 0xFF, 0});
    if (xref[num].type == 0xFF) xref[num] = XEnt{off, (uint16_t)idx, type, 0};
  }

  // ---- reading an object's value where it is written

  // "N G obj <value> [stream]" at off. num: the number it must have (NONE: any; got says).
  // A stream's length is left to fixLength(), which needs the table.
  bool readAt(uint32_t off, uint32_t num, Obj &o, uint32_t *got = nullptr) {
    o.clear();
    for (uint32_t n = 1024;; n *= 8) {
      if (n > MAX_OBJ_BYTES) n = MAX_OBJ_BYTES;
      if (off >= f.size) return false;
      uint32_t avail = std::min<uint32_t>(n, f.size - off);
      o.b.resize(avail);
      if (f.read(off, o.b.data(), avail) != avail) return false;
      Lex lx(o.b.data(), avail);
      if (lx.next() != T_NUM || !lx.isInt) return false;
      uint32_t onum = clampU32(lx.num);
      if (lx.next() != T_NUM || lx.next() != T_KW || !lx.kw("obj")) return false;
      if (num != NONE && onum != num) return false;
      if (got) *got = onum;
      Sl v;
      bool have = readValue(lx, v);
      Lex after = lx;
      Tk t = after.next();
      bool more = off + avail < f.size;
      if ((!have || after.p >= lx.e) && more && n < MAX_OBJ_BYTES) continue;  // cut off by the read: read more
      if (!have) return false;
      bool isStream = t == T_KW && after.kw("stream");
      uint32_t data = off + (uint32_t)(after.p - o.b.data());
      memmove(o.b.data(), v.p, v.n);
      o.b.resize(v.n);
      if (isStream) {
        // The data starts after the line end that follows "stream".
        int c = f.byte(data);
        if (c == '\r') c = f.byte(++data);
        if (c == '\n') data++;
        o.stream = true;
        o.sOff = data;
        o.sLen = 0;
      }
      return true;
    }
  }

  // ---- objects by number

  bool get(uint32_t num, Obj &o, int depth = 0) {
    o.clear();
    if (depth > 4) return false;
    XEnt e = entry(num);
    if (e.type == 1) {
      if (readAt(base + e.off, num, o) || (base && readAt(e.off, num, o))) return fixLength(o, depth);
      if (!rebuilt && !loading && wantNum == NONE && rebuild()) return get(num, o, depth + 1);
      return false;
    }
    if (e.type == 2) return fromStm(e.off, e.idx, num, o, depth);
    return false;
  }

  // The stream's /Length, checked against where "endstream" is; found by looking when it is wrong.
  bool fixLength(Obj &o, int depth) {
    if (!o.stream) return true;
    uint32_t data = o.sOff, len = NONE;
    Sl lv;
    double d;
    if (dictGet(o.v(), "Length", lv)) {
      uint32_t ref;
      if (vNum(lv, d)) len = clampU32(d);
      else if (vRef(lv, ref) && depth < 3) {
        Obj lo;
        if (get(ref, lo, depth + 1) && vNum(lo.v(), d)) len = clampU32(d);
      }
    }
    if (len != NONE && data + (uint64_t)len <= f.size) {
      uint8_t tail[24];
      size_t n = f.read(data + len, tail, sizeof tail);
      size_t i = 0;
      while (i < n && isWs(tail[i])) i++;
      if (n - i >= 9 && !memcmp(tail + i, "endstream", 9)) return o.sLen = len, true;
    }
    // Wrong or missing: the stream ends before the next "endstream".
    uint32_t end = f.find("endstream", data, f.size);
    if (end == NONE) end = f.size;
    if (end > data && f.byte(end - 1) == '\n') end--;
    if (end > data && f.byte(end - 1) == '\r') end--;
    o.sLen = end - data;
    return true;
  }

  bool fromStm(uint32_t snum, uint32_t idx, uint32_t num, Obj &o, int depth) {
    ObjStm *s = objStm(snum, depth);
    if (!s || idx >= s->offs.size() / 2) {
      // The index may be off: look for the number.
      if (!s) return false;
      for (idx = 0; idx < s->offs.size() / 2 && s->offs[idx * 2] != num; idx++) {}
      if (idx >= s->offs.size() / 2) return false;
    }
    if (s->offs[idx * 2] != num) {
      uint32_t i = 0;
      for (; i < s->offs.size() / 2 && s->offs[i * 2] != num; i++) {}
      if (i < s->offs.size() / 2) idx = i;
    }
    uint64_t at = (uint64_t)s->first + s->offs[idx * 2 + 1];
    if (at >= s->data.size()) return false;
    uint32_t end = (uint32_t)s->data.size();
    Lex lx(s->data.data() + at, end - (size_t)at);
    Sl v;
    if (!readValue(lx, v)) return false;
    o.set(v.p, v.n);
    return true;
  }

  ObjStm *objStm(uint32_t snum, int depth) {
    int old = 0;
    for (int i = 0; i < 4; i++) {
      if (stm[i] && stm[i]->num == snum) return stm[i]->age = ++clock, stm[i];
      if (!stm[i] || (stm[old] && stm[i]->age < stm[old]->age)) old = i;
    }
    XEnt e = entry(snum);
    if (e.type != 1) return nullptr;  // an object stream inside another: not allowed
    Obj o;
    if (!get(snum, o, depth + 1) || !o.stream) return nullptr;
    Sl v;
    double n = 0, first = 0;
    if (!dictGet(o.v(), "N", v) || !vNum(v, n) || !dictGet(o.v(), "First", v) || !vNum(v, first)) return nullptr;
    if (n < 0 || n > 100000 || first < 0) return nullptr;
    ObjStm *s = new ObjStm();
    if (!decodeAll(o, s->data, MAX_OBJSTM)) {
      delete s;
      return nullptr;
    }
    s->num = snum, s->first = clampU32(first), s->age = ++clock;
    Lex lx(s->data.data(), std::min<size_t>(s->data.size(), s->first));
    for (uint32_t i = 0; i < (uint32_t)n; i++) {
      if (lx.next() != T_NUM) break;
      uint32_t on = clampU32(lx.num);
      if (lx.next() != T_NUM) break;
      s->offs.push_back(on), s->offs.push_back(clampU32(lx.num));
    }
    // Keep the cache within OBJSTM_CACHE: drop the oldest until it fits.
    size_t total = s->data.size();
    for (ObjStm *t : stm)
      if (t) total += t->data.size();
    for (int round = 0; round < 4 && total > OBJSTM_CACHE; round++) {
      int o2 = -1;
      for (int i = 0; i < 4; i++)
        if (stm[i] && (o2 < 0 || stm[i]->age < stm[o2]->age)) o2 = i;
      if (o2 < 0) break;
      total -= stm[o2]->data.size();
      delete stm[o2];
      stm[o2] = nullptr;
    }
    for (int i = 0; i < 4; i++)
      if (!stm[i]) return stm[i] = s;
    delete stm[old];
    return stm[old] = s;
  }

  // A value, following references (an indirect object holding another reference too).
  bool resolve(Sl v, Obj &hold, Sl &out) {
    for (int i = 0; i < 4; i++) {
      uint32_t ref;
      if (!vRef(v, ref)) return out = v, true;
      if (!get(ref, hold)) return false;
      v = hold.v();
    }
    return false;
  }
  // dict[key], resolved.
  bool getKey(Sl dict, const char *key, Obj &hold, Sl &out) {
    Sl v;
    return dictGet(dict, key, v) && resolve(v, hold, out);
  }
  bool getNum(Sl dict, const char *key, double &d) {
    Obj h;
    Sl v;
    return getKey(dict, key, h, v) && vNum(v, d);
  }

  // ---- streams

  // A decoder of the stream's data through its filters; nullptr when one of
  // them is a picture's (DCT, JPX, CCITT, JBIG2) or not known.
  Dec *open(const Obj &o) {
    if (!o.stream) return nullptr;
    Dec *d = new FileDec(f, o.sOff, o.sLen);
    Obj fh, ph;
    Sl fv, pv;
    bool hasF = getKey(o.v(), "Filter", fh, fv);
    bool hasP = getKey(o.v(), "DecodeParms", ph, pv) || getKey(o.v(), "DP", ph, pv);
    if (!hasF) return d;
    Sl one[8], parm[8];
    int nf = 0, np = 0;
    if (kindOf(fv) == 'n') one[nf++] = fv;
    else if (kindOf(fv) == 'a') {
      ArrIt it(fv);
      Sl e;
      while (nf < 8 && it.next(e)) one[nf++] = e;
    }
    if (hasP && kindOf(pv) == 'd') parm[np++] = pv;
    else if (hasP && kindOf(pv) == 'a') {
      ArrIt it(pv);
      Sl e;
      while (np < 8 && it.next(e)) parm[np++] = e;
    }
    if (nf > 4) return delete d, nullptr;
    Obj hold;
    for (int i = 0; i < nf; i++) {
      char name[32];
      if (!vName(one[i], name, sizeof name)) return delete d, nullptr;
      Sl p;
      bool haveP = i < np && resolve(parm[i], hold, p) && kindOf(p) == 'd';
      if (!strcmp(name, "FlateDecode") || !strcmp(name, "Fl") || !strcmp(name, "LZWDecode") || !strcmp(name, "LZW")) {
        double pred = 1, colors = 1, bpc = 8, cols = 1, early = 1;
        if (haveP) {
          Sl x;
          if (dictGet(p, "Predictor", x)) vNum(x, pred);
          if (dictGet(p, "Colors", x)) vNum(x, colors);
          if (dictGet(p, "BitsPerComponent", x)) vNum(x, bpc);
          if (dictGet(p, "Columns", x)) vNum(x, cols);
          if (dictGet(p, "EarlyChange", x)) vNum(x, early);
        }
        if (name[0] == 'F') d = new FlateDec(d);
        else d = new LzwDec(d, early != 0);
        if (pred >= 2) {
          if (colors < 1 || colors > 32 || (bpc != 1 && bpc != 2 && bpc != 4 && bpc != 8 && bpc != 16) || cols < 1 || cols > 65536) return delete d, nullptr;
          d = new PredDec(d, pred >= 10 ? 10 : (int)pred, (int)colors, (int)bpc, (uint32_t)cols);
          if (pred != 2 && pred < 10) return delete d, nullptr;
        }
      } else if (!strcmp(name, "ASCII85Decode") || !strcmp(name, "A85")) d = new A85Dec(d);
      else if (!strcmp(name, "ASCIIHexDecode") || !strcmp(name, "AHx")) d = new AHxDec(d);
      else if (!strcmp(name, "RunLengthDecode") || !strcmp(name, "RL")) d = new RlDec(d);
      else return delete d, nullptr;
    }
    return d;
  }
  // The whole stream unpacked, at most cap bytes. False: it can't be read, or is bigger.
  template <class V>
  bool decodeAll(const Obj &o, V &out, size_t cap) {
    Dec *d = open(o);
    if (!d) return false;
    out.clear();
    size_t chunk = 16384;
    bool ok = true;
    for (;;) {
      size_t at = out.size();
      if (at >= cap) {
        uint8_t probe;
        ok = d->read(&probe, 1) == 0;
        break;
      }
      size_t take = std::min(chunk, cap - at);
      out.resize(at + take);
      size_t n = d->readAll(out.data() + at, take);
      out.resize(at + n);
      if (n < take) break;
    }
    delete d;
    return ok;
  }

  // ---- the cross-reference table

  // Lexes the file from off in a window, for the tables.
  struct FileLex {
    Doc &d;
    uint32_t base, len = 0;
    PsVec<uint8_t> buf;
    Lex lx{nullptr, 0};
    FileLex(Doc &doc, uint32_t off) : d(doc), base(off) { load(off); }
    void load(uint32_t off) {
      base = off;
      uint32_t n = off < d.f.size ? std::min<uint32_t>(32768, d.f.size - off) : 0;
      buf.resize(n);
      len = (uint32_t)d.f.read(off, buf.data(), n);
      lx = Lex(buf.data(), len);
    }
    uint32_t pos() const { return base + (uint32_t)(lx.p - buf.data()); }
    // Keeps at least 512 bytes ahead, unless the file ends.
    void ensure() {
      if (lx.e - lx.p < 512 && base + len < d.f.size) load(pos());
    }
    Tk next() {
      ensure();
      return lx.next();
    }
  };

  // A classic table ("xref" at off). False: it isn't one.
  bool classic(uint32_t off, uint32_t &prev, uint32_t &xstm) {
    FileLex fl(*this, off);
    if (fl.next() != T_KW || !fl.lx.kw("xref")) return false;
    for (;;) {
      Tk t = fl.next();
      if (t == T_KW && fl.lx.kw("trailer")) break;
      if (t != T_NUM) return false;
      double start = fl.lx.num;
      if (fl.next() != T_NUM) return false;
      double count = fl.lx.num;
      if (start < 0 || count < 0 || start + count > MAX_OBJECTS) return false;
      uint32_t s = (uint32_t)start, c = (uint32_t)count;
      if (wantNum != NONE && (wantNum < s || wantNum >= s + c)) {
        // Not the object looked for: jump over the rows (20 bytes each).
        uint32_t skip = fl.pos();
        while (isWs(f.byte(skip))) skip++;
        fl.load(skip + c * 20);
        continue;
      }
      for (uint32_t i = 0; i < c; i++) {
        if (fl.next() != T_NUM) return false;
        double o = fl.lx.num;
        if (fl.next() != T_NUM) return false;
        if (fl.next() != T_KW) return false;
        bool used = fl.lx.tn == 1 && fl.lx.ts[0] == 'n';
        uint32_t num = s + i;
        if (num == 0 && !used) continue;
        setEntry(num, used ? 1 : 0, clampU32(o), 0);
      }
    }
    // The trailer.
    uint32_t at = fl.pos();
    Obj tr;
    PsVec<uint8_t> &b = tr.b;
    b.resize(std::min<uint32_t>(65536, f.size - std::min(f.size, at)));
    b.resize(f.read(at, b.data(), b.size()));
    Lex lx(b.data(), b.size());
    Sl v;
    if (!readValue(lx, v) || kindOf(v) != 'd') return false;
    trailer(v);
    double d;
    if (dictGet(v, "Prev", v) && vNum(v, d)) prev = clampU32(d);
    Sl x;
    Lex lx2(b.data(), b.size());
    readValue(lx2, x);
    if (dictGet(x, "XRefStm", v) && vNum(v, d)) xstm = clampU32(d);
    return true;
  }

  void trailer(Sl t) {
    Sl v;
    uint32_t r;
    if (!root && dictGet(t, "Root", v) && vRef(v, r)) root = r;
    if (!info && dictGet(t, "Info", v) && vRef(v, r)) info = r;
    if (dictGet(t, "Encrypt", v) && kindOf(v)) encrypted = true;
  }

  // A cross-reference stream ("N G obj << /Type /XRef ... >> stream" at off).
  bool xstream(uint32_t off, uint32_t &prev) {
    Obj o;
    if (!readAt(off, NONE, o) && !readAt(off + base, NONE, o)) return false;
    if (!o.stream) return false;
    Sl d = o.v(), v;
    if (!dictGet(d, "Type", v) || !vNameIs(v, "XRef")) return false;
    // /Length may be a reference into a table not read yet: then the search for "endstream" finds the end.
    fixLength(o, 0);
    double num;
    trailer(d);
    double w[3] = {0, 0, 0}, size = 0;
    if (!dictGet(d, "W", v)) return false;
    {
      ArrIt it(v);
      Sl e;
      for (int i = 0; i < 3 && it.next(e); i++)
        if (!vNum(e, w[i]) || w[i] < 0 || w[i] > 8) return false;
    }
    if (dictGet(d, "Size", v)) vNum(v, size);
    double ranges[32];
    int nr = 0;
    if (dictGet(d, "Index", v) && kindOf(v) == 'a') {
      ArrIt it(v);
      Sl e;
      while (nr < 32 && it.next(e) && vNum(e, ranges[nr])) nr++;
      nr &= ~1;
    } else ranges[0] = 0, ranges[1] = size, nr = 2;
    if (dictGet(d, "Prev", v) && vNum(v, num)) prev = clampU32(num);
    int row = (int)(w[0] + w[1] + w[2]);
    if (row <= 0) return false;
    Dec *dec = open(o);
    if (!dec) return false;
    uint8_t r[24];
    for (int k = 0; k < nr; k += 2) {
      if (ranges[k] < 0 || ranges[k + 1] < 0 || ranges[k] + ranges[k + 1] > MAX_OBJECTS) break;
      uint32_t s = (uint32_t)ranges[k], c = (uint32_t)ranges[k + 1];
      bool done = false;
      for (uint32_t i = 0; i < c; i++) {
        if (dec->readAll(r, row) != (size_t)row) {
          done = true;
          break;
        }
        uint64_t field[3] = {0, 0, 0};
        int at = 0;
        for (int j = 0; j < 3; j++)
          for (int b2 = 0; b2 < (int)w[j]; b2++) field[j] = field[j] << 8 | r[at++];
        uint32_t type = w[0] == 0 ? 1 : (uint32_t)field[0];
        if (type == 1) setEntry(s + i, 1, (uint32_t)std::min<uint64_t>(field[1], 0xFFFFFFFF), 0);
        else if (type == 2) setEntry(s + i, 2, (uint32_t)std::min<uint64_t>(field[1], 0xFFFFFFFF), (uint32_t)std::min<uint64_t>(field[2], 65535));
        else if (type == 0) setEntry(s + i, 0, 0, 0);
      }
      if (done) break;
    }
    delete dec;
    return true;
  }

  // From startxref back through /Prev. False: there's no table to be read.
  bool readTables() {
    uint32_t tailFrom = f.size > 2048 ? f.size - 2048 : 0, sx = NONE, at = tailFrom;
    for (;;) {
      uint32_t hit = f.find("startxref", at, f.size);
      if (hit == NONE) break;
      sx = hit, at = hit + 1;
    }
    if (sx == NONE) return false;
    uint8_t tmp[40];
    size_t n = f.read(sx + 9, tmp, sizeof tmp);
    Lex lx(tmp, n);
    if (lx.next() != T_NUM) return false;
    uint32_t off = clampU32(lx.num);
    uint32_t seen[64];
    int nseen = 0, sections = 0;
    while (off && nseen < 64) {
      for (int i = 0; i < nseen; i++)
        if (seen[i] == off) return sections > 0;
      seen[nseen++] = off;
      uint32_t prev = 0, xs = 0;
      bool ok = classic(off + base, prev, xs) || classic(off, prev, xs);
      if (ok) {
        uint32_t ignore = 0;
        if (xs) xstream(xs, ignore);  // a hybrid file: its stream before the older tables
      } else if (!xstream(off, prev)) return sections > 0;
      sections++;
      off = prev;
    }
    return sections > 0;
  }

  // No table, or a wrong one: every "N G obj" in the file, the later
  // winning; then what object streams hold, for the numbers not found. In
  // slices (a long file takes a while on the card): rebuildStep() does about
  // budget bytes' worth and says when it's done; rebuild() all of it.
  bool rebuild() {
    while (!rebuildStep(0xFFFFFFFF)) {}
    return root != 0;
  }
  bool rebuildStep(uint32_t budget) {
    if (rb.phase == 0) {
      rebuilt = true;
      xref.clear();
      stms.clear(), loose.clear();
      root = 0;
      rb.at = 0, rb.num = 1, rb.phase = 1;
    }
    uint32_t spent = 0;
    if (rb.phase == 1) {
      uint8_t tmp[64];
      uint32_t stop = rb.at + std::min<uint32_t>(budget, f.size - rb.at);
      for (;;) {
        uint32_t hit = f.find("obj", rb.at, stop);
        if (hit == NONE) {
          // Not found up to stop: the next slice looks again from just before it ("ob|j").
          rb.at = stop >= f.size ? f.size : stop - 2;
          if (stop >= f.size) rb.phase = 2;
          return false;
        }
        rb.at = hit + 3;
        int after = f.byte(hit + 3);
        if (after >= 0 && isReg(after)) continue;
        // Back over " G N" before it.
        uint32_t s = hit > 24 ? hit - 24 : 0;
        size_t n = f.read(s, tmp, hit - s);
        int i = (int)n - 1;
        while (i >= 0 && isWs(tmp[i])) i--;
        int g1 = i;
        while (i >= 0 && isDigit(tmp[i])) i--;
        if (i == g1 || i < 0 || !isWs(tmp[i])) continue;
        while (i >= 0 && isWs(tmp[i])) i--;
        int n1 = i;
        while (i >= 0 && isDigit(tmp[i])) i--;
        if (i == n1 || (i >= 0 && isReg(tmp[i]))) continue;
        uint32_t num = 0;
        for (int k = i + 1; k <= n1 && num < 100000000; k++) num = num * 10 + (tmp[k] - '0');
        if (num == 0 || num >= MAX_OBJECTS) continue;
        if (num >= xref.size()) xref.resize(num + 1, XEnt{0, 0, 0xFF, 0});
        xref[num] = XEnt{s + (uint32_t)(i + 1), 0, 1, 0};  // later ones win: incremental updates
      }
    }
    if (rb.phase == 2) {
      // The catalog, and the object streams.
      for (; rb.num < xref.size() && spent < budget; rb.num++) {
        if (xref[rb.num].type != 1) continue;
        Obj o;
        if (!readAt(xref[rb.num].off, rb.num, o)) continue;
        spent += (uint32_t)o.b.size() + 64;
        Sl v;
        if (dictGet(o.v(), "Type", v)) {
          if (vNameIs(v, "Catalog")) root = rb.num;
          else if (vNameIs(v, "ObjStm")) stms.push_back(rb.num);
          else if (vNameIs(v, "Page") && loose.size() < MAX_PAGES) loose.push_back(rb.num);
        }
      }
      if (rb.num < xref.size()) return false;
      rb.phase = 3, rb.num = 0;
    }
    if (rb.phase == 3) {
      // What the object streams hold, for the numbers not found in the file itself.
      for (; rb.num < stms.size() && spent < budget; rb.num++) {
        uint32_t snum = stms[rb.num];
        ObjStm *st = objStm(snum, 0);
        if (!st) continue;
        spent += (uint32_t)st->data.size();
        for (size_t i = 0; i + 1 < st->offs.size(); i += 2) {
          uint32_t num = st->offs[i];
          if (num >= MAX_OBJECTS) continue;
          if (num >= xref.size()) xref.resize(num + 1, XEnt{0, 0, 0xFF, 0});
          if (xref[num].type == 0xFF) xref[num] = XEnt{snum, (uint16_t)(i / 2), 2, 0};
          Obj o;
          Sl v;
          if (!root && fromStm(snum, (uint32_t)(i / 2), num, o, 0) && dictGet(o.v(), "Type", v) && vNameIs(v, "Catalog")) root = num;
        }
      }
      if (rb.num < stms.size()) return false;
      // A trailer still says which is the document information, and if it's encrypted.
      uint32_t t = f.find("trailer", f.size > 65536 ? f.size - 65536 : 0, f.size);
      if (t != NONE) {
        Obj tr;
        tr.b.resize(4096);
        tr.b.resize(f.read(t + 7, tr.b.data(), 4096));
        Lex lx(tr.b.data(), tr.b.size());
        Sl v;
        if (readValue(lx, v)) trailer(v);
      }
      stms.clear();
      rb.phase = 4;
    }
    return true;
  }

  // Opens the file and reads the tables; rebuilds them when they don't hold
  // together. probeInfo: only the trailers, and where /Info is.
  BookErr load(const char *path, bool probeInfo) {
    if (!f.open(path)) return BE_OPEN;
    // "%PDF-" near the start (some tools put junk before it: the offsets count from it then).
    uint32_t h = f.find("%PDF-", 0, 1024);
    if (h == NONE) return BE_FORMAT;
    base = h;
    if (probeInfo) wantNum = 0;  // nothing wanted yet: the trailers only
    loading = true;
    bool ok = readTables();
    if (probeInfo) {
      if (encrypted || !ok || !info) return loading = false, ok ? BE_OK : BE_DAMAGED;
      want[0] = info;
      readTables();
      if (wantEnt[0].type == 2) want[1] = wantEnt[0].off, readTables();  // in an object stream: where that is
      loading = false;
      return BE_OK;
    }
    loading = false;
    // A table that points at the wrong places is as good as none: the job rebuilds it, a slice a step.
    Obj o;
    Sl v;
    if (!ok || !root || !get(root, o) || !dictGet(o.v(), "Pages", v)) needRebuild = true;
    return encrypted ? BE_ENCRYPTED : BE_OK;
  }
};


// ---------------------------------------------------------------- fonts

// A glyph's name to Unicode: the table, a one-letter name, "uniXXXX",
// "uXXXX[XX]", a ligature written "f_f_i", a variant "a.sc". 0: not known.
uint32_t glyphCp(const char *name) {
  char base[64];
  size_t n = 0;
  for (; name[n] && name[n] != '.' && n + 1 < sizeof base; n++) base[n] = name[n];
  base[n] = 0;
  if (!n) return 0;
  if (strchr(base, '_')) {  // a ligature of its parts
    static const struct { const char *name; uint32_t cp; } LIG[] = {
        {"f_f", 0xFB00}, {"f_i", 0xFB01}, {"f_l", 0xFB02}, {"f_f_i", 0xFB03}, {"f_f_l", 0xFB04}, {"s_t", 0xFB06}};
    for (auto &l : LIG)
      if (!strcmp(base, l.name)) return l.cp;
    *strchr(base, '_') = 0;
    return glyphCp(base);
  }
  if (n == 1) return isalpha((unsigned char)base[0]) ? (uint32_t)base[0] : 0;
  int lo = 0, hi = PDF_GLYPH_COUNT - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2, c = strcmp(base, PDF_GLYPHS[mid].name);
    if (!c) return PDF_GLYPHS[mid].cp;
    if (c < 0) hi = mid - 1;
    else lo = mid + 1;
  }
  const char *hex = nullptr;
  size_t digits = 0;
  if (!strncmp(base, "uni", 3) && n >= 7) hex = base + 3, digits = 4;
  else if (base[0] == 'u' && n >= 5 && n <= 7) hex = base + 1, digits = n - 1;
  if (!hex) return 0;
  uint32_t cp = 0;
  for (size_t i = 0; i < digits; i++) {
    int h = hexVal(hex[i]);
    if (h < 0 || (hex[i] >= 'a' && hex[i] <= 'f')) return 0;  // the forms are upper case
    cp = cp << 4 | (uint32_t)h;
  }
  return cp <= 0x10FFFF && !(cp >= 0xD800 && cp < 0xE000) ? cp : 0;
}

// Unusable as text: nothing, a control, the private use area, the replacement character.
bool badTarget(uint32_t u) { return u < 0x20 || (u >= 0xE000 && u < 0xF900) || u == 0xFFFD || u >= 0xF0000 || (u >= 0xD800 && u < 0xE000); }

const uint32_t MULTI = 0x80000000;  // a mapping to several characters: the rest is an index in the pool

struct CMapEnt {
  uint32_t lo, hi, dst;
};
struct CsRange {
  uint32_t lo, hi;
  uint8_t len;
};
struct WRange {
  uint32_t lo, hi;
  float w;
};

struct Font {
  uint32_t key = 0, age = 0;
  bool simple = true, bold = false, italic = false, ucs2 = false, hasMap = false;
  uint8_t fixedLen = 1;     // bytes a code; 0: by the code space ranges
  float scale = 0.001f;     // glyph widths to text space
  float dw = 1000, missing = 0;
  bool hasWid = false;
  uint16_t uni[256];
  float wid[256];
  PsVec<CsRange> cs;
  PsVec<CMapEnt> map;
  PsVec<uint32_t> pool;
  PsVec<WRange> w;

  size_t bytes() const { return sizeof(Font) + cs.capacity() * sizeof(CsRange) + map.capacity() * sizeof(CMapEnt) + pool.capacity() * 4 + w.capacity() * sizeof(WRange); }

  // The code at s; how many bytes it took.
  int code(const uint8_t *s, size_t n, uint32_t &c) const {
    if (fixedLen == 1 || n < 2) return c = s[0], 1;
    if (fixedLen == 2) return c = (uint32_t)s[0] << 8 | s[1], 2;
    uint32_t v = 0;
    for (int len = 1; len <= 4 && (size_t)len <= n; len++) {
      v = v << 8 | s[len - 1];
      for (const CsRange &r : cs)
        if (r.len == len && v >= r.lo && v <= r.hi) return c = v, len;
    }
    return c = s[0], 1;
  }

  // The characters of a code, at most cap; 0: none known.
  int chars(uint32_t c, uint32_t *out, int cap) const {
    int k = fromMap(c, out, cap);
    if (k) return k;
    if (ucs2) return out[0] = c, 1;
    if (simple && c < 256 && uni[c]) return out[0] = uni[c], 1;
    return 0;
  }
  // What the /ToUnicode map (or a ligature's name) says of a code; 0: nothing.
  int fromMap(uint32_t c, uint32_t *out, int cap) const {
    if (hasMap) {
      size_t lo = 0, hi = map.size();
      while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (map[mid].lo <= c) lo = mid + 1;
        else hi = mid;
      }
      if (lo > 0 && c <= map[lo - 1].hi) {
        uint32_t d = map[lo - 1].dst;
        if (!(d & MULTI)) {
          uint32_t u = d + (c - map[lo - 1].lo);
          if (!badTarget(u)) return out[0] = u, 1;
        } else if ((d & ~MULTI) < pool.size()) {
          uint32_t at = d & ~MULTI;
          int k = (int)std::min<uint32_t>(pool[at], (uint32_t)cap);
          for (int i = 0; i < k && at + 1 + i < pool.size(); i++) out[i] = pool[at + 1 + i];
          return k;
        }
      }
    }
    return 0;
  }

  // The advance of a code, in text space for a size of 1.
  float width(uint32_t c) const {
    if (simple) {
      if (c < 256 && hasWid && wid[c] > 0) return wid[c] * scale;
      if (missing > 0) return missing * scale;
      uint32_t u = c < 256 ? uni[c] : 0;
      if (u >= 32 && u < 127) return PDF_W_HELVETICA[u - 32] * 0.004f;
      return 0.5f;
    }
    for (const WRange &r : w)
      if (c >= r.lo && c <= r.hi) return r.w * 0.001f;
    return dw * 0.001f;
  }
};

// UTF-16BE (a CMap's targets) to code points.
int utf16(const std::string &b, uint32_t *out, int cap) {
  int k = 0;
  for (size_t i = 0; i + 1 < b.size() && k < cap; i += 2) {
    uint32_t u = (uint8_t)b[i] << 8 | (uint8_t)b[i + 1];
    if (u >= 0xD800 && u < 0xDC00 && i + 3 < b.size()) {
      uint32_t lo = (uint8_t)b[i + 2] << 8 | (uint8_t)b[i + 3];
      if (lo >= 0xDC00 && lo < 0xE000) u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00), i += 2;
    }
    out[k++] = u;
  }
  if (b.size() == 1 && cap) out[k++] = (uint8_t)b[0];  // a one-byte target, written short
  return k;
}
uint32_t codeOf(const std::string &b, int &len) {
  uint32_t v = 0;
  len = (int)std::min<size_t>(b.size(), 4);
  for (int i = 0; i < len; i++) v = v << 8 | (uint8_t)b[i];
  return v;
}

void addMap(Font &f, uint32_t lo, uint32_t hi, const uint32_t *cps, int n) {
  if (f.map.size() >= MAX_CMAP || n <= 0 || hi < lo) return;
  if (n == 1) {
    // One unusable target is no mapping (the encoding may know better); a range from
    // 0 counts up into the letters ("<0000> <FFFF> <0000>": the code is the character).
    if (!badTarget(cps[0]) || hi > lo) f.map.push_back(CMapEnt{lo, hi, cps[0]});
    return;
  }
  // Several characters (a ligature "ffi"): each code its own, the last character counting up.
  if (hi - lo > 255) hi = lo;
  for (uint32_t c = lo; c <= hi && f.map.size() < MAX_CMAP; c++) {
    f.map.push_back(CMapEnt{c, c, MULTI | (uint32_t)f.pool.size()});
    f.pool.push_back((uint32_t)n);
    for (int i = 0; i < n; i++) f.pool.push_back(cps[i] + (i == n - 1 ? c - lo : 0));
  }
}

void sortMap(Font &f) {
  std::sort(f.map.begin(), f.map.end(), [](const CMapEnt &x, const CMapEnt &y) { return x.lo != y.lo ? x.lo < y.lo : x.hi < y.hi; });
  f.hasMap = !f.map.empty();
}

// A CMap: a /ToUnicode one (bfchar, bfrange) or a font's encoding (only its code space ranges).
void parseCMap(Doc &doc, const Obj &o, Font &f, bool toUnicode) {
  PsVec<uint8_t> data;
  if (!doc.decodeAll(o, data, 1024 * 1024) && data.empty()) return;
  Lex lx(data.data(), data.size());
  std::string a, b;
  uint32_t cps[16];
  enum { NONE_, SPACE, CHAR, RANGE } mode = NONE_;
  for (;;) {
    Tk t = lx.next();
    if (t == T_EOF) break;
    if (t == T_KW) {
      if (lx.kw("begincodespacerange")) mode = SPACE;
      else if (lx.kw("beginbfchar")) mode = toUnicode ? CHAR : NONE_;
      else if (lx.kw("beginbfrange")) mode = toUnicode ? RANGE : NONE_;
      else if (lx.tn > 3 && !memcmp(lx.ts, "end", 3)) mode = NONE_;
      continue;
    }
    if (t != T_HEX || mode == NONE_) continue;
    a.clear();
    stringBytes(lx.ts, lx.tn, true, a, 8);
    int len;
    uint32_t lo = codeOf(a, len);
    if (mode == CHAR) {
      Tk t2 = lx.next();
      int n = 0;
      if (t2 == T_HEX) {
        b.clear();
        stringBytes(lx.ts, lx.tn, true, b, 64);
        n = utf16(b, cps, 16);
      } else if (t2 == T_NAME) {
        char nm[64];
        if (nameText(lx.ts, lx.tn, nm, sizeof nm) && (cps[0] = glyphCp(nm))) n = 1;
      }
      addMap(f, lo, lo, cps, n);
      continue;
    }
    if (lx.next() != T_HEX) continue;
    b.clear();
    stringBytes(lx.ts, lx.tn, true, b, 8);
    int len2;
    uint32_t hi = codeOf(b, len2);
    if (mode == SPACE) {
      if (len >= 1 && len <= 4 && hi >= lo && f.cs.size() < 64) f.cs.push_back(CsRange{lo, hi, (uint8_t)len});
      continue;
    }
    Tk t3 = lx.next();
    if (t3 == T_HEX) {
      b.clear();
      stringBytes(lx.ts, lx.tn, true, b, 64);
      int n = utf16(b, cps, 16);
      if (hi >= lo && hi - lo <= 0xFFFF) addMap(f, lo, hi, cps, n);
    } else if (t3 == T_AO) {
      for (uint32_t c = lo; c <= hi + 0u;) {
        Tk t4 = lx.next();
        if (t4 != T_HEX) break;
        b.clear();
        stringBytes(lx.ts, lx.tn, true, b, 64);
        addMap(f, c, c, cps, utf16(b, cps, 16));
        if (c++ == hi) break;
      }
      while (lx.p < lx.e && *lx.p != ']') lx.next();
      lx.next();
    }
    if (f.map.size() >= MAX_CMAP) break;
  }
  sortMap(f);
}

// The font's name without a subset's "ABCDEF+", and what it says of the style.
void styleOfName(const char *name, Font &f) {
  const char *plus = strchr(name, '+');
  if (plus && plus - name == 6) name = plus + 1;
  char low[96];
  size_t n = 0;
  for (; name[n] && n + 1 < sizeof low; n++) low[n] = (char)tolower((unsigned char)name[n]);
  low[n] = 0;
  if (strstr(low, "bold") || strstr(low, "black") || strstr(low, "heavy") || strstr(low, "semibold") || strstr(low, "demi")) f.bold = true;
  if (strstr(low, "italic") || strstr(low, "oblique")) f.italic = true;
  // "-It", "-BoldIt", "MinionPro-It": the short forms, after the family.
  const char *dash = strrchr(name, '-');
  if (!dash) return;
  for (const char *p = strstr(dash, "It"); p; p = strstr(p + 1, "It"))
    if (!islower((unsigned char)p[2])) f.italic = true;
  if (strstr(dash, "Bd")) f.bold = true;
}

void readDescriptor(Doc &doc, Sl font, Font &f) {
  Obj h;
  Sl d, v;
  double x;
  if (!doc.getKey(font, "FontDescriptor", h, d) || kindOf(d) != 'd') return;
  if (dictGet(d, "Flags", v) && vNum(v, x)) {
    uint32_t flags = clampU32(x);
    if (flags & 64) f.italic = true;
    if (flags & (1u << 18)) f.bold = true;
  }
  if (dictGet(d, "FontWeight", v) && vNum(v, x) && x >= 600) f.bold = true;
  if (dictGet(d, "ItalicAngle", v) && vNum(v, x) && (x < -4 || x > 4)) f.italic = true;
  if (dictGet(d, "MissingWidth", v) && vNum(v, x) && x > 0 && x < 5000) f.missing = (float)x;
}

// A glyph named for the letters it joins ("T_h", "uni00540068"): the code stands for all of them.
void ligature(Font &f, uint32_t code, const char *name) {
  uint32_t cps[8];
  int n = 0;
  if (!strncmp(name, "uni", 3) && strlen(name) >= 11 && !strchr(name, '.')) {
    for (const char *p = name + 3; n < 8 && strlen(p) >= 4; p += 4) {
      char four[8] = {'u', 'n', 'i', p[0], p[1], p[2], p[3], 0};
      if (!(cps[n++] = glyphCp(four))) return;
    }
  } else if (strchr(name, '_')) {
    char part[64];
    const char *p = name;
    while (*p && n < 8) {
      size_t k = strcspn(p, "_.");
      if (k == 0 || k >= sizeof part) return;
      memcpy(part, p, k), part[k] = 0;
      if (!(cps[n++] = glyphCp(part))) return;
      p += k;
      if (*p == '.') break;
      if (*p == '_') p++;
    }
  }
  if (n < 2) return;
  uint32_t have[8];
  if (f.fromMap(code, have, 8)) return;  // /ToUnicode said already
  addMap(f, code, code, cps, n);
  sortMap(f);
}

// The /Differences' glyphs named for several letters, after /ToUnicode (which wins).
void ligatures(Doc &doc, Sl fd, Font &f) {
  Obj eh, dh;
  Sl enc, diffs, da, e;
  if (!doc.getKey(fd, "Encoding", eh, enc) || kindOf(enc) != 'd' || !dictGet(enc, "Differences", diffs) || !doc.resolve(diffs, dh, da) || kindOf(da) != 'a') return;
  ArrIt it(da);
  uint32_t code = 0;
  double x;
  char gn[64];
  while (it.next(e)) {
    if (vNum(e, x)) code = clampU32(x);
    else if (vName(e, gn, sizeof gn) && code < 256) ligature(f, code++, gn);
  }
}

bool loadFont(Doc &doc, Sl fd, Font &f) {
  Sl v;
  Obj h;
  char sub[32] = "", name[96] = "";
  if (dictGet(fd, "Subtype", v)) vName(v, sub, sizeof sub);
  if (dictGet(fd, "BaseFont", v)) vName(v, name, sizeof name);
  styleOfName(name, f);
  for (int i = 0; i < 256; i++) f.uni[i] = 0, f.wid[i] = 0;
  bool type3 = !strcmp(sub, "Type3");
  if (!strcmp(sub, "Type0")) {
    f.simple = false;
    f.fixedLen = 2;
    Sl enc;
    Obj eh;
    if (doc.getKey(fd, "Encoding", eh, enc)) {
      char en[64] = "";
      if (vName(enc, en, sizeof en)) {
        if (strstr(en, "UCS2") || strstr(en, "UTF16")) f.ucs2 = true;
      } else if (eh.stream) {
        parseCMap(doc, eh, f, false);
        if (!f.cs.empty()) f.fixedLen = 0;
      }
    }
    // The descendant font: its widths and descriptor.
    Obj ah, dh;
    Sl arr, desc;
    if (doc.getKey(fd, "DescendantFonts", ah, arr)) {
      Sl first;
      ArrIt it(arr);
      if (kindOf(arr) == 'a' && it.next(first) && doc.resolve(first, dh, desc) && kindOf(desc) == 'd') {
        double x;
        if (dictGet(desc, "DW", v) && vNum(v, x)) f.dw = (float)x;
        readDescriptor(doc, desc, f);
        char dn[96] = "";
        if (dictGet(desc, "BaseFont", v) && vName(v, dn, sizeof dn)) styleOfName(dn, f);
        Obj wh;
        Sl wa;
        if (doc.getKey(desc, "W", wh, wa) && kindOf(wa) == 'a') {
          // [c [w1 w2 ...]] or [c1 c2 w]
          ArrIt wi(wa);
          Sl e1, e2, e3;
          while (f.w.size() < 20000 && wi.next(e1)) {
            double c1, c2, ww;
            if (!vNum(e1, c1) || !wi.next(e2)) break;
            if (kindOf(e2) == 'a') {
              ArrIt li(e2);
              Sl e;
              uint32_t c = clampU32(c1);
              while (li.next(e) && f.w.size() < 20000)
                if (vNum(e, ww)) f.w.push_back(WRange{c, c, (float)ww}), c++;
            } else if (vNum(e2, c2) && wi.next(e3) && vNum(e3, ww)) f.w.push_back(WRange{clampU32(c1), clampU32(c2), (float)ww});
          }
        }
      }
    }
  } else {
    // A simple font: one byte a code, its encoding, its widths.
    readDescriptor(doc, fd, f);
    const uint16_t *baseEnc = PDF_ENC_STANDARD;
    if (strstr(name, "Symbol")) baseEnc = PDF_ENC_SYMBOL;
    else if (strstr(name, "Dingbats")) baseEnc = nullptr;
    else if (!strcmp(sub, "TrueType")) baseEnc = PDF_ENC_WINANSI;
    Obj eh;
    Sl enc, diffs;
    bool haveDiffs = false;
    if (doc.getKey(fd, "Encoding", eh, enc)) {
      Sl bn = enc;
      if (kindOf(enc) == 'd') {
        if (!dictGet(enc, "BaseEncoding", bn)) bn = Sl();
        haveDiffs = dictGet(enc, "Differences", diffs);
      }
      char en[40] = "";
      if (bn.n && vName(bn, en, sizeof en)) {
        if (!strcmp(en, "WinAnsiEncoding")) baseEnc = PDF_ENC_WINANSI;
        else if (!strcmp(en, "MacRomanEncoding")) baseEnc = PDF_ENC_MACROMAN;
        else if (!strcmp(en, "StandardEncoding") || !strcmp(en, "MacExpertEncoding")) baseEnc = PDF_ENC_STANDARD;
      }
    }
    if (baseEnc)
      for (int i = 0; i < 256; i++) f.uni[i] = baseEnc[i];
    if (haveDiffs) {
      Obj dh2;
      Sl da;
      if (doc.resolve(diffs, dh2, da) && kindOf(da) == 'a') {
        ArrIt it(da);
        Sl e;
        uint32_t code = 0;
        while (it.next(e)) {
          double x;
          char gn[64];
          if (vNum(e, x)) code = clampU32(x);
          else if (vName(e, gn, sizeof gn)) {
            if (code < 256) f.uni[code] = (uint16_t)std::min<uint32_t>(glyphCp(gn), 0xFFFF);
            code++;
          }
        }
      }
    }
    double fc = 0, x;
    if (dictGet(fd, "FirstChar", v)) vNum(v, fc);
    Obj wh;
    Sl wa;
    if (doc.getKey(fd, "Widths", wh, wa) && kindOf(wa) == 'a') {
      ArrIt it(wa);
      Sl e;
      for (uint32_t c = clampU32(fc); c < 256 && it.next(e); c++)
        if (vNum(e, x) && x > 0 && x < 1e6) f.wid[c] = (float)x, f.hasWid = true;
    }
    if (type3) {
      Obj mh;
      Sl m, e;
      if (doc.getKey(fd, "FontMatrix", mh, m) && kindOf(m) == 'a') {
        ArrIt mi(m);
        if (mi.next(e) && vNum(e, x) && fabs(x) > 1e-6 && fabs(x) < 10) f.scale = (float)fabs(x);
      }
    } else if (!f.hasWid) {
      // A standard font has no widths written: they're known.
      const uint8_t *tbl = strstr(name, "Times") || strstr(name, "Roman") || strstr(name, "Serif") ? PDF_W_TIMES : PDF_W_HELVETICA;
      bool mono = strstr(name, "Courier") || strstr(name, "Mono");
      for (int c = 0; c < 256; c++) {
        uint32_t u = f.uni[c];
        f.wid[c] = mono ? 600 : u >= 32 && u < 127 ? tbl[u - 32] * 4.0f : 500;
      }
      f.hasWid = true;
    }
  }
  Obj th;
  Sl tu;
  if (doc.getKey(fd, "ToUnicode", th, tu) && th.stream) parseCMap(doc, th, f, true);
  if (f.simple) ligatures(doc, fd, f);
  return true;
}

// The fonts in use, by object number (or a hash of the dictionary written in place).
struct FontCache {
  Font *f[MAX_FONTS] = {};
  uint32_t clock = 0;
  ~FontCache() {
    for (Font *x : f) drop(x);
  }
  static void drop(Font *x) {
    if (!x) return;
    x->~Font();
    free(x);
  }
  Font *get(Doc &doc, uint32_t key, Sl dict) {
    size_t total = 0;
    int slot = -1, old = -1;
    for (int i = 0; i < MAX_FONTS; i++) {
      if (f[i] && f[i]->key == key) return f[i]->age = ++clock, f[i];
      if (!f[i]) slot = slot < 0 ? i : slot;
      else {
        total += f[i]->bytes();
        if (old < 0 || f[i]->age < f[old]->age) old = i;
      }
    }
    void *m = psMalloc(sizeof(Font));
    if (!m) return nullptr;
    Font *nf = new (m) Font();
    nf->key = key;
    loadFont(doc, dict, *nf);
    nf->age = ++clock;
    total += nf->bytes();
    // Room: the oldest go first.
    while ((slot < 0 || total > FONT_CACHE) && old >= 0) {
      total -= f[old]->bytes();
      drop(f[old]);
      f[old] = nullptr;
      if (slot < 0) slot = old;
      old = -1;
      for (int i = 0; i < MAX_FONTS; i++)
        if (f[i] && (old < 0 || f[i]->age < f[old]->age)) old = i;
    }
    f[slot] = nf;
    return nf;
  }
};

// ---------------------------------------------------------------- content streams

// A page's /Contents: one stream, or several read as one.
struct ConcatDec : Dec {
  Doc &d;
  PsVec<uint32_t> parts;
  size_t i = 0;
  Dec *cur = nullptr;
  explicit ConcatDec(Doc &doc) : d(doc) {}
  ~ConcatDec() override { delete cur; }
  size_t read(uint8_t *out, size_t cap) override {
    size_t made = 0;
    while (made < cap) {
      if (!cur) {
        if (i >= parts.size()) break;
        Obj o;
        if (d.get(parts[i++], o)) cur = d.open(o);
        if (cur) out[made++] = '\n';  // streams meet between tokens
        continue;
      }
      size_t n = cur->read(out + made, cap - made);
      if (!n) delete cur, cur = nullptr;
      made += n;
    }
    return made;
  }
};

// A run of text: words on one line in one style, with where it is on the page.
struct Run {
  float x0, x1, y, size;
  uint32_t text, len;
  uint8_t style;  // 1 italic, 2 bold
};
const uint8_t ST_ITALIC = 1, ST_BOLD = 2, ST_SPACE = 0x80;  // ST_SPACE: a space came after it

void mul(const float *a, const float *b, float *out) {  // out = a x b, the PDF way (row vectors)
  float r[6] = {a[0] * b[0] + a[1] * b[2], a[0] * b[1] + a[1] * b[3], a[2] * b[0] + a[3] * b[2],
                a[2] * b[1] + a[3] * b[3], a[4] * b[0] + a[5] * b[2] + b[4], a[4] * b[1] + a[5] * b[3] + b[5]};
  memcpy(out, r, sizeof r);
}

struct GState {
  float ctm[6];
  uint32_t font;  // the font's key; 0: none
  float fs, tc, tw, th, tl, ts;
};

// One content stream being read: the page's, or a form's that it calls.
struct Ctx {
  Dec *dec = nullptr;
  uint8_t *win = nullptr;
  uint32_t pos = 0, len = 0;
  bool eof = false;
  Obj res, fonts, xobjs;
  uint32_t form = 0;  // the form's object number (0: the page)
  int gdepth = 0;     // the graphics states saved when it began
  ~Ctx() {
    delete dec;
    free(win);
  }
  // At least LOOK bytes to lex from, unless the stream ends first.
  void fill() {
    if (eof || len - pos >= LOOK) return;
    memmove(win, win + pos, len - pos);
    len -= pos, pos = 0;
    while (len < WIN && !eof) {
      size_t n = dec->read(win + len, WIN - len);
      if (!n) eof = true;
      len += (uint32_t)n;
    }
  }
};

struct Arg {
  uint8_t kind;  // 'n' number, '/' name, 's' string, '[' ']' '<' '>'
  float num;
  uint32_t off, len;
};

// Reads the content streams of a page and the forms it calls, and gives the
// text shown as runs with their place on the page.
struct Interp {
  Doc &doc;
  FontCache &fonts;
  PsVec<Run> &runs;
  PsVec<char> &text;
  Ctx *ctx[MAX_FORM_DEPTH + 1] = {};
  int depth = 0;
  PsVec<GState> gs;
  GState g;
  float tm[6], tlm[6];
  Font *font = nullptr;
  PsVec<Arg> args;
  PsVec<uint8_t> argBytes;
  bool inText = false;
  uint32_t shown = 0, lost = 0;   // characters the fonts could and couldn't tell
  uint32_t consumed = 0;          // content bytes read this page
  bool full = false;              // the page's runs are at their cap: lay them out before going on
  int open = -1;                  // the run being added to
  uint32_t spaces = 0;            // space characters shown

  // How far apart two letters are to be two words, in ems. Most makers of PDFs
  // show the spaces, and set the letters of a line apart to justify it: then a
  // gap must be wide. TeX shows none, and its gaps between letters are next to
  // nothing: a narrower gap is a space there.
  float wordGap() const { return shown > 500 && spaces * 50 < shown ? 0.12f : 0.2f; }

  Interp(Doc &d, FontCache &fc, PsVec<Run> &r, PsVec<char> &t) : doc(d), fonts(fc), runs(r), text(t) {}
  ~Interp() { endAll(); }
  void endAll() {
    while (depth > 0) delete ctx[--depth];
  }

  // A page: its contents, its resources, its rotation (the text comes out upright).
  bool begin(Obj &page, Sl resources, int rotate, const float *box) {
    endAll();
    gs.clear();
    float r[6] = {1, 0, 0, 1, 0, 0};
    rotate = ((rotate % 360) + 360) % 360;
    if (rotate == 90) r[0] = 0, r[1] = -1, r[2] = 1, r[3] = 0, r[4] = -box[1], r[5] = box[2];
    else if (rotate == 180) r[0] = -1, r[3] = -1, r[4] = box[2], r[5] = box[3];
    else if (rotate == 270) r[0] = 0, r[1] = 1, r[2] = -1, r[3] = 0, r[4] = box[3], r[5] = -box[0];
    memcpy(g.ctm, r, sizeof r);
    g.font = 0, g.fs = 0, g.tc = g.tw = g.tl = g.ts = 0, g.th = 1;
    font = nullptr;
    consumed = 0;
    open = -1;
    ConcatDec *cd = new ConcatDec(doc);
    Sl c;
    uint32_t ref;
    Obj ch;
    if (dictGet(page.v(), "Contents", c)) {
      if (vRef(c, ref)) {
        // A reference to a stream, or to an array of them.
        if (doc.get(ref, ch) && !ch.stream && kindOf(ch.v()) == 'a') c = ch.v();
        else cd->parts.push_back(ref);
      }
      if (kindOf(c) == 'a') {
        ArrIt it(c);
        Sl e;
        while (it.next(e) && cd->parts.size() < 10000)
          if (vRef(e, ref)) cd->parts.push_back(ref);
      }
    }
    return push(cd, resources, 0);
  }

  bool push(Dec *dec, Sl resources, uint32_t form) {
    if (depth > MAX_FORM_DEPTH) return delete dec, false;
    Ctx *c = new Ctx();
    c->dec = dec;
    c->win = (uint8_t *)psMalloc(WIN);
    if (!c->win) return delete c, false;
    c->res.set(resources.p, resources.n);
    Sl v;
    Obj h;
    if (doc.getKey(c->res.v(), "Font", h, v)) c->fonts.set(v.p, v.n);
    if (doc.getKey(c->res.v(), "XObject", h, v)) c->xobjs.set(v.p, v.n);
    c->form = form;
    c->gdepth = (int)gs.size();
    ctx[depth++] = c;
    return true;
  }
  void pop() {
    Ctx *c = ctx[--depth];
    // A form leaves the graphics state as it found it.
    while ((int)gs.size() > c->gdepth) g = gs.back(), gs.pop_back();
    delete c;
    setFont();
  }

  void setFont() {
    font = nullptr;
    if (!g.font) return;
    if (!(g.font & MULTI)) {
      Obj o;
      if (doc.get(g.font, o)) font = fonts.get(doc, g.font, o.v());
    } else {
      for (Font *f : fonts.f)
        if (f && f->key == g.font) font = f;
    }
  }

  // ---- text

  void closeRun() {
    if (open >= 0) {
      Run &r = runs[open];
      while (r.len && text[r.text + r.len - 1] == ' ') r.len--, r.style |= ST_SPACE;  // no space at a run's end, but it's known
      if (!r.len) runs.pop_back();
    }
    open = -1;
  }

  void put(uint32_t cp) { putUtf8(text, cp); }

  // A character shown at (x, y) in the page's space, the next starting at x1.
  void glyph(const uint32_t *cp, int n, float x, float y, float x1, float size, uint8_t style) {
    if (n == 0) return;
    // Spaces of every kind are one; ligatures are their letters; what can't be shown goes.
    uint32_t out[8];
    int k = 0;
    for (int i = 0; i < n && k < 6; i++) {
      uint32_t c = cp[i];
      if (c >= 0xFB00 && c <= 0xFB06) {
        static const char *LIG[] = {"ff", "fi", "fl", "ffi", "ffl", "st", "st"};
        for (const char *p = LIG[c - 0xFB00]; *p && k < 8; p++) out[k++] = (uint8_t)*p;
        continue;
      }
      if (c == 0xA0 || c == 9 || (c >= 0x2000 && c <= 0x200A) || c == 0x202F || c == 0x205F || c == 0x3000) c = ' ';
      bool adobe = (c >= 0xF643 && c <= 0xF64C) || (c >= 0xF730 && c <= 0xF739) || (c >= 0xF761 && c <= 0xF77A);
      if (c < 0x20 || (c >= 0x7F && c < 0xA0) || (c >= 0x200B && c <= 0x200F) || c == 0x2060 || c == 0xFEFF || (badTarget(c) && !adobe)) continue;
      out[k++] = c;
    }
    if (!k) return;
    for (int i = 0; i < k; i++) {
      // Adobe's private code points for old-style figures and small capitals: plain digits and letters.
      uint32_t c = out[i];
      if (c >= 0xF643 && c <= 0xF64C) out[i] = '0' + (c - 0xF643);
      else if (c >= 0xF730 && c <= 0xF739) out[i] = '0' + (c - 0xF730);
      else if (c >= 0xF761 && c <= 0xF77A) out[i] = 'a' + (c - 0xF761);
    }
    bool space = k == 1 && out[0] == ' ';
    if (space) spaces++;
    if (open >= 0) {
      Run &r = runs[open];
      float gap = x - r.x1;
      bool sameLine = fabsf(y - r.y) < 0.3f * r.size && fabsf(size - r.size) < 0.15f * r.size;
      if (!sameLine || (r.style & ~ST_SPACE) != style || gap < -0.6f * r.size || gap > 1.0f * r.size) closeRun();
      else {
        if (space) {
          if (r.len && text[r.text + r.len - 1] != ' ') put(' '), r.len++;
          r.x1 = std::max(r.x1, x1);
          return;
        }
        if (gap > wordGap() * r.size && r.len && text[r.text + r.len - 1] != ' ') put(' '), r.len++;
        size_t before = text.size();
        for (int i = 0; i < k; i++) put(out[i]);
        r.len += (uint32_t)(text.size() - before);
        r.x1 = std::max(r.x1, x1);
        if (text.size() >= PAGE_TEXT) full = true;
        return;
      }
    }
    if (space) return;
    if (runs.size() >= MAX_RUNS || text.size() >= PAGE_TEXT) return void(full = true);
    Run r;
    r.x0 = x, r.x1 = x1, r.y = y, r.size = size, r.style = style;
    r.text = (uint32_t)text.size();
    for (int i = 0; i < k; i++) put(out[i]);
    r.len = (uint32_t)(text.size() - r.text);
    runs.push_back(r);
    open = (int)runs.size() - 1;
  }

  // Tj: the string's codes through the font, each moving the text matrix on.
  void show(const uint8_t *s, uint32_t n) {
    if (!font) {
      lost += n;
      return;
    }
    uint8_t style = (font->italic ? ST_ITALIC : 0) | (font->bold ? ST_BOLD : 0);
    for (uint32_t i = 0; i < n;) {
      uint32_t code;
      int used = font->code(s + i, n - i, code);
      i += used;
      uint32_t cps[8];
      int k = font->chars(code, cps, 8);
      float w0 = font->width(code);
      float tx = (w0 * g.fs + g.tc + (used == 1 && code == 32 ? g.tw : 0)) * g.th;
      float m[6];
      mul(tm, g.ctm, m);
      float x = g.ts * m[2] + m[4], y = g.ts * m[3] + m[5];
      float size = fabsf(g.fs) * sqrtf(m[2] * m[2] + m[3] * m[3]);
      tm[4] += tx * tm[0], tm[5] += tx * tm[1];
      float x1 = x + tx * m[0];  // the next glyph's place, along the line
      if (k == 0) {
        if (!(used == 1 && code == 32)) lost++;
        continue;
      }
      // Upright text only: what runs up a margin (arXiv's stamp) or is mirrored isn't the book's.
      if (!(m[0] > 0) || fabsf(m[1]) > 0.2f * m[0] || !(size > 0.5f) || !(size < 2000) || !finite6(m)) continue;
      shown += k;
      glyph(cps, k, x, y, x1, size, style);
    }
  }

  void showArg(const Arg &a) {
    if (a.kind == 's') show(argBytes.data() + a.off, a.len);
  }

  void nextLine(float tx, float ty) {
    float t[6] = {1, 0, 0, 1, tx, ty};
    mul(t, tlm, tlm);
    memcpy(tm, tlm, sizeof tm);
  }

  // ---- operators

  float num(int i) const {
    int base = (int)args.size() - i;  // i-th from the end, counting 1
    return base >= 0 && args[base].kind == 'n' ? args[base].num : 0;
  }
  bool name(int i, char *out, size_t cap) const {
    int base = (int)args.size() - i;
    if (base < 0 || args[base].kind != '/') return false;
    return nameText(argBytes.data() + args[base].off, args[base].len, (char *)out, cap);
  }

  void op(const uint8_t *k, uint32_t n) {
    char o[4] = {0, 0, 0, 0};
    if (n > 3) return;
    memcpy(o, k, n);
    size_t na = args.size();
    switch (o[0]) {
      case 'q':
        if (!o[1] && gs.size() < MAX_Q) gs.push_back(g);
        break;
      case 'Q':
        if (!o[1] && (int)gs.size() > ctx[depth - 1]->gdepth) {
          uint32_t was = g.font;
          g = gs.back(), gs.pop_back();
          if (g.font != was) setFont();
        }
        break;
      case 'c':
        if (o[1] == 'm' && !o[2] && na >= 6) {
          float m[6] = {num(6), num(5), num(4), num(3), num(2), num(1)};
          if (finite6(m)) mul(m, g.ctm, g.ctm);
        }
        break;
      case 'B':
        if (o[1] == 'T' && !o[2]) {
          inText = true;
          float id[6] = {1, 0, 0, 1, 0, 0};
          memcpy(tm, id, sizeof tm), memcpy(tlm, id, sizeof tlm);
        } else if (o[1] == 'I' && !o[2]) skipInlineImage();
        break;
      case 'E':
        if (o[1] == 'T' && !o[2]) inText = false;
        break;
      case 'D':
        if (o[1] == 'o' && !o[2]) callForm();
        break;
      case '\'':
        nextLine(0, -g.tl);
        if (na >= 1) showArg(args[na - 1]);
        break;
      case '"':
        if (na >= 3) g.tw = num(3), g.tc = num(2);
        nextLine(0, -g.tl);
        if (na >= 1) showArg(args[na - 1]);
        break;
      case 'T':
        switch (o[1]) {
          case 'f': {
            char nm[64];
            if (na >= 2 && name(2, nm, sizeof nm)) {
              g.fs = num(1);
              g.font = 0;
              Ctx *c = ctx[depth - 1];
              Sl v;
              uint32_t ref;
              // Fonts named in this stream's resources; a form without its own uses the page's.
              for (int d = depth - 1; d >= 0 && !g.font; d--) {
                c = ctx[d];
                if (!c->fonts.b.empty() && dictGet(c->fonts.v(), nm, v)) {
                  if (vRef(v, ref)) g.font = ref;
                  else if (kindOf(v) == 'd') {
                    uint32_t hash = 2166136261u;
                    for (uint32_t i = 0; i < v.n; i++) hash = (hash ^ v.p[i]) * 16777619u;
                    g.font = MULTI | (hash & 0x7FFFFFFF) | 1;
                    font = fonts.get(doc, g.font, v);
                    break;
                  }
                }
              }
              if (!(g.font & MULTI)) setFont();
              if (!font) g.font = 0;
            }
            break;
          }
          case 'd':
            if (na >= 2) nextLine(num(2), num(1));
            break;
          case 'D':
            if (na >= 2) g.tl = -num(1), nextLine(num(2), num(1));
            break;
          case 'm':
            if (na >= 6) {
              float m[6] = {num(6), num(5), num(4), num(3), num(2), num(1)};
              if (finite6(m)) memcpy(tm, m, sizeof m), memcpy(tlm, m, sizeof m);
            }
            break;
          case '*': nextLine(0, -g.tl); break;
          case 'L': g.tl = num(1); break;
          case 'c': g.tc = num(1); break;
          case 'w': g.tw = num(1); break;
          case 'z': g.th = num(1) / 100; break;
          case 's': g.ts = num(1); break;
          case 'j': if (na >= 1) showArg(args[na - 1]); break;
          case 'J': {
            // [(text) -120 (more)]: a number moves the next string back by thousandths of the size.
            size_t i = 0;
            while (i < na && args[i].kind != '[') i++;
            for (i++; i < na && args[i].kind != ']'; i++) {
              if (args[i].kind == 's') showArg(args[i]);
              else if (args[i].kind == 'n') {
                float tx = -args[i].num / 1000 * g.fs * g.th;
                if (fabsf(tx) < 1e6f) tm[4] += tx * tm[0], tm[5] += tx * tm[1];
              }
            }
            break;
          }
        }
        break;
    }
  }

  void callForm() {
    char nm[64];
    Ctx *c = ctx[depth - 1];
    Sl v;
    uint32_t ref;
    if (!name(1, nm, sizeof nm) || c->xobjs.b.empty() || !dictGet(c->xobjs.v(), nm, v) || !vRef(v, ref)) return;
    if (depth > MAX_FORM_DEPTH) return;
    for (int d = 0; d < depth; d++)
      if (ctx[d]->form == ref) return;  // a form calling itself
    Obj o;
    if (!doc.get(ref, o) || !o.stream) return;
    Sl st;
    if (!dictGet(o.v(), "Subtype", st) || !vNameIs(st, "Form")) return;  // a picture
    Dec *dec = doc.open(o);
    if (!dec) return;
    if (gs.size() >= MAX_Q + MAX_FORM_DEPTH) return void(delete dec);  // forms get room past q's limit
    gs.push_back(g);
    Sl mv, e;
    if (dictGet(o.v(), "Matrix", mv) && kindOf(mv) == 'a') {
      float m[6];
      ArrIt it(mv);
      int i = 0;
      double d;
      for (; i < 6 && it.next(e) && vNum(e, d); i++) m[i] = (float)d;
      if (i == 6 && finite6(m)) mul(m, g.ctm, g.ctm);
    }
    Obj rh;
    Sl res;
    if (!doc.getKey(o.v(), "Resources", rh, res) || kindOf(res) != 'd') res = c->res.v();
    Obj keep;
    keep.set(res.p, res.n);  // res may point into o or c, which push() copies from
    if (!push(dec, keep.v(), ref)) g = gs.back(), gs.pop_back();
    else ctx[depth - 1]->gdepth = (int)gs.size() - 1;  // the q above is undone when the form ends
  }

  // BI <dictionary> ID <bytes> EI: the bytes are anything, up to a white space, "EI", a white space.
  void skipInlineImage() {
    Ctx *c = ctx[depth - 1];
    for (;;) {  // to ID
      c->fill();
      Lex lx(c->win + c->pos, c->len - c->pos);
      Tk t = lx.next();
      c->pos = (uint32_t)(lx.p - c->win);
      if (t == T_EOF) return;
      if (t == T_KW && lx.kw("ID")) break;
    }
    c->pos++;  // the one white space after ID
    for (;;) {
      c->fill();
      if (c->pos + 3 > c->len) {
        c->pos = c->len;
        return;
      }
      const uint8_t *p = c->win + c->pos, *e = c->win + c->len - 2;
      for (; p < e; p++)
        if (p[0] == 'E' && p[1] == 'I' && (p + 2 >= c->win + c->len || isWs(p[2]) || p[2] == '%') && (p == c->win || isWs(p[-1]))) {
          c->pos = (uint32_t)(p + 2 - c->win);
          return;
        }
      c->pos = (uint32_t)(e - c->win);
      if (c->eof) {
        c->pos = c->len;
        return;
      }
    }
  }

  // Reads on until budget bytes have gone by, the page's runs are full, or
  // the page ends. True: the page is done.
  bool run(uint32_t budget) {
    uint32_t start = consumed;
    while (depth > 0) {
      Ctx *c = ctx[depth - 1];
      c->fill();
      Lex lx(c->win + c->pos, c->len - c->pos);
      Tk t = lx.next();
      uint32_t took = (uint32_t)(lx.p - (c->win + c->pos));
      c->pos += took;
      consumed += took;
      if (t == T_EOF) {
        args.clear(), argBytes.clear();
        if (c->eof || c->pos >= c->len) {
          pop();
          continue;
        }
        continue;
      }
      if (args.size() > 4096 || argBytes.size() > 256 * 1024) args.clear(), argBytes.clear();  // no operator in sight: junk
      Arg a;
      a.num = 0, a.off = (uint32_t)argBytes.size(), a.len = 0;
      switch (t) {
        case T_NUM: a.kind = 'n', a.num = (float)lx.num; break;
        case T_NAME:
          a.kind = '/';
          argBytes.insert(argBytes.end(), lx.ts, lx.ts + lx.tn);
          a.len = lx.tn;
          break;
        case T_STR:
        case T_HEX: {
          a.kind = 's';
          stringBytes(lx.ts, lx.tn, t == T_HEX, argBytes, argBytes.size() + 65536);
          a.len = (uint32_t)(argBytes.size() - a.off);
          break;
        }
        case T_AO: a.kind = '['; break;
        case T_AC: a.kind = ']'; break;
        case T_DO: a.kind = '<'; break;
        case T_DC: a.kind = '>'; break;
        case T_KW:
          op(lx.ts, lx.tn);
          args.clear(), argBytes.clear();
          if (full || consumed - start >= budget || consumed >= PAGE_BYTES) return depth == 0 || consumed >= PAGE_BYTES;
          continue;
        default: continue;
      }
      args.push_back(a);
    }
    closeRun();
    return true;
  }
};

// ---------------------------------------------------------------- layout

// A line of a page: its runs, left to right, and where it is.
struct Line {
  float x0, x1, y, size;
  uint32_t first, n;  // in the page's order of runs
  int dropCap;        // a big first letter set beside the lines, -1: none
  uint8_t col;        // 0 the whole width, 1 the left column, 2 the right
  bool bold, drop;    // all bold; left out (a running header, a page number)
  uint32_t chars;
};

// A page, laid out into lines, waiting for the pages around it (running
// headers are told by what the pages near it have at their edges).
struct PageLines {
  uint32_t index = 0;
  float height = 792;
  PsVec<Run> runs;
  PsVec<char> text;
  PsVec<uint32_t> order;
  PsVec<Line> lines;
  float left[3] = {0, 0, 0}, right[3] = {0, 0, 0};
  float pitch = 0;
  std::string sig[4];  // the top two and bottom two lines, digits made '#'
  int sigLine[4] = {-1, -1, -1, -1};
  float sigY[4] = {0, 0, 0, 0};  // where those lines are
  // Once written, a page is kept only for what its edges say to the pages after it.
  void slim() {
    PsVec<Run>().swap(runs);
    PsVec<char>().swap(text);
    PsVec<uint32_t>().swap(order);
    PsVec<Line>().swap(lines);
  }
};

std::string signature(const PageLines &p, const Line &l) {
  std::string s;
  for (uint32_t i = 0; i < l.n && s.size() < 80; i++) {
    const Run &r = p.runs[p.order[l.first + i]];
    for (uint32_t k = 0; k < r.len; k++) {
      char c = p.text[r.text + k];
      if (isDigit((uint8_t)c)) {
        if (s.empty() || s.back() != '#') s += '#';
      } else if ((uint8_t)c > ' ') s += (char)tolower((uint8_t)c);
    }
  }
  return s;
}

// "12", "- 12 -", "xiv", "Page 3", "3 of 20": a page number and nothing else.
bool isPageNumber(const std::string &raw) {
  std::string s;
  for (char c : raw)
    if ((uint8_t)c > ' ' && c != '-' && c != '[' && c != ']' && c != '(' && c != ')' && c != '|') s += (char)tolower((uint8_t)c);
  // dashes of every kind
  std::string t;
  for (size_t i = 0; i < s.size(); i++) {
    if ((uint8_t)s[i] == 0xE2 && i + 2 < s.size() && (uint8_t)s[i + 1] == 0x80 && (uint8_t)s[i + 2] >= 0x90 && (uint8_t)s[i + 2] <= 0x95) {
      i += 2;
      continue;
    }
    t += s[i];
  }
  if (t.compare(0, 4, "page") == 0) t = t.substr(4);
  if (t.empty() || t.size() > 12) return false;
  size_t i = 0;
  if (isDigit((uint8_t)t[0])) {
    while (i < t.size() && isDigit((uint8_t)t[i])) i++;
    if (i < t.size() && (t.compare(i, 2, "of") == 0 || t[i] == '/')) {
      i += t[i] == '/' ? 1 : 2;
      size_t j = i;
      while (i < t.size() && isDigit((uint8_t)t[i])) i++;
      if (i == j) return false;
    }
    return i == t.size();
  }
  for (; i < t.size(); i++)
    if (!strchr("ivxlcdm", t[i])) return false;
  return true;
}

// A drop cap's text: a letter or two, maybe after an opening quote ("‘G").
bool dropCapText(const char *s, uint32_t n) {
  int letters = 0, len;
  for (const char *p = s, *e = s + n; p < e; p += len) {
    uint32_t c = getUtf8(p, e, len);
    if (isLetter(c)) letters++;
    else if (letters || !(c == '"' || c == '\'' || c == 0x2018 || c == 0x201C || c == 0xAB)) return false;
  }
  return letters >= 1 && letters <= 2;
}

// Runs to lines, in the order they are read: top to bottom, and the left
// column before the right one when the page has two.
void layoutPage(PageLines &p) {
  size_t nr = p.runs.size();
  p.order.clear(), p.lines.clear();
  if (!nr) return;
  float minX = 1e9f, maxX = -1e9f;
  PsVec<float> sizes;
  for (const Run &r : p.runs) minX = std::min(minX, r.x0), maxX = std::max(maxX, r.x1), sizes.push_back(r.size);
  std::nth_element(sizes.begin(), sizes.begin() + nr / 2, sizes.end());
  float med = sizes[nr / 2];
  // Big first letters (a drop cap) are set apart from the lines; they go back in front of their line.
  PsVec<uint8_t> cls(nr, 0);  // 0 whole width, 1 left, 2 right, 3 a drop cap
  int candidates = 0;
  for (size_t i = 0; i < nr && candidates < 8; i++) {
    const Run &r = p.runs[i];
    if (r.len > 8 || !(r.size > 1.8f * med) || !dropCapText(p.text.data() + r.text, r.len)) continue;
    // Only when there are lines beside it: else it's a heading of its own ("I", a chapter's number).
    candidates++;
    for (size_t k = 0; k < nr && !cls[i]; k++) {
      const Run &o = p.runs[k];
      if (k != i && o.x0 > r.x0 + 0.5f * med && o.x0 < r.x1 + 3 * med && o.y <= r.y + r.size && o.y >= r.y - 0.3f * r.size && o.size < r.size * 0.6f) cls[i] = 3;
    }
  }
  // Two columns: a band near the middle that no run crosses, with plenty of text on both sides.
  float width = maxX - minX;
  float gutter = 0;
  if (nr >= 20 && width > 20 * med) {
    const int B = 100;
    uint16_t cover[B];
    memset(cover, 0, sizeof cover);
    for (size_t i = 0; i < nr; i++) {
      if (cls[i] == 3) continue;
      int a = (int)((p.runs[i].x0 - minX) / width * B), b = (int)((p.runs[i].x1 - minX) / width * B);
      for (int k = std::max(a, 0); k <= std::min(b, B - 1); k++)
        if (cover[k] < 60000) cover[k]++;
    }
    int bestA = -1, bestB = -1;
    for (int k = 30; k < 70; k++) {
      if (cover[k] > nr / 20) continue;
      int e = k;
      while (e + 1 < 70 && cover[e + 1] <= nr / 20) e++;
      if (e - k > bestB - bestA) bestA = k, bestB = e;
      k = e;
    }
    if (bestA >= 0 && (bestB - bestA + 1) * width / B >= 0.8f * med) {
      float g = minX + (bestA + bestB + 1) * 0.5f * width / B;
      size_t l = 0, rr = 0, cross = 0;
      for (size_t i = 0; i < nr; i++) {
        if (cls[i] == 3) continue;
        if (p.runs[i].x1 <= g) l++;
        else if (p.runs[i].x0 >= g) rr++;
        else cross++;
      }
      if (l >= nr / 4 && rr >= nr / 4 && l >= 10 && rr >= 10 && cross <= nr / 10) gutter = g;
    }
  }
  for (size_t i = 0; i < nr; i++) {
    if (cls[i] == 3) continue;
    if (gutter > 0) cls[i] = p.runs[i].x1 <= gutter ? 1 : p.runs[i].x0 >= gutter ? 2 : 0;
  }
  // Lines: runs of one class whose baselines meet, top first.
  PsVec<uint32_t> idx;
  for (size_t i = 0; i < nr; i++)
    if (cls[i] != 3) idx.push_back((uint32_t)i);
  // (std::sort with the index as the last key: stable_sort would borrow memory from the small heap)
  std::sort(idx.begin(), idx.end(), [&](uint32_t a, uint32_t b) {
    if (cls[a] != cls[b]) return cls[a] < cls[b];
    if (p.runs[a].y != p.runs[b].y) return p.runs[a].y > p.runs[b].y;
    return a < b;
  });
  for (size_t i = 0; i < idx.size();) {
    const Run &h = p.runs[idx[i]];
    size_t j = i + 1;
    float tol = 0.45f * h.size;
    while (j < idx.size() && cls[idx[j]] == cls[idx[i]] && h.y - p.runs[idx[j]].y < std::max(tol, 0.45f * p.runs[idx[j]].size)) j++;
    std::sort(idx.begin() + i, idx.begin() + j, [&](uint32_t a, uint32_t b) { return p.runs[a].x0 != p.runs[b].x0 ? p.runs[a].x0 < p.runs[b].x0 : a < b; });
    Line l;
    l.first = (uint32_t)i, l.n = (uint32_t)(j - i), l.dropCap = -1, l.col = cls[idx[i]], l.drop = false;
    l.x0 = p.runs[idx[i]].x0, l.x1 = p.runs[idx[j - 1]].x1, l.y = h.y;
    // The line's size and boldness: by the characters in it.
    // (a few sizes at most in a line: the first ones met, counted)
    float sz[4] = {0, 0, 0, 0};
    uint32_t cnt[4] = {0, 0, 0, 0}, boldN = 0, all = 0;
    for (size_t k = i; k < j; k++) {
      const Run &r = p.runs[idx[k]];
      all += r.len;
      if (r.style & ST_BOLD) boldN += r.len;
      for (int s = 0; s < 4; s++)
        if (!cnt[s] || fabsf(sz[s] - r.size) < 0.05f * r.size) {
          if (!cnt[s]) sz[s] = r.size;
          cnt[s] += r.len;
          break;
        }
      l.x1 = std::max(l.x1, r.x1);
    }
    float best = sz[0];
    for (int s = 1; s < 4; s++)
      if (cnt[s] > cnt[0]) cnt[0] = cnt[s], best = sz[s];
    l.size = best, l.bold = boldN == all, l.chars = all;
    p.lines.push_back(l);
    i = j;
  }
  p.order = idx;
  for (size_t i = 0; i < nr; i++) {
    if (cls[i] != 3) continue;
    const Run &d = p.runs[i];
    int at = -1;
    for (size_t k = 0; k < p.lines.size(); k++) {
      const Line &l = p.lines[k];
      if (l.x0 > d.x0 + 0.5f * med && l.x0 < d.x1 + 3 * med && l.y <= d.y + d.size && l.y >= d.y - 0.3f * d.size && (at < 0 || l.y > p.lines[at].y)) at = (int)k;
    }
    if (at < 0 || p.lines[at].dropCap >= 0) continue;
    p.lines[at].dropCap = (int)i;
    // The lines beside it start where it does: they aren't indented.
    for (Line &l : p.lines)
      if (l.x0 > d.x0 + 0.5f * med && l.x0 < d.x1 + 3 * med && l.y <= d.y + d.size && l.y >= d.y - 0.3f * d.size) l.x0 = d.x0;
  }
  // The lines in reading order: between lines across the whole width, the left column, then the right.
  if (gutter > 0) {
    PsVec<Line> out;
    PsVec<Line> wide, colL, colR;
    for (const Line &l : p.lines) (l.col == 0 ? wide : l.col == 1 ? colL : colR).push_back(l);
    size_t a = 0, b = 0;
    for (size_t w = 0; w <= wide.size(); w++) {
      float floor = w < wide.size() ? wide[w].y : -1e9f;
      while (a < colL.size() && colL[a].y > floor) out.push_back(colL[a++]);
      while (b < colR.size() && colR[b].y > floor) out.push_back(colR[b++]);
      if (w < wide.size()) out.push_back(wide[w]);
    }
    p.lines.swap(out);
  }
  // Each column's edges, and the page's line pitch, from its ordinary lines.
  for (int c = 0; c < 3; c++) {
    PsVec<float> xs, xe;
    for (const Line &l : p.lines)
      if (l.col == c && l.dropCap < 0 && l.chars > 20 && fabsf(l.size - med) < 0.15f * med) xs.push_back(l.x0), xe.push_back(l.x1);
    if (xs.empty())
      for (const Line &l : p.lines)
        if (l.col == c) xs.push_back(l.x0), xe.push_back(l.x1);
    if (xs.empty()) continue;
    std::sort(xs.begin(), xs.end());
    std::sort(xe.begin(), xe.end());
    // The left edge: the place most lines start at (not the indented first lines).
    float bestX = xs[0];
    size_t bestN = 0;
    for (size_t i = 0, k = 0; i < xs.size(); i++) {
      if (k < i) k = i;
      while (k < xs.size() && xs[k] - xs[i] < 0.5f * med) k++;
      if (k - i > bestN) bestN = k - i, bestX = xs[i];
    }
    p.left[c] = bestX;
    p.right[c] = xe[xe.size() * 8 / 10];
  }
  PsVec<float> dys;
  for (size_t k = 1; k < p.lines.size(); k++) {
    const Line &a = p.lines[k - 1], &b = p.lines[k];
    float dy = a.y - b.y;
    if (a.col == b.col && fabsf(a.size - med) < 0.15f * med && fabsf(b.size - med) < 0.15f * med && dy > 0.8f * med && dy < 2.5f * med) dys.push_back(dy);
  }
  if (!dys.empty()) {
    std::nth_element(dys.begin(), dys.begin() + dys.size() / 2, dys.end());
    p.pitch = dys[dys.size() / 2];
  }
  // The edges of the page: the two top lines and the two bottom ones.
  int top[2] = {-1, -1}, bot[2] = {-1, -1};
  for (size_t k = 0; k < p.lines.size(); k++) {
    int i = (int)k;
    float y = p.lines[k].y;
    if (top[0] < 0 || y > p.lines[top[0]].y) top[1] = top[0], top[0] = i;
    else if (top[1] < 0 || y > p.lines[top[1]].y) top[1] = i;
    if (bot[0] < 0 || y < p.lines[bot[0]].y) bot[1] = bot[0], bot[0] = i;
    else if (bot[1] < 0 || y < p.lines[bot[1]].y) bot[1] = i;
  }
  int edge[4] = {top[0], top[1], bot[0], bot[1]};
  for (int e = 0; e < 4; e++) {
    p.sigLine[e] = edge[e];
    p.sigY[e] = edge[e] >= 0 ? p.lines[edge[e]].y : 0;
    p.sig[e] = edge[e] >= 0 ? signature(p, p.lines[edge[e]]) : std::string();
  }
}

// A line's text, its first 300 bytes or so: enough to tell a page number or a title.
std::string lineText(const PageLines &p, const Line &l) {
  std::string s;
  for (uint32_t i = 0; i < l.n && s.size() < 300; i++) {
    const Run &r = p.runs[p.order[l.first + i]];
    if (i) s += ' ';
    s.append(p.text.data() + r.text, std::min<uint32_t>(r.len, 300));
  }
  return s;
}

// Running headers and footers, page numbers: lines at the top or bottom that
// a page near this one has too, at about the same height (the same but for a number).
void dropEdges(PageLines &p, PageLines *const *near, int nn, float body) {
  for (int e = 0; e < 4; e++) {
    int li = p.sigLine[e];
    if (li < 0 || p.sig[e].empty() || p.lines[li].drop) continue;
    Line &l = p.lines[li];
    bool outer = e == 0 || e == 2;
    // A page number is small: a big "I" is a chapter's number.
    bool drop = outer && p.lines.size() > 1 && (body <= 0 || l.size < 1.1f * body) && isPageNumber(lineText(p, l));
    for (int k = 0; k < nn && !drop && p.sig[e].size() < 120; k++) {
      const PageLines *q = near[k];
      for (int f = (e < 2 ? 0 : 2); f < (e < 2 ? 2 : 4) && !drop; f++)
        if (q->sigLine[f] >= 0 && q->sig[f] == p.sig[e] && fabsf(q->sigY[f] - l.y) < 0.04f * p.height) drop = true;
    }
    // The inner edge line only goes with the outer one: never a line in the middle of the text.
    if (drop && !outer && !p.lines[p.sigLine[e - 1]].drop) drop = false;
    if (drop) l.drop = true;
  }
}

// ---------------------------------------------------------------- lines to the flow

struct OutlineEnt {
  uint32_t page, title;  // title: offset in the pool, 0-terminated
  uint8_t level;
};

bool endsSentence(const char *s, size_t n) {
  size_t i = n;
  while (i > 0) {
    uint8_t c = s[i - 1];
    // Closing quotes and brackets, ASCII or UTF-8 (’ ” »), are looked past.
    if (c == '"' || c == '\'' || c == ')' || c == ']' || c == ' ') {
      i--;
      continue;
    }
    if (i >= 3 && (uint8_t)s[i - 3] == 0xE2 && (uint8_t)s[i - 2] == 0x80 && (c == 0x99 || c == 0x9D)) {
      i -= 3;
      continue;
    }
    if (i >= 2 && (uint8_t)s[i - 2] == 0xC2 && c == 0xBB) {
      i -= 2;
      continue;
    }
    if (i >= 3 && (uint8_t)s[i - 3] == 0xE2 && (uint8_t)s[i - 2] == 0x80 && c == 0xA6) return true;  // …
    return c == '.' || c == '!' || c == '?' || c == ':';
  }
  return false;
}

// Letters and digits, lower case: titles and headings compared.
std::string folded(const char *s, size_t n) {
  std::string out;
  for (size_t i = 0; i < n; i++) {
    uint8_t c = s[i];
    if (isalnum(c)) out += (char)tolower(c);
    else if (c >= 0x80) out += (char)c;
  }
  return out;
}

// A line's text, without a copy (a line may be long: no string on the small heap for it).
struct Txt {
  const char *p;
  size_t n;
  size_t size() const { return n; }
  const char *data() const { return p; }
  char back() const { return p[n - 1]; }
};

int letters(const Txt &s) {
  int n = 0, len;
  for (size_t i = 0; i < s.size(); i += len) n += isLetter(getUtf8(s.data() + i, s.data() + s.size(), len));
  return n;
}

// Puts the lines of finished pages into paragraphs, headings and the contents.
struct Para {
  FlowWriter &out;
  uint32_t hist[256];  // characters by size, in half points: the body's size is the commonest
  float body = 0;
  float pitch = 0;
  int indentVotes = 0, gapVotes = 0;
  bool outline = false;
  // The line written last, held back until the next says how they join.
  bool have = false;
  PsVec<char> ptext;
  PsVec<uint8_t> pstyle;  // a style for each byte of ptext
  PsVec<char> lt;         // the line being looked at (kept: no allocation a line)
  PsVec<uint8_t> lst;
  BlockKind pkind = BK_PARA;
  bool pHeading = false;
  float px0 = 0, px1 = 0, py = 0, psize = 0, pright = 0, pleft = 0;
  uint32_t ppage = 0;
  uint8_t pcol = 0;
  bool pShort = false, pVeryShort = false, pPunct = false;

  explicit Para(FlowWriter &w) : out(w) { memset(hist, 0, sizeof hist); }

  void count(const PageLines &p) {
    for (const Line &l : p.lines) {
      int b = (int)(l.size * 2 + 0.5f);
      if (b >= 0 && b < 256 && !l.drop) hist[b] += l.chars;
    }
    uint32_t best = 0;
    for (int b = 0; b < 256; b++)
      if (hist[b] > best) best = hist[b], body = b / 2.0f;
    if (p.pitch > 0) pitch = pitch > 0 ? pitch * 0.8f + p.pitch * 0.2f : p.pitch;
  }

  void flushPending(bool hyphenJoin) {
    if (!have) return;
    size_t n = ptext.size();
    if (hyphenJoin) {
      // The hyphen goes: "-", U+2010 or a soft hyphen at the end.
      if (n && ptext[n - 1] == '-') n--;
      else if (n >= 3 && (uint8_t)ptext[n - 3] == 0xE2 && (uint8_t)ptext[n - 2] == 0x80 && (uint8_t)ptext[n - 1] == 0x90) n -= 3;
      else if (n >= 2 && (uint8_t)ptext[n - 2] == 0xC2 && (uint8_t)ptext[n - 1] == 0xAD) n -= 2;
    }
    size_t i = 0;
    while (i < n) {
      size_t j = i;
      while (j < n && pstyle[j] == pstyle[i]) j++;
      bool heading = pkind == BK_H1 || pkind == BK_H2 || pkind == BK_H3;
      out.italic(pstyle[i] & ST_ITALIC);
      out.bold(!heading && (pstyle[i] & ST_BOLD));
      // Soft hyphens inside a line are no use to the reader here: they go.
      size_t k = i;
      while (k < j) {
        size_t s = k;
        while (k < j && !((uint8_t)ptext[k] == 0xC2 && k + 1 < j && (uint8_t)ptext[k + 1] == 0xAD)) k++;
        out.text(ptext.data() + s, k - s);
        if (k < j) k += 2;
      }
      i = j;
    }
    out.italic(false), out.bold(false);
    if (!hyphenJoin) out.text(" ");
    have = false;
  }

  // A line of page p, in reading order. toc: the contents entries pointing here (they start a block).
  void line(const PageLines &p, const Line &l, const OutlineEnt *toc, int ntoc, const char *pool, bool firstOnPage) {
    // The text, run by run, with a space where the runs are apart.
    PsVec<char> &t = lt;
    PsVec<uint8_t> &st = lst;
    t.clear(), st.clear();
    float em = body > 0 ? body : l.size;
    if (l.dropCap >= 0) {
      const Run &d = p.runs[l.dropCap];
      t.insert(t.end(), p.text.data() + d.text, p.text.data() + d.text + d.len);
      st.insert(st.end(), d.len, (uint8_t)(d.style & ~ST_SPACE));
      if (d.style & ST_SPACE) t.push_back(' '), st.push_back(d.style & ~ST_SPACE);  // "A" "wonderful": a word of its own
    }
    for (uint32_t i = 0; i < l.n; i++) {
      const Run &r = p.runs[p.order[l.first + i]];
      if (i) {
        const Run &q = p.runs[p.order[l.first + i - 1]];
        float gap = r.x0 - q.x1;
        // Apart, or the run before ended with a space ("Use " then an italic "e-file").
        bool apart = gap > 0.12f * std::min(q.size, r.size) || (q.style & ST_SPACE);
        if (apart && !t.empty() && t.back() != ' ') t.push_back(' '), st.push_back(r.style & q.style & ~ST_SPACE);
      }
      t.insert(t.end(), p.text.data() + r.text, p.text.data() + r.text + r.len);
      st.insert(st.end(), r.len, (uint8_t)(r.style & ~ST_SPACE));
    }
    if (t.empty()) return;
    // "• text": a list item; the pager draws the bullet.
    int blen;
    uint32_t first = getUtf8(t.data(), t.data() + t.size(), blen);
    bool item = first == 0x2022 || first == 0x25E6 || first == 0x25AA || first == 0x2023 || first == 0x25CF || first == 0x25A0 || first == 0x2219;
    if (item) {
      while ((size_t)blen < t.size() && t[blen] == ' ') blen++;
      t.erase(t.begin(), t.begin() + blen), st.erase(st.begin(), st.begin() + blen);
      if (t.empty()) return;
    }
    Txt s{t.data(), t.size()};
    // What kind of block the line would start.
    BlockKind kind = BK_PARA;
    bool heading = false, rule = false;
    float ratio = body > 0 ? l.size / body : 1;
    if (s.size() < 160) {
      if (ratio >= 1.6f) kind = BK_H1, heading = true;
      else if (ratio >= 1.3f) kind = BK_H2, heading = true;
      else if (ratio >= 1.12f) kind = BK_H3, heading = true;
    }
    float colW = p.right[l.col] - p.left[l.col];
    float mL = l.x0 - p.left[l.col], mR = p.right[l.col] - l.x1;
    bool centred = colW > 10 * em && mL > 3 * em && mR > 3 * em && fabsf(mL - mR) < 1.5f * em;
    // A break in the text: "* * *", "⁂", a row of bullets.
    int marks = 0;
    for (size_t i = 0; i < s.size() && marks >= 0;) {
      int len;
      uint32_t c = getUtf8(s.data() + i, s.data() + s.size(), len);
      i += len;
      if (c == '*' || c == '#' || c == '~' || c == 0x2042 || c == 0x2217 || c == 0xB7 || c == 0x2022 || c == 0x2726 || c == 0x2727 || c == 0x25C6) marks++;
      else if (c != ' ') marks = -1;
    }
    if (marks > 0 && s.size() <= 24) rule = true;
    // Is it the start of a new block?
    bool newBlock = !have || ntoc > 0, sameCol = have && ppage == p.index && pcol == l.col;
    bool shortPrev = have && pShort;
    bool sentence = have && endsSentence(ptext.data(), ptext.size());
    bool indented = mL > 0.7f * em && mL < 10 * em;
    float lp = pitch > 0 ? pitch : 1.25f * em;
    if (have && !newBlock) {
      if (heading || pHeading) newBlock = !(heading && pHeading && pkind == kind && sameCol && py - l.y < (ptext.size() <= 8 ? 3.2f : 2.2f) * l.size);
      else if (sameCol && py > l.y) {
        if (py - l.y > 1.45f * lp) newBlock = true, gapVotes += sentence;
        else if (indented && fabsf(l.x0 - px0) > 0.5f * em) newBlock = true, indentVotes++;
        else if (shortPrev && (sentence || (pVeryShort && pPunct))) newBlock = true;
      } else {
        // A new page or column: the same tests, but no gap to go by.
        if (indented && !centred) newBlock = true;
        else if (shortPrev && (sentence || (pVeryShort && pPunct))) newBlock = true;
        else if (sentence && gapVotes > indentVotes) newBlock = true;
      }
      if (!heading && centred && !pShort) newBlock = true;
      if (pkind == BK_CENTER && !(centred && sameCol && py - l.y < 1.6f * lp)) newBlock = true;
      if (rule || item) newBlock = true;
    }
    if (!heading && l.bold && s.size() < 80 && (!have || newBlock)) kind = BK_H3, heading = true;
    if (!heading && centred && newBlock && s.size() < 100) kind = BK_CENTER;
    if (item) kind = BK_ITEM, heading = false;
    // A word broken at the line's end: "propor-" "tion" are one.
    bool hyphen = false;
    if (have && !newBlock) {
      size_t n = ptext.size();
      bool dash = n >= 2 && ptext[n - 1] == '-' && isalpha((uint8_t)ptext[n - 2]);
      dash = dash || (n >= 3 && (uint8_t)ptext[n - 3] == 0xE2 && (uint8_t)ptext[n - 2] == 0x80 && (uint8_t)ptext[n - 1] == 0x90);
      dash = dash || (n >= 2 && (uint8_t)ptext[n - 2] == 0xC2 && (uint8_t)ptext[n - 1] == 0xAD);
      int len;
      hyphen = dash && isLower(getUtf8(s.data(), s.data() + s.size(), len));
    }
    flushPending(hyphen);
    if (rule) {
      out.rule();
      return;
    }
    if (newBlock) {
      out.endBlock();
      bool broke = false;
      for (int i = 0; i < ntoc; i++) {
        if (toc[i].level == 1 && !broke) out.pageBreak(), broke = true;
        out.tocEntry(pool + toc[i].title, toc[i].level);
      }
      // No outline: the biggest headings are the chapters.
      if (!outline && kind == BK_H1 && firstOnPage && letters(s) >= 3) {
        out.pageBreak();
        t.push_back(0);
        out.tocEntry(t.data(), 1);
        t.pop_back();
        s.p = t.data();
      }
      out.beginBlock(kind);
    } else if ((pkind == BK_CENTER && kind == BK_CENTER) || (pHeading && heading && ptext.size() <= 8)) out.lineBreak();  // "II" over "The Mail"
    ptext.swap(t);
    pstyle.swap(st);
    if (newBlock) pkind = kind, pHeading = heading;
    have = true;
    px0 = l.x0, px1 = l.x1, py = l.y, psize = l.size, ppage = p.index, pcol = l.col;
    pShort = p.right[l.col] - l.x1 > std::max(2.0f * em, 0.12f * colW);
    // Under half the width, after a comma or a stop: the end of a paragraph even without a sentence's end.
    pVeryShort = l.x1 - p.left[l.col] < 0.5f * colW && s.size() > 1;
    pPunct = s.size() && strchr(".,;:!?)", s.back());
  }

  void finish() { flushPending(false), out.endBlock(); }
};

// ---------------------------------------------------------------- the book

struct PageRef {
  uint32_t obj, res;  // the page, and the node of the tree its /Resources are in
  int16_t box[4];
  int16_t rotate;
};

int16_t clamp16(double d) { return (int16_t)(d != d ? 0 : d > 32000 ? 32000 : d < -32000 ? -32000 : d); }

// A title from the document information: what tools leave there isn't always one.
std::string cleanTitle(std::string t) {
  auto trim = [](std::string &s) {
    size_t a = 0, b = s.size();
    while (a < b && (uint8_t)s[a] <= ' ') a++;
    while (b > a && (uint8_t)s[b - 1] <= ' ') b--;
    s = s.substr(a, b - a);
  };
  trim(t);
  static const char *const PREFIX[] = {"Microsoft Word - ", "Microsoft PowerPoint - "};
  for (const char *p : PREFIX)
    if (!t.compare(0, strlen(p), p)) t = t.substr(strlen(p));
  std::string low;
  for (char c : t) low += (char)tolower((uint8_t)c);
  static const char *const EXT[] = {".doc", ".docx", ".indd", ".pdf", ".rtf", ".txt", ".qxd", ".qxp", ".odt", ".tex", ".dvi"};
  for (const char *e : EXT)
    if (low.size() >= strlen(e) && !low.compare(low.size() - strlen(e), strlen(e), e)) return std::string();
  if (low == "untitled" || low == "title" || low == "unknown" || low == "document" || low.compare(0, 8, "untitled") == 0) return std::string();
  trim(t);
  return t;
}

std::string infoString(Doc &d, Sl info, const char *key, size_t max = 400) {
  Obj h;
  Sl v;
  std::string b;
  if (!d.getKey(info, key, h, v) || !vString(v, b, 2 * max + 2)) return std::string();
  std::string s = textString(b);
  if (s.size() > max) s.resize(max);  // the flow writer and bookCleanDescription mend a cut character
  return s;
}

void readInfo(Doc &d, BookMeta &meta) {
  Obj io;
  if (!d.info || !d.get(d.info, io)) return;
  meta.title = cleanTitle(infoString(d, io.v(), "Title"));
  meta.author = cleanTitle(infoString(d, io.v(), "Author"));
  // /Subject: the description, unless it is junk (a file's name, "untitled") or the title again.
  std::string subject = infoString(d, io.v(), "Subject", BOOK_DESCRIPTION_MAX);
  std::string tidy = cleanTitle(subject);
  if (tidy.empty() || folded(tidy.data(), tidy.size()) == folded(meta.title.data(), meta.title.size())) return;
  bookCleanDescription(subject.data(), subject.size(), meta.description, BOOK_DESCRIPTION_MAX);
}

class PdfJob : public BookJob {
 public:
  static void *operator new(size_t n) noexcept { return psMalloc(n); }
  static void operator delete(void *p) { free(p); }

  PdfJob(FlowWriter &w, const char *coverPath) : out(w), interp(doc, fonts, runs, text), para(w), coverPath_(coverPath ? coverPath : "") {}
  ~PdfJob() override {
    for (PageLines *p : queue) delete p;
    if (cover_) fclose(cover_);
  }

  BookErr start(const char *path, BookMeta &meta) {
    BookErr e = doc.load(path, false);
    if (e != BE_OK) return e;
    meta_ = &meta;
    return doc.needRebuild ? BE_OK : setup();
  }

  // With the table there: the information, the pages, the outline.
  BookErr setup() {
    ready = true;
    if (doc.encrypted) return BE_ENCRYPTED;
    BookMeta &meta = *meta_;
    readInfo(doc, meta);
    Obj ro;
    if (!doc.get(doc.root, ro)) {
      if (!doc.rebuilt) return BE_DAMAGED;
      loosePages();  // no catalog left: the pages found will do
      return pages.empty() ? BE_DAMAGED : BE_OK;
    }
    std::string lang;
    Obj lh;
    Sl lv;
    if (doc.getKey(ro.v(), "Lang", lh, lv) && vString(lv, lang, 64)) meta.language = textString(lang);
    if (!walkPages(ro) || pages.empty()) {
      // The tree may be what a wrong table broke; or the file is cut short, and
      // the tree with it: the pages found in the file, in the file's order.
      if (!doc.rebuilt && doc.rebuild() && doc.get(doc.root, ro)) walkPages(ro);
      if (pages.empty()) loosePages();
      if (pages.empty()) return BE_DAMAGED;
    }
    walkOutline(ro);
    findCover();
    para.outline = !outline.empty();
    return BE_OK;
  }

  int step() override {
    if (!ready) {
      if (!doc.rebuildStep(REBUILD_BYTES)) return 0;
      BookErr e = doc.root || !doc.loose.empty() ? setup() : BE_DAMAGED;
      return e == BE_OK ? 0 : -(int)e;
    }
    if (cover_) {
      // The cover's JPEG, copied a block a step.
      uint32_t n;
      const uint8_t *p = coverLeft ? doc.f.at(coverOff, n) : nullptr;
      if (p) {
        n = std::min(n, coverLeft);
        if (fwrite(p, 1, n, cover_) != n) coverLeft = 0, coverOk = false;
        else coverOff += n, coverLeft -= n;
      } else if (coverLeft) coverOk = false, coverLeft = 0;  // the file ended first
      if (!coverLeft) {
        if (fclose(cover_) != 0) coverOk = false;
        cover_ = nullptr;
        if (coverOk) meta_->cover = 'j';
        else remove(coverPath_.c_str());
      }
      return 0;
    }
    if (page >= pages.size()) {
      finishAll();
      // A scan: next to no text for its pages, or text in fonts no one can read.
      uint32_t n = (uint32_t)pages.size() - (coverPage ? 1 : 0);  // a cover has no text to count
      if (interp.lost > interp.shown && interp.lost > 200) return -(int)BE_NO_TEXT;
      if (n > 1 && out.textBytes() < 50 * n) return -(int)BE_NO_TEXT;
      return BOOK_DONE;
    }
    if (!inPage) {
      const PageRef &pr = pages[page];
      Obj po, rn, rh;
      Sl res;
      res.p = (const uint8_t *)"<<>>", res.n = 4;
      if (!doc.get(pr.obj, po)) {
        page++;
        return progress();
      }
      if (pr.res && doc.get(pr.res, rn) && doc.getKey(rn.v(), "Resources", rh, res) && kindOf(res) == 'd') {}
      else res.p = (const uint8_t *)"<<>>", res.n = 4;
      float box[4] = {(float)pr.box[0], (float)pr.box[1], (float)pr.box[2], (float)pr.box[3]};
      runs.clear(), text.clear();
      interp.full = false;
      inPage = interp.begin(po, res, pr.rotate, box);
      height = box[3] - box[1];
      if (!inPage) {
        page++;
        return progress();
      }
    }
    bool ended = interp.run(STEP_BYTES);
    if (ended || interp.full) {
      interp.closeRun();
      enqueue();
      interp.full = false;
      runs.clear(), text.clear();
    }
    if (ended) inPage = false, page++, interp.endAll();
    return progress();
  }

 private:
  int progress() const { return pages.empty() ? 0 : (int)((uint64_t)page * 999 / pages.size()); }

  void enqueue() {
    PageLines *p = new PageLines();
    p->index = page;
    p->height = height > 10 ? height : 792;
    p->runs.swap(runs);
    p->text.swap(text);
    layoutPage(*p);
    para.count(*p);
    queue.push_back(p);
    while (queue.size() - done > 2) finalize();
  }

  void finalize() {
    PageLines *p = queue[done];
    PageLines *near[4];
    int nn = 0;
    for (size_t k = done >= 2 ? done - 2 : 0; k < queue.size() && k <= done + 2; k++)
      if (k != done && queue[k]->index != p->index) near[nn++] = queue[k];
    dropEdges(*p, near, nn, para.body);
    // The contents entries of this page (and of pages before it with no text) go with its lines.
    PsVec<OutlineEnt> here;
    while (nextToc < outline.size() && outline[nextToc].page <= p->index) here.push_back(outline[nextToc++]);
    PsVec<int> at(here.size(), -1);
    int firstLine = -1, last = -1;
    for (size_t k = 0; k < p->lines.size(); k++)
      if (!p->lines[k].drop) {
        firstLine = (int)k;
        break;
      }
    if (firstLine < 0) {
      nextToc -= here.size();  // nothing to point at: the next page with text gets them
    } else {
      for (size_t e = 0; e < here.size(); e++) {
        std::string want = folded(pool.data() + here[e].title, strlen(pool.data() + here[e].title));
        int hit = -1;
        for (size_t k = firstLine; k < p->lines.size() && hit < 0 && !want.empty(); k++) {
          if (p->lines[k].drop) continue;
          std::string s = lineText(*p, p->lines[k]);
          std::string got = folded(s.data(), s.size());
          if (got.size() >= 3 && (got.compare(0, want.size(), want) == 0 || want.compare(0, got.size(), got) == 0)) hit = (int)k;
        }
        at[e] = std::max(hit < 0 ? firstLine : hit, last);
        last = at[e];
      }
      bool first = true;
      for (size_t k = firstLine; k < p->lines.size(); k++) {
        if (p->lines[k].drop) continue;
        size_t a = 0;
        while (a < here.size() && at[a] < (int)k) a++;
        size_t b = a;
        while (b < here.size() && at[b] == (int)k) b++;
        para.line(*p, p->lines[k], here.data() + a, (int)(b - a), pool.data(), first);
        first = false;
      }
    }
    p->slim();
    done++;
    while (done > 2) {
      delete queue[0];
      queue.erase(queue.begin());
      done--;
    }
  }

  void finishAll() {
    if (finished) return;
    finished = true;
    while (done < queue.size()) finalize();
    para.finish();
  }

  // The page tree, without recursion: a stack of nodes still to see, and
  // what they inherit (/Resources, the page's size, its rotation).
  bool walkPages(Obj &rootObj) {
    pages.clear();
    Sl pv;
    uint32_t top;
    if (!dictGet(rootObj.v(), "Pages", pv) || !vRef(pv, top)) return false;
    PsVec<uint8_t> seen((doc.xref.size() + 8) / 8, 0);
    PsVec<PageRef> stack;
    stack.push_back(PageRef{top, 0, {0, 0, 612, 792}, 0});
    while (!stack.empty() && pages.size() < MAX_PAGES) {
      PageRef w = stack.back();
      stack.pop_back();
      if (w.obj / 8 >= seen.size() || (seen[w.obj / 8] & (1 << (w.obj & 7)))) continue;
      seen[w.obj / 8] |= 1 << (w.obj & 7);
      Obj o;
      if (!doc.get(w.obj, o) || kindOf(o.v()) != 'd') continue;
      Sl v, d = o.v();
      if (dictGet(d, "Resources", v)) w.res = w.obj;
      Obj bh;
      Sl box;
      if (doc.getKey(d, "CropBox", bh, box) || doc.getKey(d, "MediaBox", bh, box)) {
        ArrIt it(box);
        Sl e;
        double b[4];
        int i = 0;
        for (; i < 4 && it.next(e) && vNum(e, b[i]); i++) {}
        if (i == 4) {
          if (b[0] > b[2]) std::swap(b[0], b[2]);
          if (b[1] > b[3]) std::swap(b[1], b[3]);
          for (int k = 0; k < 4; k++) w.box[k] = clamp16(b[k]);
        }
      }
      double r;
      if (doc.getNum(d, "Rotate", r)) w.rotate = clamp16(r);
      Obj kh;
      Sl kids;
      bool isTree = doc.getKey(d, "Kids", kh, kids) && kindOf(kids) == 'a';
      if (!isTree) {
        pages.push_back(w);
        continue;
      }
      // The kids, the first on top.
      size_t at = stack.size();
      ArrIt it(kids);
      Sl e;
      uint32_t ref;
      while (it.next(e) && stack.size() < MAX_PAGES * 2u)
        if (vRef(e, ref)) stack.push_back(PageRef{ref, w.res, {w.box[0], w.box[1], w.box[2], w.box[3]}, w.rotate});
      std::reverse(stack.begin() + at, stack.end());
    }
    // Where each page object is in the order of pages: for the outline's destinations.
    byObj.clear();
    for (size_t i = 0; i < pages.size(); i++) byObj.push_back(((uint64_t)pages[i].obj << 32) | i);
    std::sort(byObj.begin(), byObj.end());
    return true;
  }

  void loosePages() {
    PsVec<uint32_t> &l = doc.loose;
    std::sort(l.begin(), l.end(), [&](uint32_t a, uint32_t b) { return doc.xref[a].off < doc.xref[b].off; });
    for (uint32_t num : l) pages.push_back(PageRef{num, num, {0, 0, 612, 792}, 0});
    byObj.clear();
    for (size_t i = 0; i < pages.size(); i++) byObj.push_back(((uint64_t)pages[i].obj << 32) | i);
    std::sort(byObj.begin(), byObj.end());
  }

  int pageOfObj(uint32_t obj) const {
    auto it = std::lower_bound(byObj.begin(), byObj.end(), (uint64_t)obj << 32);
    return it != byObj.end() && (uint32_t)(*it >> 32) == obj ? (int)(uint32_t)*it : -1;
  }

  // v, resolved, copied into hold (it may be in an object about to go).
  bool keep(Sl v, Obj &hold, Sl &found) {
    Obj r;
    Sl x;
    if (!doc.resolve(v, r, x)) return false;
    hold.set(x.p, x.n);
    found = hold.v();
    return true;
  }

  // A named destination: in the names tree (/Names /Dests) or the old /Dests dictionary.
  bool named(Obj &rootObj, Sl key, Obj &hold, Sl &found) {
    std::string want;
    bool isName = kindOf(key) == 'n';
    char nm[256];
    if (isName) {
      if (!vName(key, nm, sizeof nm)) return false;
      want = nm;
    } else if (!vString(key, want, 512)) return false;
    Obj dh;
    Sl dests;
    Sl dv;
    if (doc.getKey(rootObj.v(), "Dests", dh, dests) && kindOf(dests) == 'd' && dictGet(dests, want.c_str(), dv)) return keep(dv, hold, found);
    Obj nh, th;
    Sl names, node;
    if (!doc.getKey(rootObj.v(), "Names", nh, names) || !doc.getKey(names, "Dests", th, node)) return false;
    Obj cur;
    cur.set(node.p, node.n);
    for (int depth = 0; depth < 32; depth++) {
      Obj ah;
      Sl arr;
      if (doc.getKey(cur.v(), "Names", ah, arr) && kindOf(arr) == 'a') {
        ArrIt it(arr);
        Sl k, v;
        std::string ks;
        while (it.next(k) && it.next(v)) {
          ks.clear();
          if (vString(k, ks, 512) && ks == want) return keep(v, hold, found);
        }
        return false;
      }
      Obj kh;
      Sl kids;
      if (!doc.getKey(cur.v(), "Kids", kh, kids) || kindOf(kids) != 'a') return false;
      ArrIt it(kids);
      Sl e;
      Obj down;  // the kid to go down into (kids may be in cur: it changes after the loop)
      while (down.b.empty() && it.next(e)) {
        Obj ko;
        Sl kid;
        if (!doc.resolve(e, ko, kid)) continue;
        Obj lh;
        Sl lim, lo, hi;
        std::string a, b;
        if (doc.getKey(kid, "Limits", lh, lim)) {
          ArrIt li(lim);
          if (!li.next(lo) || !li.next(hi) || !vString(lo, a, 512) || !vString(hi, b, 512)) continue;
          if (want < a || want > b) continue;
        }
        down.set(kid.p, kid.n);
      }
      if (down.b.empty()) return false;
      cur.b.swap(down.b);
    }
    return false;
  }

  // A destination to a page index: [page /XYZ ...], a name, a dictionary with /D. -1: nowhere.
  // A name or a dictionary leads to another destination: followed a few times, not recursively.
  int destPage(Obj &rootObj, Sl d) {
    Obj cur;
    cur.set(d.p, d.n);
    for (int depth = 0; depth < 4; depth++) {
      Obj h, next;
      Sl v, nv;
      if (!doc.resolve(cur.v(), h, v)) return -1;
      char k = kindOf(v);
      if (k == 'a') {
        ArrIt it(v);
        Sl first;
        uint32_t ref;
        double n;
        if (!it.next(first)) return -1;
        if (vRef(first, ref)) return pageOfObj(ref);
        if (vNum(first, n) && n >= 0 && n < pages.size()) return (int)n;
        return -1;
      }
      if (k == 'n' || k == 's') {
        if (!named(rootObj, v, next, nv)) return -1;
      } else if (k == 'd' && dictGet(v, "D", nv)) next.set(nv.p, nv.n);
      else return -1;
      cur.b.swap(next.b);
    }
    return -1;
  }

  // The outline (the bookmarks), without recursion: the next sibling of each level waits on a stack.
  void walkOutline(Obj &rootObj) {
    Obj oh;
    Sl ol, v;
    uint32_t cur;
    if (!doc.getKey(rootObj.v(), "Outlines", oh, ol) || kindOf(ol) != 'd' || !dictGet(ol, "First", v) || !vRef(v, cur)) return;
    PsVec<uint8_t> seen((doc.xref.size() + 8) / 8, 0);
    uint32_t next[16];
    int level = 1, visits = 0;
    while (cur && outline.size() < MAX_OUTLINE && visits++ < 20000) {
      uint32_t go = 0;
      if (cur / 8 < seen.size() && !(seen[cur / 8] & (1 << (cur & 7)))) {
        seen[cur / 8] |= 1 << (cur & 7);
        Obj o;
        if (doc.get(cur, o) && kindOf(o.v()) == 'd') {
          Sl d = o.v();
          std::string raw;
          Obj th;
          Sl tv;
          if (doc.getKey(d, "Title", th, tv)) vString(tv, raw, 1024);
          std::string title = textString(raw);
          int pg = -1;
          Sl dv;
          if (dictGet(d, "Dest", dv)) pg = destPage(rootObj, dv);
          else {
            Obj ah;
            Sl a, s;
            if (doc.getKey(d, "A", ah, a) && kindOf(a) == 'd' && dictGet(a, "S", s) && vNameIs(s, "GoTo") && dictGet(a, "D", dv)) {
              Obj keep;
              keep.set(dv.p, dv.n);
              pg = destPage(rootObj, keep.v());
            }
          }
          if (pg >= 0 && !title.empty()) {
            OutlineEnt e;
            e.page = (uint32_t)pg, e.level = (uint8_t)std::min(level, 3), e.title = (uint32_t)pool.size();
            pool.insert(pool.end(), title.begin(), title.end());
            pool.push_back(0);
            outline.push_back(e);
          }
          uint32_t sib = 0, child = 0;
          if (dictGet(d, "Next", v)) vRef(v, sib);
          if (dictGet(d, "First", v)) vRef(v, child);
          if (child && level < 16) {
            next[level - 1] = sib;
            level++;
            go = child;
          } else go = sib;
        }
      }
      // Back up to a level that has a next sibling.
      while (!go && level > 1) go = next[--level - 1];
      cur = go;
    }
    // One entry above all the others (the book's title over its chapters): the chapters are the first level.
    size_t ones = 0;
    for (const OutlineEnt &e : outline) ones += e.level == 1;
    if (ones == 1 && outline.size() > 2 && outline[0].level == 1)
      for (size_t i = 1; i < outline.size(); i++) outline[i].level = (uint8_t)std::max(1, outline[i].level - 1);
    // Entries in the order of their pages, as the flow is written (the outline's own order within a page).
    // (titles go into the pool in the outline's order: its offset keeps that order)
    std::sort(outline.begin(), outline.end(), [](const OutlineEnt &a, const OutlineEnt &b) { return a.page != b.page ? a.page < b.page : a.title < b.title; });
  }

  FlowWriter &out;
  Doc doc;
  FontCache fonts;
  PsVec<Run> runs;
  PsVec<char> text;
  Interp interp;
  Para para;
  PsVec<PageRef> pages;
  PsVec<uint64_t> byObj;
  PsVec<OutlineEnt> outline;
  PsVec<char> pool;
  size_t nextToc = 0;
  PsVec<PageLines *> queue;
  size_t done = 0;
  uint32_t page = 0;
  float height = 792;
  bool inPage = false, finished = false, ready = false;
  BookMeta *meta_ = nullptr;
  std::string coverPath_;
  FILE *cover_ = nullptr;
  uint32_t coverOff = 0, coverLeft = 0;
  bool coverOk = true, coverPage = false;

  // A cover: the first page essentially one JPEG picture (one image on it, a
  // DCTDecode one, its shape the page's), copied as it is to coverPath.
  void findCover() {
    if (coverPath_.empty() || pages.empty()) return;
    const PageRef &pr = pages[0];
    Obj rn, rh, xh, img;
    Sl res, xo;
    if (!pr.res || !doc.get(pr.res, rn) || !doc.getKey(rn.v(), "Resources", rh, res) || !doc.getKey(res, "XObject", xh, xo) || kindOf(xo) != 'd') return;
    Lex lx(xo);
    lx.next();
    int images = 0;
    for (int seen = 0; seen < 64; seen++) {
      Tk t = lx.next();
      if (t == T_EOF || t == T_DC) break;
      if (t != T_NAME) continue;
      Sl v, st;
      uint32_t ref;
      Obj o;
      if (!readValue(lx, v)) break;
      if (vRef(v, ref) && doc.get(ref, o) && o.stream && dictGet(o.v(), "Subtype", st) && vNameIs(st, "Image")) images++, img = o;
    }
    if (images != 1) return;
    Obj fh;
    Sl fv, e;
    double w = 0, h = 0;
    if (!doc.getKey(img.v(), "Filter", fh, fv)) return;
    if (kindOf(fv) == 'a') {
      ArrIt it(fv);
      if (!it.next(e) || it.next(fv)) return;  // DCT alone
      fv = e;
    }
    if (!vNameIs(fv, "DCTDecode") && !vNameIs(fv, "DCT")) return;
    if (!doc.getNum(img.v(), "Width", w) || !doc.getNum(img.v(), "Height", h) || w < 200 || h < 200) return;
    float pw = pr.box[2] - pr.box[0], ph = pr.box[3] - pr.box[1];
    if (pr.rotate % 180) std::swap(pw, ph);
    double shape = (w / h) / (pw / (ph > 1 ? ph : 1));
    if (shape < 0.8 || shape > 1.25 || img.sLen < 1000 || img.sLen > (8u << 20)) return;
    uint8_t soi[2];
    if (doc.f.read(img.sOff, soi, 2) != 2 || soi[0] != 0xFF || soi[1] != 0xD8) return;
    cover_ = fopen(coverPath_.c_str(), "wb");
    coverOff = img.sOff, coverLeft = img.sLen;
    coverPage = cover_ != nullptr;
  }
};

}  // namespace

BookJob *pdfJob(const char *path, FlowWriter &out, const char *coverPath, BookMeta &meta, BookErr &err) {
  PdfJob *job = new PdfJob(out, coverPath);
  if (!job) return err = BE_MEMORY, nullptr;
  err = job->start(path, meta);
  if (err != BE_OK) {
    delete job;
    return nullptr;
  }
  return job;
}

bool pdfProbe(const char *path, BookMeta &meta, BookErr &err) {
  Doc d;
  BookErr e = d.load(path, true);
  if (e == BE_OPEN || e == BE_FORMAT) return err = e, false;
  err = BE_OK;
  if (d.encrypted || e != BE_OK) return true;  // its title can't be read here: the name will do
  readInfo(d, meta);
  return true;
}
