// ---------------------------------------------------------------- Daily Sudoku
//
// Today's Sudoku (issue #18), at Easy, Medium or Hard: made from the date by sudokugrid.h, so
// every board has the same three that day. Opened from the Daily screen (games_daily.ino), which
// keeps the date and /daily.txt; this tab draws the board and takes its taps, like the other
// games (gHeader, gButton, gTarget; games.ino). Its state lives in PSRAM (sudokuS, sudokuEnsure()).
//
// Upright: a 459 px board (51 px squares) with the pad under it, the digits 1-9 one under each
// column, then Erase and Pencil. Sideways: a 414 px board (46 px) with Back, Level and the pad
// beside it, the digits three by three. Thick lines mark the boxes. Givens are bold, your digits
// regular; a digit that clashes with its row, column or box is struck through (never refused).
// Pencil marks are small digits in a 3 x 3 inside the square. Tap a square (it gets a thick
// frame), then a digit: the same digit again takes it out; with Pencil on, digits are marks.
// A digit put in takes that mark out of its row, column and box. With the ring (UP / DOWN), OK
// on a square moves the ring to the pad, OK on a digit back to the square. OK with nothing ringed
// turns Pencil on and off. Solved: a full refresh, the time, the streak, "Come back tomorrow".
// Each level is its own puzzle, all three kept (and saved) as you leave them.
//
// Ids, for taps, the ring and `game pick <id>`:
//   0..80        a square, row * 9 + column
//   101..109     the digits 1-9; 110 Erase; 111 Pencil (on / off)
//   120..122     today's Easy, Medium, Hard (on the Level card)
//   ID_NEW (901) the Level card; ID_CANCEL (905) closes it; ID_BACK (900) the Daily screen
// `game peek`: "sudoku <easy|medium|hard> <yyyymmdd> grid <81: 1-9 or .> given <81: g or ->
// sel <square or -1> next <square>=<digit> clashes <n> done <0|1> pencil <0|1> card <0|1>
// time <s>s made <ms>ms" ("next -1=0" once solved; "sudoku none <date> card 1" before a level is
// chosen), then the targets. `game right` puts the solution's digit in the next square, `game
// wrong` a digit that is wrong there.
// Demo mode: Medium, the demo date's, about half done, with a clash and some pencil marks.
#include "sudokugrid.h"
#include "minesgrid.h"  // minesgrid::Clock: the seconds played, counted between taps

enum { SDK_DIGIT = 100, SDK_ERASE = 110, SDK_PENCIL = 111, SDK_LEVEL = 120 };  // digit d is SDK_DIGIT + d

struct SudokuLevel {
  sudokugrid::Puzzle puz;
  bool made = false;       // puz is today's (made from the date when first needed)
  bool started = false;    // something went in today (or came from /daily.txt)
  bool solved = false;
  uint8_t cells[81] = {};  // the givens and yours, 0 empty
  uint16_t pencil[81] = {};  // marks, bit d - 1 for digit d
  minesgrid::Clock clock;
  uint32_t madeMs = 0;     // how long making it took
};
struct SudokuState {
  SudokuLevel lv[3];
  int level = -1;          // on screen; -1: none chosen today
  int sel = -1;            // the square with the frame
  bool pencilMode = false, choosing = false, newBest = false;
  StrId say = S_SUDOKU_HOW;  // the line under the pad
};
static SudokuState *sudokuS = nullptr;
// No function takes or returns these types (see minesEnsure()).
static void sudokuEnsure() {
  if (sudokuS) return;
  void *p = heap_caps_malloc(sizeof(SudokuState), MALLOC_CAP_SPIRAM);
  sudokuS = p ? new (p) SudokuState() : new SudokuState();
}

static StrId sudokuLevelName(int l) { return l == 0 ? S_SUDOKU_EASY : l == 1 ? S_SUDOKU_MEDIUM : S_SUDOKU_HARD; }

// A new day, or demo mode came or went (dailyEnsure()): every level starts again.
static void sudokuReset() {
  sudokuEnsure();
  for (SudokuLevel &l : sudokuS->lv) l = SudokuLevel();
  sudokuS->level = -1, sudokuS->sel = -1, sudokuS->pencilMode = sudokuS->choosing = sudokuS->newBest = false;
  sudokuS->say = S_SUDOKU_HOW;
}

static uint32_t sudokuSecs(int level) { return minesgrid::clockShown(sudokuS->lv[level].clock, millis()) / 1000; }

// Demo mode: about half the empty squares right, one wrong digit that clashes, some marks.
static void sudokuDemoFill(int level) {
  SudokuLevel &l = sudokuS->lv[level];
  int empty = 0, wrongAt = -1;
  for (int c = 0; c < 81; c++) {
    if (l.puz.grid[c]) continue;
    if (empty++ % 2 == 0) l.cells[c] = l.puz.solution[c];
    else if (wrongAt < 0 && c > 30) wrongAt = c;
    else if (empty % 5 == 0) l.pencil[c] = sudokugrid::digitBit(l.puz.solution[c]) | sudokugrid::digitBit(l.puz.solution[c] % 9 + 1);
  }
  if (wrongAt >= 0)  // a digit its row already has: struck
    for (int i = 0; i < 9; i++)
      if (l.cells[wrongAt / 9 * 9 + i]) { l.cells[wrongAt] = l.cells[wrongAt / 9 * 9 + i], l.pencil[wrongAt] = 0; break; }
  l.started = true;
  l.clock.elapsedMs = 252000;  // 4:12, stopped
}

// Today's puzzle at `level`, made once (it takes up to ~150 ms on the board: see sudokugrid.h).
// Cells read from /daily.txt are checked against its givens; a grid that doesn't match is dropped.
static bool sudokuMake(int level) {
  SudokuLevel &l = sudokuS->lv[level];
  if (l.made) return true;
  long date = dailyDate();
  if (!date) return false;
  uint32_t t0 = millis();
  sudokugrid::Stats st;
  if (!sudokugrid::daily(date, level, l.puz, &st)) return false;
  l.made = true, l.madeMs = millis() - t0;
  Serial.printf("sudoku: made %d for %ld in %lu ms: %d clues, %d attempts%s, %lu nodes\n", level, date, (unsigned long)l.madeMs, l.puz.clues,
                st.attempts, st.fallback ? " (fallback)" : "", (unsigned long)st.nodes);
  bool fits = true;
  for (int c = 0; c < 81; c++)
    if (l.puz.grid[c] && l.cells[c] != l.puz.grid[c]) fits = false;
  if (l.started && !fits) Serial.println("sudoku: the saved grid isn't today's puzzle: dropped");
  if (!l.started || !fits) {
    for (int c = 0; c < 81; c++) l.cells[c] = l.puz.grid[c], l.pencil[c] = 0;
    l.started = l.solved = false, l.clock = minesgrid::Clock();
    if (demoMode) sudokuDemoFill(level);
  }
  l.solved = sudokugrid::solved(l.cells);
  return true;
}

// Level `level` on screen (from the card).
static void sudokuPick(int level) {
  SudokuState &s = *sudokuS;
  s.level = constrain(level, 0, sudokugrid::kLevels - 1);
  s.choosing = false, s.sel = -1, s.newBest = false;
  s.say = s.pencilMode ? S_SUDOKU_HOW_PENCIL : S_SUDOKU_HOW;
  sudokuMake(s.level);
  dailyChanged();
}

static void sudokuOpen() {
  sudokuEnsure();
  dailyDate();  // today's (a new day resets)
  SudokuState &s = *sudokuS;
  if (s.level < 0) {  // the level under way today, else the card
    for (int l = 0; l < 3 && s.level < 0; l++)
      if (s.lv[l].started && !s.lv[l].solved) s.level = l;
    if (demoMode && s.level < 0) s.level = 1;
  }
  if (s.level >= 0) s.choosing = false, sudokuMake(s.level);
  else s.choosing = true;
}

static void sudokuForget() { sudokuReset(); }

// ---- /daily.txt (games_daily.ino reads and writes the file; these are the Sudoku lines)

static void sudokuSaveTo(File &f) {
  sudokuEnsure();
  f.printf("sudoku %d\n", sudokuS->level);
  for (int l = 0; l < 3; l++) {
    const SudokuLevel &v = sudokuS->lv[l];
    if (!v.started) continue;
    char cells[82], pencil[244];
    for (int c = 0; c < 81; c++) {
      cells[c] = (char)('0' + v.cells[c]);
      snprintf(pencil + 3 * c, 4, "%03x", v.pencil[c] & sudokugrid::kAll);
    }
    cells[81] = 0;
    f.printf("sudoku%d %s %s %lu %d\n", l, cells, pencil, (unsigned long)sudokuSecs(l), v.solved ? 1 : 0);
  }
}

static void sudokuLoadLine(const String &k, const String &v) {
  sudokuEnsure();
  if (k == "sudoku") {
    sudokuS->level = constrain((int)v.toInt(), -1, 2);
    return;
  }
  int l = k.length() == 7 ? k[6] - '0' : -1;
  if (l < 0 || l > 2) return;
  int a = v.indexOf(' '), b = a < 0 ? -1 : v.indexOf(' ', a + 1), c = b < 0 ? -1 : v.indexOf(' ', b + 1);
  if (a != 81 || b != 81 + 1 + 243 || c < 0) return;  // damaged: left out
  SudokuLevel &x = sudokuS->lv[l];
  for (int i = 0; i < 81; i++) {
    char ch = v[i];
    x.cells[i] = ch >= '1' && ch <= '9' ? ch - '0' : 0;
    x.pencil[i] = (uint16_t)(strtoul(v.substring(a + 1 + 3 * i, a + 4 + 3 * i).c_str(), nullptr, 16) & sudokugrid::kAll);
  }
  x.clock = minesgrid::Clock();
  x.clock.elapsedMs = (uint32_t)v.substring(b + 1, c).toInt() * 1000;
  x.solved = v.substring(c + 1).toInt() == 1;
  x.started = true, x.made = false;  // the puzzle is made again from the date, and checked
}

// ---- geometry

static void sudokuGeometry(int &x0, int &y0, int &c) {
  if (TALL) {
    c = min(51, (W - 16) / 9);
    x0 = (W - 9 * c) / 2, y0 = gTop() + 4;
  } else {
    c = min(46, (BODY_H - 16) / 9);
    x0 = 16, y0 = BODY_Y + (BODY_H - 9 * c) / 2;
  }
}

// ---- pictures

static void sudokuIcon(int cx, int cy, int s, int t) {  // a 3 x 3 box with three digits in it
  int r = s / 2, c = s / 3;
  for (int k = 0; k < t; k++) canvas->drawRect(cx - r + k, cy - r + k, s - 2 * k, s - 2 * k, INK);
  for (int i = 1; i < 3; i++) canvas->drawFastVLine(cx - r + i * c, cy - r, s, INK), canvas->drawFastHLine(cx - r, cy - r + i * c, s, INK);
  const GFXfont *f = s >= 54 ? &FreeSansBold12pt7b : &FreeSansBold9pt7b;
  int up = s >= 54 ? 8 : 6;
  static const int AT[3][2] = {{0, 0}, {2, 1}, {1, 2}};
  static const char *const D[3] = {"5", "3", "9"};
  for (int i = 0; i < 3; i++) textCentre(cx - r + AT[i][0] * c + c / 2 + 1, cy - r + AT[i][1] * c + c / 2 + up, f, D[i]);
}

static void sudokuDrawCell(int cell, int x, int y, int c) {
  const SudokuState &s = *sudokuS;
  const SudokuLevel &l = s.lv[s.level];
  if (cell == s.sel)
    for (int k = 3; k < 6; k++) canvas->drawRect(x + k, y + k, c + 1 - 2 * k, c + 1 - 2 * k, INK);
  int v = l.cells[cell];
  if (v) {
    bool given = l.puz.grid[cell] != 0;
    char d[2] = {(char)('0' + v), 0};
    textCentre(x + c / 2 + 1, y + c / 2 + 13, given ? &FreeSansBold18pt7b : &FreeSans18pt7b, d);
    if (sudokugrid::clashes(l.cells, cell)) thickLine(x + 9, y + c - 9, x + c - 9, y + 9, 3);  // struck
    return;
  }
  if (!l.pencil[cell]) return;
  int sub = c / 3, ox = x + (c - 3 * sub) / 2, oy = y + (c - 3 * sub) / 2;
  for (int d = 1; d <= 9; d++) {
    if (!(l.pencil[cell] & sudokugrid::digitBit(d))) continue;
    char m[2] = {(char)('0' + d), 0};
    textCentre(ox + (d - 1) % 3 * sub + sub / 2 + 1, oy + (d - 1) / 3 * sub + sub / 2 + 6, &FreeSans9pt7b, m);
  }
}

static void sudokuBoard(bool targets) {
  int x0, y0, c;
  sudokuGeometry(x0, y0, c);
  for (int cell = 0; cell < 81; cell++) {
    int x = x0 + cell % 9 * c, y = y0 + cell / 9 * c;
    sudokuDrawCell(cell, x, y, c);
    if (targets) gTarget(x + 2, y + 2, c - 3, c - 3, cell);
  }
  for (int i = 0; i <= 9; i++) {  // the lines: thick round the boxes
    int t = i % 3 ? 1 : 3;
    canvas->fillRect(x0 + i * c - t / 2, y0 - 1, t, 9 * c + 3, INK);
    canvas->fillRect(x0 - 1, y0 + i * c - t / 2, 9 * c + 3, t, INK);
  }
  canvas->drawRect(x0 - 3, y0 - 3, 9 * c + 7, 9 * c + 7, INK);
}

// A digit's key on the pad.
static void sudokuKey(int x, int y, int w, int h, int d) {
  canvas->drawRoundRect(x, y, w, h, 10, INK);
  canvas->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 9, INK);
  char n[2] = {(char)('0' + d), 0};
  textCentre(x + w / 2 + 1, y + h / 2 + 13, &FreeSansBold18pt7b, n);
  gTarget(x, y, w, h, SDK_DIGIT + d);
}

// The level and the time, then what to do (or why not), at x..x+w from baseline y.
static void sudokuStatus(int x, int y, int w) {
  const SudokuState &s = *sudokuS;
  text(x, y, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, T(sudokuLevelName(s.level)), w - 90).c_str());
  textRight(x + w, y, &FreeSansBold12pt7b, dailyTime(sudokuSecs(s.level)).c_str());
  gLeft(x, y + 26, &FreeSans9pt7b, T(s.say), w, 20, 2);
}

static void sudokuCardDraw() {
  GameProgress &p = gameProgress();
  String names[3], lines[3];
  for (int l = 0; l < 3; l++) {
    const SudokuLevel &v = sudokuS->lv[l];
    names[l] = T(sudokuLevelName(l));
    lines[l] = TF(S_SUDOKU_CLUES, sudokugrid::kMinClues[l], sudokugrid::kMaxClues[l]);
    if (v.solved) lines[l] += "   " + TF(S_DAILY_SOLVED_IN, dailyTime(sudokuSecs(l)).c_str());
    else if (p.sdkBest[l]) lines[l] += "   " + TF(S_DAILY_BEST, dailyTime(p.sdkBest[l]).c_str());
  }
  dailyCard(T(S_SUDOKU_CARD), 3, SDK_LEVEL, sudokuS->level, names, lines);
}

static void drawSudoku() {
  sudokuEnsure();
  if (!dailyDate()) return dailyS->screen = DS_NO_DATE, drawDailyNoDate();
  SudokuState &s = *sudokuS;
  if (s.level < 0 && !s.choosing) sudokuOpen();  // reset (a new day, demo mode): as if opened
  if (s.level >= 0 && !sudokuMake(s.level)) s.level = -1, s.choosing = true;
  bool board = s.level >= 0, targets = !s.choosing;
  const SudokuLevel *l = board ? &s.lv[s.level] : nullptr;
  bool done = l && l->solved;
  int x0, y0, c;
  sudokuGeometry(x0, y0, c);
  GameProgress &p = gameProgress();
  int streak = gamesStreakAlive(p.sdkDay, p.sdkStreak, gToday());
  if (TALL) {
    gHeader(T(S_G_SUDOKU), "", 130);  // Back closes the card when it's up
    if (targets) gButton(W - 16 - 124, BODY_Y + 10, 124, 48, T(S_SUDOKU_LEVEL), false, ID_NEW, &FreeSansBold9pt7b);
    if (board) sudokuBoard(targets && !done);
    int py = y0 + 9 * c + 12, side = 9 * c;
    if (board && done) {
      dailySolvedPanel(x0, py - 4, side, H - py - 4, sudokuSecs(s.level), s.newBest, streak, T(S_SUDOKU_LEVEL));
    } else if (board && targets) {
      for (int d = 1; d <= 9; d++) sudokuKey(x0 + (d - 1) * c + 2, py, c - 4, 58, d);
      int by = py + 70, bw = (side - 12) / 2;
      gButton(x0, by, bw, 52, T(S_SUDOKU_ERASE), false, SDK_ERASE);
      gButton(x0 + bw + 12, by, bw, 52, T(S_SUDOKU_PENCIL), s.pencilMode, SDK_PENCIL);
      sudokuStatus(x0 + 4, by + 84, side - 8);
    }
  } else {  // beside the board: Back and Level, the pad three by three, Erase and Pencil, the status
    if (board) sudokuBoard(targets && !done);
    int px = x0 + 9 * c + 24, pw = W - 16 - px;
    if (!board) gHeader(T(S_G_SUDOKU));
    else if (targets) gButton(px, BODY_Y + 10, 116, 48, T(S_G_BACK), false, ID_BACK);
    if (targets) gButton(W - 16 - 124, BODY_Y + 10, 124, 48, T(S_SUDOKU_LEVEL), false, ID_NEW, &FreeSansBold9pt7b);
    int py = BODY_Y + 72;
    if (board && done) {
      dailySolvedPanel(px, py, pw, H - 12 - py, sudokuSecs(s.level), s.newBest, streak, T(S_SUDOKU_LEVEL));
    } else if (board && targets) {
      int kw = (pw - 20) / 3, kh = 58;
      for (int d = 1; d <= 9; d++) sudokuKey(px + (d - 1) % 3 * (kw + 10), py + (d - 1) / 3 * (kh + 10), kw, kh, d);
      int by = py + 3 * (kh + 10) + 2, bw = (pw - 10) / 2;
      gButton(px, by, bw, 50, T(S_SUDOKU_ERASE), false, SDK_ERASE);
      gButton(px + bw + 10, by, bw, 50, T(S_SUDOKU_PENCIL), s.pencilMode, SDK_PENCIL);
      sudokuStatus(px, by + 82, pw);
    }
  }
  if (s.choosing) sudokuCardDraw();
}

// ---- playing

// The clock runs from the first thing put in, and between taps (minesgrid::Clock); demo mode's stays.
static void sudokuTick(int level) {
  SudokuLevel &l = sudokuS->lv[level];
  if (demoMode) return;
  uint32_t now = millis();
  if (l.clock.running) minesgrid::clockTick(l.clock, now);
  else l.clock.running = true, l.clock.lastMs = now;
}

static void sudokuSolvedNow() {
  SudokuState &s = *sudokuS;
  SudokuLevel &l = s.lv[s.level];
  if (!demoMode) minesgrid::clockStop(l.clock, millis());
  l.solved = true;
  GameProgress &p = gameProgress();
  uint32_t secs = sudokuSecs(s.level);
  s.newBest = demoMode ? false : dailySolvedNow(p.sdkDay, p.sdkStreak, p.sdkBest[s.level], secs);
  s.sel = -1, gs->gFocusId = -1;
  dailyChanged();
  Serial.printf("sudoku: level %d solved in %lu s, streak %d\n", s.level, (unsigned long)secs, p.sdkStreak);
}

// After a change: solved, or the line under the pad.
static void sudokuAfter() {
  SudokuState &s = *sudokuS;
  SudokuLevel &l = s.lv[s.level];
  l.started = true;
  sudokuTick(s.level);
  dailyChanged();
  if (sudokugrid::solved(l.cells)) return sudokuSolvedNow(), render(true);
  s.say = sudokugrid::clashCount(l.cells) ? S_SUDOKU_CLASHES : s.pencilMode ? S_SUDOKU_HOW_PENCIL : S_SUDOKU_HOW;
  beep(2600, 15);
  render(false);
}

static void sudokuPut(int d) {
  SudokuState &s = *sudokuS;
  SudokuLevel &l = s.lv[s.level];
  if (s.sel < 0) return s.say = S_SUDOKU_HOW, render(false);
  if (l.puz.grid[s.sel]) return s.say = S_SUDOKU_GIVEN, render(false);
  uint16_t b = sudokugrid::digitBit(d);
  if (s.pencilMode) {
    if (l.cells[s.sel]) return s.say = S_SUDOKU_HOW_PENCIL, render(false);  // a mark goes in an empty square only
    l.pencil[s.sel] ^= b;
  } else if (l.cells[s.sel] == d) {
    l.cells[s.sel] = 0;
  } else {
    l.cells[s.sel] = (uint8_t)d, l.pencil[s.sel] = 0;
    for (int c = 0; c < 81; c++)
      if (sudokugrid::peers(c, s.sel)) l.pencil[c] &= (uint16_t)~b;
    if (gs->gFocusId == SDK_DIGIT + d) gs->gFocusId = s.sel;  // the ring goes back to the square
  }
  sudokuAfter();
}

static void sudokuErase() {
  SudokuState &s = *sudokuS;
  SudokuLevel &l = s.lv[s.level];
  if (s.sel < 0) return s.say = S_SUDOKU_HOW, render(false);
  if (l.puz.grid[s.sel]) return s.say = S_SUDOKU_GIVEN, render(false);
  if (!l.cells[s.sel] && !l.pencil[s.sel]) return;
  if (l.cells[s.sel]) l.cells[s.sel] = 0;
  else l.pencil[s.sel] = 0;
  sudokuAfter();
}

static void actSudoku(int id) {
  sudokuEnsure();
  dailyDate();  // first: a new day resets the grids (and the level on screen)
  SudokuState &s = *sudokuS;
  if (id == ID_BACK) {
    if (s.choosing && s.level >= 0) return s.choosing = false, gs->gFocusId = -1, render(false);
    return dailyBack();
  }
  if (s.level < 0 && !s.choosing) return sudokuOpen(), gs->gFocusId = -1, render(true);  // reset under the finger: today's, afresh
  if (id == ID_NEW) return s.choosing = true, gs->gFocusId = -1, render(false);
  if (id == ID_CANCEL) {
    if (!s.choosing) return;
    if (s.level < 0) return dailyBack();
    return s.choosing = false, gs->gFocusId = -1, render(false);
  }
  if (id >= SDK_LEVEL && id < SDK_LEVEL + 3) return sudokuPick(id - SDK_LEVEL), gs->gFocusId = -1, render(true);
  if (s.choosing || s.level < 0 || !sudokuMake(s.level) || s.lv[s.level].solved) return;
  if (id >= 0 && id < 81) {
    bool ring = gs->gFocusId == id;
    s.sel = id;
    if (ring) gs->gFocusId = SDK_DIGIT + max(1, (int)s.lv[s.level].cells[id]);  // OK on a square: the ring to the pad
    return render(false);
  }
  if (id > SDK_DIGIT && id <= SDK_DIGIT + 9) return sudokuPut(id - SDK_DIGIT);
  if (id == SDK_ERASE) return sudokuErase();
  if (id == SDK_PENCIL) {
    s.pencilMode = !s.pencilMode;
    s.say = s.pencilMode ? S_SUDOKU_HOW_PENCIL : S_SUDOKU_HOW;
    return render(false);
  }
}

// OK with nothing ringed: on the card, the level; while playing, Pencil on / off.
static bool sudokuOk() {
  sudokuEnsure();
  dailyDate();  // first: a new day resets the grids (and the level on screen)
  SudokuState &s = *sudokuS;
  if (s.choosing) return actSudoku(SDK_LEVEL + max(0, s.level)), true;
  if (s.level < 0 || s.lv[s.level].solved) return false;  // solved: the screen's main button (Level)
  actSudoku(SDK_PENCIL);
  return true;
}

// The Daily screen's line: done today, or the level under way, and the streak.
static String sudokuLine() {
  sudokuEnsure();
  dailyDate();
  GameProgress &p = gameProgress();
  long today = gToday();
  const SudokuState &s = *sudokuS;
  int under = s.level >= 0 && s.lv[s.level].started && !s.lv[s.level].solved ? s.level : -1;  // the one on screen first
  for (int l = 0; l < 3 && under < 0; l++)
    if (s.lv[l].started && !s.lv[l].solved) under = l;
  String state = under >= 0 ? TF(S_DAILY_UNDER_WAY, T(sudokuLevelName(under))) : String(T(S_SUDOKU_LEVELS));
  return dailyRowLine(today && p.sdkDay == today, state, gamesStreakAlive(p.sdkDay, p.sdkStreak, today));
}

// `game right` / `game wrong`: sudokugrid::next()'s square gets the solution's digit, or one that's wrong there.
static void sudokuAnswer(bool right) {
  sudokuEnsure();
  dailyDate();  // first: a new day resets the grids (and the level on screen)
  SudokuState &s = *sudokuS;
  if (s.choosing || s.level < 0 || !sudokuMake(s.level)) return;
  SudokuLevel &l = s.lv[s.level];
  if (l.solved) return;
  uint8_t digit = 0;
  int c = sudokugrid::next(l.puz, l.cells, &digit);
  if (c < 0) return;
  if (!right)
    for (int d = 1; d <= 9; d++)
      if (d != l.puz.solution[c] && d != l.cells[c]) { digit = (uint8_t)d; break; }
  bool pencil = s.pencilMode;
  s.sel = c, s.pencilMode = false;
  sudokuPut(digit);
  s.pencilMode = pencil;
}

static void sudokuPeek(String &s) {
  sudokuEnsure();
  dailyDate();  // first: a new day resets the grids (and the level on screen)
  static const char *const LEVELS[] = {"easy", "medium", "hard"};
  SudokuState &st = *sudokuS;
  long date = dailyDate();
  if (st.level < 0 || !sudokuMake(st.level)) {
    s += "sudoku none " + String(date) + " card " + (st.choosing ? "1" : "0");
    return;
  }
  const SudokuLevel &l = st.lv[st.level];
  char grid[82], given[82];
  for (int c = 0; c < 81; c++) grid[c] = l.cells[c] ? (char)('0' + l.cells[c]) : '.', given[c] = l.puz.grid[c] ? 'g' : '-';
  grid[81] = given[81] = 0;
  uint8_t digit = 0;
  int next = sudokugrid::next(l.puz, l.cells, &digit);
  bool done = sudokugrid::solved(l.cells);
  if (done) next = -1, digit = 0;
  char tail[160];
  snprintf(tail, sizeof tail, " sel %d next %d=%d clashes %d done %d pencil %d card %d time %lus made %lums", st.sel, next, digit,
           sudokugrid::clashCount(l.cells), done, st.pencilMode, st.choosing, (unsigned long)sudokuSecs(st.level), (unsigned long)l.madeMs);
  s += String("sudoku ") + LEVELS[st.level] + " " + date + " grid " + grid + " given " + given + tail;
}
