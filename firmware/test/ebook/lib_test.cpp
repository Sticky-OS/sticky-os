// The library (booklib.h): finding the books, the index, the cache and its
// tidying, the order of the list, reading places and bookmarks, files cut
// short, and opening a book. On folders made under build/lib_test.
//
// Converting is done by a stand-in (the library takes any converter), so the
// steps, the errors and the full card can be played; with the real text
// reader in place a plain text book is also converted for real.
// BOOKS_DIR=<folder>: that folder is scanned as a card's "books" and listed.
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <string>
#include <vector>

#include "booklib.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

// ---------------------------------------------------------------- files

static const char *BASE = "build/lib_test";

static void rmrf(const std::string &path) {
  if (path.compare(0, strlen(BASE), BASE) != 0) abort();  // only ever our own folder
  std::string cmd = "rm -rf '" + path + "'";
  if (system(cmd.c_str()) != 0) abort();
}
static void mkdirs(const std::string &path) {
  for (size_t i = 1; i <= path.size(); i++)
    if (i == path.size() || path[i] == '/') mkdir(path.substr(0, i).c_str(), 0777);
}
static bool exists(const std::string &path) {
  struct stat st;
  return lstat(path.c_str(), &st) == 0;
}
static void put(const std::string &path, const std::string &data, time_t mtime = 0) {
  size_t slash = path.rfind('/');
  if (slash != std::string::npos) mkdirs(path.substr(0, slash));
  FILE *f = fopen(path.c_str(), "wb");
  if (!f || fwrite(data.data(), 1, data.size(), f) != data.size()) {
    printf("can't write %s\n", path.c_str());
    abort();
  }
  fclose(f);
  if (mtime) {
    struct timeval tv[2] = {{mtime, 0}, {mtime, 0}};
    utimes(path.c_str(), tv);
  }
}
static std::string slurp(const std::string &path) {
  std::string s;
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) return s;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) s.append(buf, n);
  fclose(f);
  return s;
}
static ino_t inode(const std::string &path) {
  struct stat st;
  return stat(path.c_str(), &st) == 0 ? st.st_ino : 0;
}
static std::string fresh(const char *name) {
  std::string root = std::string(BASE) + "/" + name;
  rmrf(root);
  mkdirs(root);
  return root;
}
static uint32_t fnv32(const std::string &s) {
  uint32_t h = 2166136261u;
  for (unsigned char c : s) h = (h ^ c) * 16777619u;
  return h;
}
// A file of the library's kind, whole: the body and its checksum line.
static std::string sealed(const std::string &body) {
  char t[16];
  snprintf(t, sizeof t, "END %08x\n", fnv32(body));
  return body + t;
}
static std::string cacheOf(const std::string &root) { return root + "/.sticky/books/"; }

// ---------------------------------------------------------------- stand-ins

// The probe: a book whose text starts "T:<title>|A:<author>|" says so; one
// that starts with BAD can't be read.
static int probes;
static bool fakeProbe(const char *path, BookMeta &meta, BookErr &err) {
  probes++;
  meta = BookMeta();
  err = BE_OK;
  std::string s = slurp(path);
  const char *slash = strrchr(path, '/');
  meta.format = bookFormatOfName(slash ? slash + 1 : path);
  if (s.compare(0, 3, "BAD") == 0) return err = BE_DAMAGED, false;
  size_t t = s.find("T:"), a = s.find("|A:");
  if (t == 0 && a != std::string::npos) {
    meta.title = s.substr(2, a - 2);
    size_t e = s.find('|', a + 3);
    meta.author = s.substr(a + 3, e == std::string::npos ? std::string::npos : e - a - 3);
  }
  size_t d = s.find("|D:");
  if (d != std::string::npos) meta.description = s.substr(d + 3);
  return true;
}

static struct FakeCfg {
  int steps, failStep, writeFails, begins, aborts, alive;
  BookErr failErr, beginErr;
  uint32_t flowBytes;
  char cover;
  std::string title, author, description;
  void reset() {
    steps = 3, failStep = -1, writeFails = 0, begins = aborts = 0;
    failErr = BE_DAMAGED, beginErr = BE_OK, flowBytes = 1000, cover = 0;
    title = author = description = "";
  }
} fake;

class FakeConverter : public LibConverter {
 public:
  FakeConverter() { fake.alive++; }
  ~FakeConverter() override {
    fake.alive--;
    if (running_) clean();
  }
  bool begin(const char *book, const char *flow, const char *cover) override {
    fake.begins++;
    n_ = 0;
    err_ = BE_OK;
    flow_ = flow, cover_ = cover;
    if (fake.beginErr != BE_OK) return err_ = fake.beginErr, false;
    put(flow_, "");
    put(flow_ + ".toc", "");
    running_ = true;
    return true;
  }
  int step() override {
    if (!running_) return -(int)err_;
    n_++;
    if (fake.writeFails > 0 && n_ == 1) return fake.writeFails--, fail(BE_WRITE);
    if (n_ == fake.failStep) return fail(fake.failErr);
    if (n_ < fake.steps) return n_ * 999 / fake.steps;
    put(flow_, std::string(fake.flowBytes, 'x'));
    put(flow_ + ".toc", "0\t1\tOne\n");
    if (fake.cover) put(cover_, "picture");
    meta_.title = fake.title, meta_.author = fake.author, meta_.language = "en", meta_.description = fake.description;
    meta_.format = BF_TXT, meta_.cover = fake.cover;
    running_ = false;
    return BOOK_DONE;
  }
  void abort() override {
    if (!running_) return;
    fake.aborts++;
    clean();
  }
  BookErr error() const override { return err_; }
  const BookMeta &meta() const override { return meta_; }

 private:
  int fail(BookErr e) {
    err_ = e;
    clean();
    return -(int)e;
  }
  void clean() {
    remove(flow_.c_str());
    remove((flow_ + ".toc").c_str());
    remove(cover_.c_str());
    running_ = false;
  }
  int n_ = 0;
  bool running_ = false;
  BookErr err_ = BE_OK;
  BookMeta meta_;
  std::string flow_, cover_;
};
static LibConverter *makeFake(void *) { return new FakeConverter(); }

static void wire(BookLibrary &lib) {
  lib.setProbe(fakeProbe);
  lib.setConverterFactory(makeFake, nullptr);
}

// ---------------------------------------------------------------- helpers

static int scanAll(BookLibrary &lib) {
  lib.scanBegin();
  int steps = 0;
  while (lib.scanStep())
    if (++steps > 100000) {
      CHECK(!"scan never ends");
      break;
    }
  return steps;
}
static int byTitle(BookLibrary &lib, const char *title) {
  LibBook b;
  for (int i = 0; lib.book(i, b); i++)
    if (!strcmp(b.title, title)) return i;
  return -1;
}
static std::string keyOf(BookLibrary &lib, const char *title) {
  LibBook b;
  int i = byTitle(lib, title);
  return i >= 0 && lib.book(i, b) ? b.key : "";
}
static std::string titles(BookLibrary &lib) {
  std::string s;
  LibBook b;
  for (int i = 0; lib.book(i, b); i++) s += std::string(i ? "," : "") + b.title;
  return s;
}
// Opens a book all the way; the last value openStep() gave.
static int openAll(BookLibrary &lib, int i, uint32_t now) {
  if (!lib.openBegin(i, now)) return -(int)lib.openError();
  int p, guard = 0;
  while ((p = lib.openStep()) >= 0 && p < BOOK_DONE)
    if (++guard > 100000) return -999;
  return p;
}
static int openTitle(BookLibrary &lib, const char *title, uint32_t now) { return openAll(lib, byTitle(lib, title), now); }

// ---------------------------------------------------------------- the tests

static void testNames() {
  CHECK(bookLibTitleFromName("pride-and-prejudice.epub") == "pride and prejudice");
  CHECK(bookLibTitleFromName("Alice_in__Wonderland - Lewis Carroll.MOBI") == "Alice in Wonderland Lewis Carroll");
  CHECK(bookLibTitleFromName("  spaced   out .txt") == "spaced out");
  CHECK(bookLibTitleFromName("Les Mis\xC3\xA9rables.epub") == "Les Mis\xC3\xA9rables");
  CHECK(bookLibTitleFromName("vol.2.final.pdf") == "vol.2.final");
  CHECK(bookLibTitleFromName("---.txt") == "---.txt");
  CHECK(bookLibTitleFromName("bad\xFF" "byte.txt") == "bad?byte");
  // A FAT date is a clock's reading with no zone: the same number wherever the board thinks it is.
  static const char *const ZONES[] = {"UTC", "Europe/Bucharest", "America/New_York", "Asia/Kolkata"};
  static const int DAYS[][3] = {{2024, 1, 15}, {2024, 7, 15}, {1980, 1, 1}, {2026, 11, 16}};
  for (auto &d : DAYS) {
    uint32_t first = 0;
    for (size_t z = 0; z < 4; z++) {
      setenv("TZ", ZONES[z], 1);
      tzset();
      struct tm tm = {};
      tm.tm_year = d[0] - 1900, tm.tm_mon = d[1] - 1, tm.tm_mday = d[2], tm.tm_hour = 13, tm.tm_min = 37, tm.tm_sec = 42;
      tm.tm_isdst = -1;  // as ESP-IDF's FAT layer asks
      uint32_t s = bookLibLocalStamp(mktime(&tm));
      if (!z) first = s;
      CHECK(s == first);
      CHECK(s % 86400 == 13 * 3600 + 37 * 60 + 42);
    }
  }
  unsetenv("TZ");
  tzset();
}

static void testWhere() {
  {  // no card
    BookLibrary lib("build/lib_test/does-not-exist");
    wire(lib);
    CHECK(!lib.scanBegin());
    CHECK(lib.state() == LS_NO_CARD && lib.count() == 0 && !lib.scanStep());
    CHECK(!lib.openBegin(0, 0) && lib.openError() == BE_OPEN);
  }
  {  // a card with nothing for us; nothing is written on it
    std::string root = fresh("where-none");
    put(root + "/photo.jpg", "x");
    put(root + "/other/deep.txt", "x");  // the top folder only
    BookLibrary lib(root.c_str());
    wire(lib);
    CHECK(!lib.scanBegin() && lib.state() == LS_NO_FOLDER);
    while (lib.scanStep()) {}
    CHECK(!exists(root + "/.sticky"));
  }
  {  // an empty books folder
    std::string root = fresh("where-empty");
    mkdirs(root + "/books/Author");
    put(root + "/top.txt", "x");  // not looked at: there is a books folder
    BookLibrary lib(root.c_str());
    wire(lib);
    CHECK(!lib.scanBegin() && lib.state() == LS_EMPTY && lib.count() == 0);
  }
  {  // books at the top
    std::string root = fresh("where-top");
    put(root + "/one.txt", "x");
    put(root + "/Two.EPUB", "x");
    put(root + "/sub/three.txt", "x");
    put(root + "/.hidden.txt", "x");
    BookLibrary lib(root.c_str());
    wire(lib);
    CHECK(lib.scanBegin() && lib.state() == LS_OK);
    CHECK(titles(lib) == "one,Two");
    LibBook b;
    CHECK(lib.book(1, b) && b.format == BF_EPUB && !strcmp(b.path, "Two.EPUB") && !b.probed);
  }
  {  // the folder and what's under it
    std::string root = fresh("where-books");
    std::string longName = std::string(251, 'n') + ".txt";  // 255 bytes: the longest a name gets
    put(root + "/top.txt", "x");
    put(root + "/books/a.txt", "x");
    put(root + "/books/Author/b.epub", "x");
    put(root + "/books/Author/Title (12)/c.mobi", "x");
    put(root + "/books/Author/Title (12)/too/deep.txt", "x");
    put(root + "/books/.hidden.txt", "x");
    put(root + "/books/._a.txt", "x");
    put(root + "/books/.Trashes/t.txt", "x");
    put(root + "/books/__MACOSX/m.txt", "x");
    put(root + "/books/System Volume Information/s.txt", "x");
    put(root + "/books/notes.doc", "x");
    put(root + "/books/cover.jpg", "x");
    put(root + "/books/It's \"quoted\" & spaced.txt", "x");
    put(root + "/books/\xC8\x98" "apte nop\xC8\x9Bi.fb2", "x");
    put(root + "/books/" + longName, "x");
    mkdirs(root + "/books/folder.txt");  // a folder with a book's name
    BookLibrary lib(root.c_str());
    wire(lib);
    CHECK(lib.scanBegin() && lib.state() == LS_OK && !lib.truncated());
    CHECK(lib.count() == 6);
    CHECK(byTitle(lib, "a") >= 0 && byTitle(lib, "b") >= 0 && byTitle(lib, "c") >= 0);
    CHECK(byTitle(lib, "It's \"quoted\" & spaced") >= 0);
    CHECK(byTitle(lib, "\xC8\x98" "apte nop\xC8\x9Bi") >= 0);
    CHECK(byTitle(lib, std::string(200, 'n').c_str()) >= 0);  // a title is cut at 200 bytes
    CHECK(byTitle(lib, "top") < 0 && byTitle(lib, "deep") < 0);
    LibBook b;
    CHECK(lib.book(byTitle(lib, "c"), b) && !strcmp(b.path, "books/Author/Title (12)/c.mobi") && b.format == BF_MOBI);
    while (lib.scanStep()) {}
    fake.reset();
    CHECK(openTitle(lib, std::string(200, 'n').c_str(), 0) == BOOK_DONE);  // and the long name opens
    CHECK(openTitle(lib, "It's \"quoted\" & spaced", 0) == BOOK_DONE);
  }
  for (const char *name : {"Books", "BOOKS", "ebooks", "eBooks"}) {
    std::string root = fresh("where-case");
    put(root + "/" + name + "/a.txt", "x");
    BookLibrary lib(root.c_str());
    wire(lib);
    CHECK(lib.scanBegin() && lib.count() == 1);
    LibBook b;
    CHECK(lib.book(0, b) && b.path == std::string(name) + "/a.txt");
  }
}

static void testCap() {
  std::string root = fresh("cap");
  for (int i = 0; i < LIB_MAX_BOOKS + 20; i++) {
    char name[64];
    snprintf(name, sizeof name, "/books/d%d/book-%03d.txt", i % 7, i);
    put(root + name, "x");
  }
  std::string orphan = cacheOf(root) + "00000000000000aa.flow";
  put(orphan, "x");
  BookLibrary lib(root.c_str());
  wire(lib);
  CHECK(lib.scanBegin() && lib.count() == LIB_MAX_BOOKS && lib.truncated());
  CHECK(lib.scanTotal() == LIB_MAX_BOOKS && lib.scanDone() == 0);
  while (lib.scanStep()) {}
  CHECK(lib.scanDone() == LIB_MAX_BOOKS);
  CHECK(exists(orphan));  // some books aren't in the list: nothing is called gone
  // Exactly the most the list holds isn't "more".
  std::string root2 = fresh("cap-exact");
  for (int i = 0; i < LIB_MAX_BOOKS; i++) {
    char name[64];
    snprintf(name, sizeof name, "/books/book-%03d.txt", i);
    put(root2 + name, "x");
  }
  BookLibrary lib2(root2.c_str());
  wire(lib2);
  probes = 0;
  scanAll(lib2);
  CHECK(lib2.count() == LIB_MAX_BOOKS && !lib2.truncated() && probes == LIB_MAX_BOOKS);
  BookLibrary lib3(root2.c_str());
  wire(lib3);
  probes = 0;
  scanAll(lib3);
  CHECK(lib3.count() == LIB_MAX_BOOKS && probes == 0);
}

static void testIndex() {
  std::string root = fresh("index"), cache = cacheOf(root);
  put(root + "/books/alice.txt", "T:Alice's Adventures|A:Lewis Carroll|", 1700000000);
  put(root + "/books/pride-and-prejudice.txt", "T:Pride and Prejudice|A:Jane Austen|", 1700000000);
  put(root + "/books/no-title.txt", "just text", 1700000000);
  put(root + "/books/broken.epub", "BAD", 1700000000);
  std::string kAlice, kPride;
  {
    BookLibrary lib(root.c_str());
    wire(lib);
    probes = 0;
    uint32_t g = lib.generation();
    CHECK(lib.scanBegin());
    CHECK(lib.generation() != g);
    CHECK(titles(lib) == "alice,broken,no title,pride and prejudice");  // names stand in at once
    CHECK(lib.scanning() && lib.scanTotal() == 4 && lib.scanDone() == 0);
    g = lib.generation();
    CHECK(lib.scanStep() && probes == 1 && lib.scanDone() == 1);  // a book a step
    CHECK(lib.generation() != g);
    CHECK(titles(lib) == "Alice's Adventures,broken,no title,pride and prejudice");
    while (lib.scanStep()) {}
    CHECK(!lib.scanning() && probes == 4);
    CHECK(titles(lib) == "Alice's Adventures,broken,no title,Pride and Prejudice");
    LibBook b;
    CHECK(lib.book(0, b) && !strcmp(b.author, "Lewis Carroll") && b.probed && !b.error && b.status == LB_NEW && strlen(b.key) == 16);
    CHECK(lib.book(1, b) && b.error && b.probed && !strcmp(b.author, ""));  // listed by its name, flagged
    CHECK(lib.book(2, b) && !b.error && !strcmp(b.title, "no title"));
    kAlice = keyOf(lib, "Alice's Adventures"), kPride = keyOf(lib, "Pride and Prejudice");
    CHECK(exists(cache + "library.idx") && !exists(cache + "library.idx.tmp"));
  }
  {  // again: one file read, nothing probed but the broken one
    BookLibrary lib(root.c_str());
    wire(lib);
    probes = 0;
    ino_t before = inode(cache + "library.idx");
    CHECK(lib.scanBegin());
    CHECK(titles(lib) == "Alice's Adventures,broken,no title,Pride and Prejudice");
    LibBook b;
    CHECK(lib.book(0, b) && b.probed && !strcmp(b.author, "Lewis Carroll") && b.key == kAlice);
    CHECK(lib.find(kPride.c_str()) == 3 && lib.find("0000000000000000") == -1 && lib.find("zz") == -1 && lib.find(nullptr) == -1);
    while (lib.scanStep()) {}
    CHECK(probes == 1);
    CHECK(inode(cache + "library.idx") == before);  // nothing changed: not written again
  }
  // What the cache holds for Alice and for Pride, and things that aren't the library's.
  for (const char *s : {".flow", ".flow.toc", ".cover", ".meta", ".pos"}) {
    put(cache + kAlice + s, "old");
    put(cache + kPride + s, "kept");
  }
  put(cache + "00000000000000aa.flow", "x");      // a book long gone
  put(cache + "00000000000000aa.pos.tmp", "x");
  put(cache + "library.idx.tmp", "half");
  put(cache + kPride + ".flow.tmp", "x");         // never made by us in a whole state: goes
  const std::string decoys[] = {
      cache + "notes.txt", cache + "library.idx.bak", cache + "00000000000000aa.other", cache + "00000000000000aa",
      cache + "00000000000000AA.epub", cache + "g0000000000000aa.flow", cache + "00000000000000aa.flowx", cache + "x.tmp",
      cache + "00000000000000bb.flow/inside.txt",  // a folder with a name of ours
      root + "/.sticky/other.txt", root + "/.sticky/00000000000000aa.flow", root + "/00000000000000aa.flow",
      root + "/books/00000000000000aa.flow", root + "/books/library.idx",
  };
  for (auto &d : decoys) put(d, "decoy");
  symlink("../other.txt", (cache + "00000000000000cc.flow").c_str());  // a link with a name of ours
  // Alice is replaced by another file of the same name; one book goes, one comes.
  put(root + "/books/alice.txt", "T:Alice, second edition|A:Lewis Carroll|", 1700000000);
  remove((root + "/books/no-title.txt").c_str());
  put(root + "/books/Verne/around-the-world.txt", "T:Around the World|A:Jules Verne|", 1700000000);
  {
    BookLibrary lib(root.c_str());
    wire(lib);
    probes = 0;
    CHECK(lib.scanBegin());
    CHECK(titles(lib) == "Alice's Adventures,around the world,broken,Pride and Prejudice");  // not checked yet
    while (lib.scanStep()) {}
    CHECK(probes == 3);  // the changed one, the new one, the broken one
    CHECK(titles(lib) == "Alice, second edition,Around the World,broken,Pride and Prejudice");
    std::string k2 = keyOf(lib, "Alice, second edition");
    CHECK(k2.size() == 16 && k2 != kAlice);
    CHECK(keyOf(lib, "Pride and Prejudice") == kPride);
    for (const char *s : {".flow", ".flow.toc", ".cover", ".meta", ".pos"}) {
      CHECK(!exists(cache + kAlice + s));
      CHECK(slurp(cache + kPride + s) == "kept");
    }
    CHECK(!exists(cache + "00000000000000aa.flow") && !exists(cache + "00000000000000aa.pos.tmp"));
    CHECK(!exists(cache + "library.idx.tmp") && !exists(cache + kPride + ".flow.tmp"));
    for (auto &d : decoys) CHECK(slurp(d) == "decoy");
    CHECK(exists(cache + "00000000000000cc.flow"));
    CHECK(exists(root + "/books/broken.epub") && exists(root + "/books/alice.txt"));
  }
  {  // the same size, another date: another book too
    BookLibrary lib(root.c_str());
    wire(lib);
    scanAll(lib);
    std::string k2 = keyOf(lib, "Alice, second edition");
    put(root + "/books/alice.txt", "T:Alice, second edition|A:Lewis Carroll|", 1700000100);
    BookLibrary again(root.c_str());
    wire(again);
    probes = 0;
    scanAll(again);
    CHECK(probes == 2 && keyOf(again, "Alice, second edition") != k2);
  }
  {  // the index is lost: titles come back from the books, places from the .pos files
    BookLibrary lib(root.c_str());
    wire(lib);
    scanAll(lib);
    fake.reset();
    fake.title = "Pride & Prejudice (converted)";
    CHECK(openTitle(lib, "Pride and Prejudice", 5000) == BOOK_DONE);
    lib.setPosition(400, 5000);
    lib.closeBook();
    remove((cache + "library.idx").c_str());
    BookLibrary again(root.c_str());
    wire(again);
    probes = 0;
    CHECK(again.scanBegin());
    CHECK(titles(again) == "alice,around the world,broken,pride and prejudice");
    while (again.scanStep()) {}
    CHECK(probes == 3);  // the converted one is known by its .meta
    LibBook b;
    CHECK(again.book(0, b) && !strcmp(b.title, "Pride & Prejudice (converted)") && b.status == LB_READING && b.permille == 400 &&
          b.lastRead == 5000);
    CHECK(again.lastRead() == 0);
  }
}

static void testOrder() {
  std::string root = fresh("order");
  const char *const books[][2] = {{"z.txt", "Zebra"}, {"a.txt", "apple"}, {"e.txt", "\xC3\x89mile"},   {"b.txt", "Banana"},
                                  {"s.txt", "\xC8\x99oim"}, {"l.txt", "\xC5\x81\xC3\xB3" "d\xC5\xBA"}, {"t.txt", "The Trial"}};
  for (auto &b : books) put(root + "/books/" + b[0], std::string("T:") + b[1] + "|A:|");
  BookLibrary lib(root.c_str());
  wire(lib);
  scanAll(lib);
  // By title: case and the common accents don't count; "The" does.
  CHECK(titles(lib) == "apple,Banana,\xC3\x89mile,\xC5\x81\xC3\xB3" "d\xC5\xBA,\xC8\x99oim,The Trial,Zebra");
  CHECK(lib.lastRead() == -1 && lib.count(LB_NEW) == 7 && lib.count(LB_READING) == 0);
  fake.reset();
  CHECK(openTitle(lib, "Zebra", 1000) == BOOK_DONE);
  CHECK(openTitle(lib, "apple", 2000) == BOOK_DONE);
  CHECK(openTitle(lib, "Banana", 3000) == BOOK_DONE);
  lib.setFinished(3000);
  CHECK(openTitle(lib, "The Trial", 4000) == BOOK_DONE);
  lib.setFinished(4000);
  lib.closeBook();
  // Being read (the last first), not started (by title), finished (the last first).
  CHECK(titles(lib) == "apple,Zebra,\xC3\x89mile,\xC5\x81\xC3\xB3" "d\xC5\xBA,\xC8\x99oim,The Trial,Banana");
  CHECK(lib.count(LB_READING) == 2 && lib.count(LB_NEW) == 3 && lib.count(LB_FINISHED) == 2);
  LibBook b;
  CHECK(lib.book(0, b) && b.status == LB_READING && b.lastRead == 2000);
  CHECK(lib.book(2, b) && b.status == LB_NEW && b.lastRead == 0);
  CHECK(lib.book(5, b) && b.status == LB_FINISHED && b.permille == 1000);
  CHECK(lib.lastRead() == 0);
  // The clock is lost (now 0): the order still follows what was read last.
  CHECK(openTitle(lib, "Zebra", 0) == BOOK_DONE);
  CHECK(lib.book(0, b) && !strcmp(b.title, "Zebra") && b.lastRead == 4001);
  lib.setPosition(10, 0);
  lib.setPosition(20, 0);
  CHECK(lib.book(0, b) && b.lastRead == 4001);  // already the newest: it doesn't creep
  CHECK(openTitle(lib, "apple", 0) == BOOK_DONE);
  CHECK(openTitle(lib, "\xC3\x89mile", 7) == BOOK_DONE);  // a clock that is plainly wrong
  lib.closeBook();
  CHECK(titles(lib).compare(0, 19, "\xC3\x89mile,apple,Zebra,") == 0);
  CHECK(lib.book(0, b) && b.lastRead == 4003);
  CHECK(openTitle(lib, "Zebra", 9000) == BOOK_DONE);  // and it's back
  lib.closeBook();
  CHECK(lib.book(0, b) && !strcmp(b.title, "Zebra") && b.lastRead == 9000);
  std::string want = titles(lib);
  // All of it from the one file, after a restart.
  BookLibrary again(root.c_str());
  wire(again);
  probes = 0;
  CHECK(again.scanBegin());
  CHECK(titles(again) == want && probes == 0);
  CHECK(again.lastRead() == 0 && again.count(LB_FINISHED) == 2);
  CHECK(openTitle(again, "apple", 0) == BOOK_DONE);
  CHECK(again.book(0, b) && !strcmp(b.title, "apple") && b.lastRead == 9001);  // the counter was kept

  // A card that never saw a clock.
  std::string root2 = fresh("order-noclock");
  for (const char *n : {"x", "y", "w"}) put(root2 + "/books/" + n + ".txt", "x");
  BookLibrary lib2(root2.c_str());
  wire(lib2);
  scanAll(lib2);
  CHECK(openTitle(lib2, "x", 0) == BOOK_DONE);
  CHECK(openTitle(lib2, "y", 0) == BOOK_DONE);
  CHECK(titles(lib2) == "y,x,w");
  CHECK(openTitle(lib2, "x", 0) == BOOK_DONE);
  CHECK(titles(lib2) == "x,y,w");
  CHECK(lib2.book(0, b) && b.lastRead == 3);
}

static void testState() {
  std::string root = fresh("state"), cache = cacheOf(root);
  put(root + "/books/book.txt", "T:Book|A:Author|");
  put(root + "/books/other.txt", "T:Other|A:|");
  std::string key, pos;
  {
    BookLibrary lib(root.c_str());
    wire(lib);
    scanAll(lib);
    fake.reset();
    fake.flowBytes = 10000;
    CHECK(!lib.isOpen() && !lib.saveState());
    lib.setPosition(5, 1);  // no book open: nothing happens
    CHECK(!lib.addMark(5, "x") && lib.markCount() == 0);
    CHECK(openTitle(lib, "Book", 100) == BOOK_DONE);
    key = lib.openKey(), pos = cache + key + ".pos";
    CHECK(lib.isOpen() && lib.position() == 0 && lib.flowSize() == 10000 && !lib.finished() && lib.markCount() == 0);
    CHECK(exists(pos));  // opened is "being read"
    ino_t first = inode(pos), idx = inode(cache + "library.idx");
    CHECK(lib.saveState() && inode(pos) == first);  // nothing changed: nothing written
    lib.setPosition(2500, 110);
    CHECK(lib.position() == 2500 && lib.permille() == 250);
    CHECK(lib.saveState() && inode(pos) != first);
    CHECK(inode(cache + "library.idx") == idx);  // the small file only
    first = inode(pos);
    lib.setPosition(2500, 120);  // the same page
    CHECK(lib.saveState() && inode(pos) == first);
    CHECK(lib.sync() && inode(cache + "library.idx") != idx);
    lib.setPosition(99999, 130);  // beyond the end: the end
    CHECK(lib.position() == 10000 && lib.permille() == 1000 && !lib.finished());
    lib.setFinished(140);
    CHECK(lib.finished());
    lib.setPosition(9500, 150);
    CHECK(lib.finished());  // leafing back a little: still finished
    LibBook b;
    CHECK(lib.book(lib.find(key.c_str()), b) && b.status == LB_FINISHED);
    lib.setPosition(5000, 160);
    CHECK(!lib.finished() && lib.permille() == 500);  // reading it again
    CHECK(lib.book(lib.find(key.c_str()), b) && b.status == LB_READING && b.permille == 500 && b.lastRead == 160);

    // Bookmarks.
    CHECK(lib.addMark(700, "seven") && lib.addMark(100, "one") && lib.addMark(400, "four"));
    CHECK(lib.markCount() == 3 && lib.mark(0).offset == 100 && lib.mark(1).offset == 400 && lib.mark(2).offset == 700);
    CHECK(lib.addMark(400, "four again") && lib.markCount() == 3 && !strcmp(lib.mark(1).label, "four again"));
    CHECK(lib.markIn(0, 100) == -1 && lib.markIn(100, 101) == 0 && lib.markIn(101, 700) == 1 && lib.markIn(701, 9999) == -1);
    CHECK(lib.toggleMark(300, 500, "x") == 0 && lib.markCount() == 2 && lib.markIn(300, 500) == -1);  // was marked: off
    CHECK(lib.toggleMark(300, 500, "page") == 1 && lib.markCount() == 3 && lib.mark(1).offset == 300);
    CHECK(lib.addMark(310, "two on a page") && lib.toggleMark(300, 500, "x") == 0 && lib.markCount() == 2);
    lib.removeMark(5);
    lib.removeMark(-1);
    lib.removeMark(0);
    CHECK(lib.markCount() == 1 && lib.mark(0).offset == 700);
    // Labels: cut on a character, made printable.
    std::string euros;
    for (int i = 0; i < 30; i++) euros += "\xE2\x82\xAC";  // 90 bytes
    CHECK(lib.addMark(1, euros.c_str()) && strlen(lib.mark(0).label) == 78);
    CHECK(lib.addMark(2, (std::string(79, 'a') + "\xC3\xA9").c_str()) && lib.mark(1).label == std::string(79, 'a'));
    CHECK(lib.addMark(3, (std::string(78, 'a') + "\xC3\xA9").c_str()) && strlen(lib.mark(2).label) == 80);
    CHECK(lib.addMark(4, "  tab\there\nnew \x01line  ") && !strcmp(lib.mark(3).label, "tab here new line"));
    CHECK(lib.addMark(5, "bad \xFF\xC3 utf\xE2\x82") && !strcmp(lib.mark(4).label, "bad ?? utf??"));
    CHECK(lib.addMark(6, "\xF0\x9F\x93\x96 \xED\xA0\x80 \xC0\xAF") && !strcmp(lib.mark(5).label, "\xF0\x9F\x93\x96 ??? ??"));
    CHECK(lib.addMark(8, nullptr) && !strcmp(lib.mark(6).label, ""));
    CHECK(lib.addMark(20000, "past the end") && lib.mark(lib.markCount() - 1).offset == 10000);
    while (lib.markCount() < LIB_MAX_MARKS) CHECK(lib.addMark(1000 + lib.markCount(), "fill"));
    CHECK(!lib.addMark(9000, "one too many") && lib.markCount() == LIB_MAX_MARKS);
    CHECK(lib.toggleMark(9000, 9100, "x") == -1);
    CHECK(lib.addMark(700, "seven, renamed"));  // an existing one can still be relabelled
    lib.closeBook();
    CHECK(!lib.isOpen() && lib.markCount() == 0);
  }
  {
    BookLibrary lib(root.c_str());
    wire(lib);
    CHECK(lib.scanBegin());
    int begins = fake.begins;
    CHECK(lib.find(key.c_str()) == 0 && lib.lastRead() == 0);
    CHECK(lib.openBegin(lib.find(key.c_str()), 200) && lib.openStep() == BOOK_DONE && fake.begins == begins);  // at once
    CHECK(lib.position() == 5000 && lib.permille() == 500 && !lib.finished() && lib.flowSize() == 10000);
    CHECK(lib.markCount() == LIB_MAX_MARKS && lib.mark(0).offset == 1 && strlen(lib.mark(0).label) == 78);
    CHECK(!strcmp(lib.mark(3).label, "tab here new line"));
    int at = lib.markIn(700, 701);
    CHECK(at >= 0 && !strcmp(lib.mark(at).label, "seven, renamed"));
    CHECK(!strcmp(lib.openTitle(), "Book") && !strcmp(lib.openAuthor(), "Author") && lib.coverType() == 0 && !*lib.coverPath());
    CHECK(lib.flowPath() == cache + key + ".flow" && exists(lib.flowPath()));
  }
}

// Files cut short, torn, garbage: read as not there, and what can be saved is.
static void testTorn() {
  std::string root = fresh("torn"), cache = cacheOf(root);
  put(root + "/books/book.txt", "T:Book|A:Author|");
  put(root + "/books/two.txt", "T:Two|A:|");
  std::string key;
  {
    BookLibrary lib(root.c_str());
    wire(lib);
    scanAll(lib);
    fake.reset();
    CHECK(openTitle(lib, "Book", 100) == BOOK_DONE);
    key = lib.openKey();
    lib.setPosition(600, 100);
    lib.addMark(300, "a mark");
    lib.closeBook();
  }
  std::string pos = cache + key + ".pos", meta = cache + key + ".meta", idx = cache + "library.idx";
  std::string goodPos = slurp(pos), goodMeta = slurp(meta), goodIdx = slurp(idx);
  CHECK(goodPos.size() > 40 && goodMeta.size() > 40 && goodIdx.size() > 40);
  auto position = [&](int *marks = nullptr) {  // what a fresh start makes of the .pos on the card
    BookLibrary lib(root.c_str());
    wire(lib);
    lib.scanBegin();
    int p = openAll(lib, lib.find(key.c_str()), 0);
    CHECK(p == BOOK_DONE);
    if (marks) *marks = lib.markCount();
    return (long)lib.position();
  };
  int marks = 0;
  CHECK(position(&marks) == 600 && marks == 1);

  // The write stopped after the old file was removed: the new one waits beside.
  put(pos + ".tmp", goodPos);
  remove(pos.c_str());
  CHECK(position(&marks) == 600 && marks == 1 && exists(pos) && !exists(pos + ".tmp"));
  // ... before that: the old one stands, the half-written one is nothing.
  put(pos, goodPos);
  put(pos + ".tmp", goodPos.substr(0, goodPos.size() / 2));
  CHECK(position() == 600);
  // A torn old one and a whole new one.
  put(pos, goodPos.substr(0, 20));
  put(pos + ".tmp", goodPos);
  CHECK(position() == 600 && slurp(pos).size() >= goodPos.size());
  // Both torn: from the beginning.
  put(pos, "STICKYPOS 1\noffset 6");
  put(pos + ".tmp", "garbage");
  CHECK(position(&marks) == 0 && marks == 0);

  // Cut at every length, then bytes flipped: never a crash, never a made-up place.
  for (size_t n = 0; n < goodPos.size(); n++) {
    put(pos, goodPos.substr(0, n));
    remove((pos + ".tmp").c_str());
    BookLibrary lib(root.c_str());
    wire(lib);
    lib.scanBegin();
    CHECK(openAll(lib, lib.find(key.c_str()), 0) == BOOK_DONE && lib.position() == 0 && lib.markCount() == 0);
  }
  uint32_t seed = 12345;
  auto rnd = [&seed]() { return seed = seed * 1664525u + 1013904223u, seed >> 8; };
  for (int i = 0; i < 300; i++) {
    std::string bad = goodPos;
    bad[rnd() % bad.size()] ^= 1 << (rnd() % 8);
    put(pos, bad);
    BookLibrary lib(root.c_str());
    wire(lib);
    lib.scanBegin();
    CHECK(openAll(lib, lib.find(key.c_str()), 0) == BOOK_DONE);
    CHECK(lib.position() == 0 || lib.position() == 600);
  }
  // Whole by its checksum, wrong inside.
  const char *const badPos[] = {
      "STICKYPOS 1\noffset 600\n",                                       // no flow size
      "STICKYPOS 1\noffset 2000\nflow 1000\n",                           // beyond the end
      "STICKYPOS 1\noffset 6x0\nflow 1000\n",                            // not a number
      "STICKYPOS 1\noffset -1\nflow 1000\n",
      "STICKYPOS 1\noffset 99999999999\nflow 1000\n",                    // too big for the board
      "STICKYPOS 1\noffset 600\nflow 1000\npermille 1001\n",
      "STICKYPOS 1\noffset 600\nflow 1000\nfinished 2\n",
      "STICKYPOS 1\noffset 600\nflow 1000\nmark 5000\tbeyond\n",
      "STICKYPOS 1\noffset 600\nflow 1000\nmark nolabel\n",
      "STICKYPOS 2\noffset 600\nflow 1000\n",
      "offset 600\nflow 1000\n",
      "",
  };
  for (const char *b : badPos) {
    put(pos, sealed(b));
    CHECK(position() == 0);
  }
  std::string many = "STICKYPOS 1\noffset 600\nflow 1000\n";
  for (int i = 0; i < LIB_MAX_MARKS + 1; i++) many += "mark " + std::to_string(i) + "\tm\n";
  put(pos, sealed(many));
  CHECK(position() == 0);
  // A good one written by hand: marks out of order and twice come back in order, once; a long label is cut.
  put(pos, sealed("STICKYPOS 1\noffset 600\nflow 1000\nstamp 5\nfuture thing\nmark 900\tnine\nmark 100\t" + std::string(300, 'L') +
                  "\nmark 900\tnine again\n"));
  {
    BookLibrary lib(root.c_str());
    wire(lib);
    lib.scanBegin();
    CHECK(openAll(lib, lib.find(key.c_str()), 0) == BOOK_DONE && lib.position() == 600 && lib.markCount() == 2);
    CHECK(lib.mark(0).offset == 100 && strlen(lib.mark(0).label) == LIB_LABEL_MAX && !strcmp(lib.mark(1).label, "nine"));
  }
  // A place kept for a flow of another size (the book was converted again, differently): from the top.
  put(pos, sealed("STICKYPOS 1\noffset 600\nflow 2000\nfinished 1\nmark 100\told\n"));
  CHECK(position(&marks) == 0 && marks == 0);

  // The index: cut at many lengths, garbage, huge, wrong inside. Every time the list comes back whole.
  auto rescan = [&](const std::string &index, bool wantProbes) {
    put(idx, index);
    remove((idx + ".tmp").c_str());
    BookLibrary lib(root.c_str());
    wire(lib);
    probes = 0;
    scanAll(lib);
    CHECK(titles(lib) == "Book,Two");
    CHECK(wantProbes ? probes > 0 : probes == 0);
    LibBook b;
    CHECK(lib.book(0, b) && b.status == LB_READING && !strcmp(b.author, "Author"));  // the place came from the .pos
    CHECK(slurp(idx).size() > 40);
  };
  put(pos, goodPos);
  rescan(goodIdx, false);
  goodIdx = slurp(idx);
  for (size_t n = 0; n < goodIdx.size(); n += 7) rescan(goodIdx.substr(0, n), true);
  for (int i = 0; i < 200; i++) {
    std::string bad = goodIdx;
    bad[rnd() % bad.size()] ^= 1 << (rnd() % 8);
    rescan(bad, true);
  }
  std::string noise;
  for (int i = 0; i < 5000; i++) noise += (char)rnd();
  rescan(noise, true);
  rescan(std::string(10 * 1024 * 1024, 'A'), true);  // not ours: not even read
  std::string body = goodIdx.substr(0, goodIdx.size() - 13);
  rescan(sealed(body), false);
  size_t firstLine = body.find('\n', body.find("top "));
  std::string head = body.substr(0, firstLine + 1), lines = body.substr(firstLine + 1);
  size_t l1 = lines.find('\n');
  std::string line1 = lines.substr(0, l1 + 1);
  rescan(sealed(head + lines + line1), true);                             // a book twice
  rescan(sealed(head + line1.substr(0, line1.size() - 1) + "\textra\n"), true);  // a field too many
  rescan(sealed(head + "0123\t1\t1\t5\t0\t0\t0\tbooks/x.txt\tX\t\n"), true);     // a short key
  std::string l = line1;
  l.replace(l.find('\t') + 1, 0, "x");
  rescan(sealed(head + l), true);                                         // a size that isn't a number
  l = line1;
  l[0] = l[0] == '0' ? '1' : '0';
  rescan(sealed(head + l), true);                                         // a key that isn't this line's
  rescan(sealed("STICKYLIB 1 999\ntop 0\n" + lines), true);               // another flow version
  rescan(sealed("STICKYLIB 1 " + std::to_string(FLOW_VERSION) + "\ntop x\n" + lines), true);
  rescan(sealed(head + std::string(5000, 'y') + "\n"), true);             // a line far too long
  std::string lots = head;
  for (int i = 0; i < LIB_MAX_BOOKS + 1; i++) lots += line1;
  rescan(sealed(lots), true);
  // The index's own write cut short.
  put(idx + ".tmp", goodIdx);
  remove(idx.c_str());
  {
    BookLibrary lib(root.c_str());
    wire(lib);
    probes = 0;
    scanAll(lib);
    CHECK(probes == 0 && titles(lib) == "Book,Two" && exists(idx) && !exists(idx + ".tmp"));
  }

  // The .meta: torn or wrong, the book is converted again; a whole one left beside is taken.
  auto begins = [&]() {
    BookLibrary lib(root.c_str());
    wire(lib);
    lib.scanBegin();
    int before = fake.begins;
    CHECK(openAll(lib, lib.find(key.c_str()), 0) == BOOK_DONE);
    CHECK(lib.position() == 600);  // the same flow size: the place holds
    return fake.begins - before;
  };
  CHECK(begins() == 0);
  for (size_t n = 0; n < goodMeta.size(); n += 3) {
    put(meta, goodMeta.substr(0, n));
    CHECK(begins() == 1);
    CHECK(slurp(meta) == goodMeta);
  }
  for (int i = 0; i < 100; i++) {
    std::string bad = goodMeta;
    bad[rnd() % bad.size()] ^= 1 << (rnd() % 8);
    put(meta, bad);
    CHECK(begins() == 1);
  }
  std::string mbody = goodMeta.substr(0, goodMeta.size() - 13), m2 = mbody;
  put(meta, sealed(mbody));
  CHECK(begins() == 0);
  m2.replace(m2.find("flowver ") + 8, 1, "77");  // made by another firmware
  put(meta, sealed(m2));
  CHECK(begins() == 1);
  m2 = mbody;
  m2.replace(m2.find("path ") + 5, 5, "other");  // another book's
  put(meta, sealed(m2));
  CHECK(begins() == 1);
  m2 = mbody;
  m2.replace(m2.find("flowsize ") + 9, 1, "9");  // not the flow that is there
  put(meta, sealed(m2));
  CHECK(begins() == 1);
  put(meta, sealed("STICKYMETA 1\nflowver 1\n"));
  CHECK(begins() == 1);
  remove((cache + key + ".flow").c_str());  // the flow is gone
  CHECK(begins() == 1);
  put(meta + ".tmp", goodMeta);
  remove(meta.c_str());
  CHECK(begins() == 0 && exists(meta) && !exists(meta + ".tmp"));
  // Left-over .tmp files of a book that is there: the scan settles them.
  put(meta + ".tmp", "half");
  put(pos + ".tmp", goodPos);
  remove(pos.c_str());
  {
    BookLibrary lib(root.c_str());
    wire(lib);
    scanAll(lib);
    CHECK(!exists(meta + ".tmp") && slurp(meta) == goodMeta && slurp(pos) == goodPos && !exists(pos + ".tmp"));
  }
}

static void testOpen() {
  std::string root = fresh("open"), cache = cacheOf(root);
  put(root + "/books/book.txt", "T:Probe Title|A:Probe Author|");
  put(root + "/books/drm.txt", "T:Locked|A:|");
  put(root + "/books/old.txt", "T:Old|A:|");
  put(root + "/books/older.txt", "T:Older|A:|");
  put(root + "/books/done.txt", "T:Done|A:|");
  BookLibrary lib(root.c_str());
  wire(lib);
  scanAll(lib);
  fake.reset();
  CHECK(!lib.openBegin(-1, 0) && lib.openError() == BE_OPEN && !lib.openBegin(99, 0));
  CHECK(lib.openStep() < 0);

  // Steps, then ready; the converter's title is the better one.
  fake.steps = 5, fake.title = "  Converter\tTitle ", fake.author = "Converter Author", fake.cover = 'j', fake.flowBytes = 4321;
  std::string key = keyOf(lib, "Probe Title");
  CHECK(lib.openBegin(byTitle(lib, "Probe Title"), 50) && !lib.isOpen());
  CHECK(!exists(cache + key + ".meta"));
  int last = -1, n = 0, p;
  while ((p = lib.openStep()) >= 0 && p < BOOK_DONE) {
    CHECK(p > last && p <= 999 && !lib.isOpen() && !exists(cache + key + ".meta"));
    last = p, n++;
  }
  CHECK(p == BOOK_DONE && n == 4 && lib.isOpen() && lib.openError() == BE_OK && fake.alive == 0);
  CHECK(lib.openStep() == BOOK_DONE);
  CHECK(!strcmp(lib.openTitle(), "Converter Title") && !strcmp(lib.openAuthor(), "Converter Author") && lib.openKey() == key);
  CHECK(lib.coverType() == 'j' && lib.coverPath() == cache + key + ".cover" && slurp(lib.coverPath()) == "picture");
  CHECK(lib.flowSize() == 4321 && slurp(lib.flowPath()).size() == 4321 && exists(std::string(lib.flowPath()) + ".toc"));
  std::string meta = slurp(cache + key + ".meta");
  CHECK(meta.find("title Converter Title\n") != std::string::npos && meta.find("path books/book.txt\n") != std::string::npos);
  CHECK(meta.find("cover j\n") != std::string::npos && meta.find("lang en\n") != std::string::npos);
  LibBook b;
  CHECK(lib.book(0, b) && !strcmp(b.title, "Converter Title") && b.status == LB_READING && b.lastRead == 50);
  CHECK(slurp(cache + "library.idx").find("Converter Title\tConverter Author\t0\n") != std::string::npos);
  lib.setPosition(1000, 60);
  // Another book is opened: the first is saved and closed.
  fake.reset();
  int begins = fake.begins;
  CHECK(openTitle(lib, "Old", 70) == BOOK_DONE && fake.begins == begins + 1);
  CHECK(openTitle(lib, "Converter Title", 80) == BOOK_DONE && fake.begins == begins + 1 && lib.position() == 1000);
  CHECK(lib.coverType() == 'j');
  lib.closeBook();

  // Each way of failing: nothing is kept, the reason is given.
  std::string kDrm = keyOf(lib, "Locked");
  struct { BookErr err; bool atBegin, flagged; } const cases[] = {
      {BE_DRM, true, true}, {BE_FORMAT, true, true},  {BE_DAMAGED, false, true}, {BE_NO_TEXT, false, true},
      {BE_ENCRYPTED, true, true}, {BE_MEMORY, false, false}, {BE_OPEN, false, false},
  };
  int nth = 0;
  for (auto &c : cases) {
    std::string name = "Case " + std::to_string(nth++), file = root + "/books/" + name + ".txt";
    put(file, "T:" + name + "|A:|");
    scanAll(lib);
    std::string k = keyOf(lib, name.c_str());
    fake.reset();
    if (c.atBegin) fake.beginErr = c.err;
    else fake.failStep = 2, fake.failErr = c.err;
    int r = openAll(lib, lib.find(k.c_str()), 90);
    CHECK(r == -(int)c.err && lib.openError() == c.err && !lib.isOpen() && fake.alive == 0);
    CHECK(lib.openStep() == -(int)c.err);
    for (const char *s : {".flow", ".flow.toc", ".cover", ".meta", ".pos"}) CHECK(!exists(cache + k + s));
    CHECK(lib.book(lib.find(k.c_str()), b) && b.error == c.flagged && b.status == LB_NEW);
    fake.reset();  // and it opens once it can be read
    CHECK(openAll(lib, lib.find(k.c_str()), 0) == BOOK_DONE && lib.book(lib.find(k.c_str()), b) && !b.error);
    CHECK(lib.deleteBook(lib.find(k.c_str())) && !exists(file));
  }

  // Stopped half way.
  fake.reset();
  fake.steps = 10;
  CHECK(lib.openBegin(lib.find(kDrm.c_str()), 0) && lib.openStep() > 0 && lib.openStep() > 0);
  CHECK(exists(cache + kDrm + ".flow"));
  lib.openAbort();
  CHECK(fake.aborts == 1 && fake.alive == 0 && !lib.isOpen() && lib.openStep() < 0);
  for (const char *s : {".flow", ".flow.toc", ".cover", ".meta", ".pos"}) CHECK(!exists(cache + kDrm + s));
  lib.openAbort();  // twice is nothing
  // Opening another book in the middle of a conversion stops it too.
  CHECK(lib.openBegin(lib.find(kDrm.c_str()), 0) && lib.openStep() > 0);
  CHECK(openTitle(lib, "Old", 100) == BOOK_DONE && fake.aborts == 2 && !exists(cache + kDrm + ".flow"));
  lib.closeBook();

  // The card is full. "Done" is finished, "Older" and "Old" were read in that order.
  fake.reset();
  fake.flowBytes = 400 * 1024;
  CHECK(openTitle(lib, "Done", 200) == BOOK_DONE);
  lib.setFinished(200);
  lib.addMark(5, "kept");
  CHECK(openTitle(lib, "Older", 300) == BOOK_DONE);
  lib.setPosition(1234, 300);
  remove((cache + keyOf(lib, "Old") + ".meta").c_str());
  CHECK(openTitle(lib, "Old", 400) == BOOK_DONE);
  lib.closeBook();
  std::string kDone = keyOf(lib, "Done"), kOlder = keyOf(lib, "Older"), kOld = keyOf(lib, "Old");
  fake.reset();
  fake.writeFails = 1;
  begins = fake.begins;
  CHECK(openAll(lib, lib.find(kDrm.c_str()), 500) == BOOK_DONE && fake.begins == begins + 2);  // made room, once more
  CHECK(!exists(cache + kDone + ".flow") && !exists(cache + kDone + ".meta") && !exists(cache + kDone + ".flow.toc"));
  CHECK(exists(cache + kDone + ".pos"));                                         // the place stays
  CHECK(exists(cache + kOlder + ".flow") && exists(cache + kOld + ".flow"));     // 400 KB was enough
  CHECK(exists(cache + key + ".flow"));
  lib.closeBook();
  // Nothing finished is left to remove: the one not read for the longest goes.
  remove((cache + kDrm + ".meta").c_str());
  fake.reset();
  fake.writeFails = 1;
  CHECK(openAll(lib, lib.find(kDrm.c_str()), 600) == BOOK_DONE);
  CHECK(!exists(cache + key + ".flow") && exists(cache + key + ".pos") && exists(cache + key + ".cover") == false);
  CHECK(!exists(cache + kOlder + ".flow") && exists(cache + kOlder + ".pos"));  // the first wasn't room enough: the next
  CHECK(exists(cache + kOld + ".flow"));
  lib.closeBook();
  // A card that takes nothing: one more try, then the reason.
  remove((cache + kDrm + ".meta").c_str());
  fake.reset();
  fake.writeFails = 99;
  begins = fake.begins;
  CHECK(openAll(lib, lib.find(kDrm.c_str()), 700) == -(int)BE_WRITE && fake.begins == begins + 2 && lib.openError() == BE_WRITE);
  CHECK(!exists(cache + kDrm + ".meta") && !exists(cache + kDrm + ".flow") && fake.alive == 0);
  CHECK(lib.state() == LS_OK);  // the card is still there
  // Nothing to remove at all (the others' copies are gone by now): no second try.
  fake.reset();
  fake.writeFails = 99;
  begins = fake.begins;
  CHECK(openAll(lib, lib.find(kDrm.c_str()), 700) == -(int)BE_WRITE && fake.begins == begins + 1);
  // It fails on starting, too.
  fake.reset();
  CHECK(openTitle(lib, "Old", 710) == BOOK_DONE);
  lib.closeBook();
  fake.beginErr = BE_WRITE;
  begins = fake.begins;
  CHECK(openAll(lib, lib.find(kDrm.c_str()), 720) == -(int)BE_WRITE && fake.begins == begins + 2);
  CHECK(!exists(cache + kOld + ".flow") && exists(cache + kOld + ".pos"));
  // The books put aside: converted again, they are where they were, bookmarks and all.
  fake.reset();
  fake.flowBytes = 400 * 1024;
  begins = fake.begins;
  CHECK(openAll(lib, lib.find(kDone.c_str()), 800) == BOOK_DONE && fake.begins == begins + 1);
  CHECK(lib.finished() && lib.markCount() == 1 && !strcmp(lib.mark(0).label, "kept"));
  CHECK(openAll(lib, lib.find(kOlder.c_str()), 810) == BOOK_DONE && lib.position() == 1234);
  // ... unless the new flow isn't the old one.
  lib.closeBook();
  remove((cache + kOlder + ".meta").c_str());
  fake.flowBytes = 5000;
  CHECK(openAll(lib, lib.find(kOlder.c_str()), 820) == BOOK_DONE && lib.position() == 0 && lib.flowSize() == 5000);
  lib.closeBook();

  // Removing a book: its file and all that was kept of it, nothing else.
  put(cache + "notes.txt", "decoy");
  put(root + "/books/keep.md", "T:Keep|A:|");
  int before = lib.count();
  CHECK(!lib.deleteBook(-1) && !lib.deleteBook(before));
  CHECK(openAll(lib, lib.find(kDone.c_str()), 900) == BOOK_DONE);
  CHECK(lib.deleteBook(lib.find(kDone.c_str())));  // even the open one
  CHECK(!lib.isOpen() && lib.count() == before - 1 && lib.find(kDone.c_str()) == -1);
  CHECK(!exists(root + "/books/done.txt"));
  for (const char *s : {".flow", ".flow.toc", ".cover", ".meta", ".pos"}) CHECK(!exists(cache + kDone + s));
  CHECK(slurp(cache + "notes.txt") == "decoy" && exists(root + "/books/keep.md") && exists(cache + kOlder + ".pos"));
  CHECK(slurp(cache + "library.idx").find("done.txt") == std::string::npos);
  BookLibrary again(root.c_str());
  wire(again);
  probes = 0;
  scanAll(again);
  CHECK(again.count() == before && probes == 1 && byTitle(again, "Keep") >= 0 && byTitle(again, "Done") < 0);
}

// The card is pulled in the middle of things, and put back.
static void testCardGone() {
  std::string root = fresh("gone"), away = root + "-away";
  rmrf(away);
  for (const char *n : {"a", "b", "c", "d"}) put(root + "/books/" + n + ".txt", std::string("T:") + n + n + "|A:|");
  BookLibrary lib(root.c_str());
  wire(lib);
  CHECK(lib.scanBegin() && lib.scanStep());
  CHECK(rename(root.c_str(), away.c_str()) == 0);
  CHECK(!lib.scanStep() && lib.state() == LS_NO_CARD && !lib.scanning());
  CHECK(!lib.openBegin(0, 0) && lib.openError() == BE_OPEN);
  CHECK(!lib.scanBegin() && lib.state() == LS_NO_CARD && lib.count() == 0);
  CHECK(rename(away.c_str(), root.c_str()) == 0);
  scanAll(lib);
  CHECK(lib.state() == LS_OK && titles(lib) == "aa,bb,cc,dd");
  // While a book is open.
  fake.reset();
  CHECK(openTitle(lib, "bb", 10) == BOOK_DONE);
  lib.setPosition(100, 20);
  CHECK(rename(root.c_str(), away.c_str()) == 0);
  CHECK(!lib.saveState() && lib.state() == LS_NO_CARD && !lib.isOpen());
  CHECK(!lib.sync() && !lib.deleteBook(0));
  lib.closeBook();
  CHECK(rename(away.c_str(), root.c_str()) == 0);
  scanAll(lib);
  CHECK(lib.state() == LS_OK && lib.lastRead() == 0);
  // While converting.
  fake.steps = 10;
  CHECK(lib.openBegin(byTitle(lib, "cc"), 30) && lib.openStep() > 0);
  CHECK(rename(root.c_str(), away.c_str()) == 0);
  fake.failStep = 3, fake.failErr = BE_OPEN;
  int p;
  while ((p = lib.openStep()) >= 0 && p < BOOK_DONE) {}
  CHECK(p == -(int)BE_OPEN && lib.state() == LS_NO_CARD && fake.alive == 0);
  CHECK(rename(away.c_str(), root.c_str()) == 0);
  // A book that goes between the listing and its turn: dropped from the list.
  CHECK(lib.scanBegin());
  remove((root + "/books/a.txt").c_str());
  while (lib.scanStep()) {}
  CHECK(lib.state() == LS_OK && titles(lib) == "bb,cc,dd");
  // A card that can't be written (the cache can't be made): the list works, opening says why.
  std::string ro = fresh("gone-readonly");
  put(ro + "/books/a.txt", "T:Read Only|A:|");
  put(ro + "/.sticky", "a file where the folder should be");
  BookLibrary lib2(ro.c_str());
  wire(lib2);
  scanAll(lib2);
  CHECK(lib2.state() == LS_OK && titles(lib2) == "Read Only");
  CHECK(openAll(lib2, 0, 0) == -(int)BE_WRITE && lib2.state() == LS_OK);
  CHECK(slurp(ro + "/.sticky") == "a file where the folder should be");
}

// Reading statistics: a book's, and the card's.
static void testStats() {
  std::string root = fresh("stats"), cache = cacheOf(root);
  put(root + "/books/a.txt", "T:Alpha|A:|");
  put(root + "/books/b.txt", "T:Beta|A:|");
  put(root + "/books/c.txt", "T:Gamma|A:|");
  std::string kA;
  {
    BookLibrary lib(root.c_str());
    wire(lib);
    scanAll(lib);
    fake.reset();
    fake.flowBytes = 50000;
    lib.addReading(10, 1, 1, 5, 1);  // nothing open: nothing counted
    CHECK(openTitle(lib, "Alpha", 1000) == BOOK_DONE);
    kA = lib.openKey();
    const LibBookStats &st = lib.bookStats();
    CHECK(st.secs == 0 && st.sessions == 0 && st.started == 0);
    lib.addReading(60, 2, 2000, 1060, 10);
    CHECK(st.secs == 60 && st.pages == 2 && st.bytes == 2000 && st.sessions == 1 && st.started == 1060 && st.lastAt == 1060);
    CHECK(st.paceBytesPerMin() == 0);  // too little to go by
    lib.addReading(0, 0, 0, 1100, 10);  // nothing happened: nothing changes
    CHECK(st.lastAt == 1060);
    lib.addReading(200, 5, 10000, 1300, 10);
    CHECK(st.sessions == 1 && st.secs == 260 && st.pages == 7 && st.paceBytesPerMin() == 12000u * 60 / 260);
    lib.addReading(60, 1, 100, 1300 + LIB_SESSION_GAP, 10);  // a pause of exactly the gap: the same session
    CHECK(st.sessions == 1);
    lib.addReading(60, 1, 100, 1300 + 2 * LIB_SESSION_GAP + 2, 11);  // longer: a new one
    CHECK(st.sessions == 2 && st.started == 1060 && st.secs == 380);
    lib.addReading(100000, 0, 0, 9000, 11);  // more than a day at once: a day
    CHECK(st.secs == 380 + 86400);
    lib.setFinished(9100);
    CHECK(st.finishedAt == 9100);
    LibStats ls = lib.libraryStats(11);
    CHECK(ls.booksFinished == 1 && ls.booksReading == 0 && ls.totalSecs == 380 + 86400 && ls.totalPages == 9);
    lib.setPosition(100, 9200);  // read again from the start
    lib.setFinished(9300);       // and finished again: still one book
    CHECK(st.finishedAt == 9300 && lib.libraryStats(11).booksFinished == 1);
    lib.closeBook();
    LibBook b;
    CHECK(lib.book(lib.find(kA.c_str()), b) && b.secsRead == 380 + 86400);
    CHECK(exists(cache + "stats.txt"));
  }
  {  // all of it again after a restart, the list's seconds from the index alone
    BookLibrary lib(root.c_str());
    wire(lib);
    probes = 0;
    CHECK(lib.scanBegin());
    LibBook b;
    CHECK(lib.book(lib.find(kA.c_str()), b) && b.secsRead == 380 + 86400 && probes == 0);
    LibStats ls = lib.libraryStats(11);
    CHECK(ls.booksFinished == 1 && ls.totalSecs == 380 + 86400 && ls.totalPages == 9);
    CHECK(openAll(lib, lib.find(kA.c_str()), 9400) == BOOK_DONE);
    const LibBookStats &st = lib.bookStats();
    CHECK(st.secs == 380 + 86400 && st.sessions == 3 && st.started == 1060 && st.finishedAt == 9300 && st.bytes == 12200);
    lib.addReading(30, 1, 10, 9450, 11);  // within the gap of the last reading: the same session
    CHECK(st.sessions == 3);
    lib.closeBook();

    // No clock: the seconds add up, the dates stay as they were; each opening is a session.
    fake.reset();
    CHECK(openTitle(lib, "Beta", 0) == BOOK_DONE);
    lib.addReading(100, 3, 300, 0, -1);
    lib.addReading(100, 3, 300, 0, -1);
    CHECK(lib.bookStats().secs == 200 && lib.bookStats().sessions == 1 && lib.bookStats().started == 0 && lib.bookStats().lastAt == 0);
    lib.setFinished(0);
    CHECK(lib.bookStats().finishedAt == 0 && lib.libraryStats(-1).booksFinished == 2);
    lib.closeBook();
    CHECK(openTitle(lib, "Beta", 0) == BOOK_DONE);
    lib.addReading(10, 0, 0, 0, -1);
    CHECK(lib.bookStats().sessions == 2);
    lib.addReading(10, 0, 0, 20000, -1);  // the clock is back
    CHECK(lib.bookStats().started == 20000 && lib.bookStats().lastAt == 20000 && lib.bookStats().sessions == 2);
    lib.closeBook();
    LibStats ls2 = lib.libraryStats(-1);
    CHECK(ls2.totalSecs == 380 + 86400 + 30 + 220 && ls2.daysRead == 2 && ls2.secsToday == 0 && ls2.currentStreakDays == 0);
    // A book removed: its time and its finishing still count.
    CHECK(lib.deleteBook(lib.find(kA.c_str())));
    ls2 = lib.libraryStats(11);
    CHECK(ls2.totalSecs == 380 + 86400 + 30 + 220 && ls2.booksFinished == 2 && ls2.totalPages == 9 + 1 + 6);
  }
  {  // stats.txt waits for 5 minutes of reading, or a sync
    std::string r2 = fresh("stats-save"), c2 = cacheOf(r2);
    put(r2 + "/books/a.txt", "T:A|A:|");
    BookLibrary lib(r2.c_str());
    wire(lib);
    scanAll(lib);
    fake.reset();
    CHECK(openAll(lib, 0, 100) == BOOK_DONE);
    lib.addReading(60, 1, 10, 160, 1);
    CHECK(lib.saveState() && !exists(c2 + "stats.txt"));
    lib.addReading(240, 1, 10, 400, 1);
    CHECK(lib.saveState() && exists(c2 + "stats.txt"));
    ino_t in = inode(c2 + "stats.txt");
    lib.addReading(10, 1, 10, 410, 1);
    CHECK(lib.saveState() && inode(c2 + "stats.txt") == in);
    CHECK(lib.sync() && inode(c2 + "stats.txt") != in);
  }
}

static void testDays() {
  std::string root = fresh("days"), cache = cacheOf(root);
  put(root + "/books/a.txt", "T:A|A:|");
  BookLibrary lib(root.c_str());
  wire(lib);
  scanAll(lib);
  fake.reset();
  CHECK(openAll(lib, 0, 1) == BOOK_DONE);
  for (int d : {100, 101, 102, 105, 106}) lib.addReading(120, 2, 10, 0, d);
  lib.addReading(30, 1, 10, 0, 104);  // half a minute: a day read, but not for a streak
  lib.addReading(50, 1, 10, 0, -1);   // the day unknown: in the totals only
  LibStats s = lib.libraryStats(106);
  CHECK(s.totalSecs == 120 * 5 + 30 + 50 && s.totalPages == 12);
  CHECK(s.secsToday == 120 && s.secsThisWeek == 120 * 5 + 30 && s.daysRead == 5);
  CHECK(s.currentStreakDays == 2 && s.longestStreakDays == 3);
  const uint32_t want[7] = {120, 120, 120, 0, 30, 120, 120};
  for (int k = 0; k < 7; k++) CHECK(s.last7[k] == want[k]);
  s = lib.libraryStats(107);  // today not read yet: the streak holds
  CHECK(s.secsToday == 0 && s.currentStreakDays == 2 && s.last7[5] == 120 && s.secsThisWeek == 120 * 4 + 30);
  s = lib.libraryStats(108);
  CHECK(s.currentStreakDays == 0 && s.longestStreakDays == 3);
  s = lib.libraryStats(-1);
  CHECK(s.secsToday == 0 && s.secsThisWeek == 0 && s.currentStreakDays == 0 && s.last7[6] == 0 && s.daysRead == 5);
  lib.addReading(20, 0, 0, 0, 103);  // late news of an older day: put in its place
  lib.addReading(50, 0, 0, 0, 103);
  s = lib.libraryStats(106);
  CHECK(s.daysRead == 6 && s.longestStreakDays == 4 && s.last7[3] == 70);
  lib.closeBook();
  std::string saved = slurp(cache + "stats.txt");
  CHECK(saved.find("day 103 70 0\n") != std::string::npos);
  {  // after a restart
    BookLibrary again(root.c_str());
    wire(again);
    again.scanBegin();
    LibStats t = again.libraryStats(106);
    CHECK(t.daysRead == 6 && t.longestStreakDays == 4 && t.currentStreakDays == 2 && t.totalSecs == s.totalSecs && t.secsThisWeek == s.secsThisWeek);
  }
  // The log keeps LIB_STATS_DAYS days; a day far older than the newest is dropped.
  CHECK(openAll(lib, 0, 2) == BOOK_DONE);
  for (int d = 0; d < 450; d++) lib.addReading(60, 0, 0, 0, 1000 + d);
  s = lib.libraryStats(1449);
  CHECK(s.daysRead == (uint32_t)LIB_STATS_DAYS && s.longestStreakDays == (uint32_t)LIB_STATS_DAYS && s.currentStreakDays == (uint32_t)LIB_STATS_DAYS);
  lib.addReading(60, 0, 0, 0, 500);
  lib.addReading(60, 0, 0, 0, 2000000);  // nonsense
  CHECK(lib.libraryStats(1449).daysRead == (uint32_t)LIB_STATS_DAYS);
  lib.closeBook();
  std::string full = slurp(cache + "stats.txt");
  CHECK(full.find("day 1049 ") == std::string::npos && full.find("day 1050 ") != std::string::npos);

  // stats.txt torn, cut, flipped, wrong inside: read as absent (or the true one), never a crash.
  std::string good = slurp(cache + "stats.txt");
  uint32_t total = s.totalSecs + 120;
  auto load = [&](const std::string &data) {
    put(cache + "stats.txt", data);
    remove((cache + "stats.txt.tmp").c_str());
    BookLibrary l(root.c_str());
    wire(l);
    l.scanBegin();
    return l.libraryStats(1449);
  };
  CHECK(load(good).totalSecs == total && load(good).daysRead == (uint32_t)LIB_STATS_DAYS);
  for (size_t n = 0; n < good.size(); n += 97) {
    LibStats t = load(good.substr(0, n));
    CHECK(t.daysRead == 0 && t.totalPages == 0);
    CHECK(t.totalSecs > 0);  // what the list knows of the book
  }
  uint32_t seed = 777;
  for (int i = 0; i < 200; i++) {
    std::string bad = good;
    seed = seed * 1664525u + 1013904223u;
    bad[(seed >> 8) % bad.size()] ^= 1 << (seed % 8);
    LibStats t = load(bad);
    CHECK(t.daysRead == 0 || t.daysRead == (uint32_t)LIB_STATS_DAYS);
  }
  const char *const wrong[] = {
      "STICKYSTATS 1\nsecs 10\nday 5 60 1\nday 5 60 1\n",  // a day twice
      "STICKYSTATS 1\nsecs 10\nday 6 60 1\nday 5 60 1\n",  // out of order
      "STICKYSTATS 1\nsecs x\n",
      "STICKYSTATS 1\nday 5 60\n",
      "STICKYSTATS 1\nday -5 60 1\n",
      "STICKYSTATS 1\nday 99999999 60 1\n",
      "STICKYSTATS 2\nsecs 10\n",
  };
  for (const char *w : wrong) {
    LibStats t = load(sealed(w));
    CHECK(t.daysRead == 0 && t.totalPages == 0);
  }
  std::string many = "STICKYSTATS 1\n";
  for (int d = 0; d <= LIB_STATS_DAYS; d++) many += "day " + std::to_string(d) + " 60 0\n";
  CHECK(load(sealed(many)).daysRead == 0);
  CHECK(load(sealed("STICKYSTATS 1\nfinished 7\nsecs 5\npages 9\nnew thing\nday 3 61 2\n")).booksFinished == 7);
  CHECK(load(sealed("STICKYSTATS 1\nfinished 7\nsecs 5\npages 9\nday 3 61 2\n")).totalPages == 9);
  // A write cut short: the new file beside, the old one gone; the scan's tidying keeps and settles it.
  put(cache + "stats.txt.tmp", good);
  remove((cache + "stats.txt").c_str());
  {
    BookLibrary l(root.c_str());
    wire(l);
    scanAll(l);
    CHECK(slurp(cache + "stats.txt") == good && !exists(cache + "stats.txt.tmp"));
    CHECK(l.libraryStats(1449).daysRead == (uint32_t)LIB_STATS_DAYS);
  }
  CHECK(bookLibLocalDay(0, 3600) == -1 && bookLibLocalDay(86400 * 20000 + 3599, 0) == 20000);
  CHECK(bookLibLocalDay(86400 * 20000 + 82800, 3600) == 20001 && bookLibLocalDay(86400 * 20000 + 3000, -3600) == 19999);
}

// Files written by the first version of the library still load.
static void testOldFormats() {
  std::string root = fresh("old"), cache = cacheOf(root);
  put(root + "/books/a.txt", "T:Old Book|A:Someone|", 1700000000);
  std::string key;
  {
    BookLibrary lib(root.c_str());
    wire(lib);
    scanAll(lib);
    LibBook b;
    CHECK(lib.book(0, b));
    key = b.key;
  }
  struct stat st;
  stat((root + "/books/a.txt").c_str(), &st);
  char line[300];
  snprintf(line, sizeof line, "%s\t%lld\t%lld\t5\t1\t400\t77\tbooks/a.txt\tOld Book\tSomeone\n", key.c_str(), (long long)st.st_size,
           (long long)st.st_mtime);
  put(cache + "library.idx", sealed(std::string("STICKYLIB 1 ") + std::to_string(FLOW_VERSION) + "\ntop 77\n" + line));
  put(cache + key + ".flow", std::string(1000, 'x'));
  put(cache + key + ".flow.toc", "");
  put(cache + key + ".meta", sealed("STICKYMETA 1\nflowver " + std::to_string(FLOW_VERSION) +
                                    "\nflowsize 1000\nformat 5\ncover -\ntitle Old Book\nauthor Someone\nlang en\npath books/a.txt\n"));
  put(cache + key + ".pos", sealed("STICKYPOS 1\noffset 400\nflow 1000\npermille 400\nstamp 77\nfinished 0\nmark 10\tm\n"));
  BookLibrary lib(root.c_str());
  wire(lib);
  probes = 0;
  CHECK(lib.scanBegin());
  LibBook b;
  CHECK(lib.book(0, b) && b.status == LB_READING && b.permille == 400 && b.lastRead == 77 && b.secsRead == 0 && b.probed);
  while (lib.scanStep()) {}
  CHECK(probes == 0);
  LibInfo info;
  CHECK(lib.bookInfo(0, info) && info.converted && info.description.empty() && info.position == 400 && info.marks == 1 &&
        info.stats.secs == 0 && info.stats.sessions == 0);
  int begins = fake.begins;
  CHECK(openAll(lib, 0, 100) == BOOK_DONE && fake.begins == begins && lib.position() == 400 && lib.markCount() == 1);
  CHECK(lib.bookStats().secs == 0);
  lib.addReading(30, 1, 10, 130, 1);
  lib.closeBook();
  CHECK(slurp(cache + "library.idx").compare(0, 11, "STICKYLIB 2") == 0);
  CHECK(slurp(cache + key + ".pos").find("secs 30\n") != std::string::npos);
}

static void testInfo() {
  std::string root = fresh("info"), cache = cacheOf(root);
  put(root + "/books/plain.txt", "T:Plain|A:Writer|D:First para.\n\n  Second   para \x01 here.\nbad \xFF byte");
  put(root + "/books/none.txt", "T:No Desc|A:|");
  BookLibrary lib(root.c_str());
  wire(lib);
  scanAll(lib);
  LibInfo in;
  CHECK(!lib.bookInfo(-1, in) && !lib.bookInfo(lib.count(), in));
  int i = byTitle(lib, "Plain");
  CHECK(lib.bookInfo(i, in));
  CHECK(!in.converted && in.flowPath.empty() && in.coverPath.empty() && in.coverType == 0 && in.flowSize == 0);
  CHECK(in.title == "Plain" && in.author == "Writer" && in.path == "books/plain.txt" && in.format == BF_TXT && in.status == LB_NEW);
  CHECK(in.description == "First para.\nSecond para here.\nbad ? byte");
  CHECK(in.fileSize == slurp(root + "/books/plain.txt").size() && strlen(in.key) == 16 && in.position == 0 && in.marks == 0);
  CHECK(lib.bookInfo(byTitle(lib, "No Desc"), in) && in.description.empty());

  // Converted, with a description that tries to break the line format.
  fake.reset();
  fake.flowBytes = 8000, fake.cover = 'p';
  std::string desc = "Line one\nback\\slash and a \\n that is text\n\n\nend \\\ndesc \xC3\xA9t\xC3\xA9 \xF0\x9F\x93\x96 \xC3\x28 \xED\xA0\x80\nSTICKYMETA 1\nflowsize 5";
  fake.description = desc;
  CHECK(openTitle(lib, "Plain", 500) == BOOK_DONE);
  std::string key = lib.openKey();
  lib.setPosition(2000, 510);
  lib.addMark(100, "a");
  lib.addMark(3000, "b");
  lib.addReading(240, 6, 3000, 750, 3);
  std::string clean = "Line one\nback\\slash and a \\n that is text\nend \\\ndesc \xC3\xA9t\xC3\xA9 \xF0\x9F\x93\x96 ?( ???\nSTICKYMETA 1\nflowsize 5";
  CHECK(lib.bookInfo(lib.find(key.c_str()), in) && in.converted && in.description == clean);  // the open book
  CHECK(in.position == 2000 && in.marks == 2 && in.stats.secs == 240 && in.flowSize == 8000 && in.coverType == 'p');
  lib.closeBook();
  std::string meta = slurp(cache + key + ".meta");
  size_t at = meta.find("\ndesc ");
  CHECK(at != std::string::npos);
  CHECK(meta.find('\n', at + 1) == meta.find("\nEND "));  // the description is one line
  CHECK(meta.find("\nflowsize 8000\n") != std::string::npos && meta.find("\nflowsize 5") == std::string::npos);
  {
    BookLibrary again(root.c_str());
    wire(again);
    again.scanBegin();
    probes = 0;
    LibInfo k;
    CHECK(again.bookInfo(again.find(key.c_str()), k) && probes == 0);  // converted: from the .meta, the book isn't read
    CHECK(k.converted && k.description == clean && k.flowPath == cache + key + ".flow" && k.coverPath == cache + key + ".cover");
    CHECK(k.coverType == 'p' && k.position == 2000 && k.flowSize == 8000 && k.permille == 250 && k.marks == 2 && !k.finished);
    CHECK(k.status == LB_READING && k.stats.secs == 240 && k.stats.pages == 6 && k.stats.sessions == 1 && k.stats.started == 750 &&
          k.stats.paceBytesPerMin() == 750 && k.title == "Plain" && k.author == "Writer");
    CHECK(openAll(again, again.find(key.c_str()), 800) == BOOK_DONE && again.position() == 2000);
    again.closeBook();
  }
  // The copy is gone: not converted, the place kept, the description from the book again.
  remove((cache + key + ".flow").c_str());
  CHECK(lib.bookInfo(lib.find(key.c_str()), in) && !in.converted && in.flowPath.empty() && in.position == 2000 && in.flowSize == 8000);
  CHECK(in.description == "First para.\nSecond para here.\nbad ? byte" && in.stats.secs == 240);
  // Converted again into a flow of another size: the place doesn't hold, the time spent does.
  fake.reset();
  fake.flowBytes = 9000;
  CHECK(openAll(lib, lib.find(key.c_str()), 900) == BOOK_DONE && lib.position() == 0 && lib.markCount() == 0 && lib.bookStats().secs == 240);
  lib.closeBook();
  CHECK(lib.bookInfo(lib.find(key.c_str()), in) && in.converted && in.position == 0 && in.flowSize == 9000 && in.stats.secs == 240);
  // An over-long description is cut at BOOK_DESCRIPTION_MAX, on a character.
  std::string longDesc;
  while (longDesc.size() < 3 * BOOK_DESCRIPTION_MAX) longDesc += "\xC3\xA9t\xC3\xA9 ";
  put(root + "/books/long.txt", "T:Long|A:|D:" + longDesc);
  scanAll(lib);
  CHECK(lib.bookInfo(byTitle(lib, "Long"), in) && in.description.size() <= BOOK_DESCRIPTION_MAX && in.description.size() > BOOK_DESCRIPTION_MAX - 6);
  CHECK((in.description.back() & 0xC0) != 0xC0);
  // The book went: false, and the list loses it.
  remove((root + "/books/long.txt").c_str());
  int n = lib.count();
  CHECK(!lib.bookInfo(byTitle(lib, "Long"), in) && lib.count() == n - 1 && byTitle(lib, "Long") < 0);
}

// With the real text reader (not the placeholder): a plain text book, converted.
static bool realTextReader() {
  FILE *f = fopen("../../sticky-os/booktext.cpp", "rb");
  if (!f) return false;
  char line[200] = "";
  bool real = fgets(line, sizeof line, f) && !strstr(line, "PLACEHOLDER");
  fclose(f);
  return real;
}

static void testRealText(const char *fixtures) {
  if (!realTextReader()) {
    printf("lib: SKIPPED the real conversion of a text book: booktext.cpp is still the placeholder\n");
    return;
  }
  std::string root = fresh("real"), cache = cacheOf(root);
  std::string text = slurp(std::string(fixtures) + "/lib-book.txt");
  CHECK(text.size() > 500);
  put(root + "/books/a-walk.txt", text);
  put(root + "/books/empty.txt", "");
  BookLibrary lib(root.c_str());
  scanAll(lib);
  CHECK(lib.count() == 2);
  int i = byTitle(lib, "a walk");
  if (i < 0) i = lib.count() - 1 - (byTitle(lib, "empty") == lib.count() - 1);  // the reader found a title of its own
  LibBook b;
  CHECK(lib.book(i, b) && b.format == BF_TXT && !b.error);
  std::string key = b.key;
  int p, steps = 0;
  CHECK(lib.openBegin(i, 1000));
  while ((p = lib.openStep()) >= 0 && p < BOOK_DONE) steps++;
  CHECK(p == BOOK_DONE && lib.isOpen());
  std::string flow = slurp(lib.flowPath());
  CHECK(flow.size() > 400 && flow.size() == lib.flowSize() && flow.find("hedgehog") != std::string::npos);
  CHECK(exists(cache + key + ".meta") && exists(cache + key + ".flow.toc") && lib.coverType() == 0);
  printf("lib: real text book: %zu bytes -> flow %zu bytes in %d steps, title \"%s\"\n", text.size(), flow.size(), steps, lib.openTitle());
  lib.setPosition((uint32_t)flow.size() / 2, 1010);
  lib.closeBook();
  BookLibrary again(root.c_str());
  CHECK(again.scanBegin() && again.lastRead() == 0);
  CHECK(again.openBegin(0, 1020) && again.openStep() == BOOK_DONE && again.position() == flow.size() / 2);
  again.closeBook();
  // A book with nothing in it says so, and nothing is kept of it.
  int e = again.find(keyOf(lib, "empty").c_str());
  if (e >= 0) {
    while (again.scanStep()) {}
    CHECK(again.book(e, b));
    std::string ek = b.key;
    int r = openAll(again, e, 1030);
    CHECK(r < 0 && !exists(cache + ek + ".meta") && !exists(cache + ek + ".flow"));
  }
}

static double seconds() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec + tv.tv_usec / 1e6;
}

// BOOKS_DIR: a folder of real books, as a card's "books".
static void realBooks(const char *dir) {
  std::string root = fresh("real-books");
  char full[4096];
  if (!realpath(dir, full) || symlink(full, (root + "/books").c_str()) != 0) {
    printf("lib: BOOKS_DIR %s can't be used\n", dir);
    return;
  }
  for (int pass = 0; pass < 2; pass++) {
    BookLibrary lib(root.c_str());
    double t0 = seconds();
    lib.scanBegin();
    double t1 = seconds();
    int steps = 0;
    double worst = 0;
    for (;;) {
      double s = seconds();
      bool more = lib.scanStep();
      worst = std::max(worst, seconds() - s);
      if (!more) break;
      steps++;
    }
    double t2 = seconds();
    printf("lib: BOOKS_DIR pass %d (%s): %d books, listed in %.1f ms, %d steps in %.1f ms (the longest %.1f ms), index %zu bytes\n",
           pass + 1, pass ? "from the index" : "no index", lib.count(), (t1 - t0) * 1e3, steps, (t2 - t1) * 1e3, worst * 1e3,
           slurp(cacheOf(root) + "library.idx").size());
    CHECK(lib.state() == LS_OK || lib.count() == 0);
    if (pass) continue;
    LibBook b;
    for (int i = 0; lib.book(i, b); i++)
      printf("  %-5s %s%-44.44s  %-28.28s  %s\n", bookFormatLabel(b.format), b.error ? "! " : "  ", b.title, b.author, b.path);
  }
}

int main(int argc, char **argv) {
  const char *fixtures = argc > 1 ? argv[1] : "fixtures";
  mkdirs(BASE);
  testNames();
  testWhere();
  testCap();
  testIndex();
  testOrder();
  testState();
  testTorn();
  testOpen();
  testCardGone();
  testStats();
  testDays();
  testOldFormats();
  testInfo();
  testRealText(fixtures);
  if (const char *dir = getenv("BOOKS_DIR")) realBooks(dir);
  printf("lib: %s\n", fails ? "FAILED" : "ok");
  return fails ? 1 : 0;
}
