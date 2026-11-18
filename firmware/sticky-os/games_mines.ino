// ---------------------------------------------------------------- Mines
//
// Mines: the rules are CrossPlay's (minesweeper/MinesweeperCore, MinesweeperFlow, MIT; src/crossplay/README.md),
// this tab draws them and takes the taps, like the other games (gHeader,
// gButton, gTarget; games.ino). Its state lives in PSRAM (minesS, made by minesEnsure()).
// SCAFFOLD: filled in by the Mines work; until then a placeholder screen.

struct MinesState {
  bool made = false;
};
static MinesState *minesS = nullptr;
// Made on first use. No function takes or returns MinesState: the sketch's
// generated prototypes come before this tab, so they can't name its types.
static void minesEnsure() {
  if (minesS) return;
  void *p = heap_caps_malloc(sizeof(MinesState), MALLOC_CAP_SPIRAM);
  minesS = p ? new (p) MinesState() : new MinesState();
}

static void minesOpen() { minesEnsure(); }
static void drawMines() { gHeader(gameName(GAME_MINES)); }
static void actMines(int id) {
  if (id == ID_BACK) return gs->gGame = -1, gs->gFocusId = -1, render(false);
}
static void minesIcon(int cx, int cy, int s, int t) { canvas->drawRect(cx - s / 2, cy - s / 2, s, s, INK); }
static String minesLine() { return ""; }
static void minesPeek(String &s) { s += "mines"; }
static bool minesOk() { return false; }
