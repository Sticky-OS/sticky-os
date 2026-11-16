// MOBI, AZW, AZW3 (KF8) and PRC books without DRM, to the flow (bookflow.h).
//
// The text of these books is HTML, compressed a record at a time (PalmDOC's
// LZ77, or the Huffman scheme of older Kindle files), so it streams: a record
// is read, unpacked and given to HtmlToFlow, a few a step. What's worked out
// before the text, in steps of their own:
//
// - the cover, a picture record copied as it is;
// - the table of contents. First choice is the book's own index (the "NCX":
//   titles, and where in the text each points). An old MOBI without one has a
//   contents page of <a filepos=...> links, found through its <guide>; those
//   few records are read ahead. Failing both, the headings make the contents.
//   The places are byte positions in the markup: the text is fed up to each
//   one, and the entry given to the writer there;
// - for KF8, the skeleton and fragment tables. A KF8 text is not in reading
//   order: each chapter file is stored as its outer markup (the skeleton)
//   followed by the pieces that belong inside it. The tables say where each
//   piece goes, and the files are put together again as they stream by; every
//   file starts a new page. Without usable tables the text is fed as stored,
//   which reads the same for the usual book (skeletons hold no text).
//
// Nothing here trusts the file: every count, offset and length is checked.
#include "bookmobi.h"

#include <string.h>

#include <algorithm>

#include "bookcharset.h"
#include "bookformats.h"

namespace mobi {

static uint16_t be16(const uint8_t *p) { return (uint16_t)(p[0] << 8 | p[1]); }
static uint32_t be32(const uint8_t *p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }

static const size_t RECORD_MAX = 256 * 1024;      // a record the reader loads whole: the format's own are under 64 KB
static const size_t DICT_MAX = 3 * 1024 * 1024;   // a Huffman book's phrases (real ones: tens of KB to a few MB), and how many
static const size_t PHRASES_MAX = 256 * 1024;
static const int HUFF_DEPTH = 20;                 // phrases made of phrases
static const size_t DESCRIPTION_RAW = 16384;      // of a windows-1252 EXTH 103: plenty for BOOK_DESCRIPTION_MAX of text

// ---------------------------------------------------------------- text

static void putUtf8(uint32_t cp, std::string &out) {
  if (cp < 0x80) out += (char)cp;
  else if (cp < 0x800) out += (char)(0xC0 | cp >> 6), out += (char)(0x80 | (cp & 0x3F));
  else if (cp < 0x10000) out += (char)(0xE0 | cp >> 12), out += (char)(0x80 | (cp >> 6 & 0x3F)), out += (char)(0x80 | (cp & 0x3F));
  else out += (char)(0xF0 | cp >> 18), out += (char)(0x80 | (cp >> 12 & 0x3F)), out += (char)(0x80 | (cp >> 6 & 0x3F)), out += (char)(0x80 | (cp & 0x3F));
}

static uint32_t latin1(uint8_t c) {
  // Windows-1252's 0x80..0x9F; the rest is Latin-1, which is Unicode's first page.
  static const uint16_t HIGH[32] = {0x20AC, 0x81,   0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
                                    0x2039, 0x0152, 0x8D,   0x017D, 0x8F,   0x90,   0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
                                    0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x9D,   0x017E, 0x0178};
  return c >= 0x80 && c < 0xA0 ? HIGH[c - 0x80] : c;
}

void latin1ToUtf8(const uint8_t *p, size_t n, std::string &out) {
  for (size_t i = 0; i < n; i++) putUtf8(latin1(p[i]), out);
}

// A name from the file, as UTF-8 that is safe to show: cut at max bytes,
// anything that isn't a whole character becomes '?', a control a space.
static void setName(std::string &to, const uint8_t *p, size_t n, bool utf8, size_t max) {
  to.clear();
  while (n && !p[n - 1]) n--;
  if (n > max) n = max;
  if (!utf8) return latin1ToUtf8(p, n, to);
  for (size_t i = 0; i < n;) {
    uint8_t c = p[i];
    size_t len = c < 0x80 ? 1 : c >= 0xC2 && c <= 0xDF ? 2 : c >= 0xE0 && c <= 0xEF ? 3 : c >= 0xF0 && c <= 0xF4 ? 4 : 0;
    bool ok = len && i + len <= n && c >= 0x20;
    for (size_t k = 1; ok && k < len; k++) ok = (p[i + k] & 0xC0) == 0x80;
    if (ok) to.append((const char *)p + i, len), i += len;
    else to += c < 0x20 ? ' ' : '?', i++;
  }
}

size_t palmdocUnpack(const uint8_t *in, size_t n, uint8_t *out, size_t cap) {
  size_t i = 0, o = 0;
  while (i < n && o < cap) {
    uint8_t c = in[i++];
    if (c >= 1 && c <= 8) {  // the next c bytes as they are
      size_t k = std::min((size_t)c, std::min(n - i, cap - o));
      memcpy(out + o, in + i, k);
      i += k, o += k;
    } else if (c < 0x80) {
      out[o++] = c;
    } else if (c >= 0xC0) {  // a space and a character
      out[o++] = ' ';
      if (o < cap) out[o++] = c ^ 0x80;
    } else {  // a copy of text already written: 11 bits how far back, 3 bits how long
      if (i >= n) break;
      unsigned v = (unsigned)c << 8 | in[i++];
      size_t dist = v >> 3 & 0x7FF, len = (v & 7) + 3;
      if (!dist || dist > o) continue;  // broken: nothing to copy from
      for (; len && o < cap; len--, o++) out[o] = out[o - dist];  // may run into itself: byte by byte
    }
  }
  return o;
}

bool textSize(const uint8_t *rec, size_t n, uint16_t flags, size_t &len) {
  // Entries of bits 15..1, last first: each ends with its own size, a number
  // written 7 bits a byte and read backwards (the byte with the high bit starts it).
  for (int bit = 15; bit >= 1; bit--) {
    if (!(flags >> bit & 1)) continue;
    uint32_t size = 0;
    for (size_t i = n > 4 ? n - 4 : 0; i < n; i++) {
      if (rec[i] & 0x80) size = 0;
      size = size << 7 | (rec[i] & 0x7F);
    }
    if (!size || size > n) return false;
    n -= size;
  }
  // Bit 0 is another kind: the bytes that finish a character cut by the
  // record's end (they start the next record too), then one byte whose low
  // two bits count them.
  if (flags & 1) {
    if (!n) return false;
    size_t size = (rec[n - 1] & 3) + 1u;
    if (size > n) return false;
    n -= size;
  }
  len = n;
  return true;
}

// ---------------------------------------------------------------- headers

// EXTH 103 as BookMeta wants it: straight from the record when it's UTF-8,
// through a buffer in PSRAM when it's windows-1252 (the markup may be long).
static void description(const uint8_t *v, size_t n, bool utf8, std::string &out) {
  if (utf8) return bookCleanDescription((const char *)v, n, out, BOOK_DESCRIPTION_MAX);
  n = std::min(n, DESCRIPTION_RAW);
  PsVec<char> u;
  u.reserve(3 * n);
  for (size_t i = 0; i < n; i++) {
    uint32_t cp = latin1(v[i]);
    if (cp < 0x80) u.push_back((char)cp);
    else if (cp < 0x800) u.push_back((char)(0xC0 | cp >> 6)), u.push_back((char)(0x80 | (cp & 0x3F)));
    else u.push_back((char)(0xE0 | cp >> 12)), u.push_back((char)(0x80 | (cp >> 6 & 0x3F))), u.push_back((char)(0x80 | (cp & 0x3F)));
  }
  bookCleanDescription(u.data(), u.size(), out, BOOK_DESCRIPTION_MAX);
}

bool parseExth(const uint8_t *p, size_t n, bool utf8, Exth &x) {
  if (n < 12 || memcmp(p, "EXTH", 4)) return false;
  uint32_t count = be32(p + 8);
  size_t at = 12;
  bool author = false;
  for (uint32_t i = 0; i < count; i++) {
    if (n - at < 8) return false;
    uint32_t type = be32(p + at), len = be32(p + at + 4);
    if (len < 8 || len > n - at) return false;
    const uint8_t *v = p + at + 8;
    size_t vn = len - 8;
    at += len;
    if (type == 100 && !author) setName(x.author, v, vn, utf8, 200), author = true;
    else if (type == 503) setName(x.title, v, vn, utf8, 300);
    else if (type == 524) setName(x.language, v, vn, utf8, 16);
    else if (type == 103) description(v, vn, utf8, x.description);
    else if (vn == 4 && type == 201) x.cover = be32(v);
    else if (vn == 4 && type == 202) x.thumb = be32(v);
    else if (vn == 4 && type == 121) x.kf8 = be32(v);
  }
  return true;
}

// The language of a header's locale (a Windows language id), when EXTH doesn't say.
static const char *localeLanguage(uint32_t locale) {
  static const struct { uint8_t id; char code[3]; } LANG[] = {
      {0x02, "bg"}, {0x03, "ca"}, {0x04, "zh"}, {0x05, "cs"}, {0x06, "da"}, {0x07, "de"}, {0x08, "el"}, {0x09, "en"},
      {0x0A, "es"}, {0x0B, "fi"}, {0x0C, "fr"}, {0x0E, "hu"}, {0x10, "it"}, {0x11, "ja"}, {0x12, "ko"}, {0x13, "nl"},
      {0x14, "no"}, {0x15, "pl"}, {0x16, "pt"}, {0x18, "ro"}, {0x19, "ru"}, {0x1A, "hr"}, {0x1B, "sk"}, {0x1D, "sv"},
      {0x1F, "tr"}, {0x22, "uk"}, {0x24, "sl"}, {0x25, "et"}, {0x26, "lv"}, {0x27, "lt"},
  };
  for (auto &l : LANG)
    if (l.id == (locale & 0xFF)) return l.code;
  return "";
}

Book::~Book() {
  if (f_) fclose(f_);
}

uint32_t Book::recordSize(uint32_t i) const {
  if (i >= records()) return 0;
  uint32_t a = offs_[i], b = offs_[i + 1];
  return a >= 78 + 8 * records() && a < b && b <= size_ ? b - a : 0;
}

bool Book::read(uint32_t i, uint32_t off, void *to, size_t n) {
  uint32_t size = recordSize(i);
  if (off > size || n > size - off) return false;
  return fseek(f_, (long)(offs_[i] + off), SEEK_SET) == 0 && fread(to, 1, n, f_) == n;
}

bool Book::load(uint32_t i, PsVec<uint8_t> &to, size_t max) {
  size_t n = std::min((size_t)recordSize(i), max);
  to.resize(n);
  return n && read(i, 0, to.data(), n);
}

BookErr Book::open(const char *path) {
  f_ = fopen(path, "rb");
  if (!f_) return BE_OPEN;
  uint8_t h[78];
  if (fread(h, 1, sizeof h, f_) != sizeof h) return BE_FORMAT;
  palmDoc = !memcmp(h + 60, "TEXtREAd", 8);
  if (!palmDoc && memcmp(h + 60, "BOOKMOBI", 8)) return BE_FORMAT;
  long end = fseek(f_, 0, SEEK_END) == 0 ? ftell(f_) : -1;
  if (end < 0) return BE_OPEN;
  size_ = (uint32_t)std::min<unsigned long>((unsigned long)end, 0xFFFFFFF0ul);
  uint32_t n = be16(h + 76);
  if (!n || 78 + 8 * n > size_) return BE_DAMAGED;
  {
    PsVec<uint8_t> table(8 * n);
    if (fseek(f_, 78, SEEK_SET) != 0 || fread(table.data(), 1, table.size(), f_) != table.size()) return BE_OPEN;
    offs_.resize(n + 1);
    for (uint32_t i = 0; i < n; i++) offs_[i] = be32(&table[8 * i]);
    offs_[n] = size_;
  }
  BookErr e = parsePart(0, first);
  if (e != BE_OK) return e;
  title = first.exth.title.size() ? first.exth.title : first.fullName;
  if (title.empty()) {  // the database's own name: the title's start, with '_' for spaces
    setName(title, h, strnlen((const char *)h, 32), false, 32);
    std::replace(title.begin(), title.end(), '_', ' ');
  }
  author = first.exth.author;
  language = first.exth.language.size() ? first.exth.language : localeLanguage(first.locale);
  description = first.exth.description;
  return BE_OK;
}

BookErr Book::parsePart(uint32_t rec, Part &p) {
  PsVec<uint8_t> buf;
  if (!load(rec, buf, RECORD_MAX) || buf.size() < 16) return BE_DAMAGED;
  const uint8_t *r = buf.data();
  size_t len = buf.size();
  p = Part();
  p.base = rec;
  p.compression = be16(r);
  p.textLen = be32(r + 4);
  p.textRecs = be16(r + 8);
  p.recSize = be16(r + 10);
  if (!p.recSize) p.recSize = 4096;
  if (palmDoc || len < 24 || memcmp(r + 16, "MOBI", 4)) return BE_OK;  // PalmDOC: these 16 bytes are all there is
  p.encryption = be16(r + 12);
  // The header grew with the versions: a field is there if it's inside it.
  size_t hend = (size_t)std::min((uint64_t)16 + be32(r + 20), (uint64_t)len);
  auto field = [&](size_t at) { return at + 4 <= hend ? be32(r + at) : NONE; };
  if (field(28) != NONE) p.codepage = field(28);
  if (field(36) != NONE) p.version = field(36);
  if (field(92) != NONE) p.locale = field(92);
  p.firstImage = field(108);
  p.huffRec = field(112);
  p.huffCount = field(116) == NONE ? 0 : field(116);
  if (hend >= 244) p.extraFlags = be16(r + 242);
  p.ncx = field(244);
  if (p.kf8()) p.fdst = field(192), p.frag = field(248), p.skel = field(252);
  uint32_t nameAt = field(84), nameLen = field(88);
  if (nameAt != NONE && nameLen != NONE && nameAt <= len && nameLen <= len - nameAt) setName(p.fullName, r + nameAt, nameLen, p.utf8(), 300);
  if (field(128) != NONE && (field(128) & 0x40) && hend < len) parseExth(r + hend, len - hend, p.utf8(), p.exth);
  return BE_OK;
}

BookErr Book::openText() {
  part = first;
  // Are all of a part's text records in the file, and how big is the biggest?
  auto whole = [&](const Part &p, size_t &longest) {
    longest = 0;
    for (uint32_t i = 1; i <= p.textRecs; i++) {
      uint32_t size = recordSize(p.base + i);
      if (!size || size > RECORD_MAX) return false;
      longest = std::max(longest, (size_t)size);
    }
    return true;
  };
  // A file with the book twice: the KF8 half starts after a "BOUNDARY" record.
  // It's the one to read, unless the file was cut short before its end.
  size_t longest = 0;
  uint32_t b = first.exth.kf8;
  if (!first.kf8() && b != NONE && b > 0 && b < records()) {
    uint8_t mark[8];
    Part k;
    if (read(b - 1, 0, mark, 8) && !memcmp(mark, "BOUNDARY", 8) && parsePart(b, k) == BE_OK && k.kf8() && !k.encryption && whole(k, longest)) part = k;
  }
  if (part.encryption) return BE_DRM;
  if (part.compression != 1 && part.compression != 2 && part.compression != 17480) return BE_FORMAT;
  if (!part.textRecs) return BE_NO_TEXT;
  if (!whole(part, longest)) return BE_DAMAGED;
  if (part.compression == 17480 && (part.huffRec == NONE || part.huffCount < 2 || part.huffCount > 4096)) return BE_DAMAGED;
  raw_.resize(longest);
  if (part.compression != 1) text_.resize(std::max((size_t)part.recSize, (size_t)4096));
  return BE_OK;
}

BookErr Book::loadHuff(uint32_t k) {
  PsVec<uint8_t> buf;
  uint64_t rec = (uint64_t)part.base + part.huffRec + k;
  if (rec >= records() || recordSize((uint32_t)rec) > RECORD_MAX || !load((uint32_t)rec, buf, RECORD_MAX)) return BE_DAMAGED;
  if (!k) {  // the phrases' room, once: their records' size (more than the board can spare: say so)
    uint64_t dict = 0;
    for (uint32_t i = 1; i < part.huffCount && rec + i < records(); i++) dict += recordSize((uint32_t)(rec + i));
    if (dict > DICT_MAX) return BE_MEMORY;
    huff_.reserve((size_t)dict);
  }
  bool ok = k ? huff_.addCdic(buf.data(), buf.size()) : huff_.setHuff(buf.data(), buf.size());
  return ok ? BE_OK : BE_DAMAGED;
}

BookErr Book::readText(uint32_t i, const uint8_t *&p, size_t &n) {
  if (i >= part.textRecs) return BE_DAMAGED;
  uint32_t rec = part.base + 1 + i, size = recordSize(rec);
  size_t len;
  if (!size || size > raw_.size()) return BE_DAMAGED;
  if (!read(rec, 0, raw_.data(), size)) return BE_OPEN;
  if (!textSize(raw_.data(), size, part.extraFlags, len)) return BE_DAMAGED;
  p = text_.data();
  if (part.compression == 1) p = raw_.data(), n = len;
  else if (part.compression == 2) n = palmdocUnpack(raw_.data(), len, text_.data(), text_.size());
  else if (!huff_.ready() || !huff_.unpack(raw_.data(), len, text_.data(), text_.size(), n)) return BE_DAMAGED;
  return BE_OK;
}

// ---------------------------------------------------------------- HUFF/CDIC

bool HuffCdic::setHuff(const uint8_t *r, size_t n) {
  if (n < 24 || memcmp(r, "HUFF\0\0\0\x18", 8)) return false;
  uint32_t t1 = be32(r + 8), t2 = be32(r + 12);
  if (t1 > n || n - t1 < 1024 || t2 > n || n - t2 < 256) return false;
  table_.resize(256);
  for (int i = 0; i < 256; i++) table_[i] = be32(r + t1 + 4 * i);
  // The codes of each length are a run of numbers; left-aligned to 32 bits,
  // a code is at least `smallest` of its length. Lengths never used have a
  // smallest no code reaches.
  range_.assign(66, 0);
  for (int len = 1; len <= 32; len++) {
    range_[len] = (uint64_t)be32(r + t2 + 8 * (len - 1)) << (32 - len);
    range_[33 + len] = (((uint64_t)be32(r + t2 + 8 * (len - 1) + 4) + 1) << (32 - len)) - 1;
  }
  return true;
}

bool HuffCdic::addCdic(const uint8_t *r, size_t n) {
  if (n < 16 || memcmp(r, "CDIC\0\0\0\x10", 8)) return false;
  uint32_t total = be32(r + 8), bits = be32(r + 12);
  if (bits > 16 || total > PHRASES_MAX || total < phrase_.size() || dict_.size() + n > DICT_MAX) return false;
  if (phrase_.empty()) phrase_.reserve(total);
  size_t count = std::min<size_t>((size_t)1 << bits, total - phrase_.size());
  if (count > (n - 16) / 2) return false;
  size_t base = dict_.size();
  dict_.insert(dict_.end(), r + 16, r + n);  // phrase offsets count from the header's end
  for (size_t i = 0; i < count; i++) {
    size_t at = be16(r + 16 + 2 * i);
    if (at + 2 > n - 16 || (size_t)(be16(r + 16 + at) & 0x7FFF) > n - 16 - at - 2) return false;
    phrase_.push_back((uint32_t)(base + at));
  }
  return true;
}

// The eight bytes at pos, zeros past the end.
static uint64_t window(const uint8_t *in, size_t n, size_t pos) {
  uint64_t x = 0;
  for (size_t i = 0; i < 8; i++) x = x << 8 | (pos + i < n ? in[pos + i] : 0);
  return x;
}

bool HuffCdic::expand(const uint8_t *in, size_t n, int depth) {
  if (depth > HUFF_DEPTH) return false;
  uint64_t bits = (uint64_t)n * 8, x = window(in, n, 0);
  size_t pos = 0;
  int ahead = 32;  // x >> ahead: the next 32 bits
  while (len_ < cap_) {
    if (ahead <= 0) pos += 4, x = window(in, n, pos), ahead += 32;
    uint32_t code = (uint32_t)(x >> ahead), entry = table_[code >> 24], len = entry & 0x1F;
    uint64_t largest;
    if (!len) return false;
    if (entry & 0x80) {  // a short code: the table has it all
      largest = (((uint64_t)(entry >> 8) + 1) << (32 - len)) - 1;
    } else {  // a long one: the first length whose smallest code isn't above it
      while (len <= 32 && code < range_[len]) len++;
      if (len > 32) return false;
      largest = range_[33 + len];
    }
    if (bits < len) break;  // the padding after the last code
    bits -= len, ahead -= (int)len;
    if (largest < code) return false;
    uint64_t which = (largest - code) >> (32 - len);
    if (which >= phrase_.size() || !work_--) return false;
    const uint8_t *ph = &dict_[phrase_[which]];
    size_t size = be16(ph) & 0x7FFF;
    if (be16(ph) & 0x8000) {  // plain text
      size = std::min(size, cap_ - len_);
      memcpy(out_ + len_, ph + 2, size);
      len_ += size;
    } else if (!expand(ph + 2, size, depth + 1)) {  // codes again
      return false;
    }
  }
  return true;
}

bool HuffCdic::unpack(const uint8_t *in, size_t n, uint8_t *out, size_t cap, size_t &len) {
  out_ = out, cap_ = cap, len_ = 0;
  work_ = (uint32_t)(8 * cap + 1024);  // real text takes at most a code or two a byte
  bool ok = expand(in, n, 0);
  len = len_;
  return ok;
}

// ---------------------------------------------------------------- indexes

// A number of 7 bits a byte, the last byte marked by its high bit.
static bool varint(const uint8_t *r, size_t &at, size_t end, uint32_t &v) {
  v = 0;
  for (int i = 0; i < 5 && at < end; i++) {
    uint8_t c = r[at++];
    v = v << 7 | (c & 0x7F);
    if (c & 0x80) return true;
  }
  return false;
}

bool IndexEntry::get(int tag, int at, uint32_t &v) const {
  for (int i = 0; i < n; i++)
    if (vals[i].tag == tag && vals[i].at == at) return v = vals[i].v, true;
  return false;
}

bool Index::begin(Book &b, uint32_t rec) {
  tags_ = 0;
  rec_ = rec;
  if (b.recordSize(rec) > RECORD_MAX || !b.load(rec, buf_, RECORD_MAX)) return false;
  const uint8_t *r = buf_.data();
  size_t n = buf_.size();
  if (n < 56 || memcmp(r, "INDX", 4)) return false;
  size_t tagx = be32(r + 4);
  count_ = be32(r + 24);
  cncx_ = be32(r + 52);
  if (count_ > 4096 || cncx_ > 4096 || tagx > n || n - tagx < 12 || memcmp(r + tagx, "TAGX", 4)) return false;
  size_t size = be32(r + tagx + 4);
  control_ = be32(r + tagx + 8);
  if (size < 12 || size > n - tagx || (size - 12) / 4 > sizeof tagx_ / 4 || control_ > 8) return false;
  tags_ = (int)((size - 12) / 4);
  memcpy(tagx_, r + tagx + 12, tags_ * 4);
  return true;
}

bool Index::read(Book &b, uint32_t k, IndexFn fn, void *arg) {
  uint64_t rec = (uint64_t)rec_ + 1 + k;
  if (k >= count_ || rec >= b.records() || b.recordSize((uint32_t)rec) > RECORD_MAX || !b.load((uint32_t)rec, buf_, RECORD_MAX)) return false;
  const uint8_t *r = buf_.data();
  size_t n = buf_.size();
  if (n < 28 || memcmp(r, "INDX", 4)) return false;
  size_t idxt = be32(r + 20), entries = be32(r + 24);  // IDXT: where each entry starts
  if (idxt > n || n - idxt < 4 || memcmp(r + idxt, "IDXT", 4) || entries > (n - idxt - 4) / 2) return false;
  for (size_t j = 0; j < entries; j++) {
    size_t at = be16(r + idxt + 4 + 2 * j), end = j + 1 < entries ? be16(r + idxt + 4 + 2 * (j + 1)) : idxt;
    if (at >= end || end > idxt) return false;
    IndexEntry e;
    e.n = 0;
    e.labelLen = r[at++];
    e.label = r + at;
    at += e.labelLen;
    size_t control = at, data = at + control_;
    if (data > end) return false;
    // The control bytes say, tag by tag, how many values the entry has (or,
    // when a tag's bits are all set and there are several, how many bytes of them).
    struct { uint8_t tag, each; bool bytes; uint32_t n; } has[24];
    int nhas = 0;
    uint32_t byte = 0;
    for (int t = 0; t < tags_; t++) {
      const uint8_t *x = tagx_ + 4 * t;  // tag, values a time, mask, end of a control byte
      if (x[3] & 1) {
        byte++;
        continue;
      }
      if (byte >= control_) return false;
      uint32_t mask = x[2], v = r[control + byte] & mask;
      if (!v) continue;
      has[nhas].tag = x[0], has[nhas].each = x[1], has[nhas].bytes = false;
      if (v == mask && (mask & (mask - 1))) {
        has[nhas].bytes = true;
        if (!varint(r, data, end, v)) return false;
      } else {
        for (; mask && !(mask & 1); mask >>= 1) v >>= 1;
      }
      has[nhas++].n = v;
    }
    for (int h = 0; h < nhas; h++) {
      uint32_t v, got = 0;
      size_t from = data;
      uint32_t want = has[h].bytes ? has[h].n : has[h].n * has[h].each;
      while (has[h].bytes ? data - from < want : got < want) {
        if (!varint(r, data, end, v)) return false;
        if (e.n < (int)(sizeof e.vals / sizeof e.vals[0]) && got < 256) e.vals[e.n].tag = has[h].tag, e.vals[e.n].at = (uint8_t)got, e.vals[e.n].v = v, e.n++;
        got++;
      }
    }
    if (!fn(arg, e)) return false;
  }
  return true;
}

}  // namespace mobi

using namespace mobi;

namespace {

// ---------------------------------------------------------------- the contents

// The places the table of contents points at, with their titles.
struct Target {
  uint32_t pos, title;  // title: where it starts in the titles' buffer
  uint16_t seq;
  uint8_t level;
};

class Contents {
 public:
  static const size_t MAX = 2000;
  bool full() const { return list_.size() >= MAX; }
  size_t size() const { return list_.size(); }
  const Target &at(size_t i) const { return list_[i]; }
  const char *title(size_t i) const { return &titles_[list_[i].title]; }
  void clear() { list_.clear(), titles_.clear(); }
  // title: UTF-8 from the book, made safe here.
  void add(uint32_t pos, const char *title, size_t n, int level) {
    if (full() || !n) return;
    std::string t;
    setName(t, (const uint8_t *)title, n, true, 240);
    Target e = {pos, (uint32_t)titles_.size(), (uint16_t)list_.size(), (uint8_t)(level < 1 ? 1 : level > 3 ? 3 : level)};
    titles_.insert(titles_.end(), t.begin(), t.end());
    titles_.push_back(0);
    list_.push_back(e);
  }
  // In the order of the book (an index may list a level at a time).
  void sort() {
    std::sort(list_.begin(), list_.end(), [](const Target &a, const Target &b) { return a.pos != b.pos ? a.pos < b.pos : a.seq < b.seq; });
  }

 private:
  PsVec<Target> list_;
  PsVec<char> titles_;
};

// The strings of an index (CNCX records): an entry's title is an offset into them.
struct Strings {
  static const int MAX = 8;
  PsVec<uint8_t> data;
  uint32_t start[MAX + 1];
  int n = 0;
  void load(Book &b, uint32_t rec, uint32_t count) {
    PsVec<uint8_t> buf;
    start[0] = 0;
    for (n = 0; n < MAX && (uint32_t)n < count; n++) {
      if (b.recordSize(rec + n) > 0x10000 || !b.load(rec + n, buf, 0x10000)) break;
      data.insert(data.end(), buf.begin(), buf.end());
      start[n + 1] = (uint32_t)data.size();
    }
  }
  bool get(uint32_t off, const uint8_t *&p, size_t &len) const {
    uint32_t rec = off >> 16, v;
    if (rec >= (uint32_t)n) return false;
    size_t at = start[rec] + (off & 0xFFFF), end = start[rec + 1];
    if (!varint(data.data(), at, end, v) || v > end - at) return false;
    p = data.data() + at, len = v;
    return true;
  }
};

static bool isSpace(uint8_t c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
static uint8_t lower(uint8_t c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

// Does s (n bytes) start with the lower-case word w, as a whole name?
static bool startsWord(const char *s, size_t n, const char *w) {
  size_t k = strlen(w);
  if (n < k) return false;
  for (size_t i = 0; i < k; i++)
    if (lower(s[i]) != (uint8_t)w[i]) return false;
  return n == k || !(lower(s[k]) >= 'a' && lower(s[k]) <= 'z');
}

// The value of an attribute in a tag's text (without the brackets), quoted or
// not; nullptr if the tag hasn't it.
static const char *attr(const std::string &tag, const char *name, size_t &len) {
  size_t k = strlen(name);
  for (size_t i = 1; i + k < tag.size(); i++) {
    if (!isSpace(tag[i - 1]) || !startsWord(tag.data() + i, tag.size() - i, name)) continue;
    size_t at = i + k;
    while (at < tag.size() && isSpace(tag[at])) at++;
    if (at >= tag.size() || tag[at] != '=') continue;
    for (at++; at < tag.size() && isSpace(tag[at]);) at++;
    char quote = at < tag.size() && (tag[at] == '"' || tag[at] == '\'') ? tag[at++] : 0;
    size_t end = at;
    while (end < tag.size() && (quote ? tag[end] != quote : !isSpace(tag[end]) && tag[end] != '/')) end++;
    len = end - at;
    return tag.data() + at;
  }
  return nullptr;
}

static bool attrNumber(const std::string &tag, const char *name, uint32_t &v) {
  size_t len;
  const char *s = attr(tag, name, len);
  if (!s || !len || len > 10) return false;
  uint64_t x = 0;
  for (size_t i = 0; i < len; i++) {
    if (s[i] < '0' || s[i] > '9') return false;
    x = x * 10 + (s[i] - '0');
  }
  v = (uint32_t)std::min(x, (uint64_t)NONE - 1);
  return true;
}

// A link's text as a title: character references resolved, UTF-8.
static std::string titleText(const std::string &raw, bool utf8) {
  static const struct { const char *name; uint16_t cp; } NAMED[] = {
      {"amp", '&'},       {"lt", '<'},        {"gt", '>'},        {"quot", '"'},      {"apos", '\''},
      {"nbsp", ' '},      {"mdash", 0x2014},  {"ndash", 0x2013},  {"lsquo", 0x2018}, {"rsquo", 0x2019},
      {"ldquo", 0x201C},  {"rdquo", 0x201D},  {"hellip", 0x2026},
  };
  std::string out;
  for (size_t i = 0; i < raw.size(); i++) {
    uint8_t c = raw[i];
    size_t semi = c == '&' ? raw.find(';', i) : std::string::npos;
    if (semi != std::string::npos && semi - i >= 2 && semi - i <= 9) {
      uint32_t cp = 0;
      const char *s = raw.data() + i + 1;
      size_t n = semi - i - 1;
      if (s[0] == '#') {
        bool hex = n > 1 && lower(s[1]) == 'x';
        for (size_t k = hex ? 2 : 1; k < n; k++) {
          uint8_t d = lower(s[k]);
          int digit = d >= '0' && d <= '9' ? d - '0' : hex && d >= 'a' && d <= 'f' ? d - 'a' + 10 : -1;
          if (digit < 0 || cp > 0x10FFFF) {
            cp = 0;
            break;
          }
          cp = cp * (hex ? 16 : 10) + digit;
        }
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = 0;
      } else {
        for (auto &e : NAMED)
          if (strlen(e.name) == n && !memcmp(e.name, s, n)) cp = e.cp;
      }
      if (cp) {
        putUtf8(cp, out);
        i = semi;
        continue;
      }
    }
    if (utf8) out += (char)c;
    else latin1ToUtf8(&c, 1, out);
  }
  return out;
}

// Reads an old MOBI's contents page: every <a filepos=N>title</a> is an
// entry, up to the page's end.
struct ContentsPage {
  std::string tag, title;
  bool inTag = false, inLink = false, done = false;
  uint32_t target = 0, seen = 0;

  void feed(const uint8_t *p, size_t n, Contents &toc, bool utf8) {
    for (size_t i = 0; i < n && !done; i++) {
      uint8_t c = p[i];
      if (++seen > 256 * 1024) done = true;  // a contents page is a few KB
      if (inTag) {
        if (c != '>') {
          if (tag.size() < 300) tag += (char)c;
          continue;
        }
        inTag = false;
        if (startsWord(tag.data(), tag.size(), "a")) {
          if (attrNumber(tag, "filepos", target)) inLink = true, title.clear();
        } else if (startsWord(tag.data(), tag.size(), "/a")) {
          if (inLink) {
            std::string t = titleText(title, utf8);
            toc.add(target, t.data(), t.size(), 1);
          }
          inLink = false;
          if (toc.full()) done = true;
        } else if (startsWord(tag.data(), tag.size(), "mbp:pagebreak")) {
          if (toc.size()) done = true;
        } else if (inLink && startsWord(tag.data(), tag.size(), "br")) {
          title += ' ';
        }
      } else if (c == '<') {
        inTag = true;
        tag.clear();
      } else if (inLink && title.size() < 400) {
        title += (char)c;
      }
    }
  }
};

// ---------------------------------------------------------------- feeding

// Gives the markup to HtmlToFlow and, as the bytes fed reach the places the
// contents point at, the entries to the writer.
//
// An entry belongs to the block written next, and a place is often a tag
// inside that block (<h2><a id="chap01"></a>Chapter I</h2> pointed at by its
// <a>): given there, the entry would cut the heading off from its text. So
// near a place the tags that follow the last text are held back, and the
// entry goes in before them: where the paragraph before ended.
class Feeder {
 public:
  Feeder(FlowWriter &out, HtmlToFlow &html, const Contents &toc) : out_(out), html_(html), toc_(toc) {}
  uint32_t pos() const { return pos_; }
  void at(uint32_t pos) { pos_ = pos; }  // the position of the next byte, as the contents count them
  void emit(const uint8_t *p, size_t n);
  void flush() {
    if (hold_.size()) html_.feed(hold_.data(), hold_.size());
    hold_.clear();
    holding_ = false;
  }
  void endDocument() {
    flush();
    html_.finish();
    inTag_ = false;
  }

 private:
  static const uint32_t NEAR = 1024;  // how far before a place the watch starts, and the most held back
  void direct(const uint8_t *p, size_t n) {
    if (n) html_.feed((const char *)p, n);
  }
  FlowWriter &out_;
  HtmlToFlow &html_;
  const Contents &toc_;
  PsVec<char> hold_;
  size_t next_ = 0, tagAt_ = 0;
  uint32_t pos_ = 0;
  bool inTag_ = false, holding_ = false;
};

void Feeder::emit(const uint8_t *p, size_t n) {
  while (n) {
    if (next_ >= toc_.size()) {
      flush();
      direct(p, n);
      pos_ += (uint32_t)n;
      return;
    }
    uint32_t place = toc_.at(next_).pos;
    if (place <= pos_) {  // what's held, if anything, is the tags of the block to come
      out_.tocEntry(toc_.title(next_), toc_.at(next_).level);
      next_++;
      continue;
    }
    uint32_t gap = place - pos_;
    if (!holding_ && gap > NEAR) {
      size_t k = std::min(n, (size_t)(gap - NEAR));
      direct(p, k);
      inTag_ = false;
      p += k, n -= k, pos_ += (uint32_t)k;
      continue;
    }
    size_t lim = std::min(n, (size_t)gap), from = 0;  // p[from..i) not fed yet, when nothing is held
    for (size_t i = 0; i < lim; i++) {
      uint8_t c = p[i];
      if (inTag_) {
        if (holding_) hold_.push_back((char)c);
        if (c == '>') {
          inTag_ = false;
          // A picture or a rule is something on the page: the run of bare tags ends with it.
          if (holding_ && (startsWord(&hold_[tagAt_ + 1], hold_.size() - tagAt_ - 1, "img") || startsWord(&hold_[tagAt_ + 1], hold_.size() - tagAt_ - 1, "hr"))) flush(), from = i + 1;
        }
      } else if (c == '<') {
        inTag_ = true;
        if (!holding_) direct(p + from, i - from), holding_ = true;
        tagAt_ = hold_.size();
        hold_.push_back((char)c);
      } else if (isSpace(c)) {
        if (holding_) hold_.push_back((char)c);
      } else if (holding_) {  // text: what was held belongs before it after all
        flush();
        from = i;
      }
      if (holding_ && hold_.size() > NEAR) flush(), from = i + 1;
    }
    if (!holding_) direct(p + from, lim - from);
    p += lim, n -= lim, pos_ += (uint32_t)lim;
  }
}

// ---------------------------------------------------------------- the job

struct Skeleton {
  uint32_t pos, len, fragments;  // where its markup is in the text, and how many pieces follow it
};
struct Fragment {
  uint32_t insert, len;  // where it goes, counted in the text put together
};

class MobiJob : public BookJob {
 public:
  MobiJob(FlowWriter &out, const char *coverPath, BookMeta &meta) : out_(out), meta_(meta), html_(out), feed_(out, html_, toc_), coverPath_(coverPath ? coverPath : "") {}
  Book book_;
  int step() override;

 private:
  enum Phase { COVER, HUFF, FRAGMENTS, SKELETONS, NCX, GUIDE, PAGE, START, TEXT };
  static const size_t SKELETON_MAX = 256 * 1024, FRAGMENTS_MAX = 40000, SKELETONS_MAX = 8192;

  void to(Phase p) { phase_ = p, at_ = 0; }
  int indexStep(uint32_t rec, IndexFn fn, bool strings);
  static bool onFragment(void *arg, const IndexEntry &e);
  static bool onSkeleton(void *arg, const IndexEntry &e);
  static bool onNcx(void *arg, const IndexEntry &e);
  bool tablesFit() const;
  void writeCover();
  void findContentsPage();
  void startText();
  void consume(const uint8_t *p, size_t n);
  void assemble(const uint8_t *p, size_t n);
  void nextFragment();
  void documents(const uint8_t *p, size_t n);
  void newDocument();
  void plain(const uint8_t *p, size_t n);
  void finishText();

  FlowWriter &out_;
  BookMeta &meta_;
  HtmlToFlow html_;
  Contents toc_;
  Feeder feed_;
  std::string coverPath_;
  Phase phase_ = COVER;
  uint32_t at_ = 0;  // how far the phase is

  Index index_;
  Strings strings_;
  ContentsPage page_;
  uint32_t pageRec_ = 0, pageSkip_ = 0;

  // KF8: the files put together again
  PsVec<Skeleton> skeletons_;
  PsVec<Fragment> fragments_;
  PsVec<uint8_t> skeleton_;  // the one being filled
  enum { BETWEEN, IN_SKELETON, IN_FRAGMENTS } state_ = BETWEEN;
  bool tables_ = false;
  size_t skel_ = 0, frag_ = 0, used_ = 0;  // used_: how much of skeleton_ has been fed
  uint32_t fragsLeft_ = 0, fragLeft_ = 0, documents_ = 0;
  uint32_t matched_ = 0;  // of "<html", when the files are told apart by their markup

  bool plain_ = false;  // a PalmDOC of plain text: a line is a paragraph
  uint32_t rec_ = 0, rawPos_ = 0, limit_ = NONE;
};

// One record of an index a call: 0 more to come, 1 done, -1 no good.
int MobiJob::indexStep(uint32_t rec, IndexFn fn, bool strings) {
  if (!at_) {
    uint64_t abs = (uint64_t)book_.part.base + rec;
    if (rec == NONE || abs >= book_.records() || !index_.begin(book_, (uint32_t)abs)) return -1;
    if (strings) strings_.load(book_, index_.cncxRecord(), index_.cncxRecords());
    at_ = 1;
    return index_.records() ? 0 : 1;
  }
  if (!index_.read(book_, at_ - 1, fn, this)) return -1;
  return ++at_ > index_.records() ? 1 : 0;
}

bool MobiJob::onFragment(void *arg, const IndexEntry &e) {
  MobiJob *j = (MobiJob *)arg;
  Fragment f = {0, 0};
  uint64_t insert = 0;  // the label: where it goes, in digits
  for (size_t i = 0; i < e.labelLen; i++) {
    if (e.label[i] < '0' || e.label[i] > '9' || insert > NONE) return false;
    insert = insert * 10 + (e.label[i] - '0');
  }
  if (!e.labelLen || insert > NONE || !e.get(6, 1, f.len) || j->fragments_.size() >= FRAGMENTS_MAX) return false;
  f.insert = (uint32_t)insert;
  j->fragments_.push_back(f);
  return true;
}

bool MobiJob::onSkeleton(void *arg, const IndexEntry &e) {
  MobiJob *j = (MobiJob *)arg;
  Skeleton s;
  if (!e.get(1, 0, s.fragments) || !e.get(6, 0, s.pos) || !e.get(6, 1, s.len) || j->skeletons_.size() >= SKELETONS_MAX) return false;
  j->skeletons_.push_back(s);
  return true;
}

bool MobiJob::onNcx(void *arg, const IndexEntry &e) {
  MobiJob *j = (MobiJob *)arg;
  uint32_t pos = 0, row, off, label, depth = 0;
  const uint8_t *s;
  size_t n;
  // KF8 says a place as a fragment and an offset in it; old MOBI as a position in the text.
  bool placed = j->tables_ && e.get(6, 0, row) && e.get(6, 1, off) && row < j->fragments_.size();
  if (placed) pos = j->fragments_[row].insert + off;
  if ((!placed && !e.get(1, 0, pos)) || !e.get(3, 0, label) || !j->strings_.get(label, s, n)) return true;  // an entry less
  e.get(4, 0, depth);
  int level = (int)std::min(depth, (uint32_t)2) + 1;
  if (j->book_.part.utf8()) {
    j->toc_.add(pos, (const char *)s, n, level);
  } else {
    std::string t;
    latin1ToUtf8(s, std::min(n, (size_t)240), t);
    j->toc_.add(pos, t.data(), t.size(), level);
  }
  return true;
}

// The tables are used only if they describe files one after another in the text.
bool MobiJob::tablesFit() const {
  uint64_t end = 0;
  size_t f = 0;
  for (const Skeleton &s : skeletons_) {
    if (s.pos < end || s.len > SKELETON_MAX || s.fragments > fragments_.size() - f) return false;
    end = (uint64_t)s.pos + s.len;
    for (uint32_t i = 0; i < s.fragments; i++) end += fragments_[f++].len;
    if (end > NONE) return false;
  }
  return !skeletons_.empty();
}

// The cover as it is in the book: the record EXTH names, else the thumbnail,
// else the first picture that is a JPEG of some size.
void MobiJob::writeCover() {
  uint32_t base = book_.imageBase();
  if (coverPath_.empty() || base == NONE) return;
  auto kind = [&](uint64_t rec) -> char {
    uint8_t m[4];
    if (rec >= book_.records() || book_.recordSize((uint32_t)rec) < 16 || !book_.read((uint32_t)rec, 0, m, 4)) return 0;
    return m[0] == 0xFF && m[1] == 0xD8 && m[2] == 0xFF ? 'j' : !memcmp(m, "\x89PNG", 4) ? 'p' : 0;
  };
  const uint32_t named[] = {book_.part.exth.cover, book_.first.exth.cover, book_.part.exth.thumb, book_.first.exth.thumb};
  uint64_t rec = 0;
  char k = 0;
  for (int i = 0; i < 4 && !k; i++)
    if (named[i] != NONE) k = kind(rec = (uint64_t)base + named[i]);
  for (uint32_t i = 0; i < 16 && !k; i++) {
    rec = (uint64_t)base + i;
    if (rec < book_.records() && book_.recordSize((uint32_t)rec) > 10000 && kind(rec) == 'j') k = 'j';
  }
  if (!k) return;
  FILE *f = fopen(coverPath_.c_str(), "wb");
  if (!f) return;
  PsVec<uint8_t> buf(8192);
  uint32_t size = book_.recordSize((uint32_t)rec), done = 0;
  bool ok = true;
  for (; ok && done < size; done += (uint32_t)buf.size()) {
    size_t n = std::min((size_t)(size - done), buf.size());
    ok = book_.read((uint32_t)rec, done, buf.data(), n) && fwrite(buf.data(), 1, n, f) == n;
  }
  if (fclose(f) != 0) ok = false;
  if (ok) meta_.cover = k;
  else remove(coverPath_.c_str());  // a book without a cover, not a failed book
}

// An old MOBI says where its contents page is in the <guide> at the text's
// start: <reference type="toc" filepos=N>.
void MobiJob::findContentsPage() {
  PsVec<uint8_t> head;
  for (uint32_t i = 0; i < 4 && i < book_.part.textRecs; i++) {
    const uint8_t *p;
    size_t n;
    if (book_.readText(i, p, n) != BE_OK) break;
    head.insert(head.end(), p, p + n);
  }
  uint32_t pos = NONE;
  for (size_t i = 0; i + 10 < head.size() && pos == NONE; i++) {
    if (head[i] != '<' || !startsWord((const char *)&head[i + 1], head.size() - i - 1, "reference")) continue;
    size_t end = i;
    while (end < head.size() && head[end] != '>' && end - i < 400) end++;
    std::string tag((const char *)&head[i + 1], end - i - 1);
    size_t len;
    const char *type = attr(tag, "type", len);
    if (type && len == 3 && startsWord(type, 3, "toc")) attrNumber(tag, "filepos", pos);
  }
  if (pos == NONE || pos / book_.part.recSize >= book_.part.textRecs) return to(START);
  pageRec_ = pos / book_.part.recSize;  // every record but the last holds recSize bytes of text
  pageSkip_ = pos % book_.part.recSize;
  to(PAGE);
}

void MobiJob::startText() {
  const Part &p = book_.part;
  if (book_.palmDoc) {  // PalmDOC is plain text, unless it was made from HTML
    const uint8_t *s;
    size_t n;
    plain_ = true;
    if (book_.readText(0, s, n) == BE_OK)
      for (size_t i = 0; i + 6 < n && plain_; i++)
        if (s[i] == '<' && (startsWord((const char *)s + i + 1, n - i - 1, "html") || startsWord((const char *)s + i + 1, n - i - 1, "body") || startsWord((const char *)s + i + 1, n - i - 1, "p"))) plain_ = false;
  }
  if (p.kf8() && p.fdst != NONE) {
    // FDST: the text is several flows, the book first, then style sheets and drawings.
    PsVec<uint8_t> buf;
    uint64_t rec = (uint64_t)p.base + p.fdst;
    if (rec < book_.records() && book_.load((uint32_t)rec, buf, 4096) && buf.size() >= 20 && !memcmp(buf.data(), "FDST", 4)) {
      size_t at = be32(&buf[4]);
      if (be32(&buf[8]) > 0 && at <= buf.size() - 8 && be32(&buf[at + 4]) > 0) limit_ = be32(&buf[at + 4]);
    }
  }
  toc_.sort();
  if (toc_.size() < 2) toc_.clear(), html_.tocFromHeadings(true);
  html_.setCharset(p.utf8() ? "utf-8" : "windows-1252");
  to(TEXT);
}

void MobiJob::plain(const uint8_t *p, size_t n) {
  std::string line;
  for (size_t i = 0; i < n;) {
    size_t end = i;
    while (end < n && p[end] != '\n' && end - i < 200) end++;
    line.clear();
    latin1ToUtf8(p + i, end - i, line);
    out_.text(line.data(), line.size());
    if (end < n && p[end] == '\n') out_.endBlock(), end++;
    i = end;
  }
}

void MobiJob::newDocument() {
  if (documents_++) feed_.endDocument(), out_.pageBreak();
}

void MobiJob::nextFragment() {
  while (!fragLeft_) {
    if (!fragsLeft_) {  // the file is whole: the rest of its skeleton closes it
      feed_.emit(skeleton_.data() + used_, skeleton_.size() - used_);
      skel_++;
      state_ = BETWEEN;
      return;
    }
    const Fragment &f = fragments_[frag_++];
    fragsLeft_--;
    // The skeleton up to where the piece goes. A place that makes no sense
    // (behind us, past the skeleton): the piece goes in here.
    int64_t before = (int64_t)f.insert - (int64_t)feed_.pos();
    if (before > 0 && (uint64_t)before <= skeleton_.size() - used_) feed_.emit(skeleton_.data() + used_, (size_t)before), used_ += (size_t)before;
    fragLeft_ = f.len;
  }
}

void MobiJob::assemble(const uint8_t *p, size_t n) {
  uint32_t at = rawPos_;
  while (n) {
    size_t k = n;
    if (state_ == BETWEEN) {
      if (skel_ >= skeletons_.size()) return;  // past the last file: not the book's text
      const Skeleton &s = skeletons_[skel_];
      if (at < s.pos) {
        k = std::min(n, (size_t)(s.pos - at));
      } else {
        newDocument();
        skeleton_.clear();
        state_ = IN_SKELETON;
        k = 0;
      }
    } else if (state_ == IN_SKELETON) {
      const Skeleton &s = skeletons_[skel_];
      k = std::min<size_t>(n, s.len - skeleton_.size());
      skeleton_.insert(skeleton_.end(), p, p + k);
      if (skeleton_.size() == s.len) {
        used_ = 0, fragsLeft_ = s.fragments, fragLeft_ = 0;
        feed_.at(s.pos);
        state_ = IN_FRAGMENTS;
        nextFragment();
      }
    } else {
      k = std::min(n, (size_t)fragLeft_);
      feed_.emit(p, k);
      fragLeft_ -= (uint32_t)k;
      if (!fragLeft_) nextFragment();
    }
    p += k, n -= k, at += (uint32_t)k;
  }
}

// KF8 without its tables: the files are told apart by where an <html starts.
void MobiJob::documents(const uint8_t *p, size_t n) {
  static const char MARK[] = "<html";
  uint32_t before = matched_;  // of the mark, held back from the records before this one
  size_t from = 0;
  for (size_t i = 0; i < n; i++) {
    if (matched_ && p[i] == (uint8_t)MARK[matched_]) {
      if (++matched_ < 5) continue;
      size_t markAt = i + 1 - (5 - before);
      if (markAt > from) feed_.emit(p + from, markAt - from);
      newDocument();
      feed_.emit((const uint8_t *)MARK, 5);
      from = i + 1, matched_ = before = 0;
      continue;
    }
    if (matched_) {  // not a start after all
      if (before) feed_.emit((const uint8_t *)MARK, before), before = 0;
      matched_ = 0;
    }
    if (p[i] == '<') matched_ = 1;
  }
  size_t held = matched_ - before;  // the end of this record that may be a mark's start
  if (n - held > from) feed_.emit(p + from, n - held - from);
}

void MobiJob::consume(const uint8_t *p, size_t n) {
  if (plain_) plain(p, n);
  else if (tables_) assemble(p, n);
  else if (book_.part.kf8()) documents(p, n);
  else feed_.emit(p, n);
}

void MobiJob::finishText() {
  if (plain_) return;
  if (tables_ && state_ != BETWEEN) feed_.emit(skeleton_.data() + used_, skeleton_.size() - used_);  // the text ended inside a file
  if (matched_) feed_.emit((const uint8_t *)"<html", matched_);
  feed_.endDocument();
}

int MobiJob::step() {
  const Part &part = book_.part;
  switch (phase_) {
    case COVER:
      writeCover();
      to(HUFF);
      return 0;
    case HUFF: {
      if (at_ >= book_.huffRecords()) return to(part.kf8() ? FRAGMENTS : NCX), 0;
      BookErr e = book_.loadHuff(at_++);
      return e == BE_OK ? 0 : -(int)e;
    }
    case FRAGMENTS: {
      int r = indexStep(part.frag, onFragment, false);
      if (r) to(r > 0 ? SKELETONS : START);
      return 0;
    }
    case SKELETONS: {
      int r = indexStep(part.skel, onSkeleton, false);
      if (!r) return 0;
      tables_ = r > 0 && tablesFit();
      // Without the tables the index's places can't be found: the headings make the contents.
      to(tables_ ? NCX : START);
      return 0;
    }
    case NCX: {
      int r = book_.palmDoc ? -1 : indexStep(part.ncx, onNcx, true);
      if (!r) return 0;
      strings_.data = PsVec<uint8_t>();
      if (r < 0) toc_.clear();
      to(toc_.size() < 2 && !part.kf8() && !book_.palmDoc ? GUIDE : START);
      return 0;
    }
    case GUIDE:
      toc_.clear();
      findContentsPage();
      return 0;
    case PAGE: {
      const uint8_t *p;
      size_t n;
      if (pageRec_ >= part.textRecs || book_.readText(pageRec_++, p, n) != BE_OK) return to(START), 0;
      if (pageSkip_ < n) page_.feed(p + pageSkip_, n - pageSkip_, toc_, part.utf8());
      pageSkip_ = 0;
      if (page_.done) to(START);
      return 0;
    }
    case START:
      startText();
      return 0;
    case TEXT: {
      for (uint32_t done = 0; done < 32768 && rec_ < part.textRecs && rawPos_ < limit_; rec_++) {
        const uint8_t *p;
        size_t n;
        BookErr e = book_.readText(rec_, p, n);
        if (e != BE_OK) return -(int)e;
        n = std::min(n, (size_t)(limit_ - rawPos_));
        consume(p, n);
        rawPos_ += (uint32_t)n, done += (uint32_t)std::max(n, (size_t)1024);
      }
      if (rec_ < part.textRecs && rawPos_ < limit_) return 1 + (int)((uint64_t)rec_ * 998 / part.textRecs);
      finishText();
      return BOOK_DONE;
    }
  }
  return -(int)BE_DAMAGED;
}

}  // namespace

BookJob *mobiJob(const char *path, FlowWriter &out, const char *coverPath, BookMeta &meta, BookErr &err) {
  MobiJob *job = new MobiJob(out, coverPath, meta);
  err = job->book_.open(path);
  if (err == BE_OK) err = job->book_.openText();
  if (err != BE_OK) {
    delete job;
    return nullptr;
  }
  meta.title = job->book_.title;
  meta.author = job->book_.author;
  meta.language = job->book_.language;
  meta.description = job->book_.description;
  return job;
}

bool mobiProbe(const char *path, BookMeta &meta, BookErr &err) {
  Book book;
  err = book.open(path);
  if (err != BE_OK) return false;
  meta.title = book.title;
  meta.author = book.author;
  meta.language = book.language;
  meta.description = book.description;
  return true;
}
