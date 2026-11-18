// ---------------------------------------------------------------- Solitaire
//
// Solitaire (Klondike): the rules are CrossPlay's (solitaire/SolitaireCore, MIT;
// src/crossplay/README.md); what a tap does, the finish, the hint and "no moves left" are
// solui.h (tested on the computer: firmware/test/games/test_solui.cpp). This tab draws them and
// takes the taps, like the other games (gHeader, gButton, gTarget; games.ino). Its state lives
// in PSRAM (solitaireS, made by solitaireEnsure()).
//
// Tap a card: it goes to its foundation, or to the only column it fits; with more than one
// place it's picked up (drawn black), and the next tap says where. Tap the stock to turn a
// card (draw 1) or three (draw 3, chosen on the New game card). Once every column card is face
// up the deal is decided and plays itself out. Undo takes back a move at a time. OK with
// nothing ringed turns the stock (on a card: deals the same kind again).
// Cards: the rank and a suit drawn with lines; hearts and diamonds (red) in outline, clubs and
// spades (black) solid. Upright the stock, waste and foundations are a row over the columns;
// sideways the stock and waste are on the left of the columns, the foundations on the right.
//
// Ids, for taps, the ring and `game pick <id>` (solui.h): pile * 30 + the card's index in the
// pile (0 the bottom card), pile * 30 + 29 the pile's place (an empty column; any column or
// foundation as where a picked-up card goes). Piles: 0 the stock (29 turns it), 1 the waste
// (its top card), 2-5 the foundations, 6-12 the columns left to right.
//   400 Undo; 410 / 411 deal a new game, draw 1 / draw 3 (on the New game card)
//   ID_NEW (901) the New game card; ID_CANCEL (905) closes it; ID_BACK (900) the launcher
// `game peek`: "solitaire draw 1|3, moves M, state playing|won|stuck, card no|new|won, sel <id
// or -1>, stock N, waste N top <card>, found 2:<top>/<count> 3:... , cols 6:<cards> | 7:...,
// hint <id> <id> | hint 29 | hint none": cards as 10H, KS, AC; ## face down; - none. A column
// lists its cards bottom first, so the k-th card's id is pile * 30 + k. The hint is the move
// solui::hint() would make: pick the first id, then (if it was picked up) the second.
// Demo mode (`demo on`): deal SOL_DEMO_SEED and play SOL_DEMO_STEPS hints, a game half done;
// New game deals SOL_DEMO_STUCK_SEED. test_solui.cpp has the taps that win and lose them.
#include "solui.h"

enum { SOL_UNDO = 400, SOL_DRAW = 410 };
static const uint32_t SOL_DEMO_SEED = 123, SOL_DEMO_STUCK_SEED = 1725;
static const int SOL_DEMO_STEPS = 37;

struct SolitaireState {
  solitaire::Game g, scratch;  // scratch: where solui::finish() tries the end first
  solui::Sel sel;
  bool live = false;      // a deal was made
  bool choosing = false;  // the New game card is up
  bool counted = false;   // this deal is in solPlayed (from its first move)
  bool stuck = false;     // no move left that gets anywhere
  bool newBest = false;   // the win just made took the fewest moves yet
};
static SolitaireState *solitaireS = nullptr;
// Made on first use. No function takes or returns SolitaireState: the sketch's
// generated prototypes come before this tab, so they can't name its types.
static void solitaireEnsure() {
  if (solitaireS) return;
  void *p = heap_caps_malloc(sizeof(SolitaireState), MALLOC_CAP_SPIRAM);
  solitaireS = p ? new (p) SolitaireState() : new SolitaireState();
}

static void solDeal(bool three) {
  SolitaireState &s = *solitaireS;
  s.g.deal(demoMode ? SOL_DEMO_STUCK_SEED : (esp_random() | 1), three);
  s.sel.clear();
  s.live = true, s.choosing = false, s.counted = false, s.newBest = false;
  s.stuck = solui::stuck(s.g);
}

static void solDemo() {
  SolitaireState &s = *solitaireS;
  s.g.deal(SOL_DEMO_SEED, false);
  s.sel.clear();
  for (int k = 0; k < SOL_DEMO_STEPS; k++) solui::playHint(s.g, s.sel);
  s.sel.clear();
  s.live = true, s.choosing = false, s.counted = true, s.newBest = false;
  s.stuck = solui::stuck(s.g);
}

static void solitaireOpen() {
  solitaireEnsure();
  solitaireS->choosing = false;
  solitaireS->sel.clear();
  if (demoMode) solDemo();
  else if (!solitaireS->live) solDeal(false);
}

// ---- geometry

static int solCardW() { return TALL ? 62 : 64; }
static int solCardH() { return TALL ? 86 : 88; }
static int solTop() { return TALL ? gTop() + 8 : BODY_Y + 8; }  // the stock's top
// Where pile `p`'s first card goes (for the waste, its first shown card).
static void solPileXY(int p, int &x, int &y) {
  int w = solCardW(), h = solCardH();
  if (TALL) {
    int pitch = 66, x0 = (W - (6 * pitch + w)) / 2;
    if (p == solitaire::kStockPile) x = x0, y = solTop();
    else if (p == solitaire::kWastePile) x = x0 + pitch, y = solTop();
    else if (solui::isFoundation(p)) x = x0 + (3 + p - solitaire::kFirstFoundation) * pitch, y = solTop();
    else x = x0 + (p - solitaire::kFirstTableau) * pitch, y = solTop() + h + 14;
  } else {
    int pitch = 70, sx = 144;
    if (p == solitaire::kStockPile) x = sx, y = solTop();
    else if (p == solitaire::kWastePile) x = sx, y = solTop() + h + 12;
    else if (solui::isFoundation(p)) x = W - 16 - w, y = solTop() + (p - solitaire::kFirstFoundation) * (h + 8);
    else x = sx + pitch + 8 + (p - solitaire::kFirstTableau) * pitch, y = solTop();
  }
}

// ---- pictures

// A suit, about 2r across, around cx, cy. Red suits (diamonds, hearts) in outline, black solid.
static void solSuit(int suit, int cx, int cy, int r, uint16_t fg, uint16_t bg) {
  auto shape = [&](int i, uint16_t c) {
    switch (suit) {
      case 1:  // diamonds
        canvas->fillTriangle(cx, cy - r + i * 3 / 2, cx - r * 3 / 4 + i, cy, cx + r * 3 / 4 - i, cy, c);
        canvas->fillTriangle(cx, cy + r - i * 3 / 2, cx - r * 3 / 4 + i, cy, cx + r * 3 / 4 - i, cy, c);
        break;
      case 3: {  // hearts
        int rr = r / 2 - i;
        canvas->fillCircle(cx - r / 2, cy - r / 4, rr, c);
        canvas->fillCircle(cx + r / 2, cy - r / 4, rr, c);
        canvas->fillTriangle(cx - r + i * 2, cy - r / 8, cx + r - i * 2, cy - r / 8, cx, cy + r - i * 2, c);
        break;
      }
      case 2:  // spades
        canvas->fillCircle(cx - r / 2, cy + r / 5, r / 2, c);
        canvas->fillCircle(cx + r / 2, cy + r / 5, r / 2, c);
        canvas->fillTriangle(cx - r, cy + r / 8, cx + r, cy + r / 8, cx, cy - r, c);
        canvas->fillTriangle(cx, cy + r / 4, cx - r / 3, cy + r, cx + r / 3, cy + r, c);
        break;
      default:  // clubs
        canvas->fillCircle(cx, cy - r * 45 / 100, r * 42 / 100, c);
        canvas->fillCircle(cx - r * 48 / 100, cy + r * 12 / 100, r * 42 / 100, c);
        canvas->fillCircle(cx + r * 48 / 100, cy + r * 12 / 100, r * 42 / 100, c);
        canvas->fillCircle(cx, cy, r / 4, c);
        canvas->fillTriangle(cx, cy, cx - r / 3, cy + r, cx + r / 3, cy + r, c);
        break;
    }
  };
  shape(0, fg);
  if (suit == 1 || suit == 3) shape(max(2, r / 4), bg);
}

static void solBack(int x, int y, int w, int h) {
  canvas->fillRoundRect(x, y, w, h, 6, PAPER);
  for (int k = 0; k < 2; k++) canvas->drawRoundRect(x + k, y + k, w - 2 * k, h - 2 * k, 6 - k, INK);
  gHatch(x + 5, y + 5, w - 10, h - 10, 5);
}

// A face-up card. `whole`: the card on top, drawn with its big suit; `small`: the strips are
// narrow (sideways), so a smaller corner. `picked`: picked up, drawn black.
static void solFace(int x, int y, int w, int h, uint8_t card, bool whole, bool small, bool picked) {
  uint16_t fg = picked ? PAPER : INK, bg = picked ? INK : PAPER;
  canvas->fillRoundRect(x, y, w, h, 6, bg);
  for (int k = 0; k < 2; k++) canvas->drawRoundRect(x + k, y + k, w - 2 * k, h - 2 * k, 6 - k, INK);
  int suit = (int)cards::suitOf(card);
  const GFXfont *f = small ? &FreeSansBold9pt7b : &FreeSansBold12pt7b;
  canvas->setTextColor(fg);
  text(x + 5, y + (small ? 16 : 20), f, cards::rankLabel(cards::rankOf(card)));
  canvas->setTextColor(INK);
  solSuit(suit, x + w - 13, y + (small ? 10 : 12), small ? 6 : 8, fg, bg);
  if (whole) solSuit(suit, x + w / 2, y + h * 62 / 100, w * 24 / 100, fg, bg);
}

static void solPlace(int x, int y, int w, int h, const char *mark) {
  canvas->drawRoundRect(x, y, w, h, 6, INK);
  canvas->drawRoundRect(x + 3, y + 3, w - 6, h - 6, 4, INK);
  if (mark) textCentre(x + w / 2, y + h / 2 + 8, &FreeSans12pt7b, mark);
}

static void solitaireIcon(int cx, int cy, int s, int t) {
  int cw = s * 56 / 100, ch = s * 76 / 100;
  int x0 = cx - s / 2, y0 = cy - ch / 2 - s / 12, x1 = cx + s / 2 - cw, y1 = cy - ch / 2 + s / 12;
  canvas->fillRoundRect(x0, y0, cw, ch, 5, PAPER);
  for (int k = 0; k < t; k++) canvas->drawRoundRect(x0 + k, y0 + k, cw - 2 * k, ch - 2 * k, 5, INK);
  gHatch(x0 + t + 2, y0 + t + 2, cw - 2 * t - 4, ch - 2 * t - 4, 5);
  canvas->fillRoundRect(x1, y1, cw, ch, 5, PAPER);
  for (int k = 0; k < t; k++) canvas->drawRoundRect(x1 + k, y1 + k, cw - 2 * k, ch - 2 * k, 5, INK);
  solSuit(2, x1 + cw / 2, y1 + ch / 2 + s / 20, cw * 30 / 100, INK, PAPER);
  if (s >= 40) text(x1 + t + 3, y1 + t + 13, &FreeSansBold9pt7b, "A");
}

// ---- the screen

static bool solPicked(int p, int i) { return solitaireS->sel.pile == p && i >= solitaireS->sel.index; }

static void solDrawPiles(bool targets) {
  using namespace solitaire;
  SolitaireState &s = *solitaireS;
  const Game &g = s.g;
  int w = solCardW(), h = solCardH(), x, y;
  // the stock: a card's back with how many are left, or its place (a ring: turn the waste over)
  solPileXY(kStockPile, x, y);
  const Pile &st = g.pile(kStockPile);
  if (!st.empty()) {
    solBack(x, y, w, h);
    char n[12];
    snprintf(n, sizeof n, "%d", st.count);
    int nw = textW(&FreeSansBold9pt7b, n) + 10;
    canvas->fillRoundRect(x + (w - nw) / 2, y + h / 2 - 12, nw, 24, 6, PAPER);
    textCentre(x + w / 2, y + h / 2 + 6, &FreeSansBold9pt7b, n);
  } else {
    solPlace(x, y, w, h, nullptr);
    if (!g.pile(kWastePile).empty()) ring(x + w / 2, y + h / 2, w / 4, 3), canvas->fillTriangle(x + w / 2 + w / 4 - 7, y + h / 2 - 2, x + w / 2 + w / 4 + 6, y + h / 2 - 2, x + w / 2 + w / 4, y + h / 2 + 7, INK);
  }
  if (targets) gTarget(x, y, w, h, solui::cardId(kStockPile, solui::kPlace));
  // the waste: its top card (three fanned when drawing three)
  solPileXY(kWastePile, x, y);
  const Pile &wa = g.pile(kWastePile);
  if (wa.empty()) solPlace(x, y, w, h, nullptr);
  int shown = min((int)wa.count, g.drawThree() ? 3 : 1);
  for (int k = 0; k < shown; k++) {
    int i = wa.count - shown + k, cx = x + (TALL ? k * 20 : 0), cy = y + (TALL ? 0 : k * 24);
    solFace(cx, cy, w, h, wa.cards[i], k == shown - 1, false, solPicked(kWastePile, i));
    if (k == shown - 1 && targets) gTarget(cx, cy, w, h, solui::cardId(kWastePile, i));
  }
  // the foundations
  for (int f = kFirstFoundation; f < kFirstFoundation + kFoundationPiles; f++) {
    solPileXY(f, x, y);
    const Pile &fp = g.pile(f);
    if (fp.empty()) solPlace(x, y, w, h, "A");
    else solFace(x, y, w, h, fp.top(), true, false, solPicked(f, fp.count - 1));
    if (targets) gTarget(x, y, w, h, solui::cardId(f, fp.empty() ? solui::kPlace : fp.count - 1));
  }
  // the columns, fanned to fit: face-down cards close, face-up ones far enough apart to read
  int room = (TALL ? H - 8 : H - 6) - (TALL ? solTop() + h + 14 : solTop());
  for (int t = kFirstTableau; t < kPileCount; t++) {
    solPileXY(t, x, y);
    const Pile &p = g.pile(t);
    if (p.empty()) {
      solPlace(x, y, w, h, "K");
      if (targets) gTarget(x, y, w, h, solui::cardId(t, solui::kPlace));
      continue;
    }
    int downs = 0;
    while (downs < p.count && !isFaceUp(p.cards[downs])) downs++;
    int dStep, uStep;
    if (TALL) solui::fan(downs, p.count - downs, h, room, 14, 32, 6, 18, dStep, uStep);
    else solui::fan(downs, p.count - downs, h, room, 10, 26, 5, 16, dStep, uStep);
    bool small = uStep < 22;
    int cy = y;
    for (int i = 0; i < p.count; i++) {
      bool last = i == p.count - 1;
      if (i < downs) {
        solBack(x, cy, w, h);
        if (targets && s.sel.any() && i == downs - 1) gTarget(x, y, w, cy - y + dStep, solui::cardId(t, 0));  // as a place for the picked-up card
        cy += last ? 0 : dStep;
        continue;
      }
      solFace(x, cy, w, h, p.cards[i], last, small, solPicked(t, i));
      if (targets) gTarget(x, cy, w, last ? h : uStep, solui::cardId(t, i));
      cy += uStep;
    }
  }
}

static String solMovesText() { return TP(S_SOL_MOVES, solitaireS->g.moveCount()); }

// The card over the board: a win (how it went) or New game; draw 1, draw 3, and Cancel.
static void solCard() {
  SolitaireState &s = *solitaireS;
  GameProgress &p = gameProgress();
  bool won = s.g.won();
  int bw = min(W - 48, 420), bh = 250, bx = (W - bw) / 2, by = BODY_Y + (BODY_H - bh) / 2;
  canvas->fillRoundRect(bx - 6, by - 6, bw + 12, bh + 12, 22, PAPER);
  for (int k = 0; k < 3; k++) canvas->drawRoundRect(bx + k, by + k, bw - 2 * k, bh - 2 * k, 18 - k, INK);
  int y = by + 46;
  textCentre(W / 2, y, &FreeSansBold18pt7b, T(won ? S_SOL_WON : S_G_NEW_GAME));
  if (won) {
    textCentre(W / 2, y += 40, &FreeSans12pt7b, TP(S_SOL_WON_MOVES, s.g.moveCount()).c_str());
    String best = s.newBest ? String(T(S_SOL_NEW_BEST)) : TP(S_SOL_BEST, p.solBest);
    textCentre(W / 2, y += 32, &FreeSans12pt7b, fitted(&FreeSans12pt7b, best, bw - 40).c_str());
  }
  y += 26;
  int half = (bw - 48 - 16) / 2;
  for (int k = 0; k < 2; k++) {
    int x = bx + 24 + k * (half + 16);
    gButton(x, y, half, 52, T(k ? S_SOL_DRAW3 : S_SOL_DRAW1), k == (int)s.g.drawThree(), SOL_DRAW + k);
    textCentre(x + half / 2, y + 76, &FreeSans9pt7b, fitted(&FreeSans9pt7b, T(k ? S_SOL_DRAW3_LINE : S_SOL_DRAW1_LINE), half).c_str());
  }
  if (!won) gButton(bx + 24, by + bh - 62, bw - 48, 46, T(S_G_CANCEL), false, ID_CANCEL);
}

static void drawSolitaire() {
  solitaireEnsure();
  if (!solitaireS->live) solDeal(false);
  SolitaireState &s = *solitaireS;
  bool won = s.g.won(), card = s.choosing || won, targets = !card;
  const GFXfont *f9 = &FreeSansBold9pt7b;
  if (TALL) {  // Back, the moves (or "no moves left"), Undo, New game
    int y = BODY_Y + 10;
    gButton(16, y, 100, 48, T(S_G_BACK), false, ID_BACK);
    if (targets) {
      gButton(W - 16 - 124 - 10 - 96, y, 96, 48, T(S_SOL_UNDO), false, SOL_UNDO, f9);
      gButton(W - 16 - 124, y, 124, 48, T(S_G_NEW_GAME), s.stuck, ID_NEW, f9);
    }
    int mid = (116 + W - 16 - 124 - 10 - 96) / 2, mw = W - 16 - 124 - 10 - 96 - 116 - 12;
    if (s.stuck) {  // in place of the moves: "No moves left", on two lines if it needs them
      gCentred(mid, wrapped(f9, T(S_SOL_STUCK), mw).size() > 1 ? y + 20 : y + 31, f9, T(S_SOL_STUCK), mw, 22, 2);
    } else {
      const GFXfont *mf = textW(&FreeSansBold12pt7b, solMovesText().c_str()) <= mw ? &FreeSansBold12pt7b : f9;
      textCentre(mid, y + 20, mf, fitted(mf, solMovesText(), mw).c_str());
      textCentre(mid, y + 44, &FreeSans9pt7b, fitted(&FreeSans9pt7b, T(s.g.drawThree() ? S_SOL_DRAW3 : S_SOL_DRAW1), mw).c_str());
    }
  } else {  // the panel on the left: Back, Undo, New game; the moves; what's happening
    int x = 16, pw = 116, y = BODY_Y + 10;
    gButton(x, y, pw, 48, T(S_G_BACK), false, ID_BACK);
    if (targets) {
      gButton(x, y + 60, pw, 48, T(S_SOL_UNDO), false, SOL_UNDO);
      gButton(x, y + 120, pw, 48, T(S_G_NEW_GAME), s.stuck, ID_NEW, f9);
    }
    gLeft(x, y + 200, &FreeSansBold12pt7b, solMovesText(), pw, 24, 2);
    text(x, y + 252, &FreeSans9pt7b, T(s.g.drawThree() ? S_SOL_DRAW3 : S_SOL_DRAW1));
    StrId say = s.stuck ? S_SOL_STUCK_HINT : s.sel.any() ? S_SOL_WHERE : S_SOL_HOW;
    int sy = y + 290;
    if (s.stuck) sy = gLeft(x, sy, f9, T(S_SOL_STUCK), pw, 20, 2) + 4;
    gLeft(x, sy, &FreeSans9pt7b, T(say), pw, 20, s.stuck ? 4 : 6);
  }
  solDrawPiles(targets);
  if (card) solCard();
}

// ---- taps

static void solChanged() {
  SolitaireState &s = *solitaireS;
  GameProgress &p = gameProgress();
  if (!s.counted && s.g.moveCount() > 0) s.counted = true, p.solPlayed++, gamesChanged();
  if (solui::finish(s.g, s.scratch) || s.g.won()) {
    s.sel.clear();
    p.solWon++;
    s.newBest = !p.solBest || s.g.moveCount() < p.solBest;
    if (s.newBest) p.solBest = s.g.moveCount();
    gamesChanged();
    gs->gFocusId = -1;
    beep(2400, 120);
    return render(true);
  }
  s.stuck = solui::stuck(s.g);
  beep(2600, 15);
  render(false);
}

static void actSolitaire(int id) {
  solitaireEnsure();
  SolitaireState &s = *solitaireS;
  bool card = s.choosing || s.g.won();
  if (id == ID_BACK) {
    if (s.choosing) return s.choosing = false, gs->gFocusId = -1, render(false);
    s.sel.clear();
    return gs->gGame = -1, gs->gFocusId = -1, render(false);
  }
  if (id == ID_NEW) return s.choosing = true, s.sel.clear(), gs->gFocusId = -1, render(false);
  if (id == ID_CANCEL) {
    if (!s.choosing) return;
    return s.choosing = false, gs->gFocusId = -1, render(false);
  }
  if (id == SOL_DRAW || id == SOL_DRAW + 1) return solDeal(id == SOL_DRAW + 1), gs->gFocusId = -1, render(true);
  if (card) return;
  if (id == SOL_UNDO) {
    s.sel.clear();
    if (!s.g.canUndo()) return render(false);
    s.g.undo();
    s.stuck = solui::stuck(s.g);
    return render(false);
  }
  if (id < 0 || id >= solitaire::kPileCount * solui::kPer) return;
  solui::Result r = solui::tap(s.g, s.sel, solui::idPile(id), solui::idIndex(id));
  if (r == solui::MOVED || r == solui::DREW) return solChanged();
  if (r == solui::NOTHING) return beep(700, 30);
  render(false);  // picked up or put down
}

// OK with nothing ringed: on the card, the same kind of deal again; else turn the stock.
static bool solitaireOk() {
  solitaireEnsure();
  SolitaireState &s = *solitaireS;
  if (s.choosing || s.g.won()) {
    actSolitaire(SOL_DRAW + (s.g.drawThree() ? 1 : 0));
    return true;
  }
  actSolitaire(solui::cardId(solitaire::kStockPile, solui::kPlace));
  return true;
}

static String solitaireLine() {
  solitaireEnsure();
  SolitaireState &s = *solitaireS;
  GameProgress &p = gameProgress();
  if (s.live && s.g.moveCount() > 0 && !s.g.won()) return TP(S_SOL_L_ON, s.g.moveCount());
  if (p.solWon) return TF(S_SOL_L_WON, p.solWon, p.solPlayed);
  return T(S_SOL_L_HINT);
}

static String solCardName(uint8_t c) {
  if (c == cards::kNoCard) return "-";
  if (!cards::isFaceUp(c)) return "##";
  static const char SUITS[] = "CDSH";
  return String(cards::rankLabel(cards::rankOf(c))) + SUITS[(int)cards::suitOf(c)];
}

static void solitairePeek(String &out) {
  using namespace solitaire;
  solitaireEnsure();
  SolitaireState &s = *solitaireS;
  const Game &g = s.g;
  const char *state = g.won() ? "won" : s.stuck ? "stuck" : "playing";
  out += "solitaire draw " + String(g.drawThree() ? 3 : 1) + ", moves " + g.moveCount() + ", state " + state + ", card " +
         (g.won() ? "won" : s.choosing ? "new" : "no") + ", sel " + (s.sel.any() ? solui::cardId(s.sel.pile, s.sel.index) : -1);
  out += ", stock " + String(g.pile(kStockPile).count) + ", waste " + g.pile(kWastePile).count + " top " + solCardName(g.pile(kWastePile).top());
  out += ", found";
  for (int f = kFirstFoundation; f < kFirstFoundation + kFoundationPiles; f++) out += " " + String(f) + ":" + solCardName(g.pile(f).top()) + "/" + g.pile(f).count;
  out += ", cols";
  for (int t = kFirstTableau; t < kPileCount; t++) {
    out += (t == kFirstTableau ? " " : " | ") + String(t) + ":";
    for (int i = 0; i < g.pile(t).count; i++) out += (i ? " " : "") + solCardName(g.pile(t).cards[i]);
  }
  int from, index, to;
  if (g.won() || solui::hint(g, from, index, to) < 0) out += ", hint none";
  else if (from == kStockPile) out += ", hint 29";
  else out += ", hint " + String(solui::cardId(from, index)) + " " + solui::cardId(to, solui::kPlace);
}
