// ---------------------------------------------------------------- Daily
//
// The Daily tile (GAME_DAILY, issue #18): a small screen of three puzzles a day, each with its
// own streak. Daily maths (games.ino's Maths Quest in its MM_DAILY mode, started exactly as it
// always was), the Daily Sudoku (games_sudoku.ino) and the daily nonogram (games_nonogram.ino).
// The puzzles come from the date (gToday()), so every board shows the same ones that day.
//
// This tab keeps what the two puzzles share: the date they are for, the file today's grids are
// kept in (/daily.txt), the "solved" panel, and the serial port's `daily` command.
//
// /daily.txt, written from the loop (dailySave()) when something changed, never in demo mode,
// so a puzzle worked on all day survives deep sleep (a restart). One value a line:
//   date 20261122                  the day the grids are for: another day discards them
//   sudoku <level>                 the level on screen (-1: none chosen yet)
//   sudoku<level> <81 cells> <243 pencil> <secs> <solved>   per level started: the cells, '0' empty,
//                                  the pencil marks as 3 hex digits a cell (bit d-1 for digit d),
//                                  the seconds played, 1 once solved
//   nonogram <size>                the size on screen (-1: none)
//   nonogram<size> <cells> <secs> <solved>   per size started: '.' unknown, '#' filled, 'x' crossed
// The puzzles themselves aren't kept: they are made again from the date (and the givens
// checked against the cells: a grid that doesn't match today's puzzle is dropped).
//
// Ids, for taps, the ring and `game pick <id>`: 10 Daily maths, 11 Sudoku, 12 Nonogram;
// ID_BACK (900) the launcher. The puzzles' own ids are at the top of their tabs.
// `game peek` on this screen: "daily date <yyyymmdd or 0> maths <done 0|1> <streak> sudoku <done>
// <streak> nonogram <done> <streak>", then the targets.
//
// Demo mode (`demo on`): the puzzles are DAILY_DEMO_DATE's, partly done, and nothing is saved;
// when it ends, today's grids are read back from /daily.txt.

enum { DAILY_MATHS = 10, DAILY_SUDOKU = 11, DAILY_NONOGRAM = 12 };
enum { DS_MENU, DS_SUDOKU, DS_NONOGRAM, DS_NO_DATE };
static const long DAILY_DEMO_DATE = 20261124;  // demo mode's puzzles: the same pictures every time
static const char *DAILY_FILE = "/daily.txt";

struct DailyState {
  int screen = DS_MENU;
  long date = 0;       // the day the puzzles in memory are for (0: none yet)
  bool loaded = false; // /daily.txt read (once, on first use)
  bool demo = false;   // the puzzles in memory are demo mode's
  bool dirty = false;  // something to write to /daily.txt
};
static DailyState *dailyS = nullptr;

static void dailyLoad();
static void dailySaveNow();

// Made on first use, and kept to today: a new date (or demo mode starting or ending) drops the
// grids in memory. No function takes or returns DailyState (see minesEnsure()).
static void dailyEnsure() {
  if (!dailyS) {
    void *p = heap_caps_malloc(sizeof(DailyState), MALLOC_CAP_SPIRAM);
    dailyS = p ? new (p) DailyState() : new DailyState();
  }
  DailyState &d = *dailyS;
  if (d.demo != demoMode) {  // demo mode came or went: its puzzles, or today's from the file
    if (demoMode && d.dirty) dailySaveNow();
    d.demo = demoMode;
    sudokuReset(), nonogramReset();
    d.date = 0, d.loaded = demoMode;  // leaving: read the file again
  }
  if (!d.loaded) d.loaded = true, dailyLoad();
  long want = demoMode ? DAILY_DEMO_DATE : gToday();
  if (want && want != d.date) {  // a new day (or the first known one): yesterday's grids go
    if (d.date) Serial.printf("daily: a new day, %ld (was %ld)\n", want, d.date);
    sudokuReset(), nonogramReset();
    d.date = want;
    if (!demoMode) d.dirty = true;
  }
}

// The day the puzzles are for, 0 when it isn't known (no time yet).
static long dailyDate() {
  dailyEnsure();
  long today = demoMode ? DAILY_DEMO_DATE : gToday();
  return today && today == dailyS->date ? today : 0;
}
static void dailyChanged() {
  if (dailyS && !demoMode) dailyS->dirty = true;
}

// ---- /daily.txt

static void dailyLoad() {
  File f = LittleFS.open(DAILY_FILE, "r");
  if (!f) return;
  long date = 0;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    int sp = line.indexOf(' ');
    if (sp < 1) continue;
    String k = line.substring(0, sp), v = line.substring(sp + 1);
    if (k == "date") {
      date = v.toInt();
      long today = gToday();
      if (date <= 0 || (today && date != today)) break;  // another day's: nothing in it counts
      dailyS->date = date;
    } else if (date > 0 && k.startsWith("sudoku")) {
      sudokuLoadLine(k, v);
    } else if (date > 0 && k.startsWith("nonogram")) {
      nonogramLoadLine(k, v);
    }
  }
  f.close();
  Serial.printf("daily: read %s, date %ld\n", DAILY_FILE, dailyS->date);
}

// Writes /daily.txt now (dailySave() from the loop; dailyEnsure() before demo mode).
static void dailySaveNow() {
  DailyState &d = *dailyS;
  d.dirty = false;
  File f = LittleFS.open(DAILY_FILE, "w");
  if (!f) {
    Serial.println("daily: couldn't save today's grids");
    return;
  }
  f.printf("date %ld\n", d.date);
  sudokuSaveTo(f);
  nonogramSaveTo(f);
  f.close();
}

// The loop: today's grids, when they changed (never in demo mode). Not static: sticky-os.ino calls it.
void dailySave() {
  if (!dailyS || !dailyS->dirty || demoMode || dailyS->demo) return;
  dailySaveNow();
}

// ---- helpers the two puzzles share

// A time played: "4:05", "1:02:03".
static String dailyTime(uint32_t secs) {
  char b[20];
  if (secs >= 3600) snprintf(b, sizeof b, "%lu:%02lu:%02lu", (unsigned long)(secs / 3600), (unsigned long)(secs / 60 % 60), (unsigned long)(secs % 60));
  else snprintf(b, sizeof b, "%lu:%02lu", (unsigned long)(secs / 60), (unsigned long)(secs % 60));
  return b;
}

// A puzzle was solved: its streak (sdkDay/sdkStreak or nonDay/nonStreak) and best time.
// True if the time is the quickest yet.
static bool dailySolvedNow(long &day, int &streak, int &best, uint32_t secs) {
  long today = dailyDate();
  if (today && day != today) streak = gamesStreakAlive(day, streak, today) + 1, day = today;
  int s = max(1, (int)secs);
  bool newBest = !best || s < best;
  if (newBest) best = s;
  gamesChanged();
  beep(2400, 120);
  return newBest;
}

// The panel where the pad was, once a puzzle is solved: Solved!, the time, the streak, "Come
// back tomorrow", and two buttons: Daily (ID_BACK) and the level or size card (ID_NEW).
static void dailySolvedPanel(int x, int y, int w, int h, uint32_t secs, bool best, int streak, const char *otherLabel) {
  int cx = x + w / 2, by = y + h - 56;
  bool roomy = h >= 230;
  int ty = y + (roomy ? 40 : 34);
  textCentre(cx, ty, &FreeSansBold24pt7b, fitted(&FreeSansBold24pt7b, T(S_DAILY_SOLVED), w).c_str());
  ty += roomy ? 42 : 36;
  String time = TF(best ? S_DAILY_TIME_BEST : S_DAILY_TIME, dailyTime(secs).c_str());
  textCentre(cx, ty, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, time, w).c_str());
  ty += 30;
  if (streak) textCentre(cx, ty, &FreeSans12pt7b, fitted(&FreeSans12pt7b, TP(S_G_DAYS_IN_ROW, streak), w).c_str()), ty += 28;
  gCentred(cx, ty, &FreeSans9pt7b, T(S_DAILY_TOMORROW), w, 20, max(1, (by - 8 - ty) / 20 + 1));
  int bw = (w - 12) / 2;
  gButton(x, by, bw, 52, gameName(GAME_DAILY), false, ID_BACK, &FreeSansBold9pt7b);
  gButton(x + bw + 12, by, bw, 52, otherLabel, true, ID_NEW, &FreeSansBold9pt7b);
}

// The level / size card, as Mines' New game card: a title, `n` buttons (`firstId` on), each
// with a line under it, and Cancel (ID_CANCEL). `current`: the button drawn black.
static void dailyCard(const char *title, int n, int firstId, int current, const String *names, const String *lines) {
  int bw = min(W - 48, 400), bh = 74 + n * 86 + 66, bx = (W - bw) / 2, by = BODY_Y + (BODY_H - bh) / 2;
  canvas->fillRoundRect(bx - 6, by - 6, bw + 12, bh + 12, 22, PAPER);
  for (int k = 0; k < 3; k++) canvas->drawRoundRect(bx + k, by + k, bw - 2 * k, bh - 2 * k, 18 - k, INK);
  textCentre(W / 2, by + 44, &FreeSansBold18pt7b, fitted(&FreeSansBold18pt7b, title, bw - 32).c_str());
  int y = by + 66;
  for (int i = 0; i < n; i++, y += 86) {
    gButton(bx + 24, y, bw - 48, 50, names[i].c_str(), i == current, firstId + i);
    textCentre(W / 2, y + 72, &FreeSans9pt7b, fitted(&FreeSans9pt7b, lines[i], bw - 40).c_str());
  }
  gButton(bx + 24, by + bh - 60, bw - 48, 46, T(S_G_CANCEL), false, ID_CANCEL);
}

// The line under a row: what's done (or under way) today, then the streak.
static String dailyRowLine(bool done, const String &state, int streak) {
  String s = done ? String(T(S_G_L_DONE_TODAY)) : state;
  if (streak) s += (s.length() ? ", " : "") + TP(S_G_DAYS_IN_ROW, streak);
  return s;
}

// ---- the Daily screen

static void dailyIcon(int cx, int cy, int s, int t) {  // a calendar page with a star on it
  int r = s / 2, x = cx - r, y = cy - r + s / 10, w = s, h = s - s / 10;
  canvas->fillRoundRect(x, y, w, h * 30 / 100, 6, INK);
  for (int k = 0; k < t; k++) canvas->drawRoundRect(x + k, y + k, w - 2 * k, h - 2 * k, 6, INK);
  canvas->fillRoundRect(x + w / 4 - 2, y - s / 10, 5, s / 5, 2, INK);
  canvas->fillRoundRect(x + w * 3 / 4 - 2, y - s / 10, 5, s / 5, 2, INK);
  gStar(cx, y + h * 64 / 100, h * 26 / 100, true);
}

static void dailyCheck(int cx, int cy, int s) {  // done today: a tick in a ring
  ring(cx, cy, s / 2, 3);
  thickLine(cx - s * 22 / 100, cy + s * 2 / 100, cx - s * 6 / 100, cy + s * 18 / 100, 4);
  thickLine(cx - s * 6 / 100, cy + s * 18 / 100, cx + s * 24 / 100, cy - s * 18 / 100, 4);
}

static void drawDailyMenu() {
  GameProgress &p = gameProgress();
  long today = gToday();
  gHeader(gameName(GAME_DAILY));
  int top = gTop() + (TALL ? 16 : 6), gap = TALL ? 18 : 10, rh = TALL ? 140 : (H - 12 - top - 2 * gap) / 3;
  for (int i = 0; i < 3; i++) {
    int x = 24, y = top + i * (rh + gap), w = W - 48, id = DAILY_MATHS + i;
    canvas->drawRoundRect(x, y, w, rh, 16, INK);
    canvas->drawRoundRect(x + 1, y + 1, w - 2, rh - 2, 15, INK);
    int icon = min(rh - 36, 76), icx = x + 20 + icon / 2, cy = y + rh / 2;
    const char *name;
    String line;
    bool done;
    if (i == 0) {
      name = T(S_G_DAILY_MATHS), done = gamesDailyDone(today);
      dailyIcon(icx, cy, icon, max(2, icon / 18));
      line = dailyRowLine(done, T(S_G_L_FIVE_A_DAY), gamesDailyStreak(today));
    } else if (i == 1) {
      name = T(S_G_SUDOKU), done = today && p.sdkDay == today;
      sudokuIcon(icx, cy, icon, max(2, icon / 18));
      line = sudokuLine();
    } else {
      name = T(S_G_NONOGRAM), done = today && p.nonDay == today;
      nonogramIcon(icx, cy, icon, max(2, icon / 18));
      line = nonogramLine();
    }
    int tx = x + 40 + icon, tw = w - (tx - x) - (done ? 72 : 20);
    const GFXfont *nf = textW(&FreeSansBold18pt7b, name) <= tw ? &FreeSansBold18pt7b : &FreeSansBold12pt7b;  // a long name: smaller
    text(tx, cy - 6, nf, fitted(nf, name, tw).c_str());
    gLeft(tx, cy + 24, &FreeSans12pt7b, line, tw, 24, rh >= 120 ? 2 : 1);
    if (done) dailyCheck(x + w - 44, cy, 44);
    gTarget(x, y, w, rh, id);
  }
}

static void drawDailyNoDate() {
  gHeader(gameName(GAME_DAILY));
  int cy = gTop() + (TALL ? 110 : 56);
  textCentre(W / 2, cy, &FreeSansBold24pt7b, fitted(&FreeSansBold24pt7b, T(S_G_M_NO_DATE_TITLE), W - 32).c_str());
  gCentred(W / 2, cy + 60, &FreeSans12pt7b, T(S_G_M_NO_DATE), W - 80, 30, 4);
  int bw = TALL ? (W - 64) / 2 : 240;
  gButton((W - bw) / 2, H - 86, bw, 60, gameName(GAME_DAILY), true, ID_BACK);
}

static void drawDaily() {
  dailyEnsure();
  switch (dailyS->screen) {
    case DS_SUDOKU: return drawSudoku();
    case DS_NONOGRAM: return drawNonogram();
    case DS_NO_DATE: return drawDailyNoDate();
    default: return drawDailyMenu();
  }
}

// Back to the Daily screen (from a puzzle, Daily maths, the no-date notice).
static void dailyBack() {
  dailyEnsure();
  gs->gGame = GAME_DAILY;
  dailyS->screen = DS_MENU;
  gs->gFocusId = -1;
  render(false);
}

static void actDaily(int id) {
  dailyEnsure();
  DailyState &d = *dailyS;
  if (d.screen == DS_SUDOKU) return actSudoku(id);
  if (d.screen == DS_NONOGRAM) return actNonogram(id);
  if (d.screen == DS_NO_DATE) {
    if (id == ID_BACK) dailyBack();
    return;
  }
  gs->gFocusId = -1;
  switch (id) {
    case ID_BACK: gs->gGame = -1; return render(false);
    case DAILY_MATHS: gs->gGame = GAME_MATHS; return mStartDaily();  // as it always opened; Back comes here
    case DAILY_SUDOKU:
    case DAILY_NONOGRAM:
      if (!dailyDate()) return d.screen = DS_NO_DATE, render(false);
      d.screen = id == DAILY_SUDOKU ? DS_SUDOKU : DS_NONOGRAM;
      if (id == DAILY_SUDOKU) sudokuOpen();
      else nonogramOpen();
      return render(true);
  }
}

// From the launcher: the Daily screen, or the puzzle left on screen.
static void dailyOpen() {
  dailyEnsure();
  DailyState &d = *dailyS;
  if (d.screen == DS_NO_DATE || ((d.screen == DS_SUDOKU || d.screen == DS_NONOGRAM) && !dailyDate())) d.screen = DS_MENU;
  if (d.screen == DS_SUDOKU) sudokuOpen();
  if (d.screen == DS_NONOGRAM) nonogramOpen();
}

// Demo mode ended (crossplayDemoEnded): the demo grids go, dailyEnsure() reads today's back.
static void dailyForget() {
  if (!dailyS) return;
  sudokuForget(), nonogramForget();
  dailyEnsure();
}

static bool dailyOk() {
  dailyEnsure();
  if (dailyS->screen == DS_SUDOKU) return sudokuOk();
  if (dailyS->screen == DS_NONOGRAM) return nonogramOk();
  if (dailyS->screen == DS_NO_DATE) return dailyBack(), true;
  return false;
}

// The status bar's title: the puzzle's name on its screen.
static const char *dailyTitle() {
  dailyEnsure();
  if (dailyS->screen == DS_SUDOKU) return T(S_G_SUDOKU);
  if (dailyS->screen == DS_NONOGRAM) return T(S_G_NONOGRAM);
  return gameName(GAME_DAILY);
}

// The launcher's line: how many of the three are done today, else the best streak going.
static String dailyLine() {
  GameProgress &p = gameProgress();
  long today = gToday();
  int done = gamesDailyDone(today) + (today && p.sdkDay == today) + (today && p.nonDay == today);
  if (done) return TF(S_DAILY_L_DONE, done);
  int best = max(gamesDailyStreak(today), max(gamesStreakAlive(p.sdkDay, p.sdkStreak, today), gamesStreakAlive(p.nonDay, p.nonStreak, today)));
  return best ? TP(S_G_DAYS_IN_ROW, best) : String(T(S_DAILY_L_HINT));
}

static void dailyPeek(String &s) {
  dailyEnsure();
  if (dailyS->screen == DS_SUDOKU) return sudokuPeek(s);
  if (dailyS->screen == DS_NONOGRAM) return nonogramPeek(s);
  GameProgress &p = gameProgress();
  long today = gToday();
  char b[160];
  snprintf(b, sizeof b, "daily date %ld%s maths %d %d sudoku %d %d nonogram %d %d", today, dailyS->screen == DS_NO_DATE ? " (no date)" : "",
           (int)gamesDailyDone(today), gamesDailyStreak(today), today && p.sdkDay == today, gamesStreakAlive(p.sdkDay, p.sdkStreak, today),
           today && p.nonDay == today, gamesStreakAlive(p.nonDay, p.nonStreak, today));
  s += b;
}

// `game right` / `game wrong` on a puzzle: its next cell, right or wrong (see their tabs).
static void dailyAnswer(bool right) {
  dailyEnsure();
  if (dailyS->screen == DS_SUDOKU) sudokuAnswer(right);
  else if (dailyS->screen == DS_NONOGRAM) nonogramAnswer(right);
}

// ---- the serial port: `daily ...` (sticky-os.ino)
//
// daily                 the date the puzzles are for, and whether it's overridden
// daily date <yyyymmdd> test builds only (STICKY_TEST_BUILD, which ./upload.sh defines for every
//                       build that isn't a release, v*): today is that date until a restart or
//                       `daily date 0`, for Daily maths and both puzzles, their streaks included.
//                       Changing it drops the grids in memory as midnight does.
void dailyCommand(const String &arg) {
  if (arg.startsWith("date")) {
#ifdef STICKY_TEST_BUILD
    long d = arg.substring(4).toInt();
    if (d && (d < 19700101 || d > 29991231 || d / 100 % 100 < 1 || d / 100 % 100 > 12 || d % 100 < 1 || d % 100 > 31)) {
      Serial.println("daily date: <yyyymmdd>, or 0 for the clock's");
      return;
    }
    gs->gDateOverride = d;
    if (app == APP_GAMES && screen == SCREEN_APP) render(false);
#else
    Serial.println("daily date: test builds only");
    return;
#endif
  }
  long today = gToday();
  Serial.printf("daily: today %ld%s, puzzles for %ld\n", today, gs->gDateOverride ? " (daily date)" : "", dailyDate());
}
