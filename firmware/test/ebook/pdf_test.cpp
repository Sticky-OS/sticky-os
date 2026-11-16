// The PDF reader (bookpdf.cpp): the small PDFs made by
// tools/make_pdf_fixtures.py, each showing one thing a PDF can do, turned
// into flows that must be exactly so; the quick look; cut and damaged
// copies of each. With BOOKS_DIR set, every .pdf in it (and in its
// pdf-extra/ folder) end to end, with numbers.
//
//   ./run.sh pdf
//   pdf <fixtures> --dump book.pdf     (a book's flow as text, to read)
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
  int code = 0;  // BOOK_DONE or -(BookErr)
  std::string flow, toc;
  BookMeta meta;
  int steps = 0;
  double maxStep = 0, total = 0;
  std::string cover;  // the cover file's bytes
};

static Result convert(const std::string &path, double stepLimit = 0) {
  Result r;
  BookConverter c;
  std::string out = "build/pdf_test.flow";
  double t0 = now();
  remove("build/pdf_test.cover");
  if (!c.begin(path.c_str(), out.c_str(), "build/pdf_test.cover")) {
    r.code = -(int)c.error();
    return r;
  }
  for (;;) {
    double a = now();
    int p = c.step();
    double dt = now() - a;
    if (dt > r.maxStep) r.maxStep = dt;
    r.steps++;
    if (p < 0 || p >= BOOK_DONE) {
      r.code = p;
      break;
    }
    if (stepLimit > 0 && now() - t0 > stepLimit) {  // a hang: give up, and say so
      r.code = -999;
      break;
    }
  }
  r.total = now() - t0;
  r.meta = c.meta();
  if (r.code == BOOK_DONE) {
    r.flow = flowSlurp(out), r.toc = flowSlurp(out + ".toc"), r.cover = flowSlurp("build/pdf_test.cover");
    std::string why;
    if (!flowValidate(r.flow, &why)) printf("FAIL %s: bad flow: %s\n", path.c_str(), why.c_str()), fails++;
    if (!tocValidate(r.toc, r.flow, &why)) printf("FAIL %s: bad toc: %s\n", path.c_str(), why.c_str()), fails++;
  }
  return r;
}

// The flow of a fixture, as text; and that it is what it must be.
static void expect(const std::string &dir, const char *name, const char *want, const char *wantToc = nullptr) {
  Result r = convert(dir + "/" + name);
  std::string got = flowDump(r.flow);
  bool ok = r.code == BOOK_DONE && got == want && (!wantToc || r.toc == wantToc);
  if (!ok) {
    printf("FAIL %s: code %d\n--- got:\n%s\n--- want:\n%s\n", name, r.code, got.c_str(), want);
    if (wantToc) printf("--- toc got:\n%s--- want:\n%s", r.toc.c_str(), wantToc);
    fails++;
  }
}

static void expectErr(const std::string &dir, const char *name, BookErr err) {
  Result r = convert(dir + "/" + name);
  if (r.code != -(int)err) printf("FAIL %s: code %d, want %d\n", name, r.code, -(int)err), fails++;
}

static void fixtures(const std::string &dir) {
  // A classic table, Helvetica without widths, WinAnsi; two paragraphs (an indent starts the second), a bulleted line.
  expect(dir, "pdf-classic.pdf",
         "The quick brown fox jumps over the lazy dog while the small cat watches from the garden wall and thinks about the long afternoon ahead of them both.\n"
         "A second paragraph starts here and it ends right away.\n"
         "<ITEM> A list item, its bullet in WinAnsi.\n");
  // Object streams, a cross-reference stream with PNG prediction, Flate; an italic font.
  expect(dir, "pdf-modern.pdf", "Packed objects and a predicted table still read well.\n<i>In italics.</i>\n");
  // No space characters: TJ's numbers (-300 a space, -40 and 20 not) and the gaps between strings
  // (0.31 em a space, 0.18 not) say where words end; letters set apart by Tc stay one word.
  expect(dir, "pdf-tj.pdf", "Kerning wordshere and Two stringstouch Spaced\n");
  // A two-byte Identity-H font through /ToUnicode: bfchar, bfrange counting up, the array
  // form, "fi" for one code, a surrogate pair. The same kind of font with no map: nothing.
  expect(dir, "pdf-cmap.pdf", "ABC finon h\xC3\xA9\xF0\x9D\x84\x9E\n");
  // /Differences by glyph name over WinAnsi: f_i, eacute, uni0161, T_h, quoteright, Euro;
  // the font's name says italic. MacRomanEncoding.
  expect(dir, "pdf-diffs.pdf", "<i>find caf\xC3\xA9 There \xC5\xA1 it\xE2\x80\x99s \xE2\x82\xAC" "5</i> Mac \xC3\xA9t\xC3\xA9 \xE2\x80\x9Cquoted\xE2\x80\x9D\n");
  // A running header and page numbers left out; a paragraph going on over a page break; "extra-" "ordinarily" joined.
  expect(dir, "pdf-pages.pdf",
         "The first page begins a story that goes on for a while, longer than one page can hold, and so it reaches the bottom of this page and goes on "
         "over the top of the next page without a break, as books do. Here a word is broken: it is extraordinarily long. Then the paragraph ends.\n"
         "A last page with a new paragraph of its own, which is also the end of the book.\n");
  // The outline: by page reference, and by a named destination (names tree, GoTo action); a UTF-16 title.
  expect(dir, "pdf-outline.pdf",
         "\n[PAGE] <H1> Chapter One\nThis is the text of chapter one, which has a few words in it.\n"
         "\n[PAGE] <H1> Chapter Two\nThis is the text of chapter two, which has a few words in it.\n",
         "0\t1\tChapter One\n76\t1\tChapter Tw\xC3\xB6\n");
  // Contents as two streams: ASCII85 over LZW, then ASCIIHex over RunLength; a form XObject;
  // an inline image whose bytes hold "EI".
  expect(dir, "pdf-filters.pdf",
         "Old filters, LZW in ASCII85, work: aaaaaaaaaaaaaaaaaaaa. From a form, RunLength in hex.\nAfter the picture.\n");
  // Every offset in the table wrong, and a stream's /Length: found by scanning the file.
  expect(dir, "pdf-broken.pdf", "Found by looking for the objects.\n");
  // Two columns: the left one first. No outline: the title is the contents.
  std::string left, right;
  for (int i = 1; i <= 14; i++) left += "Left column words number " + std::to_string(i) + " go here. ";
  for (int i = 1; i <= 14; i++) right += "Right side text item " + std::to_string(i) + " sits there" + (i < 14 ? ". " : ".");
  expect(dir, "pdf-columns.pdf", ("\n[PAGE] <H1> A Paper In Two Columns\n" + left + right + "\n").c_str(), "0\t1\tA Paper In Two Columns\n");
  // Loops: the page tree, a form calling itself, the outline; 200 q and no Q; a reference to itself.
  expect(dir, "pdf-cycles.pdf", "\n[PAGE] Still here after all that.\nOnce from the form.\n", "0\t1\tLoop\n");
  expectErr(dir, "pdf-encrypted.pdf", BE_ENCRYPTED);
  expectErr(dir, "pdf-scan.pdf", BE_NO_TEXT);
  CHECK(convert(dir + "/pdf-classic.pdf").meta.language == "en-GB");

  // The quick look.
  BookMeta m;
  BookErr e;
  CHECK(bookProbe((dir + "/pdf-classic.pdf").c_str(), m, e) && e == BE_OK && m.format == BF_PDF);
  CHECK(m.title == "Caf\xC3\xA9" && m.author == "Ann (the) Writer");
  CHECK(m.description == "A short story about a fox.");  // /Subject, its markup gone
  CHECK(bookProbe((dir + "/pdf-modern.pdf").c_str(), m, e) && m.description.empty());  // only the title again
  // The description in the job's meta too; a cover only where the first page is one JPEG.
  Result cl = convert(dir + "/pdf-classic.pdf");
  CHECK(cl.meta.description == "A short story about a fox." && cl.meta.cover == 0 && cl.cover.empty());
  Result cv = convert(dir + "/pdf-cover.pdf");
  std::string jpeg = flowSlurp(dir + "/pdf-cover.pdf");
  size_t at = jpeg.find("\xFF\xD8\xFF\xE0");
  CHECK(cv.code == BOOK_DONE && cv.meta.cover == 'j' && cv.cover.size() == 4 + 2048 + 2 && at != std::string::npos && jpeg.compare(at, cv.cover.size(), cv.cover) == 0);
  CHECK(flowDump(cv.flow) == "The text after the cover, long enough for a book of two pages to count as text.\n");
  CHECK(bookProbe((dir + "/pdf-modern.pdf").c_str(), m, e) && m.title == "Packed");
  CHECK(bookProbe((dir + "/pdf-encrypted.pdf").c_str(), m, e) && e == BE_OK && m.title.empty());
  CHECK(bookProbe((dir + "/pdf-broken.pdf").c_str(), m, e) && m.title.empty());
  CHECK(!bookProbe((dir + "/missing.pdf").c_str(), m, e) && e == BE_OPEN);
}

// A page far bigger than a step: 30 000 lines of text, read a slice at a time and
// laid out in parts (the runs' cap), every line still there.
static void hugePage() {
  std::string content;
  for (int i = 0; i < 30000; i++) content += "BT /F1 10 Tf 1 0 0 1 50 " + std::to_string(400000 - i * 12) + " Tm (Line " + std::to_string(i) + " of the long page.) Tj ET\n";
  std::string pdf = "%PDF-1.4\n";
  std::vector<size_t> off;
  auto obj = [&](const std::string &body) { off.push_back(pdf.size()); pdf += std::to_string(off.size()) + " 0 obj\n" + body + "\nendobj\n"; };
  obj("<< /Type /Catalog /Pages 2 0 R >>");
  obj("<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
  obj("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 400 400000] /Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >>");
  obj("<< /Length " + std::to_string(content.size()) + " >>\nstream\n" + content + "\nendstream");
  obj("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>");
  size_t x = pdf.size();
  pdf += "xref\n0 6\n0000000000 65535 f \n";
  char row[32];
  for (size_t o : off) snprintf(row, sizeof row, "%010lu 00000 n \n", (unsigned long)o), pdf += row;
  pdf += "trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n" + std::to_string(x) + "\n%%EOF\n";
  FILE *f = fopen("build/pdf_huge.pdf", "wb");
  fwrite(pdf.data(), 1, pdf.size(), f);
  fclose(f);
  Result r = convert("build/pdf_huge.pdf");
  CHECK(r.code == BOOK_DONE);
  CHECK(r.steps > (int)(content.size() / (64 * 1024)));  // a slice a step
  CHECK(r.flow.find("Line 0 of the long page.") != std::string::npos);
  CHECK(r.flow.find("Line 29999 of the long page.") != std::string::npos);
  printf("  huge page: %zu bytes of content, %d steps, longest %.1f ms\n", content.size(), r.steps, r.maxStep * 1000);
}

// Cut at many lengths, bytes flipped with a fixed seed: every variant ends,
// soon, with a flow or an error; nothing reads out of bounds (UBSan).
static void robustness(const std::string &path, int cuts, int flips, unsigned seed) {
  std::string good = flowSlurp(path);
  if (good.empty()) return;
  srand(seed);
  int done = 0, refused = 0;
  double worst = 0;
  for (int i = 0; i < cuts + flips; i++) {
    std::string bad = good;
    if (i < cuts) bad.resize(good.size() * i / cuts);
    else
      for (int k = 1 + rand() % 4; k > 0; k--) bad[rand() % bad.size()] ^= (char)(1 << (rand() % 8));
    FILE *f = fopen("build/pdf_mut.pdf", "wb");
    fwrite(bad.data(), 1, bad.size(), f);
    fclose(f);
    BookMeta m;
    BookErr e;
    bookProbe("build/pdf_mut.pdf", m, e);
    Result r = convert("build/pdf_mut.pdf", 10);
    if (r.code == -999) printf("FAIL %s variant %d doesn't end\n", path.c_str(), i), fails++;
    if (r.total > worst) worst = r.total;
    (r.code == BOOK_DONE ? done : refused)++;
  }
  printf("  %s: %d variants, %d converted, %d refused, slowest %.0f ms\n", path.substr(path.rfind('/') + 1).c_str(), cuts + flips, done, refused, worst * 1000);
}

// The real books in BOOKS_DIR (and BOOKS_DIR/pdf-extra): numbers, and a look at how they begin.
static void realBooks(const char *books) {
  std::vector<std::string> paths;
  for (std::string d : {std::string(books), std::string(books) + "/pdf-extra"}) {
    DIR *dir = opendir(d.c_str());
    if (!dir) continue;
    while (dirent *e = readdir(dir)) {
      std::string n = e->d_name;
      if (n.size() > 4 && n.compare(n.size() - 4, 4, ".pdf") == 0) paths.push_back(d + "/" + n);
    }
    closedir(dir);
  }
  std::sort(paths.begin(), paths.end());
  for (const std::string &p : paths) {
    BookMeta m;
    BookErr e;
    double t0 = now();
    bool ok = bookProbe(p.c_str(), m, e);
    double probe = now() - t0;
    Result r = convert(p, 600);
    size_t toc = 0;
    for (char c : r.toc) toc += c == '\n';
    printf("  %s: probe %s \"%s\" / \"%s\" in %.1f ms; %s: flow %zu bytes, %zu contents entries, %d steps, longest step %.1f ms, all %.0f ms\n",
           p.substr(p.rfind('/') + 1).c_str(), ok ? "ok" : "failed", m.title.c_str(), m.author.c_str(), probe * 1000,
           r.code == BOOK_DONE ? "converted" : ("error " + std::to_string(-r.code)).c_str(), r.flow.size(), toc, r.steps, r.maxStep * 1000, r.total * 1000);
    printf("    description %zu bytes \"%.60s\"; cover %s (%zu bytes)\n", r.meta.description.size(), r.meta.description.c_str(), r.meta.cover ? "JPEG" : "none", r.cover.size());
    CHECK(r.code == BOOK_DONE || r.code == -(int)BE_NO_TEXT || r.code == -(int)BE_ENCRYPTED);
    if (getenv("PDF_SHOW")) printf("%s\n", flowDump(r.flow, 3000).c_str());
  }
  std::string gatsby = std::string(books) + "/great-gatsby.pdf";
  // Its tables gone ("startxref" no more): rebuilt by scanning, a slice a step, and the same book comes out.
  std::string bytes = flowSlurp(gatsby);
  if (!bytes.empty()) {
    Result good = convert(gatsby);
    for (size_t at = bytes.find("startxref"); at != std::string::npos; at = bytes.find("startxref", at)) bytes[at] = 'X';
    FILE *f = fopen("build/pdf_notable.pdf", "wb");
    fwrite(bytes.data(), 1, bytes.size(), f);
    fclose(f);
    Result r = convert("build/pdf_notable.pdf");
    CHECK(r.code == BOOK_DONE && r.flow == good.flow);
    printf("  great-gatsby.pdf without its tables: %d steps, longest %.1f ms, %s\n", r.steps, r.maxStep * 1000, r.flow == good.flow ? "the same flow" : "a different flow");
  }
  robustness(gatsby, 12, 12, 11);
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : "fixtures";
  if (argc > 3 && !strcmp(argv[2], "--dump")) {
    Result r = convert(argv[3]);
    printf("code %d title [%s] author [%s] lang [%s] steps %d\n%s\n--- toc\n%s", r.code, r.meta.title.c_str(), r.meta.author.c_str(),
           r.meta.language.c_str(), r.steps, flowDump(r.flow).c_str(), r.toc.c_str());
    return 0;
  }
  fixtures(dir);
  hugePage();
  unsigned seed = 1;
  DIR *d = opendir(dir.c_str());
  std::vector<std::string> names;
  while (dirent *e = d ? readdir(d) : nullptr) {
    std::string n = e->d_name;
    if (n.compare(0, 4, "pdf-") == 0) names.push_back(n);
  }
  if (d) closedir(d);
  std::sort(names.begin(), names.end());
  for (const std::string &n : names) robustness(dir + "/" + n, 40, 60, seed++);
  if (const char *books = getenv("BOOKS_DIR")) realBooks(books);
  printf("pdf: %s\n", fails ? "FAILED" : "ok");
  return fails ? 1 : 0;
}
