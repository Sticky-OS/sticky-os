// Reading a zip (an EPUB is one): the list of its files, and one file's
// bytes streamed out, stored or deflated (bookinflate.h). No zip64, no
// encryption, no multi-part archives: none of which an EPUB uses.
//
// Nothing in the file is trusted: every offset and size is checked against
// the file's real length, the list is capped at ZIP_MAX_ENTRIES files, and a
// file gives at most the size the list says (and no more than deflate could
// ever make of its packed bytes).
//
// Plain C++ over stdio, no Arduino. The list lives in PSRAM (psalloc.h):
// 20 bytes a file plus its name.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "bookinflate.h"
#include "psalloc.h"

static const int ZIP_MAX_ENTRIES = 4000;

class ZipFile {
 public:
  ZipFile() {}
  ~ZipFile() { close(); }
  ZipFile(const ZipFile &) = delete;
  ZipFile &operator=(const ZipFile &) = delete;

  // Opens the file and reads its list. False: not a zip, or damaged
  // (notZip says which: the file doesn't even start like one).
  bool open(const char *path, bool *notZip = nullptr);
  void close();
  int count() const { return (int)entries_.size(); }
  // By name, exactly; failing that, whatever the letters' case. -1: not there.
  int find(const char *name) const;
  const char *name(int i) const { return names_.data() + entries_[i].name; }
  uint32_t size(int i) const { return entries_[i].usize; }  // unpacked, as the list says

 private:
  friend class ZipReader;
  struct Entry {
    uint32_t name;    // where in names_
    uint32_t csize, usize, header;
    uint16_t method;
    bool encrypted;
  };
  FILE *f_ = nullptr;
  uint32_t fileSize_ = 0;
  PsVec<Entry> entries_;
  PsVec<char> names_;
  PsVec<uint16_t> sorted_;  // the entries in the order of their names, for find()
};

// One file of the zip, read from its start to its end. One reader at a
// time per ZipFile (they share the open file).
class ZipReader {
 public:
  ZipReader() {}
  ~ZipReader() { free(buf_); }
  ZipReader(const ZipReader &) = delete;
  ZipReader &operator=(const ZipReader &) = delete;

  // False: no such entry, a packing we don't read, or its header is damaged.
  bool open(ZipFile &zip, int index);
  // The next bytes: how many were put in buf (cap > 0), 0 at the file's
  // end, -1 if it is damaged or cut short.
  long read(uint8_t *buf, size_t cap);
  // Packed bytes taken from the zip so far: how far through the file we are.
  uint32_t packedRead() const { return ctaken_; }

 private:
  ZipFile *zip_ = nullptr;
  Inflater inf_;
  uint8_t *buf_ = nullptr;       // packed bytes waiting to be unpacked
  size_t bufLen_ = 0, bufAt_ = 0;
  uint32_t pos_ = 0;             // where in the zip the next packed bytes are
  uint32_t cleft_ = 0, ctaken_ = 0, uleft_ = 0;
  bool deflated_ = false, done_ = true, bad_ = false;
};

// A whole file of the zip in memory, for the small ones (an EPUB's list of
// chapters): out gets its bytes and a NUL after them (so out.size() is the
// file's size + 1). False: damaged, or more than maxSize bytes.
bool zipReadAll(ZipFile &zip, int index, PsVec<char> &out, size_t maxSize);
