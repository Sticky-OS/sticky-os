// ---------------------------------------------------------------- Daily, and its Sudoku
//
// The Games folder's Daily tile (GAME_DAILY, issue #18) opens the Daily screen: a row for Daily
// maths (games.ino's maths round, unchanged; its Back comes back here), a row for Sudoku and a
// row for the Nonogram (games_nonogram.ino, which this tab sends its taps to). Like
// the other games (gHeader, gButton, gTarget; games.ino), this tab draws them and takes the taps.
// Its state lives in PSRAM (dailyS, dailyEnsure()).
//
// Sudoku: the puzzle of the day at three levels, made on the board from the date (sudokugrid.h:
// the same on every board), once a day: kept in PSRAM, so reopening never makes it again. Tap a
// square (ringed; its row, column and box hatched lightly), then a number on the pad; Pencil puts
// numbers in small, as notes; Erase clears a square, Undo takes back a step. Given numbers are
// bold and can't change; nothing you put in is refused or flagged until you ask. Hint (issue #25):
// your numbers that aren't the solution's are struck through ("2 of your numbers are wrong"), and
// stay so until you change them; with none wrong, a square is ringed and the board says why it's
// the one to do (only one number fits; the only place for a number in its row, column or box; or
// just "try this one"), and Hint again puts its number in. Each tap that shows something counts;
// a puzzle solved with hints keeps its streak and shows them, but sets no best time. Placing a
// number takes its pencil marks off the row, column and box. Solved: a full refresh, the time,
// the streak, "Come back tomorrow".
//
// Today's grids (numbers, pencil marks, time) are in GameProgress::sdkPlay, written to
// /daily.txt from the loop (gamesDailyChanged()), so a day's work survives deep sleep; a grid for
// another date is dropped when its level is opened. The clock counts the time the puzzle is on
// screen, a gap between taps counting at most SDK_GAP_MS (a board left alone doesn't run it up).
//
// Ids, for taps, the ring and `game pick <id>`:
//   10 Daily maths, 11 Sudoku, 12 Nonogram (the Daily screen; the Nonogram's own: games_nonogram.ino)
//   0..80 a square (row * 9 + column); 101..109 the numbers; 110 Erase; 111 Pencil; 112 Undo; 113 Hint
//   120 / 121 / 122 a level (the levels card); 130 Levels (back to the card); ID_BACK (900)
// `game peek`: "sudoku <level> <date> grid <81: digit or .> given <81: g or -> sel <cell> next
// <cell>=<digit> clashes <n> done <0|1> hints <n> struck <n> ringed <cell>", then " card" when the
// levels card is up. On the Daily
// screen: "daily maths <done 0|1> streak <n>, sudoku <easy medium hard done 0|1> streak <n>,
// nonogram <8x8 10x10 done 0|1> streak <n>".
// `game right` / `game wrong`: the next square (sudokugrid::nextCell) gets its number, or a wrong one.
// Demo mode (`demo on`): the first Sudoku opened is Medium, half done, with pencil marks and a
// wrong number a hint has struck through, from 12:34; nothing is saved.
#include "sudokugrid.h"

enum { DY_MENU, DY_SUDOKU, DY_NONO };  // the Daily screen, Sudoku, or the Nonogram
enum { SK_CARD, SK_PLAY };         // Sudoku: the levels card, or a puzzle
enum { DY_MATHS = 10, DY_SDK = 11, DY_NONOGRAM = 12, SK_DIGIT = 100, SK_ERASE = 110, SK_PENCIL = 111, SK_UNDO = 112, SK_HINT = 113, SK_LEVEL = 120, SK_LEVELS = 130 };
enum { SAY_NONE, SAY_GIVEN, SAY_WRONG, SAY_HINT };
static const uint32_t SDK_GAP_MS = 10 * 60 * 1000UL;
static const int SDK_UNDO = 32;

struct DailyState {
  int screen = DY_MENU, sk = SK_CARD;
  int level = 0, sel = -1, say = SAY_NONE;
  int wrongN = 0;                 // SAY_WRONG: how many a hint struck through
  sudokugrid::Hint hint;          // SAY_HINT: the square it ringed (cell -1: none); Hint again fills it
  bool pencil = false, newBest = false, demoMade = false;
  bool running = false;           // the clock
  uint32_t lastMs = 0, carryMs = 0;
  sudokugrid::Puzzle cache[3];    // the day's puzzles, made when first wanted (date -1: none)
  sudokugrid::Work work;          // the generator's room
  sudokugrid::Puzzle spare;
  struct Snap {
    uint8_t value[81];
    uint16_t pencil[81];
  } undo[SDK_UNDO];
  int undoN = 0, undoTop = 0;     // steps kept; where the next goes
};
static DailyState *dailyS = nullptr;
// Made on first use. No function takes or returns DailyState or a sudokugrid type: the sketch's
// generated prototypes come before this tab, so they can't name them.
static void dailyEnsure() {
  if (dailyS) return;
  void *p = heap_caps_malloc(sizeof(DailyState), MALLOC_CAP_SPIRAM);
  dailyS = p ? new (p) DailyState() : new DailyState();
}

static StrId sdkLevelName(int l) { return l == 0 ? S_SUDOKU_EASY : l == 1 ? S_SUDOKU_MEDIUM : S_SUDOKU_HARD; }
static StrId sdkLevelHow(int l) { return l == 0 ? S_SUDOKU_EASY_HOW : l == 1 ? S_SUDOKU_MEDIUM_HOW : S_SUDOKU_HARD_HOW; }
static const char *const SDK_LEVEL_IDS[3] = {"easy", "medium", "hard"};

static String sdkTime(int secs) {
  char b[16];
  if (secs >= 3600) snprintf(b, sizeof b, "%d:%02d:%02d", secs / 3600, secs / 60 % 60, secs % 60);
  else snprintf(b, sizeof b, "%d:%02d", secs / 60, secs % 60);
  return b;
}

// ---- the day's puzzle and today's grid

// The day's puzzle at a level, made once: seeded from the date, then the games' generator back
// to a fresh seed (as mathDaily does).
static void sdkMake(int level) {
  dailyEnsure();
  long day = gToday();
  sudokugrid::Puzzle &p = dailyS->cache[level];
  if (p.date != day) {
    uint32_t t0 = millis();
    gameSeed(sudokugrid::seed(day, level));
    sudokugrid::make(p, day, level, dailyS->work, dailyS->spare, gameRnd);
    gameSeed(0);
    Serial.printf("sudoku: %s for %ld made in %lu ms: %d clues, needs %d, try %d of %d, %lu nodes\n", SDK_LEVEL_IDS[level], day,
                  (unsigned long)(millis() - t0), p.clues, (int)p.needs, p.tries, p.attempts, (unsigned long)p.nodes);
  }
}
// The puzzle itself: a macro, as no function may return a sudokugrid type (see dailyEnsure).
#define sdkPuzzle(level) (sdkMake(level), dailyS->cache[(level)])

static bool sdkStartedToday(int level) {
  const GameProgress::SudokuPlay &s = gameProgress().sdkPlay[level];
  return gToday() && s.date == gToday();
}
static bool sdkSolvedToday(int level) { return sdkStartedToday(level) && gameProgress().sdkPlay[level].done; }

// Today's grid at a level, fresh when it's for another day (or doesn't fit the puzzle).
static GameProgress::SudokuPlay &sdkGrid(int level) {
  GameProgress::SudokuPlay &s = gameProgress().sdkPlay[level];
  const sudokugrid::Puzzle &p = sdkPuzzle(level);
  if (s.date != gToday() || !sudokugrid::fits(s.value, p)) {
    s = GameProgress::SudokuPlay();
    s.date = gToday();
    memcpy(s.value, p.given, 81);
    gamesDailyChanged();
  }
  return s;
}
static GameProgress::SudokuPlay &sdkNow() { return gameProgress().sdkPlay[dailyS->level]; }

// The demo's grid: Medium, about half the empty squares filled, a few pencil marks, a clash struck through.
static void sdkDemoGrid() {
  const sudokugrid::Puzzle &p = sdkPuzzle(1);
  GameProgress::SudokuPlay &s = gameProgress().sdkPlay[1];
  s = GameProgress::SudokuPlay();
  s.date = gToday(), s.secs = 754;
  memcpy(s.value, p.given, 81);
  int empties = 0, clash = -1;
  for (int i = 0; i < 81; i++) {
    if (p.given[i]) continue;
    if (empties++ % 2 == 0 && i < 54) s.value[i] = p.solution[i];
    else if (i >= 54 && clash < 0) clash = i;
    else if (empties % 3 == 0) s.pencil[i] = (1u << p.solution[i]) | (1u << (p.solution[i] % 9 + 1));
  }
  if (clash >= 0) {  // a number that's already in its row, which a hint has struck through
    for (int j = clash - clash % 9; j < clash - clash % 9 + 9; j++)
      if (p.given[j] && p.given[j] != p.solution[clash]) s.value[clash] = p.given[j];
    sudokugrid::wrongCells(s.value, p, s.marked);
    s.hints = 1;
  }
  dailyS->level = 1, dailyS->sel = clash, dailyS->demoMade = true;
}

// ---- the clock

static int sdkSecsShown() {
  if (!dailyS || dailyS->level < 0) return 0;
  const GameProgress::SudokuPlay &s = sdkNow();
  if (!dailyS->running) return s.secs;
  uint32_t gap = min<uint32_t>(millis() - dailyS->lastMs, SDK_GAP_MS);
  return s.secs + (dailyS->carryMs + gap) / 1000;
}
static void sdkTick() {
  if (!dailyS->running) return;
  uint32_t now = millis(), total = dailyS->carryMs + min<uint32_t>(now - dailyS->lastMs, SDK_GAP_MS);
  GameProgress::SudokuPlay &s = sdkNow();
  if (total >= 1000) s.secs += total / 1000, gamesDailyChanged();
  dailyS->carryMs = total % 1000, dailyS->lastMs = now;
}
static void sdkResume() {  // in demo mode too (nothing is saved): screenshots show a clock that runs
  if (sdkNow().done) return;
  dailyS->running = true, dailyS->lastMs = millis();
}
// Leaving the puzzle (Back, Levels, home): the clock stops, the time is kept.
static void sdkPause() {
  if (!dailyS || !dailyS->running) return;
  sdkTick();
  dailyS->running = false;
}

// ---- the Daily screen's lines (also the launcher's)

static String sdkLine() {
  long today = gToday();
  String done;
  for (int l = 0; l < 3; l++)
    if (sdkSolvedToday(l)) done += String(done.length() ? ", " : "") + T(sdkLevelName(l));
  if (done.length()) return TF(S_SUDOKU_L_DONE, done.c_str());
  if (gamesSudokuStreak(today)) return TP(S_G_DAYS_IN_ROW, gamesSudokuStreak(today));
  return T(S_SUDOKU_L_HINT);
}
static String dailyMathsLine() {
  long today = gToday();
  return gamesDailyDone(today) ? String(T(S_G_L_DONE_TODAY)) : gamesDailyStreak(today) ? TP(S_G_DAYS_IN_ROW, gamesDailyStreak(today)) : String(T(S_G_L_FIVE_A_DAY));
}

static String dailyLine() {
  long today = gToday();
  int done = gamesDailyDone(today) + gamesSudokuDone(today) + gamesNonogramDone(today);
  if (done) return TF(S_DAILY_L_DONE, done, 3);
  int streak = max(gamesDailyStreak(today), max(gamesSudokuStreak(today), gamesNonogramStreak(today)));
  if (streak) return TP(S_G_DAYS_IN_ROW, streak);
  return T(S_DAILY_L_HINT);
}

// ---- opening

static void dailyOpen() {
  dailyEnsure();
  dailyS->screen = DY_MENU;
}

// Demo mode ended (crossplayDemoEnded): the next open makes today's grid again from the real progress.
static void dailyForget() {
  if (!dailyS) return;
  dailyS->demoMade = false, dailyS->running = false, dailyS->sel = -1, dailyS->undoN = 0;
  dailyS->screen = DY_MENU;
  nonogramForget();
}

static void sdkStart(int level) {
  DailyState &d = *dailyS;
  sdkPause();
  d.level = constrain(level, 0, 2);
  bool demoGrid = demoMode && !d.demoMade && d.level == 1;
  if (demoGrid) sdkDemoGrid();
  sdkGrid(d.level);
  d.sk = SK_PLAY, d.say = SAY_NONE, d.newBest = false, d.undoN = 0, d.pencil = false, d.hint.cell = -1;
  if (!demoGrid) d.sel = -1;
  GameProgress &p = gameProgress();
  if (p.sdkLevel != d.level) p.sdkLevel = d.level, gamesChanged();
  sdkResume();
}

// Sudoku from the Daily screen: the level last played if it's under way today, else the levels card.
static void sdkOpen() {
  DailyState &d = *dailyS;
  d.screen = DY_SUDOKU;
  int l = gameProgress().sdkLevel;
  if (demoMode && !d.demoMade) l = 1;
  bool demoFirst = demoMode && !d.demoMade;  // demo mode's first open: the half-done Medium
  if (gToday() && (demoFirst || (sdkStartedToday(l) && !sdkSolvedToday(l)))) sdkStart(l);
  else d.sk = SK_CARD;
}

// ---- geometry: portrait 51 px squares with the pad below; sideways 46 px with the pad beside

static void sdkGeometry(int &x0, int &y0, int &c) {
  if (TALL) c = 51, x0 = (W - 9 * c) / 2, y0 = gTop() + 4;
  else c = 46, x0 = 12, y0 = BODY_Y + (BODY_H - 9 * c) / 2;
}

// ---- pictures

// The launcher's and the Daily screen's little picture of a sudoku: a 3 x 3 of boxes, one
// square black, numbers in two.
static void sudokuIcon(int cx, int cy, int s) {
  int r = s / 2, t = max(2, s / 18), c = s / 3;
  for (int k = 0; k < t; k++) canvas->drawRect(cx - r + k, cy - r + k, s - 2 * k, s - 2 * k, INK);
  for (int i = 1; i < 3; i++) {
    canvas->fillRect(cx - r + i * c - t / 2, cy - r, max(1, t - 1), s, INK);
    canvas->fillRect(cx - r, cy - r + i * c - t / 2, s, max(1, t - 1), INK);
  }
  canvas->fillRect(cx - r + 2 * c + 1, cy - r + t, c - t - 1, c - t, INK);
  if (s < 40) return;
  const GFXfont *f = s >= 56 ? &FreeSansBold12pt7b : &FreeSansBold9pt7b;
  int half = (s >= 56 ? 17 : 13) / 2;  // half a digit's height
  auto at = [&](int col, int row, const char *d) { textCentre(cx - r + col * c + c / 2 + 1, cy - r + row * c + c / 2 + half, f, d); };
  at(0, 0, "5"), at(1, 1, "3"), at(2, 2, "9");
}

// The Daily tile: a calendar page with a star (as Daily maths had).
static void dailyIcon(int cx, int cy, int s) {
  int r = s / 2, t = max(2, s / 18);
  int x = cx - r, y = cy - r + s / 10, w = s, h = s - s / 10;
  canvas->fillRoundRect(x, y, w, h * 30 / 100, 6, INK);
  for (int k = 0; k < t; k++) canvas->drawRoundRect(x + k, y + k, w - 2 * k, h - 2 * k, 6, INK);
  canvas->fillRoundRect(x + w / 4 - 2, y - s / 10, 5, s / 5, 2, INK);
  canvas->fillRoundRect(x + w * 3 / 4 - 2, y - s / 10, 5, s / 5, 2, INK);
  gStar(cx, y + h * 64 / 100, h * 26 / 100, true);
}

static void sdkCheckMark(int x, int y, int s, uint16_t ink) {  // a tick, s wide, its foot at x + s / 3, y
  thickLine(x, y - s / 3, x + s / 3, y, 4, ink);
  thickLine(x + s / 3, y, x + s, y - s * 2 / 3, 4, ink);
}

// One square: hatched lightly when it's in the picked square's row, column or box; ringed when
// it's the picked one; its number (bold given, regular yours, struck through when a hint showed it
// wrong) or its pencil marks.
static void sdkDrawCell(int i) {
  DailyState &d = *dailyS;
  const sudokugrid::Puzzle &p = sdkPuzzle(d.level);
  const GameProgress::SudokuPlay &s = sdkNow();
  int x0, y0, c;
  sdkGeometry(x0, y0, c);
  int x = x0 + sudokugrid::colOf(i) * c, y = y0 + sudokugrid::rowOf(i) * c;
  bool live = !s.done;
  if (live && d.sel >= 0 && sudokugrid::peers(i, d.sel)) gHatch(x + 2, y + 2, c - 3, c - 3, 14);
  int v = s.value[i];
  if (v) {
    bool given = p.given[i] != 0;
    const GFXfont *f = given ? &FreeSansBold18pt7b : &FreeSans18pt7b;
    char t[2] = {(char)('0' + v), 0};
    int tw = textW(f, t), bx = x + c / 2 - tw / 2;
    canvas->fillRect(bx - 4, y + c / 2 - 15, tw + 8, 30, PAPER);  // clear of the hatching
    text(bx, y + c / 2 + 12, f, t);
    if (!given && sudokugrid::marked(s.marked, i)) thickLine(x + c / 5, y + c * 4 / 5, x + c * 4 / 5, y + c / 5, 3);
  } else if (s.pencil[i]) {
    int sub = c / 3;
    for (int k = 1; k <= 9; k++) {
      if (!(s.pencil[i] & (1u << k))) continue;
      char t[2] = {(char)('0' + k), 0};
      int cx = x + 1 + ((k - 1) % 3) * sub + sub / 2, cy = y + 1 + ((k - 1) / 3) * sub + sub / 2;
      canvas->fillRect(cx - 6, cy - 8, 12, 16, PAPER);
      textCentre(cx + 1, cy + 6, &FreeSans9pt7b, t);
    }
  }
  if (live && i == d.sel)
    for (int k = 2; k < 6; k++) canvas->drawRect(x + k, y + k, c + 1 - 2 * k, c + 1 - 2 * k, INK);
}

static void sdkDrawBoard() {
  DailyState &d = *dailyS;
  int x0, y0, c;
  sdkGeometry(x0, y0, c);
  bool live = !sdkNow().done;
  for (int i = 0; i < 81; i++) {
    int x = x0 + sudokugrid::colOf(i) * c, y = y0 + sudokugrid::rowOf(i) * c;
    canvas->drawRect(x, y, c + 1, c + 1, INK);
    sdkDrawCell(i);
    if (live) gTarget(x + 2, y + 2, c - 3, c - 3, i);
  }
  for (int b = 0; b <= 9; b += 3)  // the boxes' lines, and the frame
    for (int k = -1; k <= 1; k++) {
      canvas->drawFastVLine(x0 + b * c + k, y0 - 1, 9 * c + 3, INK);
      canvas->drawFastHLine(x0 - 1, y0 + b * c + k, 9 * c + 3, INK);
    }
  for (int k = 2; k <= 3; k++) canvas->drawRect(x0 - k, y0 - k, 9 * c + 1 + 2 * k, 9 * c + 1 + 2 * k, INK);
  (void)d;
}

// A number on the pad: bold; thin and regular once all nine are placed.
static void sdkDigitKey(int x, int y, int w, int h, int digit) {
  const GameProgress::SudokuPlay &s = sdkNow();
  int n = 0;
  for (int i = 0; i < 81; i++) n += s.value[i] == digit;
  bool all = n >= 9;
  int r = min(14, h / 3);
  canvas->drawRoundRect(x, y, w, h, r, INK);
  if (!all) canvas->drawRoundRect(x + 1, y + 1, w - 2, h - 2, r - 1, INK), canvas->drawRoundRect(x + 2, y + 2, w - 4, h - 4, r - 2, INK);
  char t[2] = {(char)('0' + digit), 0};
  if (dailyS->pencil) {  // pencil: the numbers small, as they'll go in
    textCentre(x + w / 2 + 1, y + h / 2 + 8, all ? &FreeSans12pt7b : &FreeSansBold12pt7b, t);
  } else {
    const GFXfont *f = all ? &FreeSans18pt7b : &FreeSansBold24pt7b;
    textCentre(x + w / 2 + 1, y + h / 2 + (all ? 12 : 17), f, t);
  }
  gTarget(x, y, w, h, SK_DIGIT + digit);
}

// Erase, Pencil, Undo and Hint: in a row of `perRow` (4 upright, 2 sideways: two rows), each h high.
static void sdkTools(int x, int y, int w, int h, int gap, int perRow) {
  int bw = (w - (perRow - 1) * gap) / perRow;
  const GFXfont *f = &FreeSansBold12pt7b;
  for (StrId id : {S_SUDOKU_ERASE, S_SUDOKU_PENCIL, S_SUDOKU_UNDO, S_SUDOKU_HINT})
    if (textW(f, T(id)) > bw - 14) f = &FreeSansBold9pt7b;
  struct Tool {
    StrId label;
    int id;
  } tools[] = {{S_SUDOKU_ERASE, SK_ERASE}, {S_SUDOKU_PENCIL, SK_PENCIL}, {S_SUDOKU_UNDO, SK_UNDO}, {S_SUDOKU_HINT, SK_HINT}};
  for (int k = 0; k < 4; k++) {
    int tx = x + k % perRow * (bw + gap), ty = y + k / perRow * (h + gap);
    if (tools[k].id == SK_UNDO && !dailyS->undoN) {  // nothing to undo: a thin outline, the word in regular type, no target
      canvas->drawRoundRect(tx, ty, bw, h, min(h / 2, 20), INK);
      const GFXfont *rf = f == &FreeSansBold12pt7b ? &FreeSans12pt7b : &FreeSans9pt7b;
      textCentre(tx + bw / 2, ty + h / 2 + 8, rf, fitted(rf, T(S_SUDOKU_UNDO), bw - 14).c_str());
      continue;
    }
    // Four across upright leave ~107 px: the words get all but 7 px a side (gButton keeps 12).
    pillButton(tx, ty, bw, h, fitted(f, T(tools[k].label), bw - 14).c_str(), tools[k].id == SK_PENCIL && dailyS->pencil, f);
    gTarget(tx, ty, bw, h, tools[k].id);
  }
}

static String sdkSay() {
  DailyState &d = *dailyS;
  const GameProgress::SudokuPlay &s = sdkNow();
  if (d.say == SAY_GIVEN) return T(S_SUDOKU_GIVEN);
  if (d.say == SAY_WRONG) return TP(S_SUDOKU_HINT_WRONG, d.wrongN);
  if (d.say == SAY_HINT && d.hint.cell >= 0) {
    switch (d.hint.kind) {
      case sudokugrid::HINT_ONLY: return T(S_SUDOKU_HINT_ONLY);
      case sudokugrid::HINT_ROW: return TF(S_SUDOKU_HINT_ROW, d.hint.digit);
      case sudokugrid::HINT_COL: return TF(S_SUDOKU_HINT_COL, d.hint.digit);
      case sudokugrid::HINT_BOX: return TF(S_SUDOKU_HINT_BOX, d.hint.digit);
      default: return T(S_SUDOKU_HINT_NEXT);
    }
  }
  int empty = 0;
  for (int i = 0; i < 81; i++) empty += !s.value[i];
  if (d.pencil) return T(S_SUDOKU_HOW_PENCIL);
  if (d.sel < 0) return T(S_SUDOKU_HOW);
  return TP(S_SUDOKU_LEFT, empty);
}

// Solved: the time (a best?), the streak, come back tomorrow; from y down, centred on cx.
static void sdkSolvedPanel(int cx, int y, int w) {
  long today = gToday();
  textCentre(cx, y + 34, &FreeSansBold24pt7b, fitted(&FreeSansBold24pt7b, T(S_SUDOKU_SOLVED), w).c_str());
  String time = TF(S_SUDOKU_SOLVED_IN, sdkTime(sdkNow().secs).c_str());
  textCentre(cx, y + 76, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, time, w).c_str());
  int yy = y + 76;
  if (sdkNow().hints) yy += 28, textCentre(cx, yy, &FreeSans12pt7b, fitted(&FreeSans12pt7b, TP(S_SUDOKU_WITH_HINTS, sdkNow().hints), w).c_str());
  if (dailyS->newBest) yy += 28, textCentre(cx, yy, &FreeSans12pt7b, fitted(&FreeSans12pt7b, T(S_SUDOKU_NEW_BEST), w).c_str());
  if (gamesSudokuStreak(today)) yy += 30, textCentre(cx, yy, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, TP(S_G_DAYS_IN_ROW, gamesSudokuStreak(today)), w).c_str());
  gCentred(cx, yy + 30, &FreeSans12pt7b, T(S_SUDOKU_TOMORROW), w, 26, 2);
}

static void drawSudokuPlay() {
  DailyState &d = *dailyS;
  const GameProgress::SudokuPlay &s = sdkNow();
  int x0, y0, c;
  sdkGeometry(x0, y0, c);
  int side = 9 * c;
  String info = String(T(sdkLevelName(d.level))) + "  " + sdkTime(sdkSecsShown());
  if (TALL) {  // Back, the level and the time, Levels; the board; the pad; what's happening
    gButton(16, BODY_Y + 10, 104, 48, T(S_G_BACK), false, ID_BACK);
    gButton(W - 16 - 120, BODY_Y + 10, 120, 48, T(S_SUDOKU_LEVELS), false, SK_LEVELS, &FreeSansBold9pt7b);
    textCentre((16 + 104 + W - 16 - 120) / 2, BODY_Y + 42, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, info, W - 280).c_str());
    sdkDrawBoard();
    int py = y0 + side + 12;
    if (s.done) return sdkSolvedPanel(W / 2, py, W - 40);
    for (int k = 1; k <= 9; k++) sdkDigitKey(x0 + (k - 1) * c + 2, py, c - 4, 70, k);
    sdkTools(x0, py + 82, side, 54, 6, 4);
    gCentred(W / 2, py + 82 + 54 + 26, &FreeSans9pt7b, sdkSay(), W - 24, 20, 2);
  } else {  // the board on the left; beside it Back and Levels, the level and the time, the pad, what's happening
    sdkDrawBoard();
    int px = x0 + side + 22, pw = W - 14 - px;
    gButton(px, BODY_Y + 8, 110, 48, T(S_G_BACK), false, ID_BACK);
    gButton(W - 14 - 124, BODY_Y + 8, 124, 48, T(S_SUDOKU_LEVELS), false, SK_LEVELS, &FreeSansBold9pt7b);
    textCentre(px + pw / 2, BODY_Y + 88, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, info, pw).c_str());
    if (s.done) return sdkSolvedPanel(px + pw / 2, BODY_Y + 104, pw);
    // The tools in two rows of two (four across are too narrow for their words in some languages).
    int gap = 8, kw = (pw - 2 * gap) / 3, kh = 50, ky = BODY_Y + 100;
    for (int k = 1; k <= 9; k++) sdkDigitKey(px + (k - 1) % 3 * (kw + gap), ky + (k - 1) / 3 * (kh + gap), kw, kh, k);
    int ty = ky + 3 * (kh + gap) + 2, th = 44;
    sdkTools(px, ty, pw, th, gap, 2);
    gLeft(px, ty + 2 * th + gap + 18, &FreeSans9pt7b, sdkSay(), pw, 19, 3);
  }
}

// The levels card: a button a level (a tick when solved today), what it is, how it's going.
static void drawSudokuCard() {
  GameProgress &p = gameProgress();
  long today = gToday();
  gHeader(T(S_SUDOKU_TODAY));
  if (!today) {
    gCentred(W / 2, gTop() + 80, &FreeSans12pt7b, T(S_SUDOKU_NO_DATE), W - 80, 30, 4);
    return;
  }
  int bw = min(W - 48, 420), bx = (W - bw) / 2, step = TALL ? 150 : 104, y = gTop() + (TALL ? 24 : 8);
  for (int l = 0; l < 3; l++, y += step) {
    gButton(bx, y, bw, TALL ? 64 : 56, T(sdkLevelName(l)), false, SK_LEVEL + l);
    if (sdkSolvedToday(l)) sdkCheckMark(bx + bw - 58, y + (TALL ? 44 : 40), 30, INK);
    String how = T(sdkLevelHow(l)), state;
    const GameProgress::SudokuPlay &s = p.sdkPlay[l];
    if (sdkSolvedToday(l)) state = TF(S_SUDOKU_SOLVED_IN, sdkTime(s.secs).c_str());
    else if (sdkStartedToday(l) && s.secs) state = TF(S_SUDOKU_UNDER_WAY, sdkTime(s.secs).c_str());
    if (p.sdkBest[l] && !sdkSolvedToday(l)) state += String(state.length() ? "   " : "") + TF(S_SUDOKU_BEST, sdkTime(p.sdkBest[l]).c_str());
    int ly = y + (TALL ? 64 : 56) + 24;
    if (TALL) {
      textCentre(W / 2, ly, &FreeSans9pt7b, fitted(&FreeSans9pt7b, how, bw).c_str());
      if (state.length()) textCentre(W / 2, ly + 22, &FreeSansBold9pt7b, fitted(&FreeSansBold9pt7b, state, bw).c_str());
    } else {
      String line = how + (state.length() ? "   " + state : String());
      textCentre(W / 2, ly, &FreeSans9pt7b, fitted(&FreeSans9pt7b, line, W - 40).c_str());
    }
  }
  int streak = gamesSudokuStreak(today);
  if (streak) textCentre(W / 2, H - (TALL ? 70 : 34), &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, TP(S_G_DAYS_IN_ROW, streak), W - 40).c_str());
  if (TALL) textCentre(W / 2, H - 30, &FreeSans9pt7b, fitted(&FreeSans9pt7b, T(S_DAILY_FOOT), W - 32).c_str());
}

// The Daily screen: a row a puzzle, its picture, name and line. Sideways, three rows of 120
// don't fit under the date: 100 there.
static void drawDailyMenu() {
  gHeader(gameName(GAME_DAILY));
  long today = gToday();
  int y = gTop() + (TALL ? 20 : 10), rh = TALL ? 150 : 100, x = 24, w = W - 48;
  if (today) {
    struct tm d = {};
    d.tm_year = today / 10000 - 1900, d.tm_mon = today / 100 % 100 - 1, d.tm_mday = today % 100, d.tm_hour = 12;
    mktime(&d);
    textCentre(W / 2, y + 16, &FreeSans12pt7b, tDate(S_FMT_DATE_FULL, &d).c_str());
    y += 34;
  }
  struct Row {
    int id;
    const char *name;
    String line;
  } rows[] = {{DY_MATHS, T(S_G_DAILY), dailyMathsLine()}, {DY_SDK, T(S_SUDOKU_NAME), sdkLine()}, {DY_NONOGRAM, T(S_NONO_NAME), nonogramLine()}};
  for (const Row &r : rows) {
    canvas->drawRoundRect(x, y, w, rh - 20, 18, INK);
    canvas->drawRoundRect(x + 1, y + 1, w - 2, rh - 22, 17, INK);
    int icx = x + 20 + 40, icy = y + (rh - 20) / 2;
    if (r.id == DY_MATHS) gameIcon(GAME_MATHS, icx, icy, 64);
    else if (r.id == DY_SDK) sudokuIcon(icx, icy, 64);
    else nonogramIcon(icx, icy, 60);
    int tx = x + 128, tw = w - 128 - 20;
    text(tx, icy - 6, &FreeSansBold18pt7b, fitted(&FreeSansBold18pt7b, r.name, tw).c_str());
    text(tx, icy + 26, &FreeSans12pt7b, fitted(&FreeSans12pt7b, r.line, tw).c_str());
    gTarget(x, y, w, rh - 20, r.id);
    y += rh;
  }
  gCentred(W / 2, H - (TALL ? 30 : 20), &FreeSans9pt7b, T(S_DAILY_FOOT), W - 32, 20, 2);
}

static void drawDaily() {
  dailyEnsure();
  if (dailyS->screen == DY_MENU) return drawDailyMenu();
  if (dailyS->screen == DY_NONO) return drawNonogram();
  if (dailyS->sk == SK_CARD || !gToday()) return drawSudokuCard();
  drawSudokuPlay();
}

// ---- playing

static void sdkPush() {
  DailyState &d = *dailyS;
  const GameProgress::SudokuPlay &s = sdkNow();
  DailyState::Snap &n = d.undo[d.undoTop];
  memcpy(n.value, s.value, 81), memcpy(n.pencil, s.pencil, sizeof n.pencil);
  d.undoTop = (d.undoTop + 1) % SDK_UNDO, d.undoN = min(d.undoN + 1, SDK_UNDO);
}

static void sdkSolved() {
  DailyState &d = *dailyS;
  GameProgress &p = gameProgress();
  GameProgress::SudokuPlay &s = sdkNow();
  sdkTick();
  d.running = false;
  s.done = true;
  int secs = max(1, s.secs);
  d.newBest = !s.hints && (!p.sdkBest[d.level] || secs < p.sdkBest[d.level]);  // with hints: no best time
  if (d.newBest) p.sdkBest[d.level] = secs;
  gamesSudokuSolved(gToday());
  gamesChanged(), gamesDailyChanged();
  d.sel = -1, d.undoN = 0;
  gs->gFocusId = -1;
  Serial.printf("sudoku: %s solved in %d s, %d hints\n", SDK_LEVEL_IDS[d.level], secs, s.hints);
  beep(2400, 120);
  render(true);
}

// A number (1..9) from the pad into the picked square; with Pencil, its mark on or off. The same
// number again takes it out.
static void sdkPut(int digit) {
  DailyState &d = *dailyS;
  GameProgress::SudokuPlay &s = sdkNow();
  const sudokugrid::Puzzle &p = sdkPuzzle(d.level);
  d.say = SAY_NONE, d.hint.cell = -1;
  if (d.sel < 0) return render(false);
  int i = d.sel;
  if (p.given[i]) return d.say = SAY_GIVEN, beep(900, 40), render(false);
  if (d.pencil && s.value[i]) return;  // a mark under a number: nothing to show
  sdkPush();
  if (d.pencil) {
    s.pencil[i] ^= 1u << digit;
  } else if (s.value[i] == digit) {
    s.value[i] = 0, sudokugrid::unmark(s.marked, i);
  } else {
    s.value[i] = digit, s.pencil[i] = 0, sudokugrid::unmark(s.marked, i);
    sudokugrid::clearPeersMark(s.pencil, i, digit);
  }
  sdkTick();
  gamesDailyChanged();
  if (sudokugrid::solved(s.value, p)) return sdkSolved();
  beep(2600, 15);
  render(false);
}

// Hint: the square it ringed, picked still and empty, gets its number; else your wrong numbers are
// struck through; else a square is ringed, with why. A tap that shows nothing new doesn't count.
static void sdkHint() {
  DailyState &d = *dailyS;
  GameProgress::SudokuPlay &s = sdkNow();
  const sudokugrid::Puzzle &p = sdkPuzzle(d.level);
  if (d.say == SAY_HINT && d.hint.cell >= 0 && d.sel == d.hint.cell && !s.value[d.hint.cell]) {
    bool pencil = d.pencil;
    d.pencil = false;
    sdkPut(d.hint.digit);  // undone like any number
    d.pencil = pencil;
    return;
  }
  uint32_t wrong[3];
  int n = sudokugrid::wrongCells(s.value, p, wrong);
  d.hint.cell = -1;
  if (n) {
    bool fresh = (wrong[0] & ~s.marked[0]) || (wrong[1] & ~s.marked[1]) || (wrong[2] & ~s.marked[2]);
    memcpy(s.marked, wrong, sizeof wrong);
    if (fresh) s.hints++;
    d.say = SAY_WRONG, d.wrongN = n;
  } else if (sudokugrid::findHint(s.value, p, d.hint)) {
    s.hints++;
    d.sel = d.hint.cell, d.say = SAY_HINT;
  }
  sdkTick(), gamesDailyChanged();
  beep(2200, 15);
  render(false);
}

static void sdkCommand(int id) {
  DailyState &d = *dailyS;
  GameProgress::SudokuPlay &s = sdkNow();
  if (id >= 0 && id < 81) {
    d.sel = d.sel == id ? -1 : id, d.say = SAY_NONE, d.hint.cell = -1;
    return render(false);
  }
  if (id == SK_HINT) return sdkHint();
  if (id > SK_DIGIT && id <= SK_DIGIT + 9) return sdkPut(id - SK_DIGIT);
  if (id == SK_PENCIL) return d.pencil = !d.pencil, d.say = d.say == SAY_HINT ? SAY_HINT : SAY_NONE, render(false);  // a ringed hint stays
  d.hint.cell = -1;
  if (id == SK_ERASE) {
    d.say = SAY_NONE;
    if (d.sel >= 0 && sdkPuzzle(d.level).given[d.sel]) d.say = SAY_GIVEN;
    else if (d.sel >= 0 && (s.value[d.sel] || s.pencil[d.sel]))
      sdkPush(), s.value[d.sel] = 0, s.pencil[d.sel] = 0, sudokugrid::unmark(s.marked, d.sel), sdkTick(), gamesDailyChanged();
    return render(false);
  }
  if (id == SK_UNDO && d.undoN) {
    d.undoTop = (d.undoTop + SDK_UNDO - 1) % SDK_UNDO, d.undoN--;
    for (int i = 0; i < 81; i++)  // a square that changes back loses its strike
      if (s.value[i] != d.undo[d.undoTop].value[i]) sudokugrid::unmark(s.marked, i);
    memcpy(s.value, d.undo[d.undoTop].value, 81), memcpy(s.pencil, d.undo[d.undoTop].pencil, sizeof s.pencil);
    d.say = SAY_NONE;
    sdkTick(), gamesDailyChanged();
    return render(false);
  }
}

static void actDaily(int id) {
  dailyEnsure();
  DailyState &d = *dailyS;
  gs->gFocusId = -1;
  if (d.screen == DY_MENU) {
    if (id == ID_BACK) return gs->gGame = -1, render(false);
    if (id == DY_MATHS) return gs->gGame = GAME_MATHS, mStartDaily();
    if (id == DY_SDK) return sdkOpen(), render(true);
    if (id == DY_NONOGRAM) return d.screen = DY_NONO, nonogramOpen(), render(true);
    return;
  }
  if (id == ID_BACK) {  // the puzzle -> the Daily screen; the card too
    sdkPause(), nonogramPause();
    d.screen = DY_MENU;
    return render(false);
  }
  if (d.screen == DY_NONO) return actNonogram(id);
  if (d.sk == SK_CARD) {
    if (id >= SK_LEVEL && id < SK_LEVEL + 3 && gToday()) return sdkStart(id - SK_LEVEL), render(true);
    return;
  }
  if (id == SK_LEVELS) return sdkPause(), d.sk = SK_CARD, render(false);
  if (!sdkNow().done) sdkCommand(id);
}

// OK with nothing ringed: the card's first level not solved; in a puzzle Pencil on or off; solved, the card.
static bool dailyOk() {
  dailyEnsure();
  DailyState &d = *dailyS;
  if (d.screen == DY_MENU) return false;
  if (d.screen == DY_NONO) return nonogramOk();
  if (d.sk == SK_CARD) {
    for (int l = 0; l < 3; l++)
      if (!sdkSolvedToday(l)) return actDaily(SK_LEVEL + l), true;
    return false;
  }
  if (sdkNow().done) return actDaily(SK_LEVELS), true;
  actDaily(SK_PENCIL);
  return true;
}

// Leaving the app (home): the clock stops.
static void dailyLeave() { sdkPause(), nonogramPause(); }

// `game right` / `game wrong`: the next square gets its number, or one that's wrong there.
static bool dailyAnswer(bool right) {
  if (dailyS && dailyS->screen == DY_NONO) return nonogramAnswer(right);
  if (!dailyS || dailyS->screen != DY_SUDOKU || dailyS->sk != SK_PLAY || sdkNow().done) return false;
  const sudokugrid::Puzzle &p = sdkPuzzle(dailyS->level);
  int i = sudokugrid::nextCell(sdkNow().value, p);
  if (i < 0) return false;
  dailyS->sel = i, dailyS->pencil = false;
  int want = p.solution[i];
  sdkPut(right ? want : want % 9 + 1);
  return true;
}

static void dailyPeek(String &s) {
  dailyEnsure();
  DailyState &d = *dailyS;
  long today = gToday();
  if (d.screen == DY_MENU) {
    s += "daily maths " + String(gamesDailyDone(today) ? 1 : 0) + " streak " + gamesDailyStreak(today) + ", sudoku";
    for (int l = 0; l < 3; l++) s += String(" ") + SDK_LEVEL_IDS[l] + " " + (sdkSolvedToday(l) ? 1 : 0);
    s += " streak " + String(gamesSudokuStreak(today));
    s += ", nonogram";
    for (int z = 0; z < 2; z++) s += String(z ? " 10x10 " : " 8x8 ") + (nonoSolvedToday(z) ? 1 : 0);
    s += " streak " + String(gamesNonogramStreak(today));
    return;
  }
  if (d.screen == DY_NONO) return nonogramPeek(s);
  const sudokugrid::Puzzle &p = sdkPuzzle(d.level);
  const GameProgress::SudokuPlay &g = gameProgress().sdkPlay[d.level];
  bool current = g.date == today && sudokugrid::fits(g.value, p);
  char grid[82], given[82];
  for (int i = 0; i < 81; i++) {
    int v = current ? g.value[i] : p.given[i];
    grid[i] = v ? '0' + v : '.', given[i] = p.given[i] ? 'g' : '-';
  }
  grid[81] = given[81] = 0;
  int next = sudokugrid::nextCell(current ? g.value : p.given, p);
  int struck = 0;
  for (int i = 0; i < 81; i++) struck += current && sudokugrid::marked(g.marked, i);
  char head[340];
  snprintf(head, sizeof head, "sudoku %s %ld grid %s given %s sel %d next %d=%d clashes %d done %d hints %d struck %d ringed %d", SDK_LEVEL_IDS[d.level],
           today, grid, given, d.sel, next, next >= 0 ? p.solution[next] : 0, current ? sudokugrid::countClashes(g.value) : 0, current && g.done ? 1 : 0,
           current ? g.hints : 0, struck, d.say == SAY_HINT ? d.hint.cell : -1);
  s += head;
  if (d.sk == SK_CARD) s += " card";
}

static bool dailyOnSudoku() { return dailyS && dailyS->screen == DY_SUDOKU; }
static bool dailyOnNonogram() { return dailyS && dailyS->screen == DY_NONO; }

// `daily date <YYYYMMDD>` (test builds only: a dev-* version): the daily puzzles' today, to check
// that boards agree and that streaks roll over; `daily date off` goes back to the clock.
void dailyDateCommand(const String &arg) {  // not static: sticky-os.ino calls it (the generated prototype)
  if (strncmp(updVersion(), "dev-", 4) != 0) return (void)Serial.println("daily: date is for test builds only");
  if (arg == "off") gDateOverride = 0;
  else if (arg.length() == 8 && arg.toInt() > 19700101) gDateOverride = arg.toInt();
  else return (void)Serial.println("daily: date <YYYYMMDD> | off");
  Serial.printf("daily: today is %ld\n", gToday());
  if (app == APP_GAMES && gs->gGame == GAME_DAILY) render(true);
}
