// The Books app's pager: turns a flow (bookflow.h) into laid-out pages for a
// page size and a set of fonts, and moves through the book. It doesn't draw:
// a page is a list of lines, a line a list of runs (a word, or a piece of
// one), each with its x, its font and its text, ready for the screen's own
// text function. Justified lines are spelled out the same way (every word
// has its x), so the drawing code knows nothing about typesetting.
//
// Plain C++ (no Arduino): the same code runs in the tests on a computer
// (firmware/test/ebook/page_test.cpp).
//
//   BookLayout lay;                       // the text area, the fonts' sizes, advance()
//   BookPager pager;
//   if (!pager.open("/sd/books/.sticky/1a2b.flow", lay, savedPosition)) ...   // status()
//   const BookPage &pg = pager.page();
//   for (const PageLine &ln : pg.lines)
//     for (int i = 0; i < ln.runCount; i++) {
//       const PageRun &r = pg.runs[ln.firstRun + i];
//       setFont(r.role); setCursor(left + r.x, top + ln.baseline); print(pg.text(r));
//     }
//   for (const PageRule &r : pg.rules) drawFastHLine(left + r.x, top + r.y, r.width);
//   pager.nextPage();  save(pager.position());
//
// How pages are cut (what the screens can rely on):
//
// - A place in a book is a byte offset in its flow. position() is the one to
//   save; open() and gotoOffset() show the page that holds a given offset.
// - Pages lie on a grid that depends only on the flow and the layout, never
//   on how the reader got there. The flow is cut into segments at "anchors":
//   offset 0 and the first block that starts at or after every multiple of
//   BOOK_STRIDE bytes (a block longer than 8 KB is cut inside, at a space).
//   Pages are laid out forward from the segment's anchor, and the last page
//   of a segment ends at the next anchor, so it can be short: one short page
//   about every 20 to 30 screens, the price of never having to lay out more
//   than one segment to show any page. A chapter (FL_PAGE) starts a page too.
// - So: after nextPage(), prevPage() shows the identical page (same start,
//   same end, same lines) and the other way round, after any walk; a page's
//   end is the next page's start; open(page.start) shows that page again.
// - Changing the layout keeps position(): the new page is the one holding
//   it. Changing it back shows the page the reader was on (position() only
//   moves when a page is turned or a goto is made).
// - Work in one call: nextPage() lays out one page; any other call lays out
//   at most one segment (BOOK_STRIDE + about 8.5 KB of flow) plus one page.
//
// Typesetting: lines break at spaces (never at a no-break space), at a soft
// hyphen (shown as "-" only there), after a hyphen or dash inside a word,
// and anywhere in a word wider than the line. Headings stay with the text
// that follows them, a paragraph's lone first or last line is kept off a
// page's bottom or top when one line's move does it, a rule is dropped at a
// page's top or bottom. A character the font can't draw (advance 0) is left out.
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "psalloc.h"

// The fonts a page is drawn with. A heading is all in its own font (italic
// and bold inside it are not followed).
enum BookRole : uint8_t { BR_REGULAR, BR_ITALIC, BR_BOLD, BR_BOLD_ITALIC, BR_H1, BR_H2, BR_H3, BR_COUNT };

static const uint32_t BOOK_STRIDE = 32768;

struct BookFontMetrics {
  int16_t lineHeight = 0;  // the font's line advance, px
  int16_t ascent = 0;      // baseline to the top of the tallest letter
};

// What the screen gives the pager. Lengths in pixels.
struct BookLayout {
  int16_t width = 0, height = 0;  // the text area
  BookFontMetrics font[BR_COUNT];
  // The pen advance of one character in a font; 0: the font hasn't got it
  // (it is left out). No kerning. Called once per character and font for
  // ASCII, seldom for the rest (the pager keeps what it learned).
  int (*advance)(void *ctx, int role, uint32_t codePoint) = nullptr;
  void *ctx = nullptr;
  int16_t lineSpacing = 100;  // percent of the fonts' line height
  int16_t paraIndent = 0;     // a paragraph's first line (not after a heading, a rule, a chapter's start)
  int16_t paraSpace = 0;      // between paragraphs
  int16_t quoteIndent = 0;    // a quote's left and right margins
  int16_t itemIndent = 0;     // where a list item's text starts; the bullet or number hangs left of it
  int16_t headAbove = -1;     // around headings; -1: a body line above, half of one below
  int16_t headBelow = -1;
  bool justify = true;
  bool centreHeadings = true;
};

enum : uint8_t {
  PR_SPACE_BEFORE = 1,  // a space separates it from the run before (else they touch: one word in two fonts)
  PR_BULLET = 2,        // a list bullet the pager added: not the book's text
  PR_HYPHEN = 4,        // ends with a "-" the pager added at a soft hyphen
  PR_MARKER = 8,        // a list item's own number ("12.")
};

struct PageRun {
  int16_t x;        // of its left edge, from the text area's left
  uint16_t width;
  uint32_t text;    // in the page's arena: BookPage::text()
  uint16_t len;     // bytes, without the 0 that ends it
  uint8_t role;     // BookRole
  uint8_t flags;    // PR_*
};

struct PageLine {
  int16_t baseline;  // y, from the text area's top
  int16_t top, height;  // the line's box (leading included)
  uint16_t runCount;  // none: a blank line
  uint32_t firstRun;  // in BookPage::runs
};

// A break in the text: a short centred line (or draw "* * *" centred on it).
struct PageRule {
  int16_t x, y, width;
};

// One page. Reused from page to page: nothing is allocated at a page turn
// once the vectors have grown to a page's size.
struct BookPage {
  uint32_t start = 0, end = 0;  // flow offsets: its first byte, and the next page's
  PsVec<PageLine> lines;
  PsVec<PageRun> runs;
  PsVec<PageRule> rules;
  PsVec<char> arena;
  // A run's text: UTF-8, ended by a 0, no control characters, no spaces.
  const char *text(const PageRun &r) const { return arena.data() + r.text; }
  bool blank() const { return lines.empty(); }
  void clear() { lines.clear(), runs.clear(), rules.clear(), arena.clear(); }
};

enum BookPagerStatus : uint8_t {
  BP_CLOSED,  // no book open
  BP_OK,
  BP_EMPTY,   // the flow has no bytes: nothing to show
  BP_IO,      // the flow can't be read (a removed card): page() is blank; gotoOffset(position()) tries again
};

struct BookTocItem {
  uint32_t offset = 0;
  uint8_t level = 0;        // 1..3; 0: no such entry
  bool current = false;     // the chapter position() is in
  const char *title = "";   // at most 120 bytes; valid until the book is closed
};

// For the tests and for measuring: the work done since resetStats().
struct BookPagerStats {
  uint32_t bytesRead = 0;    // from the file
  uint32_t seeks = 0;
  uint32_t bytesLaidOut = 0; // flow bytes gone through line breaking (a look-ahead counts again)
  uint32_t pages = 0;        // pages laid out, shown or not
};

class BookPager {
 public:
  BookPager();
  ~BookPager();
  BookPager(const BookPager &) = delete;
  BookPager &operator=(const BookPager &) = delete;

  // Opens a flow (and reads "<flow>.toc" if it is there) and shows the page
  // holding `position` (clamped to the flow). False: status() says why.
  bool open(const char *flowPath, const BookLayout &layout, uint32_t position = 0);
  void close();
  BookPagerStatus status() const;

  // A new text size or page shape: shows the page holding position() again.
  bool setLayout(const BookLayout &layout);

  // The page to draw. Blank when status() isn't BP_OK.
  const BookPage &page() const;
  // The reader's place, to save: the page's start after a page turn, the
  // offset asked for after a goto. Always inside the page shown.
  uint32_t position() const;
  uint32_t flowSize() const;
  int permille() const;  // of position() in the flow, 0..999

  // False, and nothing changes, at the book's end / start.
  bool nextPage();
  bool prevPage();
  bool atStart() const;
  bool atEnd() const;
  // Show the page holding an offset / a thousandth of the book / a contents entry.
  bool gotoOffset(uint32_t offset);
  bool gotoPermille(int permille);
  bool gotoToc(int index);

  // The contents. The current chapter is the last entry at or before
  // position(): -1 and "" before the first one.
  int tocCount() const;
  BookTocItem toc(int index) const;
  int currentToc() const;
  const char *chapterTitle() const;

  // Page turns left until the next contents entry (or the book's end): 0 on
  // a chapter's last page, -1 while it isn't known. Counting is done by
  // tick(), a slice (about 8 KB of flow) at a time: call it when idle. It
  // returns false when there is nothing left to count. A page turn keeps
  // what was counted; a layout change or a jump starts again.
  bool tick();
  int pagesLeftInChapter() const;
  // Estimates, from the size of the pages laid out so far in this layout
  // ("about 312 pages"): they move a little as the reader goes. -1: no page yet.
  int estimatedPages() const;
  int estimatedPageNumber() const;  // of the page shown, 1..estimatedPages()

  const BookPagerStats &stats() const;
  void resetStats();

  struct Impl;

 private:
  Impl *d_;
};
