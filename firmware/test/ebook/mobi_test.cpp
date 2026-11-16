// The MOBI reader (bookmobi.h): PalmDOC LZ77, the trailing entries of text
// records, EXTH, and whole books made by tools/make_mobi_fixtures.py
// (uncompressed, LZ77, HUFF/CDIC, PalmDOC text, KF8, MOBI 7 + KF8, DRM):
// their text byte for byte, the contents, the covers, and cut and damaged
// copies of each. With BOOKS_DIR set, every .mobi/.azw/.azw3/.prc in it too.
//
//   ./run.sh mobi
//   mobi <fixtures> --raw book.mobi out.html   (the book's text as stored, for comparing)
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <algorithm>
#include <string>
#include <vector>

#include "bookformats.h"
#include "bookmobi.h"
#include "flowcheck.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

typedef std::string Bytes;
static Bytes B(const char *s, size_t n) { return Bytes(s, n); }

static Bytes unpack(const Bytes &in, size_t cap = 4096) {
  std::vector<uint8_t> out(cap + 1, 0xEE);
  size_t n = mobi::palmdocUnpack((const uint8_t *)in.data(), in.size(), out.data(), cap);
  if (out[cap] != 0xEE) printf("FAIL wrote past the end\n"), fails++;
  return Bytes((const char *)out.data(), n);
}

static void testLz77() {
  CHECK(unpack("abc") == "abc");
  CHECK(unpack(B("\0\x09\x7f", 3)) == B("\0\x09\x7f", 3));       // bytes that stand for themselves
  CHECK(unpack(B("\x03\x80\xff\x01x", 5)) == B("\x80\xff\x01x", 4)); // 1..8: that many as they are
  CHECK(unpack("\xE8\xE5") == " h e");                              // 0xC0..: a space and a character
  // A copy: 0x80 | distance << 3 | (length - 3), in two bytes.
  CHECK(unpack(B("abcd\x80\x20", 6)) == "abcdabc");                 // 4 back, 3 long
  CHECK(unpack(B("ab\x80\x17", 4)) == "abababababab");            // 2 back, 10 long: runs into itself
  CHECK(unpack(B("a\x80\x0d", 3)) == "aaaaaaaaa");                  // 1 back, 8 long
  // Broken: a copy from before the start, a pair cut short, more than fits.
  CHECK(unpack(B("a\x80\x50x", 4)) == "ax");
  CHECK(unpack(B("ab\x80", 3)) == "ab");
  CHECK(unpack(B("abcdef", 6), 4) == "abcd");
  CHECK(unpack(B("a\x80\x0f", 3), 5) == "aaaaa");
  CHECK(unpack(B("\x08xy", 3)) == "xy");
  CHECK(unpack(B("\xE8", 1), 1) == " ");
  // Random input: never out of bounds (UBSan and the guard byte say).
  srand(7);
  for (int t = 0; t < 2000; t++) {
    Bytes r;
    for (int i = rand() % 300; i > 0; i--) r += (char)(rand() & 0xFF);
    unpack(r, 1 + rand() % 600);
  }
}

static bool size(const Bytes &rec, uint16_t flags, size_t &len) { return mobi::textSize((const uint8_t *)rec.data(), rec.size(), flags, len); }

static void testTrailers() {
  size_t n = 0;
  CHECK(size("hello", 0, n) && n == 5);
  // Bit 0: the end of a cut character, then a byte counting it (low two bits).
  CHECK(size(B("text\xA9\x01", 6), 1, n) && n == 4);
  CHECK(size(B("text\x00", 5), 1, n) && n == 4);
  CHECK(size(B("text\x80\x80\xFE", 7), 1, n) && n == 4);  // 0xFE & 3 = 2
  // Bit 1: an entry ending in its own size, read backwards (here 3: two bytes and 0x83).
  CHECK(size(B("text\x11\x22\x83", 7), 2, n) && n == 4);
  // Both: the multibyte bytes come first in the record, then bit 1's entry.
  CHECK(size(B("text\xA9\x01\x11\x82", 8), 3, n) && n == 4);
  // Bits 1 and 2: the higher one is last.
  CHECK(size(B("text\x55\x82\x55\x55\x83", 9), 6, n) && n == 4);
  // A size of more than seven bits: 200 = 0x81 0x48.
  Bytes big = "text" + Bytes(198, 'z') + "\x81\x48";
  CHECK(size(big, 2, n) && n == 4);
  // Bad: sizes past the record, zero, an empty record.
  CHECK(!size(B("ab\x85", 3), 2, n));
  CHECK(!size(B("ab\x80", 3), 2, n));
  CHECK(!size(B("", 0), 1, n));
  CHECK(!size(B("\x03", 1), 1, n));
}

static Bytes exthRec(uint32_t type, const Bytes &v) {
  uint32_t len = 8 + (uint32_t)v.size();
  char h[8] = {(char)(type >> 24), (char)(type >> 16), (char)(type >> 8), (char)type, (char)(len >> 24), (char)(len >> 16), (char)(len >> 8), (char)len};
  return Bytes(h, 8) + v;
}

static Bytes exthBlock(const Bytes &recs, uint32_t count) {
  uint32_t len = 12 + (uint32_t)recs.size();
  char h[12] = {'E', 'X', 'T', 'H', (char)(len >> 24), (char)(len >> 16), (char)(len >> 8), (char)len, (char)(count >> 24), (char)(count >> 16), (char)(count >> 8), (char)count};
  return Bytes(h, 12) + recs;
}

static void testExth() {
  Bytes recs = exthRec(100, "First Author") + exthRec(100, "Second") + exthRec(503, "Caf\xC3\xA9") + exthRec(524, "fr") + exthRec(201, B("\0\0\0\x05", 4)) +
               exthRec(202, B("\0\0\0\x06", 4)) + exthRec(121, B("\0\0\1\0", 4)) + exthRec(999, "ignored");
  Bytes all = exthBlock(recs, 8);
  mobi::Exth x;
  CHECK(mobi::parseExth((const uint8_t *)all.data(), all.size(), true, x));
  CHECK(x.author == "First Author" && x.title == "Caf\xC3\xA9" && x.language == "fr");
  CHECK(x.cover == 5 && x.thumb == 6 && x.kf8 == 256);
  // windows-1252 names become UTF-8; broken UTF-8 is made safe.
  mobi::Exth y;
  Bytes l1 = exthBlock(exthRec(503, "Caf\xE9 \x93q\x94") + exthRec(100, "bad \xC3 x\x01"), 2);
  CHECK(mobi::parseExth((const uint8_t *)l1.data(), l1.size(), false, y) && y.title == "Caf\xC3\xA9 \xE2\x80\x9Cq\xE2\x80\x9D");
  CHECK(mobi::parseExth((const uint8_t *)l1.data(), l1.size(), true, y) && y.author == "bad ? x ");
  // Cut, or a record claiming more than there is, or shorter than its own header: what came before stays.
  mobi::Exth z;
  Bytes cut = all.substr(0, all.size() - 3);
  CHECK(!mobi::parseExth((const uint8_t *)cut.data(), cut.size(), true, z) && z.author == "First Author");
  Bytes liar = exthBlock(exthRec(100, "A") + Bytes("\0\0\0\x64\xff\xff\xff\xff", 8), 2);
  CHECK(!mobi::parseExth((const uint8_t *)liar.data(), liar.size(), true, z));
  Bytes tiny = exthBlock(Bytes("\0\0\0\x64\0\0\0\x04", 8), 1);
  CHECK(!mobi::parseExth((const uint8_t *)tiny.data(), tiny.size(), true, z));
  CHECK(!mobi::parseExth((const uint8_t *)"EXTX", 4, true, z));
}

// ---------------------------------------------------------------- books

static std::string F;  // the fixtures folder
static const std::string OUT = "build/mobi_test";

static Bytes slurp(const std::string &p) { return flowSlurp(p); }
static void spit(const std::string &p, const Bytes &b) {
  FILE *f = fopen(p.c_str(), "wb");
  if (f) fwrite(b.data(), 1, b.size(), f), fclose(f);
}

// The book's text as stored, through the reader's own unpacking; err on failure.
static Bytes rawText(const std::string &path, BookErr &err) {
  mobi::Book b;
  Bytes out;
  err = b.open(path.c_str());
  if (err == BE_OK) err = b.openText();
  for (uint32_t k = 0; err == BE_OK && k < b.huffRecords(); k++) err = b.loadHuff(k);
  for (uint32_t i = 0; err == BE_OK && i < b.part.textRecs; i++) {
    const uint8_t *p;
    size_t n;
    err = b.readText(i, p, n);
    if (err == BE_OK) out.append((const char *)p, n);
  }
  return out;
}

struct Converted {
  int result = 0, steps = 0;
  BookMeta meta;
  Bytes flow, toc, cover;
  double longest = 0;
};

static double now() {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}

static Converted convert(const std::string &book) {
  Converted c;
  std::string flow = OUT + ".flow", cover = OUT + ".cover";
  remove(cover.c_str());
  BookConverter conv;
  if (!conv.begin(book.c_str(), flow.c_str(), cover.c_str())) return c.result = -(int)conv.error(), c;
  for (;;) {
    double t = now();
    c.result = conv.step();
    t = now() - t;
    if (t > c.longest) c.longest = t;
    if (++c.steps > 200000) return printf("FAIL %s: no end\n", book.c_str()), fails++, c;
    if (c.result < 0 || c.result == BOOK_DONE) break;
  }
  c.meta = conv.meta();
  if (c.result == BOOK_DONE) c.flow = slurp(flow), c.toc = slurp(flow + ".toc"), c.cover = slurp(cover);
  return c;
}

static void checkFlow(const std::string &name, const Converted &c) {
  std::string why;
  if (!flowValidate(c.flow, &why)) printf("FAIL %s: flow: %s\n", name.c_str(), why.c_str()), fails++;
  if (!tocValidate(c.toc, c.flow, &why)) printf("FAIL %s: toc: %s\n", name.c_str(), why.c_str()), fails++;
}

// The text of the block at off, without its prefix bytes.
static std::string blockAt(const Bytes &flow, uint32_t off, bool *page = nullptr) {
  if (off >= flow.size()) return "";
  size_t end = flow.find('\n', off);
  std::string b = flow.substr(off, end - off);
  if (page) *page = b.size() && (uint8_t)b[0] == FL_PAGE;
  size_t i = 0;
  while (i < b.size() && (uint8_t)b[i] < 0x20) i++;
  return b.substr(i);
}

static void testFixture(const char *name, const char *html, const char *title, const char *author, const char *lang, char cover,
                        std::vector<std::string> chapters) {
  std::string path = F + "/" + name;
  BookErr err;
  Bytes raw = rawText(path, err), want = slurp(F + "/" + html);
  CHECK(err == BE_OK);
  if (raw != want) printf("FAIL %s: text differs from %s (%zu bytes, want %zu)\n", name, html, raw.size(), want.size()), fails++;
  BookMeta m;
  CHECK(bookProbe(path.c_str(), m, err) && m.format == BF_MOBI);
  if (m.title != title || m.author != author || m.language != lang)
    printf("FAIL %s: probe \"%s\" \"%s\" \"%s\"\n", name, m.title.c_str(), m.author.c_str(), m.language.c_str()), fails++;
  Converted c = convert(path);
  CHECK(c.result == BOOK_DONE);
  checkFlow(name, c);
  CHECK(c.meta.title == title && c.meta.cover == cover);
  CHECK(cover ? c.cover.size() > 16 : c.cover.empty());
  // Every chapter: in the contents, pointing at its heading, which starts a page.
  auto toc = tocParse(c.toc);
  if (toc.size() != chapters.size()) printf("FAIL %s: %zu contents entries, want %zu\n", name, toc.size(), chapters.size()), fails++;
  for (size_t i = 0; i < toc.size() && i < chapters.size(); i++) {
    bool page;
    std::string b = blockAt(c.flow, toc[i].offset, &page);
    if (toc[i].title != chapters[i] || b.find(chapters[i]) == std::string::npos || (!page && toc[i].offset))
      printf("FAIL %s: entry %zu \"%s\" -> \"%s\"\n", name, i, toc[i].title.c_str(), b.c_str()), fails++;
  }
  CHECK(c.flow.find(FL_ITALIC_ON) != std::string::npos || !strcmp(name, "mobi-palmdoc.prc"));
}

static void testFixtures() {
  testFixture("mobi-plain-1252.mobi", "mobi-plain-1252.html", "Caf\xC3\xA9 Stories", "Ann \xC3\x89" "crivain", "fr", 'j',
              {"The First Chapter", "Caf\xC3\xA9 & Cr\xC3\xA8me", "The \xE2\x80\x9CLast\xE2\x80\x9D One"});
  testFixture("mobi-lz77-utf8.mobi", "mobi-lz77-utf8.html", "Made Up Two", "Bo Writer", "en", 'p',
              {"Chapter One", "Chapter \xD0\x94\xD0\xB2\xD0\xB0", "Chapter Three", "Chapter Four", "Chapter Five"});
  testFixture("mobi-huff.mobi", "mobi-huff.html", "Huffman Book", "Cy Coder", "fr", 0, {"Opening", "Middle", "Ending"});
  testFixture("mobi-palmdoc.prc", "mobi-palmdoc.html", "Plain Palm Text", "", "", 0, {});

  // EXTH 103, made plain: HTML in windows-1252, and UTF-8 text whose line end parts paragraphs.
  BookMeta dm;
  BookErr derr;
  CHECK(bookProbe((F + "/mobi-plain-1252.mobi").c_str(), dm, derr) && dm.description == "Une histoire caf\xC3\xA9 & cr\xC3\xA8me.\nSecond \xC3\xA9t\xC3\xA9.");
  CHECK(bookProbe((F + "/mobi-lz77-utf8.mobi").c_str(), dm, derr) && dm.description == "First line \xD0\xB6\xD0\xB8\xD0\xB7\xD0\xBD\xD1\x8C.\nSecond line.");
  CHECK(bookProbe((F + "/mobi-huff.mobi").c_str(), dm, derr) && dm.description.empty());
  CHECK(convert(F + "/mobi-lz77-utf8.mobi").meta.description == "First line \xD0\xB6\xD0\xB8\xD0\xB7\xD0\xBD\xD1\x8C.\nSecond line.");
  Converted withDesc = convert(F + "/mobi-plain-1252.mobi");
  CHECK(withDesc.meta.description == "Une histoire caf\xC3\xA9 & cr\xC3\xA8me.\nSecond \xC3\xA9t\xC3\xA9.");

  // The cover is the record as it is: the thumbnail, here, as EXTH 201 names a GIF.
  Converted plain = convert(F + "/mobi-plain-1252.mobi");
  CHECK(plain.cover.size() == 306 && (uint8_t)plain.cover[0] == 0xFF && (uint8_t)plain.cover[1] == 0xD8);

  // PalmDOC text: a line a paragraph, windows-1252.
  Converted palm = convert(F + "/mobi-palmdoc.prc");
  Bytes palmText = slurp(F + "/mobi-palmdoc.html");
  size_t lines = 1;
  for (char ch : palmText) lines += ch == '\n';
  size_t blocks = 0;
  for (char ch : palm.flow) blocks += ch == '\n';
  CHECK(blocks == lines && palm.flow.find("na\xC3\xAFve") != std::string::npos);

  // KF8: the files put together again, each on a page of its own, the contents at their headings.
  Converted k8 = convert(F + "/mobi-kf8.azw3");
  CHECK(k8.result == BOOK_DONE);
  checkFlow("mobi-kf8.azw3", k8);
  auto toc = tocParse(k8.toc);
  CHECK(toc.size() == 3);
  for (size_t i = 0; i < toc.size(); i++) {
    bool page;
    std::string b = blockAt(k8.flow, toc[i].offset, &page);
    char want[32];
    snprintf(want, sizeof want, "EIGHT File %zu", i + 1);
    if (b != want || (i && !page) || toc[i].title != "File " + std::to_string(i + 1)) printf("FAIL kf8 entry %zu -> \"%s\"\n", i, b.c_str()), fails++;
  }
  CHECK(k8.flow.find("p { margin") == std::string::npos);  // the style sheet flow is not text
  CHECK(k8.flow.find("\xC3\xA9t\xC3\xA9") != std::string::npos);
  // Each heading's paragraph follows it: the pieces went inside their skeletons, in order.
  Bytes put = slurp(F + "/mobi-kf8.html");
  CHECK(put.find("<div aid=\"1\"><h2>EIGHT File 1") != std::string::npos);

  // MOBI 7 + KF8: the KF8 half only, not the book twice.
  Converted combo = convert(F + "/mobi-combo.mobi");
  CHECK(combo.result == BOOK_DONE && combo.flow == k8.flow && combo.meta.cover == 'j');
  CHECK(combo.flow.find("SEVEN") == std::string::npos);
  // Cut inside the KF8 half: the MOBI 7 one is whole, and is read.
  Bytes whole = slurp(F + "/mobi-combo.mobi");
  spit(OUT + "-cut.mobi", whole.substr(0, whole.find("BOUNDARY") + 900));
  Converted cut = convert(OUT + "-cut.mobi");
  CHECK(cut.result == BOOK_DONE && cut.flow.find("SEVEN only") != std::string::npos && cut.flow.find("EIGHT") == std::string::npos);

  // DRM: listed in the library with its title, refused when opened.
  BookMeta m;
  BookErr err;
  CHECK(bookProbe((F + "/mobi-drm.mobi").c_str(), m, err) && m.title == "A Locked Book" && m.author == "Di Lock");
  Converted drm = convert(F + "/mobi-drm.mobi");
  CHECK(drm.result == -(int)BE_DRM);

  // Not a book; a database too small for its record table.
  spit(OUT + "-junk.mobi", Bytes(100, 'x'));
  CHECK(!bookProbe((OUT + "-junk.mobi").c_str(), m, err));
  CHECK(!mobiProbe((OUT + "-junk.mobi").c_str(), m, err) && err == BE_FORMAT);
  Bytes head = whole.substr(0, 78);
  head[76] = '\x7F', head[77] = '\xFF';
  spit(OUT + "-short.mobi", head);
  CHECK(!mobiProbe((OUT + "-short.mobi").c_str(), m, err) && err == BE_DAMAGED);
}

// Cut at many lengths, bytes flipped with a fixed seed: an answer every
// time, never a crash or a hang (UBSan, and convert()'s step bound).
static void robustness(const std::string &path, int cuts, int flips) {
  Bytes good = slurp(path);
  if (good.empty()) return;
  std::string tmp = OUT + "-bad.mobi";
  uint32_t seed = 12345;
  auto rnd = [&]() { return seed = seed * 1103515245 + 12345, seed >> 8; };
  int done = 0, failed = 0;
  for (int i = 0; i < cuts + flips; i++) {
    Bytes b = good;
    if (i < cuts) b.resize((size_t)((uint64_t)good.size() * i / cuts));
    else
      for (int k = 1 + rnd() % 8; k > 0; k--) {
        // Most flips where the structure is: the headers, the indexes' records, the record table.
        size_t at = rnd() % 3 ? rnd() % std::min(good.size(), (size_t)12000) : rnd() % good.size();
        b[at] ^= (char)(1 << (rnd() % 8));
      }
    spit(tmp, b);
    BookMeta m;
    BookErr err;
    bookProbe(tmp.c_str(), m, err);
    Converted c = convert(tmp);
    if (c.result == BOOK_DONE) {
      done++;
      std::string why;
      if (!flowValidate(c.flow, &why) || !tocValidate(c.toc, c.flow, &why)) printf("FAIL %s variant %d: %s\n", path.c_str(), i, why.c_str()), fails++;
    } else {
      failed++;
    }
  }
  printf("  %s: %d variants, %d converted, %d refused\n", path.substr(path.rfind('/') + 1).c_str(), cuts + flips, done, failed);
}

// ---------------------------------------------------------------- real books

static bool isUtf8(const Bytes &s, size_t &bad) {
  for (size_t i = 0; i < s.size();) {
    uint8_t c = s[i];
    size_t n = c < 0x80 ? 1 : c >= 0xC2 && c <= 0xDF ? 2 : c >= 0xE0 && c <= 0xEF ? 3 : c >= 0xF0 && c <= 0xF4 ? 4 : 0;
    bool ok = n && i + n <= s.size();
    for (size_t k = 1; ok && k < n; k++) ok = ((uint8_t)s[i + k] & 0xC0) == 0x80;
    if (!ok) return bad = i, false;
    i += n;
  }
  return true;
}

static size_t count(const Bytes &s, const char *w) {
  size_t n = 0;
  for (size_t at = s.find(w); at != std::string::npos; at = s.find(w, at + 1)) n++;
  return n;
}

static void realBook(const std::string &dir, const std::string &name) {
  std::string path = dir + "/" + name;
  mobi::Book b;
  if (b.open(path.c_str()) != BE_OK) return;
  BookErr err;
  Bytes raw = rawText(path, err);
  mobi::Book info;
  info.open(path.c_str());
  info.openText();
  Converted c = convert(path);
  auto toc = tocParse(c.toc);
  size_t pages = count(c.flow, "\x0C");
  printf("  %s: %s%s, text %zu bytes (header says %u), err %d; flow %zu bytes, %zu pages, %zu contents, cover %c, %d steps, longest %.2f ms, reader memory %zu\n",
         name.c_str(), info.part.kf8() ? "KF8" : info.palmDoc ? "PalmDOC" : "MOBI", info.part.compression == 17480 ? " HUFF/CDIC" : info.part.compression == 2 ? " LZ77" : "",
         raw.size(), info.part.textLen, err, c.flow.size(), pages, toc.size(), c.meta.cover ? c.meta.cover : '-', c.steps, c.longest * 1000, info.memory());
  if (c.meta.description.size()) printf("    description (%zu bytes): %.120s\n", c.meta.description.size(), c.meta.description.c_str());
  if (err != BE_OK) return;
  // The header's length is the text's (KF8's may count only the first flow, the book; style sheets follow).
  if (info.part.kf8() ? raw.size() < info.part.textLen : raw.size() != info.part.textLen) printf("FAIL %s: %zu bytes of text, the header says %u\n", name.c_str(), raw.size(), info.part.textLen), fails++;
  size_t bad;
  if (info.part.utf8() && !isUtf8(raw, bad)) printf("FAIL %s: broken UTF-8 at %zu (record %zu)\n", name.c_str(), bad, bad / info.part.recSize), fails++;
  CHECK(c.result == BOOK_DONE);
  checkFlow(name, c);
  // How many entries point at a block that holds their title (or the next one does: a page number before it).
  size_t titled = 0;
  for (auto &e : toc) {
    std::string word = e.title.substr(0, e.title.find(' ')), here = blockAt(c.flow, e.offset);
    size_t next = c.flow.find('\n', e.offset);
    titled += here.find(word) != std::string::npos || (next != std::string::npos && blockAt(c.flow, (uint32_t)next + 1).find(word) != std::string::npos);
  }
  printf("    %zu of %zu contents entries at their title\n", titled, toc.size());
  if (name == "alice.mobi" || name == "alice.azw3") {
    CHECK(toc.size() >= 14 && pages >= 14);
    int chapters = 0;
    for (auto &e : toc) chapters += e.title.compare(0, 8, "CHAPTER ") == 0;
    CHECK(chapters == 12);
    // The same words as the plain text: the book's name for her, and its last lines.
    Bytes txt = slurp(dir + "/alice.txt");
    if (txt.size()) {
      size_t inText = count(txt, "Alice"), inFlow = count(c.flow, "Alice");
      printf("    \"Alice\": %zu in alice.txt, %zu in the flow\n", inText, inFlow);
      CHECK(inFlow + 20 > inText && inFlow < inText + 20);
      CHECK(c.flow.find("and the happy summer days.") != std::string::npos);
    }
    CHECK(count(c.flow, "Down the Rabbit-Hole") <= 3);  // the chapter once, plus the contents
  }
}

static void realBooks() {
  const char *dir = getenv("BOOKS_DIR");
  if (!dir) return;
  DIR *d = opendir(dir);
  if (!d) return;
  std::vector<std::string> names;
  while (struct dirent *e = readdir(d)) {
    std::string n = e->d_name;
    for (const char *ext : {".mobi", ".azw", ".azw3", ".prc"})
      if (n.size() > strlen(ext) && n.compare(n.size() - strlen(ext), strlen(ext), ext) == 0 && n[0] != '.') names.push_back(n);
  }
  closedir(d);
  std::sort(names.begin(), names.end());
  printf("books in %s:\n", dir);
  for (auto &n : names) realBook(dir, n);
  for (const char *n : {"alice.mobi", "alice.azw3"}) robustness(std::string(dir) + "/" + n, 40, 120);
}

int main(int argc, char **argv) {
  F = argc > 1 ? argv[1] : "fixtures";
  if (argc == 5 && !strcmp(argv[2], "--raw")) {
    BookErr err;
    Bytes raw = rawText(argv[3], err);
    spit(argv[4], raw);
    printf("%zu bytes, err %d\n", raw.size(), err);
    return err != BE_OK;
  }
  testLz77();
  testTrailers();
  testExth();
  testFixtures();
  printf("robustness:\n");
  for (const char *n : {"mobi-plain-1252.mobi", "mobi-lz77-utf8.mobi", "mobi-huff.mobi", "mobi-palmdoc.prc", "mobi-kf8.azw3", "mobi-combo.mobi"})
    robustness(F + "/" + n, 60, 250);
  realBooks();
  printf("mobi: %s\n", fails ? "FAILED" : "ok");
  return fails ? 1 : 0;
}
