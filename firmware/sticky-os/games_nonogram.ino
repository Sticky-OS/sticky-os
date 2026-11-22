// ---------------------------------------------------------------- Daily: the Nonogram
//
// The Daily screen's third row (games_sudoku.ino draws the screen and sends this tab its taps;
// issue #18): today's nonogram (picross), 8 x 8 or 10 x 10, made on the board from the date by
// nonogrid.h, so every board has the same two that day, each with one solution that line logic
// finds without guessing. Made once a day and kept in PSRAM (nonoS, nonoEnsure()).
//
// The clues sit left of each row and above each column (9pt); a clue whose line is complete and
// right is struck through. Fill / Cross (the one in use black; OK swaps them) say what a tap
// does: fill a square, or cross it out as known empty; the same tap again clears it. Solved
// (every line matches its clue: there's one solution, so that is it): a full refresh, the time,
// the streak, "Come back tomorrow", as Sudoku's. Upright, Back, the size and time, Size along the
// top, the grid, then Fill / Cross and how to play; sideways, the grid on the left, the rest beside.
//
// Today's grids (marks, time) are in GameProgress::nonPlay, written to /daily.txt from the loop
// (gamesDailyChanged()), so a day's work survives deep sleep; a grid for another date, or made on
// another puzzle (its check, a changed generator), is dropped when its size is opened. The clock
// counts as Sudoku's: while the puzzle is on screen, a gap between taps counting at most 10 minutes.
//
// Ids, for taps, the ring and `game pick <id>`:
//   12 Nonogram (the Daily screen, games_sudoku.ino)
//   0..side*side-1 a square, row * side + column (0..63 at 8 x 8, 0..99 at 10 x 10)
//   200 Fill, 201 Cross; 210 / 211 the 8 x 8 / 10 x 10 (the sizes card); 230 Size (back to the card)
//   ID_BACK (900): the Daily screen
// `game peek`: "nonogram <8x8|10x10> <date> rows <clue>|<clue>... cols <clue>|... grid <rows, '|'
// between: # filled, x crossed, . unknown> next <square>=<fill|cross> done <0|1> mode <fill|cross>
// time <s>s made <ms>ms", a clue its blocks with commas ("3,1"), "next -1=none" once solved; then
// " card" when the sizes card is up. Before a size is open today: "nonogram none <date> card".
// `game right` gives nonogrid::next()'s square the solution's mark, `game wrong` the other one.
// Demo mode (`demo on`): the first Nonogram opened is today's 10 x 10, partly done; nothing is saved.
#include "nonogrid.h"

enum { NK_CARD, NK_PLAY };  // the sizes card, or a puzzle
enum { NONO_FILL = 200, NONO_CROSS = 201, NONO_SIZE = 210, NONO_SIZES = 230 };
static const uint32_t NONO_GAP_MS = 10 * 60 * 1000UL;  // as Sudoku's SDK_GAP_MS (that tab comes after this one)

struct NonoState {
  int nk = NK_CARD, size = 0;
  bool crossMode = false, newBest = false, demoMade = false;
  bool running = false;              // the clock
  uint32_t lastMs = 0, carryMs = 0;
  nonogrid::Puzzle cache[nonogrid::kSizes];  // the day's puzzles, made when first wanted
  long cacheDate[nonogrid::kSizes] = {};
  uint32_t madeMs[nonogrid::kSizes] = {};
};
static NonoState *nonoS = nullptr;
// Made on first use. No function takes or returns NonoState or a nonogrid type (see dailyEnsure()).
static void nonoEnsure() {
  if (nonoS) return;
  void *p = heap_caps_malloc(sizeof(NonoState), MALLOC_CAP_SPIRAM);
  nonoS = p ? new (p) NonoState() : new NonoState();
}

static StrId nonoSizeName(int z) { return z == 0 ? S_NONO_EASY : S_NONO_NORMAL; }
static const char *const NONO_SIZE_IDS[nonogrid::kSizes] = {"8x8", "10x10"};
static const char *const NONO_SIZE_SHOWN[nonogrid::kSizes] = {"8 x 8", "10 x 10"};

// ---- the day's puzzle and today's grid

// The day's puzzle at a size, made once a day. False when there's no date, or (never in the
// tests: test_nonogrid.cpp) no try made one.
static bool nonoMake(int size) {
  nonoEnsure();
  long day = gToday();
  if (!day) return false;
  if (nonoS->cacheDate[size] == day) return true;
  uint32_t t0 = millis();
  nonogrid::Stats st;
  if (!nonogrid::daily(day, size, nonoS->cache[size], &st)) return false;
  nonoS->cacheDate[size] = day, nonoS->madeMs[size] = millis() - t0;
  Serial.printf("nonogram: %s for %ld made in %lu ms, %d tries, %lu lines\n", NONO_SIZE_IDS[size], day, (unsigned long)nonoS->madeMs[size],
                st.attempts, (unsigned long)st.steps);
  return true;
}
// The puzzle itself: a macro, as no function may return a nonogrid type (see nonoEnsure).
#define nonoPuzzle(size) (nonoS->cache[(size)])

// A number for the puzzle, so marks saved on another one (a changed generator) aren't put on it.
static uint16_t nonoCheck(int size) {
  const nonogrid::Puzzle &p = nonoPuzzle(size);
  uint32_t h = 7;
  for (int c = 0; c < p.side * p.side; c++) h = (h * 31 + p.filled[c] + c) % 65521;
  return (uint16_t)(h ? h : 1);
}

static bool nonoStartedToday(int size) {
  const GameProgress::NonogramPlay &g = gameProgress().nonPlay[size];
  return gToday() && g.date == gToday();
}
static bool nonoSolvedToday(int size) { return nonoStartedToday(size) && gameProgress().nonPlay[size].done; }

// Today's grid at a size (the puzzle made first), fresh when it's for another day or puzzle.
static GameProgress::NonogramPlay &nonoGrid(int size) {
  GameProgress::NonogramPlay &g = gameProgress().nonPlay[size];
  if (nonoMake(size) && (g.date != gToday() || g.check != nonoCheck(size))) {
    if (g.date == gToday()) Serial.println("nonogram: the saved grid isn't today's puzzle: dropped");
    g = GameProgress::NonogramPlay();
    g.date = gToday(), g.check = nonoCheck(size);
    gamesDailyChanged();
  }
  return g;
}
static GameProgress::NonogramPlay &nonoNow() { return gameProgress().nonPlay[nonoS->size]; }

// The demo's grid: the 10 x 10, a little over half of it worked out, from 2:48.
static void nonoDemoGrid() {
  GameProgress::NonogramPlay &g = gameProgress().nonPlay[1];
  g = GameProgress::NonogramPlay();
  if (!nonoMake(1)) return;
  const nonogrid::Puzzle &p = nonoPuzzle(1);
  g.date = gToday(), g.check = nonoCheck(1), g.secs = 168;
  for (int c = 0; c < 55; c++) g.marks[c] = p.filled[c] ? nonogrid::kFilled : c % 3 ? nonogrid::kCrossed : nonogrid::kUnknown;
  nonoS->demoMade = true;
}

// ---- the clock, as Sudoku's

static int nonoSecsShown() {
  if (!nonoS) return 0;
  const GameProgress::NonogramPlay &g = nonoNow();
  if (!nonoS->running) return g.secs;
  uint32_t gap = min<uint32_t>(millis() - nonoS->lastMs, NONO_GAP_MS);
  return g.secs + (nonoS->carryMs + gap) / 1000;
}
static void nonoTick() {
  if (!nonoS->running) return;
  uint32_t now = millis(), total = nonoS->carryMs + min<uint32_t>(now - nonoS->lastMs, NONO_GAP_MS);
  GameProgress::NonogramPlay &g = nonoNow();
  if (total >= 1000) g.secs += total / 1000, gamesDailyChanged();
  nonoS->carryMs = total % 1000, nonoS->lastMs = now;
}
static void nonoResume() {  // in demo mode too (nothing is saved)
  if (nonoNow().done) return;
  nonoS->running = true, nonoS->lastMs = millis();
}
// Leaving the puzzle (Back, Size, home): the clock stops, the time is kept.
static void nonogramPause() {
  if (!nonoS || !nonoS->running) return;
  nonoTick();
  nonoS->running = false;
}

// ---- the Daily screen's line

static String nonogramLine() {
  long today = gToday();
  String done;
  for (int z = 0; z < nonogrid::kSizes; z++)
    if (nonoSolvedToday(z)) done += String(done.length() ? ", " : "") + NONO_SIZE_SHOWN[z];
  if (done.length()) return TF(S_SUDOKU_L_DONE, done.c_str());
  if (gamesNonogramStreak(today)) return TP(S_G_DAYS_IN_ROW, gamesNonogramStreak(today));
  return T(S_NONO_L_HINT);
}

// ---- opening

static void nonoStart(int size) {
  NonoState &s = *nonoS;
  nonogramPause();
  s.size = constrain(size, 0, nonogrid::kSizes - 1);
  if (demoMode && !s.demoMade && s.size == 1) nonoDemoGrid();
  nonoGrid(s.size);
  s.nk = NK_PLAY, s.newBest = false, s.crossMode = false;
  GameProgress &p = gameProgress();
  if (p.nonSize != s.size) p.nonSize = s.size, gamesChanged();
  nonoResume();
}

// The Nonogram from the Daily screen: the size last played if it's under way today, else the sizes card.
static void nonogramOpen() {
  nonoEnsure();
  int z = gameProgress().nonSize;
  bool demoFirst = demoMode && !nonoS->demoMade;  // demo mode's first open: the partly done 10 x 10
  if (demoFirst) z = 1;
  if (gToday() && (demoFirst || (nonoStartedToday(z) && !nonoSolvedToday(z)))) nonoStart(z);
  else nonoS->nk = NK_CARD, nonoS->size = z;
}

// Demo mode ended (crossplayDemoEnded): the next open starts from the real progress again.
static void nonogramForget() {
  if (!nonoS) return;
  nonoS->demoMade = false, nonoS->running = false, nonoS->nk = NK_CARD;
}

// ---- geometry: the clues' room, the square, where the grid starts

// A row's or column's clue as text, the blocks with `sep` between, for the size on screen.
static String nonoClueText(bool column, int i, const char *sep) {
  const nonogrid::Puzzle &p = nonoPuzzle(nonoS->size);
  String s;
  for (int k = 0; k < nonogrid::clueCount(p, column, i); k++) s += (k ? sep : "") + String(nonogrid::clueAt(p, column, i, k));
  return s;
}

static const int NONO_CLUE_LINE = 17;  // the column clues' line height (9pt)

// Upright: about 39 px squares at 10 x 10, 48 at 8 x 8; sideways about 32 and 40, with 250 px beside.
static void nonoGeometry(int &gx, int &gy, int &c) {
  const nonogrid::Puzzle &p = nonoPuzzle(nonoS->size);
  int side = p.side, rowW = 0, colN = 1;
  for (int i = 0; i < side; i++) {
    rowW = max(rowW, textW(&FreeSans9pt7b, nonoClueText(false, i, " ").c_str()));
    colN = max(colN, nonogrid::clueCount(p, true, i));
  }
  int leftW = rowW + 12, topH = colN * NONO_CLUE_LINE + 8;
  if (TALL) {
    int top = gTop() + 4, availW = W - 16 - leftW, availH = H - 150 - top - topH;
    c = min(48, min(availW / side, availH / side));
    gx = (W - leftW - side * c) / 2 + leftW, gy = top + topH;
  } else {
    int availW = W - 16 - 250 - 24 - leftW, availH = BODY_H - 16 - topH;
    c = min(48, min(availW / side, availH / side));
    gx = 16 + leftW, gy = BODY_Y + 8 + topH + (availH - side * c) / 2;
  }
}

// ---- pictures

// The Daily screen's little picture of a nonogram: a heart in a 5 x 5 grid.
static void nonogramIcon(int cx, int cy, int s) {
  static const uint8_t HEART[5] = {0x0A, 0x1F, 0x1F, 0x0E, 0x04};
  int t = max(2, s / 18), c = s / 5, x0 = cx - 5 * c / 2, y0 = cy - 5 * c / 2;
  for (int r = 0; r < 5; r++)
    for (int k = 0; k < 5; k++) {
      canvas->drawRect(x0 + k * c, y0 + r * c, c + 1, c + 1, INK);
      if (HEART[r] >> (4 - k) & 1) canvas->fillRect(x0 + k * c + 2, y0 + r * c + 2, c - 3, c - 3, INK);
    }
  for (int k = 1; k < t; k++) canvas->drawRect(x0 - k, y0 - k, 5 * c + 1 + 2 * k, 5 * c + 1 + 2 * k, INK);
}

static void nonoBoard() {
  const GameProgress::NonogramPlay &g = nonoNow();
  const nonogrid::Puzzle &p = nonoPuzzle(nonoS->size);
  int gx, gy, c;
  nonoGeometry(gx, gy, c);
  int side = p.side, every = side == 10 ? 5 : 4;
  bool live = !g.done;
  for (int i = 0; i < side; i++) {  // the clues, struck once their line is right
    String rowClue = nonoClueText(false, i, " ");
    int ry = gy + i * c + c / 2 + 6, rw = textW(&FreeSans9pt7b, rowClue.c_str());
    textRight(gx - 7, ry, &FreeSans9pt7b, rowClue.c_str());
    if (nonogrid::lineDone(p, g.marks, false, i)) canvas->fillRect(gx - 9 - rw, ry - 6, rw + 4, 2, INK);
    bool colDone = nonogrid::lineDone(p, g.marks, true, i);
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
      if (g.marks[id] == nonogrid::kFilled) canvas->fillRect(x + 3, y + 3, c - 5, c - 5, INK);
      else if (g.marks[id] == nonogrid::kCrossed) gGlyphX(x + c / 2, y + c / 2, c * 22 / 100, 2);
      if (live) gTarget(x + 1, y + 1, c - 1, c - 1, id);
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

// Solved: the time (a best?), the streak, come back tomorrow; from y down, centred on cx (as Sudoku's).
static void nonoSolvedPanel(int cx, int y, int w) {
  long today = gToday();
  textCentre(cx, y + 34, &FreeSansBold24pt7b, fitted(&FreeSansBold24pt7b, T(S_SUDOKU_SOLVED), w).c_str());
  String time = TF(S_SUDOKU_SOLVED_IN, sdkTime(nonoNow().secs).c_str());
  textCentre(cx, y + 76, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, time, w).c_str());
  int yy = y + 76;
  if (nonoS->newBest) yy += 28, textCentre(cx, yy, &FreeSans12pt7b, fitted(&FreeSans12pt7b, T(S_SUDOKU_NEW_BEST), w).c_str());
  if (gamesNonogramStreak(today)) yy += 30, textCentre(cx, yy, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, TP(S_G_DAYS_IN_ROW, gamesNonogramStreak(today)), w).c_str());
  gCentred(cx, yy + 30, &FreeSans12pt7b, T(S_SUDOKU_TOMORROW), w, 26, 2);
}

static void drawNonogramPlay() {
  NonoState &s = *nonoS;
  const GameProgress::NonogramPlay &g = nonoNow();
  int gx, gy, c;
  nonoGeometry(gx, gy, c);
  int side = nonoPuzzle(s.size).side * c;
  String info = String(NONO_SIZE_SHOWN[s.size]) + "  " + sdkTime(nonoSecsShown());
  const char *how = T(s.crossMode ? S_NONO_HOW_CROSS : S_NONO_HOW_FILL);
  if (TALL) {  // Back, the size and the time, Size; the grid; Fill / Cross; how to play
    gButton(16, BODY_Y + 10, 104, 48, T(S_G_BACK), false, ID_BACK);
    gButton(W - 16 - 120, BODY_Y + 10, 120, 48, T(S_NONO_SIZE), false, NONO_SIZES, &FreeSansBold9pt7b);
    textCentre((16 + 104 + W - 16 - 120) / 2, BODY_Y + 42, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, info, W - 280).c_str());
    nonoBoard();
    int y = gy + side + 14;
    if (g.done) return nonoSolvedPanel(W / 2, y, W - 40);
    nonoModes(24, y, W - 48, 52);
    gCentred(W / 2, y + 52 + 28, &FreeSans9pt7b, how, W - 48, 20, max(1, (H - 8 - (y + 80)) / 20 + 1));
  } else {  // the grid on the left; beside it Back and Size, the size and the time, Fill / Cross, how to play
    nonoBoard();
    int px = gx + side + 24, pw = W - 16 - px;
    gButton(px, BODY_Y + 8, 110, 48, T(S_G_BACK), false, ID_BACK);
    gButton(W - 16 - 124, BODY_Y + 8, 124, 48, T(S_NONO_SIZE), false, NONO_SIZES, &FreeSansBold9pt7b);
    textCentre(px + pw / 2, BODY_Y + 88, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, info, pw).c_str());
    if (g.done) return nonoSolvedPanel(px + pw / 2, BODY_Y + 104, pw);
    nonoModes(px, BODY_Y + 104, pw, 56);
    gLeft(px, BODY_Y + 104 + 56 + 32, &FreeSans9pt7b, how, pw, 20, 6);
  }
}

// The sizes card: a button a size (a tick when solved today), how it's going (as Sudoku's levels card).
static void drawNonogramCard() {
  GameProgress &p = gameProgress();
  long today = gToday();
  gHeader(T(S_NONO_CARD));
  if (!today) {
    gCentred(W / 2, gTop() + 80, &FreeSans12pt7b, T(S_SUDOKU_NO_DATE), W - 80, 30, 4);
    return;
  }
  int bw = min(W - 48, 420), bx = (W - bw) / 2, step = TALL ? 150 : 120, y = gTop() + (TALL ? 24 : 16);
  for (int z = 0; z < nonogrid::kSizes; z++, y += step) {
    gButton(bx, y, bw, TALL ? 64 : 56, T(nonoSizeName(z)), false, NONO_SIZE + z);
    if (nonoSolvedToday(z)) sdkCheckMark(bx + bw - 58, y + (TALL ? 44 : 40), 30, INK);
    const GameProgress::NonogramPlay &g = p.nonPlay[z];
    String state;
    if (nonoSolvedToday(z)) state = TF(S_SUDOKU_SOLVED_IN, sdkTime(g.secs).c_str());
    else if (nonoStartedToday(z) && g.secs) state = TF(S_SUDOKU_UNDER_WAY, sdkTime(g.secs).c_str());
    if (p.nonBest[z] && !nonoSolvedToday(z)) state += String(state.length() ? "   " : "") + TF(S_SUDOKU_BEST, sdkTime(p.nonBest[z]).c_str());
    if (state.length()) textCentre(W / 2, y + (TALL ? 64 : 56) + 26, &FreeSansBold9pt7b, fitted(&FreeSansBold9pt7b, state, bw).c_str());
  }
  int streak = gamesNonogramStreak(today);
  if (streak) textCentre(W / 2, H - (TALL ? 70 : 34), &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, TP(S_G_DAYS_IN_ROW, streak), W - 40).c_str());
  if (TALL) textCentre(W / 2, H - 30, &FreeSans9pt7b, fitted(&FreeSans9pt7b, T(S_DAILY_FOOT), W - 32).c_str());
}

static void drawNonogram() {
  nonoEnsure();
  if (nonoS->nk == NK_PLAY && gToday() && nonoMake(nonoS->size)) {
    nonoGrid(nonoS->size);  // a new day since it was opened: today's, afresh
    return drawNonogramPlay();
  }
  nonoS->nk = NK_CARD;
  drawNonogramCard();
}

// ---- playing

static void nonoSolved() {
  NonoState &s = *nonoS;
  GameProgress &p = gameProgress();
  GameProgress::NonogramPlay &g = nonoNow();
  nonoTick();
  s.running = false;
  g.done = true;
  int secs = max(1, g.secs);
  s.newBest = !p.nonBest[s.size] || secs < p.nonBest[s.size];
  if (s.newBest) p.nonBest[s.size] = secs;
  gamesNonogramSolved(gToday());
  gamesChanged(), gamesDailyChanged();
  gs->gFocusId = -1;
  Serial.printf("nonogram: %s solved in %d s\n", NONO_SIZE_IDS[s.size], secs);
  beep(2400, 120);
  render(true);
}

static void nonoSet(int cell, uint8_t mark) {
  GameProgress::NonogramPlay &g = nonoNow();
  g.marks[cell] = mark;
  nonoTick();
  gamesDailyChanged();
  if (nonogrid::solved(nonoPuzzle(nonoS->size), g.marks)) return nonoSolved();
  beep(2600, 15);
  render(false);
}

// A tap on the Nonogram's screens; ID_BACK is the Daily screen's (games_sudoku.ino: actDaily).
static void actNonogram(int id) {
  nonoEnsure();
  NonoState &s = *nonoS;
  if (s.nk == NK_CARD) {
    if (id >= NONO_SIZE && id < NONO_SIZE + nonogrid::kSizes && gToday()) return nonoStart(id - NONO_SIZE), render(true);
    return;
  }
  if (id == NONO_SIZES) return nonogramPause(), s.nk = NK_CARD, render(false);
  if (!nonoMake(s.size)) return;
  GameProgress::NonogramPlay &g = nonoGrid(s.size);
  if (g.done) return;
  if (id == NONO_FILL || id == NONO_CROSS) {
    if (s.crossMode == (id == NONO_CROSS)) return;
    s.crossMode = id == NONO_CROSS;
    return render(false);
  }
  int side = nonoPuzzle(s.size).side;
  if (id < 0 || id >= side * side) return;
  uint8_t want = s.crossMode ? nonogrid::kCrossed : nonogrid::kFilled;
  nonoSet(id, g.marks[id] == want ? (uint8_t)nonogrid::kUnknown : want);
}

// OK with nothing ringed: the card's first size not solved; in a puzzle Fill <-> Cross; solved, the card.
static bool nonogramOk() {
  nonoEnsure();
  NonoState &s = *nonoS;
  if (s.nk == NK_CARD) {
    for (int z = 0; z < nonogrid::kSizes; z++)
      if (!nonoSolvedToday(z)) return actNonogram(NONO_SIZE + z), true;
    return false;
  }
  if (nonoNow().done) return actNonogram(NONO_SIZES), true;
  actNonogram(s.crossMode ? NONO_FILL : NONO_CROSS);
  return true;
}

// `game right` / `game wrong`: nonogrid::next()'s square gets its mark, or the other one.
static bool nonogramAnswer(bool right) {
  if (!nonoS || nonoS->nk != NK_PLAY || !nonoMake(nonoS->size)) return false;
  GameProgress::NonogramPlay &g = nonoGrid(nonoS->size);
  const nonogrid::Puzzle &p = nonoPuzzle(nonoS->size);
  if (g.done) return false;
  uint8_t mark = 0;
  int c = nonogrid::next(p, g.marks, &mark);
  if (c < 0) return false;
  if (!right) {
    mark = mark == nonogrid::kFilled ? nonogrid::kCrossed : nonogrid::kFilled;
    if (g.marks[c] == mark)  // wrong there already: the first unknown square gets a wrong mark
      for (int i = 0; i < p.side * p.side; i++)
        if (g.marks[i] == nonogrid::kUnknown) {
          c = i, mark = p.filled[i] ? nonogrid::kCrossed : nonogrid::kFilled;
          break;
        }
  }
  nonoSet(c, mark);
  return true;
}

static void nonogramPeek(String &s) {
  nonoEnsure();
  NonoState &st = *nonoS;
  long today = gToday();
  if (!today || !nonoMake(st.size) || (st.nk == NK_CARD && !nonoStartedToday(st.size))) {
    s += "nonogram none " + String(today) + " card";
    return;
  }
  const nonogrid::Puzzle &p = nonoPuzzle(st.size);
  const GameProgress::NonogramPlay &g = gameProgress().nonPlay[st.size];
  bool current = g.date == today && g.check == nonoCheck(st.size);
  uint8_t marks[nonogrid::kMaxCells] = {};
  if (current) memcpy(marks, g.marks, sizeof marks);
  String rows, cols, grid;
  for (int i = 0; i < p.side; i++) {
    if (i) rows += '|', cols += '|', grid += '|';
    rows += nonoClueText(false, i, ",");
    cols += nonoClueText(true, i, ",");
    for (int k = 0; k < p.side; k++) {
      uint8_t m = marks[i * p.side + k];
      grid += m == nonogrid::kFilled ? '#' : m == nonogrid::kCrossed ? 'x' : '.';
    }
  }
  uint8_t mark = 0;
  int next = nonogrid::next(p, marks, &mark);
  bool done = nonogrid::solved(p, marks);
  char tail[120];
  snprintf(tail, sizeof tail, " next %d=%s done %d mode %s time %ds made %lums", done ? -1 : next,
           done || next < 0 ? "none" : mark == nonogrid::kFilled ? "fill" : "cross", done, st.crossMode ? "cross" : "fill",
           st.nk == NK_PLAY && current ? nonoSecsShown() : g.secs, (unsigned long)st.madeMs[st.size]);
  s += String("nonogram ") + NONO_SIZE_IDS[st.size] + " " + today + " rows " + rows + " cols " + cols + " grid " + grid + tail;
  if (st.nk == NK_CARD) s += " card";
}
