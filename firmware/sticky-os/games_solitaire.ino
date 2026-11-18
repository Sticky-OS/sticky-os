// ---------------------------------------------------------------- Solitaire
//
// Solitaire: the rules are CrossPlay's (solitaire/SolitaireCore, MIT; src/crossplay/README.md),
// this tab draws them and takes the taps, like the other games (gHeader,
// gButton, gTarget; games.ino). Its state lives in PSRAM (solitaireS, made by solitaireEnsure()).
// SCAFFOLD: filled in by the Solitaire work; until then a placeholder screen.

struct SolitaireState {
  bool made = false;
};
static SolitaireState *solitaireS = nullptr;
// Made on first use. No function takes or returns SolitaireState: the sketch's
// generated prototypes come before this tab, so they can't name its types.
static void solitaireEnsure() {
  if (solitaireS) return;
  void *p = heap_caps_malloc(sizeof(SolitaireState), MALLOC_CAP_SPIRAM);
  solitaireS = p ? new (p) SolitaireState() : new SolitaireState();
}

static void solitaireOpen() { solitaireEnsure(); }
static void drawSolitaire() { gHeader(gameName(GAME_SOLITAIRE)); }
static void actSolitaire(int id) {
  if (id == ID_BACK) return gs->gGame = -1, gs->gFocusId = -1, render(false);
}
static void solitaireIcon(int cx, int cy, int s, int t) { canvas->drawRect(cx - s / 2, cy - s / 2, s, s, INK); }
static String solitaireLine() { return ""; }
static void solitairePeek(String &s) { s += "solitaire"; }
static bool solitaireOk() { return false; }
