// ---------------------------------------------------------------- Chess
//
// Chess: the rules are CrossPlay's (chess/ChessCore, ChessEngine, ChessPieces, MIT; src/crossplay/README.md),
// this tab draws them and takes the taps, like the other games (gHeader,
// gButton, gTarget; games.ino). Its state lives in PSRAM (chessS, made by chessEnsure()).
// SCAFFOLD: filled in by the Chess work; until then a placeholder screen.

struct ChessState {
  bool made = false;
};
static ChessState *chessS = nullptr;
// Made on first use. No function takes or returns ChessState: the sketch's
// generated prototypes come before this tab, so they can't name its types.
static void chessEnsure() {
  if (chessS) return;
  void *p = heap_caps_malloc(sizeof(ChessState), MALLOC_CAP_SPIRAM);
  chessS = p ? new (p) ChessState() : new ChessState();
}

static void chessOpen() { chessEnsure(); }
static void drawChess() { gHeader(gameName(GAME_CHESS)); }
static void actChess(int id) {
  if (id == ID_BACK) return gs->gGame = -1, gs->gFocusId = -1, render(false);
}
static void chessIcon(int cx, int cy, int s, int t) { canvas->drawRect(cx - s / 2, cy - s / 2, s, s, INK); }
static String chessLine() { return ""; }
static void chessPeek(String &s) { s += "chess"; }
static bool chessOk() { return false; }
