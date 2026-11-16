// The Books library (booklib.h): the list of books, the cache of converted
// ones and the reading places, over stdio / dirent / stat.
//
// The files it writes are text, a line a fact, closed by "END <checksum>":
//
//   library.idx   STICKYLIB 2 <flow version>     (version 1, without the seconds, is still read)
//                 top <the newest "last read" given>
//                 <key> \t <size> \t <date> \t <format> \t <flags> \t <permille> \t <last read> \t <path> \t <title> \t <author> \t <seconds read>
//   <key>.meta    STICKYMETA 1, then "flowver", "flowsize", "format", "cover", "title", "author", "lang", "path",
//                 "desc" (the description; a new line in it is written \n, a backslash \\)
//   <key>.pos     STICKYPOS 1, then "offset", "flow", "permille", "stamp", "finished", a "mark <offset> \t <label>"
//                 a bookmark, and the reading: "secs", "pages", "sessions", "started", "finishedAt", "bytes",
//                 "last", "counted" (each 0 when missing)
//   stats.txt     STICKYSTATS 1, then "finished", "secs", "pages" (the card's totals) and a
//                 "day <local day> <seconds> <pages>" a day read, by day, the last LIB_STATS_DAYS
//
// Lines a file doesn't know are passed over, so a newer firmware's files still load.
//
// Everything read back is checked (the checksum first, then every field), and
// a file that fails is treated as not there: the index is rebuilt from the
// books, a book is converted again, a place starts at the beginning.
#include "booklib.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>

namespace {

typedef PsVec<char> Buf;

// LibEntry::flags. The low three are kept in the index.
enum : uint8_t { F_PROBED = 1, F_ERROR = 2, F_FINISHED = 4, F_SAVED = 7, F_KEYED = 0x40, F_VERIFIED = 0x80 };
enum : uint8_t { SP_IDLE, SP_VERIFY, SP_SWEEP_LIST, SP_SWEEP_DEL };
enum : uint8_t { OP_NONE, OP_CONVERT, OP_OPEN };

const size_t PATH_MAX_BYTES = 700;   // "books/" + two folders + a name, each up to 255
const size_t TITLE_MAX = 200, AUTHOR_MAX = 100, LANG_MAX = 16;
const size_t IDX_MAX_BYTES = 1024 * 1024;  // 500 lines of the longest kind fit; anything bigger isn't ours
const size_t SMALL_MAX_BYTES = 8192;       // a .meta or a .pos
const size_t TRAILER = 13;                 // "END xxxxxxxx\n"
const int INDEX_EVERY = 16;                // probes between two saves of the index during a long scan
const int SWEEP_READ = 64, SWEEP_DELETE = 16;
const size_t SWEEP_NAMES_MAX = 128 * 1024;
const char *const SUFFIX[] = {".flow.toc", ".flow", ".cover", ".meta", ".pos"};
const uint32_t STATS_SAVE_SECS = 5 * 60;   // reading time between two saves of stats.txt
const uint32_t PACE_MIN_SECS = 3 * 60, PACE_MIN_PAGES = 5;
const uint32_t STREAK_SECS = 60;           // a day counts for a streak with this much read
const int32_t DAY_MAX = 1000000;           // year 4707: a day number beyond is nonsense

// ---------------------------------------------------------------- small things

uint32_t fnv32(const char *p, size_t n) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < n; i++) h = (h ^ (uint8_t)p[i]) * 16777619u;
  return h;
}

char lowerAscii(char c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

// Paths are compared as the card does: letters' case doesn't count.
int foldCmp(const char *a, const char *b) {
  for (;; a++, b++) {
    unsigned char x = lowerAscii(*a), y = lowerAscii(*b);
    if (x != y) return x < y ? -1 : 1;
    if (!x) return 0;
  }
}

// The book's key: its path (case folded), size and date, as 64-bit FNV-1a.
uint64_t makeKey(const char *path, uint32_t size, uint32_t mtime) {
  uint64_t h = 14695981039346656037ull;
  auto add = [&h](uint8_t b) { h = (h ^ b) * 1099511628211ull; };
  for (const char *p = path; *p; p++) add(lowerAscii(*p));
  add(0);
  for (int i = 0; i < 4; i++) add(size >> (8 * i));
  for (int i = 0; i < 4; i++) add(mtime >> (8 * i));
  return h;
}

void keyHex(uint64_t key, char out[17]) {
  for (int i = 0; i < 16; i++) out[i] = "0123456789abcdef"[(key >> (60 - 4 * i)) & 15];
  out[16] = 0;
}

bool parseHex(const char *p, int digits, uint64_t &v) {
  v = 0;
  for (int i = 0; i < digits; i++) {
    char c = p[i];
    int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
    if (d < 0) return false;
    v = v << 4 | d;
  }
  return true;
}

// Digits only, and it must fit: no signs, spaces or "1e9".
bool parseU32(const char *p, size_t n, uint32_t &v) {
  if (n < 1 || n > 10) return false;
  uint64_t x = 0;
  for (size_t i = 0; i < n; i++) {
    if (p[i] < '0' || p[i] > '9') return false;
    x = x * 10 + (p[i] - '0');
  }
  if (x > 0xFFFFFFFFull) return false;
  v = (uint32_t)x;
  return true;
}

void put(Buf &b, const char *s, size_t n) { b.insert(b.end(), s, s + n); }
void put(Buf &b, const char *s) { put(b, s, strlen(s)); }
void put(Buf &b, const std::string &s) { put(b, s.data(), s.size()); }
void putNum(Buf &b, uint32_t v) {
  char t[12];
  put(b, t, snprintf(t, sizeof t, "%lu", (unsigned long)v));
}
void putLine(Buf &b, const char *name, uint32_t v) {
  put(b, name);
  put(b, " ");
  putNum(b, v);
  put(b, "\n");
}
void putLine(Buf &b, const char *name, const std::string &v) {
  put(b, name);
  put(b, " ");
  put(b, v);
  put(b, "\n");
}

// Text fit to be a line's field and to be drawn: valid UTF-8 (a bad byte
// becomes '?'), no control characters, spaces single, none at the ends, at
// most maxBytes and cut between two characters.
std::string cleanText(const char *s, size_t n, size_t maxBytes) {
  std::string out;
  bool space = false;
  for (size_t i = 0; i < n;) {
    uint8_t c = s[i];
    size_t len = 1;
    bool ok = true;
    if (c >= 0x80) {
      len = c >= 0xF5 ? 0 : c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC2 ? 2 : 0;
      ok = len && i + len <= n;
      for (size_t k = 1; ok && k < len; k++) ok = ((uint8_t)s[i + k] & 0xC0) == 0x80;
      if (ok && len >= 3) {  // overlong forms, surrogates, beyond U+10FFFF
        uint8_t d = s[i + 1];
        ok = !(c == 0xE0 && d < 0xA0) && !(c == 0xED && d >= 0xA0) && !(c == 0xF0 && d < 0x90) && !(c == 0xF4 && d >= 0x90);
      }
      if (!ok) len = 1;
    } else if (c <= 0x20 || c == 0x7F) {
      space = true;
      i++;
      continue;
    }
    size_t gap = space && out.size() ? 1 : 0;
    if (out.size() + gap + len > maxBytes) break;
    if (gap) out += ' ';
    space = false;
    if (ok) out.append(s + i, len);
    else out += '?';
    i += len;
  }
  return out;
}
std::string cleanText(const std::string &s, size_t maxBytes) { return cleanText(s.data(), s.size(), maxBytes); }

// A description: paragraphs split by one '\n', each cleaned as above.
std::string cleanDesc(const char *s, size_t n) {
  std::string out;
  for (size_t i = 0; i < n;) {
    const char *nl = (const char *)memchr(s + i, '\n', n - i);
    size_t end = nl ? nl - s : n;
    size_t room = BOOK_DESCRIPTION_MAX - out.size() - (out.size() ? 1 : 0);
    if (out.size() + 1 >= BOOK_DESCRIPTION_MAX) break;
    std::string para = cleanText(s + i, end - i, room);
    if (para.size()) {
      if (out.size()) out += '\n';
      out += para;
    }
    i = end + 1;
  }
  return out;
}

uint32_t addSat(uint32_t a, uint32_t b) { return a > 0xFFFFFFFFu - b ? 0xFFFFFFFFu : a + b; }

// A title's next character for sorting: lower case, the common Latin accents
// taken off (é: e, ł: l, ș: s), so "Émile" sorts with the E's. 0 at the end.
uint32_t foldNext(const unsigned char *&p) {
  static const char L1[] = "aaaaaaaceeeeiiiidnooooo*ouuuuy*s";  // U+00C0..DF, and E0..FF but the last
  static const char LA[] =                                      // U+0100..017F
      "aaaaaaccccccccddddeeeeeeeeeegggggggghhhhiiiiiiiiiiiijjkkkllllllllllnnnnnnnnnoooooooorrrrrrssssssssttttttuuuuuuuuuuuuwwyyyzzzzzzs";
  static_assert(sizeof LA == 129, "one letter for each of U+0100..017F");
  uint32_t c = *p;
  if (!c) return 0;
  p++;
  if (c < 0x80) return lowerAscii((char)c);
  int more = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
  c &= more == 3 ? 0x07 : more == 2 ? 0x0F : more == 1 ? 0x1F : 0xFF;
  while (more-- > 0 && (*p & 0xC0) == 0x80) c = c << 6 | (*p++ & 0x3F);
  if (c >= 0xC0 && c <= 0xFF) {
    if (c == 0xFF) return 'y';
    char f = L1[(c - 0xC0) & 31];
    return f != '*' ? (uint32_t)f : c == 0xDE ? 0xFE : c;  // the thorn's lower case; the signs stay
  }
  if (c >= 0x100 && c <= 0x17F) return LA[c - 0x100];
  if (c == 0x218 || c == 0x219) return 's';
  if (c == 0x21A || c == 0x21B) return 't';
  return c;
}

int titleCmp(const char *a, const char *b) {
  const unsigned char *x = (const unsigned char *)a, *y = (const unsigned char *)b;
  for (;;) {
    uint32_t c = foldNext(x), d = foldNext(y);
    if (c != d) return c < d ? -1 : 1;
    if (!c) return 0;
  }
}

bool hiddenName(const char *name) {
  return name[0] == '.' || !foldCmp(name, "System Volume Information") || !foldCmp(name, "__MACOSX") ||
         !foldCmp(name, "$RECYCLE.BIN");
}

bool hasControl(const char *name) {
  for (; *name; name++)
    if ((unsigned char)*name < 0x20 || *name == 0x7F) return true;
  return false;
}

// 1 a file, 2 a folder, 0 anything else. FAT's readdir says; where it
// doesn't (DT_UNKNOWN, a link on a computer), stat does.
int kindOf(const std::string &dir, const struct dirent *e) {
#ifdef DT_DIR
  if (e->d_type == DT_DIR) return 2;
  if (e->d_type == DT_REG) return 1;
#endif
  struct stat st;
  if (stat((dir + "/" + e->d_name).c_str(), &st) != 0) return 0;
  return S_ISDIR(st.st_mode) ? 2 : S_ISREG(st.st_mode) ? 1 : 0;
}

bool isDir(const std::string &path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

// A plain file of ours to remove or rename: never a folder, never a link.
bool isPlainFile(const std::string &path) {
  struct stat st;
#ifdef ESP_PLATFORM
  return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);  // FAT has no links
#else
  return lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
#endif
}

bool fileSize(const std::string &path, uint32_t &size) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
  size = (uint64_t)st.st_size > 0xFFFFFFFFull ? 0xFFFFFFFFu : (uint32_t)st.st_size;
  return true;
}

uint32_t fileStamp(time_t t) {
#ifdef ESP_PLATFORM
  return bookLibLocalStamp(t);
#else
  return (uint32_t)t;
#endif
}

// ---------------------------------------------------------------- checked files

// The whole file if it is whole: it ends with "END " and the checksum of
// everything before. out is that "everything before".
bool readChecked(const std::string &path, size_t maxBytes, Buf &out) {
  out.clear();
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) return false;
  bool ok = false;
  if (fseek(f, 0, SEEK_END) == 0) {
    long n = ftell(f);
    if (n >= (long)TRAILER && (size_t)n <= maxBytes && fseek(f, 0, SEEK_SET) == 0) {
      out.resize((size_t)n);
      ok = fread(out.data(), 1, (size_t)n, f) == (size_t)n;
    }
  }
  fclose(f);
  uint64_t sum = 0;
  if (ok) {
    size_t body = out.size() - TRAILER;
    const char *t = out.data() + body;
    ok = !memcmp(t, "END ", 4) && t[12] == '\n' && parseHex(t + 4, 8, sum) && (uint32_t)sum == fnv32(out.data(), body) &&
         (body == 0 || out[body - 1] == '\n');
    if (ok) out.resize(body);
  }
  if (!ok) out.clear();
  return ok;
}

// As readChecked, minding a write that was cut short: the new file may still
// be waiting beside (name.tmp) with the old one already removed.
bool readRecovering(const std::string &path, size_t maxBytes, Buf &out) {
  std::string tmp = path + ".tmp";
  if (readChecked(path, maxBytes, out)) return true;
  if (readChecked(tmp, maxBytes, out)) {
    remove(path.c_str());  // a torn one, or none
    if (rename(tmp.c_str(), path.c_str()) != 0) remove(tmp.c_str());  // a locked card: what was read still counts
    return true;
  }
  return false;
}

// Closes body with its checksum and puts it in place of path: written beside
// first, so path is always either the old file or the new one (or, for a
// moment, only name.tmp: readRecovering takes that).
bool writeChecked(const std::string &path, Buf &body) {
  char t[16];
  snprintf(t, sizeof t, "END %08lx\n", (unsigned long)fnv32(body.data(), body.size()));
  put(body, t, TRAILER);
  std::string tmp = path + ".tmp";
  FILE *f = fopen(tmp.c_str(), "wb");
  if (!f) return false;
  bool ok = fwrite(body.data(), 1, body.size(), f) == body.size();
  if (fflush(f) != 0) ok = false;
  if (ok && fsync(fileno(f)) != 0) ok = false;
  if (fclose(f) != 0) ok = false;
  if (ok) {
    remove(path.c_str());  // FAT's rename doesn't replace
    ok = rename(tmp.c_str(), path.c_str()) == 0;
  }
  if (!ok) remove(tmp.c_str());
  return ok;
}

// The body's next line (without its '\n'); false at the end.
bool nextLine(const char *&p, const char *end, const char *&line, size_t &len) {
  if (p >= end) return false;
  const char *nl = (const char *)memchr(p, '\n', end - p);
  if (!nl) return false;  // readChecked made sure the body ends with one
  line = p;
  len = nl - p;
  p = nl + 1;
  return true;
}

// "name value": is this line that name, and where its value is.
bool isField(const char *line, size_t len, const char *name, const char *&val, size_t &vlen) {
  size_t n = strlen(name);
  if (len < n + 1 || memcmp(line, name, n) || line[n] != ' ') return false;
  val = line + n + 1;
  vlen = len - n - 1;
  return true;
}

// ---------------------------------------------------------------- .meta and .pos

struct PosHead {
  uint32_t offset = 0, flowSize = 0, stamp = 0;
  uint16_t permille = 0;
  bool finished = false, counted = false;
  LibBookStats stats = {};
};

bool markLess(const LibMark &a, const LibMark &b) { return a.offset < b.offset; }

void setLabel(LibMark &m, const char *label, size_t n) {
  std::string s = cleanText(label, n, LIB_LABEL_MAX);
  memcpy(m.label, s.c_str(), s.size() + 1);
}

// marks may be nullptr: only the head is wanted (the list's line).
bool readPos(const std::string &path, PosHead &h, LibMark *marks, int *nMarks) {
  Buf b;
  if (!readRecovering(path, SMALL_MAX_BYTES, b)) return false;
  const char *p = b.data(), *end = p + b.size(), *line, *v;
  size_t len, vl;
  if (!nextLine(p, end, line, len) || len != 11 || memcmp(line, "STICKYPOS 1", 11)) return false;
  h = PosHead();
  uint32_t x = 0;
  int seen = 0, n = 0;
  while (nextLine(p, end, line, len)) {
    if (isField(line, len, "offset", v, vl)) {
      if (!parseU32(v, vl, h.offset)) return false;
      seen |= 1;
    } else if (isField(line, len, "flow", v, vl)) {
      if (!parseU32(v, vl, h.flowSize)) return false;
      seen |= 2;
    } else if (isField(line, len, "permille", v, vl)) {
      if (!parseU32(v, vl, x) || x > 1000) return false;
      h.permille = (uint16_t)x;
    } else if (isField(line, len, "stamp", v, vl)) {
      if (!parseU32(v, vl, h.stamp)) return false;
    } else if (isField(line, len, "finished", v, vl)) {
      if (!parseU32(v, vl, x) || x > 1) return false;
      h.finished = x == 1;
    } else if (isField(line, len, "mark", v, vl)) {
      const char *tab = (const char *)memchr(v, '\t', vl);
      if (!tab || !parseU32(v, tab - v, x) || n >= LIB_MAX_MARKS || x > h.flowSize) return false;
      if (marks) {
        marks[n].offset = x;
        setLabel(marks[n], tab + 1, v + vl - (tab + 1));
      }
      n++;
    } else if (isField(line, len, "counted", v, vl)) {
      if (!parseU32(v, vl, x) || x > 1) return false;
      h.counted = x == 1;
    } else {
      static const char *const NAME[] = {"secs", "pages", "sessions", "started", "finishedAt", "bytes", "last"};
      uint32_t *const field[] = {&h.stats.secs,       &h.stats.pages, &h.stats.sessions, &h.stats.started,
                                 &h.stats.finishedAt, &h.stats.bytes, &h.stats.lastAt};
      for (int k = 0; k < 7; k++)
        if (isField(line, len, NAME[k], v, vl)) {
          if (!parseU32(v, vl, *field[k])) return false;
          break;
        }
    }
  }
  if (seen != 3 || h.offset > h.flowSize) return false;
  if (!marks && nMarks) *nMarks = n;
  if (marks) {
    std::stable_sort(marks, marks + n, markLess);
    int kept = 0;
    for (int i = 0; i < n; i++)  // one bookmark a place
      if (!kept || marks[i].offset != marks[kept - 1].offset) marks[kept++] = marks[i];
    *nMarks = kept;
  }
  return true;
}

bool writePos(const std::string &path, const LibReadState &s) {
  Buf b;
  put(b, "STICKYPOS 1\n");
  putLine(b, "offset", s.offset);
  putLine(b, "flow", s.flowSize);
  putLine(b, "permille", s.permille);
  putLine(b, "stamp", s.stamp);
  putLine(b, "finished", s.finished ? 1 : 0);
  putLine(b, "secs", s.stats.secs);
  putLine(b, "pages", s.stats.pages);
  putLine(b, "sessions", s.stats.sessions);
  putLine(b, "started", s.stats.started);
  putLine(b, "finishedAt", s.stats.finishedAt);
  putLine(b, "bytes", s.stats.bytes);
  putLine(b, "last", s.stats.lastAt);
  putLine(b, "counted", s.counted ? 1 : 0);
  for (int i = 0; i < s.nMarks; i++) {
    put(b, "mark ");
    putNum(b, s.marks[i].offset);
    put(b, "\t");
    put(b, s.marks[i].label);
    put(b, "\n");
  }
  return writeChecked(path, b);
}

}  // namespace

struct LibMeta {
  uint32_t flowVersion = 0, flowSize = 0;
  BookFormat format = BF_UNKNOWN;
  char cover = 0;
  std::string title, author, lang, path, description;
};

namespace {

bool readMeta(const std::string &path, LibMeta &m) {
  Buf b;
  if (!readRecovering(path, SMALL_MAX_BYTES, b)) return false;
  const char *p = b.data(), *end = p + b.size(), *line, *v;
  size_t len, vl;
  if (!nextLine(p, end, line, len) || len != 12 || memcmp(line, "STICKYMETA 1", 12)) return false;
  m = LibMeta();
  uint32_t x = 0;
  int seen = 0;
  while (nextLine(p, end, line, len)) {
    if (isField(line, len, "flowver", v, vl)) {
      if (!parseU32(v, vl, m.flowVersion)) return false;
      seen |= 1;
    } else if (isField(line, len, "flowsize", v, vl)) {
      if (!parseU32(v, vl, m.flowSize)) return false;
      seen |= 2;
    } else if (isField(line, len, "format", v, vl)) {
      if (!parseU32(v, vl, x) || x == BF_UNKNOWN || x > BF_HTML) return false;
      m.format = (BookFormat)x;
      seen |= 4;
    } else if (isField(line, len, "cover", v, vl)) {
      if (vl != 1 || (v[0] != 'j' && v[0] != 'p' && v[0] != '-')) return false;
      m.cover = v[0] == '-' ? 0 : v[0];
    } else if (isField(line, len, "title", v, vl)) m.title = cleanText(v, vl, TITLE_MAX);
    else if (isField(line, len, "author", v, vl)) m.author = cleanText(v, vl, AUTHOR_MAX);
    else if (isField(line, len, "lang", v, vl)) m.lang = cleanText(v, vl, LANG_MAX);
    else if (isField(line, len, "desc", v, vl)) {
      std::string d;
      for (size_t i = 0; i < vl; i++) {
        if (v[i] == '\\' && i + 1 < vl && (v[i + 1] == 'n' || v[i + 1] == '\\')) d += v[++i] == 'n' ? '\n' : '\\';
        else d += v[i];
      }
      m.description = cleanDesc(d.data(), d.size());
    }
    else if (isField(line, len, "path", v, vl)) {
      if (!vl || vl > PATH_MAX_BYTES) return false;
      m.path.assign(v, vl);
      seen |= 8;
    }
  }
  return seen == 15;
}

bool writeMeta(const std::string &path, const LibMeta &m) {
  Buf b;
  put(b, "STICKYMETA 1\n");
  putLine(b, "flowver", m.flowVersion);
  putLine(b, "flowsize", m.flowSize);
  putLine(b, "format", (uint32_t)m.format);
  putLine(b, "cover", std::string(1, m.cover ? m.cover : '-'));
  putLine(b, "title", m.title);
  putLine(b, "author", m.author);
  putLine(b, "lang", m.lang);
  putLine(b, "path", m.path);
  put(b, "desc ");
  for (char c : m.description) {
    if (c == '\n') put(b, "\\n");
    else if (c == '\\') put(b, "\\\\");
    else b.push_back(c);
  }
  put(b, "\n");
  return writeChecked(path, b);
}

// ---------------------------------------------------------------- the index, read

struct IdxRec {
  const char *path, *title, *author;  // in the file's text, ended there
  uint64_t key;
  uint32_t size, mtime, stamp, secs;
  uint16_t permille;
  uint8_t format, flags;
  bool used;
};

// Every line or nothing: a field that doesn't parse, a book twice, too many
// lines, and the index is built again from the books.
bool loadIndex(const std::string &path, Buf &text, PsVec<IdxRec> &recs, uint32_t &top) {
  recs.clear();
  if (!readRecovering(path, IDX_MAX_BYTES, text)) return false;
  const char *p = text.data(), *end = p + text.size(), *line, *v;
  size_t len, vl;
  char head[32];
  size_t hl = snprintf(head, sizeof head, "STICKYLIB 2 %d", FLOW_VERSION);
  if (!nextLine(p, end, line, len) || len != hl || memcmp(line, "STICKYLIB ", 10) || memcmp(line + 11, head + 11, hl - 11)) return false;
  const int fields = line[10] == '2' ? 11 : line[10] == '1' ? 10 : 0;  // version 1: no seconds read
  if (!fields) return false;
  if (!nextLine(p, end, line, len) || !isField(line, len, "top", v, vl) || !parseU32(v, vl, top)) return false;
  while (nextLine(p, end, line, len)) {
    if (recs.size() >= (size_t)LIB_MAX_BOOKS) return false;
    if (memchr(line, 0, len)) return false;
    char *f[11], *q = (char *)line, *le = q + len;
    size_t fl[11];
    for (int i = 0; i < fields; i++) {
      f[i] = q;
      char *tab = i < fields - 1 ? (char *)memchr(q, '\t', le - q) : le;
      if (!tab) return false;
      fl[i] = tab - q;
      *tab = 0;  // the line's own '\n' for the last
      q = tab + 1;
    }
    if (memchr(f[fields - 1], '\t', fl[fields - 1])) return false;
    IdxRec r = {};
    if (fields == 11 && !parseU32(f[10], fl[10], r.secs)) return false;
    uint32_t fmt, flags, permille;
    if (fl[0] != 16 || !parseHex(f[0], 16, r.key) || !parseU32(f[1], fl[1], r.size) || !parseU32(f[2], fl[2], r.mtime) ||
        !parseU32(f[3], fl[3], fmt) || fmt == BF_UNKNOWN || fmt > BF_HTML || !parseU32(f[4], fl[4], flags) || flags > F_SAVED ||
        !parseU32(f[5], fl[5], permille) || permille > 1000 || !parseU32(f[6], fl[6], r.stamp) || !fl[7] ||
        fl[7] > PATH_MAX_BYTES || !fl[8] || fl[8] > TITLE_MAX || fl[9] > AUTHOR_MAX)
      return false;
    if (r.key != makeKey(f[7], r.size, r.mtime)) return false;  // the line is of one piece
    r.format = (uint8_t)fmt, r.flags = (uint8_t)flags, r.permille = (uint16_t)permille;
    r.path = f[7], r.title = f[8], r.author = f[9];
    recs.push_back(r);
  }
  std::sort(recs.begin(), recs.end(), [](const IdxRec &a, const IdxRec &b) { return foldCmp(a.path, b.path) < 0; });
  for (size_t i = 1; i < recs.size(); i++)
    if (!foldCmp(recs[i - 1].path, recs[i].path)) return false;
  return true;
}

// Is this a name the library gives its files in the cache? "library.idx", or
// 16 hex digits and one of the endings, each maybe with ".tmp" after.
bool cacheName(const char *name, bool &isIndex, uint64_t &key, int &suffix, bool &tmp) {
  size_t n = strlen(name);
  tmp = n > 4 && !strcmp(name + n - 4, ".tmp");
  if (tmp) n -= 4;
  isIndex = n == 11 && !memcmp(name, "library.idx", 11);
  if (isIndex) return true;
  if (n == 9 && !memcmp(name, "stats.txt", 9)) return key = 0, suffix = -1, true;
  if (n < 17 || !parseHex(name, 16, key)) return false;
  for (int i = 0; i < (int)(sizeof SUFFIX / sizeof SUFFIX[0]); i++)
    if (strlen(SUFFIX[i]) == n - 16 && !memcmp(name + 16, SUFFIX[i], n - 16)) return suffix = i, true;
  return false;
}

class RealConverter : public LibConverter {
 public:
  bool begin(const char *book, const char *flow, const char *cover) override { return c_.begin(book, flow, cover); }
  int step() override { return c_.step(); }
  void abort() override { c_.abort(); }
  BookErr error() const override { return c_.error(); }
  const BookMeta &meta() const override { return c_.meta(); }

 private:
  BookConverter c_;
};
LibConverter *makeReal(void *) { return new RealConverter(); }

}  // namespace

std::string bookLibTitleFromName(const char *fileName) {
  std::string s = fileName;
  size_t dot = s.rfind('.');
  if (dot != std::string::npos && dot > 0) s.resize(dot);
  for (char &c : s)
    if (c == '_' || c == '-') c = ' ';
  std::string t = cleanText(s, TITLE_MAX);
  return t.size() ? t : cleanText(fileName, strlen(fileName), TITLE_MAX);  // "---.txt"
}

int32_t bookLibLocalDay(uint32_t now, int32_t utcOffsetSecs) {
  if (!now) return -1;
  int64_t t = (int64_t)now + utcOffsetSecs;
  return t < 0 ? -1 : (int32_t)(t / 86400);
}

uint32_t LibBookStats::paceBytesPerMin() const {
  if (secs < PACE_MIN_SECS || pages < PACE_MIN_PAGES) return 0;
  uint64_t pace = (uint64_t)bytes * 60 / secs;
  return pace > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)pace;
}

uint32_t bookLibLocalStamp(time_t t) {
  struct tm tm;
  if (!localtime_r(&t, &tm)) return (uint32_t)t;
  // Days since 1970 of that calendar day, then the clock's reading.
  long y = tm.tm_year + 1900L, m = tm.tm_mon + 1;
  if (m <= 2) y--;
  long era = (y >= 0 ? y : y - 399) / 400, yoe = y - era * 400;
  long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + tm.tm_mday - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  int64_t days = (int64_t)era * 146097 + doe - 719468;
  return (uint32_t)(days * 86400 + tm.tm_hour * 3600L + tm.tm_min * 60L + tm.tm_sec);
}

// ---------------------------------------------------------------- the library

BookLibrary::BookLibrary(const char *root) : root_(root), make_(makeReal), probe_(bookProbe) {
  while (root_.size() > 1 && root_.back() == '/') root_.pop_back();
  cacheDir_ = root_ + "/.sticky/books";
  memset(&st_, 0, sizeof st_);
  pool_.push_back(0);  // place 0: the empty string
}

BookLibrary::~BookLibrary() {
  closeBook();
  openAbort();
  if (sweepDir_) closedir(sweepDir_);
}

void BookLibrary::setConverterFactory(LibConverterFactory make, void *arg) {
  make_ = make ? make : makeReal;
  makeArg_ = arg;
}
void BookLibrary::setProbe(LibProbe probe) { probe_ = probe ? probe : bookProbe; }

uint32_t BookLibrary::intern(const std::string &s) {
  if (s.empty()) return 0;
  uint32_t at = (uint32_t)pool_.size();
  pool_.insert(pool_.end(), s.c_str(), s.c_str() + s.size() + 1);
  return at;
}

std::string BookLibrary::cachePath(uint64_t key, const char *suffix) const {
  char k[17];
  keyHex(key, k);
  return cacheDir_ + "/" + k + suffix;
}

// FAT makes one folder at a time.
bool BookLibrary::ensureCacheDir() {
  if (isDir(cacheDir_)) return true;
  mkdir((root_ + "/.sticky").c_str(), 0777);
  mkdir(cacheDir_.c_str(), 0777);
  return isDir(cacheDir_);
}

// Something that should have worked didn't: is the card still there? If not,
// everything in flight is dropped and state() says so, until the next scanBegin().
bool BookLibrary::checkCard() {
  DIR *d = opendir(root_.c_str());
  if (d) {
    closedir(d);
    return true;
  }
  state_ = LS_NO_CARD;
  phase_ = SP_IDLE;
  if (sweepDir_) closedir(sweepDir_);
  sweepDir_ = nullptr;
  sweepNames_.clear();
  if (conv_) conv_->abort();
  delete conv_;
  conv_ = nullptr;
  dropOpen();
  gen_++;
  return false;
}

void BookLibrary::clearList() {
  entries_.clear();
  order_.clear();
  pool_.clear();
  pool_.push_back(0);
  sortDirty_ = true;
  truncated_ = listErr_ = indexDirty_ = false;
  top_ = 0;
  cursor_ = 0;
  probes_ = 0;
}

void BookLibrary::eraseEntry(size_t idx) {
  if (openIdx_ == (int)idx) {
    if (conv_) conv_->abort();
    delete conv_;
    conv_ = nullptr;
    dropOpen();
  } else if (openIdx_ > (int)idx) openIdx_--;
  entries_.erase(entries_.begin() + idx);
  if (cursor_ > idx) cursor_--;
  sortDirty_ = indexDirty_ = true;
  gen_++;
}

// ---------------------------------------------------------------- scanning

// The books of one folder, then of the folders in it. One folder is open at
// a time (the card allows few), so the sub-folders' names wait in a list.
void BookLibrary::listDir(const std::string &rel, int depth, int maxDepth) {
  std::string dir = rel.empty() ? root_ : root_ + "/" + rel;
  DIR *d = opendir(dir.c_str());
  if (!d) {
    listErr_ = true;
    return;
  }
  Buf subs;
  for (;;) {
    errno = 0;
    struct dirent *e = readdir(d);
    if (!e) {
      if (errno) listErr_ = true;
      break;
    }
    const char *name = e->d_name;
    size_t n = strlen(name);
    if (!n || n > 255 || hiddenName(name) || hasControl(name)) continue;
    int kind = kindOf(dir, e);
    if (kind == 2) {
      if (depth < maxDepth) put(subs, name, n + 1);
      continue;
    }
    BookFormat format = bookFormatOfName(name);
    if (kind != 1 || format == BF_UNKNOWN) continue;
    std::string path = rel.empty() ? std::string(name) : rel + "/" + name;
    if (path.size() > PATH_MAX_BYTES) continue;
    if (entries_.size() >= (size_t)LIB_MAX_BOOKS) {
      truncated_ = true;
      break;
    }
    LibEntry en = {};
    en.path = intern(path);
    en.format = format;
    entries_.push_back(en);
  }
  closedir(d);
  for (size_t i = 0; i < subs.size() && !truncated_; i += strlen(&subs[i]) + 1) listDir(rel + "/" + &subs[i], depth + 1, maxDepth);
}

// What the index knows of the books just listed, matched by path: their
// titles and how far each was read. Whether a book is still the same file
// (size, date) is checked later, a book a step: that takes a look at the
// card for each, and the list shouldn't wait for it.
void BookLibrary::adoptIndex() {
  Buf text;
  PsVec<IdxRec> recs;
  uint32_t top = 0;
  bool have = loadIndex(cacheDir_ + "/library.idx", text, recs, top);
  if (have) top_ = top;
  else recs.clear();  // what was read before the line that failed counts for nothing
  size_t used = 0;
  for (LibEntry &e : entries_) {
    std::string path = str(e.path);
    IdxRec *r = nullptr;
    size_t lo = 0, hi = recs.size();
    while (lo < hi) {
      size_t mid = (lo + hi) / 2;
      int c = foldCmp(recs[mid].path, path.c_str());
      if (!c) {
        r = &recs[mid];
        break;
      }
      if (c < 0) lo = mid + 1;
      else hi = mid;
    }
    if (r && !r->used) {
      r->used = true;
      used++;
      e.key = r->key, e.size = r->size, e.mtime = r->mtime, e.stamp = r->stamp, e.permille = r->permille, e.secs = r->secs;
      e.format = r->format;
      e.flags = r->flags | F_KEYED;
      e.title = intern(r->title);
      e.author = intern(r->author);
      if (e.stamp > top_) top_ = e.stamp;
    } else {
      size_t slash = path.rfind('/');
      e.title = intern(bookLibTitleFromName(path.c_str() + (slash == std::string::npos ? 0 : slash + 1)));
    }
  }
  // Worth writing again: books came or went. (With no index and no books, nothing is created on the card.)
  indexDirty_ = have ? used != recs.size() || used != entries_.size() : !entries_.empty();
}

bool BookLibrary::scanBegin() {
  closeBook();
  openAbort();
  if (sweepDir_) closedir(sweepDir_);
  sweepDir_ = nullptr;
  sweepNames_.clear();
  phase_ = SP_IDLE;
  clearList();
  days_.clear();  // read again when wanted: it may be another card
  statsLoaded_ = statsDirty_ = false;
  totFinished_ = totSecs_ = totPages_ = statsUnsaved_ = 0;
  gen_++;
  DIR *d = opendir(root_.c_str());
  if (!d) {
    state_ = LS_NO_CARD;
    return false;
  }
  // The books' folder: "books" before "ebooks", whatever the letters' case.
  std::string folder;
  int best = 2;
  for (;;) {
    errno = 0;
    struct dirent *e = readdir(d);
    if (!e) {
      if (errno) listErr_ = true;
      break;
    }
    int rank = !foldCmp(e->d_name, "books") ? 0 : !foldCmp(e->d_name, "ebooks") ? 1 : 2;
    if (rank == 2 || kindOf(root_, e) != 2) continue;
    if (rank < best || (rank == best && folder.compare(e->d_name) > 0)) best = rank, folder = e->d_name;
  }
  closedir(d);
  if (folder.size()) listDir(folder, 0, 2);
  else listDir("", 0, 0);
  if (listErr_ && entries_.empty() && !checkCard()) return false;
  // By path, and each path once (two names that differ by their case are one file to the card).
  std::sort(entries_.begin(), entries_.end(),
            [this](const LibEntry &a, const LibEntry &b) { return foldCmp(str(a.path), str(b.path)) < 0; });
  size_t kept = 0;
  for (size_t i = 0; i < entries_.size(); i++)
    if (!kept || foldCmp(str(entries_[kept - 1].path), str(entries_[i].path))) entries_[kept++] = entries_[i];
  entries_.resize(kept);
  adoptIndex();
  state_ = entries_.size() ? LS_OK : folder.size() ? LS_EMPTY : LS_NO_FOLDER;
  phase_ = SP_VERIFY;
  return state_ == LS_OK;
}

int BookLibrary::scanDone() const { return phase_ == SP_VERIFY ? (int)std::min(cursor_, entries_.size()) : (int)entries_.size(); }
int BookLibrary::scanTotal() const { return (int)entries_.size(); }

// Is the book still the file the index knew? Its size and date say. A new or
// changed one gets its key here and starts as a book never read.
bool BookLibrary::verify(size_t idx, bool &changed) {
  LibEntry &e = entries_[idx];
  changed = false;
  if (e.flags & F_VERIFIED) return true;
  std::string rel = str(e.path);
  struct stat st;
  if (stat((root_ + "/" + rel).c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
  uint32_t size = (uint64_t)st.st_size > 0xFFFFFFFFull ? 0xFFFFFFFFu : (uint32_t)st.st_size, mtime = fileStamp(st.st_mtime);
  uint64_t key = makeKey(rel.c_str(), size, mtime);
  if (!(e.flags & F_KEYED) || key != e.key) {
    size_t slash = rel.rfind('/');
    e.key = key, e.size = size, e.mtime = mtime, e.stamp = 0, e.permille = 0, e.secs = 0, e.author = 0;
    e.flags = 0;
    e.format = (uint8_t)bookFormatOfName(rel.c_str() + (slash == std::string::npos ? 0 : slash + 1));
    e.title = intern(bookLibTitleFromName(rel.c_str() + (slash == std::string::npos ? 0 : slash + 1)));
    changed = true;
    sortDirty_ = indexDirty_ = true;
    gen_++;
  }
  e.flags |= F_KEYED | F_VERIFIED;
  return true;
}

// The book's title and author: from its .meta if it was converted before
// (the index was lost), else by a look into the file. fresh: the index
// didn't know this book, so its place, if the card has one, is read too.
void BookLibrary::probe(LibEntry &e, bool fresh) {
  std::string rel = str(e.path);
  uint8_t before = e.flags;
  if (fresh) {
    PosHead h;
    if (readPos(cachePath(e.key, ".pos"), h, nullptr, nullptr)) {
      e.permille = h.permille, e.stamp = h.stamp, e.secs = h.stats.secs;
      if (h.finished) e.flags |= F_FINISHED;
      if (h.stamp > top_) top_ = h.stamp;
    }
  }
  LibMeta m;
  BookMeta bm;
  BookErr err = BE_OK;
  bool ok = readMeta(cachePath(e.key, ".meta"), m) && !foldCmp(m.path.c_str(), rel.c_str());
  if (ok) bm.title = m.title, bm.author = m.author, bm.format = m.format;
  else ok = probe_((root_ + "/" + rel).c_str(), bm, err);
  e.flags |= F_PROBED;
  if (ok) {
    std::string title = cleanText(bm.title, TITLE_MAX), author = cleanText(bm.author, AUTHOR_MAX);
    if (title.size() && title != str(e.title)) e.title = intern(title);
    if (author != str(e.author)) e.author = intern(author);
    if (bm.format != BF_UNKNOWN && bm.format <= BF_HTML) e.format = bm.format;
    e.flags &= ~F_ERROR;
  } else {
    e.flags |= F_ERROR;
  }
  if (fresh || e.flags != before) sortDirty_ = indexDirty_ = true, gen_++;
}

bool BookLibrary::scanStep() {
  switch (phase_) {
    case SP_VERIFY: {
      if (cursor_ >= entries_.size()) {
        if (indexDirty_) writeIndex();
        // The cache is only tidied when the list is known to be whole: a folder
        // that couldn't be read, or more books than the list holds, and a book
        // that is there would look gone.
        phase_ = listErr_ || truncated_ ? SP_IDLE : SP_SWEEP_LIST;
        return phase_ != SP_IDLE;
      }
      size_t idx = cursor_;
      bool changed = false;
      if (!verify(idx, changed)) {
        if (!checkCard()) return false;
        eraseEntry(idx);  // gone since it was listed
        if (entries_.empty()) state_ = LS_EMPTY;
        return true;
      }
      cursor_++;
      LibEntry &e = entries_[idx];
      if (!(e.flags & F_PROBED) || (e.flags & F_ERROR)) {
        probe(e, changed);
        if (++probes_ % INDEX_EVERY == 0 && indexDirty_) writeIndex();
        if ((e.flags & F_ERROR) && !checkCard()) return false;
      }
      return true;
    }
    case SP_SWEEP_LIST:
      if (!sweepList()) phase_ = sweepNames_.empty() ? SP_IDLE : SP_SWEEP_DEL;
      return phase_ != SP_IDLE;
    case SP_SWEEP_DEL:
      sweepDelete();
      if (sweepPos_ >= sweepNames_.size()) {
        sweepNames_.clear();
        phase_ = SP_IDLE;
      }
      return phase_ != SP_IDLE;
    default:
      return false;
  }
}

// The cache's files that belong to no book of the list are noted (a letter
// for what to do, then the name), some names a call. Only names of the
// library's own making are ever noted: anything else in the folder is left alone.
bool BookLibrary::sweepList() {
  if (!sweepDir_) {
    live_.clear();
    for (const LibEntry &e : entries_)
      if (e.flags & F_KEYED) live_.push_back(e.key);
    std::sort(live_.begin(), live_.end());
    sweepNames_.clear();
    sweepPos_ = 0;
    sweepDir_ = opendir(cacheDir_.c_str());
    if (!sweepDir_) return false;  // nothing was ever kept
  }
  for (int k = 0; k < SWEEP_READ; k++) {
    struct dirent *e = readdir(sweepDir_);
    if (!e || sweepNames_.size() > SWEEP_NAMES_MAX) {
      closedir(sweepDir_);
      sweepDir_ = nullptr;
      live_.clear();
      return false;
    }
#ifdef DT_DIR
    if (e->d_type == DT_DIR) continue;
#endif
    bool isIndex, tmp;
    uint64_t key = 0;
    int suffix = 0;
    if (!cacheName(e->d_name, isIndex, key, suffix, tmp)) continue;
    char act;
    if (isIndex) act = tmp ? 'd' : 0;  // the index is in memory by now; a half-written one is of no use
    else if (suffix < 0) act = tmp ? 'r' : 0;  // stats.txt: a book's key has nothing to do with it
    else if (!std::binary_search(live_.begin(), live_.end(), key)) act = 'd';
    else if (!tmp) act = 0;
    else act = !strcmp(SUFFIX[suffix], ".meta") || !strcmp(SUFFIX[suffix], ".pos") ? 'r' : 'd';
    if (!act) continue;
    sweepNames_.push_back(act);
    put(sweepNames_, e->d_name, strlen(e->d_name) + 1);
  }
  return true;
}

void BookLibrary::sweepDelete() {
  for (int k = 0; k < SWEEP_DELETE && sweepPos_ < sweepNames_.size(); k++) {
    char act = sweepNames_[sweepPos_];
    const char *name = &sweepNames_[sweepPos_ + 1];
    sweepPos_ += strlen(name) + 2;
    std::string path = cacheDir_ + "/" + name;
    if (!isPlainFile(path)) continue;
    if (act == 'r') {  // a write cut short: the new file is taken if it is whole and the old one is gone
      std::string final = path.substr(0, path.size() - 4);
      Buf b;
      if (!isPlainFile(final) && readChecked(path, SMALL_MAX_BYTES, b) && rename(path.c_str(), final.c_str()) == 0) continue;
    }
    remove(path.c_str());
  }
}

bool BookLibrary::writeIndex() {
  if (!ensureCacheDir()) return false;
  Buf b;
  char t[40];
  put(b, t, snprintf(t, sizeof t, "STICKYLIB 2 %d\n", FLOW_VERSION));
  putLine(b, "top", top_);
  for (const LibEntry &e : entries_) {
    if (!(e.flags & F_KEYED) || !(e.flags & F_PROBED)) continue;  // not looked at yet: the next scan will
    keyHex(e.key, t);
    put(b, t, 16);
    const uint32_t nums[] = {e.size, e.mtime, e.format, (uint32_t)(e.flags & F_SAVED), e.permille, e.stamp};
    for (uint32_t v : nums) {
      put(b, "\t");
      putNum(b, v);
    }
    const char *const strs[] = {str(e.path), str(e.title), str(e.author)};
    for (const char *s : strs) {
      put(b, "\t");
      put(b, s);
    }
    put(b, "\t");
    putNum(b, e.secs);
    put(b, "\n");
  }
  if (!writeChecked(cacheDir_ + "/library.idx", b)) return false;
  indexDirty_ = false;
  return true;
}

// ---------------------------------------------------------------- the list

LibStatus BookLibrary::statusOf(const LibEntry &e) const {
  return e.flags & F_FINISHED ? LB_FINISHED : e.stamp ? LB_READING : LB_NEW;
}

void BookLibrary::ensureSorted() {
  if (!sortDirty_ && order_.size() == entries_.size()) return;
  order_.resize(entries_.size());
  for (size_t i = 0; i < order_.size(); i++) order_[i] = (uint16_t)i;
  std::sort(order_.begin(), order_.end(), [this](uint16_t x, uint16_t y) {
    const LibEntry &a = entries_[x], &b = entries_[y];
    LibStatus sa = statusOf(a), sb = statusOf(b);
    if (sa != sb) return sa < sb;
    if (sa != LB_NEW && a.stamp != b.stamp) return a.stamp > b.stamp;
    int c = titleCmp(str(a.title), str(b.title));
    return c ? c < 0 : foldCmp(str(a.path), str(b.path)) < 0;
  });
  sortDirty_ = false;
}

int BookLibrary::count(LibStatus status) const {
  int n = 0;
  for (const LibEntry &e : entries_) n += statusOf(e) == status;
  return n;
}

bool BookLibrary::book(int i, LibBook &out) {
  ensureSorted();
  if (i < 0 || i >= (int)order_.size()) return false;
  const LibEntry &e = entries_[order_[i]];
  out.title = str(e.title);
  out.author = str(e.author);
  out.path = str(e.path);
  if (e.flags & F_KEYED) keyHex(e.key, out.key);
  else out.key[0] = 0;  // the scan hasn't got to it; openBegin() works all the same
  out.format = (BookFormat)e.format;
  out.status = statusOf(e);
  out.permille = e.permille;
  out.lastRead = e.stamp;
  out.probed = e.flags & F_PROBED;
  out.error = e.flags & F_ERROR;
  out.secsRead = e.secs;
  return true;
}

int BookLibrary::find(const char *key16) {
  uint64_t key;
  if (!key16 || strlen(key16) != 16 || !parseHex(key16, 16, key)) return -1;
  ensureSorted();
  for (size_t i = 0; i < order_.size(); i++) {
    const LibEntry &e = entries_[order_[i]];
    if ((e.flags & F_KEYED) && e.key == key) return (int)i;
  }
  return -1;
}

int BookLibrary::lastRead() {
  ensureSorted();
  return order_.size() && statusOf(entries_[order_[0]]) == LB_READING ? 0 : -1;
}

// The converted copy goes; the place stays if asked (a book put aside to
// make room is read on from where it was, once converted again).
void BookLibrary::removeCache(uint64_t key, bool keepPos) {
  remove(cachePath(key, ".meta").c_str());  // first: without it the rest counts for nothing
  remove(cachePath(key, ".meta.tmp").c_str());
  remove(cachePath(key, ".flow").c_str());
  remove(cachePath(key, ".flow.toc").c_str());
  remove(cachePath(key, ".cover").c_str());
  if (keepPos) return;
  remove(cachePath(key, ".pos").c_str());
  remove(cachePath(key, ".pos.tmp").c_str());
}

bool BookLibrary::deleteBook(int i) {
  ensureSorted();
  if (i < 0 || i >= (int)order_.size()) return false;
  size_t idx = order_[i];
  LibEntry e = entries_[idx];
  std::string path = root_ + "/" + str(e.path);
  if (remove(path.c_str()) != 0 && isPlainFile(path)) {  // still there: a locked card
    checkCard();
    return false;
  }
  if (!checkCard()) return false;
  if (e.flags & F_KEYED) removeCache(e.key, false);
  eraseEntry(idx);
  if (entries_.empty() && state_ == LS_OK) state_ = LS_EMPTY;
  writeIndex();
  return true;
}

// ---------------------------------------------------------------- opening a book

bool BookLibrary::isOpen() const { return openPhase_ == OP_OPEN; }

void BookLibrary::dropOpen() {
  openPhase_ = OP_NONE;
  openIdx_ = -1;
  stDirty_ = false;
  flowPath_.clear();
  coverPath_.clear();
  openTitle_.clear();
  openAuthor_.clear();
  coverType_ = 0;
  openKey_[0] = 0;
  openDesc_.clear();
  firstReading_ = false;
  memset(&st_, 0, sizeof st_);
}

bool BookLibrary::startConvert() {
  const LibEntry &e = entries_[openIdx_];
  std::string book = root_ + "/" + str(e.path), flow = cachePath(e.key, ".flow"), cover = cachePath(e.key, ".cover");
  if (!conv_) conv_ = make_(makeArg_);
  if (!conv_) return openErr_ = BE_MEMORY, false;
  if (conv_->begin(book.c_str(), flow.c_str(), cover.c_str())) return true;
  openErr_ = conv_->error() == BE_OK ? BE_DAMAGED : conv_->error();
  return false;
}

// The card is full: the converted copies of other books go, the finished
// ones first, then those not read for the longest, until there is room for
// about `need` bytes. Their places and bookmarks stay. False: nothing to remove.
bool BookLibrary::evict(uint64_t need) {
  PsVec<uint16_t> cand;
  for (size_t i = 0; i < entries_.size(); i++)
    if ((int)i != openIdx_ && (entries_[i].flags & F_KEYED) && statusOf(entries_[i]) != LB_NEW) cand.push_back((uint16_t)i);
  std::sort(cand.begin(), cand.end(), [this](uint16_t x, uint16_t y) {
    const LibEntry &a = entries_[x], &b = entries_[y];
    bool fa = a.flags & F_FINISHED, fb = b.flags & F_FINISHED;
    if (fa != fb) return fa;
    return a.stamp != b.stamp ? a.stamp < b.stamp : x < y;
  });
  uint64_t freed = 0;
  bool any = false;
  for (uint16_t i : cand) {
    if (freed >= need) break;
    uint64_t key = entries_[i].key;
    uint32_t flow = 0, cover = 0;
    bool hasFlow = fileSize(cachePath(key, ".flow"), flow), hasCover = fileSize(cachePath(key, ".cover"), cover);
    if (!hasFlow && !hasCover) continue;
    removeCache(key, true);
    freed += (uint64_t)flow + cover;
    any = true;
  }
  return any;
}

int BookLibrary::openFail(BookErr err) {
  if (conv_) conv_->abort();
  delete conv_;
  conv_ = nullptr;
  if (openIdx_ >= 0 && (err == BE_FORMAT || err == BE_DAMAGED || err == BE_DRM || err == BE_ENCRYPTED || err == BE_NO_TEXT)) {
    LibEntry &e = entries_[openIdx_];  // the book's own fault: the list shows it
    if (!(e.flags & F_ERROR)) e.flags |= F_ERROR | F_PROBED, indexDirty_ = true, gen_++;
  }
  dropOpen();
  if (err == BE_OPEN || err == BE_WRITE) checkCard();
  openErr_ = err;
  return -(int)err;
}

// Was the book converted, by this firmware, and is the copy whole?
bool BookLibrary::metaValid(const LibEntry &e, LibMeta &m, uint32_t &flowSize) {
  flowSize = 0;
  return readMeta(cachePath(e.key, ".meta"), m) && m.flowVersion == (uint32_t)FLOW_VERSION && !foldCmp(m.path.c_str(), str(e.path)) &&
         fileSize(cachePath(e.key, ".flow"), flowSize) && flowSize && flowSize == m.flowSize;
}

bool BookLibrary::openBegin(int i, uint32_t now) {
  closeBook();
  openAbort();
  openErr_ = BE_OK;
  ensureSorted();
  if (state_ == LS_NO_CARD || i < 0 || i >= (int)order_.size()) return openErr_ = BE_OPEN, false;
  size_t idx = order_[i];
  bool changed;
  entries_[idx].flags &= ~F_VERIFIED;  // the file may have changed since the scan
  if (!verify(idx, changed)) return openFail(BE_OPEN), false;
  if (!ensureCacheDir()) return openFail(BE_WRITE), false;
  openIdx_ = (int)idx;
  now_ = now;
  retried_ = false;
  const LibEntry &e = entries_[idx];
  std::string metaPath = cachePath(e.key, ".meta");
  LibMeta m;
  uint32_t size = 0;
  if (metaValid(e, m, size)) {
    finishOpen(m, size);
    return true;
  }
  // Not converted, or by an older firmware, or the copy is damaged: again.
  remove(metaPath.c_str());
  remove((metaPath + ".tmp").c_str());
  bool ok = startConvert();
  if (!ok && openErr_ == BE_WRITE) {
    retried_ = true;
    ok = evict((uint64_t)e.size * 3 + 256 * 1024) && startConvert();
  }
  if (!ok) return openFail(openErr_ == BE_OK ? BE_DAMAGED : openErr_), false;
  openErr_ = BE_OK;
  openPhase_ = OP_CONVERT;
  return true;
}

int BookLibrary::openStep() {
  if (openPhase_ == OP_OPEN) return BOOK_DONE;
  if (openPhase_ != OP_CONVERT || !conv_) return -(int)(openErr_ == BE_OK ? BE_OPEN : openErr_);
  int p = conv_->step();
  if (p >= 0 && p < BOOK_DONE) return p;
  const LibEntry &e = entries_[openIdx_];
  if (p != BOOK_DONE) {
    BookErr err = conv_->error() == BE_OK ? BE_DAMAGED : conv_->error();
    if (err == BE_WRITE && !retried_) {  // no room: make some, and once more from the start
      retried_ = true;
      if (evict((uint64_t)e.size * 3 + 256 * 1024) && startConvert()) return 0;
    }
    return openFail(err);
  }
  LibMeta m;
  const BookMeta &bm = conv_->meta();
  m.flowVersion = FLOW_VERSION;
  m.title = cleanText(bm.title, TITLE_MAX);  // the converter read the whole book: its word is the better one
  m.author = cleanText(bm.author, AUTHOR_MAX);
  if (m.title.empty()) m.title = str(e.title);
  if (m.author.empty()) m.author = str(e.author);
  m.lang = cleanText(bm.language, LANG_MAX);
  m.description = cleanDesc(bm.description.data(), bm.description.size());
  m.format = bm.format != BF_UNKNOWN && bm.format <= BF_HTML ? bm.format : (BookFormat)e.format;
  m.cover = bm.cover == 'j' || bm.cover == 'p' ? bm.cover : 0;
  m.path = str(e.path);
  uint32_t size = 0, coverSize = 0;
  if (m.cover && !(fileSize(cachePath(e.key, ".cover"), coverSize) && coverSize)) m.cover = 0;
  delete conv_;  // done: its files stay
  conv_ = nullptr;
  bool ok = fileSize(cachePath(e.key, ".flow"), size) && size;
  m.flowSize = size;
  if (!ok || !writeMeta(cachePath(e.key, ".meta"), m)) {
    removeCache(e.key, true);
    return openFail(BE_WRITE);
  }
  finishOpen(m, size);
  return BOOK_DONE;
}

void BookLibrary::openAbort() {
  if (openPhase_ != OP_CONVERT) return;
  if (conv_) conv_->abort();
  delete conv_;
  conv_ = nullptr;
  dropOpen();
}

// The flow is there and whole: the book is open. Its place is taken up if
// it was kept for this very flow (the size says), else it starts at the top.
void BookLibrary::finishOpen(const LibMeta &m, uint32_t flowSize) {
  LibEntry &e = entries_[openIdx_];
  flowPath_ = cachePath(e.key, ".flow");
  coverType_ = m.cover;
  coverPath_ = m.cover ? cachePath(e.key, ".cover") : std::string();
  keyHex(e.key, openKey_);
  if (m.title.size() && m.title != str(e.title)) e.title = intern(m.title);
  if (m.author.size() && m.author != str(e.author)) e.author = intern(m.author);
  e.format = m.format;
  e.flags = (e.flags | F_PROBED) & ~F_ERROR;
  openTitle_ = str(e.title);
  openAuthor_ = str(e.author);
  openDesc_ = m.description;
  PosHead h;
  memset(&st_, 0, sizeof st_);
  if (readPos(cachePath(e.key, ".pos"), h, st_.marks, &st_.nMarks)) {
    // The time spent on it holds whatever happened to the copy; a place only in the copy it was kept for.
    st_.stats = h.stats, st_.counted = h.counted, st_.stamp = h.stamp;
    if (h.stamp > top_) top_ = h.stamp;
    if (h.flowSize == flowSize) st_.offset = h.offset, st_.permille = h.permille, st_.finished = h.finished;
    else st_.nMarks = 0;
  } else {
    st_.nMarks = 0;
  }
  firstReading_ = true;
  st_.flowSize = flowSize;
  openPhase_ = OP_OPEN;
  touch(now_);
  stDirty_ = true;
  stateToEntry();
  sync();  // the list knows at once that this is the book being read
}

// ---------------------------------------------------------------- the place

// "Last read" is now, or, when the clock is behind what the card has seen
// (no clock at all: now is 0), one more than the newest given so far.
void BookLibrary::touch(uint32_t now) {
  if (now > top_) st_.stamp = top_ = now;
  else if (st_.stamp != top_ || !top_) st_.stamp = top_ = top_ == 0xFFFFFFFFu ? top_ : top_ + 1;
}

void BookLibrary::stateToEntry() {
  LibEntry &e = entries_[openIdx_];
  uint8_t flags = st_.finished ? e.flags | F_FINISHED : e.flags & ~F_FINISHED;
  if (e.stamp != st_.stamp || e.flags != flags) sortDirty_ = true, gen_++;
  else if (e.permille != st_.permille) gen_++;
  else if (e.secs == st_.stats.secs) return;
  e.stamp = st_.stamp, e.permille = st_.permille, e.flags = flags, e.secs = st_.stats.secs;
  indexDirty_ = true;
}

void BookLibrary::setPosition(uint32_t offset, uint32_t now) {
  if (!isOpen()) return;
  if (offset > st_.flowSize) offset = st_.flowSize;
  uint16_t permille = st_.flowSize ? (uint16_t)((uint64_t)offset * 1000 / st_.flowSize) : 0;
  bool finished = st_.finished && permille >= 900;
  if (offset == st_.offset && finished == st_.finished) return;
  st_.offset = offset, st_.permille = permille, st_.finished = finished;
  touch(now);
  stDirty_ = true;
  stateToEntry();
}

void BookLibrary::setFinished(uint32_t now) {
  if (!isOpen() || st_.finished) return;
  st_.finished = true;
  st_.permille = 1000;
  if (now) st_.stats.finishedAt = now;
  if (!st_.counted) {  // once a book, however many times it's read again
    st_.counted = true;
    loadStats();
    totFinished_++;
    statsDirty_ = true;
  }
  touch(now);
  stDirty_ = true;
  stateToEntry();
}

int BookLibrary::markIn(uint32_t start, uint32_t end) const {
  for (int i = 0; i < st_.nMarks; i++)
    if (st_.marks[i].offset >= start && st_.marks[i].offset < end) return i;
  return -1;
}

bool BookLibrary::addMark(uint32_t offset, const char *label) {
  if (!isOpen()) return false;
  if (offset > st_.flowSize) offset = st_.flowSize;
  if (!label) label = "";
  int at = 0;
  while (at < st_.nMarks && st_.marks[at].offset < offset) at++;
  if (at == st_.nMarks || st_.marks[at].offset != offset) {
    if (st_.nMarks >= LIB_MAX_MARKS) return false;
    memmove(&st_.marks[at + 1], &st_.marks[at], (st_.nMarks - at) * sizeof(LibMark));
    st_.nMarks++;
    st_.marks[at].offset = offset;
  }
  setLabel(st_.marks[at], label, strlen(label));
  stDirty_ = true;
  return true;
}

void BookLibrary::removeMark(int i) {
  if (!isOpen() || i < 0 || i >= st_.nMarks) return;
  memmove(&st_.marks[i], &st_.marks[i + 1], (st_.nMarks - i - 1) * sizeof(LibMark));
  st_.nMarks--;
  stDirty_ = true;
}

int BookLibrary::toggleMark(uint32_t start, uint32_t end, const char *label) {
  int i = markIn(start, end);
  if (i < 0) return addMark(start, label) ? 1 : -1;
  for (; i >= 0; i = markIn(start, end)) removeMark(i);
  return 0;
}

bool BookLibrary::saveState() {
  if (!isOpen()) return false;
  if (!stDirty_) return true;
  if (!writePos(cachePath(entries_[openIdx_].key, ".pos"), st_)) {
    checkCard();
    return false;
  }
  stDirty_ = false;
  if (statsDirty_ && statsUnsaved_ >= STATS_SAVE_SECS) writeStats();
  return true;
}

bool BookLibrary::sync() {
  bool ok = !isOpen() || saveState();
  if (ok && indexDirty_) ok = writeIndex();
  if (ok && statsDirty_) ok = writeStats();
  return ok;
}

void BookLibrary::closeBook() {
  if (!isOpen()) return;
  sync();
  dropOpen();
}

// ---------------------------------------------------------------- statistics

void BookLibrary::addReading(uint32_t seconds, uint32_t pagesForward, uint32_t bytesForward, uint32_t now, int32_t localDay) {
  if (!isOpen() || (!seconds && !pagesForward && !bytesForward)) return;
  if (seconds > 86400) seconds = 86400;  // more than a day since the last call isn't reading
  LibBookStats &b = st_.stats;
  // A new session: the first reading of the book, or after a long pause (by
  // the clock; without one, each time the book is opened).
  bool fresh = now && b.lastAt && now >= b.lastAt ? now - b.lastAt > LIB_SESSION_GAP : firstReading_;
  if (!b.sessions || fresh) b.sessions = addSat(b.sessions, 1);
  firstReading_ = false;
  b.secs = addSat(b.secs, seconds);
  b.pages = addSat(b.pages, pagesForward);
  b.bytes = addSat(b.bytes, bytesForward);
  if (now) {
    b.lastAt = now;
    if (!b.started) b.started = now;
  }
  stDirty_ = true;
  stateToEntry();
  loadStats();
  totSecs_ = addSat(totSecs_, seconds);
  totPages_ = addSat(totPages_, pagesForward);
  statsUnsaved_ = addSat(statsUnsaved_, seconds);
  statsDirty_ = true;
  if (localDay < 0 || localDay > DAY_MAX) return;
  if (days_.size() && localDay <= days_.back().day - LIB_STATS_DAYS) return;  // a clock far behind: too old to keep
  auto it = std::lower_bound(days_.begin(), days_.end(), localDay, [](const LibDay &d, int32_t v) { return d.day < v; });
  if (it == days_.end() || it->day != localDay) it = days_.insert(it, LibDay{localDay, 0, 0});
  it->secs = addSat(it->secs, seconds);
  it->pages = addSat(it->pages, pagesForward);
  int32_t oldest = days_.back().day - LIB_STATS_DAYS;
  size_t drop = 0;
  while (drop < days_.size() && days_[drop].day <= oldest) drop++;
  days_.erase(days_.begin(), days_.begin() + drop);
}

// stats.txt, once a card: torn or missing, the figures start again from nothing
// (libraryStats() still counts what the list knows).
void BookLibrary::loadStats() {
  if (statsLoaded_) return;
  statsLoaded_ = true;
  days_.clear();
  totFinished_ = totSecs_ = totPages_ = 0;
  Buf b;
  if (!readRecovering(cacheDir_ + "/stats.txt", SMALL_MAX_BYTES + LIB_STATS_DAYS * 40, b)) return;
  const char *p = b.data(), *end = p + b.size(), *line, *v;
  size_t len, vl;
  if (!nextLine(p, end, line, len) || len != 13 || memcmp(line, "STICKYSTATS 1", 13)) return;
  uint32_t fin = 0, secs = 0, pages = 0;
  PsVec<LibDay> days;
  while (nextLine(p, end, line, len)) {
    bool ok = true;
    if (isField(line, len, "finished", v, vl)) ok = parseU32(v, vl, fin);
    else if (isField(line, len, "secs", v, vl)) ok = parseU32(v, vl, secs);
    else if (isField(line, len, "pages", v, vl)) ok = parseU32(v, vl, pages);
    else if (isField(line, len, "day", v, vl)) {
      const char *s1 = (const char *)memchr(v, ' ', vl), *s2 = s1 ? (const char *)memchr(s1 + 1, ' ', v + vl - s1 - 1) : nullptr;
      LibDay d = {0, 0, 0};
      uint32_t day = 0;
      ok = s2 && parseU32(v, s1 - v, day) && day <= (uint32_t)DAY_MAX && parseU32(s1 + 1, s2 - s1 - 1, d.secs) &&
           parseU32(s2 + 1, v + vl - s2 - 1, d.pages) && (days.empty() || (int32_t)day > days.back().day) &&
           days.size() < (size_t)LIB_STATS_DAYS;
      d.day = (int32_t)day;
      if (ok) days.push_back(d);
    }
    if (!ok) return;
  }
  totFinished_ = fin, totSecs_ = secs, totPages_ = pages;
  days_.swap(days);
}

bool BookLibrary::writeStats() {
  if (!ensureCacheDir()) return false;
  Buf b;
  put(b, "STICKYSTATS 1\n");
  putLine(b, "finished", totFinished_);
  putLine(b, "secs", totSecs_);
  putLine(b, "pages", totPages_);
  for (const LibDay &d : days_) {
    put(b, "day ");
    putNum(b, (uint32_t)d.day);
    put(b, " ");
    putNum(b, d.secs);
    put(b, " ");
    putNum(b, d.pages);
    put(b, "\n");
  }
  if (!writeChecked(cacheDir_ + "/stats.txt", b)) {
    checkCard();
    return false;
  }
  statsDirty_ = false;
  statsUnsaved_ = 0;
  return true;
}

LibStats BookLibrary::libraryStats(int32_t today) {
  loadStats();
  LibStats s;
  memset(&s, 0, sizeof s);
  uint32_t listSecs = 0, finished = 0;
  for (const LibEntry &e : entries_) {
    listSecs = addSat(listSecs, e.secs);
    LibStatus st = statusOf(e);
    finished += st == LB_FINISHED;
    s.booksReading += st == LB_READING;
  }
  // The card's own figures, or what the list knows if that's more (stats.txt lost).
  s.booksFinished = std::max(totFinished_, finished);
  s.totalSecs = std::max(totSecs_, listSecs);
  s.totalPages = totPages_;
  uint32_t run = 0;
  int32_t prev = -2;
  for (const LibDay &d : days_) {
    if (d.secs < STREAK_SECS) continue;
    s.daysRead++;
    run = d.day == prev + 1 ? run + 1 : 1;
    prev = d.day;
    s.longestStreakDays = std::max(s.longestStreakDays, run);
  }
  if (today < 0) return s;
  auto secsOn = [this](int32_t day) -> uint32_t {
    auto it = std::lower_bound(days_.begin(), days_.end(), day, [](const LibDay &d, int32_t v) { return d.day < v; });
    return it != days_.end() && it->day == day ? it->secs : 0;
  };
  for (int k = 0; k < 7; k++) {
    s.last7[6 - k] = secsOn(today - k);
    s.secsThisWeek = addSat(s.secsThisWeek, s.last7[6 - k]);
  }
  s.secsToday = s.last7[6];
  int32_t day = s.secsToday >= STREAK_SECS ? today : today - 1;  // today not read yet: the streak isn't broken
  while (day >= 0 && secsOn(day) >= STREAK_SECS) s.currentStreakDays++, day--;
  return s;
}

bool BookLibrary::bookInfo(int i, LibInfo &out) {
  out = LibInfo();
  ensureSorted();
  if (state_ == LS_NO_CARD || i < 0 || i >= (int)order_.size()) return false;
  size_t idx = order_[i];
  bool changed;
  entries_[idx].flags &= ~F_VERIFIED;  // looked at again: the page may be shown long after the scan
  if (!verify(idx, changed)) {
    if (checkCard()) eraseEntry(idx);  // gone from the card
    return false;
  }
  const LibEntry &e = entries_[idx];
  out.title = str(e.title);
  out.author = str(e.author);
  out.path = str(e.path);
  keyHex(e.key, out.key);
  out.format = (BookFormat)e.format;
  out.status = statusOf(e);
  out.error = e.flags & F_ERROR;
  out.fileSize = e.size;
  out.permille = e.permille;
  out.finished = e.flags & F_FINISHED;
  out.stats.secs = e.secs;
  if ((int)idx == openIdx_ && isOpen()) {  // the open book: what's in memory is newer than the card
    out.converted = true;
    out.flowPath = flowPath_, out.coverPath = coverPath_, out.coverType = coverType_;
    out.description = openDesc_;
    out.position = st_.offset, out.flowSize = st_.flowSize, out.permille = st_.permille, out.finished = st_.finished;
    out.marks = st_.nMarks;
    out.stats = st_.stats;
    return true;
  }
  LibMeta m;
  uint32_t flowSize = 0;
  out.converted = metaValid(e, m, flowSize);
  if (out.converted) {
    out.flowPath = cachePath(e.key, ".flow");
    out.coverType = m.cover;
    if (m.cover) out.coverPath = cachePath(e.key, ".cover");
    out.description = m.description;
    out.flowSize = flowSize;
  } else {
    BookMeta bm;
    BookErr err = BE_OK;
    if (probe_((root_ + "/" + out.path).c_str(), bm, err)) out.description = cleanDesc(bm.description.data(), bm.description.size());
  }
  PosHead h;
  int marks = 0;
  if (readPos(cachePath(e.key, ".pos"), h, nullptr, &marks)) {
    out.stats = h.stats;
    out.finished = h.finished;
    out.permille = h.permille;
    if (!out.converted || h.flowSize == flowSize) {  // as openBegin() would take it up
      out.position = h.offset, out.marks = marks;
      if (!out.converted) out.flowSize = h.flowSize;
    } else {
      out.permille = 0, out.finished = false;
    }
  }
  return true;
}
