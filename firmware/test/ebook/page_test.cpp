// The pager (bookpage.h): line breaking and the blocks' shapes on small
// hand-made flows, then the promises about pages (nothing lost, nothing past
// the edges, the same page whichever way it is reached, bounded work) on
// generated books, broken flows, and real books when BOOKS_DIR is set.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <map>
#include <string>
#include <vector>

#include "bookflow.h"
#include "bookpage.h"
#include "ebook.h"

static int fails;
#define CHECK(c) do { if (!(c)) { if (fails < 40) printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

// ---- fonts made of numbers

struct Metrics {
  int mode;   // 0: every character 10 px; 1: widths that differ by character and font
  int scale;  // percent
};

static int advance(void *ctx, int role, uint32_t cp) {
  const Metrics *m = (const Metrics *)ctx;
  if (m->mode == 0) return cp < 0x2500 || cp == 0xFFFD ? 10 : 0;
  int a;
  if (cp == ' ') a = 5;
  else if (cp < 0x80) a = 6 + (int)(cp * 7 % 9);
  else if (cp < 0x250) a = 9;
  else if (cp == 0x2013) a = 10;
  else if (cp == 0x2014) a = 16;
  else if (cp >= 0x2018 && cp <= 0x201D) a = 5;
  else if (cp == 0x2022) a = 8;
  else if (cp == 0xFFFD) a = 12;
  else return 0;
  if (role == BR_BOLD || role == BR_BOLD_ITALIC) a++;
  if (role >= BR_H1) a = a * (role == BR_H1 ? 18 : role == BR_H2 ? 15 : 13) / 10;
  return a * m->scale / 100;
}

static Metrics fixedM = {0, 100}, variedM = {1, 100}, bigM = {1, 150};

static BookLayout layout(int w, int h, Metrics *m, bool justify = true) {
  BookLayout l;
  l.width = w, l.height = h;
  for (int r = 0; r < BR_COUNT; r++) {
    int lh = r == BR_H1 ? 40 : r == BR_H2 ? 32 : r == BR_H3 ? 28 : 24;
    l.font[r].lineHeight = lh * m->scale / 100;
    l.font[r].ascent = lh * 3 / 4 * m->scale / 100;
  }
  l.advance = advance, l.ctx = m;
  l.justify = justify;
  l.paraIndent = 20, l.quoteIndent = 30, l.itemIndent = 30;
  return l;
}

// ---- helpers

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

static void spit(const std::string &path, const std::string &data) {
  FILE *f = fopen(path.c_str(), "wb");
  if (f) fwrite(data.data(), 1, data.size(), f), fclose(f);
}

// A flow written as it is (no FlowWriter: also for broken ones), without contents.
static std::string raw(const char *name, const std::string &bytes, const std::string &toc = "") {
  std::string path = std::string("build/page_") + name + ".flow";
  spit(path, bytes);
  if (toc.empty()) remove((path + ".toc").c_str());
  else spit(path + ".toc", toc);
  return path;
}

static std::string lineText(const BookPage &pg, const PageLine &ln) {
  std::string s;
  for (int i = 0; i < ln.runCount; i++) {
    const PageRun &r = pg.runs[ln.firstRun + i];
    if (i && (r.flags & PR_SPACE_BEFORE)) s += ' ';
    s += pg.text(r);
    if (r.flags & (PR_BULLET | PR_MARKER)) s += '|';
  }
  return s;
}

static std::string pageText(const BookPage &pg) {
  std::string s;
  for (const PageLine &ln : pg.lines) s += lineText(pg, ln) + "\n";
  return s;
}

static uint32_t rngState;
static uint32_t rnd() {
  rngState ^= rngState << 13, rngState ^= rngState >> 17, rngState ^= rngState << 5;
  return rngState;
}
static uint32_t rnd(uint32_t n) { return rnd() % n; }

static uint64_t hashPage(const BookPage &pg) {
  uint64_t h = 1469598103934665603ull;
  auto mix = [&](uint64_t v) { h = (h ^ v) * 1099511628211ull; };
  mix(pg.start), mix(pg.end);
  for (const PageLine &ln : pg.lines) {
    mix((uint64_t)ln.baseline), mix(ln.runCount);
    for (int i = 0; i < ln.runCount; i++) {
      const PageRun &r = pg.runs[ln.firstRun + i];
      mix((uint64_t)r.x), mix(r.width), mix(r.role), mix(r.flags);
      for (const char *p = pg.text(r); *p; p++) mix((uint8_t)*p);
    }
  }
  for (const PageRule &r : pg.rules) mix((uint64_t)r.x), mix((uint64_t)r.y), mix((uint64_t)r.width);
  return h;
}

static uint32_t utf8(const std::string &s, size_t i, uint32_t &cp) {
  uint8_t b = s[i];
  int need = b < 0x80 ? 0 : (b & 0xE0) == 0xC0 ? 1 : (b & 0xF0) == 0xE0 ? 2 : (b & 0xF8) == 0xF0 ? 3 : -1;
  if (need < 0 || i + need >= s.size() + (need ? 0 : 1)) return cp = 0xFFFD, 1;
  cp = need == 0 ? b : need == 1 ? b & 0x1F : need == 2 ? b & 0x0F : b & 0x07;
  for (int k = 1; k <= need; k++) cp = cp << 6 | ((uint8_t)s[i + k] & 0x3F);
  return need + 1;
}

// What a walk through the whole book must print: the flow's characters,
// without the control bytes, the spaces, the soft hyphens and what the font hasn't got.
static std::string expected(const std::string &flow, Metrics *m) {
  std::string out;
  for (size_t i = 0; i < flow.size();) {
    uint32_t cp, n = utf8(flow, i, cp);
    if (cp >= 0x20 && cp != 0x7F && cp != ' ' && cp != 0xA0 && cp != 0xAD && advance(m, BR_REGULAR, cp)) out.append(flow, i, n);
    i += n;
  }
  return out;
}

// The edges, the runs' widths and order, the lines' order: what must hold on any page.
static void checkGeometry(const BookPage &pg, const BookLayout &l) {
  int prevBottom = 0;
  for (const PageLine &ln : pg.lines) {
    CHECK(ln.top >= prevBottom);
    prevBottom = ln.top + ln.height;
    int desc = l.font[BR_REGULAR].lineHeight - l.font[BR_REGULAR].ascent;
    if (ln.runCount && pg.runs[ln.firstRun].role >= BR_H1) {
      const BookFontMetrics &f = l.font[pg.runs[ln.firstRun].role];
      desc = f.lineHeight - f.ascent;
    }
    CHECK(ln.baseline >= ln.top && ln.baseline + desc <= l.height);
    int right = 0;
    for (int i = 0; i < ln.runCount; i++) {
      const PageRun &r = pg.runs[ln.firstRun + i];
      CHECK(r.x >= right);
      CHECK(r.x + r.width <= l.width);
      right = r.x + r.width;
      CHECK(r.len == strlen(pg.text(r)) && r.len > 0);
      int w = 0;
      std::string t = pg.text(r);
      for (size_t k = 0; k < t.size();) {
        uint32_t cp, n = utf8(t, k, cp);
        CHECK(cp >= 0x20 && cp != ' ');
        w += advance(l.ctx, r.role, cp), k += n;
      }
      CHECK(w == r.width);
    }
  }
  for (const PageRule &r : pg.rules) CHECK(r.x >= 0 && r.x + r.width <= l.width && r.y >= 0 && r.y < l.height);
}

struct GridPage {
  uint32_t end;
  uint64_t hash;
};
typedef std::map<uint32_t, GridPage> Grid;

// From the first page to the last: the pages join, nothing is lost. Returns the grid.
static Grid walk(const std::string &path, const BookLayout &l, bool checkText = true) {
  Grid grid;
  std::string flow = slurp(path), got;
  BookPager p;
  CHECK(p.open(path.c_str(), l, 0));
  CHECK(p.page().start == 0 && p.atStart());
  uint32_t prevEnd = 0;
  for (size_t guard = 0; guard <= flow.size(); guard++) {
    const BookPage &pg = p.page();
    CHECK(pg.start == prevEnd && pg.end > pg.start && p.position() == pg.start);
    prevEnd = pg.end;
    checkGeometry(pg, l);
    grid[pg.start] = {pg.end, hashPage(pg)};
    for (const PageLine &ln : pg.lines)
      for (int i = 0; i < ln.runCount; i++) {
        const PageRun &r = pg.runs[ln.firstRun + i];
        if (r.flags & PR_BULLET) continue;
        got.append(pg.text(r), r.len - (r.flags & PR_HYPHEN ? 1 : 0));
      }
    if (!p.nextPage()) break;
  }
  CHECK(prevEnd == flow.size() && p.atEnd());
  if (checkText) {
    std::string want = expected(flow, (Metrics *)l.ctx);
    CHECK(got == want);
    if (got != want) {
      size_t i = 0;
      while (i < got.size() && i < want.size() && got[i] == want[i]) i++;
      printf("  text differs at %zu of %zu/%zu: got \"%s\" want \"%s\"\n", i, got.size(), want.size(), got.substr(i, 30).c_str(), want.substr(i, 30).c_str());
    }
  }
  return grid;
}

// ---- generated books

static const char *SYL[] = {"a", "be", "con", "de", "er", "fi", "gro", "ha", "in", "jo", "ka", "lim", "mo", "ne", "or", "pre", "qua", "ri", "st", "tion",
                            "un", "ver", "wa", "xi", "yo", "z", "\xC3\xA9", "\xC3\xB1o", "\xC3\xBC", "I", "The", "ed", "ing", "ly", "s"};

static void word(FlowWriter &w) {
  uint32_t kind = rnd(100);
  if (kind < 2) {  // a word no line holds
    for (int i = 0, n = 60 + rnd(200); i < n; i++) w.text(SYL[rnd(35)]);
    return;
  }
  int n = 1 + rnd(4);
  for (int i = 0; i < n; i++) {
    w.text(SYL[rnd(35)]);
    if (i + 1 < n) {
      uint32_t r = rnd(40);
      if (r < 4) w.text("\xC2\xAD");  // a soft hyphen
      else if (r == 4) w.text("-");
      else if (r == 5) w.text("\xE2\x80\x94");  // an em dash
      else if (r == 6) w.italic(rnd(2));        // a font change inside the word
      else if (r == 7) w.bold(rnd(2));
    }
  }
  uint32_t r = rnd(30);
  if (r == 0) w.text(",");
  else if (r == 1) w.text(".\xE2\x80\x9D");
  else if (r == 2) w.text("\xC2\xA0!");  // a no-break space
  else if (r == 3) w.text("\xE4\xB8\xAD");  // a character the fonts haven't got
}

static void sentence(FlowWriter &w, int words) {
  for (int i = 0; i < words; i++) {
    if (i) w.text(" ");
    uint32_t r = rnd(60);
    if (r == 0) w.italic(true);
    else if (r == 1) w.italic(false);
    else if (r == 2) w.bold(true);
    else if (r == 3) w.bold(false);
    else if (r == 4 && i) w.lineBreak();
    else if (r == 5 && i) w.lineBreak(), w.lineBreak();
    word(w);
  }
}

// A book of `blocks` blocks of every kind. chapters: with page breaks and contents.
static std::string generate(const char *name, uint32_t seed, int blocks, bool chapters) {
  rngState = seed;
  std::string path = std::string("build/page_") + name + ".flow";
  FlowWriter w;
  CHECK(w.open(path.c_str()));
  int chapter = 0;
  for (int b = 0; b < blocks; b++) {
    uint32_t r = rnd(100);
    if (chapters && (b == 0 || rnd(60) == 0)) {
      char title[40];
      snprintf(title, sizeof title, "Chapter %d", ++chapter);
      w.tocEntry(title, 1);
      w.pageBreak();
      w.beginBlock(BK_H1);
      w.text(title);
      if (rnd(3) == 0) w.beginBlock(BK_H2), sentence(w, 2 + rnd(12));
      continue;
    }
    if (r < 3) w.beginBlock(BK_H2), sentence(w, 1 + rnd(8));
    else if (r < 5) w.beginBlock(BK_H3), sentence(w, 1 + rnd(5));
    else if (r < 9) w.beginBlock(BK_QUOTE), sentence(w, 3 + rnd(60));
    else if (r < 12) w.beginBlock(BK_CENTER), sentence(w, 1 + rnd(20));
    else if (r < 18) {
      w.beginBlock(BK_ITEM);
      if (rnd(2)) {
        char num[16];
        snprintf(num, sizeof num, "%u%s ", rnd(2) ? rnd(200) : rnd(9) + 1, rnd(2) ? "." : ")");
        w.text(num);
      }
      sentence(w, 1 + rnd(30));
    } else if (r < 20) w.rule();
    else if (r < 22 && chapters) w.pageBreak();
    else w.beginBlock(), sentence(w, 1 + rnd(rnd(8) ? 90 : 400));
  }
  CHECK(w.close());
  return path;
}

// ---- small flows: lines and blocks

static void testLines() {
  BookLayout l = layout(100, 240, &fixedM, false);
  l.paraIndent = 0;
  BookPager p;
  // Greedy breaking on spaces.
  CHECK(p.open(raw("t", "aaaa bbbb cccc dd\n").c_str(), l));
  CHECK(pageText(p.page()) == "aaaa bbbb\ncccc dd\n");
  CHECK(p.page().lines[0].baseline == 18 && p.page().lines[1].baseline == 42);
  CHECK(p.page().runs[1].x == 50 && p.page().runs[1].width == 40 && (p.page().runs[1].flags & PR_SPACE_BEFORE));
  CHECK(p.page().start == 0 && p.page().end == 18 && p.atEnd() && p.atStart() && !p.nextPage() && !p.prevPage());
  // A soft hyphen shows only where the line breaks.
  CHECK(p.open(raw("t", "abcdef\xC2\xADghijkl\n").c_str(), l));
  CHECK(pageText(p.page()) == "abcdef-\nghijkl\n");
  CHECK(p.page().runs[0].flags & PR_HYPHEN);
  CHECK(p.open(raw("t", "abc\xC2\xAD" "def gh\n").c_str(), l));
  CHECK(pageText(p.page()) == "abcdef gh\n" && p.page().runs[0].width == 60);
  // ... and not when the hyphen itself wouldn't fit: the word is cut instead.
  CHECK(p.open(raw("t", "abcdefghij\xC2\xADkl\n").c_str(), l));
  CHECK(pageText(p.page()) == "abcdefghij\nkl\n");
  // After a hyphen or a dash inside a word.
  CHECK(p.open(raw("t", "aaa well-known x\n").c_str(), l));
  CHECK(pageText(p.page()) == "aaa well-\nknown x\n");
  CHECK(p.open(raw("t", "aaaaaa b\xE2\x80\x94" "cccc\n").c_str(), l));
  CHECK(pageText(p.page()) == "aaaaaa b\xE2\x80\x94\ncccc\n");
  // A word wider than the line is cut by characters.
  CHECK(p.open(raw("t", "x " + std::string(35, 'w') + " y\n").c_str(), l));
  CHECK(pageText(p.page()) == "x\nwwwwwwwwww\nwwwwwwwwww\nwwwwwwwwww\nwwwww y\n");
  // A no-break space takes room and doesn't break.
  CHECK(p.open(raw("t", "aa bbb\xC2\xA0" "ccc dd\n").c_str(), l));
  CHECK(pageText(p.page()) == "aa bbbccc\ndd\n");
  CHECK(p.page().runs[2].x == 70 && !(p.page().runs[2].flags & PR_SPACE_BEFORE));
  CHECK(p.open(raw("t", "aa bbbb\xC2\xA0" "ccc dd\n").c_str(), l));
  CHECK(pageText(p.page()) == "aa\nbbbbccc\ndd\n");
  // Forced breaks; two leave a blank line, a third adds nothing.
  CHECK(p.open(raw("t", "a\x0B" "b\x0B\x0B" "c\x0B\x0B\x0B" "d\n").c_str(), l));
  CHECK(pageText(p.page()) == "a\nb\n\nc\n\nd\n");
  // Fonts change inside a line and inside a word.
  CHECK(p.open(raw("t", "ab\x0E" "cd\x0F \x10" "e\x0E" "f\x11\x0F\n").c_str(), l));
  {
    const BookPage &pg = p.page();
    CHECK(pg.runs.size() == 4 && pg.lines.size() == 1);
    if (pg.runs.size() == 4) {
      CHECK(!strcmp(pg.text(pg.runs[0]), "ab") && pg.runs[0].role == BR_REGULAR);
      CHECK(!strcmp(pg.text(pg.runs[1]), "cd") && pg.runs[1].role == BR_ITALIC && pg.runs[1].x == 20 && !(pg.runs[1].flags & PR_SPACE_BEFORE));
      CHECK(!strcmp(pg.text(pg.runs[2]), "e") && pg.runs[2].role == BR_BOLD && pg.runs[2].x == 50 && (pg.runs[2].flags & PR_SPACE_BEFORE));
      CHECK(!strcmp(pg.text(pg.runs[3]), "f") && pg.runs[3].role == BR_BOLD_ITALIC && pg.runs[3].x == 60);
    }
  }
  // Italic carries over a line break, and ends with its block.
  CHECK(p.open(raw("t", "aaaa \x0E" "bbbb cccc\nd\n").c_str(), l));
  CHECK(p.page().runs.size() == 4 && p.page().runs[2].role == BR_ITALIC && p.page().runs[3].role == BR_REGULAR);
  // What the font hasn't got is left out; a paragraph of nothing takes no line.
  CHECK(p.open(raw("t", "a\xE4\xB8\xAD" "b\n\xE4\xB8\xAD\xE4\xB8\xAD\nc\n").c_str(), l));
  CHECK(pageText(p.page()) == "ab\nc\n" && p.page().lines[1].top == 24);
  // Broken UTF-8 becomes U+FFFD; control bytes that mean nothing are skipped; no last new line.
  CHECK(p.open(raw("t", "a\xFF" "b\x1F\x09" "c\xC3").c_str(), l));
  CHECK(pageText(p.page()) == "a\xEF\xBF\xBD" "bc\xEF\xBF\xBD\n" && p.page().end == 7);
}

static void testJustify() {
  BookLayout l = layout(300, 2000, &variedM, true);
  rngState = 7;
  std::string text;
  for (int i = 0; i < 400; i++) text += std::string(SYL[rnd(26)]) + SYL[rnd(26)] + (i % 37 == 36 ? "\x0B" : " ");
  text += "end\n";
  BookPager p;
  CHECK(p.open(raw("t", text).c_str(), l));
  const BookPage &pg = p.page();
  CHECK(pg.lines.size() > 20);
  int flush = 0, ragged = 0;
  for (size_t i = 0; i < pg.lines.size(); i++) {
    const PageLine &ln = pg.lines[i];
    const PageRun &first = pg.runs[ln.firstRun], &last = pg.runs[ln.firstRun + ln.runCount - 1];
    CHECK(first.x == (i == 0 ? 0 : 0));
    // The gaps and what they would be unstretched.
    int natural = 0, stretched = 0, gaps = 0;
    for (int k = 1; k < ln.runCount; k++) {
      const PageRun &a = pg.runs[ln.firstRun + k - 1], &b = pg.runs[ln.firstRun + k];
      stretched += b.x - (a.x + a.width), natural += 5, gaps++;
      CHECK(b.x - (a.x + a.width) >= 5);
    }
    bool atEdge = last.x + last.width == l.width;
    if (atEdge) {
      flush++;
      CHECK(stretched * 2 <= natural * 5);
    } else {
      ragged++;
      CHECK(stretched == natural);
    }
    // The remainder is spread: no gap differs from another by more than a pixel.
    if (atEdge && gaps) {
      int lo = 1000, hi = 0;
      for (int k = 1; k < ln.runCount; k++) {
        int g = pg.runs[ln.firstRun + k].x - (pg.runs[ln.firstRun + k - 1].x + pg.runs[ln.firstRun + k - 1].width);
        lo = g < lo ? g : lo, hi = g > hi ? g : hi;
      }
      CHECK(hi - lo <= 1);
    }
  }
  CHECK(flush > 15);
  CHECK(ragged >= 11);  // the ten lines before a forced break, and the last
  // A line of one long word and a short one isn't stretched beyond reason.
  CHECK(p.open(raw("t", "ab " + std::string(40, 'm') + " cd ef\n").c_str(), l));
  CHECK(p.page().runs[0].x == 0 && p.page().lines[0].runCount == 1);
  // Justify off: every gap is a space.
  l.justify = false;
  CHECK(p.open(raw("t", text).c_str(), l));
  for (const PageLine &ln : p.page().lines)
    for (int k = 1; k < ln.runCount; k++) CHECK(p.page().runs[ln.firstRun + k].x - (p.page().runs[ln.firstRun + k - 1].x + p.page().runs[ln.firstRun + k - 1].width) == 5);
}

static void testBlocks() {
  BookLayout l = layout(200, 300, &fixedM, false);  // 20 characters a line
  BookPager p;
  // Paragraphs: the first isn't indented, nor the one after a heading or a rule; the others are.
  CHECK(p.open(raw("t", "one\ntwo\n\x02Head\nthree\nfour\n\x07\nfive\nsix\n").c_str(), l));
  {
    const BookPage &pg = p.page();
    CHECK(pageText(pg) == "one\ntwo\nHead\nthree\nfour\nfive\nsix\n");
    int want[] = {0, 20, 80, 0, 20, 0, 20};  // "Head" is centred: (200 - 40) / 2
    for (int i = 0; i < 7 && i < (int)pg.runs.size(); i++) CHECK(pg.runs[i].x == want[i]);
    CHECK(pg.runs[2].role == BR_H2);
    // Space around the heading (a line above, half below), a line's box for the rule.
    CHECK(pg.lines[1].top == 24 && pg.lines[2].top == 72 && pg.lines[2].height == 32 && pg.lines[3].top == 116);
    CHECK(pg.rules.size() == 1 && pg.rules[0].y == pg.lines[4].top + 24 + 12 + 12 && pg.rules[0].x == 75 && pg.rules[0].width == 50);
  }
  l.height = 240;  // 10 lines a page
  // Spaced paragraphs instead.
  l.paraIndent = 0, l.paraSpace = 10;
  CHECK(p.open(raw("t", "one\ntwo\n").c_str(), l));
  CHECK(p.page().runs[1].x == 0 && p.page().lines[1].top == 34);
  l.paraIndent = 20, l.paraSpace = 0;
  // Headings left when asked.
  l.centreHeadings = false;
  CHECK(p.open(raw("t", "\x01Head\n").c_str(), l));
  CHECK(p.page().runs[0].x == 0 && p.page().runs[0].role == BR_H1 && p.page().lines[0].baseline == 30);
  l.centreHeadings = true;
  // A quote is indented on both sides, a centred block centred line by line.
  CHECK(p.open(raw("t", "\x04" "aaaa bbbb cccc dddd\n\x05xx yyyy zzzzzzzz aaaa\n").c_str(), l));
  CHECK(pageText(p.page()) == "aaaa bbbb cccc\ndddd\nxx yyyy zzzzzzzz\naaaa\n");
  CHECK(p.page().runs[0].x == 30 && p.page().runs[3].x == 30 && p.page().runs[4].x == 20 && p.page().runs[7].x == 80);
  // List items: a bullet, or the item's own number, and the text hanging after it.
  CHECK(p.open(raw("t", "\x06" "aaaa bbbb cccc dddd eeee\n\x06" "12. aaaa bbbb cccc dddd\n\x06" "1) x\n\x06" "1984 was\n").c_str(), l));
  {
    const BookPage &pg = p.page();
    CHECK(pageText(pg) == "\xE2\x80\xA2|aaaa bbbb cccc\ndddd eeee\n12.|aaaa bbbb cccc\ndddd\n1)|x\n\xE2\x80\xA2|1984 was\n");
    CHECK(pg.runs.size() == 16);
    if (pg.runs.size() == 16) {
      CHECK((pg.runs[0].flags & PR_BULLET) && pg.runs[0].x == 10 && pg.runs[1].x == 30 && pg.runs[4].x == 30);
      CHECK((pg.runs[6].flags & PR_MARKER) && pg.runs[6].x == 0 && pg.runs[7].x == 40 && pg.runs[10].x == 40);  // "12." is wider than the indent
      CHECK(pg.runs[11].x == 0 && pg.runs[12].x == 30 && (pg.runs[13].flags & PR_BULLET) && pg.runs[14].x == 30);
    }
  }
  // A chapter starts a page; not a blank one when the page is still empty.
  CHECK(p.open(raw("t", "\x0C\x01One\ntext\n\x0C\x01Two\nmore\n").c_str(), l));
  CHECK(pageText(p.page()) == "One\ntext\n" && p.page().start == 0 && p.page().end == 11);
  CHECK(p.nextPage() && pageText(p.page()) == "Two\nmore\n" && p.page().runs[1].x == 0 && !p.nextPage());
  CHECK(p.prevPage() && p.page().end == 11);
  // A rule at a page's top is dropped, and at its bottom (here: before a chapter, and with no room).
  CHECK(p.open(raw("t", "\x07\na\n\x07\n\x0C" "b\n").c_str(), l));
  CHECK(p.page().rules.empty() && pageText(p.page()) == "a\n" && p.nextPage() && pageText(p.page()) == "b\n");
  std::string nine;
  for (int i = 0; i < 9; i++) nine += "x\n";
  CHECK(p.open(raw("t", nine + "\x07\ny\n").c_str(), l));
  CHECK(p.page().lines.size() == 9 && p.page().rules.empty() && p.nextPage() && p.page().rules.empty() && pageText(p.page()) == "y\n");
  // A heading stays with what follows: here it would be the page's last line but one.
  std::string six;
  for (int i = 0; i < 6; i++) six += "x\n";
  std::string para = "aaaa bbbb cccc dddd eeee ffff gggg hhhh\n";  // three lines
  CHECK(p.open(raw("t", six + "\x02Head\n" + para).c_str(), l));
  CHECK(p.page().lines.size() == 6 && p.nextPage() && p.page().runs[0].role == BR_H2 && p.page().lines[0].top == 0);
  // ... with one line of room less above, it and two lines fit.
  CHECK(p.open(raw("t", six.substr(2) + "\x02Head\n" + para + para).c_str(), l));
  CHECK(p.page().lines.size() == 8 && p.page().runs[5].role == BR_H2);
  // No first line alone at the bottom: the paragraph starts the next page.
  CHECK(p.open(raw("t", nine + para).c_str(), l));
  CHECK(p.page().lines.size() == 9 && p.nextPage() && p.page().lines.size() == 3);
  // No last line alone at the top: the line before goes with it.
  CHECK(p.open(raw("t", six + "\n" + para + "aaaa bbbb cccc dddd eeee ffff gggg hhhh iiii jjjj kkkk llll\n").c_str(), l));
  CHECK(p.page().lines.size() == 9 && p.nextPage() && p.page().lines.size() == 4);
  // The last line's descent stays on the page.
  l.height = 239;
  CHECK(p.open(raw("t", nine + "y\nz\n").c_str(), l));
  CHECK(p.page().lines.size() == 9);
  l.height = 240;
  // A blank line never starts a page.
  CHECK(p.open(raw("t", nine.substr(2) + "a\x0B\x0B" "b\n").c_str(), l));
  CHECK(p.page().lines.size() == 10 && p.nextPage() && pageText(p.page()) == "b\n");
}

static void testEdges() {
  BookLayout l = layout(200, 240, &fixedM);
  BookPager p;
  CHECK(p.status() == BP_CLOSED && p.page().blank() && !p.nextPage() && !p.prevPage() && !p.tick() && p.tocCount() == 0);
  CHECK(!p.open("build/page_no_such.flow", l) && p.status() == BP_IO);
  CHECK(!p.open(raw("t", "").c_str(), l) && p.status() == BP_EMPTY && p.page().blank());
  CHECK(!p.nextPage() && !p.prevPage() && !p.gotoPermille(500) && p.pagesLeftInChapter() == -1 && p.estimatedPages() == -1);
  // Nothing but what can't be shown: one blank page, cleanly.
  CHECK(p.open(raw("t", "\n\n\x07\n\xE4\xB8\xAD\n\x0C\n").c_str(), l) && p.status() == BP_OK && p.page().blank());
  CHECK(p.page().start == 0 && p.page().end == p.flowSize() && !p.nextPage());
  // Offsets past the end are the last page.
  CHECK(p.open(raw("t", "abc\ndef\n").c_str(), l, 99999) && p.position() == 7 && pageText(p.page()) == "abc\ndef\n");
  CHECK(p.gotoOffset(0xFFFFFFFF) && p.gotoPermille(5000) && p.gotoPermille(-3) && p.position() == 0);
  // A layout that makes no sense still gives pages.
  BookLayout bad;
  CHECK(p.setLayout(bad) && p.page().blank());
  bad.advance = advance, bad.ctx = &fixedM, bad.width = 3, bad.height = -5;
  bad.paraIndent = 30000, bad.lineSpacing = -1;
  CHECK(p.setLayout(bad) && p.page().blank() && p.page().end == 8);  // nothing fits an 8 px line: left out
  // The contents.
  std::string longTitle(300, 'T');
  longTitle.replace(119, 2, "\xC3\xA9");  // cut at 120 bytes, not inside this character
  std::string toc = "0\t1\tOne\nbroken line\n4\t2\t" + longTitle + "\n2\t1\tBackwards\n999\t3\tLast\n8\t9\tBad level\n";
  CHECK(p.open(raw("t", "abc\ndef\nghi\n", toc).c_str(), l, 5));
  CHECK(p.tocCount() == 4);
  CHECK(!strcmp(p.toc(0).title, "One") && p.toc(0).offset == 0 && p.toc(0).level == 1 && !p.toc(0).current);
  CHECK(strlen(p.toc(1).title) == 119 && p.toc(1).offset == 4 && p.toc(1).level == 2 && !p.toc(1).current && p.toc(2).current);
  CHECK(p.toc(2).offset == 4 && p.toc(3).offset == 11 && p.toc(7).level == 0 && p.toc(-1).level == 0);
  CHECK(p.currentToc() == 2 && !strcmp(p.chapterTitle(), "Backwards"));
  CHECK(p.gotoOffset(2) && p.currentToc() == 0 && p.gotoToc(3) && p.position() == 11 && !strcmp(p.chapterTitle(), "Last") && !p.gotoToc(4));
  CHECK(p.open(raw("t", "abc\ndef\nghi\n", "4\t1\tLater\n").c_str(), l, 0) && p.currentToc() == -1 && !strcmp(p.chapterTitle(), ""));
  p.close();
  CHECK(p.status() == BP_CLOSED && p.page().blank());
}

// ---- the promises, on generated books

static void testGrid(const std::string &path, const BookLayout &l, const BookLayout &other, uint32_t seed) {
  Grid grid = walk(path, l);
  std::vector<uint32_t> starts;
  for (auto &g : grid) starts.push_back(g.first);
  uint32_t size = (uint32_t)slurp(path).size();
  auto onGrid = [&](const BookPage &pg) {
    auto it = grid.find(pg.start);
    bool ok = it != grid.end() && it->second.end == pg.end && it->second.hash == hashPage(pg);
    CHECK(ok);
    return ok;
  };
  BookPager p;
  // Opening at a page's start shows that page; anywhere else, the page holding the place.
  rngState = seed;
  for (size_t i = 0; i < starts.size(); i += 1 + rnd(4)) {
    CHECK(p.open(path.c_str(), l, starts[i]));
    CHECK(p.page().start == starts[i] && onGrid(p.page()));
  }
  CHECK(p.open(path.c_str(), l, 0));
  for (int i = 0; i < 150; i++) {
    uint32_t x = rnd(size);
    CHECK(p.gotoOffset(x) && p.position() == x);
    CHECK(p.page().start <= x && x < p.page().end && onGrid(p.page()));
  }
  // A random walk: every page is one of the grid's, the neighbours are the neighbours.
  for (int i = 0; i < 1500; i++) {
    uint32_t op = rnd(20), s = p.page().start, e = p.page().end;
    if (op < 8) {
      bool moved = p.nextPage();
      CHECK(moved == (e < size));
      if (moved) CHECK(p.page().start == e && p.position() == e);
      else CHECK(p.page().start == s && p.page().end == e);
    } else if (op < 16) {
      bool moved = p.prevPage();
      CHECK(moved == (s > 0));
      if (moved) CHECK(p.page().end == s && p.position() == p.page().start);
      else CHECK(p.page().start == s && p.page().end == e);
    } else if (op == 16) {
      int pm = rnd(1001);
      CHECK(p.gotoPermille(pm));
      CHECK(p.page().start <= p.position() && p.position() < p.page().end);
      CHECK(pm == 1000 || p.permille() == pm || p.permille() == pm - 1);
    } else if (op == 17 && p.tocCount()) {
      int t = rnd(p.tocCount());
      CHECK(p.gotoToc(t) && p.position() == p.toc(t).offset && p.currentToc() >= t && p.toc(p.currentToc()).offset == p.toc(t).offset);
    } else if (op == 18) {
      p.tick();
    } else {
      // The layout changes and comes back: the same page again, whatever was shown in between.
      uint32_t pos = p.position();
      CHECK(p.setLayout(other) && p.position() == pos && p.page().start <= pos && pos < p.page().end);
      CHECK(p.setLayout(l) && p.position() == pos && p.page().start == s && p.page().end == e);
    }
    onGrid(p.page());
  }
  // Back from the end to the start, page by page: the same pages as forward.
  CHECK(p.gotoOffset(size - 1));
  size_t n = 1;
  while (p.prevPage()) n++, onGrid(p.page());
  CHECK(n == grid.size() && p.page().start == 0);
}

static void testCount(const std::string &path, const BookLayout &l) {
  Grid grid = walk(path, l, false);
  BookPager p;
  CHECK(p.open(path.c_str(), l, 0));
  CHECK(p.tocCount() > 3);
  // What is left in the chapter, counted the slow way.
  auto left = [&]() {
    int cur = p.currentToc();
    uint32_t end = cur + 1 < p.tocCount() ? p.toc(cur + 1).offset : p.flowSize();
    int n = 0;
    for (auto it = grid.upper_bound(p.page().start); it != grid.end() && it->first < end; ++it) n++;
    return n;
  };
  for (int t = 0; t < p.tocCount(); t += 2) {
    CHECK(p.gotoToc(t));
    int guard = 0;
    while (p.tick() && guard++ < 10000) CHECK(p.pagesLeftInChapter() == -1);
    CHECK(p.pagesLeftInChapter() == left());
    // Page turns keep the count without more work.
    for (int i = 0; i < 3 && p.pagesLeftInChapter() > 0; i++) {
      CHECK(p.nextPage());
      CHECK(p.pagesLeftInChapter() == left());
    }
    if (p.prevPage()) {
      p.tick();
      CHECK(p.pagesLeftInChapter() == left() || p.pagesLeftInChapter() == -1);
    }
  }
  // Counting while turning pages.
  CHECK(p.gotoToc(1));
  for (int i = 0; i < 40; i++) {
    p.tick();
    if (i % 3 == 0) p.nextPage();
    if (i % 7 == 0) p.prevPage();
    CHECK(p.pagesLeftInChapter() == -1 || p.pagesLeftInChapter() == left());
  }
  // The estimate, once the whole book has been seen, is about right.
  CHECK(p.gotoOffset(0));
  while (p.nextPage()) {}
  int est = p.estimatedPages(), real = (int)grid.size();
  CHECK(est > real * 8 / 10 && est < real * 12 / 10);
  CHECK(p.estimatedPageNumber() > est * 9 / 10 && p.estimatedPageNumber() <= est);
}

// One chapter of a megabyte: no call may lay out more than a segment.
static void testBounded() {
  rngState = 99;
  std::string path = "build/page_big.flow";
  {
    FlowWriter w;
    CHECK(w.open(path.c_str()));
    while (w.offset() < 1000000) {
      w.beginBlock();
      for (int i = 0, n = 5 + rnd(200); i < n; i++) {
        if (i) w.text(" ");
        for (int k = 0, m = 1 + rnd(3); k < m; k++) w.text(SYL[rnd(26)]);
      }
      if (rnd(200) == 0) {  // a paragraph of many pages: the anchors must cope
        for (int i = 0; i < 9000; i++) w.text(" lorem");
      }
    }
    CHECK(w.close());
  }
  uint32_t size = (uint32_t)slurp(path).size();
  BookLayout l = layout(432, 700, &variedM);
  const uint32_t BOUND = BOOK_STRIDE + 8192 + 512 + 4096;  // a segment, and a page
  BookPager p;
  uint32_t worstLaid = 0, worstRead = 0;
  auto within = [&](bool pageTurn) {
    const BookPagerStats &s = p.stats();
    if (s.bytesLaidOut > worstLaid) worstLaid = s.bytesLaidOut;
    if (s.bytesRead > worstRead) worstRead = s.bytesRead;
    CHECK(s.bytesLaidOut <= BOUND);
    CHECK(s.bytesRead <= BOUND + 2 * 16384 + 3 * 8192 + 2048);  // the windows, the anchors' own reads
    if (pageTurn) CHECK(s.bytesLaidOut <= 4096 && s.pages == 1 && s.bytesRead <= 16384 + 8192 + 512);
    p.resetStats();
  };
  CHECK(p.open(path.c_str(), l, size * 9 / 10));
  within(false);
  CHECK(p.setLayout(layout(700, 432, &bigM)));
  within(false);
  CHECK(p.setLayout(l));
  within(false);
  rngState = 5;
  for (int i = 0; i < 400; i++) {
    uint32_t op = rnd(10);
    if (op < 4) p.nextPage(), within(true);
    else if (op < 8) p.prevPage(), within(false);
    else if (op == 8) p.gotoOffset(rnd(size)), within(false);
    else p.tick(), within(false);
  }
  // Reading on: one page laid out per turn, the file read once, in order.
  CHECK(p.gotoOffset(0));
  p.resetStats();
  uint32_t turns = 0;
  clock_t t0 = clock();
  while (p.nextPage()) turns++;
  double us = (double)(clock() - t0) * 1e6 / CLOCKS_PER_SEC / (turns ? turns : 1);
  const BookPagerStats &s = p.stats();
  CHECK(s.pages == turns);
  CHECK(s.bytesRead <= size + size / 8 && s.bytesLaidOut <= size + size / 4);
  CHECK(s.seeks <= 2 * (size / BOOK_STRIDE) + 4);
  printf("  1 MB, one chapter, 432x700: %u pages, %.0f bytes a page, %.1f us a page turn here (sanitizer build); "
         "worst call laid out %u bytes, read %u; reading through: %u seeks\n",
         turns + 1, (double)size / (turns + 1), us, worstLaid, worstRead, s.seeks);
  // Going back through it all costs a segment once per segment, not per page.
  p.resetStats();
  uint32_t back = 0;
  while (p.prevPage()) back++;
  CHECK(back == turns);
  CHECK(p.stats().bytesLaidOut <= 2 * size + size / 2);
}

// ---- broken flows

static void drive(const std::string &path, const BookLayout &l, uint32_t size) {
  BookPager p;
  if (!p.open(path.c_str(), l, size / 3)) {
    CHECK(size == 0 && p.status() == BP_EMPTY);
    return;
  }
  uint32_t guard = 0;
  for (int i = 0; i < 12; i++) {
    uint32_t s = p.page().start;
    if (!p.prevPage()) break;
    CHECK(p.page().end == s);
  }
  while (guard++ <= size) {
    uint32_t e = p.page().end;
    checkGeometry(p.page(), l);
    CHECK(p.page().end > p.page().start);
    if (!p.nextPage()) break;
    CHECK(p.page().start == e);
  }
  CHECK(guard <= size + 1 && p.page().end == size);
  for (int i = 0; i < 20; i++) {
    uint32_t x = rnd(size);
    CHECK(p.gotoOffset(x) && p.page().start <= x && x < p.page().end);
    uint32_t s = p.page().start, e = p.page().end;
    if (p.prevPage()) CHECK(p.page().end == s && p.nextPage());
    CHECK(p.page().start == s && p.page().end == e);
    p.tick();
  }
}

static void testFuzz() {
  BookLayout a = layout(432, 700, &variedM), b = layout(120, 90, &fixedM);
  std::string good = slurp(generate("fuzzsrc", 4242, 500, true));
  for (int round = 0; round < 120; round++) {
    rngState = 1000 + round;
    std::string data;
    uint32_t kind = round % 6, n = round < 6 ? round : rnd(round % 10 == 0 ? 120000 : 6000);
    if (kind == 0) {  // noise
      for (uint32_t i = 0; i < n; i++) data += (char)rnd(256);
    } else if (kind == 1) {  // mostly control bytes
      for (uint32_t i = 0; i < n; i++) data += (char)(rnd(4) ? rnd(0x20) : 'a' + rnd(26));
    } else if (kind == 2) {  // a good flow cut short
      data = good.substr(0, rnd((uint32_t)good.size()));
    } else if (kind == 3) {  // a good flow with bytes flipped
      data = good.substr(rnd(2000), 20000);
      for (int i = 0; i < 300; i++) data[rnd((uint32_t)data.size())] = (char)rnd(256);
    } else if (kind == 4) {  // one byte over and over: no block ever ends, or nothing but ends
      static const char one[] = {'\n', '\x0B', '\x0C', '\x07', '\x01', ' ', 'x', '\xAD', '\xC2', '\x0E'};
      data.assign(n * 10, one[round / 6 % 10]);
    } else {  // text with no block ends and no spaces at all
      for (uint32_t i = 0; i < n * 8; i++) data += (char)('a' + rnd(26));
    }
    std::string toc;
    for (int i = 0; i < 20; i++) toc += (char)rnd(256);
    toc += "\n12\t1\tx\n5\t2\t\xFF\xFE\n7\t1\t";
    std::string path = raw("fuzz", data, toc);
    drive(path, a, (uint32_t)data.size());
    drive(path, b, (uint32_t)data.size());
  }
  // The card pulled out with the book open: an error, not rubbish.
  std::string path = raw("gone", good);
  BookPager p;
  CHECK(p.open(path.c_str(), a, 0));
  spit(path, good.substr(0, 20000));
  CHECK(!p.gotoPermille(900) && p.status() == BP_IO && p.page().blank());
  CHECK(!p.nextPage() && !p.prevPage() && !p.tick());
  spit(path, good);
  CHECK(p.gotoOffset(p.position()) && p.status() == BP_OK && !p.page().blank());
  while (p.tick()) {}
}

// ---- real books

// A plain text book as a flow, the simple way: a paragraph per run of lines,
// "CHAPTER" lines as chapters.
static bool textToFlow(const std::string &txt, const std::string &flow) {
  std::string all = slurp(txt);
  if (all.empty()) return false;
  FlowWriter w;
  if (!w.open(flow.c_str())) return false;
  bool para = false;
  for (size_t i = 0; i < all.size();) {
    size_t e = all.find('\n', i);
    if (e == std::string::npos) e = all.size();
    std::string line = all.substr(i, e - i);
    i = e + 1;
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
    size_t b = line.find_first_not_of(' ');
    if (b == std::string::npos) {
      para = false;
      continue;
    }
    line = line.substr(b);
    if (!line.compare(0, 8, "CHAPTER ")) {
      w.tocEntry(line.c_str(), 1);
      w.pageBreak();
      w.beginBlock(BK_H1);
      w.text(line.c_str());
      w.endBlock();
      para = false;
      continue;
    }
    if (!para) w.beginBlock(), para = true;
    else w.text(" ");
    w.text(line.c_str());
  }
  return w.close();
}

static void testBooks(const char *dir) {
  static const char *names[] = {"alice.epub", "alice.txt"};
  for (const char *name : names) {
    std::string book = std::string(dir) + "/" + name, flow = std::string("build/page_real_") + name + ".flow";
    BookConverter c;
    int step = -1;
    if (c.begin(book.c_str(), flow.c_str(), nullptr))
      while ((step = c.step()) >= 0 && step < BOOK_DONE) {}
    if (step != BOOK_DONE) {
      // No converter yet (or it failed): a text file is still real prose, as paragraphs.
      bool plain = !strcmp(name + strlen(name) - 4, ".txt") && textToFlow(book, flow);
      printf("  %s: not converted (error %d)%s\n", name, (int)c.error(), plain ? ": paragraphs split at blank lines instead" : ": skipped");
      if (!plain) continue;
    }
    uint32_t size = (uint32_t)slurp(flow).size();
    struct {
      const char *label;
      BookLayout l;
    } shapes[] = {{"portrait 432x700", layout(432, 700, &variedM)}, {"portrait, large", layout(432, 700, &bigM)},
                  {"landscape 752x420", layout(752, 420, &variedM)}, {"landscape, large", layout(752, 420, &bigM)}};
    printf("  %s: flow %u bytes;", name, size);
    for (auto &s : shapes) printf(" %s: %zu pages;", s.label, walk(flow, s.l).size());
    printf("\n");
    BookPager p;
    clock_t t0 = clock();
    CHECK(p.open(flow.c_str(), shapes[0].l, 0));
    double open0 = (double)(clock() - t0) * 1e3 / CLOCKS_PER_SEC;
    uint32_t laid0 = p.stats().bytesLaidOut;
    p.close();
    t0 = clock();
    CHECK(p.open(flow.c_str(), shapes[0].l, (uint32_t)((uint64_t)size * 9 / 10)));
    double open90 = (double)(clock() - t0) * 1e3 / CLOCKS_PER_SEC;
    uint32_t laid90 = p.stats().bytesLaidOut;
    CHECK(p.gotoOffset(0));
    p.resetStats();
    t0 = clock();
    int turns = 0;
    while (turns < 200 && p.nextPage()) turns++;
    double next = (double)(clock() - t0) * 1e3 / CLOCKS_PER_SEC;
    printf("    open at 0: %.3f ms (%u bytes laid out); open at 90%%: %.3f ms (%u bytes); %d page turns: %.3f ms, %u bytes read, %u seeks; contents: %d entries\n",
           open0, laid0, open90, laid90, turns, next, p.stats().bytesRead, p.stats().seeks, p.tocCount());
    testGrid(flow, shapes[0].l, shapes[3].l, 31);
  }
}

int main() {
  testLines();
  testJustify();
  testBlocks();
  testEdges();
  BookLayout portrait = layout(432, 700, &variedM), landscape = layout(752, 420, &variedM);
  BookLayout large = layout(432, 700, &bigM), tiny = layout(180, 130, &fixedM, false);
  BookLayout spaced = layout(752, 420, &bigM, false);
  spaced.paraIndent = 0, spaced.paraSpace = 14, spaced.lineSpacing = 130, spaced.centreHeadings = false;
  std::string chapters = generate("chapters", 1, 3000, true), plain = generate("plain", 2, 2500, false);
  testGrid(chapters, portrait, large, 11);
  testGrid(chapters, landscape, portrait, 12);
  testGrid(chapters, tiny, spaced, 13);
  testGrid(plain, large, landscape, 14);
  testGrid(plain, spaced, tiny, 15);
  testGrid(plain, portrait, spaced, 16);
  testCount(chapters, portrait);
  testCount(chapters, spaced);
  testBounded();
  testFuzz();
  if (const char *dir = getenv("BOOKS_DIR")) testBooks(dir);
  printf("page: %s\n", fails ? "FAILED" : "ok");
  return fails ? 1 : 0;
}
