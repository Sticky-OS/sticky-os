// ---------------------------------------------------------------- Daily, and its Sudoku
//
// The Games folder's Daily tile (GAME_DAILY, issue #18) opens the Daily screen: a row for Daily
// maths (games.ino's maths round, unchanged; its Back comes back here) and a row for Sudoku. Like
// the other games (gHeader, gButton, gTarget; games.ino), this tab draws them and takes the taps.
// Its state lives in PSRAM (dailyS, dailyEnsure()).
//
// Sudoku: the puzzle of the day at three levels, made on the board from the date (sudokugrid.h:
// the same on every board), once a day: kept in PSRAM, so reopening never makes it again. Tap a
// square (ringed; its row, column and box hatched lightly), then a number on the pad; Pencil puts
// numbers in small, as notes; Erase clears a square, Undo takes back a step. Given numbers are
// bold and can't change; a number of yours that clashes with another in its row, column or box
// is struck through (never refused). Placing a number takes its pencil marks off the row, column
// and box. Solved: a full refresh, the time, the streak, "Come back tomorrow".
//
// Today's grids (numbers, pencil marks, time) are in GameProgress::sdkPlay, written to
// /daily.txt from the loop (gamesDailyChanged()), so a day's work survives deep sleep; a grid for
// another date is dropped when its level is opened. The clock counts the time the puzzle is on
// screen, a gap between taps counting at most SDK_GAP_MS (a board left alone doesn't run it up).
//
// Ids, for taps, the ring and `game pick <id>`:
//   10 Daily maths, 11 Sudoku (the Daily screen)
//   0..80 a square (row * 9 + column); 101..109 the numbers; 110 Erase; 111 Pencil; 112 Undo
//   120 / 121 / 122 a level (the levels card); 130 Levels (back to the card); ID_BACK (900)
// `game peek`: "sudoku <level> <date> grid <81: digit or .> given <81: g or -> sel <cell> next
// <cell>=<digit> clashes <n> done <0|1>", then " card" when the levels card is up. On the Daily
// screen: "daily maths <done 0|1> streak <n>, sudoku <easy medium hard done 0|1> streak <n>".
// `game right` / `game wrong`: the next square (sudokugrid::nextCell) gets its number, or a wrong one.
// Demo mode (`demo on`): Medium, half done, with pencil marks and a clash, at 12:34.
#include "sudokugrid.h"

enum { DY_MENU, DY_SUDOKU };       // the Daily screen, or Sudoku
enum { SK_CARD, SK_PLAY };         // Sudoku: the levels card, or a puzzle
enum { DY_MATHS = 10, DY_SDK = 11, SK_DIGIT = 100, SK_ERASE = 110, SK_PENCIL = 111, SK_UNDO = 112, SK_LEVEL = 120, SK_LEVELS = 130 };
enum { SAY_NONE, SAY_GIVEN };
static const uint32_t SDK_GAP_MS = 10 * 60 * 1000UL;
static const int SDK_UNDO = 32;

struct DailyState {
  int screen = DY_MENU, sk = SK_CARD;
  int level = 0, sel = -1, say = SAY_NONE;
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
    Serial.printf("sudoku: %s for %ld made in %lu ms: %d clues, needs %d, %d tries\n", SDK_LEVEL_IDS[level], day,
                  (unsigned long)(millis() - t0), p.clues, (int)p.needs, p.tries);
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

// The demo's grid: Medium, about half the empty squares filled, a few pencil marks, a clash.
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
  if (clash >= 0) {  // a number that's already in its row
    for (int j = clash - clash % 9; j < clash - clash % 9 + 9; j++)
      if (p.given[j] && p.given[j] != p.solution[clash]) s.value[clash] = p.given[j];
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
static void sdkResume() {
  if (demoMode || sdkNow().done) return;
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
  int done = gamesDailyDone(today) + gamesSudokuDone(today);
  if (done) return TF(S_DAILY_L_DONE, done, 2);
  int streak = max(gamesDailyStreak(today), gamesSudokuStreak(today));
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
}

static void sdkStart(int level) {
  DailyState &d = *dailyS;
  sdkPause();
  d.level = constrain(level, 0, 2);
  bool demoGrid = demoMode && !d.demoMade && d.level == 1;
  if (demoGrid) sdkDemoGrid();
  sdkGrid(d.level);
  d.sk = SK_PLAY, d.say = SAY_NONE, d.newBest = false, d.undoN = 0, d.pencil = false;
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
  if (gToday() && (demoMode || (sdkStartedToday(l) && !sdkSolvedToday(l)))) sdkStart(l);
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
// it's the picked one; its number (bold given, regular yours, struck through when it clashes) or
// its pencil marks.
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
    if (!given && sudokugrid::clashes(s.value, i)) thickLine(x + c / 5, y + c * 4 / 5, x + c * 4 / 5, y + c / 5, 3);
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

static void sdkTools(int x, int y, int w, int h, int gap) {
  int bw = (w - 2 * gap) / 3;
  const GFXfont *f = &FreeSansBold12pt7b;
  for (StrId id : {S_SUDOKU_ERASE, S_SUDOKU_PENCIL, S_SUDOKU_UNDO})
    if (textW(f, T(id)) > bw - 20) f = &FreeSansBold9pt7b;
  gButton(x, y, bw, h, T(S_SUDOKU_ERASE), false, SK_ERASE, f);
  gButton(x + bw + gap, y, bw, h, T(S_SUDOKU_PENCIL), dailyS->pencil, SK_PENCIL, f);
  if (dailyS->undoN) gButton(x + 2 * (bw + gap), y, bw, h, T(S_SUDOKU_UNDO), false, SK_UNDO, f);
  else pillButton(x + 2 * (bw + gap), y, bw, h, fitted(f, T(S_SUDOKU_UNDO), bw - 24).c_str(), false, f);  // nothing to undo: no target
}

static String sdkSay() {
  DailyState &d = *dailyS;
  const GameProgress::SudokuPlay &s = sdkNow();
  if (d.say == SAY_GIVEN) return T(S_SUDOKU_GIVEN);
  bool clash = false;
  int empty = 0;
  for (int i = 0; i < 81; i++) {
    empty += !s.value[i];
    if (!clash && !sdkPuzzle(d.level).given[i] && sudokugrid::clashes(s.value, i)) clash = true;
  }
  if (clash) return T(S_SUDOKU_CLASH);
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
    sdkTools(x0, py + 82, side, 54, 10);
    gCentred(W / 2, py + 82 + 54 + 26, &FreeSans9pt7b, sdkSay(), W - 24, 20, 2);
  } else {  // the board on the left; beside it Back and Levels, the level and the time, the pad, what's happening
    sdkDrawBoard();
    int px = x0 + side + 22, pw = W - 14 - px;
    gButton(px, BODY_Y + 8, 110, 48, T(S_G_BACK), false, ID_BACK);
    gButton(W - 14 - 124, BODY_Y + 8, 124, 48, T(S_SUDOKU_LEVELS), false, SK_LEVELS, &FreeSansBold9pt7b);
    textCentre(px + pw / 2, BODY_Y + 88, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, info, pw).c_str());
    if (s.done) return sdkSolvedPanel(px + pw / 2, BODY_Y + 104, pw);
    int gap = 8, kw = (pw - 2 * gap) / 3, kh = 62, ky = BODY_Y + 102;
    for (int k = 1; k <= 9; k++) sdkDigitKey(px + (k - 1) % 3 * (kw + gap), ky + (k - 1) / 3 * (kh + gap), kw, kh, k);
    int ty = ky + 3 * (kh + gap) + 4;
    sdkTools(px, ty, pw, 52, gap);
    gLeft(px, ty + 52 + 26, &FreeSans9pt7b, sdkSay(), pw, 20, 3);
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

// The Daily screen: a row a puzzle, its picture, name and line.
static void drawDailyMenu() {
  gHeader(gameName(GAME_DAILY));
  long today = gToday();
  int y = gTop() + (TALL ? 20 : 10), rh = TALL ? 150 : 120, x = 24, w = W - 48;
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
  } rows[] = {{DY_MATHS, T(S_G_DAILY), dailyMathsLine()}, {DY_SDK, T(S_SUDOKU_NAME), sdkLine()}};
  for (const Row &r : rows) {
    canvas->drawRoundRect(x, y, w, rh - 20, 18, INK);
    canvas->drawRoundRect(x + 1, y + 1, w - 2, rh - 22, 17, INK);
    int icx = x + 20 + 40, icy = y + (rh - 20) / 2;
    if (r.id == DY_MATHS) gameIcon(GAME_MATHS, icx, icy, 64);
    else sudokuIcon(icx, icy, 64);
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
  d.newBest = !p.sdkBest[d.level] || secs < p.sdkBest[d.level];
  if (d.newBest) p.sdkBest[d.level] = secs;
  gamesSudokuSolved(gToday());
  gamesChanged(), gamesDailyChanged();
  d.sel = -1, d.undoN = 0;
  gs->gFocusId = -1;
  Serial.printf("sudoku: %s solved in %d s\n", SDK_LEVEL_IDS[d.level], secs);
  beep(2400, 120);
  render(true);
}

// A number (1..9) from the pad into the picked square; with Pencil, its mark on or off. The same
// number again takes it out.
static void sdkPut(int digit) {
  DailyState &d = *dailyS;
  GameProgress::SudokuPlay &s = sdkNow();
  const sudokugrid::Puzzle &p = sdkPuzzle(d.level);
  d.say = SAY_NONE;
  if (d.sel < 0) return render(false);
  int i = d.sel;
  if (p.given[i]) return d.say = SAY_GIVEN, beep(900, 40), render(false);
  if (d.pencil && s.value[i]) return;  // a mark under a number: nothing to show
  sdkPush();
  if (d.pencil) {
    s.pencil[i] ^= 1u << digit;
  } else if (s.value[i] == digit) {
    s.value[i] = 0;
  } else {
    s.value[i] = digit, s.pencil[i] = 0;
    sudokugrid::clearPeersMark(s.pencil, i, digit);
  }
  sdkTick();
  gamesDailyChanged();
  if (sudokugrid::solved(s.value, p)) return sdkSolved();
  beep(2600, 15);
  render(false);
}

static void sdkCommand(int id) {
  DailyState &d = *dailyS;
  GameProgress::SudokuPlay &s = sdkNow();
  if (id >= 0 && id < 81) {
    d.sel = d.sel == id ? -1 : id, d.say = SAY_NONE;
    return render(false);
  }
  if (id > SK_DIGIT && id <= SK_DIGIT + 9) return sdkPut(id - SK_DIGIT);
  if (id == SK_PENCIL) return d.pencil = !d.pencil, d.say = SAY_NONE, render(false);
  if (id == SK_ERASE) {
    d.say = SAY_NONE;
    if (d.sel >= 0 && sdkPuzzle(d.level).given[d.sel]) d.say = SAY_GIVEN;
    else if (d.sel >= 0 && (s.value[d.sel] || s.pencil[d.sel])) sdkPush(), s.value[d.sel] = 0, s.pencil[d.sel] = 0, sdkTick(), gamesDailyChanged();
    return render(false);
  }
  if (id == SK_UNDO && d.undoN) {
    d.undoTop = (d.undoTop + SDK_UNDO - 1) % SDK_UNDO, d.undoN--;
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
    return;
  }
  if (id == ID_BACK) {  // the puzzle -> the Daily screen; the card too
    sdkPause();
    d.screen = DY_MENU;
    return render(false);
  }
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
static void dailyLeave() { sdkPause(); }

// `game right` / `game wrong`: the next square gets its number, or one that's wrong there.
static bool dailyAnswer(bool right) {
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
    return;
  }
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
  char head[300];
  snprintf(head, sizeof head, "sudoku %s %ld grid %s given %s sel %d next %d=%d clashes %d done %d", SDK_LEVEL_IDS[d.level], today, grid,
           given, d.sel, next, next >= 0 ? p.solution[next] : 0, current ? sudokugrid::countClashes(g.value) : 0, current && g.done ? 1 : 0);
  s += head;
  if (d.sk == SK_CARD) s += " card";
}

static bool dailyOnSudoku() { return dailyS && dailyS->screen == DY_SUDOKU; }

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
