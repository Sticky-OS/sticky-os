// ---------------------------------------------------------------- books
//
// The Books app: an e-reader for the books on the microSD card (a folder
// named books). This tab draws it and takes its taps, swipes and buttons;
// the work is in plain C++ beside it:
//
//   booklib.*    which books there are, the converted copies on the card,
//                where the reader was in each (positions, bookmarks)
//   ebook.*      turning a book (EPUB, MOBI/AZW3, FB2, PDF, text, HTML) into
//   book*.cpp    the app's own text format once, the first time it's opened
//   bookpage.*   laying that text out in pages, and moving through them
//   bookfonts.*  the reading typeface: FreeSerif, four styles, five sizes
//
// Screens: the library (a Continue card, then the books being read, the new
// ones and the finished ones), "getting the book ready" the first time a book
// is opened, and the reader: the cover, then the pages, full screen, with the
// chapter and the time at the top and how far at the foot. A tap on the right
// of a page turns it, on the left goes back, in the middle opens the menu
// (the status bar, the title, a progress bar to jump with, and Library,
// Contents, Bookmarks and Text). The top right corner adds or removes a
// bookmark. UP / DOWN turn the pages too, OK opens the menu.
//
// E-paper: one partial refresh a page; show() puts in a full one every 12.
// Arduino joins the sketch's .ino files into one, so the screen helpers of
// sticky-os.ino are at hand here; every function below takes plain types only
// (Arduino writes its prototypes above everything).

enum BooksMode { BM_NONE, BM_LIBRARY, BM_OPENING, BM_ERROR, BM_READING, BM_MENU, BM_TEXT, BM_TOC, BM_MARKS, BM_DETAILS, BM_STATS };

static const int BK_TOP = 54;        // the reader: the text starts under the header line
static const int BK_FOOT = 40;       // and ends over the foot
static const int BK_LIB_ROW = 84;    // a book in the list
static const int BK_LIB_HEAD = 46;   // a section's name
static const int BK_LIST_ROW = 62;   // a contents entry, a bookmark
static const int BK_MENU_TOP = 78;   // the menu's title band, under the status bar
static const int BK_MAX_PAGES = 64;  // the library's pages remembered

struct BooksState {
  int mode = BM_NONE;
  BookLibrary *lib = nullptr;
  BookPager *pager = nullptr;
  int size = BOOK_SIZE_DEFAULT;
  bool justify = true;
  int spacing = 1, margins = 1;  // 0 tight / narrow, 1 normal, 2 loose / wide
  bool hintSeen = false;
  bool cover = false;           // the cover is on screen (before the first page)
  bool coverGrey = false;       // ... drawn from the book's own picture
  int16_t layoutW = 0, layoutH = 0, layoutSize = -1, layoutLook = -1;
  bool layoutJustify = true;
  unsigned cardGen = 0;
  uint32_t libGen = 0, drawnAt = 0;
  int openProgress = 0, openDrawn = -1;
  BookErr openErr = BE_OK;
  String errTitle, errText, openTitle, openAuthor;
  int turns = 0;                // page turns since the place was last written
  // The library's screen
  int libPage = 0, libPages = 1, focus = -2;  // focus: a place in the list, -1 the Continue card, -2 none
  int16_t pageStart[BK_MAX_PAGES];
  int hitN = 0;
  struct { int16_t y0, y1; int16_t book; } hits[24];  // book -1: the Continue card
  // Contents and bookmarks
  int listPage = 0;
  String note;                  // a line for the reader's foot, once ("Bookmark added")
  // A book's page (BM_DETAILS): what the library knows of it, its contents
  int detail = -1, detailPage = 0, detailPages = 1;
  LibInfo info;
  PsVec<uint32_t> dOff, dTitle;  // its contents: where each entry points, and its title in dPool
  PsVec<uint8_t> dLevel;
  PsVec<char> dPool;
  int dCurrent = -1;            // the chapter the reader is in
  int dHitN = 0;
  struct { int16_t y0, y1, what; } dHits[24];  // what: -1 the button, 0.. a chapter
  int statsFrom = BM_LIBRARY;   // where the statistics go back to
  // Reading time: counted between page turns, given to the library at each
  int toOpenToc = -1;           // a chapter tapped on a book's page: go there once the book is open
  uint32_t turnAt = 0, pendMs = 0, pendPages = 0, pendBytes = 0;
  uint32_t turnLayoutMs = 0;
};
static uint32_t bkDrawMs;  // the last page's drawing, before it went to the panel
static BooksState *bs;

void booksBegin() {
  void *mem = psMalloc(sizeof(BooksState));
  bs = mem ? new (mem) BooksState() : new BooksState();
  // Kept as points ("bsizept"), so the ladder can grow; "bsize" was an index on the first ladder (12..21).
  static const int8_t FIRST_LADDER[] = {12, 14, 16, 18, 21};
  int pt = prefs.getInt("bsizept", 0);
  if (!pt && prefs.isKey("bsize")) pt = FIRST_LADDER[constrain(prefs.getInt("bsize", 2), 0, 4)];
  bs->size = pt ? bkPt(pt) : BOOK_SIZE_DEFAULT;
  bs->justify = prefs.getBool("bjust", true);
  bs->spacing = constrain(prefs.getInt("bspace", 1), 0, 2);
  bs->margins = constrain(prefs.getInt("bmarg", 1), 0, 2);
  bs->hintSeen = prefs.getBool("bhint", false);
}

static uint32_t booksNow() { return timeKnown() ? (uint32_t)time(nullptr) : 0; }

// The text size whose body type is `pt` points (bookfonts.h's ladder): the
// screens' own titles ask by size, so a new step on the ladder moves nothing.
static int bkPt(int pt) {
  for (int s = 0; s < BOOK_SIZES; s++)
    if (bookFont(s, BOOK_REGULAR)->pt >= pt) return s;
  return BOOK_SIZES - 1;
}

// ---- fonts and the page's shape

static const BookFont *booksRoleFont(int role) {
  switch (role) {
    case BR_ITALIC: return bookFont(bs->size, BOOK_ITALIC);
    case BR_BOLD: return bookFont(bs->size, BOOK_BOLD);
    case BR_BOLD_ITALIC: return bookFont(bs->size, BOOK_BOLD_ITALIC);
    case BR_H1: return bookHeadingFont(bs->size, 1);
    case BR_H2: return bookHeadingFont(bs->size, 2);
    case BR_H3: return bookHeadingFont(bs->size, 3);
    default: return bookFont(bs->size, BOOK_REGULAR);
  }
}

// The pager's measure of a character: what the canvas prints, but a soft
// hyphen is the pager's to show (only where it breaks a line).
static int booksAdvance(void *, int role, uint32_t cp) {
  if (cp == 0xAD) return 0;
  const BookFont *f = booksRoleFont(role);
  return bookAdvance(f->base, f->ext, cp);
}

static int booksMargin() {
  static const int8_t PORTRAIT[] = {16, 30, 50}, LANDSCAPE[] = {28, 48, 80};
  return (TALL ? PORTRAIT : LANDSCAPE)[bs->margins];
}
static int booksLook() { return bs->justify | bs->spacing << 1 | bs->margins << 3; }  // what else the pages depend on

static void booksLayout(BookLayout &L) {
  L.width = W - 2 * booksMargin();
  L.height = H - BK_TOP - BK_FOOT;
  for (int r = 0; r < BR_COUNT; r++) {
    const BookFont *f = booksRoleFont(r);
    L.font[r].lineHeight = f->yAdvance;
    L.font[r].ascent = f->ascent;
  }
  int em = bookFont(bs->size, BOOK_REGULAR)->pt * 141 / 72;
  L.advance = booksAdvance;
  L.ctx = nullptr;
  static const int16_t SPACING[] = {90, 100, 118};
  L.lineSpacing = SPACING[bs->spacing];
  L.paraIndent = em * 3 / 2;
  L.paraSpace = 0;
  L.quoteIndent = em;
  L.itemIndent = em * 3 / 2;
  L.justify = bs->justify;
  L.centreHeadings = true;
}

// The pages follow the board's turns and the text size.
static void booksFitLayout() {
  if (!bs->pager || bs->pager->status() == BP_CLOSED) return;
  if (bs->layoutW == W && bs->layoutH == H && bs->layoutSize == bs->size && bs->layoutLook == booksLook()) return;
  BookLayout L;
  booksLayout(L);
  uint32_t t = millis();
  bs->pager->resetStats();
  bs->pager->setLayout(L);
  bs->layoutW = W, bs->layoutH = H, bs->layoutSize = bs->size, bs->layoutLook = booksLook();
  const BookPagerStats &st = bs->pager->stats();
  Serial.printf("books: layout %d x %d, size %d, in %lu ms (%lu bytes read, %lu seeks, %lu laid out, %lu pages)\n", L.width, L.height, bs->size,
                (unsigned long)(millis() - t), (unsigned long)st.bytesRead, (unsigned long)st.seeks, (unsigned long)st.bytesLaidOut, (unsigned long)st.pages);
}

// ---- the library

static bool booksLibReady() {
  if (!bs->lib) {
    void *mem = psMalloc(sizeof(BookLibrary));
    if (!mem) return false;
    bs->lib = new (mem) BookLibrary(CARD_ROOT);
  }
  return true;
}

static void booksScan() {
  if (!booksLibReady()) return;
  if (!cardMount()) {
    bs->lib->scanBegin();  // it finds no card and says so (LS_NO_CARD)
    return;
  }
  bs->cardGen = cardGeneration();
  uint32_t t = millis();
  bs->lib->scanBegin();
  Serial.printf("books: %d on the card (listed in %lu ms)\n", bs->lib->count(), (unsigned long)(millis() - t));
}

static const char *booksErrText(int e) {
  switch (e) {
    case BE_OPEN: return T(S_BOOKERR_OPEN);
    case BE_FORMAT: return T(S_BOOKERR_FORMAT);
    case BE_DRM: return T(S_BOOKERR_DRM);
    case BE_ENCRYPTED: return T(S_BOOKERR_ENCRYPTED);
    case BE_NO_TEXT: return T(S_BOOKERR_NO_TEXT);
    case BE_WRITE: return T(S_BOOKERR_WRITE);
    case BE_MEMORY: return T(S_BOOKERR_MEMORY);
    default: return T(S_BOOKERR_DAMAGED);
  }
}

// The first letter of a title, upper case: the small cover in the list.
static String booksInitial(const char *title) {
  for (const char *article : {"The ", "A ", "An "})  // "A Tale of Two Cities": T
    if (!strncasecmp(title, article, strlen(article)) && title[strlen(article)]) title += strlen(article);
  int i = 0;
  uint32_t cp = utf8Next(title, i);
  String s;
  if (cp) utf8Append(s, cp);
  return utf8Upper(s, i18nLang() == L_TR);
}

static void booksMiniCover(int x, int y, int w, int h, const char *title, bool filled) {
  if (filled) canvas->fillRoundRect(x, y, w, h, 4, INK);
  else {
    canvas->drawRoundRect(x, y, w, h, 4, INK);
    canvas->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 3, INK);
  }
  canvas->fillRect(x + 5, y, 3, h, filled ? PAPER : INK);  // the spine
  const BookFont *f = bookFont(bkPt(14), BOOK_BOLD);
  String c = booksInitial(title);
  canvas->setTextColor(filled ? PAPER : INK);
  textCentre(x + w / 2 + 3, y + h / 2 + f->capHeight / 2, f->base, c.c_str());
  canvas->setTextColor(INK);
}

// What kind of book, as its file says: "EPUB", "AZW3", "TXT", "PDF".
static String booksKind(const char *path, int format) {
  const char *dot = strrchr(path, '.');
  if (!dot || strlen(dot) < 2 || strlen(dot) > 9) return bookFormatLabel((BookFormat)format);
  String k = dot + 1;
  k.toUpperCase();
  if (k == "MARKDOWN") k = "MD";
  return k;
}

static void booksProgressBar(int x, int y, int w, int permille, uint16_t ink) {
  canvas->drawRect(x, y, w, 8, ink);
  canvas->fillRect(x, y, constrain(permille, 0, 1000) * w / 1000, 8, ink);
}

static const char *booksSectionName(int status) {
  return T(status == LB_READING ? S_BOOKS_READING : status == LB_NEW ? S_BOOKS_NEW : S_BOOKS_FINISHED);
}

// The list's pages: where each starts, as a place in the list. A section's
// name is never the last line of a page. The first page starts under the
// Continue card.
static int booksContinueH() { return 150; }

static void booksPaginate() {
  BookLibrary &lib = *bs->lib;
  int n = lib.count(), top = BODY_Y + 10, bottom = H - 44;
  bool cont = lib.lastRead() >= 0;
  int y = top + 46 + (cont ? booksContinueH() + 14 : 0), page = 0, prev = -1;  // 46: the statistics line
  bs->pageStart[0] = 0;
  LibBook b;
  for (int i = 0; i < n; i++) {
    if (!lib.book(i, b)) break;
    int need = BK_LIB_ROW + (b.status != prev ? BK_LIB_HEAD : 0);
    if (y + need > bottom && i > bs->pageStart[page] && page + 1 < BK_MAX_PAGES) {
      bs->pageStart[++page] = i;
      y = top;
      need = BK_LIB_ROW + BK_LIB_HEAD;  // a page starts with its section's name
    }
    y += need;
    prev = b.status;
  }
  bs->libPages = page + 1;
  bs->libPage = constrain(bs->libPage, 0, bs->libPages - 1);
}

static int booksPageOf(int index) {
  int p = 0;
  while (p + 1 < bs->libPages && bs->pageStart[p + 1] <= index) p++;
  return p;
}

static void booksFocusRing(int x, int y, int w, int h) {
  for (int k = 0; k < 4; k++) canvas->drawRoundRect(x - 6 - k, y - 4 - k, w + 12 + 2 * k, h + 8 + 2 * k, 10 + k, INK);
}

static void booksDrawEmpty(const char *title, const char *line) {
  int y = BODY_Y + (TALL ? 120 : 70), cx = W / 2;
  // A closed book, big.
  canvas->fillRoundRect(cx - 44, y, 88, 120, 8, INK);
  canvas->fillRect(cx - 32, y, 6, 120, PAPER);
  canvas->fillRect(cx - 18, y + 26, 50, 6, PAPER);
  canvas->fillRect(cx - 18, y + 40, 36, 4, PAPER);
  y += 172;
  textCentre(cx, y, &FreeSansBold18pt7b, title);
  y += 44;
  for (const char *l : {line, T(S_BOOKS_FORMATS), T(S_BOOKS_FREE)}) {
    for (const String &s : wrapped(&FreeSans12pt7b, l, W - 80)) {
      textCentre(cx, y, &FreeSans12pt7b, s.c_str());
      y += 30;
    }
    y += 14;
  }
}

static void booksDrawLibrary() {
  BookLibrary &lib = *bs->lib;
  bs->hitN = 0;
  bs->libGen = lib.generation();
  if (lib.state() == LS_NO_CARD) return booksDrawEmpty(T(S_BOOKS_NO_CARD), T(S_BOOKS_NO_CARD_1));
  if (lib.state() != LS_OK) return booksDrawEmpty(T(S_BOOKS_NONE), T(S_BOOKS_NONE_1));
  booksPaginate();
  int m = TALL ? 24 : 40, top = BODY_Y + 10, y = top;
  LibBook b;
  int cont = lib.lastRead();
  if (bs->libPage == 0) {  // the statistics, one line: tap it for all of them
    LibStats st = lib.libraryStats(booksLocalDay());
    String line = st.totalSecs ? TF(S_BOOKS_STATS_READ, booksDuration(st.totalSecs).c_str()) : String(T(S_BOOKS_STATS_NONE));
    if (st.currentStreakDays) line += "  \xC2\xB7  " + TP(S_BOOKS_DAYS_ROW, st.currentStreakDays);
    for (int k = 0; k < 4; k++) canvas->fillRect(m + k * 7, y + 26 - (k + 1) * 5, 5, (k + 1) * 5, INK);  // a little chart
    text(m + 40, y + 24, &FreeSans12pt7b, fitted(&FreeSans12pt7b, line, W - 2 * m - 70).c_str());
    chevron(W - m, y + 16);
    bs->hits[bs->hitN++] = {(int16_t)(y - 8), (int16_t)(y + 40), -3};
    y += 46;
  }
  if (bs->libPage == 0 && cont >= 0 && lib.book(cont, b)) {  // the Continue card
    int h = booksContinueH();
    canvas->fillRoundRect(m, y, W - 2 * m, h, 14, INK);
    {  // the book, white on the black card
      int cx = m + 18, cy = y + 18, cw = 70, ch = h - 36;
      const BookFont *f = bookFont(bkPt(16), BOOK_BOLD);
      String c = booksInitial(b.title);
      canvas->fillRoundRect(cx, cy, cw, ch, 4, PAPER);
      canvas->fillRect(cx + 6, cy, 3, ch, INK);
      textCentre(cx + cw / 2 + 4, cy + ch / 2 + f->capHeight / 2, f->base, c.c_str());
    }
    canvas->setTextColor(PAPER);
    int tx = m + 110, tw = W - m - 24 - tx;
    text(tx, y + 32, &FreeSans9pt7b, T(S_BOOKS_CONTINUE));
    const BookFont *tf = bookFont(bkPt(TALL ? 14 : 16), BOOK_BOLD);
    text(tx, y + 32 + tf->yAdvance, tf->base, fitted(tf->base, b.title, tw).c_str());
    if (b.author[0]) text(tx, y + 32 + tf->yAdvance + 28, &FreeSans12pt7b, fitted(&FreeSans12pt7b, b.author, tw).c_str());
    char pct[16];
    snprintf(pct, sizeof pct, T(S_BOOKS_PERCENT), b.permille / 10);
    int pw = textW(&FreeSansBold12pt7b, pct);
    for (int k = 0; k < 2; k++) canvas->drawRect(tx + k, y + h - 34 + k, tw - pw - 16 - 2 * k, 12 - 2 * k, PAPER);
    canvas->fillRect(tx, y + h - 34, (tw - pw - 16) * b.permille / 1000, 12, PAPER);
    textRight(tx + tw, y + h - 22, &FreeSansBold12pt7b, pct);
    canvas->setTextColor(INK);
    if (bs->focus == -1) booksFocusRing(m, y, W - 2 * m, h);
    bs->hits[bs->hitN++] = {(int16_t)y, (int16_t)(y + h), -1};
    y += h + 14;
  }
  int first = bs->pageStart[bs->libPage], last = bs->libPage + 1 < bs->libPages ? bs->pageStart[bs->libPage + 1] : lib.count();
  int prev = -1;
  if (first > 0 && lib.book(first - 1, b)) prev = b.status;
  if (bs->libPage > 0) prev = -1;  // every page names its section
  char line[64];
  for (int i = first; i < last; i++) {
    if (!lib.book(i, b)) break;
    if (b.status != prev) {  // the section's name and how many
      snprintf(line, sizeof line, "%s  %d", booksSectionName(b.status), lib.count((LibStatus)b.status));
      text(m, y + 30, &FreeSansBold12pt7b, line);
      canvas->drawFastHLine(m, y + BK_LIB_HEAD - 6, W - 2 * m, INK);
      y += BK_LIB_HEAD;
      prev = b.status;
    }
    int rh = BK_LIB_ROW;
    booksMiniCover(m, y + 8, 48, rh - 16, b.title, b.status == LB_READING);
    int tx = m + 66, right = W - m;
    // On the right: how far, or done.
    if (b.status == LB_READING) snprintf(line, sizeof line, T(S_BOOKS_PERCENT), b.permille / 10);
    else line[0] = 0;
    int rw = line[0] ? textW(&FreeSansBold12pt7b, line) + 12 : 0;
    if (line[0]) textRight(right, y + rh / 2 + 8, &FreeSansBold12pt7b, line);
    if (b.status == LB_FINISHED) {  // a tick
      thickLine(right - 26, y + rh / 2, right - 16, y + rh / 2 + 10, 3);
      thickLine(right - 16, y + rh / 2 + 10, right, y + rh / 2 - 10, 3);
      rw = 40;
    }
    const BookFont *tf = bookFont(bkPt(12), BOOK_BOLD);
    text(tx, y + 34, tf->base, fitted(tf->base, b.title, right - rw - tx).c_str());
    String meta = b.author[0] ? String(b.author) + "  \xC2\xB7  " : String();
    meta += booksKind(b.path, b.format);
    if (b.error) meta += "  !";
    text(tx, y + 62, &FreeSans9pt7b, fitted(&FreeSans9pt7b, meta, right - rw - tx).c_str());
    if (bs->focus == i) booksFocusRing(m, y + 4, W - 2 * m, rh - 8);
    if (bs->hitN < 24) bs->hits[bs->hitN++] = {(int16_t)y, (int16_t)(y + rh), (int16_t)i};
    y += rh;
  }
  // The foot: how many, the page, or the scan going on.
  if (lib.scanning()) snprintf(line, sizeof line, T(S_BOOKS_LOOKING), lib.scanDone(), lib.scanTotal());
  else utf8Copy(line, TP(S_BOOKS_COUNT, lib.count()).c_str(), sizeof line);
  text(m, H - 14, &FreeSans9pt7b, line);
  if (bs->libPages > 1) {
    snprintf(line, sizeof line, T(S_BOOKS_PAGE_OF), bs->libPage + 1, bs->libPages);
    textRight(W - m, H - 14, &FreeSansBold9pt7b, line);
  }
  if (lib.truncated()) textCentre(W / 2, H - 14, &FreeSans9pt7b, T(S_BOOKS_TOO_MANY));
}

// ---- opening a book

static void booksStartReading();

static void booksOpen(int index) {
  if (!booksLibReady() || index < 0 || index >= bs->lib->count()) return;
  if (bs->pager) bs->pager->close();
  LibBook b;
  bs->lib->book(index, b);
  bs->openTitle = b.title;
  bs->openAuthor = b.author;
  Serial.printf("books: opening \"%s\" (%s)\n", b.title, b.path);
  if (!bs->lib->openBegin(index, booksNow())) {
    bs->openErr = bs->lib->openError();
    bs->mode = BM_ERROR;
    return;
  }
  bs->mode = BM_OPENING;
  bs->openProgress = 0;
  bs->openDrawn = -1;
  bs->drawnAt = 0;
  // Already converted: it's ready at once.
  int p = bs->lib->openStep();
  if (p == BOOK_DONE) return booksStartReading();
  if (p < 0) bs->openErr = bs->lib->openError(), bs->mode = BM_ERROR;
  else bs->openProgress = p;
}

static void booksStartReading() {
  BookLibrary &lib = *bs->lib;
  if (!bs->pager) {
    void *mem = psMalloc(sizeof(BookPager));
    bs->pager = mem ? new (mem) BookPager() : nullptr;
  }
  BookLayout L;
  booksLayout(L);
  uint32_t t = millis();
  if (!bs->pager || !bs->pager->open(lib.flowPath(), L, lib.position())) {
    bs->openErr = BE_OPEN;
    bs->mode = BM_ERROR;
    return;
  }
  bs->layoutW = W, bs->layoutH = H, bs->layoutSize = bs->size, bs->layoutLook = booksLook();
  bs->openTitle = lib.openTitle();
  bs->openAuthor = lib.openAuthor();
  bs->cover = lib.position() == 0;
  bs->turns = 0;
  bs->note = "";
  bs->mode = BM_READING;
  bs->turnAt = millis(), bs->pendMs = bs->pendPages = bs->pendBytes = 0;
  if (bs->toOpenToc >= 0) {  // a chapter tapped on the book's page
    bs->cover = false;
    bs->pager->gotoToc(bs->toOpenToc);
    bs->toOpenToc = -1;
  }
  prefs.putString("bopen", lib.openKey());  // a restart or a wake opens it again
  Serial.printf("books: reading \"%s\" at %lu of %lu (page in %lu ms, %d contents entries)\n", lib.openTitle(),
                (unsigned long)lib.position(), (unsigned long)lib.flowSize(), (unsigned long)(millis() - t), bs->pager->tocCount());
}

static void booksToLibrary() {
  booksSync();
  if (bs->lib && bs->lib->isOpen()) bs->lib->closeBook();
  if (bs->pager) bs->pager->close();
  prefs.remove("bopen");
  bs->mode = BM_LIBRARY;
  bs->focus = -2;
  bs->libPage = 0;
  booksScan();
}

static void booksDrawOpening() {
  int m = TALL ? 40 : 80, y = BODY_Y + (TALL ? 140 : 70);
  const BookFont *tf = bookFont(bkPt(TALL ? 18 : 21), BOOK_BOLD);
  for (const String &l : wrapped(tf->base, bs->openTitle, W - 2 * m)) {
    textCentre(W / 2, y, tf->base, l.c_str());
    y += tf->yAdvance;
  }
  if (bs->openAuthor.length()) {
    const BookFont *af = bookFont(bkPt(14), BOOK_ITALIC);
    textCentre(W / 2, y + 10, af->base, fitted(af->base, bs->openAuthor, W - 2 * m).c_str());
    y += af->yAdvance + 10;
  }
  y += 50;
  char line[64];
  snprintf(line, sizeof line, "%s  %d%%", T(S_BOOKS_PREPARING), bs->openProgress / 10);
  textCentre(W / 2, y, &FreeSansBold12pt7b, line);
  y += 22;
  canvas->drawRoundRect(m, y, W - 2 * m, 18, 9, INK);
  canvas->fillRoundRect(m, y, max(18, (W - 2 * m) * bs->openProgress / 1000), 18, 9, INK);
  y += 56;
  for (const String &l : wrapped(&FreeSans12pt7b, T(S_BOOKS_PREPARING_NOTE), W - 2 * m)) {
    textCentre(W / 2, y, &FreeSans12pt7b, l.c_str());
    y += 28;
  }
  int bw = 220;
  pillButton(W / 2 - bw / 2, H - 100, bw, 60, T(S_G_CANCEL), false, &FreeSansBold12pt7b);
}

static void booksDrawError() {
  int m = TALL ? 40 : 80, y = BODY_Y + (TALL ? 120 : 60);
  textCentre(W / 2, y, &FreeSansBold18pt7b, T(S_BOOKS_CANT_OPEN));
  y += 50;
  const BookFont *tf = bookFont(bkPt(14), BOOK_ITALIC);
  for (const String &l : wrapped(tf->base, bs->openTitle, W - 2 * m)) {
    textCentre(W / 2, y, tf->base, l.c_str());
    y += tf->yAdvance;
  }
  y += 30;
  for (const String &l : wrapped(&FreeSans12pt7b, booksErrText(bs->openErr), W - 2 * m)) {
    textCentre(W / 2, y, &FreeSans12pt7b, l.c_str());
    y += 30;
  }
  int bw = min(W - 80, 320);
  pillButton(W / 2 - bw / 2, H - 110, bw, 64, T(S_BOOKS_BACK_TO_LIST), true, &FreeSansBold12pt7b);
}

// ---- the reader

static bool booksReading() { return bs && (bs->mode == BM_READING || bs->mode == BM_MENU || bs->mode == BM_TEXT); }

// The reader has the whole screen; its menu and the lists show the status bar.
bool booksFullScreen() { return bs && bs->mode == BM_READING; }

static void booksRibbon(int x, int y, int w, int h, uint16_t ink) {  // a bookmark: a strip with a notch
  canvas->fillRect(x, y, w, h - w / 2, ink);
  canvas->fillTriangle(x, y + h - w / 2, x + w - 1, y + h - w / 2, x, y + h, ink);
  canvas->fillTriangle(x + w - 1, y + h - w / 2, x + w - 1, y + h, x, y + h - w / 2, ink);
  canvas->fillTriangle(x + 1, y + h, x + w - 2, y + h, x + w / 2, y + h - w / 2, ink == INK ? PAPER : INK);
}

static void booksDrawPageText() {
  const BookPage &pg = bs->pager->page();
  int left = booksMargin();
  canvas->setTextWrap(false);  // a glyph touching the edge must stay on its line
  for (const PageLine &ln : pg.lines)
    for (uint32_t i = 0; i < ln.runCount; i++) {
      const PageRun &r = pg.runs[ln.firstRun + i];
      const BookFont *f = booksRoleFont(r.role);
      text(left + r.x, BK_TOP + ln.baseline, f->base, pg.text(r));
    }
  for (const PageRule &r : pg.rules) {
    canvas->drawFastHLine(left + r.x, BK_TOP + r.y, r.width, INK);
    canvas->drawFastHLine(left + r.x, BK_TOP + r.y + 1, r.width, INK);
  }
  canvas->setTextWrap(true);
}

// The header line: the chapter on the left, the time and the battery on the right.
static void booksDrawHeader(bool marked) {
  int m = booksMargin(), right0 = W - m - (marked ? 34 : 0);
  char right[24] = "";
  if (timeKnown()) {
    time_t t = time(nullptr);
    strftime(right, sizeof right, "%H:%M", localtime(&t));
  }
  if (now.battery) snprintf(right + strlen(right), sizeof right - strlen(right), "%s%d%%", right[0] ? "   " : "", now.percent);
  int rw = textW(&FreeSans9pt7b, right);
  textRight(right0, 26, &FreeSans9pt7b, right);
  const char *ch = bs->pager->chapterTitle();
  String title = ch && ch[0] ? String(ch) : bs->openTitle;
  text(m, 26, &FreeSans9pt7b, fitted(&FreeSans9pt7b, title, right0 - m - rw - 24).c_str());
}

static void booksDrawFoot() {
  BookPager &p = *bs->pager;
  int m = booksMargin();
  char pct[12];
  snprintf(pct, sizeof pct, T(S_BOOKS_PERCENT), p.atEnd() ? 100 : p.permille() / 10);
  textRight(W - m, H - 12, &FreeSans9pt7b, pct);
  String left;
  if (bs->note.length()) left = bs->note;
  else if (!bs->hintSeen) left = T(S_BOOKS_HINT);
  else if (p.atEnd()) left = T(S_BOOKS_THE_END);
  else if (p.pagesLeftInChapter() == 0) left = T(S_BOOKS_LAST_IN_CHAPTER);
  else if (booksChapterTimeLeft().length()) left = TF(S_BOOKS_MIN_LEFT_CHAPTER, booksChapterTimeLeft().c_str());
  else if (p.pagesLeftInChapter() > 0) left = TP(S_BOOKS_LEFT_CHAPTER, p.pagesLeftInChapter());
  text(m, H - 12, &FreeSans9pt7b, fitted(&FreeSans9pt7b, left, W - 2 * m - textW(&FreeSans9pt7b, pct) - 20).c_str());
}

// The cover: the book's own picture in four greys, or one set in type.
// Returns true when it went to the panel itself (the grey picture).
static bool booksDrawCover(bool fs) {
  BookLibrary &lib = *bs->lib;
  int top = fs ? 0 : BODY_Y;
  bs->coverGrey = false;
  if (lib.coverType() == 'j' && photoBuffer == nullptr) photoBuffer = (uint8_t *)heap_caps_malloc(NW / 4 * NW, MALLOC_CAP_SPIRAM);
  if (lib.coverType() == 'j' && photoBuffer) {
    const char *why = "";
    uint32_t t = millis();
    if (jpegRender(lib.coverPath(), photoBuffer, W, H - top, true, &why)) {
      Serial.printf("books: cover drawn in %lu ms\n", (unsigned long)(millis() - t));
      bs->coverGrey = true;
      showGrey(photoBuffer, top);
      return true;
    }
    Serial.printf("books: cover: %s\n", why);
  }
  // Set in type: the title white on black, the author under it.
  int m = TALL ? 36 : 60, bandH = (H - top) * 58 / 100;
  canvas->fillRect(0, top, W, bandH, INK);
  canvas->setTextColor(PAPER);
  const BookFont *tf = bookHeadingFont(bkPt(TALL ? 18 : 21), 1);
  std::vector<String> lines = wrapped(tf->base, bs->openTitle, W - 2 * m);
  if (lines.size() > 4) lines.resize(4);
  int y = top + bandH - 50 - (int)(lines.size() - 1) * tf->yAdvance;
  for (const String &l : lines) {
    textCentre(W / 2, y, tf->base, l.c_str());
    y += tf->yAdvance;
  }
  canvas->setTextColor(INK);
  const BookFont *af = bookFont(bkPt(TALL ? 16 : 18), BOOK_ITALIC);
  if (bs->openAuthor.length()) textCentre(W / 2, top + bandH + 70, af->base, fitted(af->base, bs->openAuthor, W - 2 * m).c_str());
  canvas->drawFastHLine(W / 2 - 40, top + bandH + 110, 80, INK);
  return false;
}

// The menu's toolbar: four buttons with a picture and a word.
static void booksToolIcon(int k, int cx, int cy) {
  switch (k) {
    case 0:  // Library: books on a shelf
      canvas->fillRect(cx - 20, cy - 16, 9, 32, INK);
      canvas->fillRect(cx - 8, cy - 20, 9, 36, INK);
      for (int i = 0; i < 3; i++) thickLine(cx + 6 + i, cy - 14, cx + 16 + i, cy + 15, 2);
      canvas->fillRect(cx - 24, cy + 17, 48, 3, INK);
      break;
    case 1:  // Contents: a list
      for (int i = 0; i < 4; i++) {
        canvas->fillCircle(cx - 16, cy - 15 + i * 10, 2, INK);
        canvas->fillRect(cx - 9, cy - 16 + i * 10, i % 2 ? 22 : 28, 3, INK);
      }
      break;
    case 2: booksRibbon(cx - 10, cy - 20, 20, 38, INK); break;
    default: {  // Text: "Aa"
      const BookFont *big = bookFont(bkPt(18), BOOK_REGULAR), *small = bookFont(bkPt(12), BOOK_REGULAR);
      int w = textW(big->base, "A") + textW(small->base, "a");
      text(cx - w / 2, cy + big->capHeight / 2, big->base, "A");
      text(cx - w / 2 + textW(big->base, "A") + 1, cy + big->capHeight / 2, small->base, "a");
    }
  }
}

static int booksMenuBottom() { return bs->mode == BM_TEXT ? H - 316 : H - (TALL ? 236 : 212); }  // where the menu's foot panel starts

// The Text panel's rows: the sizes, then line spacing, margins and the switch.
static int bkTextSizesY() { return booksMenuBottom() + 56; }
static const int BK_TEXT_SIZE_H = 64, BK_TEXT_ROW = 52;
static int bkTextRowY(int row) { return bkTextSizesY() + BK_TEXT_SIZE_H + 14 + row * BK_TEXT_ROW; }  // a row's top
static const int BK_SEG_W = 62, BK_SEG_H = 42;
static int bkSegX(int k) { return W - booksMargin() - (3 - k) * (BK_SEG_W + 8) + 8; }

// Three choices on a row: tight, normal, loose lines; narrow, normal, wide margins.
static void booksSegments(int row, int chosen) {
  int y = bkTextRowY(row) + (BK_TEXT_ROW - BK_SEG_H) / 2;
  for (int k = 0; k < 3; k++) {
    int x = bkSegX(k);
    bool on = k == chosen;
    uint16_t fg = on ? PAPER : INK;
    if (on) canvas->fillRoundRect(x, y, BK_SEG_W, BK_SEG_H, 10, INK);
    else canvas->drawRoundRect(x, y, BK_SEG_W, BK_SEG_H, 10, INK), canvas->drawRoundRect(x + 1, y + 1, BK_SEG_W - 2, BK_SEG_H - 2, 9, INK);
    if (row == 0) {  // lines further apart
      int gap = 4 + k * 3, top = y + BK_SEG_H / 2 - gap - 1;
      for (int l = 0; l < 3; l++) canvas->fillRect(x + 16, top + l * gap, BK_SEG_W - 32, 2, fg);
    } else {  // a page with its text further in
      int in = 4 + k * 5;
      canvas->drawRect(x + 14, y + 8, BK_SEG_W - 28, BK_SEG_H - 16, fg);
      for (int l = 0; l < 3; l++) canvas->fillRect(x + 14 + in / 2 + 1, y + 13 + l * 6, BK_SEG_W - 28 - in - 2, 2, fg);
    }
  }
}

static int booksBarX0() { return booksMargin(); }
static int booksBarX1() { return W - booksMargin(); }
static int booksBarY() { return booksMenuBottom() + (TALL ? 76 : 86); }

static void booksDrawMenu() {
  BookPager &p = *bs->pager;
  int m = booksMargin();
  // The title band under the status bar.
  canvas->fillRect(0, BAR_H, W, BK_MENU_TOP, PAPER);
  const BookFont *tf = bookFont(bkPt(14), BOOK_BOLD);
  text(m, BAR_H + 34, tf->base, fitted(tf->base, bs->openTitle, W - 2 * m).c_str());
  if (bs->openAuthor.length()) text(m, BAR_H + 62, &FreeSans9pt7b, fitted(&FreeSans9pt7b, bs->openAuthor, W - 2 * m).c_str());
  canvas->drawFastHLine(0, BAR_H + BK_MENU_TOP - 2, W, INK);
  canvas->drawFastHLine(0, BAR_H + BK_MENU_TOP - 1, W, INK);
  // The foot: the chapter, the page, the progress bar, the buttons.
  int y = booksMenuBottom();
  canvas->fillRect(0, y, W, H - y, PAPER);
  canvas->drawFastHLine(0, y, W, INK);
  canvas->drawFastHLine(0, y + 1, W, INK);
  if (bs->mode == BM_TEXT) {
    text(m, y + 38, &FreeSansBold12pt7b, T(S_BOOKS_SIZE));
    pillButton(W - m - 140, y + 10, 140, 44, T(S_BOOKS_DONE), true, &FreeSansBold12pt7b);
    int n = BOOK_SIZES, gap = 8, bw = (W - 2 * m - gap * (n - 1)) / n, by = bkTextSizesY(), bh = BK_TEXT_SIZE_H;
    for (int s = 0; s < n; s++) {
      int bx = m + s * (bw + gap);
      bool on = s == bs->size;
      if (on) canvas->fillRoundRect(bx, by, bw, bh, 12, INK);
      else canvas->drawRoundRect(bx, by, bw, bh, 12, INK), canvas->drawRoundRect(bx + 1, by + 1, bw - 2, bh - 2, 11, INK);
      const BookFont *f = bookFont(s, BOOK_REGULAR);
      canvas->setTextColor(on ? PAPER : INK);
      textCentre(bx + bw / 2, by + bh / 2 + f->capHeight / 2, f->base, "A");
      canvas->setTextColor(INK);
    }
    text(m, bkTextRowY(0) + 33, &FreeSans12pt7b, T(S_BOOKS_SPACING));
    booksSegments(0, bs->spacing);
    text(m, bkTextRowY(1) + 33, &FreeSans12pt7b, T(S_BOOKS_MARGINS));
    booksSegments(1, bs->margins);
    int sy = bkTextRowY(2) + 34;
    text(m, sy, &FreeSans12pt7b, T(S_BOOKS_JUSTIFY));
    int sw = 64, sx = W - m - sw;  // a switch
    canvas->fillRoundRect(sx, sy - 24, sw, 32, 16, bs->justify ? INK : PAPER);
    canvas->drawRoundRect(sx, sy - 24, sw, 32, 16, INK);
    canvas->fillCircle(bs->justify ? sx + sw - 16 : sx + 16, sy - 8, 12, bs->justify ? PAPER : INK);
    return;
  }
  const char *ch = p.chapterTitle();
  if (ch && ch[0]) text(m, y + 34, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, ch, W - 2 * m).c_str());
  char line[64];
  if (p.estimatedPages() > 0) snprintf(line, sizeof line, T(S_BOOKS_PAGE_ABOUT), p.estimatedPageNumber(), p.estimatedPages());
  else line[0] = 0;
  text(m, y + 60, &FreeSans9pt7b, line);
  snprintf(line, sizeof line, T(S_BOOKS_PERCENT), p.atEnd() ? 100 : p.permille() / 10);
  textRight(W - m, y + 60, &FreeSansBold9pt7b, line);
  // The progress bar: a tick at each chapter, a knob where the reader is.
  int x0 = booksBarX0(), x1 = booksBarX1(), by = booksBarY();
  canvas->fillRect(x0, by - 2, x1 - x0, 4, INK);
  uint32_t size = max<uint32_t>(1, p.flowSize());
  for (int i = 0; i < p.tocCount(); i++) {
    BookTocItem it = p.toc(i);
    if (it.level != 1) continue;
    int x = x0 + (int)((uint64_t)it.offset * (x1 - x0) / size);
    canvas->fillRect(x, by - 9, 2, 18, INK);
  }
  int kx = x0 + (p.atEnd() ? 1000 : p.permille()) * (x1 - x0) / 1000;
  canvas->fillCircle(kx, by, 12, INK);
  canvas->fillCircle(kx, by, 5, PAPER);
  // The buttons.
  int ty = by + (TALL ? 34 : 22), bw = (W - 2 * m) / 4;
  const StrId labels[] = {S_BOOKS_LIBRARY, S_BOOKS_CONTENTS, S_BOOKS_BOOKMARKS, S_BOOKS_TEXT};
  for (int k = 0; k < 4; k++) {
    int cx = m + bw * k + bw / 2;
    booksToolIcon(k, cx, ty + 26);
    textCentre(cx, ty + 78, &FreeSans9pt7b, fitted(&FreeSans9pt7b, T(labels[k]), bw - 6).c_str());
  }
}

// Draws the reader (and the cover). True: it went to the panel by itself.
static bool booksDrawReader(bool fs) {
  uint32_t t0 = millis();
  struct Timer { uint32_t t0; ~Timer() { bkDrawMs = millis() - t0; } } timer{t0};
  booksFitLayout();
  BookPager &p = *bs->pager;
  if (p.status() == BP_IO) {
    bs->openErr = BE_OPEN;
    bs->errText = T(S_BOOKS_CARD_OUT);
  }
  if (bs->cover && bs->mode == BM_READING) return booksDrawCover(fs);
  bool marked = bs->lib->markIn(p.page().start, p.page().end) >= 0;
  if (bs->mode == BM_READING) booksDrawHeader(marked);
  if (p.status() == BP_OK) booksDrawPageText();
  else textCentre(W / 2, H / 2, &FreeSansBold12pt7b, T(S_BOOKS_CARD_OUT));
  if (marked && bs->mode == BM_READING) booksRibbon(W - 16 - 20, 0, 20, 40, INK);  // in the corner, over the header line's end
  if (bs->mode == BM_READING) booksDrawFoot();
  else booksDrawMenu();
  return false;
}

// ---- contents and bookmarks: a list with the status bar

static int booksListTop() { return BODY_Y + 64; }
static int booksListRows() { return max(1, (H - booksListTop() - 70) / BK_LIST_ROW); }

static void booksDrawList() {
  int m = TALL ? 24 : 40, y = BODY_Y + 44;
  bool toc = bs->mode == BM_TOC;
  BookPager &p = *bs->pager;
  text(m, y, &FreeSansBold18pt7b, T(toc ? S_BOOKS_CONTENTS : S_BOOKS_BOOKMARKS));
  int n = toc ? p.tocCount() : bs->lib->markCount(), rows = booksListRows(), pages = max(1, (n + rows - 1) / rows);
  bs->listPage = constrain(bs->listPage, 0, pages - 1);
  if (!n) {
    int ly = booksListTop() + 40;
    for (const String &l : wrapped(&FreeSans12pt7b, T(toc ? S_BOOKS_NO_CONTENTS : S_BOOKS_NO_MARKS), W - 2 * m)) {
      text(m, ly, &FreeSans12pt7b, l.c_str());
      ly += 30;
    }
  }
  int cur = toc ? p.currentToc() : -1;
  char line[24];
  for (int r = 0; r < rows; r++) {
    int i = bs->listPage * rows + r;
    if (i >= n) break;
    int ry = booksListTop() + r * BK_LIST_ROW;
    if (toc) {
      BookTocItem it = p.toc(i);
      int x = m + 26 + (it.level - 1) * 28;
      const GFXfont *f = it.level == 1 || i == cur ? &FreeSansBold12pt7b : &FreeSans12pt7b;
      if (i == cur) canvas->fillCircle(m + 8, ry + BK_LIST_ROW / 2 - 4, 7, INK);
      snprintf(line, sizeof line, "%d%%", (int)((uint64_t)it.offset * 100 / max<uint32_t>(1, p.flowSize())));
      int pw = textW(&FreeSans9pt7b, line);
      textRight(W - m, ry + BK_LIST_ROW / 2 + 4, &FreeSans9pt7b, line);
      text(x, ry + BK_LIST_ROW / 2 + 4, f, fitted(f, it.title, W - m - pw - 16 - x).c_str());
    } else {
      const LibMark &mk = bs->lib->mark(i);
      booksRibbon(m, ry + 12, 16, 30, INK);
      snprintf(line, sizeof line, "%d%%", (int)((uint64_t)mk.offset * 100 / max<uint32_t>(1, p.flowSize())));
      // The X that removes it, on the right.
      int xr = W - m;
      thickLine(xr - 20, ry + 20, xr - 4, ry + 36, 3);
      thickLine(xr - 4, ry + 20, xr - 20, ry + 36, 3);
      int pw = textW(&FreeSans9pt7b, line);
      textRight(xr - 44, ry + BK_LIST_ROW / 2 + 4, &FreeSans9pt7b, line);
      text(m + 32, ry + BK_LIST_ROW / 2 + 4, &FreeSans12pt7b, fitted(&FreeSans12pt7b, mk.label[0] ? mk.label : "...", xr - 60 - pw - m - 32).c_str());
    }
    if (r + 1 < rows && i + 1 < n) canvas->drawFastHLine(m, ry + BK_LIST_ROW - 1, W - 2 * m, INK);
  }
  pillButton(m, H - 62, 150, 50, T(S_BACK), false, &FreeSansBold12pt7b);
  if (pages > 1) {
    snprintf(line, sizeof line, "%d / %d", bs->listPage + 1, pages);
    textRight(W - m, H - 30, &FreeSansBold12pt7b, line);
  }
}


// ---- time, for the statistics

// Local days since 1970 (the library's day log), -1 while the clock is unknown.
static int32_t booksLocalDay() {
  if (!timeKnown()) return -1;
  time_t t = time(nullptr);
  struct tm lt;
  localtime_r(&t, &lt);
  int y = lt.tm_year + 1900, m = lt.tm_mon + 1, d = lt.tm_mday;
  y -= m <= 2;  // days from the civil date (H. Hinnant's algorithm)
  int era = (y >= 0 ? y : y - 399) / 400, yoe = y - era * 400;
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

// "2 h 15 min", "40 min", "under a minute".
static String booksDuration(uint32_t secs) {
  if (!secs) return TF(S_BOOKS_MIN, 0);
  if (secs < 60) return T(S_BOOKS_LESS_MIN);
  if (secs < 3600) return TF(S_BOOKS_MIN, (int)(secs / 60));
  return TF(S_BOOKS_HOURS_MIN, (int)(secs / 3600), (int)(secs / 60 % 60));
}

static String booksDate(uint32_t epoch) {
  char d[48];
  time_t t = epoch;
  tStrftime(d, sizeof d, "%e %B", localtime(&t));
  String s = d;
  s.trim();
  return s;
}

// The time since the last page turn, counted as reading; a long gap (the
// board put down, the reader asleep at it) counts as a minute.
static void booksClock() {
  uint32_t t = millis();
  if (bs->turnAt) {
    uint32_t dt = t - bs->turnAt;
    bs->pendMs += dt > 5 * 60000UL ? 60000UL : dt;
  }
  bs->turnAt = t;
}

// What was read since the last call, to the library (it keeps it with the place).
static void booksGiveReading() {
  if (!bs->lib || !bs->lib->isOpen()) return;
  if (bs->pendMs < 1000 && !bs->pendPages) return;
  bs->lib->addReading(bs->pendMs / 1000, bs->pendPages, bs->pendBytes, booksNow(), booksLocalDay());
  bs->pendMs %= 1000, bs->pendPages = bs->pendBytes = 0;
}

// The time left in the chapter at the reader's pace ("12 min"), or "" until the pace is known.
static String booksChapterTimeLeft() {
  BookPager &p = *bs->pager;
  uint32_t pace = bs->lib->bookStats().paceBytesPerMin();
  if (!pace) return String();
  uint32_t end = p.flowSize(), at = p.page().start;
  for (int i = 0; i < p.tocCount(); i++) {
    uint32_t o = p.toc(i).offset;
    if (o > at) {
      end = o;
      break;
    }
  }
  return booksDuration((uint32_t)((uint64_t)(end - at) * 60 / pace));
}

// ---- a book's page: its cover, what it's about, how far, its chapters

static void booksLoadDetailToc(const char *flowPath) {
  bs->dOff.clear(), bs->dTitle.clear(), bs->dLevel.clear(), bs->dPool.clear();
  bs->dCurrent = -1;
  String path = String(flowPath) + ".toc";
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) return;
  char line[300];
  while (bs->dOff.size() < 2000 && fgets(line, sizeof line, f)) {
    char *tab1 = strchr(line, '\t'), *tab2 = tab1 ? strchr(tab1 + 1, '\t') : nullptr;
    if (!tab2) continue;
    char *end = tab2 + 1 + strcspn(tab2 + 1, "\r\n");
    *end = 0;
    uint32_t off = strtoul(line, nullptr, 10);
    if (off <= bs->info.position) bs->dCurrent = bs->dOff.size();
    bs->dOff.push_back(off);
    bs->dLevel.push_back((uint8_t)constrain(atoi(tab1 + 1), 1, 3));
    bs->dTitle.push_back(bs->dPool.size());
    bs->dPool.insert(bs->dPool.end(), tab2 + 1, end + 1);
  }
  fclose(f);
}

void booksShowDetails(int index) {
  if (!bs->lib || !bs->lib->bookInfo(index, bs->info)) return;
  bs->detail = index;
  bs->detailPage = 0;
  bs->mode = BM_DETAILS;
  if (bs->info.converted) booksLoadDetailToc(bs->info.flowPath.c_str());
  else bs->dOff.clear(), bs->dTitle.clear(), bs->dLevel.clear(), bs->dPool.clear(), bs->dCurrent = -1;
  Serial.printf("books: page of \"%s\": %s, %d chapters, %lu s read\n", bs->info.title.c_str(), bs->info.converted ? "ready" : "not opened yet",
                (int)bs->dOff.size(), (unsigned long)bs->info.stats.secs);
  render(false);
}

// What the page holds, top to bottom; it's cut into screens (swipe up for more).
enum { DI_HEAD, DI_BUTTON, DI_PROGRESS, DI_STATS, DI_HEADING, DI_TEXT, DI_GAP, DI_CHAPTER, DI_NOTE };
struct DItem { uint8_t kind; int16_t h; int16_t ref; };  // ref: a heading's or note's string, a line, a chapter

static const BookFont *booksDescFont() { return bookFont(bkPt(14), BOOK_REGULAR); }

// The page's items and its description's lines, made afresh at each drawing.
// (Statics, not parameters: Arduino writes every function's prototype above
// this file's types.)
static std::vector<DItem> bkItems;
static std::vector<String> bkLines;

static void booksDetailItems() {
  std::vector<DItem> &items = bkItems;
  std::vector<String> &lines = bkLines;
  items.clear(), lines.clear();
  const LibInfo &in = bs->info;
  int m = TALL ? 24 : 40;
  bool started = in.position > 0 || in.finished || in.stats.secs > 0;
  items.push_back({DI_HEAD, (int16_t)(TALL ? 214 : 190), 0});
  items.push_back({DI_BUTTON, 86, 0});
  if (started) items.push_back({DI_PROGRESS, 64, 0});
  if (in.stats.secs > 0 || in.stats.pages > 0) items.push_back({DI_STATS, (int16_t)(TALL ? 150 : 84), 0});
  const BookFont *df = booksDescFont();
  if (in.description.size()) {  // a book that doesn't say what it's about has no such part
    items.push_back({DI_HEADING, 52, (int16_t)S_BOOKS_ABOUT});
    String d = in.description.c_str();
    int at = 0;
    while (at <= (int)d.length()) {
      int nl = d.indexOf('\n', at);
      String para = d.substring(at, nl < 0 ? d.length() : nl);
      for (const String &l : wrapped(df->base, para, W - 2 * m)) {
        items.push_back({DI_TEXT, (int16_t)df->yAdvance, (int16_t)lines.size()});
        lines.push_back(l);
      }
      items.push_back({DI_GAP, 10, 0});
      if (nl < 0) break;
      at = nl + 1;
    }
  }
  items.push_back({DI_HEADING, 52, (int16_t)S_BOOKS_CHAPTERS});
  if (!in.converted) items.push_back({DI_NOTE, 64, (int16_t)S_BOOKS_CHAPTERS_LATER});
  else if (bs->dOff.empty()) items.push_back({DI_NOTE, 40, (int16_t)S_BOOKS_NO_CONTENTS});
  for (int i = 0; i < (int)bs->dOff.size(); i++) items.push_back({DI_CHAPTER, 54, (int16_t)i});
}

// The cover beside the title: the book's picture, put in afterwards in four
// greys (booksDetailsGrey), or one set in type.
static int bkCoverX, bkCoverY, bkCoverW, bkCoverH;

static void booksDrawDetailHead(int y) {
  const LibInfo &in = bs->info;
  int m = TALL ? 24 : 40, cw = 120, ch = TALL ? 180 : 160;
  bkCoverW = 0;
  if (in.converted && in.coverType == 'j') {
    bkCoverX = m, bkCoverY = y, bkCoverW = cw, bkCoverH = ch;
    canvas->drawRect(m - 1, y - 1, cw + 2, ch + 2, INK);
  } else {  // in type: the title white on black
    canvas->fillRoundRect(m, y, cw, ch, 6, INK);
    canvas->fillRect(m + 8, y, 3, ch, PAPER);
    canvas->setTextColor(PAPER);
    std::vector<String> t = wrapped(&FreeSansBold9pt7b, in.title.c_str(), cw - 26);
    for (size_t i = 0; i < t.size() && i < 5; i++) text(m + 18, y + 30 + i * 22, &FreeSansBold9pt7b, t[i].c_str());
    std::vector<String> a = wrapped(&FreeSans9pt7b, in.author.c_str(), cw - 26);
    for (size_t i = 0; i < a.size() && i < 2; i++) text(m + 18, y + ch - 34 + i * 20, &FreeSans9pt7b, a[i].c_str());
    canvas->setTextColor(INK);
  }
  int x = m + cw + 20, w = W - m - x, ty = y + 30;
  const BookFont *tf = bookFont(bkPt(TALL ? 14 : 16), BOOK_BOLD);
  std::vector<String> t = wrapped(tf->base, in.title.c_str(), w);
  for (size_t i = 0; i < t.size() && i < 3; i++, ty += tf->yAdvance) text(x, ty, tf->base, t[i].c_str());
  if (in.author.size()) {
    text(x, ty + 4, &FreeSans12pt7b, fitted(&FreeSans12pt7b, in.author.c_str(), w).c_str());
    ty += 34;
  }
  String kind = booksKind(in.path.c_str(), in.format);
  String size = in.fileSize >= 1024 * 1024 ? TF(S_BOOKS_SIZE_MB, kind.c_str(), (int)(in.fileSize >> 20), (int)((in.fileSize & 0xFFFFF) * 10 >> 20))
                                           : TF(S_BOOKS_SIZE_KB, kind.c_str(), (int)max<uint32_t>(1, in.fileSize >> 10));
  text(x, ty + 6, &FreeSans9pt7b, size.c_str());
  String state = in.finished && in.stats.finishedAt ? TF(S_BOOKS_FINISHED_ON, booksDate(in.stats.finishedAt).c_str())
                 : in.finished                      ? String(T(S_BOOKS_FINISHED))
                 : in.position > 0                  ? TF(S_BOOKS_PERCENT, in.permille / 10)
                                                    : String(T(S_BOOKS_NEW));
  text(x, ty + 40, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, state, w).c_str());
}

static void booksStatCell(int x, int y, int w, const char *label, const String &value) {
  text(x, y + 18, &FreeSans9pt7b, fitted(&FreeSans9pt7b, label, w - 8).c_str());
  text(x, y + 50, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, value, w - 8).c_str());
}

static void booksDrawDetails() {
  const LibInfo &in = bs->info;
  booksDetailItems();
  std::vector<DItem> &items = bkItems;
  std::vector<String> &lines = bkLines;
  int m = TALL ? 24 : 40, top = BODY_Y + 14, bottom = H - 70;
  // The screens: where each starts (a heading never ends one).
  std::vector<int> starts = {0};
  for (int i = 0, y = top; i < (int)items.size(); i++) {
    int need = items[i].h + (items[i].kind == DI_HEADING && i + 1 < (int)items.size() ? items[i + 1].h : 0);
    if (y + need > bottom && i > starts.back()) starts.push_back(i), y = top;
    y += items[i].h;
  }
  bs->detailPages = starts.size();
  bs->detailPage = constrain(bs->detailPage, 0, bs->detailPages - 1);
  int first = starts[bs->detailPage], last = bs->detailPage + 1 < bs->detailPages ? starts[bs->detailPage + 1] : items.size();
  bs->dHitN = 0;
  bkCoverW = 0;
  uint32_t flow = max<uint32_t>(1, in.flowSize);
  for (int i = first, y = top; i < last; i++) {
    const DItem &it = items[i];
    switch (it.kind) {
      case DI_HEAD: booksDrawDetailHead(y); break;
      case DI_BUTTON: {
        StrId label = in.finished ? S_BOOKS_READ_AGAIN : in.position > 0 ? S_BOOKS_CONTINUE_BTN : S_BOOKS_START;
        pillButton(m, y + 8, W - 2 * m, 64, T(label), true, &FreeSansBold12pt7b);
        if (bs->dHitN < 24) bs->dHits[bs->dHitN++] = {(int16_t)y, (int16_t)(y + it.h), -1};
        break;
      }
      case DI_PROGRESS: {
        int pm = in.finished ? 1000 : in.permille;
        canvas->drawRoundRect(m, y + 6, W - 2 * m, 14, 7, INK);
        canvas->fillRoundRect(m, y + 6, max(14, (W - 2 * m) * pm / 1000), 14, 7, INK);
        if (bs->dCurrent >= 0) text(m, y + 48, &FreeSans12pt7b, TF(S_BOOKS_CHAPTER_OF, bs->dCurrent + 1, (int)bs->dOff.size()).c_str());
        textRight(W - m, y + 48, &FreeSansBold12pt7b, TF(S_BOOKS_PERCENT, pm / 10).c_str());
        break;
      }
      case DI_STATS: {
        const LibBookStats &st = in.stats;
        uint32_t pace = st.paceBytesPerMin();
        String left = !pace ? String(T(S_BOOKS_LEFT_UNKNOWN))
                            : booksDuration((uint32_t)((uint64_t)(in.flowSize > in.position ? in.flowSize - in.position : 0) * 60 / pace));
        String pages = String(st.pages);
        String started = st.started ? booksDate(st.started) : String("-");
        int cols = TALL ? 2 : 4, cw = (W - 2 * m) / cols;
        const char *labels[] = {T(S_BOOKS_TIME_READ), T(S_BOOKS_PAGES_READ), T(S_BOOKS_LEFT), TF(S_BOOKS_STARTED_ON, "").c_str()};
        String values[] = {booksDuration(st.secs), pages, in.finished ? String("-") : left, started};
        String startedLabel = TF(S_BOOKS_STARTED_ON, "");
        startedLabel.trim();
        for (int k = 0; k < 4; k++) {
          int cx = m + (k % cols) * cw, cy = y + (k / cols) * 72;
          booksStatCell(cx, cy, cw, k == 3 ? startedLabel.c_str() : labels[k], values[k]);
        }
        break;
      }
      case DI_HEADING:
        text(m, y + 34, &FreeSansBold12pt7b, T((StrId)it.ref));
        canvas->drawFastHLine(m, y + 46, W - 2 * m, INK);
        break;
      case DI_TEXT: {
        const BookFont *df = booksDescFont();
        text(m, y + df->ascent + 2, df->base, lines[it.ref].c_str());
        break;
      }
      case DI_NOTE: {
        int ly = y + 26;
        for (const String &l : wrapped(&FreeSans12pt7b, T((StrId)it.ref), W - 2 * m)) text(m, ly, &FreeSans12pt7b, l.c_str()), ly += 28;
        break;
      }
      case DI_CHAPTER: {
        int c = it.ref, x = m + 24 + (bs->dLevel[c] - 1) * 24;
        bool cur = c == bs->dCurrent;
        const GFXfont *f = cur ? &FreeSansBold12pt7b : &FreeSans12pt7b;
        if (cur) canvas->fillCircle(m + 7, y + it.h / 2 - 4, 6, INK);
        char pct[12];
        snprintf(pct, sizeof pct, "%d%%", (int)((uint64_t)bs->dOff[c] * 100 / flow));
        int pw = textW(&FreeSans9pt7b, pct);
        textRight(W - m, y + it.h / 2 + 4, &FreeSans9pt7b, pct);
        text(x, y + it.h / 2 + 4, f, fitted(f, &bs->dPool[bs->dTitle[c]], W - m - pw - 16 - x).c_str());
        canvas->drawFastHLine(x, y + it.h - 1, W - m - x, INK);
        if (bs->dHitN < 24) bs->dHits[bs->dHitN++] = {(int16_t)y, (int16_t)(y + it.h), (int16_t)c};
        break;
      }
      default: break;
    }
    y += it.h;
  }
  pillButton(m, H - 62, 150, 50, T(S_BACK), false, &FreeSansBold12pt7b);
  char line[24];
  if (bs->detailPages > 1) {
    snprintf(line, sizeof line, "%d / %d", bs->detailPage + 1, bs->detailPages);
    textRight(W - m, H - 30, &FreeSansBold12pt7b, line);
    if (bs->detailPage == 0) textRight(W - m - textW(&FreeSansBold12pt7b, line) - 20, H - 30, &FreeSans9pt7b, T(S_BOOKS_MORE_BELOW));
  }
}

// The cover in its box on the book's page. In black and white (the greys
// the decoder makes, as a halftone), so the page goes up with a quick partial
// refresh rather than the panel's slow four-grey one; the last cover decoded
// is kept, so turning the page's screens or coming back costs nothing.
static uint8_t *bkCover;     // bkCoverW x bkCoverH, 2 bits a pixel
static char bkCoverKey[17];  // whose it is
static void booksDetailsCover() {
  if (!bkCoverW || bs->info.coverType != 'j') return;
  if (strcmp(bkCoverKey, bs->info.key) || !bkCover) {
    if (!bkCover) bkCover = (uint8_t *)psMalloc(120 * 180 / 4);  // the largest box (booksDrawDetailHead)
    const char *why = "";
    uint32_t t = millis();
    if (!bkCover || !jpegRender(bs->info.coverPath.c_str(), bkCover, bkCoverW, bkCoverH, true, &why)) {
      Serial.printf("books: cover: %s\n", why);
      bkCoverKey[0] = 0;
      return;
    }
    Serial.printf("books: cover decoded in %lu ms\n", (unsigned long)(millis() - t));
    strlcpy(bkCoverKey, bs->info.key, sizeof bkCoverKey);
  }
  // Four greys to ink and paper: black, a dense and a light 2 x 2 pattern, white.
  for (int y = 0; y < bkCoverH; y++)
    for (int x = 0; x < bkCoverW; x++) {
      int level = bkCover[y * (bkCoverW / 4) + x / 4] >> ((3 - (x & 3)) * 2) & 3;
      bool odd = (x ^ y) & 1, ink = level == 0 || (level == 1 && !(odd && (y & 1))) || (level == 2 && odd && (y & 1));
      canvas->drawPixel(bkCoverX + x, bkCoverY + y, ink ? INK : PAPER);
    }
}

// Opens the book from its page; a chapter tapped there is gone to once it's open.
static void booksDetailsRead(int chapter) {
  int i = bs->detail;
  booksOpen(i);
  if (chapter >= 0 && bs->mode == BM_READING && bs->pager) {
    bs->cover = false;
    bs->pager->gotoToc(chapter);
    booksSaveSoon();
  } else if (chapter >= 0) bs->toOpenToc = chapter;
  render(false);
}

void booksDetailsTap(int x, int y) {
  if (y > H - 70 && x < 200) {  // Back
    bs->mode = BM_LIBRARY;
    return render(false);
  }
  for (int k = 0; k < bs->dHitN; k++)
    if (y >= bs->dHits[k].y0 && y < bs->dHits[k].y1) return booksDetailsRead(bs->dHits[k].what);
}

// ---- statistics

static void booksDrawStats() {
  LibStats st = bs->lib->libraryStats(booksLocalDay());
  int m = TALL ? 24 : 40, y = BODY_Y + 44;
  text(m, y, &FreeSansBold18pt7b, T(S_BOOKS_STATS));
  y += 24;
  int cols = TALL ? 2 : 3, cw = (W - 2 * m) / cols, rowH = TALL ? 96 : 84;
  String longest = TF(S_BOOKS_LONGEST, (int)st.longestStreakDays);
  struct { StrId label; String value; } cells[] = {
      {S_BOOKS_TODAY, booksDuration(st.secsToday)},          {S_BOOKS_THIS_WEEK, booksDuration(st.secsThisWeek)},
      {S_BOOKS_STREAK, String(st.currentStreakDays)},         {S_BOOKS_BOOKS_FINISHED, String(st.booksFinished)},
      {S_BOOKS_TOTAL_TIME, booksDuration(st.totalSecs)},      {S_BOOKS_TOTAL_PAGES, String(st.totalPages)},
  };
  for (int k = 0; k < 6; k++) {
    int cx = m + (k % cols) * cw, cy = y + (k / cols) * rowH;
    text(cx, cy + 22, &FreeSans12pt7b, fitted(&FreeSans12pt7b, T(cells[k].label), cw - 10).c_str());
    text(cx, cy + 66, &FreeSansBold18pt7b, fitted(&FreeSansBold18pt7b, cells[k].value, cw - 10).c_str());
    if (cells[k].label == S_BOOKS_STREAK && st.longestStreakDays) textRight(cx + cw - 16, cy + 66, &FreeSans9pt7b, longest.c_str());
  }
  y += rowH * ((6 + cols - 1) / cols) + 10;
  // The last 7 days, a bar each, today on the right.
  text(m, y + 24, &FreeSansBold12pt7b, T(S_BOOKS_LAST_7));
  int chartTop = y + 44, chartH = H - 90 - chartTop - 30, bw = (W - 2 * m) / 7;
  uint32_t most = 60;
  for (int d = 0; d < 7; d++) most = max(most, st.last7[d]);
  int32_t today = booksLocalDay();
  for (int d = 0; d < 7; d++) {
    uint32_t v = st.last7[d];  // [6] today
    int h = chartH > 0 ? (int)((uint64_t)v * chartH / most) : 0;
    int bx = m + d * bw + 8, by = chartTop + chartH - h;
    if (h) canvas->fillRect(bx, by, bw - 16, h, INK);
    canvas->drawFastHLine(m, chartTop + chartH, W - 2 * m, INK);
    if (v >= 60) textCentre(bx + (bw - 16) / 2, by - 6, &FreeSans9pt7b, String(v / 60).c_str());
    if (today >= 0) {
      int wd = ((today - (6 - d)) % 7 + 7 + 3) % 7;  // Monday 0: 1 January 1970 was a Thursday
      textCentre(bx + (bw - 16) / 2, chartTop + chartH + 24, &FreeSansBold9pt7b, TItem(S_DAYS_INITIAL, wd).c_str());
    }
  }
  pillButton(m, H - 62, 150, 50, T(S_BACK), false, &FreeSansBold12pt7b);
}

void booksShowStats(int from) {
  bs->statsFrom = from;
  bs->mode = BM_STATS;
  render(false);
}

// ---- for the sketch: drawing, taps, swipes, buttons

// What render() calls in the app. True: the screen went to the panel already.
bool booksDraw(bool fs) {
  if (!bs) return false;
  if (bs->mode == BM_NONE) booksOpenApp();
  switch (bs->mode) {
    case BM_OPENING: booksDrawOpening(); return false;
    case BM_ERROR: booksDrawError(); return false;
    case BM_READING:
    case BM_MENU:
    case BM_TEXT: return booksDrawReader(fs);
    case BM_TOC:
    case BM_MARKS: booksDrawList(); return false;
    case BM_DETAILS: booksDrawDetails(); booksDetailsCover(); return false;
    case BM_STATS: booksDrawStats(); return false;
    default: booksDrawLibrary(); return false;
  }
}

// Opening the app: straight back into the book that was open when it was
// left (a restart or a wake too), else the library.
void booksOpenApp() {
  if (!bs) return;
  if (booksReading() && bs->pager && bs->pager->status() == BP_OK && bs->lib && bs->lib->isOpen() && cardIn()) {
    bs->mode = BM_READING;
    return;
  }
  bs->mode = BM_LIBRARY;
  bs->focus = -2;
  booksScan();
  String key = prefs.getString("bopen", "");
  if (key.length() == 16 && bs->lib && bs->lib->state() == LS_OK) {
    int i = bs->lib->find(key.c_str());
    if (i >= 0) booksOpen(i);
  }
}

static void booksSaveSoon() {
  if (!bs->lib || !bs->lib->isOpen()) return;
  booksGiveReading();
  bs->lib->setPosition(bs->pager->position(), booksNow());
  if (bs->pager->atEnd()) bs->lib->setFinished(booksNow());
  if (++bs->turns >= 5) {
    bs->turns = 0;
    bs->lib->saveState();
  }
}

static void booksTurn(int by) {
  BookPager &p = *bs->pager;
  bs->note = "";
  booksClock();
  uint32_t leaving = p.page().end - p.page().start;
  if (by > 0 && bs->cover) bs->cover = false;
  else if (by < 0 && p.atStart() && !bs->cover) bs->cover = true;
  else {
    uint32_t t = millis();
    bool moved = false;
    for (int i = 0; i < abs(by); i++) moved |= by > 0 ? p.nextPage() : p.prevPage();
    if (!moved) return;
    if (by > 0) bs->pendPages += by, bs->pendBytes += leaving * by;
    bs->turnLayoutMs = millis() - t;
  }
  if (!bs->hintSeen && !bs->cover) {
    bs->hintSeen = true;
    prefs.putBool("bhint", true);
  }
  uint32_t t0 = millis();
  booksSaveSoon();
  uint32_t tSave = millis() - t0;
  render(bs->coverGrey);  // coming off the grey cover: a full refresh
  bs->coverGrey = bs->coverGrey && bs->cover;
  // A page turn's time, from the tap: laying it out, saving the place, drawing it, the panel.
  Serial.printf("books: page at %lu (%d%%): layout %lu ms, save %lu ms, draw %lu ms, panel %lu ms\n", (unsigned long)p.position(), p.permille() / 10,
                (unsigned long)bs->turnLayoutMs, (unsigned long)tSave, (unsigned long)bkDrawMs, (unsigned long)(millis() - t0 - tSave - bkDrawMs));
  bs->turnLayoutMs = 0;
}

static void booksGoto(int permille) {
  bs->cover = false;
  bs->pager->gotoPermille(constrain(permille, 0, 999));
  booksSaveSoon();
}

static void booksSetSize(int s) {
  bs->size = constrain(s, 0, BOOK_SIZES - 1);
  prefs.putInt("bsizept", bookFont(bs->size, BOOK_REGULAR)->pt);
}

// Taps: what's under the finger on each screen.
void booksTap(int x, int y) {
  if (!bs) return;
  switch (bs->mode) {
    case BM_OPENING:
      if (y > H - 120) {  // Cancel
        bs->lib->openAbort();
        booksToLibrary();
      }
      return render(false);
    case BM_ERROR: booksToLibrary(); return render(false);
    case BM_READING: {
      int corner = 70;
      if (x > W - corner && y < corner && !bs->cover) {  // the top right corner: a bookmark
        const BookPage &pg = bs->pager->page();
        String label = bs->pager->chapterTitle();
        if (pg.runs.size()) {
          String words;
          for (size_t i = 0; i < pg.runs.size() && words.length() < 40; i++) words += String(i ? " " : "") + pg.text(pg.runs[i]);
          label = label.length() ? label + ": " + words : words;
        }
        int r = bs->lib->toggleMark(pg.start, pg.end, label.c_str());
        bs->note = r == 1 ? T(S_BOOKS_MARK_ADDED) : r < 0 ? T(S_BOOKS_MARKS_FULL) : "";
        bs->lib->saveState();
        return render(false);
      }
      if (x < W * 30 / 100) return booksTurn(-1);
      if (x > W * 70 / 100 || y > H * 75 / 100 || bs->cover) return booksTurn(1);
      bs->mode = BM_MENU;  // the middle
      return render(false);
    }
    case BM_MENU: {
      int bottom = booksMenuBottom();
      if (y > booksBarY() - 30 && y < booksBarY() + 30 && x > booksBarX0() - 20 && x < booksBarX1() + 20) {
        booksGoto((x - booksBarX0()) * 1000 / max(1, booksBarX1() - booksBarX0()));
        return render(false);
      }
      if (y > booksBarY() + 30) {  // the buttons
        int k = constrain((x - booksMargin()) * 4 / max(1, W - 2 * booksMargin()), 0, 3);
        if (k == 0) {
          bs->lib->sync();
          booksToLibrary();
        } else if (k == 1 || k == 2) {
          bs->mode = k == 1 ? BM_TOC : BM_MARKS;
          int rows = booksListRows();
          bs->listPage = k == 1 && bs->pager->currentToc() > 0 ? bs->pager->currentToc() / rows : 0;
        } else bs->mode = BM_TEXT;
        return render(false);
      }
      if (y < BAR_H + BK_MENU_TOP || y < bottom) {  // the page: the menu goes away
        bs->mode = BM_READING;
        return render(false);
      }
      return;
    }
    case BM_TEXT: {
      int y0 = booksMenuBottom(), m = booksMargin(), gap = 8, bw = (W - 2 * m - gap * (BOOK_SIZES - 1)) / BOOK_SIZES;
      int by = bkTextSizesY();
      if (y < y0 || (y < by && x > W - m - 160)) {  // Done, or the page above: back to it
        bs->mode = BM_READING;
        return render(false);
      }
      if (y >= by && y < by + BK_TEXT_SIZE_H) {
        booksSetSize((x - m) / (bw + gap));
        return render(false);
      }
      for (int row = 0; row < 3; row++) {
        if (y < bkTextRowY(row) || y >= bkTextRowY(row) + BK_TEXT_ROW) continue;
        if (row == 2) {
          bs->justify = !bs->justify;
          prefs.putBool("bjust", bs->justify);
          return render(false);
        }
        for (int k = 0; k < 3; k++)
          if (x >= bkSegX(k) - 4 && x < bkSegX(k) + BK_SEG_W + 4) {
            (row == 0 ? bs->spacing : bs->margins) = k;
            prefs.putInt(row == 0 ? "bspace" : "bmarg", k);
            return render(false);
          }
      }
      return;
    }
    case BM_TOC:
    case BM_MARKS: {
      if (y > H - 70 && x < 200) {
        bs->mode = BM_MENU;
        return render(false);
      }
      int r = (y - booksListTop()) / BK_LIST_ROW, rows = booksListRows();
      if (y < booksListTop() || r >= rows) return;
      int i = bs->listPage * rows + r;
      if (bs->mode == BM_TOC) {
        if (i >= bs->pager->tocCount()) return;
        bs->cover = false;
        bs->pager->gotoToc(i);
      } else {
        if (i >= bs->lib->markCount()) return;
        if (x > W - 80) {  // the X
          bs->lib->removeMark(i);
          bs->lib->saveState();
          return render(false);
        }
        bs->cover = false;
        bs->pager->gotoOffset(bs->lib->mark(i).offset);
      }
      booksSaveSoon();
      bs->mode = BM_READING;
      return render(false);
    }
    case BM_DETAILS: return booksDetailsTap(x, y);
    case BM_STATS:
      if (y > H - 76 && x < 200) {
        bs->mode = bs->statsFrom;
        return render(false);
      }
      return;
    default: {  // the library
      for (int k = 0; k < bs->hitN; k++)
        if (y >= bs->hits[k].y0 && y < bs->hits[k].y1) {
          int b = bs->hits[k].book;
          if (b == -3) return booksShowStats(BM_LIBRARY);
          if (b < 0) booksOpen(bs->lib->lastRead());  // the Continue card: straight into the book
          else return booksShowDetails(b);            // a book: its page
          return render(false);
        }
    }
  }
}

// Swipes: sideways turns a page; up and down page a list. False: the
// sketch's own meaning (home from the bottom edge, quick settings).
bool booksSwipe(int x0, int y0, int dx, int dy) {
  if (!bs) return false;
  bool vertical = abs(dy) > abs(dx);
  if (vertical && dy < 0 && y0 > H - 70) return false;  // home
  if (vertical && dy > 0 && y0 < BAR_H + 24 && bs->mode != BM_READING) return false;  // quick settings
  switch (bs->mode) {
    case BM_READING:
    case BM_MENU:
      if (!vertical) {
        bs->mode = BM_READING;
        booksTurn(dx < 0 ? 1 : -1);
      } else {
        bs->mode = dy > 0 ? BM_MENU : BM_READING;
        render(false);
      }
      return true;
    case BM_DETAILS:
      if (!vertical) return false;
      bs->detailPage = constrain(bs->detailPage + (dy < 0 ? 1 : -1), 0, bs->detailPages - 1);
      render(false);
      return true;
    case BM_STATS: return !vertical ? false : true;
    case BM_TOC:
    case BM_MARKS:
      if (!vertical) return false;
      bs->listPage += dy < 0 ? 1 : -1;
      bs->listPage = max(0, bs->listPage);
      render(false);
      return true;
    case BM_LIBRARY:
      if (!vertical) return false;  // sideways: the next app, as everywhere
      bs->libPage = constrain(bs->libPage + (dy < 0 ? 1 : -1), 0, bs->libPages - 1);
      bs->focus = -2;
      render(false);
      return true;
    default: return true;
  }
}

// UP / DOWN: the pages in the reader and the lists; the ring in the library.
void booksMove(int move) {
  if (!bs) return;
  switch (bs->mode) {
    case BM_READING:
    case BM_MENU:
    case BM_TEXT:
      bs->mode = BM_READING;
      return booksTurn(move);
    case BM_TOC:
    case BM_MARKS:
      bs->listPage = max(0, bs->listPage + move);
      return render(false);
    case BM_DETAILS:
      bs->detailPage = constrain(bs->detailPage + move, 0, bs->detailPages - 1);
      return render(false);
    case BM_LIBRARY: {
      if (!bs->lib || bs->lib->state() != LS_OK) return;
      int lo = bs->lib->lastRead() >= 0 ? -1 : 0, n = bs->lib->count();
      int f = bs->focus == -2 ? (move > 0 ? lo : n - 1) : bs->focus + move;
      bs->focus = constrain(f, lo, n - 1);
      bs->libPage = bs->focus < 0 ? 0 : booksPageOf(bs->focus);
      return render(false);
    }
    default: return;
  }
}

// OK: the menu in the reader, the ringed book in the library.
void booksOk() {
  if (!bs) return;
  switch (bs->mode) {
    case BM_READING: bs->mode = BM_MENU; break;
    case BM_MENU:
    case BM_TEXT:
    case BM_TOC:
    case BM_MARKS: bs->mode = BM_READING; break;
    case BM_ERROR: booksToLibrary(); break;
    case BM_LIBRARY:
      if (bs->focus == -1 || (bs->focus == -2 && bs->lib && bs->lib->lastRead() >= 0)) booksOpen(bs->lib->lastRead());
      else if (bs->focus >= 0) return booksShowDetails(bs->focus);
      break;
    case BM_DETAILS: return booksDetailsRead(-1);  // the big button
    case BM_STATS: bs->mode = bs->statsFrom; break;
    default: return;
  }
  render(false);
}

// Each pass of the loop: the scan, a book being got ready, counting the
// chapter's pages when nobody's touching; the place written on leaving.
void booksTick() {
  if (!bs) return;
  bool here = screen == SCREEN_APP && app == APP_BOOKS;
  static bool wasHere;
  if (wasHere && !here) booksSync();
  wasHere = here;
  if (!here) return;
  if (bs->mode == BM_LIBRARY && bs->lib && bs->lib->scanning()) {
    bs->lib->scanStep();
    bool changed = bs->lib->generation() != bs->libGen;
    if ((changed && millis() - bs->drawnAt > 2500) || !bs->lib->scanning()) {
      bs->drawnAt = millis();
      render(false);
    }
  } else if (bs->mode == BM_OPENING) {
    uint32_t t = millis();
    int p;
    do p = bs->lib->openStep();
    while (p >= 0 && p < BOOK_DONE && millis() - t < 120);
    if (p == BOOK_DONE) {
      Serial.println("books: ready");
      booksStartReading();
      render(false);
    } else if (p < 0) {
      bs->openErr = bs->lib->openError();
      Serial.printf("books: can't open: %d\n", (int)bs->openErr);
      bs->mode = BM_ERROR;
      render(false);
    } else {
      bs->openProgress = p;
      if (p - bs->openDrawn >= 50 && millis() - bs->drawnAt > 1500) {
        bs->openDrawn = p;
        bs->drawnAt = millis();
        render(false);
      }
    }
  } else if (booksReading() && bs->pager && millis() - lastActivity > 1000) {
    // A page turned and nothing since for 3 s: the place onto the card (a
    // pulled cable or a crash then costs nothing).
    if (bs->turns && millis() - lastActivity > 3000) {
      bs->turns = 0;
      bs->lib->saveState();
    }
    if (bs->mode != BM_READING) return;
    static int leftShown = -2;
    if (bs->pager->tick()) return;
    // Counted to the chapter's end: the foot can say how many pages are left.
    int left = bs->pager->pagesLeftInChapter();
    if (left != leftShown && leftShown == -1 && !bs->cover) {
      leftShown = left;
      render(false);
    }
    leftShown = left;
  }
}

// The place and the bookmarks onto the card: on leaving, before sleeping.
void booksSync() {
  if (bs && bs->lib && bs->lib->isOpen()) {
    if (bs->turnAt && booksReading()) booksClock();
    bs->turnAt = 0;
    booksGiveReading();
    if (bs->pager && bs->pager->status() == BP_OK) bs->lib->setPosition(bs->pager->position(), booksNow());
    bs->lib->sync();
  }
}

// The serial port's "book ..." commands (CONTRIBUTING.md).
void booksCommand(String cmd) {
  cmd.trim();
  if (!bs) return;
  int sp = cmd.indexOf(' ');
  String verb = sp < 0 ? cmd : cmd.substring(0, sp), arg = sp < 0 ? String() : cmd.substring(sp + 1);
  bool open = bs->pager && bs->pager->status() == BP_OK && bs->lib && bs->lib->isOpen();
  if (verb == "") {
    openApp(APP_BOOKS);
  } else if (verb == "library") {
    screen = SCREEN_APP, app = APP_BOOKS;
    booksToLibrary();
    render(false);
  } else if (verb == "list") {
    if (!bs->lib || bs->lib->state() == LS_IDLE) booksScan();
    while (bs->lib->scanning()) bs->lib->scanStep();
    LibBook b;
    Serial.printf("BOOKS %d (state %d)\n", bs->lib->count(), (int)bs->lib->state());
    for (int i = 0; i < bs->lib->count(); i++)
      if (bs->lib->book(i, b))
        Serial.printf("BOOK %d %s %d%% %s \"%s\" \"%s\" %s%s\n", i, b.status == LB_READING ? "reading" : b.status == LB_NEW ? "new" : "finished",
                      b.permille / 10, bookFormatLabel(b.format), b.title, b.author, b.path, b.error ? " (error)" : "");
    Serial.println("END");
  } else if (verb == "open") {
    screen = SCREEN_APP, app = APP_BOOKS;
    if (!bs->lib || bs->lib->state() == LS_IDLE) booksScan();
    uint32_t t = millis();
    booksOpen(arg.toInt());
    while (bs->mode == BM_OPENING) {  // the serial port waits: report as it goes
      int p = bs->lib->openStep();
      if (p == BOOK_DONE) booksStartReading();
      else if (p < 0) bs->openErr = bs->lib->openError(), bs->mode = BM_ERROR;
    }
    Serial.printf("book: open %s in %lu ms%s\n", bs->mode == BM_ERROR ? "FAILED" : "ok", (unsigned long)(millis() - t),
                  bs->mode == BM_ERROR ? booksErrText(bs->openErr) : "");
    render(true);
  } else if (verb == "details") {  // book n's page (book details <n>), or the next screen of it (book details more)
    screen = SCREEN_APP, app = APP_BOOKS;
    if (!bs->lib || bs->lib->state() == LS_IDLE) booksScan();
    if (arg == "more" && bs->mode == BM_DETAILS) {
      bs->detailPage = min(bs->detailPage + 1, bs->detailPages - 1);
      render(false);
    } else booksShowDetails(arg.toInt());
  } else if (verb == "stats") {  // the reading statistics
    screen = SCREEN_APP, app = APP_BOOKS;
    if (!bs->lib || bs->lib->state() == LS_IDLE) booksScan();
    LibStats st = bs->lib->libraryStats(booksLocalDay());
    Serial.printf("book: stats finished %lu reading %lu, %lu s %lu pages, today %lu s, week %lu s, streak %lu (longest %lu), days %lu\n",
                  (unsigned long)st.booksFinished, (unsigned long)st.booksReading, (unsigned long)st.totalSecs, (unsigned long)st.totalPages,
                  (unsigned long)st.secsToday, (unsigned long)st.secsThisWeek, (unsigned long)st.currentStreakDays, (unsigned long)st.longestStreakDays,
                  (unsigned long)st.daysRead);
    booksShowStats(bs->mode == BM_STATS ? BM_LIBRARY : bs->mode);
  } else if (verb == "start") {  // as a tap on book n: it gets ready while the loop goes on (the progress screen)
    screen = SCREEN_APP, app = APP_BOOKS;
    if (!bs->lib || bs->lib->state() == LS_IDLE) booksScan();
    booksOpen(arg.toInt());
    render(bs->mode == BM_READING);
  } else if (!open) {
    Serial.println("book: no book open (book open <n>)");
  } else if (verb == "next" || verb == "prev") {
    bs->mode = BM_READING;
    booksTurn((verb == "next" ? 1 : -1) * max(1L, arg.toInt()));
  } else if (verb == "goto") {
    bs->mode = BM_READING;
    booksGoto(arg.toInt());
    render(false);
  } else if (verb == "toc") {
    if (arg.length()) {
      bs->cover = false;
      bs->pager->gotoToc(arg.toInt());
      booksSaveSoon();
      bs->mode = BM_READING;
      render(false);
    } else {
      for (int i = 0; i < bs->pager->tocCount(); i++) {
        BookTocItem it = bs->pager->toc(i);
        Serial.printf("TOC %d %d %lu %s%s\n", i, it.level, (unsigned long)it.offset, it.title, it.current ? " (current)" : "");
      }
      Serial.println("END");
    }
  } else if (verb == "size") {
    booksSetSize(arg.toInt());
    render(false);
  } else if (verb == "spacing" || verb == "margins") {  // 0 tight / narrow, 1 normal, 2 loose / wide
    (verb == "spacing" ? bs->spacing : bs->margins) = constrain(arg.toInt(), 0, 2);
    prefs.putInt(verb == "spacing" ? "bspace" : "bmarg", verb == "spacing" ? bs->spacing : bs->margins);
    render(false);
  } else if (verb == "justify") {
    bs->justify = arg != "off";
    prefs.putBool("bjust", bs->justify);
    render(false);
  } else if (verb == "cover") {
    bs->cover = true;
    bs->mode = BM_READING;
    render(true);
  } else if (verb == "menu" || verb == "text" || verb == "contents" || verb == "marks" || verb == "page") {
    bs->mode = verb == "menu" ? BM_MENU : verb == "text" ? BM_TEXT : verb == "contents" ? BM_TOC : verb == "marks" ? BM_MARKS : BM_READING;
    bs->listPage = 0;
    if (verb == "page") bs->cover = false;
    render(false);
  } else if (verb == "mark") {
    const BookPage &pg = bs->pager->page();
    int r = bs->lib->toggleMark(pg.start, pg.end, bs->pager->chapterTitle());
    bs->lib->saveState();
    Serial.printf("book: mark %s\n", r == 1 ? "added" : r == 0 ? "removed" : "full");
    render(false);
  } else if (verb == "peek") {  // the page as text
    BookPager &p = *bs->pager;
    const BookPage &pg = p.page();
    Serial.printf("PAGE %lu-%lu of %lu, %d%%, chapter \"%s\", left in chapter %d, about %d of %d%s\n", (unsigned long)pg.start,
                  (unsigned long)pg.end, (unsigned long)p.flowSize(), p.permille() / 10, p.chapterTitle(), p.pagesLeftInChapter(),
                  p.estimatedPageNumber(), p.estimatedPages(), bs->cover ? " (cover shown)" : "");
    for (const PageLine &ln : pg.lines) {
      String s;
      for (uint32_t i = 0; i < ln.runCount; i++) {
        const PageRun &r = pg.runs[ln.firstRun + i];
        if (i && (r.flags & PR_SPACE_BEFORE)) s += ' ';
        s += pg.text(r);
      }
      Serial.printf("| %s\n", s.c_str());
    }
    Serial.println("END");
  } else if (verb == "bench") {  // n page turns laid out, not drawn: how long each takes
    int n = max(1L, arg.toInt());
    BookPager &p = *bs->pager;
    uint32_t t = millis(), worst = 0;
    int done = 0;
    for (; done < n; done++) {
      uint32_t t1 = millis();
      if (!p.nextPage()) break;
      worst = max(worst, millis() - t1);
    }
    uint32_t total = millis() - t;
    Serial.printf("book: bench %d pages in %lu ms, %lu ms a page, worst %lu ms\n", done, (unsigned long)total,
                  (unsigned long)(done ? total / done : 0), (unsigned long)worst);
    booksSaveSoon();
    render(false);
  } else {
    Serial.printf("book: unknown \"%s\"\n", cmd.c_str());
  }
}

// The app's icon: an open book.
void booksIcon(int cx, int cy, int s) {
  int w = s / 2, h = s * 70 / 100, t = max(3, s / 16), y = cy - h / 2;
  for (int side = -1; side <= 1; side += 2) {
    int x0 = cx, x1 = cx + side * w;
    for (int k = 0; k < t; k++) {
      canvas->drawLine(x0, y + 6 + k, x1, y + k, INK);           // the top edge, rising outwards
      canvas->drawLine(x1 - side * k, y, x1 - side * k, y + h - 6, INK);  // the outer edge
      canvas->drawLine(x0, y + h + k, x1, y + h - 6 + k, INK);   // the bottom edge
    }
    for (int l = 0; l < 3; l++) {  // lines of text
      int ly = y + 16 + l * (h - 24) / 3;
      thickLine(cx + side * 8, ly + 4, cx + side * (w - 10), ly, 2);
    }
  }
  canvas->fillRect(cx - t / 2, y + 6, t, h, INK);  // the spine
}
