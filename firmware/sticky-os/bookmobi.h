// The insides of the MOBI reader (bookmobi.cpp), for that file and its tests
// (firmware/test/ebook/mobi_test.cpp). The app and the other formats use
// mobiJob and mobiProbe (bookformats.h), not this.
//
// A MOBI, AZW, AZW3 or PRC file is a Palm database: a table of records. The
// first holds the headers (PalmDOC, MOBI, EXTH: the names and where things
// are), the next ones the text, compressed a record (4096 bytes of text) at a
// time, then indexes (the contents, and a KF8 book's skeletons and
// fragments), then the pictures. A file made by Amazon's tools may hold the
// book twice, old MOBI first and KF8 (AZW3) after a boundary record.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

#include "ebook.h"
#include "psalloc.h"

namespace mobi {

static const uint32_t NONE = 0xFFFFFFFF;  // "no such record" in the headers

// PalmDOC's LZ77. Returns the bytes written, never more than cap; broken
// input gives some text, never a read or write out of bounds.
size_t palmdocUnpack(const uint8_t *in, size_t n, uint8_t *out, size_t cap);

// How much of a text record is text: what the "extra data flags" say follows
// it (the rest of a character cut by the record's end, indexing data) is
// taken off. False: the entries don't fit the record.
bool textSize(const uint8_t *rec, size_t n, uint16_t flags, size_t &len);

// Windows-1252 to UTF-8, added to out.
void latin1ToUtf8(const uint8_t *p, size_t n, std::string &out);

// What the reader takes from the EXTH records.
struct Exth {
  std::string title, author, language;                 // 503, the first 100, 524
  std::string description;                              // 103, made plain (bookCleanDescription)
  uint32_t cover = NONE, thumb = NONE, kf8 = NONE;     // 201, 202, 121
};
// p: from the "EXTH" mark to the record's end. False when the list is cut or
// broken (what was read before that stays in x).
bool parseExth(const uint8_t *p, size_t n, bool utf8, Exth &x);

// The Huffman scheme of older Kindle files (compression 17480): a HUFF
// record with the code tables, CDIC records with the phrases, which may
// themselves be coded.
class HuffCdic {
 public:
  bool setHuff(const uint8_t *rec, size_t n);
  bool addCdic(const uint8_t *rec, size_t n);
  void reserve(size_t dictBytes) { dict_.reserve(dictBytes); }
  bool ready() const { return table_.size() == 256 && !phrase_.empty(); }
  // One text record. False: the data is broken. Text past cap is dropped.
  bool unpack(const uint8_t *in, size_t n, uint8_t *out, size_t cap, size_t &len);
  size_t memory() const { return dict_.size() + phrase_.size() * 4 + 1024 + 66 * 8; }

 private:
  bool expand(const uint8_t *in, size_t n, int depth);
  PsVec<uint32_t> table_;  // by a code's first byte: its length, or where to start looking
  PsVec<uint64_t> range_;  // by length 0..32: the smallest code, then (33 on) the largest
  PsVec<uint8_t> dict_;    // the CDIC records' phrases, one after another
  PsVec<uint32_t> phrase_; // where each starts in dict_ (at its length)
  uint8_t *out_ = nullptr;
  size_t cap_ = 0, len_ = 0;
  uint32_t work_ = 0;      // codes still allowed for this record: a bound on hostile data
};

// One book in the file: the headers of record 0, or of a KF8 half's first record.
struct Part {
  uint32_t base = 0;  // the record its headers are in; its other records count from here
  uint16_t compression = 1, recSize = 4096, encryption = 0, extraFlags = 0;
  uint32_t textLen = 0, textRecs = 0;
  uint32_t version = 0, codepage = 1252, locale = 0;
  uint32_t firstImage = NONE, huffRec = NONE, huffCount = 0;
  uint32_t ncx = NONE, frag = NONE, skel = NONE, fdst = NONE;
  std::string fullName;
  Exth exth;
  bool utf8() const { return codepage == 65001; }
  bool kf8() const { return version == 8; }
};

class Book {
 public:
  Book() {}
  ~Book();
  Book(const Book &) = delete;
  Book &operator=(const Book &) = delete;

  // The record table and record 0: enough for title, author and language.
  BookErr open(const char *path);
  // Picks the half to read (KF8 when it's there and whole), refuses DRM and
  // text records that are missing, readies the buffers. part is set after it.
  BookErr openText();
  // Compression 17480 only: the records (HUFF, then CDICs) to load, one a
  // call, before readText. BE_MEMORY: the phrases are more than DICT_MAX (3 MB).
  uint32_t huffRecords() const { return part.compression == 17480 ? part.huffCount : 0; }
  BookErr loadHuff(uint32_t k);
  // Text record i (0 .. part.textRecs-1), unpacked: p is good until the next call.
  BookErr readText(uint32_t i, const uint8_t *&p, size_t &n);

  // Any record: its size (0: not in the file, as after a cut download) and its bytes.
  uint32_t records() const { return offs_.empty() ? 0 : (uint32_t)offs_.size() - 1; }
  uint32_t recordSize(uint32_t i) const;
  bool read(uint32_t i, uint32_t off, void *to, size_t n);
  // The first max bytes of record i at most; false if it can't be read.
  bool load(uint32_t i, PsVec<uint8_t> &to, size_t max);

  std::string title, author, language, description;  // UTF-8; description as BookMeta has it
  bool palmDoc = false;                 // a plain PalmDOC text (TEXtREAd), no MOBI header
  Part first, part;                     // record 0's, and the one being read
  uint32_t imageBase() const { return first.firstImage; }
  size_t memory() const { return offs_.size() * 4 + raw_.size() + text_.size() + huff_.memory(); }

 private:
  BookErr parsePart(uint32_t rec, Part &p);
  FILE *f_ = nullptr;
  uint32_t size_ = 0;
  PsVec<uint32_t> offs_;  // where each record starts; one more: the file's size
  PsVec<uint8_t> raw_, text_;
  HuffCdic huff_;
};

// An index (INDX records): entries of a label and numbered values, laid out
// as the TAGX table of its first record says. The contents (NCX) and a KF8
// book's skeleton and fragment tables are indexes.
struct IndexEntry {
  const uint8_t *label;
  size_t labelLen;
  struct Val {
    uint8_t tag, at;
    uint32_t v;
  } vals[16];  // the first values of the entry's tags; more are skipped
  int n;
  bool get(int tag, int at, uint32_t &v) const;
};
typedef bool (*IndexFn)(void *arg, const IndexEntry &e);  // false: stop, the index is no good

class Index {
 public:
  // rec: the index's first record. False: not an index, or broken.
  bool begin(Book &b, uint32_t rec);
  uint32_t records() const { return count_; }                 // of entries, read with read(0 .. records-1)
  uint32_t cncxRecord() const { return rec_ + 1 + count_; }   // the strings the entries point into
  uint32_t cncxRecords() const { return cncx_; }
  bool read(Book &b, uint32_t k, IndexFn fn, void *arg);

 private:
  uint32_t rec_ = 0, count_ = 0, cncx_ = 0, control_ = 0;
  uint8_t tagx_[4 * 24];
  int tags_ = 0;
  PsVec<uint8_t> buf_;
};

}  // namespace mobi
