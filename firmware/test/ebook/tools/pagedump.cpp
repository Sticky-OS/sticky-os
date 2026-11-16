// Prints a flow's pages as the pager (bookpage.h) lays them out, as text:
// every run at its x on a grid of character cells, a rule as "* * *", the
// page's offsets above it. For eyeballing line breaks, justification,
// headings and page ends without the board.
//
//   pagedump [options] <file.flow | file.txt>
//     -w W -h H     the text area in pixels (432 x 700)
//     -c C          the cell: every character C px wide (10); the grid is W / C columns
//     -l L          the line height in px (2 * C), headings larger
//     -f SIZE       measure with the board's real fonts (bookfonts.h, text size 0..4)
//                   instead; the grid then shows each run at x / C, roughly
//     -s OFFSET     the first page shown holds this offset (0)
//     -n N          pages to print (3); -n 0: none, only the counts and the time
//     -r            ragged right (no justification)
//     -b            go back from the end instead: the last N pages, last first
//   A .txt file is first turned into a flow (a paragraph per blank-line
//   block; "Chapter ..." lines become chapters), for trying before the
//   converters are there; it is written beside the binary as pagedump.flow.
// To build it see README.md here.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <string>
#include <vector>

#include "bookflow.h"
#include "bookfonts.h"
#include "bookpage.h"

static int cell = 10, fontSize = -1;

static int monoAdvance(void *, int role, uint32_t cp) { return cp == 0xFFFD ? 0 : role >= BR_H1 ? cell + cell / 2 : cell; }

static int fontAdvance(void *, int role, uint32_t cp) {
  const BookFont *f = role >= BR_H1 ? bookHeadingFont(fontSize, role - BR_H1 + 1) : bookFont(fontSize, role);
  return bookAdvance(f->base, f->ext, cp);
}

static BookLayout makeLayout(int w, int h, int lineH, bool justify) {
  BookLayout l;
  l.width = w, l.height = h, l.justify = justify;
  for (int r = 0; r < BR_COUNT; r++) {
    if (fontSize >= 0) {
      const BookFont *f = r >= BR_H1 ? bookHeadingFont(fontSize, r - BR_H1 + 1) : bookFont(fontSize, r);
      l.font[r].lineHeight = f->yAdvance, l.font[r].ascent = f->ascent;
    } else {
      l.font[r].lineHeight = r >= BR_H1 ? lineH * 3 / 2 : lineH;
      l.font[r].ascent = l.font[r].lineHeight * 3 / 4;
    }
  }
  l.advance = fontSize >= 0 ? fontAdvance : monoAdvance;
  int em = fontSize >= 0 ? bookFont(fontSize, 0)->pt : cell * 2;
  l.paraIndent = (int16_t)(em * 3 / 2), l.quoteIndent = (int16_t)(em * 2), l.itemIndent = (int16_t)(em * 2);
  return l;
}

// A plain text file as a flow: blank lines end paragraphs.
static bool txtToFlow(const char *txt, const char *flow) {
  FILE *in = fopen(txt, "rb");
  if (!in) return false;
  FlowWriter w;
  if (!w.open(flow)) return fclose(in), false;
  char line[4096];
  bool para = false;
  while (fgets(line, sizeof line, in)) {
    char *p = line;
    while (*p == ' ' || *p == '\t') p++;
    size_t n = strlen(p);
    while (n && (p[n - 1] == '\n' || p[n - 1] == '\r' || p[n - 1] == ' ')) p[--n] = 0;
    if (!n) {
      para = false;
      continue;
    }
    if (!strncmp(p, "CHAPTER ", 8) || !strncmp(p, "Chapter ", 8)) {
      w.tocEntry(p, 1);
      w.pageBreak();
      w.beginBlock(BK_H1);
      w.text(p);
      w.endBlock();
      para = false;
      continue;
    }
    if (!para) w.beginBlock(), para = true;
    else w.text(" ");
    w.text(p);
  }
  fclose(in);
  return w.close();
}

static void print(const BookPage &pg, const BookLayout &l) {
  int cols = l.width / cell;
  printf("+-- page %u..%u, %zu lines %s+\n", pg.start, pg.end, pg.lines.size(), std::string(cols > 30 ? cols - 30 : 0, '-').c_str());
  size_t rule = 0;
  for (const PageLine &ln : pg.lines) {
    for (; rule < pg.rules.size() && pg.rules[rule].y < ln.top; rule++) {
      std::string s(cols, ' ');
      int c = (pg.rules[rule].x + pg.rules[rule].width / 2) / cell - 2;
      if (c >= 0 && c + 5 <= cols) s.replace(c, 5, "* * *");
      printf("|%s|\n", s.c_str());
    }
    // Columns hold bytes here; a run's letters beyond ASCII take one column each.
    std::vector<std::string> row(cols, " ");
    for (int i = 0; i < ln.runCount; i++) {
      const PageRun &r = pg.runs[ln.firstRun + i];
      int c = r.x / cell;
      for (const char *p = pg.text(r); *p && c < cols;) {
        int len = 1;
        while (((uint8_t)p[len] & 0xC0) == 0x80) len++;
        row[c++].assign(p, len);
        p += len;
      }
    }
    std::string s;
    for (auto &c : row) s += c;
    printf("|%s| %d\n", s.c_str(), ln.baseline);
  }
  for (; rule < pg.rules.size(); rule++) printf("|%*s* * *%*s|\n", cols / 2 - 2, "", cols - cols / 2 - 3, "");
}

int main(int argc, char **argv) {
  int w = 432, h = 700, lineH = -1, pages = 3;
  long start = 0;
  bool justify = true, back = false;
  const char *path = nullptr;
  for (int i = 1; i < argc; i++) {
    const char *a = argv[i];
    const char *v = i + 1 < argc ? argv[i + 1] : "0";
    if (!strcmp(a, "-w")) w = atoi(v), i++;
    else if (!strcmp(a, "-h")) h = atoi(v), i++;
    else if (!strcmp(a, "-c")) cell = atoi(v), i++;
    else if (!strcmp(a, "-l")) lineH = atoi(v), i++;
    else if (!strcmp(a, "-f")) fontSize = atoi(v), i++;
    else if (!strcmp(a, "-s")) start = atol(v), i++;
    else if (!strcmp(a, "-n")) pages = atoi(v), i++;
    else if (!strcmp(a, "-r")) justify = false;
    else if (!strcmp(a, "-b")) back = true;
    else path = a;
  }
  if (!path || cell < 1) {
    fprintf(stderr, "usage: pagedump [-w W] [-h H] [-c CELL] [-l LINE] [-f SIZE] [-s OFFSET] [-n PAGES] [-r] [-b] <file.flow|file.txt>\n");
    return 2;
  }
  if (fontSize > 4) fontSize = 4;
  if (lineH < 1) lineH = cell * 2;
  std::string flow = path;
  if (flow.size() > 4 && flow.compare(flow.size() - 4, 4, ".txt") == 0) {
    flow = "pagedump.flow";
    if (!txtToFlow(path, flow.c_str())) return fprintf(stderr, "can't convert %s\n", path), 1;
  }
  BookLayout l = makeLayout(w, h, lineH, justify);
  BookPager p;
  if (!p.open(flow.c_str(), l, (uint32_t)(start < 0 ? 0 : start))) {
    fprintf(stderr, "can't open %s (status %d)\n", flow.c_str(), (int)p.status());
    return 1;
  }
  if (back) p.gotoOffset(0xFFFFFFFF);
  for (int i = 0; i < pages; i++) {
    print(p.page(), l);
    printf("   %d%%  page ~%d of ~%d  chapter: %s\n\n", p.permille() / 10, p.estimatedPageNumber(), p.estimatedPages(), p.chapterTitle());
    if (!(back ? p.prevPage() : p.nextPage())) break;
  }
  // The whole book, forward, timed.
  p.gotoOffset(0);
  p.resetStats();
  int n = 1;
  clock_t t0 = clock();
  while (p.nextPage()) n++;
  double us = (double)(clock() - t0) * 1e6 / CLOCKS_PER_SEC / n;
  printf("%d pages, %u bytes of flow (%.0f a page), %.1f us a page, %d contents entries\n", n, p.flowSize(), (double)p.flowSize() / n, us, p.tocCount());
  return 0;
}
