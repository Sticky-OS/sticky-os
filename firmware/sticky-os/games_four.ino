// ---------------------------------------------------------------- Four in a Row
//
// Four in a Row: the rules are CrossPlay's (connectfour/ConnectFourCore, ConnectFourBrain, ConnectFourFlow, MIT; src/crossplay/README.md),
// this tab draws them and takes the taps, like the other games (gHeader,
// gButton, gTarget; games.ino). Its state lives in PSRAM (fourS, made by fourEnsure()).
// SCAFFOLD: filled in by the Four in a Row work; until then a placeholder screen.

struct FourState {
  bool made = false;
};
static FourState *fourS = nullptr;
// Made on first use. No function takes or returns FourState: the sketch's
// generated prototypes come before this tab, so they can't name its types.
static void fourEnsure() {
  if (fourS) return;
  void *p = heap_caps_malloc(sizeof(FourState), MALLOC_CAP_SPIRAM);
  fourS = p ? new (p) FourState() : new FourState();
}

static void fourOpen() { fourEnsure(); }
static void drawFour() { gHeader(gameName(GAME_FOUR)); }
static void actFour(int id) {
  if (id == ID_BACK) return gs->gGame = -1, gs->gFocusId = -1, render(false);
}
static void fourIcon(int cx, int cy, int s, int t) { canvas->drawRect(cx - s / 2, cy - s / 2, s, s, INK); }
static String fourLine() { return ""; }
static void fourPeek(String &s) { s += "four in a row"; }
static bool fourOk() { return false; }
