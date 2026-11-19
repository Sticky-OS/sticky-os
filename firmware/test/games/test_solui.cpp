// Solitaire's taps (firmware/sticky-os/solui.h) on CrossPlay's Klondike rules: what a tap
// does, the finish, the hint, "no moves left", the columns' fan, and the two scripted games
// the on-board tests play in demo mode (games_solitaire.ino: SOL_DEMO_*).
#include <cstdio>
#include <vector>

#include "solui.h"

using namespace solui;
using namespace solitaire;
using cards::kAce;
using cards::kKing;

static int checks = 0, failures = 0;
#define CHECK(c)                                                    \
  do {                                                              \
    checks++;                                                       \
    if (!(c)) failures++, std::printf("FAIL line %d: %s\n", __LINE__, #c); \
  } while (0)

// A board made card by card: `piles` lists what goes where (face up unless marked down);
// everything else goes face down into the stock (then the last columns, when it's full).
struct Place {
  int pile;
  uint8_t card;
  bool up;
};
static bool build(Game &g, const std::vector<Place> &places) {
  Game::Save s;
  g.deal(1, false);
  g.save(s);
  for (int i = 0; i < kPileCount; i++) s.counts[i] = 0;
  bool used[kDeck] = {};
  for (const Place &p : places) {
    s.piles[p.pile][s.counts[p.pile]++] = p.up ? faceUp(p.card) : faceDown(p.card);
    used[(int)suitOf(p.card) * kRanks + rankOf(p.card)] = true;
  }
  int spill = kStockPile;
  for (int i = 0; i < kDeck; i++) {
    if (used[i]) continue;
    while (s.counts[spill] == kPileCapacity) spill = spill == kStockPile ? kPileCount - 1 : spill - 1;
    s.piles[spill][s.counts[spill]++] = faceDown(makeCard((Suit)(i / kRanks), i % kRanks));
  }
  return g.restore(s);
}
static uint8_t C(Suit s, int rank) { return makeCard(s, rank); }
static const int T1 = kFirstTableau, T2 = kFirstTableau + 1, T3 = kFirstTableau + 2, T4 = kFirstTableau + 3;

static void testTaps() {
  Game g;
  Sel s;
  // An ace goes up by itself.
  CHECK(build(g, {{T1, C(Suit::Hearts, 5), false}, {T1, C(Suit::Spades, kAce), true}}));
  CHECK(tap(g, s, T1, 1) == MOVED);
  CHECK(g.pile(kFirstFoundation).count == 1);
  CHECK(!s.any());
  CHECK(solitaire::isFaceUp(g.pile(T1).top()));  // the card under it turned over

  // A card with one column to go to goes there; with two, it's picked up.
  CHECK(build(g, {{T1, C(Suit::Hearts, 6), true}, {T2, C(Suit::Spades, 7), true}, {T3, C(Suit::Clubs, 9), true}}));
  CHECK(tap(g, s, T1, 0) == MOVED);
  CHECK(g.pile(T2).count == 2 && g.pile(T1).count == 0);
  CHECK(build(g, {{T1, C(Suit::Hearts, 6), true}, {T2, C(Suit::Spades, 7), true}, {T3, C(Suit::Clubs, 7), true}}));
  CHECK(tap(g, s, T1, 0) == SELECTED);
  CHECK(s.pile == T1 && s.index == 0);
  CHECK(tap(g, s, T3, kPlace) == MOVED);  // then the place it goes
  CHECK(g.pile(T3).count == 2 && !s.any());

  // The same card again puts it down; a tap where it can't go picks up the tapped card instead.
  CHECK(build(g, {{T1, C(Suit::Hearts, 6), true}, {T2, C(Suit::Spades, 7), true}, {T3, C(Suit::Clubs, 7), true}, {T4, C(Suit::Diamonds, 2), true}}));
  CHECK(tap(g, s, T1, 0) == SELECTED);
  CHECK(tap(g, s, T1, 0) == DESELECTED);
  CHECK(!s.any());
  CHECK(tap(g, s, T1, 0) == SELECTED);
  CHECK(tap(g, s, T4, 0) == SELECTED);  // the 2 doesn't take the 6: the 2 is picked up
  CHECK(s.pile == T4);
  CHECK(tap(g, s, T4, kPlace) == DESELECTED);

  // A run: tapping its top card takes the run.
  CHECK(build(g, {{T1, C(Suit::Clubs, 3), false}, {T1, C(Suit::Hearts, 8), true}, {T1, C(Suit::Spades, 7), true}, {T2, C(Suit::Spades, 9), true}}));
  CHECK(tap(g, s, T1, 1) == MOVED);
  CHECK(g.pile(T2).count == 3 && g.pile(T1).count == 1 && solitaire::isFaceUp(g.pile(T1).top()));

  // A king at a column's bottom doesn't count an empty column as a place; elsewhere it does, once.
  CHECK(build(g, {{T1, C(Suit::Hearts, kKing), true}, {T2, C(Suit::Clubs, 4), false}, {T2, C(Suit::Spades, kKing), true}}));
  int d[7];
  CHECK(destinations(g, T1, 0, d, 7) == 0);
  CHECK(destinations(g, T2, 1, d, 7) == 1);
  CHECK(tap(g, s, T2, 1) == MOVED);
  CHECK(g.pile(T2).count == 1);

  // The stock draws, the waste's tap is its top card, a foundation's card is only picked up.
  CHECK(build(g, {{kWastePile, C(Suit::Clubs, 5), true}, {kWastePile, C(Suit::Diamonds, kAce), true}, {kFirstFoundation, C(Suit::Spades, kAce), true}}));
  CHECK(tap(g, s, kWastePile, 0) == MOVED);  // the ace on top goes up, though the 5 was tapped
  CHECK(g.pile(kWastePile).count == 1 && g.pile(kFirstFoundation + 1).count == 1);
  int stock = g.pile(kStockPile).count;
  CHECK(tap(g, s, kStockPile, kPlace) == DREW);
  CHECK(g.pile(kStockPile).count == stock - 1 && g.pile(kWastePile).count == 2);
  CHECK(tap(g, s, kFirstFoundation, 0) == SELECTED);
  CHECK(tap(g, s, kFirstFoundation, 0) == DESELECTED);
  // A face-down card, an empty place, nothing selected: nothing.
  CHECK(build(g, {{T1, C(Suit::Clubs, 5), false}, {T1, C(Suit::Clubs, 6), true}}));
  CHECK(tap(g, s, T1, 0) == NOTHING);
  CHECK(tap(g, s, T2, kPlace) == NOTHING);
}

static int downs(const Game &g) {
  int n = 0;
  for (int t = kFirstTableau; t < kPileCount; t++)
    for (int i = 0; i < g.pile(t).count; i++) n += !isFaceUp(g.pile(t).cards[i]);
  return n;
}

// Every deal played by the hint, both draw sizes: the hint always changes something, the
// finish wins whenever it says so (and only once nothing is hidden), "no moves left" is
// only said of an unwon game, and the cards are all there.
static void testSoak() {
  static Game g, scratch;
  int won = 0, stuckGames = 0;
  for (uint32_t seed = 1; seed <= 600; seed++) {
    for (int three = 0; three < 2; three++) {
      g.deal(seed, three);
      Sel s;
      for (int step = 0; step < 800; step++) {
        bool decided = tableauAllUp(g);
        if (finish(g, scratch)) {
          CHECK(decided && g.won());
          won++;
          break;
        }
        if (!three) CHECK(!decided || g.won());  // draw one: nothing hidden always finishes
        if (stuck(g)) {
          stuckGames++;
          CHECK(!g.won());
          break;
        }
        int moves = g.moveCount();
        CHECK(playHint(g, s));
        CHECK(g.moveCount() == moves + 1);
        int total = 0;
        for (int p = 0; p < kPileCount; p++) total += g.pile(p).count;
        CHECK(total == kDeck);
      }
    }
  }
  std::printf("  soak: %d of 1200 deals won by the hint, %d with no moves left\n", won, stuckGames);
  CHECK(won > 250);
}

static void testFan() {
  // The longest column there can be (6 down, 13 up) fits both screens' rooms.
  int d, u;
  fan(6, 13, 86, 576, 14, 30, 6, 18, d, u);
  CHECK(6 * d + 12 * u + 86 <= 576 && d <= 14 && u <= 30);
  fan(6, 13, 88, 418, 12, 24, 5, 16, d, u);
  CHECK(6 * d + 12 * u + 88 <= 418);
  fan(2, 3, 86, 576, 14, 30, 6, 18, d, u);
  CHECK(d == 14 && u == 30);  // short columns keep the full spacing
  fan(3, 0, 86, 100, 14, 30, 6, 18, d, u);
  CHECK(2 * d + 86 <= 100 || d == 6);
}

// The scripts games_solitaire.ino's demo plays (the on-board tests use them): deal
// SOL_DEMO_SEED, play SOL_DEMO_STEPS hints, then these taps win; SOL_DEMO_STUCK_SEED and its
// taps end with no moves left.
static void testDemoScripts() {
  static Game g, scratch;
  Sel s;
  g.deal(123, false);
  for (int k = 0; k < 37; k++) CHECK(playHint(g, s));
  CHECK(downs(g) == 11 && !g.won());
  const int win[] = {29, 38, 334, 366, 365, 274, 333, 332, 364, 363, 362, 331, 361, 273, 272, 271, 242, 241};
  bool done = false;
  for (int id : win) {
    CHECK(!done);
    CHECK(tap(g, s, idPile(id), idIndex(id)) != NOTHING);
    done = finish(g, scratch);
  }
  CHECK(done && g.won());
  std::printf("  demo deal won in %d moves\n", g.moveCount());

  g.deal(1725, false);
  s.clear();
  const int lose[] = {242, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 39, 29, 39, 367, 29, 29, 29, 29, 29, 29, 29, 29, 46, 29, 29, 29, 29, 49};
  for (int id : lose) {
    CHECK(!stuck(g));
    CHECK(tap(g, s, idPile(id), idIndex(id)) != NOTHING);
    CHECK(!finish(g, scratch));
  }
  CHECK(stuck(g));
}

int main() {
  testTaps();
  testFan();
  testDemoScripts();
  testSoak();
  std::printf("solui: %d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
