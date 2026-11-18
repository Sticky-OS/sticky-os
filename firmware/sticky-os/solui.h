// Solitaire's taps and helpers around CrossPlay's Klondike rules
// (src/crossplay/solitaire/SolitaireCore.h): what a tap on a card does, the finish once nothing
// is hidden, a hint, "no moves left", and how far apart a column's cards can be drawn. Pure: no
// screen, no Arduino; games_solitaire.ino draws, firmware/test/games/test_solui.cpp tests.
//
// Ids (the screen's targets, and `game pick <id>`): pile * 30 + the card's index in the pile
// (0 at the bottom), or pile * 30 + 29 for the pile's place (an empty column, a pile as a
// destination). Piles are the core's: 0 stock, 1 waste, 2-5 foundations, 6-12 the columns.
#pragma once
#include <stdint.h>
#include "src/crossplay/solitaire/SolitaireCore.h"

namespace solui {

using solitaire::Game;
using solitaire::kFirstFoundation;
using solitaire::kFirstTableau;
using solitaire::kFoundationPiles;
using solitaire::kPileCount;
using solitaire::kStockPile;
using solitaire::kTableauPiles;
using solitaire::kWastePile;

static const int kPer = 30, kPlace = 29;
inline int cardId(int pile, int index) { return pile * kPer + index; }
inline int idPile(int id) { return id / kPer; }
inline int idIndex(int id) { return id % kPer; }
inline bool isTableau(int p) { return p >= kFirstTableau && p < kPileCount; }
inline bool isFoundation(int p) { return p >= kFirstFoundation && p < kFirstFoundation + kFoundationPiles; }

struct Sel {
  int pile = -1, index = -1;
  bool any() const { return pile >= 0; }
  void clear() { pile = index = -1; }
};

enum Result { NOTHING, SELECTED, DESELECTED, MOVED, DREW };

// The columns a run (pile, index up) could go to: every empty column counts once (they're
// all the same), and a king that is already a column's bottom card doesn't count an empty one.
inline int destinations(const Game &g, int pile, int index, int *out, int max) {
  int n = 0, count = g.pile(pile).count - index;
  bool emptySeen = false;
  for (int t = kFirstTableau; t < kPileCount && n < max; t++) {
    if (t == pile || !g.canMove(pile, t, count)) continue;
    if (g.pile(t).empty()) {
      if (emptySeen || (isTableau(pile) && index == 0)) continue;
      emptySeen = true;
    }
    out[n++] = t;
  }
  return n;
}

// A tap on a card (index 0..) or a pile's place (kPlace). With a card picked up, a tap on
// another pile puts it there if it may go. With none: the stock draws; a card or run with an
// obvious place (its foundation, or the only column it fits) goes there; else it's picked up.
// A foundation's card is only ever picked up (taking one down is rarely meant).
inline Result tap(Game &g, Sel &s, int pile, int index) {
  if (pile < 0 || pile >= kPileCount) return NOTHING;
  if (pile == kStockPile) {
    s.clear();
    return g.drawFromStock() ? DREW : NOTHING;
  }
  const solitaire::Pile &p = g.pile(pile);
  if (s.any()) {
    if (pile != s.pile) {
      int count = g.pile(s.pile).count - s.index;
      int from = s.pile;
      s.clear();
      if (g.move(from, pile, count)) return MOVED;
      // Not there: the tapped card is picked up instead, if it can move at all.
      if (index != kPlace && index < p.count && g.runLength(pile, index) > 0 && pile != kWastePile) {
        s.pile = pile, s.index = index;
        return SELECTED;
      }
      return DESELECTED;
    }
    // The same pile: another card of it picks up from there; the same card puts it down.
    if (index != s.index && index != kPlace && index < p.count && g.runLength(pile, index) > 0) {
      s.index = index;
      return SELECTED;
    }
    s.clear();
    return DESELECTED;
  }
  if (index == kPlace || index >= p.count) return NOTHING;
  if (pile == kWastePile) index = p.count - 1;  // the waste: its top card, wherever the finger lands
  int run = g.runLength(pile, index);
  if (run <= 0) return NOTHING;
  if (isFoundation(pile)) {
    s.pile = pile, s.index = index;
    return SELECTED;
  }
  if (run == 1) {
    int f = g.foundationFor(pile);
    if (f >= 0 && g.move(pile, f, 1)) return MOVED;
  }
  int to[kTableauPiles];
  if (destinations(g, pile, index, to, kTableauPiles) == 1 && g.move(pile, to[0], run)) return MOVED;
  s.pile = pile, s.index = index;
  return SELECTED;
}

inline bool tableauAllUp(const Game &g) {
  for (int t = kFirstTableau; t < kPileCount; t++)
    for (int i = 0; i < g.pile(t).count; i++)
      if (!solitaire::isFaceUp(g.pile(t).cards[i])) return false;
  return true;
}

// Once every column card is face up the deal is decided: plays it out, foundation moves and
// stock turns, on `scratch` first; true (and g the won game) only when that wins.
inline bool finish(Game &g, Game &scratch) {
  if (g.won() || !tableauAllUp(g)) return false;
  scratch = g;
  int idle = 0;
  for (int guard = 0; guard < 4000 && !scratch.won(); guard++) {
    if (scratch.autoPlay()) {
      idle = 0;
      continue;
    }
    int left = scratch.pile(kStockPile).count + scratch.pile(kWastePile).count;
    if (!left || ++idle > left + 2 || !scratch.drawFromStock()) break;
  }
  if (!scratch.won()) return false;
  g = scratch;
  return true;
}

// Whether `card` could go anywhere now: a foundation, or a column.
inline bool placeable(const Game &g, uint8_t card) {
  using namespace solitaire;
  int rank = rankOf(card);
  for (int f = kFirstFoundation; f < kFirstFoundation + kFoundationPiles; f++) {
    const Pile &p = g.pile(f);
    if (p.empty() ? rank == 0 : (suitOf(p.top()) == suitOf(card) && rankOf(p.top()) + 1 == rank)) return true;
  }
  for (int t = kFirstTableau; t < kPileCount; t++) {
    const Pile &p = g.pile(t);
    if (p.empty() ? rank == kRanks - 1 : (isFaceUp(p.top()) && isRed(p.top()) != isRed(card) && rankOf(p.top()) == rank + 1)) return true;
  }
  return false;
}

// A move worth making, for the hint and "no moves left". Kinds, best first: 0 to a foundation,
// 1 a column move that turns a card over (or empties a column for a king), 2 the waste's
// card to a column, 3 a column move that frees a card for a foundation or the stock, 4 a stock
// turn that can bring a playable card. -1: none.
inline int hint(const Game &g, int &from, int &index, int &to) {
  using namespace solitaire;
  int bestKind = 99;
  auto offer = [&](int kind, int f, int i, int t) {
    if (kind < bestKind) bestKind = kind, from = f, index = i, to = t;
  };
  // the waste's top card
  const Pile &w = g.pile(kWastePile);
  if (!w.empty()) {
    int f = g.foundationFor(kWastePile);
    if (f >= 0) offer(0, kWastePile, w.count - 1, f);
    int d[kTableauPiles];
    if (destinations(g, kWastePile, w.count - 1, d, kTableauPiles)) offer(2, kWastePile, w.count - 1, d[0]);
  }
  bool waiting = false;      // a stock card that would fit under a run, once the run moves
  bool kingWaiting = false;  // a king not at a column's bottom: an empty column would help
  for (int t = kFirstTableau; t < kPileCount; t++)
    for (int i = 1; i < g.pile(t).count; i++)
      if (isFaceUp(g.pile(t).cards[i]) && rankOf(g.pile(t).cards[i]) == kRanks - 1) kingWaiting = true;
  for (int i = 0; i < g.pile(kStockPile).count; i++)
    if (rankOf(g.pile(kStockPile).cards[i]) == kRanks - 1) kingWaiting = true;
  for (int i = 0; i < w.count; i++)
    if (rankOf(w.cards[i]) == kRanks - 1) kingWaiting = true;
  for (int t = kFirstTableau; t < kPileCount; t++) {
    const Pile &p = g.pile(t);
    if (p.empty()) continue;
    int f = g.foundationFor(t);
    if (f >= 0) offer(0, t, p.count - 1, f);
    for (int i = 0; i < p.count; i++) {
      if (!isFaceUp(p.cards[i]) || g.runLength(t, i) <= 0) continue;
      int d[kTableauPiles], n = destinations(g, t, i, d, kTableauPiles);
      if (!n) continue;
      if (i == 0) {
        if (kingWaiting && rankOf(p.cards[0]) != kRanks - 1) offer(1, t, i, d[0]);
        continue;
      }
      uint8_t under = p.cards[i - 1];
      if (!isFaceUp(under)) {
        offer(1, t, i, d[0]);
        continue;
      }
      // Under it is a face-up card: worth it if that card then goes up, or takes the waste's
      // card. (Only the waste's top: one from deeper in the stock would make two columns
      // swap the run back and forth; that card asks for a stock turn below instead.)
      bool frees = false;
      for (int ff = kFirstFoundation; ff < kFirstFoundation + kFoundationPiles && !frees; ff++) {
        const Pile &fp = g.pile(ff);
        frees = fp.empty() ? rankOf(under) == 0 : (suitOf(fp.top()) == suitOf(under) && rankOf(fp.top()) + 1 == rankOf(under));
      }
      if (!w.empty() && isRed(w.top()) != isRed(under) && rankOf(w.top()) + 1 == rankOf(under)) frees = true;
      if (frees) offer(3, t, i, d[0]);
      else for (int u = 0; u < 2; u++) {  // a card in the stock or the waste that would fit there later
        const Pile &sp = g.pile(u ? kWastePile : kStockPile);
        for (int k = 0; k < sp.count; k++)
          if (isRed(sp.cards[k]) != isRed(under) && rankOf(sp.cards[k]) + 1 == rankOf(under)) waiting = true;
      }
    }
  }
  if (bestKind > 3 && (g.pile(kStockPile).count > 0 || w.count > 1)) {
    bool any = waiting;
    for (int i = 0; i < g.pile(kStockPile).count && !any; i++) any = placeable(g, faceUp(g.pile(kStockPile).cards[i]));
    for (int i = 0; i + 1 < w.count && !any; i++) any = placeable(g, w.cards[i]);  // the top was looked at above
    if (any) offer(4, kStockPile, 0, kStockPile);
  }
  return bestKind == 99 ? -1 : bestKind;
}

// The hint, played as the taps a person would make (the card, then where it goes): true when
// something changed. `taps` gets the ids tapped (up to 2), for the tests' scripts.
inline bool playHint(Game &g, Sel &s, int *taps = nullptr, int *ntaps = nullptr) {
  int f, i, t, n = 0;
  if (ntaps) *ntaps = 0;
  if (hint(g, f, i, t) < 0) return false;
  s.clear();
  auto press = [&](int pile, int index) {
    if (taps) taps[n] = cardId(pile, index);
    n++;
    if (ntaps) *ntaps = n;
    return tap(g, s, pile, index);
  };
  if (f == kStockPile) return press(kStockPile, kPlace) == DREW;
  Result r = press(f, i);
  if (r == SELECTED) r = press(t, kPlace);
  return r == MOVED;
}

// No move left that gets anywhere (an approximation: the board stays playable, Undo works).
inline bool stuck(const Game &g) {
  int f, i, t;
  return !g.won() && hint(g, f, i, t) < 0;
}

// How far apart to draw a column's cards: `downs` face down, `ups` face up, in `room` pixels
// from the column's top to the bottom of its last card. Face-down cards shrink first (to
// minDown), then face-up ones (to minUp); never more than the defaults.
inline void fan(int downs, int ups, int cardH, int room, int down, int up, int minDown, int minUp, int &outDown, int &outUp) {
  outDown = down, outUp = up;
  int gaps = ups > 0 ? ups - 1 : 0;
  if (ups == 0 && downs > 0) downs--;  // the last card is the one drawn whole
  auto need = [&]() { return downs * outDown + gaps * outUp + cardH; };
  while (need() > room && outDown > minDown) outDown--;
  while (need() > room && outUp > minUp) outUp--;
}

}  // namespace solui
