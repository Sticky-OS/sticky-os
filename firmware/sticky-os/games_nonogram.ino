// ---------------------------------------------------------------- daily nonogram
//
// Today's nonogram (picross, issue #18), 8 x 8 (Easy) or 10 x 10 (Normal): made from the date by
// nonogrid.h, so every board has the same two that day, each with one solution that line logic
// finds without guessing. Opened from the Daily screen (games_daily.ino, which keeps the date and
// /daily.txt); this tab draws it and takes its taps. Its state lives in PSRAM (nonoS, nonoEnsure()).
//
// The clues sit on the left of each row and on top of each column (FreeSans 9pt), the grid fills
// the rest: about 37 px squares at 10 x 10 upright, with Fill / Cross and the status under it;
// sideways, beside it. A clue whose line is complete and right is struck through. The Fill /
// Cross switch (the one in use black; OK swaps them) says what a tap does: fill a square (black),
// or cross it out (x) as known empty; the same tap again clears it. Solved (every line matches its
// clue: the puzzle has one solution, so that is it): a full refresh, the time, the streak, "Come
// back tomorrow". Each size is its own puzzle, both kept (and saved) as you leave them.
//
// Ids, for taps, the ring and `game pick <id>`:
//   0..side*side-1  a square, row * side + column (0..63 at 8 x 8, 0..99 at 10 x 10)
//   200 Fill, 201 Cross; 210 / 211 today's 8 x 8 / 10 x 10 (on the Size card)
//   ID_NEW (901) the Size card; ID_CANCEL (905) closes it; ID_BACK (900) the Daily screen
// `game peek`: "nonogram <8x8|10x10> <yyyymmdd> rows <clue>|<clue>... cols <clue>|... grid <rows,
// '|' between: # filled, x crossed, . unknown> next <square>=<fill|cross> done <0|1> mode
// <fill|cross> card <0|1> time <s>s made <ms>ms", a clue its blocks with commas ("3,1"); "next
// -1=none" once solved; "nonogram none <date> card 1" before a size is chosen. Then the targets.
// `game right` gives nonogrid::next()'s square the solution's mark, `game wrong` the other one.
// Demo mode: 10 x 10, the demo date's, partly done.
#include "nonogrid.h"
#include "minesgrid.h"  // minesgrid::Clock

enum { NONO_FILL = 200, NONO_CROSS = 201, NONO_SIZE = 210 };

struct NonoSize {
  nonogrid::Puzzle puz;
  bool made = false, started = false, solved = false;
  uint8_t marks[nonogrid::kMaxCells] = {};  // nonogrid::kUnknown, kFilled, kCrossed
  minesgrid::Clock clock;
  uint32_t madeMs = 0;
  uint16_t check = 0;  // from /daily.txt: the puzzle the marks were made on (nonoCheck())
};
struct NonoState {
  NonoSize sz[nonogrid::kSizes];
  int size = -1;  // on screen; -1: none chosen today
  bool crossMode = false, choosing = false, newBest = false;
};
static NonoState *nonoS = nullptr;
// No function takes or returns these types (see minesEnsure()).
static void nonoEnsure() {
  if (nonoS) return;
  void *p = heap_caps_malloc(sizeof(NonoState), MALLOC_CAP_SPIRAM);
  nonoS = p ? new (p) NonoState() : new NonoState();
}

static StrId nonoSizeName(int z) { return z == 0 ? S_NONO_EASY : S_NONO_NORMAL; }

static void nonogramReset() {
  nonoEnsure();
  for (NonoSize &z : nonoS->sz) z = NonoSize();
  nonoS->size = -1, nonoS->crossMode = nonoS->choosing = nonoS->newBest = false;
}

static uint32_t nonoSecs(int z) { return minesgrid::clockShown(nonoS->sz[z].clock, millis()) / 1000; }

// A number for the puzzle, so marks saved for another one (a changed generator) aren't put on it.
static uint16_t nonoCheck(int z) {
  const nonogrid::Puzzle &p = nonoS->sz[z].puz;
  uint32_t h = 7;
  for (int c = 0; c < p.side * p.side; c++) h = (h * 31 + p.filled[c] + c) % 65521;
  return (uint16_t)(h ? h : 1);
}

static void nonoDemoFill(int size) {
  NonoSize &z = nonoS->sz[size];
  int n = z.puz.side * z.puz.side;
  for (int c = 0; c < n * 55 / 100; c++) z.marks[c] = z.puz.filled[c] ? nonogrid::kFilled : c % 3 ? nonogrid::kCrossed : nonogrid::kUnknown;
  z.started = true;
  z.clock.elapsedMs = 168000;  // 2:48, stopped
}

static bool nonoMake(int size) {
  NonoSize &z = nonoS->sz[size];
  if (z.made) return true;
  long date = dailyDate();
  if (!date) return false;
  uint32_t t0 = millis();
  nonogrid::Stats st;
  if (!nonogrid::daily(date, size, z.puz, &st)) return false;
  z.made = true, z.madeMs = millis() - t0;
  Serial.printf("nonogram: made %dx%d for %ld in %lu ms, %d attempts, %lu lines\n", z.puz.side, z.puz.side, date, (unsigned long)z.madeMs,
                st.attempts, (unsigned long)st.steps);
  bool fits = !z.started || z.check == nonoCheck(size);
  if (!fits) Serial.println("nonogram: the saved grid isn't today's puzzle: dropped");
  if (!z.started || !fits) {
    for (uint8_t &m : z.marks) m = nonogrid::kUnknown;
    z.started = false, z.clock = minesgrid::Clock();
    if (demoMode) nonoDemoFill(size);
  }
  z.check = nonoCheck(size);
  z.solved = nonogrid::solved(z.puz, z.marks);
  return true;
}

static void nonoPick(int size) {
  NonoState &s = *nonoS;
  s.size = constrain(size, 0, nonogrid::kSizes - 1);
  s.choosing = false, s.newBest = false;
  nonoMake(s.size);
  dailyChanged();
}

static void nonogramOpen() {
  nonoEnsure();
  dailyDate();
  NonoState &s = *nonoS;
  if (s.size < 0) {
    for (int z = 0; z < nonogrid::kSizes && s.size < 0; z++)
      if (nonoS->sz[z].started && !nonoS->sz[z].solved) s.size = z;
    if (demoMode && s.size < 0) s.size = 1;
  }
  if (s.size >= 0) s.choosing = false, nonoMake(s.size);
  else s.choosing = true;
}

static void nonogramForget() { nonogramReset(); }

// ---- /daily.txt (games_daily.ino reads and writes the file; these are the nonogram lines)

static void nonogramSaveTo(File &f) {
  nonoEnsure();
  f.printf("nonogram %d\n", nonoS->size);
  for (int z = 0; z < nonogrid::kSizes; z++) {
    const NonoSize &v = nonoS->sz[z];
    if (!v.started) continue;
    int n = nonogrid::kSide[z] * nonogrid::kSide[z];
    char cells[nonogrid::kMaxCells + 1];
    for (int c = 0; c < n; c++) cells[c] = v.marks[c] == nonogrid::kFilled ? '#' : v.marks[c] == nonogrid::kCrossed ? 'x' : '.';
    cells[n] = 0;
    f.printf("nonogram%d %s %lu %d %u\n", z, cells, (unsigned long)nonoSecs(z), v.solved ? 1 : 0, (unsigned)v.check);
  }
}

static void nonogramLoadLine(const String &k, const String &v) {
  nonoEnsure();
  if (k == "nonogram") {
    nonoS->size = constrain((int)v.toInt(), -1, nonogrid::kSizes - 1);
    return;
  }
  int z = k.length() == 9 ? k[8] - '0' : -1;
  if (z < 0 || z >= nonogrid::kSizes) return;
  int n = nonogrid::kSide[z] * nonogrid::kSide[z];
  int a = v.indexOf(' '), b = a < 0 ? -1 : v.indexOf(' ', a + 1), c = b < 0 ? -1 : v.indexOf(' ', b + 1);
  if (a != n || c < 0) return;  // damaged: left out
  NonoSize &x = nonoS->sz[z];
  for (int i = 0; i < n; i++) x.marks[i] = v[i] == '#' ? nonogrid::kFilled : v[i] == 'x' ? nonogrid::kCrossed : nonogrid::kUnknown;
  x.clock = minesgrid::Clock();
  x.clock.elapsedMs = (uint32_t)v.substring(a + 1, b).toInt() * 1000;
  x.solved = v.substring(b + 1, c).toInt() == 1;
  x.check = (uint16_t)v.substring(c + 1).toInt();
  x.started = true, x.made = false;
}

// ---- geometry: the clues' room, the square, where the grid starts

// A row's or column's clue as text, the blocks with `sep` between, for the size on screen.
static String nonoClueText(bool column, int i, const char *sep) {
  const nonogrid::Puzzle &p = nonoS->sz[nonoS->size].puz;
  String s;
  for (int k = 0; k < nonogrid::clueCount(p, column, i); k++) s += (k ? sep : "") + String(nonogrid::clueAt(p, column, i, k));
  return s;
}

static const int NONO_CLUE_LINE = 17;  // the column clues' line height (9pt)

static void nonoGeometry(int &gx, int &gy, int &c, int &leftW, int &topH) {
  const nonogrid::Puzzle &p = nonoS->sz[nonoS->size].puz;
  int side = p.side, rowW = 0, colN = 1;
  for (int i = 0; i < side; i++) {
    rowW = max(rowW, textW(&FreeSans9pt7b, nonoClueText(false, i, " ").c_str()));
    colN = max(colN, nonogrid::clueCount(p, true, i));
  }
  leftW = rowW + 12, topH = colN * NONO_CLUE_LINE + 8;
  if (TALL) {
    int top = gTop() + 4, availW = W - 16 - leftW, availH = H - 150 - top - topH;
    c = min(48, min(availW / side, availH / side));
    gx = (W - leftW - side * c) / 2 + leftW, gy = top + topH;
  } else {  // 250 px beside it for the controls
    int availW = W - 16 - 250 - 24 - leftW, availH = BODY_H - 16 - topH;
    c = min(48, min(availW / side, availH / side));
    gx = 16 + leftW, gy = BODY_Y + 8 + topH + (availH - side * c) / 2;
  }
}

// ---- pictures

static void nonogramIcon(int cx, int cy, int s, int t) {  // a heart in a 5 x 5 grid
  static const uint8_t HEART[5] = {0x0A, 0x1F, 0x1F, 0x0E, 0x04};
  int c = s / 5, x0 = cx - 5 * c / 2, y0 = cy - 5 * c / 2;
  for (int r = 0; r < 5; r++)
    for (int k = 0; k < 5; k++) {
      canvas->drawRect(x0 + k * c, y0 + r * c, c + 1, c + 1, INK);
      if (HEART[r] >> (4 - k) & 1) canvas->fillRect(x0 + k * c + 2, y0 + r * c + 2, c - 3, c - 3, INK);
    }
  for (int k = 1; k < t; k++) canvas->drawRect(x0 - k, y0 - k, 5 * c + 1 + 2 * k, 5 * c + 1 + 2 * k, INK);
}

static void nonoBoard(bool targets) {
  NonoState &s = *nonoS;
  const NonoSize &z = s.sz[s.size];
  const nonogrid::Puzzle &p = z.puz;
  int gx, gy, c, leftW, topH;
  nonoGeometry(gx, gy, c, leftW, topH);
  int side = p.side, every = side == 10 ? 5 : 4;
  for (int i = 0; i < side; i++) {  // the clues, struck once their line is right
    String rowClue = nonoClueText(false, i, " ");
    int ry = gy + i * c + c / 2 + 6, rw = textW(&FreeSans9pt7b, rowClue.c_str());
    textRight(gx - 7, ry, &FreeSans9pt7b, rowClue.c_str());
    if (nonogrid::lineDone(p, z.marks, false, i)) canvas->fillRect(gx - 9 - rw, ry - 6, rw + 4, 2, INK);
    bool colDone = nonogrid::lineDone(p, z.marks, true, i);
    int n = nonogrid::clueCount(p, true, i), cx = gx + i * c + c / 2;
    for (int k = 0; k < n; k++) {
      String num = String(nonogrid::clueAt(p, true, i, k));
      int y = gy - 6 - (n - 1 - k) * NONO_CLUE_LINE, w = textW(&FreeSans9pt7b, num.c_str());
      textCentre(cx + 1, y, &FreeSans9pt7b, num.c_str());
      if (colDone) canvas->fillRect(cx - w / 2 - 2, y - 6, w + 5, 2, INK);
    }
  }
  for (int r = 0; r < side; r++)
    for (int k = 0; k < side; k++) {
      int id = r * side + k, x = gx + k * c, y = gy + r * c;
      if (z.marks[id] == nonogrid::kFilled) canvas->fillRect(x + 3, y + 3, c - 5, c - 5, INK);
      else if (z.marks[id] == nonogrid::kCrossed) gGlyphX(x + c / 2, y + c / 2, c * 22 / 100, 2);
      if (targets) gTarget(x + 1, y + 1, c - 1, c - 1, id);
    }
  for (int i = 0; i <= side; i++) {  // the lines: thicker every 5 (every 4 at 8 x 8)
    int t = i % every ? 1 : 3;
    canvas->fillRect(gx + i * c - t / 2, gy - 1, t, side * c + 3, INK);
    canvas->fillRect(gx - 1, gy + i * c - t / 2, side * c + 3, t, INK);
  }
}

// Fill / Cross, the one in use black.
static void nonoModes(int x, int y, int w, int h) {
  int bw = (w - 12) / 2;
  gButton(x, y, bw, h, T(S_NONO_FILL), !nonoS->crossMode, NONO_FILL);
  gButton(x + bw + 12, y, bw, h, T(S_NONO_CROSS), nonoS->crossMode, NONO_CROSS);
}

static void nonoStatus(int x, int y, int w, int lines) {
  const NonoState &s = *nonoS;
  text(x, y, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, T(nonoSizeName(s.size)), w - 90).c_str());
  textRight(x + w, y, &FreeSansBold12pt7b, dailyTime(nonoSecs(s.size)).c_str());
  gLeft(x, y + 26, &FreeSans9pt7b, T(s.crossMode ? S_NONO_HOW_CROSS : S_NONO_HOW_FILL), w, 20, lines);
}

static void nonoCardDraw() {
  GameProgress &p = gameProgress();
  String names[nonogrid::kSizes], lines[nonogrid::kSizes];
  for (int z = 0; z < nonogrid::kSizes; z++) {
    names[z] = T(nonoSizeName(z));
    const NonoSize &v = nonoS->sz[z];
    if (v.solved) lines[z] = TF(S_DAILY_SOLVED_IN, dailyTime(nonoSecs(z)).c_str());
    else if (v.started) lines[z] = TF(S_DAILY_UNDER_WAY, dailyTime(nonoSecs(z)).c_str());
    else if (p.nonBest[z]) lines[z] = TF(S_DAILY_BEST, dailyTime(p.nonBest[z]).c_str());
  }
  dailyCard(T(S_NONO_CARD), nonogrid::kSizes, NONO_SIZE, nonoS->size, names, lines);
}

static void drawNonogram() {
  nonoEnsure();
  if (!dailyDate()) return dailyS->screen = DS_NO_DATE, drawDailyNoDate();
  NonoState &s = *nonoS;
  if (s.size < 0 && !s.choosing) nonogramOpen();  // reset (a new day, demo mode): as if opened
  if (s.size >= 0 && !nonoMake(s.size)) s.size = -1, s.choosing = true;
  bool board = s.size >= 0, targets = !s.choosing;
  bool done = board && s.sz[s.size].solved;
  GameProgress &p = gameProgress();
  int streak = gamesStreakAlive(p.nonDay, p.nonStreak, gToday());
  int gx = 0, gy = 0, c = 0, leftW = 0, topH = 0;
  if (board) nonoGeometry(gx, gy, c, leftW, topH);
  int side = board ? s.sz[s.size].puz.side : 0;
  if (TALL) {
    gHeader(T(S_G_NONOGRAM), "", 130);
    if (targets) gButton(W - 16 - 124, BODY_Y + 10, 124, 48, T(S_NONO_SIZE), false, ID_NEW, &FreeSansBold9pt7b);
    if (board) {
      nonoBoard(targets && !done);
      int y = gy + side * c + 14, x = 24, w = W - 48;
      if (done) dailySolvedPanel(x, y - 6, w, H - y - 2, nonoSecs(s.size), s.newBest, streak, T(S_NONO_SIZE));
      else if (targets) nonoModes(x, y, w, 52), nonoStatus(x, y + 86, w, max(1, (H - 8 - (y + 112)) / 20 + 1));
    }
  } else {
    if (!board) gHeader(T(S_G_NONOGRAM));
    else nonoBoard(targets && !done);
    int px = board ? gx + side * c + 24 : 0, pw = W - 16 - px;
    if (board && targets) gButton(px, BODY_Y + 10, 116, 48, T(S_G_BACK), false, ID_BACK);
    if (targets) gButton(W - 16 - 124, BODY_Y + 10, 124, 48, T(S_NONO_SIZE), false, ID_NEW, &FreeSansBold9pt7b);
    int py = BODY_Y + 72;
    if (board && done) dailySolvedPanel(px, py, pw, H - 12 - py, nonoSecs(s.size), s.newBest, streak, T(S_NONO_SIZE));
    else if (board && targets) nonoModes(px, py, pw, 56), nonoStatus(px, py + 100, pw, 6);
  }
  if (s.choosing) nonoCardDraw();
}

// ---- playing

static void nonoTick(int size) {
  NonoSize &z = nonoS->sz[size];
  if (demoMode) return;
  uint32_t now = millis();
  if (z.clock.running) minesgrid::clockTick(z.clock, now);
  else z.clock.running = true, z.clock.lastMs = now;
}

static void nonoSet(int cell, uint8_t mark) {
  NonoState &s = *nonoS;
  NonoSize &z = s.sz[s.size];
  z.marks[cell] = mark;
  z.started = true;
  nonoTick(s.size);
  dailyChanged();
  if (nonogrid::solved(z.puz, z.marks)) {
    if (!demoMode) minesgrid::clockStop(z.clock, millis());
    z.solved = true;
    GameProgress &p = gameProgress();
    uint32_t secs = nonoSecs(s.size);
    s.newBest = demoMode ? false : dailySolvedNow(p.nonDay, p.nonStreak, p.nonBest[s.size], secs);
    gs->gFocusId = -1;
    Serial.printf("nonogram: %dx%d solved in %lu s, streak %d\n", z.puz.side, z.puz.side, (unsigned long)secs, p.nonStreak);
    return render(true);
  }
  beep(2600, 15);
  render(false);
}

static void actNonogram(int id) {
  nonoEnsure();
  dailyDate();  // first: a new day resets the grids (and the size on screen)
  NonoState &s = *nonoS;
  if (id == ID_BACK) {
    if (s.choosing && s.size >= 0) return s.choosing = false, gs->gFocusId = -1, render(false);
    return dailyBack();
  }
  if (s.size < 0 && !s.choosing) return nonogramOpen(), gs->gFocusId = -1, render(true);  // reset under the finger: today's, afresh
  if (id == ID_NEW) return s.choosing = true, gs->gFocusId = -1, render(false);
  if (id == ID_CANCEL) {
    if (!s.choosing) return;
    if (s.size < 0) return dailyBack();
    return s.choosing = false, gs->gFocusId = -1, render(false);
  }
  if (id >= NONO_SIZE && id < NONO_SIZE + nonogrid::kSizes) return nonoPick(id - NONO_SIZE), gs->gFocusId = -1, render(true);
  if (s.choosing || s.size < 0 || !nonoMake(s.size) || s.sz[s.size].solved) return;
  if (id == NONO_FILL || id == NONO_CROSS) {
    if (s.crossMode == (id == NONO_CROSS)) return;
    s.crossMode = id == NONO_CROSS;
    return render(false);
  }
  NonoSize &z = s.sz[s.size];
  if (id < 0 || id >= z.puz.side * z.puz.side) return;
  uint8_t want = s.crossMode ? nonogrid::kCrossed : nonogrid::kFilled;
  nonoSet(id, z.marks[id] == want ? (uint8_t)nonogrid::kUnknown : want);
}

// OK with nothing ringed: on the card, the size; while playing, Fill <-> Cross.
static bool nonogramOk() {
  nonoEnsure();
  dailyDate();  // first: a new day resets the grids (and the size on screen)
  NonoState &s = *nonoS;
  if (s.choosing) return actNonogram(NONO_SIZE + max(0, s.size)), true;
  if (s.size < 0 || s.sz[s.size].solved) return false;
  actNonogram(s.crossMode ? NONO_FILL : NONO_CROSS);
  return true;
}

static String nonogramLine() {
  nonoEnsure();
  dailyDate();
  GameProgress &p = gameProgress();
  long today = gToday();
  const NonoState &s = *nonoS;
  int under = s.size >= 0 && s.sz[s.size].started && !s.sz[s.size].solved ? s.size : -1;
  for (int z = 0; z < nonogrid::kSizes && under < 0; z++)
    if (s.sz[z].started && !s.sz[z].solved) under = z;
  String state = under >= 0 ? TF(S_DAILY_UNDER_WAY, T(nonoSizeName(under))) : String(T(S_NONO_SIZES));
  return dailyRowLine(today && p.nonDay == today, state, gamesStreakAlive(p.nonDay, p.nonStreak, today));
}

// `game right` / `game wrong`: nonogrid::next()'s square gets its mark, or the other one.
static void nonogramAnswer(bool right) {
  nonoEnsure();
  dailyDate();  // first: a new day resets the grids (and the size on screen)
  NonoState &s = *nonoS;
  if (s.choosing || s.size < 0 || !nonoMake(s.size)) return;
  NonoSize &z = s.sz[s.size];
  if (z.solved) return;
  uint8_t mark = 0;
  int c = nonogrid::next(z.puz, z.marks, &mark);
  if (c < 0) return;
  if (!right) {
    mark = mark == nonogrid::kFilled ? nonogrid::kCrossed : nonogrid::kFilled;
    if (z.marks[c] == mark)  // wrong there already: the first unknown square gets a wrong mark
      for (int i = 0; i < z.puz.side * z.puz.side; i++)
        if (z.marks[i] == nonogrid::kUnknown) { c = i, mark = z.puz.filled[i] ? nonogrid::kCrossed : nonogrid::kFilled; break; }
  }
  nonoSet(c, mark);
}

static void nonogramPeek(String &s) {
  nonoEnsure();
  dailyDate();  // first: a new day resets the grids (and the size on screen)
  NonoState &st = *nonoS;
  long date = dailyDate();
  if (st.size < 0 || !nonoMake(st.size)) {
    s += "nonogram none " + String(date) + " card " + (st.choosing ? "1" : "0");
    return;
  }
  const NonoSize &z = st.sz[st.size];
  const nonogrid::Puzzle &p = z.puz;
  String rows, cols, grid;
  for (int i = 0; i < p.side; i++) {
    if (i) rows += '|', cols += '|', grid += '|';
    rows += nonoClueText(false, i, ",");
    cols += nonoClueText(true, i, ",");
    for (int k = 0; k < p.side; k++) {
      uint8_t m = z.marks[i * p.side + k];
      grid += m == nonogrid::kFilled ? '#' : m == nonogrid::kCrossed ? 'x' : '.';
    }
  }
  uint8_t mark = 0;
  int next = nonogrid::next(p, z.marks, &mark);
  bool done = nonogrid::solved(p, z.marks);
  char tail[120];
  snprintf(tail, sizeof tail, " next %d=%s done %d mode %s card %d time %lus made %lums", done ? -1 : next,
           done || next < 0 ? "none" : mark == nonogrid::kFilled ? "fill" : "cross", done, st.crossMode ? "cross" : "fill", st.choosing,
           (unsigned long)nonoSecs(st.size), (unsigned long)z.madeMs);
  s += "nonogram " + String(p.side) + "x" + p.side + " " + date + " rows " + rows + " cols " + cols + " grid " + grid + tail;
}
