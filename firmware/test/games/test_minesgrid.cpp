// Mines in three sizes (firmware/sticky-os/minesgrid.h): the small board plays exactly like
// CrossPlay's core (src/crossplay/minesweeper/MinesweeperCore.h), every size keeps the core's
// promises (the first tap is safe and opens a flood, perfect play wins, a mine loses), the
// clock, and the demo boards the on-board tests play (games_mines.ino).
#include <cstdio>

#include "minesgrid.h"

static int checks = 0, failures = 0;
#define CHECK(c)                                                    \
  do {                                                              \
    checks++;                                                       \
    if (!(c)) failures++, std::printf("FAIL line %d: %s\n", __LINE__, #c); \
  } while (0)

namespace ms = minesweeper;
namespace mg = minesgrid;

static bool same(const ms::Game &a, const mg::Game &b) {
  if ((int)a.status != (int)b.status || a.rng != b.rng) return false;
  for (int c = 0; c < ms::kColumns; c++)
    for (int r = 0; r < ms::kRows; r++)
      if (a.cell[c][r] != mg::at(b, c, r)) return false;
  return true;
}

// The same seeds and the same taps, anything at all: the same boards, cell for cell.
static void testSmallIsCrossPlays() {
  uint32_t pick = 12345;
  int differ = 0;
  for (uint32_t seed = 1; seed <= 3000; seed++) {
    ms::Game a;
    mg::Game b;
    ms::start(a, seed);
    mg::start(b, 0, seed);
    for (int step = 0; step < 60 && !ms::over(a); step++) {
      uint32_t roll = ms::nextRandom(pick);
      int c = roll % 8, r = (roll >> 8) % 10, what = (roll >> 16) % 5;
      bool x, y;
      if (what == 0) x = ms::toggleFlag(a, c, r), y = mg::toggleFlag(b, c, r);
      else if (what == 1) x = ms::chord(a, c, r), y = mg::chord(b, c, r);
      else x = ms::dig(a, c, r), y = mg::dig(b, c, r);
      if (x != y || !same(a, b) || ms::minesRemaining(a) != mg::minesRemaining(b)) differ++;
    }
  }
  CHECK(differ == 0);
}

static void testEverySize() {
  for (int size = 0; size < mg::kSizes; size++) {
    int wins = 0;
    for (uint32_t seed = 1; seed <= 500; seed++) {
      mg::Game g;
      mg::start(g, size, seed);
      CHECK(g.cols * 42 <= 480 && g.rows >= g.cols);
      int c0 = seed % g.cols, r0 = (seed / 7) % g.rows;
      CHECK(mg::dig(g, c0, r0));
      CHECK(g.status != mg::Status::Lost);
      CHECK(mg::count(g, mg::kMine) == g.mines);
      CHECK(mg::neighbouringMines(g, c0, r0) == 0 && mg::count(g, mg::kRevealed) > 1);
      // No blank revealed cell beside a covered one (the core's flood bug, kept fixed).
      for (int c = 0; c < g.cols; c++)
        for (int r = 0; r < g.rows; r++)
          if ((mg::at(g, c, r) & mg::kRevealed) && !mg::neighbouringMines(g, c, r))
            for (int dc = -1; dc <= 1; dc++)
              for (int dr = -1; dr <= 1; dr++)
                if (mg::inside(g, c + dc, r + dr)) CHECK(mg::at(g, c + dc, r + dr) & mg::kRevealed);
      // Perfect play: every safe cell, with flags on the mines and chords where they help.
      mg::Game lose = g;
      for (int i = 0; i < mg::cells(g); i++)
        if (g.cell[i] & mg::kMine) CHECK(mg::toggleFlag(g, i % g.cols, i / g.cols));
      CHECK(mg::minesRemaining(g) == 0);
      for (int i = 0; i < mg::cells(g) && !mg::over(g); i++)
        if (!(g.cell[i] & mg::kMine) && !(g.cell[i] & mg::kRevealed)) CHECK(mg::dig(g, i % g.cols, i / g.cols));
      CHECK(g.status == mg::Status::Won);
      wins += g.status == mg::Status::Won;
      CHECK(!mg::toggleFlag(g, 0, 0) && !mg::dig(g, 0, 0));  // settled: nothing more
      // A mine loses, and stops everything.
      for (int i = 0; i < mg::cells(lose); i++)
        if (lose.cell[i] & mg::kMine) {
          CHECK(mg::dig(lose, i % lose.cols, i / lose.cols));
          break;
        }
      CHECK(lose.status == mg::Status::Lost);
    }
    CHECK(wins == 500);
  }
  // Chording: a number with its flags opens the rest; a wrong count does nothing.
  mg::Game g;
  mg::start(g, 1, 77);
  CHECK(mg::reveal(g, 4, 6));
  int chorded = 0, refused = 0;
  for (int i = 0; i < mg::cells(g); i++) {
    int c = i % g.cols, r = i / g.cols;
    if (!mg::chordable(g, c, r)) continue;
    mg::Game h = g;
    CHECK(!mg::chord(h, c, r) || mg::neighbouringMines(h, c, r) == 0);
    refused++;
    bool safeLeft = false;  // a covered neighbour that isn't a mine: the chord opens it
    for (int dc = -1; dc <= 1; dc++)
      for (int dr = -1; dr <= 1; dr++) {
        if (mg::has(h, c + dc, r + dr, mg::kMine) && !mg::has(h, c + dc, r + dr, mg::kFlagged)) mg::toggleFlag(h, c + dc, r + dr);
        if (mg::inside(h, c + dc, r + dr) && !(mg::at(h, c + dc, r + dr) & (mg::kMine | mg::kRevealed))) safeLeft = true;
      }
    CHECK(mg::chord(h, c, r) == safeLeft);
    CHECK(h.status != mg::Status::Lost && !mg::chordable(h, c, r));
    chorded++;
  }
  CHECK(chorded > 0 && refused == chorded);
}

static void testClock() {
  mg::Clock k;
  CHECK(mg::clockShown(k, 5000) == 0);
  mg::clockStart(k, 1000);
  CHECK(mg::clockShown(k, 4000) == 3000);
  mg::clockTick(k, 4000);
  CHECK(mg::clockShown(k, 4000 + 60UL * 60 * 1000) == 3000 + mg::kGapCapMs);  // an hour away: five minutes
  mg::clockStop(k, 9000);
  CHECK(k.elapsedMs == 8000 && mg::clockShown(k, 99999) == 8000);
  mg::clockStart(k, 0xFFFFF000u);  // millis() wrapping
  CHECK(mg::clockShown(k, 0x1000) == 0x2000);
}

// The demo boards and the on-board tests' scripts: these picks win; `mine` loses.
static void testDemo() {
  static const int win0[] = {1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 19, 23, 48, 56, 57, 58, 59, 64, 74, 75};
  static const int win1[] = {0,  1,  2,  3,  4,  5,  10, 18, 19, 20, 27, 34, 43, 52, 53, 54, 62, 63, 64, 70, 71, 72,
                             73, 74, 75, 77, 78, 79, 80, 81, 83, 84, 85, 86, 90, 91, 93, 94, 95, 96, 99, 101, 103, 104};
  struct Script {
    const int *picks;
    int n, mine, revealed;
  } scripts[2] = {{win0, sizeof win0 / sizeof *win0, 0, 35}, {win1, sizeof win1 / sizeof *win1, 9, 25}};
  for (int size = 0; size < 2; size++) {
    mg::Game g;
    mg::demo(g, size);
    CHECK(g.status == mg::Status::Playing && mg::count(g, mg::kFlagged) == 3);
    CHECK(mg::count(g, mg::kRevealed) == scripts[size].revealed);
    mg::Game lose = g;
    for (int k = 0; k < scripts[size].n; k++) {
      CHECK(g.status == mg::Status::Playing);
      int id = scripts[size].picks[k];
      CHECK(mg::dig(g, id % g.cols, id / g.cols));
    }
    CHECK(g.status == mg::Status::Won);
    CHECK(mg::dig(lose, scripts[size].mine % lose.cols, scripts[size].mine / lose.cols));
    CHECK(lose.status == mg::Status::Lost);
  }
}

int main() {
  testSmallIsCrossPlays();
  testEverySize();
  testClock();
  testDemo();
  std::printf("minesgrid: %d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
