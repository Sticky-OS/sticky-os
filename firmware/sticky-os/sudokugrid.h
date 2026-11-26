// Sudoku's rules, apart from the screen (games_sudoku.ino draws them): the puzzle of the day,
// made on the board from the date (issue #18), its solver, its grader, the clash check and the Hint button's. Plain
// C++ with no Arduino, so firmware/test/games/test_sudokugrid.cpp runs it on a computer.
//
// The puzzle of the day, for a date (20261122) and a level:
//  1. The random numbers are the games' own (games.cpp: gameSeed / gameRnd, xorshift32), seeded
//     with seed(date, level) = date * 2654435761 ^ the level's salt. The salt carries a version
//     digit ('SDK1'): a changed generator must change it, so puzzles change on purpose only.
//     The caller passes gameRnd in (the host test has a copy), then calls gameSeed(0).
//  2. A whole grid, filled by backtracking with 1-9 tried in a shuffled order.
//  3. Cells taken away in a shuffled order, two at a time opposite each other (the usual
//     symmetric look), each pair only while the puzzle still has exactly one solution (a
//     counter that stops at 2, within a node budget) and the level's techniques still solve it
//     (Easy: singles; Medium: singles and pairs), down to the level's fewest clues.
//  4. A try that ends with too many clues, or that an easier level's techniques could solve
//     (Medium wants a pair, Hard more than pairs), is made again from the next numbers, up to
//     kTries[level] times; then the closest try is taken.
//
// Cells are 0..80, row * 9 + column; a value 0 is empty, else 1..9. Pencil marks are bits 1..9.
#pragma once
#include <stdint.h>
#include <string.h>

namespace sudokugrid {

static const int kCells = 81, kLevels = 3;
static const int kFewest[kLevels] = {36, 30, 26};  // clues: Easy 36-40, Medium 30-34, Hard 26-29
static const int kMost[kLevels] = {40, 34, 29};
static const uint32_t kSalt[kLevels] = {0x53444B31u, 0x53444B32u, 0x53444B33u};  // 'SDK1', 'SDK2', 'SDK3'
// Tries a level may take (a fixed count, never a time limit: every board must make the same
// puzzle). Measured on the board, a try takes about 32 ms (Easy), 42 ms (Medium) and 25 ms (Hard),
// so the worst day stays under the 300 ms the loop can spare.
static const int kTries[kLevels] = {1, 5, 8};
static const uint32_t kNodeBudget = 20000;  // one count of solutions; over it the cell stays

// The techniques a puzzle needs (grade()).
enum Technique { SINGLES = 0, PAIRS = 1, MORE = 2 };

inline uint32_t seed(long date, int level) { return (uint32_t)date * 2654435761u ^ kSalt[level]; }

// gameRnd: lo..hi, both included.
typedef int (*Rand)(int lo, int hi);

inline int rowOf(int i) { return i / 9; }
inline int colOf(int i) { return i % 9; }
inline int boxOf(int i) { return (i / 27) * 3 + (i % 9) / 3; }
inline bool peers(int a, int b) { return a != b && (rowOf(a) == rowOf(b) || colOf(a) == colOf(b) || boxOf(a) == boxOf(b)); }
inline int bits(uint16_t m) { return __builtin_popcount(m); }
// The j-th cell (0..8) of unit k: kind 0 rows, 1 columns, 2 boxes.
inline int unitCell(int kind, int k, int j) { return kind == 0 ? k * 9 + j : kind == 1 ? j * 9 + k : (k / 3) * 27 + (k % 3) * 3 + (j / 3) * 9 + j % 3; }

// ---- what's still allowed: a bit per number used in each row, column and box

struct Used {
  uint16_t row[9], col[9], box[9];
};
inline uint16_t allowed(const Used &u, int i) { return (uint16_t)(~(u.row[rowOf(i)] | u.col[colOf(i)] | u.box[boxOf(i)]) & 0x3FE); }
inline void put(Used &u, int i, int d) {
  uint16_t b = 1u << d;
  u.row[rowOf(i)] |= b, u.col[colOf(i)] |= b, u.box[boxOf(i)] |= b;
}
inline void take(Used &u, int i, int d) {
  uint16_t b = (uint16_t)~(1u << d);
  u.row[rowOf(i)] &= b, u.col[colOf(i)] &= b, u.box[boxOf(i)] &= b;
}
// False when a number is there twice already.
inline bool usedOf(const uint8_t g[kCells], Used &u) {
  memset(&u, 0, sizeof u);
  for (int i = 0; i < kCells; i++)
    if (g[i]) {
      if (!(allowed(u, i) & (1u << g[i]))) return false;
      put(u, i, g[i]);
    }
  return true;
}

// The empty cell with the fewest numbers left (-1: none empty); `left` gets them.
inline int tightest(const uint8_t g[kCells], const Used &u, uint16_t &left) {
  int best = -1, bestN = 10;
  for (int i = 0; i < kCells; i++)
    if (!g[i]) {
      uint16_t a = allowed(u, i);
      int n = bits(a);
      if (n < bestN) best = i, bestN = n, left = a;
      if (n <= 1) break;
    }
  return best;
}

// ---- the search: depth first, the tightest cell first, without recursion (the board's loop has
// an 8 KB stack; a call a cell would take several KB of it). Its frames are in a Work, which the
// board keeps in PSRAM.

struct Frame {
  uint8_t cell, n, k;  // the cell; how many numbers to try there, and the next one
  uint8_t order[9];    // the numbers, in the order they're tried
};
struct Work {
  Frame f[kCells];
  uint32_t nodes = 0;  // numbers tried by the last search
};

// Counts the ways to fill g, no further than `limit`. With rnd the numbers are tried in a random
// order (filling a grid); without, smallest first. At `limit`, g is left as that solution. Past
// `budget` numbers tried (0: no budget) it gives up and returns `limit`, as if there were that many.
inline int search(uint8_t g[kCells], Used &u, int limit, Work &w, Rand rnd = nullptr, uint32_t budget = 0) {
  int depth = 0, found = 0;
  bool descend = true;
  w.nodes = 0;
  for (;;) {
    if (descend) {
      uint16_t left = 0;
      int i = tightest(g, u, left);
      if (i < 0) {
        if (++found >= limit) return found;
      } else {
        Frame &fr = w.f[depth++];
        fr.cell = i, fr.n = 0, fr.k = 0;
        for (int d = 1; d <= 9; d++)
          if (left & (1u << d)) fr.order[fr.n++] = d;
        for (int k = fr.n - 1; rnd && k > 0; k--) {  // Fisher-Yates
          int j = rnd(0, k);
          uint8_t t = fr.order[k];
          fr.order[k] = fr.order[j], fr.order[j] = t;
        }
      }
    }
    // The next number in the deepest open cell; back up a cell when it has none left.
    if (!depth) return found;
    Frame &fr = w.f[depth - 1];
    if (g[fr.cell]) take(u, fr.cell, g[fr.cell]), g[fr.cell] = 0;
    if (fr.k >= fr.n) {
      depth--, descend = false;
      continue;
    }
    if (budget && ++w.nodes > budget) return limit;
    if (!budget) w.nodes++;
    int d = fr.order[fr.k++];
    g[fr.cell] = d, put(u, fr.cell, d), descend = true;
  }
}

inline int countSolutions(const uint8_t puzzle[kCells], Work &w, int limit = 2, uint32_t budget = 0) {
  uint8_t g[kCells];
  memcpy(g, puzzle, kCells);
  Used u;
  if (!usedOf(g, u)) return 0;
  return search(g, u, limit, w, nullptr, budget);
}
// Solves in place; false when there's no solution.
inline bool solve(uint8_t g[kCells], Work &w) {
  Used u;
  if (!usedOf(g, u)) return false;
  return search(g, u, 1, w) == 1;
}

// ---- the grader: a small logic solver, going as a person would

// Naked singles (a cell with one number left) and hidden singles (a number with one place left
// in a row, column or box); with `pairs`, also naked pairs (two cells of a unit left with the same
// two numbers: no other cell there has them), hidden pairs (two numbers with the same two places
// in a unit: those cells hold nothing else) and pointing pairs (a number's places in a box all on
// one line: not elsewhere on that line; and the same from a line into a box).
// True when that fills the grid.
inline bool logicSolves(const uint8_t puzzle[kCells], bool pairs) {
  uint8_t g[kCells];
  uint16_t cand[kCells];
  memcpy(g, puzzle, kCells);
  Used u;
  if (!usedOf(g, u)) return false;
  for (int i = 0; i < kCells; i++) cand[i] = g[i] ? 0 : allowed(u, i);
  auto place = [&](int i, int d) {
    g[i] = d, cand[i] = 0;
    for (int j = 0; j < kCells; j++)
      if (peers(i, j)) cand[j] &= (uint16_t)~(1u << d);
  };
  for (;;) {
    bool progress = false, full = true;
    for (int i = 0; i < kCells; i++)
      if (!g[i]) {
        full = false;
        if (!cand[i]) return false;
        if (bits(cand[i]) == 1) place(i, __builtin_ctz(cand[i])), progress = true;
      }
    if (full) return true;
    for (int kind = 0; kind < 3; kind++)
      for (int k = 0; k < 9; k++)
        for (int d = 1; d <= 9; d++) {
          int where = -1, n = 0;
          for (int j = 0; j < 9 && n < 2; j++) {
            int i = unitCell(kind, k, j);
            if (g[i] == d) n = 2;  // placed already
            else if (!g[i] && (cand[i] & (1u << d))) where = i, n++;
          }
          if (n == 1) place(where, d), progress = true;
        }
    if (progress) continue;
    if (!pairs) return false;
    for (int kind = 0; kind < 3; kind++)
      for (int k = 0; k < 9; k++) {
        // naked pairs
        for (int a = 0; a < 9; a++) {
          int ia = unitCell(kind, k, a);
          if (g[ia] || bits(cand[ia]) != 2) continue;
          for (int b = a + 1; b < 9; b++) {
            int ib = unitCell(kind, k, b);
            if (g[ib] || cand[ib] != cand[ia]) continue;
            for (int j = 0; j < 9; j++) {
              int i = unitCell(kind, k, j);
              if (i != ia && i != ib && !g[i] && (cand[i] & cand[ia])) cand[i] &= (uint16_t)~cand[ia], progress = true;
            }
          }
        }
        // hidden pairs: where each number can go in this unit, a bit per place
        uint16_t where[10] = {};
        for (int j = 0; j < 9; j++) {
          int i = unitCell(kind, k, j);
          for (int d = 1; d <= 9; d++)
            if (!g[i] && (cand[i] & (1u << d))) where[d] |= 1u << j;
        }
        for (int d1 = 1; d1 <= 9; d1++) {
          if (bits(where[d1]) != 2) continue;
          for (int d2 = d1 + 1; d2 <= 9; d2++) {
            if (where[d2] != where[d1]) continue;
            uint16_t keep = (1u << d1) | (1u << d2);
            for (int j = 0; j < 9; j++)
              if (where[d1] & (1u << j)) {
                int i = unitCell(kind, k, j);
                if (cand[i] & ~keep) cand[i] &= keep, progress = true;
              }
          }
        }
      }
    // pointing pairs (box -> line) and box-line reduction (line -> box)
    for (int b = 0; b < 9; b++)
      for (int d = 1; d <= 9; d++) {
        int rows = 0, cols = 0, n = 0;
        for (int j = 0; j < 9; j++) {
          int i = unitCell(2, b, j);
          if (!g[i] && (cand[i] & (1u << d))) rows |= 1 << rowOf(i), cols |= 1 << colOf(i), n++;
        }
        if (n < 2) continue;
        for (int line = 0; line < 2; line++) {
          int m = line ? cols : rows;
          if (bits(m) != 1) continue;
          int r = __builtin_ctz(m);
          for (int j = 0; j < 9; j++) {
            int i = unitCell(line, r, j);
            if (boxOf(i) != b && !g[i] && (cand[i] & (1u << d))) cand[i] &= (uint16_t)~(1u << d), progress = true;
          }
        }
      }
    for (int line = 0; line < 2; line++)
      for (int r = 0; r < 9; r++)
        for (int d = 1; d <= 9; d++) {
          int boxes = 0;
          for (int j = 0; j < 9; j++) {
            int i = unitCell(line, r, j);
            if (!g[i] && (cand[i] & (1u << d))) boxes |= 1 << boxOf(i);
          }
          if (bits(boxes) != 1) continue;
          int b = __builtin_ctz(boxes);
          for (int j = 0; j < 9; j++) {
            int i = unitCell(2, b, j);
            bool onLine = line ? colOf(i) == r : rowOf(i) == r;
            if (!onLine && !g[i] && (cand[i] & (1u << d))) cand[i] &= (uint16_t)~(1u << d), progress = true;
          }
        }
    if (!progress) return false;
  }
}

// What a puzzle needs: SINGLES, PAIRS, or MORE (the grader alone can't finish it).
inline Technique grade(const uint8_t puzzle[kCells]) {
  if (logicSolves(puzzle, false)) return SINGLES;
  if (logicSolves(puzzle, true)) return PAIRS;
  return MORE;
}

// ---- the puzzle of the day

struct Puzzle {
  long date = -1;
  int level = 0, clues = 0, tries = 0;  // tries: which try was kept (1 the first)
  int attempts = 0;                     // how many tries were made
  uint32_t nodes = 0;                   // numbers tried by every search of those tries: the work done
  Technique needs = SINGLES;
  uint8_t given[kCells] = {};     // the puzzle: 0 an empty cell
  uint8_t solution[kCells] = {};  // its only solution
};

inline int countClues(const uint8_t g[kCells]) {
  int n = 0;
  for (int i = 0; i < kCells; i++) n += g[i] != 0;
  return n;
}

// One try: a fresh grid, then cells taken away as far as the level allows.
inline void attempt(Puzzle &p, int level, Work &w, Rand rnd) {
  memset(p.solution, 0, kCells);
  Used u;
  memset(&u, 0, sizeof u);
  search(p.solution, u, 1, w, rnd);  // an empty grid always fills
  p.nodes += w.nodes;
  memcpy(p.given, p.solution, kCells);
  // The pairs (i, 80 - i) for i 0..40 (40 is the middle, alone), in a random order.
  uint8_t order[41];
  for (int i = 0; i < 41; i++) order[i] = i;
  for (int k = 40; k > 0; k--) {
    int j = rnd(0, k);
    uint8_t t = order[k];
    order[k] = order[j], order[j] = t;
  }
  int clues = kCells;
  for (int k = 0; k < 41; k++) {
    int a = order[k], b = kCells - 1 - a, n = a == b ? 1 : 2;
    if (clues - n < kFewest[level]) continue;
    uint8_t va = p.given[a], vb = p.given[b];
    p.given[a] = p.given[b] = 0;
    bool ok = countSolutions(p.given, w, 2, kNodeBudget) == 1;
    p.nodes += w.nodes;
    ok = ok && (level == 2 || logicSolves(p.given, level == 1));
    if (ok) clues -= n;
    else p.given[a] = va, p.given[b] = vb;
  }
  p.clues = clues;
  p.needs = grade(p.given);
}

// How far a try is from what its level wants (0: just right).
inline int miss(const Puzzle &p) {
  int m = p.clues > kMost[p.level] ? (p.clues - kMost[p.level]) * 2 : 0;
  if ((int)p.needs < p.level) m += 1;  // easier than its level
  return m;
}

// `best` is room for the closest try so far (the board keeps it in PSRAM, beside w). rnd must be
// seeded with seed(date, level) first.
inline void make(Puzzle &p, long date, int level, Work &w, Puzzle &best, Rand rnd) {
  level = level < 0 ? 0 : level >= kLevels ? kLevels - 1 : level;
  p.date = date, p.level = level;
  best.clues = kCells + 1;
  int bestMiss = 1 << 20;
  uint32_t nodes = 0;
  for (int t = 1; t <= kTries[level]; t++) {
    p.nodes = 0;
    attempt(p, level, w, rnd);
    nodes += p.nodes;
    p.tries = p.attempts = t, p.nodes = nodes;
    int m = miss(p);
    if (!m) return;
    if (m < bestMiss) best = p, bestMiss = m;
  }
  p = best;
  p.attempts = kTries[level], p.nodes = nodes;
}

// ---- playing

// True when the number in cell i is also in its row, column or box (a clash: shown, never refused).
inline bool clashes(const uint8_t v[kCells], int i) {
  if (!v[i]) return false;
  for (int j = 0; j < kCells; j++)
    if (v[j] == v[i] && peers(i, j)) return true;
  return false;
}
inline int countClashes(const uint8_t v[kCells]) {
  int n = 0;
  for (int i = 0; i < kCells; i++) n += clashes(v, i);
  return n;
}
inline bool solved(const uint8_t v[kCells], const Puzzle &p) { return memcmp(v, p.solution, kCells) == 0; }
// A number placed in cell i takes its pencil mark off its row, column and box.
inline void clearPeersMark(uint16_t pencil[kCells], int i, int d) {
  for (int j = 0; j < kCells; j++)
    if (peers(i, j)) pencil[j] &= (uint16_t)~(1u << d);
}
// The values a save left still fit the puzzle: every given where it is, numbers 0..9.
inline bool fits(const uint8_t v[kCells], const Puzzle &p) {
  for (int i = 0; i < kCells; i++)
    if (v[i] > 9 || (p.given[i] && v[i] != p.given[i])) return false;
  return true;
}
// The Hint button (issue #25). Your numbers that aren't the solution's: a bit a cell in `out`
// (bit i % 32 of out[i / 32]); the count. Givens are never wrong.
inline int wrongCells(const uint8_t v[kCells], const Puzzle &p, uint32_t out[3]) {
  int n = 0;
  out[0] = out[1] = out[2] = 0;
  for (int i = 0; i < kCells; i++)
    if (v[i] && !p.given[i] && v[i] != p.solution[i]) out[i / 32] |= 1u << (i % 32), n++;
  return n;
}
inline bool marked(const uint32_t m[3], int i) { return m[i / 32] & (1u << (i % 32)); }
inline void unmark(uint32_t m[3], int i) { m[i / 32] &= ~(1u << (i % 32)); }

// What a hint points at, when your numbers are all right: an empty cell and its number, and why.
enum HintKind {
  HINT_ONLY = 0,  // only one number fits the cell (the others are in its row, column or box)
  HINT_ROW = 1,   // the only cell in its row where that number can go
  HINT_COL = 2,   // ... in its column
  HINT_BOX = 3,   // ... in its box
  HINT_NEXT = 4,  // neither: the empty cell with the fewest numbers left (the number from the solution)
};
struct Hint {
  int cell = -1, digit = 0;
  HintKind kind = HINT_NEXT;
};
// The next hint for v, whose numbers should all be right (wrongCells first): a cell where only one
// number fits, else a number with one place in a row, column or box, else the empty cell with the
// fewest candidates. Candidates come from the numbers on the grid, not the pencil marks. False when
// the grid is full.
inline bool findHint(const uint8_t v[kCells], const Puzzle &p, Hint &h) {
  Used u = {};
  for (int i = 0; i < kCells; i++)
    if (v[i]) put(u, i, v[i]);
  uint16_t cand[kCells];
  int fewest = -1;
  for (int i = 0; i < kCells; i++) {
    cand[i] = v[i] ? 0 : allowed(u, i);
    if (v[i]) continue;
    if (bits(cand[i]) == 1 && __builtin_ctz(cand[i]) == p.solution[i]) {
      h.cell = i, h.digit = p.solution[i], h.kind = HINT_ONLY;
      return true;
    }
    if (fewest < 0 || bits(cand[i]) < bits(cand[fewest])) fewest = i;
  }
  if (fewest < 0) return false;
  static const HintKind kinds[3] = {HINT_ROW, HINT_COL, HINT_BOX};
  for (int kind = 0; kind < 3; kind++)
    for (int k = 0; k < 9; k++)
      for (int d = 1; d <= 9; d++) {
        int where = -1, n = 0;
        for (int j = 0; j < 9 && n < 2; j++) {
          int i = unitCell(kind, k, j);
          if (v[i] == d) n = 2;
          else if (!v[i] && (cand[i] & (1u << d))) where = i, n++;
        }
        if (n == 1 && p.solution[where] == d) {
          h.cell = where, h.digit = d, h.kind = kinds[kind];
          return true;
        }
      }
  h.cell = fewest, h.digit = p.solution[fewest], h.kind = HINT_NEXT;
  return true;
}

// The first empty cell, else the first wrong one (-1: solved): what `game right` fills.
inline int nextCell(const uint8_t v[kCells], const Puzzle &p) {
  for (int i = 0; i < kCells; i++)
    if (!v[i]) return i;
  for (int i = 0; i < kCells; i++)
    if (v[i] != p.solution[i]) return i;
  return -1;
}

}  // namespace sudokugrid
