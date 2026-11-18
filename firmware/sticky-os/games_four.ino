// ---------------------------------------------------------------- Four in a Row
//
// Four in a Row: the rules are CrossPlay's (connectfour/ConnectFourCore, ConnectFourBrain, ConnectFourFlow, MIT; src/crossplay/README.md),
// this tab draws them and takes the taps, like the other games (gHeader,
// gButton, gTarget; games.ino). Its state lives in PSRAM (fourS, made by fourEnsure()).
//
// You and the board take turns to start, a game each, like X and O. Your discs
// are rings, the board's are solid; the disc played last has a dot in it, and
// the winning four are struck through. The board looks boards::kFourDepth[level]
// plies ahead (boardgames.h: the levels' one place) and replies at once: its
// search takes well under a second, so a move is one partial refresh; a new
// game and the end are full ones.
//
// Layout (from W, H): the board on the left with the controls beside it in
// landscape; in portrait the header on top, the board, the controls below.
//
// Targets, in ring order (UP / DOWN), and the ids `game pick <id>` takes:
//   0..6     a column, left to right (only the ones that take a disc, on your turn)
//   10..12   the level: easy, medium, hard
//   901      New game (ID_NEW)
//   900      Back (ID_BACK)
// OK with no ring: rings the middle-most open column; after the end, New game.
// `game peek`: "four <6 rows, top first, '/' between; . empty, X you, O the board>
//   <you to move|board to move|you win|board wins|draw>, level <n>, last <column|->"

#include "boardgames.h"

struct FourState {
  bool made = false;
  connectfour::Game game = {};
  uint8_t human = connectfour::kLight;  // which side you are: Light always opens, so it's whoever starts
  bool humanStarts = false;             // flipped by every new game: the first one is yours
  int win[connectfour::kLine] = {};     // the winning four (flat column * kRows + row), when there is one
  StrId say = S_FOUR_START;             // what's happening, beside the board
};
static FourState *fourS = nullptr;
// Made on first use. No function takes or returns FourState: the sketch's
// generated prototypes come before this tab, so they can't name its types.
static void fourEnsure() {
  if (fourS) return;
  void *p = heap_caps_malloc(sizeof(FourState), MALLOC_CAP_SPIRAM);
  fourS = p ? new (p) FourState() : new FourState();
}

enum : int16_t { FOUR_LEVEL0 = 10 };

static bool fourOver() { return connectfour::over(fourS->game); }
static bool fourYourTurn() { return !fourOver() && fourS->game.turn == fourS->human; }
static int fourHumanDiscs() {
  int n = 0;
  for (int c = 0; c < connectfour::kColumns; c++)
    for (int r = 0; r < connectfour::kRows; r++) n += fourS->game.cell[c][r] == fourS->human;
  return n;
}

static void fourBoardMoves() {
  int col = boards::fourColumn(fourS->game, boards::kFourDepth[boards::clampLevel(gameProgress().fourLevel)]);
  if (col != connectfour::kNoColumn) connectfour::drop(fourS->game, col);
}

static void fourNew() {
  fourS->made = true;
  connectfour::start(fourS->game);
  fourS->humanStarts = !fourS->humanStarts;
  fourS->human = fourS->humanStarts ? connectfour::kLight : connectfour::kDark;
  fourS->say = fourS->humanStarts ? S_FOUR_START : S_FOUR_BOARD_FIRST;
  if (demoMode) {  // screenshots: a game under way, your three on the floor and the board's block at its end
    fourS->humanStarts = true, fourS->human = connectfour::kLight;
    for (int c : {3, 3, 4, 2, 2, 4, 5, 6}) connectfour::drop(fourS->game, c);
    fourS->say = S_FOUR_YOUR_TURN;
    return;
  }
  if (!fourS->humanStarts) fourBoardMoves();
}

// The game has just ended: the tally, a sound, the line to mark.
static void fourEnd() {
  using namespace connectfour;
  GameProgress &p = gameProgress();
  Outcome o = fourS->game.outcome;
  bool youWon = o != Outcome::Draw && (o == Outcome::LightWins) == (fourS->human == kLight);
  if (o == Outcome::Draw) p.fourDraws++, fourS->say = S_FOUR_DRAW;
  else if (youWon) p.fourWins++, fourS->say = S_FOUR_WIN, beep(2400, 120);
  else p.fourLosses++, fourS->say = S_FOUR_LOSE, beep(800, 200);
  if (o != Outcome::Draw) winningLine(fourS->game, o == Outcome::LightWins ? kLight : kDark, fourS->win);
  gamesChanged();
  gs->gFocusId = -1;
}

static void fourDrop(int col) {
  if (!fourYourTurn() || !connectfour::canDrop(fourS->game, col)) return;
  connectfour::drop(fourS->game, col);
  if (!fourOver()) fourBoardMoves();
  fourS->say = S_FOUR_YOUR_TURN;
  if (fourOver()) {
    fourEnd();
    return render(true);
  }
  beep(2600, 15);
  render(false);
}

static void fourGeometry(int &x0, int &y0, int &c) {
  using namespace connectfour;
  if (TALL) c = (W - 40) / kColumns, x0 = (W - kColumns * c) / 2, y0 = gTop() + 10;
  else c = min(68, (BODY_H - 24) / kRows), x0 = 24, y0 = BODY_Y + (BODY_H - kRows * c) / 2;
}

static void drawFour() {
  using namespace connectfour;
  fourEnsure();
  if (!fourS->made) fourNew();
  GameProgress &p = gameProgress();
  int x0, y0, c;
  fourGeometry(x0, y0, c);
  int r = c * 40 / 100;
  for (int k = 0; k < 3; k++) canvas->drawRoundRect(x0 - 8 + k, y0 - 8 + k, kColumns * c + 16 - 2 * k, kRows * c + 16 - 2 * k, 16, INK);
  // The columns first: they're what the ring goes through first.
  bool yours = fourYourTurn();
  for (int col = 0; col < kColumns; col++)
    if (yours && canDrop(fourS->game, col)) gTarget(x0 + col * c + 6, y0, c - 12, kRows * c, col);
  for (int col = 0; col < kColumns; col++)
    for (int row = 0; row < kRows; row++) {
      int cx = x0 + col * c + c / 2, cy = y0 + (kRows - 1 - row) * c + c / 2;  // row 0 is the floor
      uint8_t v = fourS->game.cell[col][row];
      bool last = col == fourS->game.lastColumn && row == fourS->game.lastRow;
      if (v == kEmpty) {
        if (yours && gs->gFocusId == col && row == landingRow(fourS->game, col)) {  // where the ringed column's disc would land
          canvas->drawCircle(cx, cy, r, INK), canvas->drawCircle(cx, cy, r - 1, INK);
        } else {
          canvas->fillCircle(cx, cy, 3, INK);
        }
      } else if (v == fourS->human) {
        ring(cx, cy, r, 6);
        if (last) canvas->fillCircle(cx, cy, r / 3, INK);
      } else {
        canvas->fillCircle(cx, cy, r, INK);
        if (last) canvas->fillCircle(cx, cy, r / 3, PAPER);
      }
    }
  if (fourOver() && fourS->game.outcome != Outcome::Draw) {  // the four, struck through: ink with a white core reads on both kinds of disc
    int a = fourS->win[0], b = fourS->win[kLine - 1];
    int ax = x0 + (a / kRows) * c + c / 2, ay = y0 + (kRows - 1 - a % kRows) * c + c / 2;
    int bx = x0 + (b / kRows) * c + c / 2, by = y0 + (kRows - 1 - b % kRows) * c + c / 2;
    thickLine(ax, ay, bx, by, 13);
    thickLine(ax, ay, bx, by, 5, PAPER);
  }

  // Beside the board (below it in portrait): what's happening, the level, the tally, a new game.
  int px = TALL ? 24 : x0 + kColumns * c + 24, pw = W - 24 - px, py;
  const GFXfont *sayFont = fourOver() ? &FreeSansBold18pt7b : &FreeSansBold12pt7b;
  if (TALL) {
    py = gCentred(px + pw / 2, y0 + kRows * c + 46, sayFont, T(fourS->say), pw, 32, 2) - 6;
  } else {
    py = gCentred(px + pw / 2, BODY_Y + 104, sayFont, T(fourS->say), pw, 32, 3) - 6;
    py = max(py, BODY_Y + 150);
  }
  static const StrId LEVELS[3] = {S_FOUR_EASY, S_FOUR_MEDIUM, S_FOUR_HARD};
  int lw = (pw - 20) / 3;
  for (int l = 0; l < 3; l++) {
    int lx = px + l * (lw + 10);
    pillButton(lx, py, lw, 48, fitted(&FreeSansBold9pt7b, T(LEVELS[l]), lw - 12).c_str(), l == p.fourLevel, &FreeSansBold9pt7b);
    gTarget(lx, py, lw, 48, FOUR_LEVEL0 + l);
  }
  String tally = TF(S_FOUR_TALLY, p.fourWins, p.fourDraws, p.fourLosses);
  const GFXfont *tf = textW(&FreeSans12pt7b, tally.c_str()) <= pw ? &FreeSans12pt7b : &FreeSans9pt7b;
  textCentre(px + pw / 2, py + 90, tf, fitted(tf, tally, pw).c_str());
  gButton(px, H - 80, pw, 60, T(S_G_NEW_GAME), fourOver(), ID_NEW);
  if (TALL) gHeader(gameName(GAME_FOUR));
  else gButton(px, BODY_Y + 10, 116, 48, T(S_G_BACK), false, ID_BACK);
}

static void actFour(int id) {
  fourEnsure();
  if (id == ID_BACK) return gs->gGame = -1, gs->gFocusId = -1, render(false);
  if (id == ID_NEW) return fourNew(), gs->gFocusId = -1, render(true);
  if (id >= FOUR_LEVEL0 && id < FOUR_LEVEL0 + 3) {
    gameProgress().fourLevel = id - FOUR_LEVEL0;
    gamesChanged();
    if (!fourOver() && fourHumanDiscs() == 0)  // not begun: begin again at the new level, the same side starting
      fourS->humanStarts = !fourS->humanStarts, fourNew();
    return render(false);
  }
  if (id >= 0 && id < connectfour::kColumns) fourDrop(id);
}

static void fourOpen() {
  fourEnsure();
  if (!fourS->made || fourOver() || demoMode) fourNew();  // else the game where it was left
}

// A frame with two rows of discs in it, and a ring dropping in.
static void fourIcon(int cx, int cy, int s, int t) {
  int r = s / 2, c = s / 4, d = c * 40 / 100 + 1, top = cy - r + s / 4;
  for (int k = 0; k < t; k++) canvas->drawRoundRect(cx - r + k, top + k, s - 2 * k, s * 3 / 4 - 2 * k, s / 8, INK);
  int y0 = top + (s * 3 / 4 - 2 * c) / 2;
  static const char ROWS[2][5] = {"..O.", "XOXO"};
  for (int row = 0; row < 2; row++)
    for (int col = 0; col < 4; col++) {
      int x = cx - 2 * c + col * c + c / 2, y = y0 + row * c + c / 2;
      char v = ROWS[row][col];
      if (v == 'X') canvas->fillCircle(x, y, d, INK);
      else if (v == 'O') ring(x, y, d, max(2, t));
    }
  canvas->fillCircle(cx - c / 2, top - d - 2, d, INK);  // the next disc, over the frame
}

static String fourLine() {
  GameProgress &p = gameProgress();
  if (fourS && fourS->made && !fourOver() && fourHumanDiscs() > 0) return T(S_FOUR_L_UNDER_WAY);
  if (p.fourWins + p.fourDraws + p.fourLosses) return TF(S_FOUR_L_TALLY, p.fourWins, p.fourDraws);
  return T(S_FOUR_L_HINT);
}

static void fourPeek(String &s) {
  using namespace connectfour;
  fourEnsure();
  s += "four ";
  for (int row = kRows - 1; row >= 0; row--) {
    for (int col = 0; col < kColumns; col++) {
      uint8_t v = fourS->game.cell[col][row];
      s += v == kEmpty ? '.' : v == fourS->human ? 'X' : 'O';
    }
    if (row) s += '/';
  }
  Outcome o = fourS->game.outcome;
  if (o == Outcome::Running) s += fourYourTurn() ? " you to move" : " board to move";
  else if (o == Outcome::Draw) s += " draw";
  else s += ((o == Outcome::LightWins) == (fourS->human == kLight)) ? " you win" : " board wins";
  s += ", level " + String(gameProgress().fourLevel) + ", last ";
  s += fourS->game.lastColumn >= 0 ? String((int)fourS->game.lastColumn) : String("-");
}

// OK with nothing ringed: your turn rings the middle-most column that takes a disc.
static bool fourOk() {
  fourEnsure();
  if (!fourYourTurn()) return false;  // the end: New game
  for (int col : connectfour::kSearchOrder)
    if (connectfour::canDrop(fourS->game, col)) {
      gs->gFocusId = col;
      render(false);
      return true;
    }
  return false;
}
