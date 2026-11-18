// ---------------------------------------------------------------- Mines
//
// Mines: the rules are CrossPlay's (minesweeper/MinesweeperCore, MIT; src/crossplay/README.md),
// in three sizes by minesgrid.h (small is CrossPlay's own 8 x 10 board, played the same; medium
// 9 x 12, large 10 x 13). This tab draws them and takes the taps, like the other games
// (gHeader, gButton, gTarget; games.ino). Its state lives in PSRAM (minesS, minesEnsure()).
//
// Tap a square to dig it (the first is always safe, and opens a patch); a number that already
// has its flags around it opens the rest (a chord). The Dig / Flag switch says what a tap does.
// OK with nothing ringed swaps Dig and Flag, or starts the next game once one is over.
// The counter is mines minus flags; the clock counts seconds between taps (minesgrid::Clock),
// drawn only when something changes. Upright, the board is the size's cols across; sideways
// it lies on its side (rows across), with the controls beside it.
//
// Ids, for taps, the ring and `game pick <id>`:
//   0..cells-1   a square: row * cols + col, the board held upright (as `game peek` lists them)
//   200 Dig, 201 Flag (the switch)
//   210 / 211 / 212  a new game, small / medium / large (on the New game card)
//   ID_NEW (901) the New game card; ID_CANCEL (905) closes it; ID_BACK (900) the launcher
// `game peek`: "mines <size> <cols>x<rows> <n> mines, mode dig|flag, state fresh|playing|won|lost,
// revealed N, flags F, left L, time Ts, card no|yes, next <a safe covered id or -1>, mine <a covered
// mine's id or -1>, cells <rows, '|' between>": '.' covered, 'x' covered mine, 'F' flag on a mine,
// 'f' flag on a safe square, '0'-'8' open, '*' an opened mine.
// Demo mode (`demo on`): every board is minesgrid::demo(), opened in the middle with three flags,
// the clock at 1:23 and stopped.
#include "minesgrid.h"

enum { MINES_DIG = 200, MINES_FLAG = 201, MINES_SIZE = 210 };

struct MinesState {
  minesgrid::Game g;
  minesgrid::Clock clock;
  int level = 0;          // the board's size, 0..2
  bool live = false;      // a board was made
  bool flagMode = false;  // a tap flags (else digs)
  bool choosing = false;  // the New game card is up
  bool newBest = false;   // the win just made was the quickest at its size
};
static MinesState *minesS = nullptr;
// Made on first use. No function takes or returns MinesState: the sketch's
// generated prototypes come before this tab, so they can't name its types.
static void minesEnsure() {
  if (minesS) return;
  void *p = heap_caps_malloc(sizeof(MinesState), MALLOC_CAP_SPIRAM);
  minesS = p ? new (p) MinesState() : new MinesState();
}

static StrId minesSizeName(int l) { return l == 0 ? S_MINES_SMALL : l == 1 ? S_MINES_MEDIUM : S_MINES_LARGE; }

static String minesTime(uint32_t secs) {
  char b[16];
  snprintf(b, sizeof b, "%lu:%02lu", (unsigned long)(secs / 60), (unsigned long)(secs % 60));
  return b;
}
static uint32_t minesSecs() { return minesgrid::clockShown(minesS->clock, millis()) / 1000; }

static void minesStart(int level) {
  MinesState &m = *minesS;
  m.level = constrain(level, 0, minesgrid::kSizes - 1);
  m.live = true, m.flagMode = false, m.choosing = false, m.newBest = false;
  m.clock = minesgrid::Clock();
  if (demoMode) {
    minesgrid::demo(m.g, m.level);
    m.clock.elapsedMs = 83000;  // 1:23, and stopped: the same picture every time
  } else {
    minesgrid::start(m.g, m.level, esp_random() | 1);
  }
  GameProgress &p = gameProgress();
  if (p.minesLevel != m.level) p.minesLevel = m.level, gamesChanged();
}

static void minesOpen() {
  minesEnsure();
  minesS->choosing = false;
  if (demoMode || !minesS->live) minesStart(gameProgress().minesLevel);
}

// ---- geometry: the board's cell, where it starts, and which way it lies

static bool minesSideways() { return !TALL; }
static int minesAcross() { return minesSideways() ? minesS->g.rows : minesS->g.cols; }
static int minesDown() { return minesSideways() ? minesS->g.cols : minesS->g.rows; }
static int minesStripY() { return gTop() + 2; }  // upright: the switch, the counter and the clock

static void minesGeometry(int &x0, int &y0, int &c) {
  int across = minesAcross(), down = minesDown();
  if (TALL) {
    int top = minesStripY() + 64, room = H - 10 - top;
    c = min((W - 16) / across, room / down);
    x0 = (W - across * c) / 2, y0 = top;
  } else {  // 210 px beside the board for the controls
    int room = BODY_H - 16;
    c = min(room / down, (W - 16 - 24 - 210) / across);
    x0 = 16, y0 = BODY_Y + 8 + (room - down * c) / 2;
  }
}

// Where square (col, row) is on the screen.
static void minesCellXY(int col, int row, int &x, int &y) {
  int x0, y0, c;
  minesGeometry(x0, y0, c);
  int sx = minesSideways() ? row : col, sy = minesSideways() ? col : row;
  x = x0 + sx * c, y = y0 + sy * c;
}

// ---- pictures

static void minesMine(int cx, int cy, int r, uint16_t ink, uint16_t paper) {
  int t = max(2, r / 4);
  for (int k = 0; k < 4; k++) {
    float a = k * PI / 4;
    int dx = cosf(a) * r * 1.45f, dy = sinf(a) * r * 1.45f;
    thickLine(cx - dx, cy - dy, cx + dx, cy + dy, t, ink);
  }
  canvas->fillCircle(cx, cy, r, ink);
  canvas->fillCircle(cx - r / 3, cy - r / 3, max(2, r / 4), paper);
}

static void minesFlag(int cx, int cy, int s, uint16_t ink) {
  int pole = max(2, s / 12), top = cy - s * 40 / 100, bottom = cy + s * 36 / 100, px = cx - s / 6;
  canvas->fillRect(px, top, pole, bottom - top, ink);
  canvas->fillTriangle(px + pole, top, px + pole, top + s * 36 / 100, px + pole + s * 44 / 100, top + s * 18 / 100, ink);
  canvas->fillRect(px - s / 5, bottom - pole, s * 2 / 5 + pole, pole + 1, ink);
}

static void minesIcon(int cx, int cy, int s, int t) {
  (void)t;
  minesMine(cx, cy, s * 26 / 100, INK, PAPER);
}

// One square. Covered: a hatched tile; flagged: a clear tile with a black flag; open: white
// with its number; at the end the mines show (the one that went off is black).
static void minesDrawCell(int col, int row) {
  using namespace minesgrid;
  const Game &g = minesS->g;
  int x, y, x0, y0, c;
  minesCellXY(col, row, x, y);
  minesGeometry(x0, y0, c);
  uint8_t v = at(g, col, row);
  bool lost = g.status == Status::Lost, won = g.status == Status::Won;
  canvas->drawRect(x, y, c + 1, c + 1, INK);  // the grid
  if (v & kRevealed) {
    if (v & kMine) {  // the one that went off
      canvas->fillRect(x + 1, y + 1, c - 1, c - 1, INK);
      minesMine(x + c / 2, y + c / 2, c * 22 / 100, PAPER, INK);
      return;
    }
    int n = neighbouringMines(g, col, row);
    if (n) {
      const GFXfont *f = c >= 46 ? &FreeSansBold18pt7b : &FreeSansBold12pt7b;
      char d[2] = {(char)('0' + n), 0};
      textCentre(x + c / 2 + 1, y + c / 2 + (c >= 46 ? 13 : 9), f, d);
    }
    return;
  }
  for (int k = 2; k < 4; k++) canvas->drawRect(x + k, y + k, c + 1 - 2 * k, c + 1 - 2 * k, INK);
  bool flag = (v & kFlagged) || (won && (v & kMine));
  if (flag) {
    minesFlag(x + c / 2, y + c / 2, c, INK);
    if (lost && !(v & kMine)) gGlyphX(x + c / 2, y + c / 2, c * 30 / 100, 3);  // a flag that was wrong
  } else if (lost && (v & kMine)) {
    minesMine(x + c / 2, y + c / 2, c * 22 / 100, INK, PAPER);
  } else {
    gHatch(x + 5, y + 5, c - 9, c - 9, 6);
  }
}

// The Dig / Flag switch: the side in use is black.
static void minesSwitch(int x, int y, int w, int h) {
  int half = w / 2;
  for (int i = 0; i < 2; i++) {
    bool on = (i == 1) == minesS->flagMode;
    int bx = x + i * half, rad = min(h / 2, 20);
    uint16_t fg = on ? PAPER : INK;
    if (on) canvas->fillRoundRect(bx, y, half, h, rad, INK);
    else canvas->drawRoundRect(bx, y, half, h, rad, INK), canvas->drawRoundRect(bx + 1, y + 1, half - 2, h - 2, rad - 1, INK);
    const char *label = T(i ? S_MINES_FLAG : S_MINES_DIG);
    int iconW = h * 40 / 100, tw = textW(&FreeSansBold12pt7b, label), left = bx + (half - tw - iconW - 8) / 2;
    if (i) {
      minesFlag(left + iconW / 2, y + h / 2, h * 56 / 100, fg);
    } else {  // a spade's blade for Dig
      canvas->fillTriangle(left + iconW / 2, y + h * 26 / 100, left + 1, y + h / 2, left + iconW - 1, y + h / 2, fg);
      canvas->fillRect(left + iconW / 2 - 2, y + h / 2, 4, h * 26 / 100, fg);
    }
    canvas->setTextColor(fg);
    text(left + iconW + 8, y + h / 2 + 8, &FreeSansBold12pt7b, label);
    canvas->setTextColor(INK);
    gTarget(bx, y, half, h, i ? MINES_FLAG : MINES_DIG);
  }
}

// The counter (a mine and how many are left) and the clock, at baseline y.
static void minesCounter(int x, int right, int y) {
  char n[12];
  snprintf(n, sizeof n, "%d", minesgrid::minesRemaining(minesS->g));
  minesMine(x + 12, y - 12, 9, INK, PAPER);
  text(x + 32, y, &FreeSansBold18pt7b, n);
  textRight(right, y, &FreeSansBold18pt7b, minesTime(minesSecs()).c_str());
}

// What's happening, in words.
static String minesSay() {
  using minesgrid::Status;
  switch (minesS->g.status) {
    case Status::Won: return TF(minesS->newBest ? S_MINES_WON_BEST : S_MINES_WON, minesTime(minesSecs()).c_str());
    case Status::Lost: return T(S_MINES_LOST);
    case Status::Fresh: return T(S_MINES_FIRST);
    default: return T(minesS->flagMode ? S_MINES_HOW_FLAG : S_MINES_HOW_DIG);
  }
}

// The New game card: the three sizes (the current one black), their size and best time, Cancel.
static void minesChooser() {
  GameProgress &p = gameProgress();
  int bw = min(W - 48, 400), bh = 74 + 3 * 86 + 66, bx = (W - bw) / 2, by = BODY_Y + (BODY_H - bh) / 2;
  canvas->fillRoundRect(bx - 6, by - 6, bw + 12, bh + 12, 22, PAPER);
  for (int k = 0; k < 3; k++) canvas->drawRoundRect(bx + k, by + k, bw - 2 * k, bh - 2 * k, 18 - k, INK);
  textCentre(W / 2, by + 44, &FreeSansBold18pt7b, T(S_G_NEW_GAME));
  int y = by + 66;
  for (int l = 0; l < minesgrid::kSizes; l++, y += 86) {
    gButton(bx + 24, y, bw - 48, 50, T(minesSizeName(l)), l == minesS->level, MINES_SIZE + l);
    const minesgrid::Size &z = minesgrid::kSize[l];
    String line = TF(S_MINES_SIZE_LINE, z.cols, z.rows, z.mines);
    if (p.minesBest[l]) line += "   " + TF(S_MINES_BEST, minesTime(p.minesBest[l]).c_str());
    textCentre(W / 2, y + 72, &FreeSans9pt7b, fitted(&FreeSans9pt7b, line, bw - 40).c_str());
  }
  gButton(bx + 24, by + bh - 60, bw - 48, 46, T(S_G_CANCEL), false, ID_CANCEL);
}

static void drawMines() {
  using namespace minesgrid;
  minesEnsure();
  if (!minesS->live) minesStart(gameProgress().minesLevel);
  MinesState &m = *minesS;
  const Game &g = m.g;
  int x0, y0, c;
  minesGeometry(x0, y0, c);
  bool ended = over(g), targets = !m.choosing;
  if (TALL) {  // the header with New game; under it the switch (or how it ended), the counter, the clock
    gHeader(gameName(GAME_MINES), "", 130);  // Back closes the card when it's up
    if (targets) gButton(W - 16 - 124, BODY_Y + 10, 124, 48, T(S_G_NEW_GAME), ended, ID_NEW, &FreeSansBold9pt7b);
    int sy = minesStripY();
    if (ended) gLeft(16, sy + 26, &FreeSansBold12pt7b, minesSay(), 226, 24, 2);
    else if (targets) minesSwitch(16, sy + 4, 210, 52);
    minesCounter(250, W - 20, sy + 44);
  }
  // The board: the squares a tap can change are targets, in reading order.
  for (int sy = 0; sy < minesDown(); sy++)
    for (int sx = 0; sx < minesAcross(); sx++) {
      int col = minesSideways() ? sy : sx, row = minesSideways() ? sx : sy;
      minesDrawCell(col, row);
      if (!targets || ended) continue;
      if (!(at(g, col, row) & kRevealed) || (!m.flagMode && chordable(g, col, row))) {
        int x, y;
        minesCellXY(col, row, x, y);
        gTarget(x + 3, y + 3, c - 5, c - 5, row * g.cols + col);
      }
    }
  for (int k = 0; k < 2; k++) canvas->drawRect(x0 - 1 - k, y0 - 1 - k, minesAcross() * c + 3 + 2 * k, minesDown() * c + 3 + 2 * k, INK);
  if (TALL) {
    int below = y0 + minesDown() * c + 24;
    if (below <= H - 6 && !ended) textCentre(W / 2, below, &FreeSans9pt7b, fitted(&FreeSans9pt7b, minesSay(), W - 32).c_str());
  } else {  // beside the board: Back, the switch, the counter and the clock, what's happening, the size, New game
    int px = x0 + minesAcross() * c + 24, pw = W - 16 - px;
    if (targets) gButton(px, BODY_Y + 10, 116, 48, T(S_G_BACK), false, ID_BACK);
    if (targets && !ended) minesSwitch(px, BODY_Y + 76, pw, 52);
    minesCounter(px, W - 16, BODY_Y + 178);
    gLeft(px, BODY_Y + 222, ended ? &FreeSansBold12pt7b : &FreeSans12pt7b, minesSay(), pw, 28, 3);
    const Size &z = kSize[m.level];
    // the size: its name, and under it its columns, rows and mines
    text(px, H - 116, &FreeSansBold9pt7b, fitted(&FreeSansBold9pt7b, T(minesSizeName(m.level)), pw).c_str());
    text(px, H - 92, &FreeSans9pt7b, fitted(&FreeSans9pt7b, TF(S_MINES_SIZE_LINE, z.cols, z.rows, z.mines), pw).c_str());
    if (targets) gButton(px, H - 76, pw, 60, T(S_G_NEW_GAME), ended, ID_NEW);
  }
  if (m.choosing) minesChooser();
}

// A game just ended: the tally, the best time, a sound.
static void minesEnded() {
  MinesState &m = *minesS;
  GameProgress &p = gameProgress();
  minesgrid::clockStop(m.clock, millis());
  if (m.g.status == minesgrid::Status::Won) {
    int secs = max(1, (int)minesSecs());
    p.minesWon[m.level]++;
    m.newBest = !p.minesBest[m.level] || secs < p.minesBest[m.level];
    if (m.newBest) p.minesBest[m.level] = secs;
    beep(2400, 120);
  } else {
    p.minesLost++;
    beep(800, 200);
  }
  gamesChanged();
  gs->gFocusId = -1;
}

static void minesTapCell(int id) {
  using namespace minesgrid;
  MinesState &m = *minesS;
  Game &g = m.g;
  if (m.choosing || over(g) || id < 0 || id >= cells(g)) return;
  int col = id % g.cols, row = id / g.cols;
  bool fresh = g.status == Status::Fresh;
  if (!(m.flagMode ? toggleFlag(g, col, row) : dig(g, col, row))) return;  // nothing changed: nothing to draw
  uint32_t now = millis();
  if (fresh && g.status != Status::Fresh && !demoMode) clockStart(m.clock, now);
  else if (m.clock.running) clockTick(m.clock, now);
  if (over(g)) {
    minesEnded();
    return render(true);
  }
  beep(2600, 15);
  render(false);
}

static void actMines(int id) {
  minesEnsure();
  MinesState &m = *minesS;
  if (id == ID_BACK) {
    if (m.choosing) return m.choosing = false, gs->gFocusId = -1, render(false);
    return gs->gGame = -1, gs->gFocusId = -1, render(false);
  }
  if (id == ID_NEW) return m.choosing = true, gs->gFocusId = -1, render(false);
  if (id == ID_CANCEL) {
    if (!m.choosing) return;
    return m.choosing = false, gs->gFocusId = -1, render(false);
  }
  if (id >= MINES_SIZE && id < MINES_SIZE + minesgrid::kSizes) return minesStart(id - MINES_SIZE), gs->gFocusId = -1, render(true);
  if (id == MINES_DIG || id == MINES_FLAG) {
    if (m.choosing || minesgrid::over(m.g) || m.flagMode == (id == MINES_FLAG)) return;
    m.flagMode = id == MINES_FLAG;
    return render(false);
  }
  minesTapCell(id);
}

// OK with nothing ringed: on the card, its size; after the end, the next game; else Dig <-> Flag.
static bool minesOk() {
  minesEnsure();
  MinesState &m = *minesS;
  if (m.choosing || minesgrid::over(m.g)) {
    minesStart(m.level);
    gs->gFocusId = -1;
    render(true);
    return true;
  }
  actMines(m.flagMode ? MINES_DIG : MINES_FLAG);
  return true;
}

static String minesLine() {
  minesEnsure();
  GameProgress &p = gameProgress();
  if (minesS->live && minesS->g.status == minesgrid::Status::Playing) return TF(S_MINES_L_LEFT, minesgrid::minesRemaining(minesS->g));
  int won = p.minesWon[0] + p.minesWon[1] + p.minesWon[2];
  if (won) return TF(S_MINES_L_WON, won);
  return T(S_MINES_L_HINT);
}

static void minesPeek(String &s) {
  using namespace minesgrid;
  minesEnsure();
  MinesState &m = *minesS;
  const Game &g = m.g;
  static const char *const SIZES[] = {"small", "medium", "large"}, *const STATES[] = {"fresh", "playing", "won", "lost"};
  int next = -1, mine = -1;
  String cellsText;
  for (int r = 0; r < g.rows; r++) {
    if (r) cellsText += '|';
    for (int c = 0; c < g.cols; c++) {
      uint8_t v = at(g, c, r);
      int id = r * g.cols + c;
      char ch = (v & kRevealed) ? ((v & kMine) ? '*' : char('0' + neighbouringMines(g, c, r)))
                : (v & kFlagged) ? ((v & kMine) ? 'F' : 'f') : ((v & kMine) ? 'x' : '.');
      if (ch == '.' && next < 0) next = id;
      if (ch == 'x' && mine < 0) mine = id;
      cellsText += ch;
    }
  }
  if (g.status == Status::Fresh) next = (g.rows / 2) * g.cols + g.cols / 2;
  if (over(g)) next = mine = -1;
  char head[220];
  snprintf(head, sizeof head, "mines %s %dx%d %d mines, mode %s, state %s, revealed %d, flags %d, left %d, time %lus, card %s, next %d, mine %d, cells ",
           SIZES[m.level], g.cols, g.rows, g.mines, m.flagMode ? "flag" : "dig", STATES[(int)g.status], count(g, kRevealed), count(g, kFlagged),
           minesRemaining(g), (unsigned long)minesSecs(), m.choosing ? "yes" : "no", next, mine);
  s += head;
  s += cellsText;
}
