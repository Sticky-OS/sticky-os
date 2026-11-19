// Mines in three sizes: CrossPlay's rules (src/crossplay/minesweeper/MinesweeperCore.h) for a
// board whose size is chosen when it is made. Pure: no screen, no Arduino; tested on the
// computer by firmware/test/games/test_minesgrid.cpp, which also checks that the small board
// (8 x 10, 10 mines: CrossPlay's own) plays byte for byte like CrossPlay's core.
//
// Why not the core itself: its size is three constants (kColumns, kRows, kMines) and it
// mustn't be edited (src/crossplay/README.md), so the medium and large boards need these
// functions with the size in the game. They follow the core line for line: the same random
// numbers (minesweeper::nextRandom), the mines laid after the first tap away from it and its
// neighbours, the flood marked when pushed, chording that only ever calls reveal().
//
// A cell's id is row * cols + col, the board held upright (cols across the short side).
#pragma once
#include <stdint.h>
#include "src/crossplay/minesweeper/MinesweeperCore.h"

namespace minesgrid {

using minesweeper::kFlagged;
using minesweeper::kMine;
using minesweeper::kRevealed;
using minesweeper::Status;

struct Size {
  uint8_t cols, rows, mines;
};
// Small is CrossPlay's board. All three keep cells of 42 px or more sideways and upright
// (games_mines.ino): 8 x 10 at 12.5 %, 9 x 12 at 14.8 %, 10 x 13 at 15.4 % mines.
static const int kSizes = 3;
static const Size kSize[kSizes] = {{8, 10, 10}, {9, 12, 16}, {10, 13, 20}};
static const int kMaxCells = 10 * 13;

struct Game {
  uint8_t cols = 8, rows = 10, mines = 10;
  uint8_t cell[kMaxCells] = {};
  uint32_t rng = 0;
  Status status = Status::Fresh;
};

inline int cells(const Game &g) { return g.cols * g.rows; }
inline bool inside(const Game &g, int c, int r) { return c >= 0 && c < g.cols && r >= 0 && r < g.rows; }
inline uint8_t &at(Game &g, int c, int r) { return g.cell[r * g.cols + c]; }
inline uint8_t at(const Game &g, int c, int r) { return g.cell[r * g.cols + c]; }
inline bool has(const Game &g, int c, int r, uint8_t flag) { return inside(g, c, r) && (at(g, c, r) & flag); }

inline void start(Game &g, int size, uint32_t seed) {
  if (size < 0 || size >= kSizes) size = 0;
  g.cols = kSize[size].cols, g.rows = kSize[size].rows, g.mines = kSize[size].mines;
  for (int i = 0; i < kMaxCells; i++) g.cell[i] = 0;
  g.rng = seed;
  g.status = Status::Fresh;
}

inline int neighbouring(const Game &g, int c, int r, uint8_t flag) {
  int n = 0;
  for (int dc = -1; dc <= 1; ++dc)
    for (int dr = -1; dr <= 1; ++dr)
      if ((dc || dr) && has(g, c + dc, r + dr, flag)) ++n;
  return n;
}
inline int neighbouringMines(const Game &g, int c, int r) { return neighbouring(g, c, r, kMine); }
inline int neighbouringFlags(const Game &g, int c, int r) { return neighbouring(g, c, r, kFlagged); }

inline void layMines(Game &g, int safeC, int safeR) {
  int placed = 0, n = cells(g);
  while (placed < g.mines) {
    uint32_t roll = minesweeper::nextRandom(g.rng) % (uint32_t)n;
    int c = (int)roll % g.cols, r = (int)roll / g.cols;
    if (at(g, c, r) & kMine) continue;
    int dc = c - safeC, dr = r - safeR;
    if (dc >= -1 && dc <= 1 && dr >= -1 && dr <= 1) continue;
    at(g, c, r) |= kMine;
    ++placed;
  }
}

inline int count(const Game &g, uint8_t flag) {
  int n = 0;
  for (int i = 0; i < cells(g); i++) n += (g.cell[i] & flag) != 0;
  return n;
}

inline bool allSafeCellsRevealed(const Game &g) {
  for (int i = 0; i < cells(g); i++)
    if (!(g.cell[i] & kMine) && !(g.cell[i] & kRevealed)) return false;
  return true;
}

inline void floodFrom(Game &g, int c0, int r0) {
  uint8_t queue[kMaxCells][2];
  int head = 0, tail = 0;
  if (at(g, c0, r0) & (kRevealed | kFlagged)) return;
  at(g, c0, r0) |= kRevealed;
  queue[tail][0] = (uint8_t)c0, queue[tail][1] = (uint8_t)r0, ++tail;
  while (head < tail) {
    int c = queue[head][0], r = queue[head][1];
    ++head;
    if (neighbouringMines(g, c, r)) continue;
    for (int dc = -1; dc <= 1; ++dc)
      for (int dr = -1; dr <= 1; ++dr) {
        if (!dc && !dr) continue;
        int nc = c + dc, nr = r + dr;
        if (!inside(g, nc, nr) || (at(g, nc, nr) & (kRevealed | kFlagged))) continue;
        at(g, nc, nr) |= kRevealed;
        queue[tail][0] = (uint8_t)nc, queue[tail][1] = (uint8_t)nr, ++tail;
      }
  }
}

inline bool over(const Game &g) { return g.status == Status::Won || g.status == Status::Lost; }

inline bool canReveal(const Game &g, int c, int r) {
  return inside(g, c, r) && !over(g) && !(at(g, c, r) & (kRevealed | kFlagged));
}

inline bool reveal(Game &g, int c, int r) {
  if (!canReveal(g, c, r)) return false;
  if (g.status == Status::Fresh) layMines(g, c, r), g.status = Status::Playing;
  if (at(g, c, r) & kMine) {
    at(g, c, r) |= kRevealed;
    g.status = Status::Lost;
    return true;
  }
  floodFrom(g, c, r);
  if (allSafeCellsRevealed(g)) g.status = Status::Won;
  return true;
}

inline bool chord(Game &g, int c, int r) {
  if (!inside(g, c, r) || g.status != Status::Playing || !(at(g, c, r) & kRevealed)) return false;
  if (neighbouringFlags(g, c, r) != neighbouringMines(g, c, r)) return false;
  bool changed = false;
  for (int dc = -1; dc <= 1; ++dc)
    for (int dr = -1; dr <= 1; ++dr)
      if ((dc || dr) && reveal(g, c + dc, r + dr)) changed = true;
  return changed;
}

inline bool toggleFlag(Game &g, int c, int r) {
  if (!inside(g, c, r) || over(g) || (at(g, c, r) & kRevealed)) return false;
  at(g, c, r) ^= kFlagged;
  return true;
}

inline int minesRemaining(const Game &g) { return g.mines - count(g, kFlagged); }

// The Dig tool: a covered cell opens, a number that carries its flags chords.
inline bool dig(Game &g, int c, int r) { return reveal(g, c, r) || chord(g, c, r); }

// Whether a tap on a revealed number would chord: some neighbour still covered and unflagged.
inline bool chordable(const Game &g, int c, int r) {
  if (!inside(g, c, r) || !(at(g, c, r) & kRevealed) || (at(g, c, r) & kMine) || !neighbouringMines(g, c, r)) return false;
  for (int dc = -1; dc <= 1; ++dc)
    for (int dr = -1; dr <= 1; ++dr)
      if ((dc || dr) && inside(g, c + dc, r + dr) && !(at(g, c + dc, r + dr) & (kRevealed | kFlagged))) return true;
  return false;
}

// The screenshots' game (demo mode), the same every time: a board of `size` opened in the
// middle, with flags on the first three mines that touch the opening.
static const uint32_t kDemoSeed = 0x5EED0042;
inline void demo(Game &g, int size) {
  start(g, size, kDemoSeed + size);
  reveal(g, g.cols / 2, g.rows / 2);
  int flags = 0;
  for (int i = 0; i < cells(g) && flags < 3; i++) {
    int c = i % g.cols, r = i / g.cols;
    if ((g.cell[i] & kMine) && neighbouring(g, c, r, kRevealed)) g.cell[i] |= kFlagged, flags++;
  }
}

// ---- the clock: seconds played, counted between actions. A gap longer than kGapCapMs
// (the board left on the home screen) counts as kGapCapMs, so time away isn't played time.
static const uint32_t kGapCapMs = 5UL * 60 * 1000;
struct Clock {
  uint32_t elapsedMs = 0, lastMs = 0;
  bool running = false;
};
inline uint32_t clockShown(const Clock &k, uint32_t now) {
  if (!k.running) return k.elapsedMs;
  uint32_t gap = now - k.lastMs;
  return k.elapsedMs + (gap > kGapCapMs ? kGapCapMs : gap);
}
inline void clockTick(Clock &k, uint32_t now) {
  k.elapsedMs = clockShown(k, now);
  k.lastMs = now;
}
inline void clockStart(Clock &k, uint32_t now) { k.elapsedMs = 0, k.lastMs = now, k.running = true; }
inline void clockStop(Clock &k, uint32_t now) { clockTick(k, now), k.running = false; }

}  // namespace minesgrid
