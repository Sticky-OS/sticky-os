// Specimen sheets of the Books app's reading fonts (bookfonts.h), drawn as
// the board draws them (gfxdraw.h) on a 480 x 800 page: for every text size
// a page of prose with its three headings, a page of the languages' letters,
// and one sheet with every style of every size on a shared baseline.
//   fontspecimen <folder>     writes <folder>/specimen-*.png
// To build it see README.md here.
#include <stdio.h>

#include <string>
#include <vector>

#include "gfxdraw.h"

static const int PAGE_W = 480, PAGE_H = 800, MARGIN = 24;

// A paragraph with *bold* and _italic_ words (both: bold italic), set between
// the margins from baseline y on; returns the baseline after it.
static int paragraph(GfxFrame &page, int size, int y, const char *text, bool justify, int lead) {
  struct Piece { const BookFont *font; std::string s; bool spaceBefore; int w; };
  std::vector<Piece> pieces;
  int style = 0;
  bool space = false;
  std::string cur;
  auto flush = [&]() {
    if (cur.empty()) return;
    const BookFont *f = bookFont(size, style);
    pieces.push_back({f, cur, space, bookTextAdvance(f->base, f->ext, cur.c_str())});
    cur.clear();
    space = false;
  };
  for (const char *p = text; *p; p++) {
    if (*p == '*' || *p == '_') flush(), style ^= *p == '*' ? BOOK_BOLD : BOOK_ITALIC;
    else if (*p == ' ') flush(), space = true;
    else cur += *p;
  }
  flush();
  const int width = PAGE_W - 2 * MARGIN, gap = bookFont(size, BOOK_REGULAR)->space;
  for (size_t at = 0; at < pieces.size();) {
    // As many words as fit: a piece without a space before it belongs to the word before.
    size_t end = at;
    int w = 0, gaps = 0;
    while (end < pieces.size()) {
      size_t wordEnd = end + 1;
      int ww = pieces[end].w;
      while (wordEnd < pieces.size() && !pieces[wordEnd].spaceBefore) ww += pieces[wordEnd++].w;
      int add = (end > at ? gap : 0) + ww;
      if (end > at && w + add > width) break;
      w += add, gaps += end > at, end = wordEnd;
    }
    bool last = end == pieces.size();
    int extra = justify && !last && gaps ? width - w : 0, x = MARGIN, g = 0;
    for (size_t i = at; i < end; i++) {
      if (i > at && pieces[i].spaceBefore) g++, x += gap + extra * g / gaps - extra * (g - 1) / gaps;
      x = page.drawText(x, y, pieces[i].font, pieces[i].s.c_str());
    }
    y += lead;
    at = end;
  }
  return y;
}

static int heading(GfxFrame &page, int size, int level, int y, const char *text) {
  const BookFont *f = bookHeadingFont(size, level);
  y += f->yAdvance;
  page.drawText(MARGIN, y, f, text);
  return y;
}

static bool save(const GfxFrame &page, const std::string &folder, const char *name, int pt) {
  char path[512];
  if (pt) snprintf(path, sizeof path, "%s/specimen-%s-%d.png", folder.c_str(), name, pt);
  else snprintf(path, sizeof path, "%s/specimen-%s.png", folder.c_str(), name);
  if (!page.savePng(path)) return fprintf(stderr, "can't write %s\n", path), false;
  printf("%s\n", path);
  return true;
}

int main(int argc, char **argv) {
  if (argc != 2) return fprintf(stderr, "usage: %s <folder for the PNGs>\n", argv[0]), 1;
  std::string folder = argv[1];
  bool ok = true;

  for (int size = 0; size < BOOK_SIZES; size++) {
    const BookFont *body = bookFont(size, BOOK_REGULAR);
    int lead = body->yAdvance;

    GfxFrame prose(PAGE_W, PAGE_H);
    int y = heading(prose, size, 1, 8, "The Lighthouse");
    y = heading(prose, size, 2, y + 4, "Two: Low Water");
    y = heading(prose, size, 3, y + 4, "What the keeper wrote");
    y += lead + 6;
    y = paragraph(prose, size, y,
                  "The tide had gone out _further_ than anyone in the village could remember, and the old "
                  "keeper stood at the rail with his lamp unlit. “It will come back,” he said — "
                  "*it always comes back* — but he did not sound _quite_ certain… Below him the "
                  "rocks lay bare for half a mile, *_black and shining_*, and the gulls walked where the "
                  "boats had floated. He wrote the hour in the log: 4:37, wind NNW, glass falling; "
                  "then, after a while, ‘nothing to report’.",
                  true, lead);
    y = paragraph(prose, size, y,
                  "By evening 1,250 people had gathered on the quay (some said 2,000). Nobody spoke; "
                  "the fishermen’s wives counted the masts – 36, 37, 38 – and counted again.",
                  true, lead);
    paragraph(prose, size, y + lead / 3, "0123456789 £€$% & @ #42 ½ ° © № 7 * [x] {y} a/b <c>", false, lead);
    ok &= save(prose, folder, "prose", body->pt);

    GfxFrame lang(PAGE_W, PAGE_H);
    y = lead;
    static const char *const LINES[] = {
        "Français: _Où_ est le cœur de l’été ? « Noël », à l’œil nu, déjà vu, *Été À Ça Ê*.",
        "Română: _Știință_ și răbdare; înțeleg că mâine plouă. *Țări, Șah, În, Â*.",
        "Polski: _Zażółć gęślą jaźń_, łódź, święto, *Łódź ŻŹ ĆŚ*.",
        "Deutsch: _Größe_, Übung, Ärger, Öl; „Fußgänger“, *Mädchen ÜÖÄ*.",
        "Türkçe: _ışık_, İstanbul, ğüneş, çocuk, *ŞİĞ ÇÖÜ*.",
        "Español: ¿_Qué_ niño? ¡Mañana! pingüino, año, *Ñandú ÁÉÍÓÚ*.",
        "Česky: _Příliš žluťoučký kůň_ úpěl ďábelské ódy. *ŘŠŽ ŮĚ*.",
        "Русский: _Съешь ещё_ этих мягких булок, *да выпей чаю*. ЖЩЫЮЯ ЇїЄє Јљњџ",
        "“Quotes” ‘single’ „low“ ‚single‘ ‹x› «x» — em – en − minus - hyphen … • ™",
        "Folded: ‰ → ‟ ‐ Āā ẞ ⅓ (what the fonts lack)",
    };
    for (const char *line : LINES) y = paragraph(lang, size, y, line, false, lead) + lead / 4;
    ok &= save(lang, folder, "languages", body->pt);
  }

  // The four styles of every size along one baseline, with marks at the
  // baseline and the x-height of the regular at both ends to see that the
  // styles agree; then the letters that reach furthest, between marks at the
  // regular's ink limits (nothing may cross those).
  GfxFrame all(PAGE_W, PAGE_H);
  int y = 4;
  for (int size = 0; size < BOOK_SIZES; size++) {
    const BookFont *r = bookFont(size, BOOK_REGULAR);
    y += r->yAdvance;
    int x = MARGIN;
    for (int style = 0; style < 4; style++) x = all.drawText(x, y, bookFont(size, style), style < 3 ? "Hxgp " : "Hxgp");
    for (int mark : {MARGIN - 16, x + 4}) all.hline(mark, y, 12), all.hline(mark, y - r->xHeight, 12);
    y += r->yAdvance;
    x = MARGIN;
    for (int style = 0; style < 4; style++) x = all.drawText(x, y, bookFont(size, style), style < 3 ? "\u00c5\u0170\u00c7\u021b " : "\u00c5\u0170\u00c7\u021b");
    for (int mark : {MARGIN - 16, x + 4}) all.hline(mark, y - r->inkTop - 1, 12), all.hline(mark, y + r->inkBottom, 12);
    y += 8;
  }
  for (int pt : {24, 28}) {
    const BookFont *f = bookHeadingFont(pt == 24 ? 4 : 5, 1);
    y += f->yAdvance;
    char line[64];
    snprintf(line, sizeof line, "Bold %d: Chapter Éț", f->pt);
    all.drawText(MARGIN, y, f, line);
  }
  ok &= save(all, folder, "styles", 0);
  return ok ? 0 : 1;
}
