#include "bookzip.h"

#include <ctype.h>
#include <string.h>

#include <algorithm>

namespace {

const size_t ZIP_IN = 4096;             // packed bytes read from the card at a time
const size_t ZIP_NAMES_MAX = 512 * 1024;

uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

bool readAt(FILE *f, uint32_t at, void *buf, size_t n) { return fseek(f, (long)at, SEEK_SET) == 0 && fread(buf, 1, n, f) == n; }

int cmpNoCase(const char *a, const char *b) {
  for (;; a++, b++) {
    int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
    if (d || !*a) return d;
  }
}

}  // namespace

void ZipFile::close() {
  if (f_) fclose(f_);
  f_ = nullptr;
  PsVec<Entry>().swap(entries_);
  PsVec<char>().swap(names_);
  PsVec<uint16_t>().swap(sorted_);
}

bool ZipFile::open(const char *path, bool *notZip) {
  close();
  if (notZip) *notZip = false;
  f_ = fopen(path, "rb");
  if (!f_) return false;
  uint8_t head[4];
  if (fread(head, 1, 4, f_) != 4 || memcmp(head, "PK", 2)) {
    if (notZip) *notZip = true;
    return close(), false;
  }
  if (fseek(f_, 0, SEEK_END) != 0) return close(), false;
  long size = ftell(f_);
  if (size < 22 || (unsigned long)size > 0xFFFFFFFFul) return close(), false;
  fileSize_ = (uint32_t)size;

  // The end record: in the last 22 bytes, or before a comment of up to 64 KB.
  // The file's tail is looked at 4 KB first, then all of it that can matter.
  uint32_t dirAt = 0, dirSize = 0, total = 0;
  bool found = false;
  for (int pass = 0; pass < 2 && !found; pass++) {
    uint32_t span = pass ? 65557 : 4096;
    if (span > fileSize_) span = fileSize_;
    uint8_t *tail = (uint8_t *)psMalloc(span);
    if (!tail) return close(), false;
    if (readAt(f_, fileSize_ - span, tail, span)) {
      for (uint32_t i = span - 22 + 1; i-- > 0 && !found;) {
        if (memcmp(tail + i, "PK\5\6", 4)) continue;
        total = le16(tail + i + 10), dirSize = le32(tail + i + 12), dirAt = le32(tail + i + 16);
        // The real one says where a directory fits before it.
        found = (uint64_t)dirAt + dirSize <= (uint64_t)fileSize_ - span + i;
      }
    }
    free(tail);
    if (span == fileSize_) break;
  }
  if (!found) return close(), false;

  // The directory, a block at a time: an entry may lie across two.
  size_t cap = 16384;
  uint8_t *buf = (uint8_t *)psMalloc(cap);
  if (!buf) return close(), false;
  size_t have = 0, at = 0;
  uint32_t left = dirSize;
  bool ok = fseek(f_, (long)dirAt, SEEK_SET) == 0;
  while (ok && (int)entries_.size() < ZIP_MAX_ENTRIES && (total == 0xFFFF || entries_.size() < total)) {
    if (have - at < 46 || have - at < 46u + le16(buf + at + 28)) {
      // Not a whole entry in the buffer: move what's there to its front and read on.
      memmove(buf, buf + at, have - at);
      have -= at, at = 0;
      size_t want = cap - have < left ? cap - have : left;
      if (!want) break;
      size_t got = fread(buf + have, 1, want, f_);
      have += got, left -= (uint32_t)got;
      if (got != want) ok = false;
      if (have < 46 || have < 46u + le16(buf + 28)) break;
    }
    const uint8_t *e = buf + at;
    if (memcmp(e, "PK\1\2", 4)) break;
    size_t nameLen = le16(e + 28), skip = (size_t)le16(e + 30) + le16(e + 32);
    Entry en;
    en.encrypted = le16(e + 8) & 1;
    en.method = le16(e + 10);
    en.csize = le32(e + 20), en.usize = le32(e + 24), en.header = le32(e + 42);
    en.name = (uint32_t)names_.size();
    if (names_.size() + nameLen + 1 > ZIP_NAMES_MAX) break;
    // A name with a NUL in it would read as a shorter one: it ends there.
    for (size_t i = 0; i < nameLen && e[46 + i]; i++) names_.push_back((char)e[46 + i]);
    names_.push_back(0);
    entries_.push_back(en);
    at += 46 + nameLen;
    // The extra field and the comment: skipped, in the buffer or in the file.
    if (skip <= have - at) {
      at += skip;
    } else {
      size_t beyond = skip - (have - at);
      if (beyond > left || fseek(f_, (long)beyond, SEEK_CUR) != 0) break;
      left -= (uint32_t)beyond;
      have = at = 0;
    }
  }
  free(buf);
  if (entries_.empty()) return close(), false;
  sorted_.resize(entries_.size());
  for (size_t i = 0; i < sorted_.size(); i++) sorted_[i] = (uint16_t)i;
  const char *names = names_.data();
  const Entry *ents = entries_.data();
  std::sort(sorted_.begin(), sorted_.end(), [names, ents](uint16_t a, uint16_t b) {
    int c = strcmp(names + ents[a].name, names + ents[b].name);
    return c ? c < 0 : a < b;
  });
  return true;
}

int ZipFile::find(const char *nm) const {
  size_t lo = 0, hi = sorted_.size();
  while (lo < hi) {
    size_t mid = (lo + hi) / 2;
    int c = strcmp(name(sorted_[mid]), nm);
    if (!c) {
      while (mid > 0 && !strcmp(name(sorted_[mid - 1]), nm)) mid--;  // the first of twins
      return sorted_[mid];
    }
    if (c < 0) lo = mid + 1;
    else hi = mid;
  }
  for (size_t i = 0; i < entries_.size(); i++)
    if (!cmpNoCase(name((int)i), nm)) return (int)i;
  return -1;
}

bool ZipReader::open(ZipFile &zip, int index) {
  done_ = true, bad_ = true;
  zip_ = &zip;
  if (!zip.f_ || index < 0 || index >= zip.count()) return false;
  const ZipFile::Entry &e = zip.entries_[index];
  if (e.encrypted || (e.method != 0 && e.method != 8)) return false;
  uint8_t h[30];
  if ((uint64_t)e.header + 30 > zip.fileSize_ || !readAt(zip.f_, e.header, h, 30) || memcmp(h, "PK\3\4", 4)) return false;
  uint64_t data = (uint64_t)e.header + 30 + le16(h + 26) + le16(h + 28);
  if (data > zip.fileSize_) return false;
  pos_ = (uint32_t)data;
  // The sizes are the directory's (the header's own may be zero, written after the data).
  cleft_ = e.csize;
  if (cleft_ > zip.fileSize_ - pos_) cleft_ = zip.fileSize_ - pos_;  // cut short: read() finds out
  deflated_ = e.method == 8;
  // Deflate can't make more than 1032 bytes of one: a size beyond that is a lie.
  uint64_t most = deflated_ ? (uint64_t)cleft_ * 1032 + 1024 : cleft_;
  uleft_ = e.usize < most ? e.usize : (uint32_t)(most > 0xFFFFFFFFull ? 0xFFFFFFFFull : most);
  ctaken_ = 0;
  bufLen_ = bufAt_ = 0;
  if (deflated_) {
    if (!buf_) buf_ = (uint8_t *)psMalloc(ZIP_IN);
    if (!buf_ || !inf_.begin(false)) return false;
  }
  done_ = false, bad_ = false;
  return true;
}

long ZipReader::read(uint8_t *out, size_t cap) {
  if (bad_) return -1;
  if (done_ || !cap) return 0;
  if (cap > uleft_) cap = uleft_;
  if (!cap) return done_ = true, 0;
  FILE *f = zip_->f_;
  if (!deflated_) {
    size_t n = cap < cleft_ ? cap : cleft_;
    if (!n) return done_ = true, 0;
    if (!readAt(f, pos_, out, n)) return bad_ = true, -1;
    pos_ += (uint32_t)n, cleft_ -= (uint32_t)n, ctaken_ += (uint32_t)n, uleft_ -= (uint32_t)n;
    return (long)n;
  }
  size_t made = 0;
  while (made < cap) {
    if (bufAt_ == bufLen_ && cleft_) {
      size_t n = cleft_ < ZIP_IN ? cleft_ : ZIP_IN;
      if (!readAt(f, pos_, buf_, n)) return bad_ = true, -1;
      pos_ += (uint32_t)n, cleft_ -= (uint32_t)n, ctaken_ += (uint32_t)n;
      bufLen_ = n, bufAt_ = 0;
    }
    size_t used = 0, got = 0;
    Inflater::Result r = inf_.run(buf_ + bufAt_, bufLen_ - bufAt_, used, out + made, cap - made, got);
    bufAt_ += used, made += got;
    if (r == Inflater::DONE) {
      done_ = true;
      break;
    }
    if (r == Inflater::BAD) return bad_ = true, -1;
    if (r == Inflater::MORE_INPUT && bufAt_ == bufLen_ && !cleft_) return bad_ = true, -1;  // the packed bytes ran out
  }
  uleft_ -= (uint32_t)made;
  if (!uleft_) done_ = true;
  return (long)made;
}

bool zipReadAll(ZipFile &zip, int index, PsVec<char> &out, size_t maxSize) {
  out.clear();
  ZipReader r;
  if (!r.open(zip, index)) return false;
  size_t size = zip.size(index);  // read() gives no more than this
  if (size > maxSize) return false;
  out.resize(size + 1);
  size_t len = 0;
  while (len < size) {
    long n = r.read((uint8_t *)out.data() + len, size - len);
    if (n < 0) return out.clear(), false;
    if (n == 0) break;
    len += (size_t)n;
  }
  out.resize(len + 1);
  out[len] = 0;
  return true;
}
