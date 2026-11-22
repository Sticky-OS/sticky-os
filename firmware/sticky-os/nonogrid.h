// Daily nonogram (issue #18): today's picture puzzle made from the date, the same on every board
// that runs this generator. Pure: no screen, no Arduino, no heap; tested on the computer by
// firmware/test/games/test_nonogrid.cpp.
//
// How a day's puzzle is made (daily()):
//   1. The seed is date * 2654435761u ^ salt, as in sudokugrid.h (and Daily maths): the salt
//      names the puzzle, the size and the generator's version (kSalt).
//   2. A random grid: 8 x 8 (Easy) or 10 x 10 (Normal), a seeded 50-60 % of the cells filled,
//      in seeded places. A grid with an empty row or column is thrown away.
//   3. The clues, then the line solver from a blank grid. It is kept only if the solver finishes
//      it: then it has one solution, found with single-line logic and no guessing.
//   4. Otherwise the next sub-seed, up to kAttempts (test_nonogrid.cpp shows how few it takes).
//
// The line solver (solve()): a line at a time, every placement of its clue that fits the cells
// already known; a cell filled in all of them is filled, a cell empty in all is empty. Lines
// whose cells changed go round again, until nothing changes.
//
// Memory: no heap. The deepest stack is daily() -> solve() -> solveLine(), whose placement
// search recurses once per block (at most 5 deep, ~40 bytes a level): under 600 bytes in all.
//
// A cell's id is row * side + col. Marks (the player's, and the solver's): kUnknown, kFilled,
// kCrossed. "Line" L is row L for L < side, column L - side after.
#pragma once
#include <stdint.h>

namespace nonogrid {

// ======================================================================== random numbers
//
// gameRnd's xorshift32 (games.cpp), as in sudokugrid.h: a copy with its own state, so nothing
// here moves gameRnd's sequence. Kept separate so each header stands alone.
struct Rng {
  uint32_t state;
  explicit Rng(uint32_t seed) : state(seed ? seed : 1) {}
  uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  int range(int lo, int hi) {  // lo..hi inclusive, as gameRnd
    uint32_t s = next();
    return hi <= lo ? lo : lo + (int)(s % (uint32_t)(hi - lo + 1));
  }
};

constexpr uint32_t fourcc(char a, char b, char c, char d) {
  return (uint32_t)(uint8_t)a << 24 | (uint32_t)(uint8_t)b << 16 | (uint32_t)(uint8_t)c << 8 | (uint32_t)(uint8_t)d;
}

static const int kSizes = 2;  // 0 Easy, 1 Normal
static const uint8_t kSide[kSizes] = {8, 10};
// 'N' 'G' for nonogram, the size's letter, the generator's version digit last.
static const uint32_t kSalt[kSizes] = {fourcc('N', 'G', 'E', '1'), fourcc('N', 'G', 'N', '1')};
static const int kMaxSide = 10, kMaxCells = 100, kMaxLines = 2 * kMaxSide;
static const int kMaxClues = (kMaxSide + 1) / 2;  // 1 0 1 0 ... : 5 blocks in 10 cells
static const int kAttempts = 32;  // ~3 in 4 random grids pass: 32 failing in a row is ~1e-17

inline uint32_t daySeed(long date, uint32_t salt, int attempt) {
  uint32_t s = ((uint32_t)date * 2654435761u ^ salt) ^ ((uint32_t)attempt * 0x9E3779B9u);
  return s ? s : 1;
}

enum Mark : uint8_t { kUnknown = 0, kFilled = 1, kCrossed = 2 };

struct Puzzle {
  uint8_t size = 0, side = 8;   // size 0 or 1; side 8 or 10
  uint8_t filled[kMaxCells];    // the solution: 1 filled, 0 empty
  uint8_t clue[kMaxLines][kMaxClues];  // per line (rows then columns), its blocks in order
  uint8_t clues[kMaxLines];            // how many blocks; never 0 here
  uint8_t filledCount = 0;
};

struct Stats {
  uint16_t attempts = 0;   // sub-seeds tried, 1 = the first one did
  uint16_t emptyLine = 0;  // attempts thrown away for an empty row or column
  uint16_t stuck = 0;      // attempts the line solver couldn't finish
  uint32_t steps = 0;      // lines solved, all attempts
  uint32_t lastSteps = 0;  // lines solved for the puzzle returned
  uint32_t placements = 0; // clue placements tried, all attempts
};

inline int lines(const Puzzle &p) { return 2 * p.side; }
inline int cells(const Puzzle &p) { return p.side * p.side; }
inline int lineCell(const Puzzle &p, int line, int i) {
  return line < p.side ? line * p.side + i : i * p.side + (line - p.side);
}
inline int rowLine(const Puzzle &, int row) { return row; }
inline int colLine(const Puzzle &p, int col) { return p.side + col; }

// The clue of a row or column: clueCount() blocks, clueAt(..., k) the k-th from the left or top.
inline int clueCount(const Puzzle &p, bool column, int index) { return p.clues[column ? p.side + index : index]; }
inline int clueAt(const Puzzle &p, bool column, int index, int k) { return p.clue[column ? p.side + index : index][k]; }

// The blocks of a line of marks (or of the solution, with isFilled), as clues are made.
inline int blocksOf(const Puzzle &p, const uint8_t *cellsIn, uint8_t want, int line, uint8_t out[kMaxClues + 1]) {
  int n = 0, run = 0;
  for (int i = 0; i <= p.side; i++) {
    bool on = i < p.side && cellsIn[lineCell(p, line, i)] == want;
    if (on) run++;
    else if (run) {
      if (n > kMaxClues) return n + 1;  // more blocks than any clue has: can't match
      out[n++] = (uint8_t)run, run = 0;
    }
  }
  return n;
}

inline void makeClues(Puzzle &p) {
  for (int l = 0; l < kMaxLines; l++) {  // the lines an 8 x 8 doesn't use are zeroed too
    uint8_t b[kMaxClues + 1];
    p.clues[l] = l < lines(p) ? (uint8_t)blocksOf(p, p.filled, 1, l, b) : 0;
    for (int k = 0; k < kMaxClues; k++) p.clue[l][k] = k < p.clues[l] ? b[k] : 0;
  }
}

// ======================================================================== the line solver

// Every placement of blocks k.. of `clue`, from cell `from`, that fits the known cells; each
// whole placement (a bit per cell, filled) is ANDed into *always and ORed into *ever.
struct LineSearch {
  const uint8_t *clue;
  int blocks, n;
  uint16_t isFilled, isEmpty;  // the known cells
  uint16_t always, ever;
  uint32_t found, tried;
  int tail[kMaxClues + 1];  // cells blocks k.. need, with their gaps

  void place(int k, int from, uint16_t sofar) {
    if (k == blocks) {
      uint16_t rest = (uint16_t)(((1u << n) - 1) & ~((1u << from) - 1));
      if (rest & isFilled) return;  // a filled cell left over
      always &= sofar, ever |= sofar, found++;
      return;
    }
    int len = clue[k];
    for (int s = from; s + tail[k] <= n; s++) {
      tried++;
      if (s > from && (isFilled & (1u << (s - 1)))) return;  // the gap before passed a filled cell
      uint16_t block = (uint16_t)(((1u << len) - 1) << s);
      if (block & isEmpty) continue;
      int end = s + len;
      if (end < n && (isFilled & (1u << end))) continue;  // must end before a filled cell
      place(k + 1, end < n ? end + 1 : n, (uint16_t)(sofar | block));
    }
  }
};

// Solves one line of length n: fills *mustFill and *mustEmpty with what every placement agrees
// on. False if no placement fits (the known cells contradict the clue).
inline bool solveLine(const uint8_t *clue, int blocks, int n, uint16_t isFilled, uint16_t isEmpty, uint16_t *mustFill,
                      uint16_t *mustEmpty, uint32_t *tried = nullptr) {
  const uint16_t all = (uint16_t)((1u << n) - 1);
  LineSearch s;
  s.clue = clue, s.blocks = blocks, s.n = n, s.isFilled = isFilled, s.isEmpty = isEmpty;
  s.always = all, s.ever = 0, s.found = 0, s.tried = 0;
  s.tail[blocks] = 0;
  for (int k = blocks - 1; k >= 0; k--) s.tail[k] = clue[k] + (k + 1 < blocks ? 1 + s.tail[k + 1] : 0);
  s.place(0, 0, 0);
  if (tried) *tried += s.tried;
  if (!s.found) return false;
  *mustFill = s.always, *mustEmpty = (uint16_t)(all & ~s.ever);
  return true;
}

enum Result : int8_t { kContradiction = -1, kStuck = 0, kSolved = 1 };

// The line solver on `marks` (kUnknown, kFilled, kCrossed; crossed is known empty), in place.
inline int solve(const Puzzle &p, uint8_t marks[kMaxCells], uint32_t *steps = nullptr, uint32_t *tried = nullptr) {
  bool dirty[kMaxLines];
  for (int l = 0; l < lines(p); l++) dirty[l] = true;
  for (bool again = true; again;) {
    again = false;
    for (int l = 0; l < lines(p); l++) {
      if (!dirty[l]) continue;
      dirty[l] = false;
      uint16_t f = 0, e = 0;
      for (int i = 0; i < p.side; i++) {
        uint8_t m = marks[lineCell(p, l, i)];
        if (m == kFilled) f |= (uint16_t)(1u << i);
        if (m == kCrossed) e |= (uint16_t)(1u << i);
      }
      uint16_t mf, me;
      if (steps) ++*steps;
      if (!solveLine(p.clue[l], p.clues[l], p.side, f, e, &mf, &me, tried)) return kContradiction;
      uint16_t fresh = (uint16_t)((mf & ~f) | (me & ~e));
      for (int i = 0; i < p.side; i++) {
        if (!(fresh & (1u << i))) continue;
        marks[lineCell(p, l, i)] = (mf & (1u << i)) ? kFilled : kCrossed;
        int cross = l < p.side ? p.side + i : i;  // the line through this cell the other way
        dirty[cross] = true, again = true;
      }
    }
  }
  for (int c = 0; c < cells(p); c++)
    if (marks[c] == kUnknown) return kStuck;
  return kSolved;
}

// ======================================================================== making today's puzzle

// Today's nonogram, size 0 (8 x 8, Easy) or 1 (10 x 10, Normal), for a date written yyyymmdd.
// False for a date that isn't one (<= 0), or if no sub-seed made a puzzle the line solver
// finishes (never in the tests: see test_nonogrid.cpp).
inline bool daily(long yyyymmdd, int size, Puzzle &out, Stats *stats = nullptr) {
  Stats local;
  Stats &st = stats ? *stats : local;
  st = Stats();
  if (yyyymmdd <= 0) return false;
  if (size < 0) size = 0;
  if (size >= kSizes) size = kSizes - 1;
  Puzzle &p = out;
  p.size = (uint8_t)size, p.side = kSide[size];
  const int n = cells(p);
  for (int a = 0; a < kAttempts; a++) {
    st.attempts = (uint16_t)(a + 1);
    Rng rng(daySeed(yyyymmdd, kSalt[size], a));
    int want = rng.range((n + 1) / 2, n * 6 / 10);  // 50-60 %
    uint8_t order[kMaxCells];
    for (int i = 0; i < n; i++) order[i] = (uint8_t)i;
    for (int i = n - 1; i > 0; i--) {
      int j = rng.range(0, i);
      uint8_t t = order[i];
      order[i] = order[j], order[j] = t;
    }
    for (int i = 0; i < kMaxCells; i++) p.filled[i] = 0;
    for (int i = 0; i < want; i++) p.filled[order[i]] = 1;
    p.filledCount = (uint8_t)want;
    makeClues(p);
    bool emptyLine = false;
    for (int l = 0; l < lines(p); l++) emptyLine |= p.clues[l] == 0;
    if (emptyLine) {
      st.emptyLine++;
      continue;
    }
    uint8_t marks[kMaxCells] = {};
    uint32_t steps = 0;
    int r = solve(p, marks, &steps, &st.placements);
    st.steps += steps;
    if (r == kSolved) {
      st.lastSteps = steps;
      return true;
    }
    st.stuck++;
  }
  return false;
}

// ======================================================================== the player's marks

// A row or column whose filled marks make exactly its clue: its clue is struck through.
inline bool lineDone(const Puzzle &p, const uint8_t marks[kMaxCells], bool column, int index) {
  int l = column ? p.side + index : index;
  uint8_t b[kMaxClues + 1];
  int n = blocksOf(p, marks, kFilled, l, b);
  if (n != p.clues[l]) return false;
  for (int k = 0; k < n; k++)
    if (b[k] != p.clue[l][k]) return false;
  return true;
}

// Every line matches its clue. The puzzle has one solution, so that is it; crosses don't matter.
inline bool solved(const Puzzle &p, const uint8_t marks[kMaxCells]) {
  for (int i = 0; i < p.side; i++)
    if (!lineDone(p, marks, false, i) || !lineDone(p, marks, true, i)) return false;
  return true;
}

// The cell `game peek` names as "next" and `game right` sets: one the line solver deduces in a
// single pass from the player's right marks (a wrong mark counts as unknown, so it can be the
// cell named). A fill first, then a cross on a wrongly filled cell, then any cross; if no line
// gives anything, the first wrong or missing fill. *mark is kFilled or kCrossed, the solution's.
// -1 when solved.
inline int next(const Puzzle &p, const uint8_t marks[kMaxCells], uint8_t *mark = nullptr) {
  if (solved(p, marks)) return -1;
  uint8_t known[kMaxCells];
  for (int c = 0; c < cells(p); c++) {
    bool right = (marks[c] == kFilled && p.filled[c]) || (marks[c] == kCrossed && !p.filled[c]);
    known[c] = right ? marks[c] : (uint8_t)kUnknown;
  }
  int fill = -1, fix = -1, cross = -1;
  for (int l = 0; l < lines(p) && fill < 0; l++) {
    uint16_t f = 0, e = 0;
    for (int i = 0; i < p.side; i++) {
      uint8_t m = known[lineCell(p, l, i)];
      if (m == kFilled) f |= (uint16_t)(1u << i);
      if (m == kCrossed) e |= (uint16_t)(1u << i);
    }
    uint16_t mf, me;
    if (!solveLine(p.clue[l], p.clues[l], p.side, f, e, &mf, &me)) continue;  // can't: known is right
    for (int i = 0; i < p.side; i++) {
      int c = lineCell(p, l, i);
      if (known[c] != kUnknown) continue;
      if ((mf & (1u << i)) && fill < 0) fill = c;
      if ((me & (1u << i)) && marks[c] == kFilled && fix < 0) fix = c;
      if ((me & (1u << i)) && cross < 0) cross = c;
    }
  }
  int at = fill >= 0 ? fill : fix >= 0 ? fix : cross;
  if (at < 0)
    for (int c = 0; c < cells(p) && at < 0; c++)
      if ((marks[c] == kFilled) != (p.filled[c] != 0)) at = c;
  if (at < 0) return -1;
  if (mark) *mark = p.filled[at] ? kFilled : kCrossed;
  return at;
}

}  // namespace nonogrid
