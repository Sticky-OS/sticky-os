// The core readers through BookConverter (ebook.h): EPUB 2 and 3, FB2,
// plain text in both shapes, Markdown, HTML; their meta and covers; DRM;
// cut-short and damaged copies of every fixture; and, with BOOKS_DIR set,
// every real .epub / .fb2 / .txt / .md / .html there, end to end.
// The fixtures are made by tools/make_fixtures.py.
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <algorithm>
#include <string>
#include <vector>

#include "ebook.h"
#include "flowcheck.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static double now() {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}

struct Result {
  int end = 0;  // BOOK_DONE or -(BookErr)
  int steps = 0;
  double maxStep = 0, total = 0;
  BookMeta meta;
  std::string flow, toc, cover;
};

static const char *FLOW = "build/formats_test.flow";
static const char *COVER = "build/formats_test.cover";

// Converts a book; a step budget catches a reader that never ends.
static Result convert(const std::string &book, int maxSteps = 200000) {
  Result r;
  remove(COVER);
  BookConverter c;
  double t0 = now();
  if (!c.begin(book.c_str(), FLOW, COVER)) {
    r.end = -(int)c.error();
    CHECK(c.error() != BE_OK);
    return r;
  }
  int last = 0;
  for (;;) {
    double s = now();
    int p = c.step();
    double d = now() - s;
    if (d > r.maxStep) r.maxStep = d;
    r.steps++;
    if (p < 0 || p == BOOK_DONE) {
      r.end = p;
      break;
    }
    CHECK(p >= last && p < BOOK_DONE);  // progress never goes back
    last = p;
    if (r.steps > maxSteps) {
      printf("FAIL %s: more than %d steps\n", book.c_str(), maxSteps);
      fails++;
      c.abort();
      r.end = -1;
      break;
    }
  }
  r.total = now() - t0;
  r.meta = c.meta();
  if (r.end == BOOK_DONE) {
    r.flow = flowSlurp(FLOW), r.toc = flowSlurp(std::string(FLOW) + ".toc"), r.cover = flowSlurp(COVER);
    std::string why;
    if (!flowValidate(r.flow, &why)) printf("FAIL %s: flow: %s\n", book.c_str(), why.c_str()), fails++;
    if (!tocValidate(r.toc, r.flow, &why)) printf("FAIL %s: toc: %s\n", book.c_str(), why.c_str()), fails++;
    if (r.meta.cover) CHECK(r.cover.size() > 0);
  } else {
    // A failed conversion leaves nothing behind.
    FILE *f = fopen(FLOW, "rb");
    CHECK(!f);
    if (f) fclose(f);
  }
  return r;
}

struct Expect {
  const char *file, *flow, *toc;
};

static const Expect EXPECT[] = {
    {"epub-two.epub",
     "\x0C\x01" "Chapter 1\nIt was \x0Ewinter\x0F on the river.\n\x05* * *\nThe ink stayed\xC2\xA0" "dark \xE2\x80\x94 and quiet\xE2\x80\xA6\n\x0C\x02" "Chapter 2\nA \x10paper\x11 boat.\n\x03" "Deep\ndown\ndeeper\n",
     "0\x09" "1\x09Part One\n0\x09" "2\x09" "Chapter 1\n87\x09" "2\x09" "Chapter 2 & more\n115\x09" "3\x09" "A deep part\n126\x09" "3\x09" "Deeper\n"},
    {"epub-three.epub",
     "\x0C\x01Premier chapitre\nL\xE2\x80\x99\xC3\xA9t\xC3\xA9 \xC3\xA0 Paris.\n\x0C" "Avant.\n\x02" "Deuxi\xC3\xA8me\nTexte.\nMilieu trois fin.\nApr\xC3\xA8s.\n",
     "0\x09" "1\x09Premier chapitre\n47\x09" "1\x09" "Deuxi\xC3\xA8me\n83\x09" "2\x09Trois\n"},
    {"epub-fonts.epub",
     "\x0COpen.\n",
     ""},
    {"fb2-river.fb2",
     "\x0C\x01\xD0\xA2\xD0\xB8\xD1\x85\xD0\xB0\xD1\x8F \xD1\x80\xD0\xB5\xD0\xBA\xD0\xB0\n\x04\xD0\xA1\xD0\xBB\xD0\xBE\xD0\xB2\xD0\xB0 & \xD0\xB2\xD0\xBE\xD0\xB4\xD0\xB0.\n\x05\xD0\x9A\xD1\x82\xD0\xBE-\xD1\x82\xD0\xBE\n\x0C\x01\xD0\x93\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB0 1\x0B\xD0\x97\xD0\xB8\xD0\xBC\xD0\xB0\n\xD0\x91\xD1\x8B\xD0\xBB\xD0\xBE \x0E\xD1\x82\xD0\xB8\xD1\x85\xD0\xBE\x0F \xD0\xB8 \x10\xD1\x82\xD0\xB5\xD0\xBC\xD0\xBD\xD0\xBE\x11.[1]\n\x05* * *\n\x05\xD0\x9F\xD0\xB5\xD1\x81\xD0\xBD\xD1\x8F\n\xD0\xA1\xD1\x82\xD1\x80\xD0\xBE\xD0\xBA\xD0\xB0 \xD0\xBE\xD0\xB4\xD0\xB8\xD0\xBD,\x0B\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xBA\xD0\xB0 \xD0\xB4\xD0\xB2\xD0\xB0.\n\x02\xD0\xA7\xD0\xB0\xD1\x81\xD1\x82\xD1\x8C\n\xD0\x92\xD0\xBD\xD1\x83\xD1\x82\xD1\x80\xD0\xB8.\n\x0C\x01\xD0\x93\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB0 2\n\x04\xD0\xA6\xD0\xB8\xD1\x82\xD0\xB0\xD1\x82\xD0\xB0.\n\xD0\x9A\xD0\xBE\xD0\xBD\xD0\xB5\xD1\x86.\n\x0C\x01Notes\n\x03" "1\n\xD0\x9F\xD1\x80\xD0\xB8\xD0\xBC\xD0\xB5\xD1\x87\xD0\xB0\xD0\xBD\xD0\xB8\xD0\xB5.\n",
     "0\x09" "1\x09\xD0\xA2\xD0\xB8\xD1\x85\xD0\xB0\xD1\x8F \xD1\x80\xD0\xB5\xD0\xBA\xD0\xB0\n59\x09" "1\x09\xD0\x93\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB0 1 \xD0\x97\xD0\xB8\xD0\xBC\xD0\xB0\n186\x09" "2\x09\xD0\xA7\xD0\xB0\xD1\x81\xD1\x82\xD1\x8C\n212\x09" "1\x09\xD0\x93\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB0 2\n254\x09" "1\x09Notes\n"},
    {"text-wrapped-1252.txt",
     "The Project Gutenberg eBook of The Quiet River\nTitle: The Quiet River\nAuthor: Ana Ink\nLanguage: English\n*** START OF THE PROJECT GUTENBERG EBOOK THE QUIET RIVER ***\n\x07\nContents\nCHAPTER I. Winter\x0B" "CHAPTER II. The Boat\n\x0C\x02" "CHAPTER I.\x0BWinter\nIt was winter on the river, and the ink in the lantern room had gone thick and slow. Nobody read at night any more; the caf\xC3\xA9 was shut and the paper boats stayed \x0Every\x0F still along the quay, waiting for spring.\nChapter one of the old book was the best part, she said, and closed it.\nHow quiet the river,\x0BHow dark the ink,\x0BHow slow the winter\x0B" "Before you think.\n\x07\n\x0C\x02" "CHAPTER II.\x0BThe Boat\nA paper boat is a small thing, but it carries a word from one bank to the other if the wind is kind and the reader patient enough to wait.\n\x07\n*** END OF THE PROJECT GUTENBERG EBOOK THE QUIET RIVER ***\n",
     "215\x09" "1\x09" "CHAPTER I. Winter\n597\x09" "1\x09" "CHAPTER II. The Boat\n"},
    {"text-perline.txt",
     "\x0C\x01PART ONE\n\x0C\x02" "Chapter 1: The Ink\nIt was winter on the river, and the ink in the lantern room had gone thick and slow, so nobody read at night any more, not even the old ferryman.\nThe paper boats stayed still along the quay, waiting for spring, and the caf\xC3\xA9 on the corner kept its shutters down from November until the thaw.\n\x0C\x02II\nThe paper boats stayed still along the quay, waiting for spring, and the caf\xC3\xA9 on the corner kept its shutters down from November until the thaw.\n",
     "0\x09" "1\x09PART ONE\n11\x09" "2\x09" "Chapter 1: The Ink\n324\x09" "2\x09II\n"},
    {"text-cp1251.txt",
     "\x0C\x02\xD0\x93\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB0 1\n\xD0\x91\xD1\x8B\xD0\xBB\xD0\xBE \xD1\x82\xD0\xB8\xD1\x85\xD0\xBE \xD0\xBD\xD0\xB0 \xD1\x80\xD0\xB5\xD0\xBA\xD0\xB5, \xD0\xB8 \xD1\x87\xD0\xB5\xD1\x80\xD0\xBD\xD0\xB8\xD0\xBB\xD0\xB0 \xD0\xB2 \xD0\xBA\xD0\xBE\xD0\xBC\xD0\xBD\xD0\xB0\xD1\x82\xD0\xB5 \xD1\x81 \xD1\x84\xD0\xBE\xD0\xBD\xD0\xB0\xD1\x80\xD1\x91\xD0\xBC \xD1\x81\xD1\x82\xD0\xB0\xD0\xBB\xD0\xB8 \xD0\xB3\xD1\x83\xD1\x81\xD1\x82\xD1\x8B\xD0\xBC\xD0\xB8 \xD0\xB8 \xD0\xBC\xD0\xB5\xD0\xB4\xD0\xBB\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8B\xD0\xBC\xD0\xB8.\n\x0C\x02\xD0\x93\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB0 2\n\xD0\x9A\xD0\xBE\xD0\xBD\xD0\xB5\xD1\x86 \xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD1\x80\xD0\xB8\xD0\xB8, \xD0\xB8 \xD0\xB2\xD1\x81\xD0\xB5 \xD1\x83\xD1\x88\xD0\xBB\xD0\xB8 \xD0\xB4\xD0\xBE\xD0\xBC\xD0\xBE\xD0\xB9.\n",
     "0\x09" "1\x09\xD0\x93\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB0 1\n153\x09" "1\x09\xD0\x93\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB0 2\n"},
    {"text-notes.md",
     "\x0C\x01The Quiet River\nA \x0Equiet\x0F river and \x10" "dark\x11 ink, with code and a link *not italic*. Second line of the same paragraph.\n\x02Underlined\n\x06one\n\x06two \x0Eit\x0F\n\x06" "1. first\n\x06" "2. second\n\x04" "A quote on two lines.\n\x07\n\xC2\xA0\xC2\xA0" "code line\x0Bsecond\n\x02Two & three\nLast paragraph with [a boat] and snake_case_word.\n",
     "0\x09" "1\x09The Quiet River\n120\x09" "2\x09Underlined\n214\x09" "2\x09Two & three\n"},
    {"html-utf16.html",
     "\x01One\ncaf\xC3\xA9 \xE2\x80\x9Cquiet\xE2\x80\x9D \xE2\x80\x94 done\n\x02Two\nend\n",
     "0\x09" "1\x09One\n32\x09" "2\x09Two\n"},
    {"html-latin1.htm",
     "na\xC3\xAFve\n",
     ""},
};

static void exact(const std::string &dir) {
  for (const Expect &e : EXPECT) {
    Result r = convert(dir + "/" + e.file);
    if (r.end != BOOK_DONE) {
      printf("FAIL %s: ended %d\n", e.file, r.end);
      fails++;
      continue;
    }
    if (r.flow != e.flow) printf("FAIL %s flow:\n%s\n--- want ---\n%s\n", e.file, flowDump(r.flow).c_str(), flowDump(e.flow).c_str()), fails++;
    if (r.toc != e.toc) printf("FAIL %s toc:\n%s--- want ---\n%s", e.file, r.toc.c_str(), e.toc), fails++;
  }
}

static void meta(const std::string &dir) {
  struct M { const char *file, *title, *author, *lang; char cover; const char *description; };
  static const M WANT[] = {
      {"epub-two.epub", "The Quiet River", "Ana Ink", "en", 'j', "A short book about ink & rivers.\nSecond paragraph."},
      {"epub-three.epub", "L\xE2\x80\x99" "Encre & la Rivi\xC3\xA8re", "Ana Ink", "fr", 'p', "Un livre court. Fin \xC3\xA9t\xC3\xA9."},
      {"fb2-river.fb2", "\xD0\xA2\xD0\xB8\xD1\x85\xD0\xB0\xD1\x8F \xD1\x80\xD0\xB5\xD0\xBA\xD0\xB0", "\xD0\x90\xD0\xBD\xD0\xBD\xD0\xB0 \xD0\x98. \xD0\xA7\xD0\xB5\xD1\x80\xD0\xBD\xD0\xB8\xD0\xBB\xD0\xBE\xD0\xB2\xD0\xB0", "ru", 'j',
       "\xD0\x9A\xD0\xBD\xD0\xB8\xD0\xB3\xD0\xB0 \xD0\xBE \xD1\x80\xD0\xB5\xD0\xBA\xD0\xB5 \xD0\xB8 \xD1\x87\xD0\xB5\xD1\x80\xD0\xBD\xD0\xB8\xD0\xBB\xD0\xB0\xD1\x85.\n"
       "\xD0\x92\xD1\x82\xD0\xBE\xD1\x80\xD0\xB0\xD1\x8F \xD1\x87\xD0\xB0\xD1\x81\xD1\x82\xD1\x8C."},
      {"text-wrapped-1252.txt", "The Quiet River", "Ana Ink", "en", 0, ""},
      {"text-perline.txt", "", "", "", 0, ""},
      {"html-utf16.html", "A River & Ink", "", "", 0, "About a river & ink."},
      {"html-latin1.htm", "Caf\xC3\xA9", "", "", 0, ""},
  };
  for (const M &m : WANT) {
    std::string path = dir + "/" + m.file;
    BookMeta pm;
    BookErr err;
    CHECK(bookProbe(path.c_str(), pm, err));
    Result r = convert(path);
    for (const BookMeta *b : {&pm, &r.meta}) {
      if (b->title != m.title || b->author != m.author || b->language != m.lang || b->description != m.description)
        printf("FAIL %s meta: \"%s\" \"%s\" \"%s\" \"%s\"\n", m.file, b->title.c_str(), b->author.c_str(), b->language.c_str(), b->description.c_str()), fails++;
    }
    CHECK(r.meta.cover == m.cover);
    if (m.cover == 'j') CHECK(r.cover.size() > 4 && (uint8_t)r.cover[0] == 0xFF && (uint8_t)r.cover[1] == 0xD8 && (uint8_t)r.cover[r.cover.size() - 1] == 0xD9);
    if (m.cover == 'p') CHECK(r.cover.size() > 8 && !memcmp(r.cover.data(), "\x89PNG", 4) && !memcmp(r.cover.data() + r.cover.size() - 4, "\xAE\x42\x60\x82", 4));
  }
  // DRM: refused, by the probe too.
  BookMeta pm;
  BookErr err;
  CHECK(!bookProbe((dir + "/epub-drm.epub").c_str(), pm, err) && err == BE_DRM);
  CHECK(convert(dir + "/epub-drm.epub").end == -(int)BE_DRM);
  // No cover wanted: none written.
  {
    BookConverter c;
    remove(COVER);
    CHECK(c.begin((dir + "/epub-two.epub").c_str(), FLOW, nullptr));
    int p;
    while ((p = c.step()) >= 0 && p < BOOK_DONE) {}
    CHECK(p == BOOK_DONE && c.meta().cover == 0);
    FILE *f = fopen(COVER, "rb");
    CHECK(!f);
    if (f) fclose(f);
  }
}

static uint32_t lcg(uint32_t &x) { return x = x * 1103515245u + 12345u; }

// Every fixture cut short and damaged: no crash, no hang, a valid flow or an error.
static void hostile(const std::string &dir) {
  static const char *const FILES[] = {"epub-two.epub", "epub-three.epub", "epub-fonts.epub", "fb2-river.fb2", "text-wrapped-1252.txt",
                                      "text-perline.txt", "text-cp1251.txt", "text-notes.md", "html-utf16.html", "html-latin1.htm"};
  uint32_t seed = 2024;
  int runs = 0, done = 0;
  for (const char *name : FILES) {
    std::string good = flowSlurp(dir + "/" + name), ext = strrchr(name, '.');
    std::string path = std::string("build/formats_hostile") + ext;
    auto run = [&](const std::string &bytes) {
      FILE *f = fopen(path.c_str(), "wb");
      fwrite(bytes.data(), 1, bytes.size(), f);
      fclose(f);
      BookMeta pm;
      BookErr err;
      bookProbe(path.c_str(), pm, err);
      Result r = convert(path, 20000);
      runs++;
      done += r.end == BOOK_DONE;
    };
    size_t stride = good.size() / 150 + 1;
    for (size_t cut = 0; cut < good.size(); cut += stride) run(good.substr(0, cut));
    for (int k = 0; k < 150; k++) {
      std::string bad = good;
      int flips = 1 + lcg(seed) % 4;
      for (int j = 0; j < flips; j++) bad[(lcg(seed) >> 8) % bad.size()] ^= (char)(1 << ((lcg(seed) >> 16) % 8));
      run(bad);
    }
  }
  printf("  hostile: %d runs, %d converted\n", runs, done);
}

// Every real book in BOOKS_DIR, end to end.
static void real(const char *dir) {
  DIR *d = opendir(dir);
  if (!d) return;
  std::vector<std::string> names;
  while (struct dirent *e = readdir(d)) {
    std::string n = e->d_name;
    BookFormat f = bookFormatOfName(n.c_str());
    if (f == BF_EPUB || f == BF_FB2 || f == BF_TXT || f == BF_HTML) names.push_back(n);
  }
  closedir(d);
  std::sort(names.begin(), names.end());
  for (const std::string &n : names) {
    std::string path = std::string(dir) + "/" + n;
    long size = (long)flowSlurp(path).size();
    BookMeta pm;
    BookErr err;
    double t0 = now();
    bool probed = bookProbe(path.c_str(), pm, err);
    double probe = now() - t0;
    Result r = convert(path);
    std::vector<FlowTocLine> toc = tocParse(r.toc);
    printf("  %-28s %9ld -> %8zu bytes, toc %3zu, %4d steps, max step %5.2f ms, total %7.1f ms, probe %.1f ms  \"%s\" / %s / %s%s\n", n.c_str(), size, r.flow.size(),
           toc.size(), r.steps, r.maxStep * 1000, r.total * 1000, probe * 1000, r.meta.title.c_str(), r.meta.author.c_str(), r.meta.language.c_str(),
           r.meta.cover ? " cover" : "");
    if (r.meta.description.size()) printf("    description (%zu bytes): %.150s\n", r.meta.description.size(), r.meta.description.c_str());
    CHECK(pm.description == r.meta.description);
    if (r.end != BOOK_DONE) {
      printf("FAIL %s: ended %d\n", n.c_str(), r.end);
      fails++;
      continue;
    }
    CHECK(probed && pm.title == r.meta.title);
    // Plausible: text of at least a tenth of a plain file, a hundredth of a packed or picture-filled one.
    long least = bookFormatOfName(n.c_str()) == BF_EPUB ? size / 100 : size / 10;
    if ((long)r.flow.size() < least) printf("FAIL %s: only %zu bytes of text\n", n.c_str(), r.flow.size()), fails++;
    if (toc.empty()) printf("FAIL %s: no contents\n", n.c_str()), fails++;
  }
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : "fixtures";
  exact(dir);
  meta(dir);
  hostile(dir);
  if (const char *books = getenv("BOOKS_DIR")) real(books);
  remove(FLOW), remove((std::string(FLOW) + ".toc").c_str()), remove(COVER);
  printf("formats: %s\n", fails ? "FAILED" : "ok");
  return fails ? 1 : 0;
}
