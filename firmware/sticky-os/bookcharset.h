// What the book readers share about text: the encodings books come in
// (UTF-8, UTF-16, the single-byte Windows and ISO ones) turned into UTF-8,
// and HTML's named characters (&eacute;). Not for the app.
//
// Plain C++, no Arduino. The tables are const: they live in flash.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string>

enum Charset : uint8_t {
  CS_UTF8,
  CS_1252,     // windows-1252, and iso-8859-1 (read the same way, as browsers do)
  CS_1250,     // windows-1250: Central European (Polish, Czech, Romanian)
  CS_8859_2,   // iso-8859-2
  CS_1251,     // windows-1251: Cyrillic
  CS_KOI8R,
  CS_UTF16LE,
  CS_UTF16BE,
};

// By the name a document gives ("UTF-8", "iso-8859-1", "cp1251"; any case).
// False: a name we don't know (cs is left alone).
bool charsetByName(const char *name, size_t n, Charset &cs);
// The character of a byte 0x80..0xFF in a single-byte charset (U+FFFD: none).
uint32_t charsetHigh(Charset cs, uint8_t b);

// UTF-8 of one character: writes 1..4 bytes, returns how many.
int utf8Encode(uint32_t cp, char out[4]);
void utf8Append(std::string &s, uint32_t cp);
// Is it UTF-8 all through? A character cut off by the buffer's end is fine.
// highs (may be nullptr) gets the number of characters beyond ASCII.
bool utf8Valid(const uint8_t *p, size_t n, size_t *highs = nullptr);

// Bytes in a charset, in pieces of any size, to UTF-8 (appended to out).
class CharsetDecoder {
 public:
  explicit CharsetDecoder(Charset cs = CS_UTF8) : cs_(cs) {}
  void reset(Charset cs) { cs_ = cs, have_ = false, hi_ = 0; }
  Charset charset() const { return cs_; }
  void feed(const char *p, size_t n, std::string &out);

 private:
  Charset cs_;
  bool have_ = false;  // UTF-16: the first byte of a unit is waiting
  uint8_t first_ = 0;
  uint32_t hi_ = 0;    // UTF-16: a high surrogate is waiting for its pair
};

// The character of a named entity ("eacute", "nbsp"; without & and ;). 0: unknown.
uint32_t htmlEntity(const char *name, size_t n);
// &amp; &#233; &#xE9; &eacute; in a short string (an attribute, a title)
// replaced by their characters; what isn't an entity stays as it is.
void decodeEntities(std::string &s);

// A book's description (ebook.h's BookMeta::description) from what the book
// holds: HTML (real markup, or escaped: "&lt;p&gt;"), or plain text. Out:
// plain UTF-8, tags left out (paragraph-like ones, and <br>, part
// paragraphs), entities resolved, spaces single, paragraphs separated by one
// '\n' (in plain text, a line end parts them), no other controls, broken
// UTF-8 dropped, at most `max` bytes cut on a character.
void bookCleanDescription(const char *in, size_t n, std::string &out, size_t max = 2000);
