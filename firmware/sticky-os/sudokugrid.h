// Daily Sudoku (issue #18): today's puzzle made from the date, the same on every board that
// runs this generator. Pure: no screen, no Arduino, no heap; tested on the computer by
// firmware/test/games/test_sudokugrid.cpp.
//
// How a day's puzzle is made (daily()):
//   1. The seed is date * 2654435761u ^ salt, as Daily maths seeds its questions (mathDaily,
//      games.cpp). The salt names the puzzle, the level and the generator's version (kSalt), so a
//      changed generator changes the puzzles on purpose: bump the version digit with it.
//   2. A full grid by backtracking, each cell trying 1-9 in its own seeded order.
//   3. Digging, in a seeded order (dig()), so that each level is what it says, both ways:
//        Easy   36-40 clues, solvable by naked and hidden singles alone;
//        Medium 30-34 clues, solvable by singles, locked candidates (pointing, claiming) and
//               naked and hidden pairs, and NOT by singles alone;
//        Hard   26-29 clues, one solution, and NOT solvable by Medium's techniques.
//      A puzzle the grader solves has one solution (every step is forced), so Easy and Medium need
//      no search; Hard asks the grader first and only then the search.
//   4. An attempt that isn't in range and hard enough tries the next sub-seed, up to kAttempts;
//      after that the best attempt is taken (Stats::fallback, rare: see the test). Every puzzle
//      returned has exactly one solution: a removal the search couldn't settle is put back.
//
// Memory: no heap, no recursion. The deepest stack is daily() -> dig() -> tryRemove() ->
// countSolutions() or grade(), about 1.7 KB in all (the 729-byte digit orders, two 81-byte cell
// orders, a work grid, 81 search frames of 4 bytes or the grader's 250 bytes), well inside the
// loop task's ~8 KB.
//
// Time: kWorkBudget caps one daily() (search nodes, and grader passes at kPassWork nodes each),
// kSearchBudget one search, kAttempts the retries. test_sudokugrid.cpp prints the host's times
// and counts, and the board estimate.
//
// A cell's id is row * 9 + col. Digits are 1-9, 0 is an empty cell.
#pragma once
#include <stdint.h>

namespace sudokugrid {

// ======================================================================== random numbers
//
// The firmware's own generator, so a puzzle can be checked by hand against gameRnd: xorshift32
// with the same shifts, and range() is gameRnd(lo, hi) line for line (games.cpp). It is a copy,
// not a call, because this header can't depend on games.cpp, and its state is its own: making a
// puzzle never moves gameRnd's sequence, so there is no gameSeed(0) to restore afterwards.
struct Rng {
  uint32_t state;
  explicit Rng(uint32_t seed) : state(seed ? seed : 1) {}  // gameSeed(0) means "random": never here
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
// 'S' 'K' for Sudoku, the level's letter, the generator's version digit last. Version 2: the
// levels graded both ways (Medium needs more than singles, Hard more than Medium's techniques).
static const uint32_t kSalt[3] = {fourcc('S', 'K', 'E', '2'), fourcc('S', 'K', 'M', '2'), fourcc('S', 'K', 'H', '2')};

// Sub-seed `attempt` of a date: attempt 0 is the issue's date * 2654435761u ^ salt, the retries
// are spread by the golden ratio so they never land on another level's seed.
inline uint32_t daySeed(long date, uint32_t salt, int attempt) {
  uint32_t s = ((uint32_t)date * 2654435761u ^ salt) ^ ((uint32_t)attempt * 0x9E3779B9u);
  return s ? s : 1;
}

// ======================================================================== levels and budgets

static const int kLevels = 3;  // 0 Easy, 1 Medium, 2 Hard
static const uint8_t kMinClues[kLevels] = {36, 30, 26};
static const uint8_t kMaxClues[kLevels] = {40, 34, 29};
// Sub-seeds per level. Over 3650 days (test_sudokugrid.cpp) Medium needed at most 37 when not
// capped, 20 at the 99th percentile; Hard 16 and 8. Easy always takes the first.
static const int kAttempts[kLevels] = {1, 24, 12};
// Work is counted in search nodes; a grader pass costs about as much as kPassWork of them (host,
// fitted over 2026: ~0.04 us a node, ~0.85 us a pass). kWorkBudget caps one daily(): when it runs
// out, digging stops (the puzzle still unique) and the best attempt so far is taken.
static const uint32_t kPassWork = 20;
static const uint32_t kSearchBudget = 20000;  // one search (the fill, or a uniqueness check)
// The worst day of ten years (2026-35), host, Apple M5 Max: ~3.8 ms (Hard) and ~3.7 ms (Medium,
// its 24 attempts), so ~150 ms on the board if it is 40x slower. A 3x margin under 300 ms would
// need ~2.5 ms: with these techniques that costs more than 1 % fallbacks.
static const uint32_t kWorkBudget = 80000;

// The techniques the grader knows, in order: a puzzle's grade is the hardest it needed.
// kLocked is pointing (a box's candidates for a digit all in one row or column: the digit leaves
// the rest of that line) and claiming, or box-line reduction (a line's candidates for a digit all
// in one box: the digit leaves the rest of that box). It is its own tier, tried before pairs,
// because people meet it first; Medium allows both tiers.
enum Technique : uint8_t { kUnsolved = 0, kSingles = 1, kLocked = 2, kPairs = 3, kBeyond = 4 };
static const uint8_t kMediumTechniques = kPairs;  // singles, locked candidates, naked and hidden pairs

struct Puzzle {
  uint8_t grid[81];      // the givens, 0 elsewhere
  uint8_t solution[81];  // the one solution
  uint8_t clues;
  uint8_t level;
};

struct Stats {
  uint8_t attempts = 0;      // sub-seeds tried, 1 = the first one did
  bool fallback = false;     // no attempt was in range and hard enough: the best was taken
  uint8_t technique = 0;     // what the puzzle returned needs: kSingles, kLocked, kPairs or kBeyond
  uint32_t nodes = 0;        // search nodes, the fills and all searches of this daily()
  uint32_t maxNodes = 0;     // the most nodes one fill or search took
  uint32_t graderSteps = 0;  // grader passes, all calls
  uint32_t cuts = 0;         // searches cut short by a budget (each kept its clue)
};

// ======================================================================== the grid

// The 27 units: rows 0-8, columns 9-17, boxes 18-26; i is 0-8 along the unit. Tables, built at
// compile time (324 bytes of flash), because the grader and the search ask all the time.
struct Tables {
  uint8_t unit[27][9];
  uint8_t box[81];
  constexpr Tables() : unit(), box() {
    for (int c = 0; c < 81; c++) box[c] = (uint8_t)(c / 27 * 3 + c % 9 / 3);
    for (int i = 0; i < 9; i++)
      for (int u = 0; u < 9; u++) {
        unit[u][i] = (uint8_t)(u * 9 + i);
        unit[9 + u][i] = (uint8_t)(i * 9 + u);
        unit[18 + u][i] = (uint8_t)((u / 3 * 3 + i / 3) * 9 + u % 3 * 3 + i % 3);
      }
  }
};
static constexpr Tables kTables{};
inline int boxOf(int cell) { return kTables.box[cell]; }
inline int unitCell(int unit, int i) { return kTables.unit[unit][i]; }
inline bool peers(int a, int b) { return a != b && (a / 9 == b / 9 || a % 9 == b % 9 || boxOf(a) == boxOf(b)); }
inline uint16_t bit(int digit) { return (uint16_t)(1u << (digit - 1)); }
inline int bits(uint16_t m) { return __builtin_popcount(m); }
inline int lowDigit(uint16_t m) { return __builtin_ctz(m) + 1; }
static const uint16_t kAll = 0x1FF;

// The other cells in its row, column or box that hold the same digit as `cell` (0 if it's empty).
inline int clashes(const uint8_t cells[81], int cell) {
  if (cell < 0 || cell >= 81 || !cells[cell]) return 0;
  int n = 0;
  for (int c = 0; c < 81; c++) n += cells[c] == cells[cell] && peers(c, cell);
  return n;
}
// How many cells clash with another (game peek's "clashes").
inline int clashCount(const uint8_t cells[81]) {
  int n = 0;
  for (int c = 0; c < 81; c++) n += clashes(cells, c) > 0;
  return n;
}
// Full, with no clash: with one solution, that is the solution.
inline bool solved(const uint8_t cells[81]) {
  for (int c = 0; c < 81; c++)
    if (!cells[c]) return false;
  return clashCount(cells) == 0;
}
inline int clueCount(const uint8_t grid[81]) {
  int n = 0;
  for (int c = 0; c < 81; c++) n += grid[c] != 0;
  return n;
}

// ======================================================================== the solution counter
//
// Depth-first, always on the empty cell with the fewest candidates. Counts solutions up to
// `limit`; the first one goes to `first`. `order`, if given, is each cell's 9 digits in the order
// to try them (the full grid's seeded shuffles); otherwise lowest first. Stops after `budget`
// nodes (digits placed) with *cut set: the count is then only a lower bound. `ban`, if >= 0, is a
// cell that may not hold `banDigit`: dig() asks whether a removed clue could be anything else.
// Stack: 81 frames of 4 bytes, a grid and the masks, ~450 bytes.
inline int countSolutions(const uint8_t grid[81], int limit, uint32_t budget, uint8_t *first = nullptr,
                          uint32_t *nodesOut = nullptr, bool *cut = nullptr, const uint8_t *order = nullptr,
                          int ban = -1, int banDigit = 0) {
  uint8_t g[81];
  uint16_t row[9] = {}, col[9] = {}, box[9] = {};
  if (cut) *cut = false;
  if (nodesOut) *nodesOut = 0;
  for (int c = 0; c < 81; c++) {
    g[c] = grid[c];
    if (!g[c]) continue;
    uint16_t b = bit(g[c]);
    if ((row[c / 9] | col[c % 9] | box[boxOf(c)]) & b) return 0;  // the givens clash
    row[c / 9] |= b, col[c % 9] |= b, box[boxOf(c)] |= b;
  }
  struct Frame {
    uint8_t cell, tried;  // tried: how far along order[] this cell has got
    uint16_t left;        // candidates not yet tried
  } stack[81];
  int depth = 0, found = 0;
  uint32_t nodes = 0;
  for (;;) {
    int best = -1, bestN = 10;
    uint16_t bestM = 0;
    for (int c = 0; c < 81; c++) {
      if (g[c]) continue;
      uint16_t m = (uint16_t)(~(row[c / 9] | col[c % 9] | box[boxOf(c)]) & kAll);
      if (c == ban) m &= (uint16_t)~bit(banDigit);
      int n = bits(m);
      if (n < bestN) {
        best = c, bestN = n, bestM = m;
        if (n <= 1) break;
      }
    }
    if (best < 0) {  // full: a solution
      if (!found && first)
        for (int c = 0; c < 81; c++) first[c] = g[c];
      if (++found >= limit) break;
    } else if (bestN > 0) {
      stack[depth].cell = (uint8_t)best, stack[depth].tried = 0, stack[depth].left = bestM;
      depth++;
    }
    // The next digit at the top frame, backing out of frames that have none left.
    bool placed = false;
    while (depth > 0 && !placed) {
      Frame &f = stack[depth - 1];
      int c = f.cell;
      if (g[c]) {
        uint16_t b = bit(g[c]);
        row[c / 9] &= ~b, col[c % 9] &= ~b, box[boxOf(c)] &= ~b;
        g[c] = 0;
      }
      if (!f.left) {
        depth--;
        continue;
      }
      int d = 0;
      if (order) {
        while (!(f.left & bit(order[c * 9 + f.tried]))) f.tried++;
        d = order[c * 9 + f.tried++];
      } else {
        d = lowDigit(f.left);
      }
      uint16_t b = bit(d);
      f.left &= ~b;
      g[c] = (uint8_t)d;
      row[c / 9] |= b, col[c % 9] |= b, box[boxOf(c)] |= b;
      placed = true;
      if (++nodes > budget) {
        if (cut) *cut = true;
        if (nodesOut) *nodesOut = nodes;
        return found;
      }
    }
    if (!placed) break;  // every branch tried
  }
  if (nodesOut) *nodesOut = nodes;
  return found;
}

// ======================================================================== the grader
//
// A small logic solver, the way a person plays, in tiers: naked singles (a cell with one
// candidate) and hidden singles (a digit with one place in a unit); if `allowed` >= kLocked,
// pointing and claiming (see Technique); if `allowed` >= kPairs, naked pairs (two cells of a unit
// with the same two candidates: those leave the unit's other cells) and hidden pairs (two digits
// with the same two places in a unit: those cells keep only them). A tier is used only when the
// tiers below it have nothing left, and after any progress it starts again from singles.
// Returns the hardest tier needed (kSingles, kLocked or kPairs), or kUnsolved if it got stuck or
// met a contradiction. A solve leaves the grid in `out`, if given; *steps counts its passes.
// Stack: ~350 bytes.
struct Grader {
  uint8_t g[81];
  uint16_t cand[81];
  int left;  // empty cells

  void place(int c, int d) {
    g[c] = (uint8_t)d, cand[c] = 0, left--;
    uint16_t b = bit(d);
    int r = c / 9, k = c % 9, x = boxOf(c);
    for (int i = 0; i < 9; i++) {
      cand[r * 9 + i] &= ~b;
      cand[i * 9 + k] &= ~b;
      cand[unitCell(18 + x, i)] &= ~b;
    }
  }
  bool start(const uint8_t grid[81]) {
    for (int c = 0; c < 81; c++) g[c] = 0, cand[c] = kAll;
    left = 81;
    for (int c = 0; c < 81; c++)
      if (grid[c]) {
        if (!(cand[c] & bit(grid[c]))) return false;  // clashes with a given placed before
        place(c, grid[c]);
      }
    return true;
  }
  // One round of singles: 1 if any digit went in, 0 if none, -1 on a contradiction.
  int singles() {
    int progress = 0;
    for (int c = 0; c < 81; c++) {
      if (g[c]) continue;
      if (!cand[c]) return -1;
      if (!(cand[c] & (cand[c] - 1))) place(c, lowDigit(cand[c])), progress = 1;
    }
    for (int u = 0; u < 27; u++) {
      // A digit's places in the unit, counted 0, 1, more: once and twice, a bit per digit.
      uint16_t have = 0, once = 0, twice = 0;
      for (int i = 0; i < 9; i++) {
        int c = unitCell(u, i);
        if (g[c]) have |= bit(g[c]);
        else twice |= once & cand[c], once |= cand[c];
      }
      uint16_t missing = (uint16_t)(kAll & ~have);
      if (missing & ~once) return -1;  // a digit with nowhere to go
      for (uint16_t single = (uint16_t)(missing & once & ~twice); single; single &= (uint16_t)(single - 1)) {
        int d = lowDigit(single), at = -1;
        for (int i = 0; i < 9 && at < 0; i++) {
          int c = unitCell(u, i);
          if (!g[c] && (cand[c] & bit(d))) at = c;
        }
        if (at < 0) return -1;  // its one place took another digit just now
        place(at, d), progress = 1;
      }
    }
    return progress;
  }
  // One round of locked candidates (pointing and claiming): true if any candidate went. Works on
  // the 54 segments where a row or column crosses a box (3 cells each): seg[0][r][j] is row r's
  // cells in box column j, seg[1][c][j] column c's in box row j, the OR of their candidates. A
  // digit in a segment and in neither of its box's other two segments in that direction is pointing
  // (it leaves the rest of the line); in neither of its line's other two, claiming (it leaves the
  // rest of the box). Masks taken before the round only ever see more candidates than there are,
  // so the round eliminates no more than it should.
  bool locked() {
    uint16_t seg[2][9][3];
    for (int a = 0; a < 9; a++)
      for (int j = 0; j < 3; j++) {
        seg[0][a][j] = cand[a * 9 + j * 3] | cand[a * 9 + j * 3 + 1] | cand[a * 9 + j * 3 + 2];
        seg[1][a][j] = cand[j * 27 + a] | cand[j * 27 + 9 + a] | cand[j * 27 + 18 + a];
      }
    bool progress = false;
    for (int v = 0; v < 2; v++)
      for (int a = 0; a < 9; a++)
        for (int j = 0; j < 3; j++) {
          uint16_t m = seg[v][a][j];
          if (!m) continue;
          int a0 = a / 3 * 3;
          uint16_t inBox = seg[v][a0 + (a + 1) % 3][j] | seg[v][a0 + (a + 2) % 3][j];
          uint16_t inLine = seg[v][a][(j + 1) % 3] | seg[v][a][(j + 2) % 3];
          uint16_t point = (uint16_t)(m & ~inBox & inLine), claim = (uint16_t)(m & ~inLine & inBox);
          if (!point && !claim) continue;
          for (int i = 0; i < 9; i++) {
            int along = v == 0 ? a * 9 + i : i * 9 + a;  // the line, outside the segment
            if (point && i / 3 != j && (cand[along] & point)) cand[along] &= ~point, progress = true;
            int across = v == 0 ? (a0 + i / 3) * 9 + j * 3 + i % 3 : (j * 3 + i % 3) * 9 + a0 + i / 3;
            bool other = v == 0 ? across / 9 != a : across % 9 != a;  // the box, outside the line
            if (claim && other && (cand[across] & claim)) cand[across] &= ~claim, progress = true;
          }
        }
    return progress;
  }
  // One round of pairs: true if any candidate went.
  bool pairs() {
    bool progress = false;
    for (int u = 0; u < 27; u++) {
      for (int i = 0; i < 9; i++) {  // naked
        int a = unitCell(u, i);
        if (g[a] || bits(cand[a]) != 2) continue;
        for (int j = i + 1; j < 9; j++) {
          int b = unitCell(u, j);
          if (g[b] || cand[b] != cand[a]) continue;
          for (int k = 0; k < 9; k++) {
            int c = unitCell(u, k);
            if (c == a || c == b || g[c] || !(cand[c] & cand[a])) continue;
            cand[c] &= ~cand[a], progress = true;
          }
        }
      }
      // Hidden: only digits with exactly two places in the unit can pair up.
      uint16_t once = 0, twice = 0, thrice = 0;
      for (int i = 0; i < 9; i++) {
        uint16_t m = cand[unitCell(u, i)];  // 0 for a filled cell
        thrice |= twice & m, twice |= once & m, once |= m;
      }
      uint16_t two = (uint16_t)(twice & ~thrice);
      if (bits(two) < 2) continue;
      uint16_t where[10] = {};  // where[d] = the unit's places for d, a bit per i
      for (int i = 0; i < 9; i++) {
        uint16_t m = (uint16_t)(cand[unitCell(u, i)] & two);
        for (; m; m &= (uint16_t)(m - 1)) where[lowDigit(m)] |= (uint16_t)(1u << i);
      }
      for (uint16_t ds = two; ds; ds &= (uint16_t)(ds - 1)) {
        int d = lowDigit(ds);
        for (uint16_t es = (uint16_t)(ds & (ds - 1)); es; es &= (uint16_t)(es - 1)) {
          int e = lowDigit(es);
          if (where[e] != where[d]) continue;
          uint16_t keep = bit(d) | bit(e);
          for (int i = 0; i < 9; i++)
            if (where[d] & (1u << i)) {
              int c = unitCell(u, i);
              if (cand[c] & ~keep) cand[c] &= keep, progress = true;
            }
        }
      }
    }
    return progress;
  }
};

inline int grade(const uint8_t grid[81], int allowed, uint8_t *out = nullptr, uint32_t *steps = nullptr) {
  Grader s;
  if (!s.start(grid)) return kUnsolved;
  int used = kSingles;
  while (s.left) {
    if (steps) ++*steps;
    int r = s.singles();
    if (r < 0) return kUnsolved;
    if (r > 0) continue;
    if (allowed >= kLocked && s.locked()) {
      if (used < kLocked) used = kLocked;
      continue;
    }
    if (allowed < kPairs || !s.pairs()) return kUnsolved;
    used = kPairs;
  }
  if (out)
    for (int c = 0; c < 81; c++) out[c] = s.g[c];
  return used;
}

// ======================================================================== making today's puzzle

// A grade as a difficulty: kBeyond for what Medium's techniques can't solve.
inline int difficulty(int grade) { return grade == kUnsolved ? kBeyond : grade; }
// What each level needs beyond what it allows: Medium more than singles, Hard more than Medium.
static const uint8_t kNeeds[kLevels] = {kSingles, kLocked, kBeyond};

inline void spend(uint32_t &workLeft, uint32_t work) { workLeft = work < workLeft ? workLeft - work : 0; }

// Tries removing `c` from p.grid: whether the puzzle stays within the level (Easy and Medium: their
// techniques solve it; Hard: unique), leaving `c` removed if so. `diff` is the puzzle's difficulty
// before, and after if it stays. A clue that is a naked or hidden single in what's left (by the
// givens alone) needs no grading: the first step puts it back, so nothing changes. `workLeft` is what's left of kWorkBudget.
inline bool tryRemove(Puzzle &p, int c, int &diff, uint32_t &workLeft, Stats &st) {
  const int level = p.level;
  uint8_t d = p.grid[c];
  p.grid[c] = 0;
  uint16_t row[9] = {}, col[9] = {}, box[9] = {};  // the digits each unit's givens hold
  for (int x = 0; x < 81; x++)
    if (p.grid[x]) row[x / 9] |= bit(p.grid[x]), col[x % 9] |= bit(p.grid[x]), box[boxOf(x)] |= bit(p.grid[x]);
  auto open = [&](int x) { return (uint16_t)(kAll & ~(row[x / 9] | col[x % 9] | box[boxOf(x)])); };
  if (bits(open(c)) == 1) return true;  // a naked single
  const int units[3] = {c / 9, 9 + c % 9, 18 + boxOf(c)};
  for (int u : units) {
    bool elsewhere = false;
    for (int i = 0; i < 9 && !elsewhere; i++) {
      int x = unitCell(u, i);
      elsewhere = x != c && !p.grid[x] && (open(x) & bit(d));
    }
    if (!elsewhere) return true;  // a hidden single
  }
  uint32_t passes = st.graderSteps;
  int g = grade(p.grid, level == 0 ? kSingles : kMediumTechniques, nullptr, &st.graderSteps);
  spend(workLeft, (st.graderSteps - passes) * kPassWork);
  bool keep = g != kUnsolved;  // solved by logic: one solution, no search needed
  if (level == 2 && !keep && workLeft) {
    uint32_t n = 0;
    bool cut = false;
    uint32_t budget = workLeft < kSearchBudget ? workLeft : kSearchBudget;
    // Unique still, if no solution puts anything else where the clue was (it had one before).
    keep = countSolutions(p.grid, 1, budget, nullptr, &n, &cut, nullptr, c, d) == 0 && !cut;
    if (n > budget) n = budget;
    spend(workLeft, n), st.nodes += n;
    if (n > st.maxNodes) st.maxNodes = n;
    st.cuts += cut;
  }
  if (!keep) p.grid[c] = d;
  else diff = difficulty(g);
  return keep;
}

// Whether the puzzle is hard enough for its level (kNeeds), by the grader alone. Step 3 asks,
// after a clue goes back: singles first (cheap, and they settle "no").
inline bool hardEnough(const Puzzle &p, uint32_t &workLeft, Stats &st) {
  if (p.level == 0) return true;
  uint32_t passes = st.graderSteps;
  bool hard = grade(p.grid, kSingles, nullptr, &st.graderSteps) == kUnsolved;
  if (hard) hard = (grade(p.grid, kMediumTechniques, nullptr, &st.graderSteps) != kUnsolved) == (p.level == 1);
  spend(workLeft, (st.graderSteps - passes) * kPassWork);
  return hard;
}

// Digs `p.solution` in seeded order (see the top), in three steps:
//   1. Down: a removal stays if the puzzle stays within the level (tryRemove), to one clue above
//      the level's minimum (Easy: to the seeded target, and that's all).
//   2. Medium and Hard: at each count from there down to the minimum, until the puzzle is hard
//      enough (kNeeds), the next clue out is the first given left, in the seeded order, whose
//      removal makes it so; if none does, the first that keeps it within the level. Looking ahead
//      like this (rather than only taking the next cell) is what makes most attempts succeed:
//      Medium ~1 in 4 attempts instead of ~1 in 20.
//   3. If it is hard enough: removed cells return, in the reverse of the order they went, while it
//      stays hard enough, until it has the seeded `target` clues. A clue added never costs the
//      one solution, nor the techniques' solve.
inline void dig(Puzzle &p, Rng &rng, int target, uint32_t &workLeft, Stats &st) {
  uint8_t order[81], gone[81];
  int nGone = 0;
  for (int i = 0; i < 81; i++) order[i] = (uint8_t)i;
  for (int i = 80; i > 0; i--) {
    int j = rng.range(0, i);
    uint8_t t = order[i];
    order[i] = order[j], order[j] = t;
  }
  for (int c = 0; c < 81; c++) p.grid[c] = p.solution[c];
  p.clues = 81;
  const int lo = kMinClues[p.level];
  const int floor = p.level == 0 ? target : lo + 1;
  int diff = kSingles, k = 0;
  for (; k < 81 && p.clues > floor && workLeft; k++)
    if (tryRemove(p, order[k], diff, workLeft, st)) p.clues--, gone[nGone++] = order[k];
  if (p.level == 0) return;
  while (p.clues > lo && diff < kNeeds[p.level] && workLeft) {  // step 2
    int first = -1, firstDiff = diff, at = -1;
    for (int j = k; j < 81 && workLeft; j++) {
      if (!p.grid[order[j]]) continue;
      int dj = diff;
      if (!tryRemove(p, order[j], dj, workLeft, st)) continue;
      bool hard = dj >= kNeeds[p.level];
      if (first < 0 || hard) first = order[j], firstDiff = dj, at = j;
      p.grid[order[j]] = p.solution[order[j]];
      if (hard) break;
    }
    if (first < 0) return;
    p.grid[first] = 0, p.clues--, gone[nGone++] = (uint8_t)first, diff = firstDiff;
    uint8_t t = order[at];  // it went: move it before the rest of the order
    order[at] = order[k], order[k] = t, k++;
  }
  if (diff < kNeeds[p.level]) return;
  for (int i = nGone - 1; i >= 0 && p.clues < target; i--) {  // step 3
    int c = gone[i];
    p.grid[c] = p.solution[c];
    if (hardEnough(p, workLeft, st)) p.clues++;
    else p.grid[c] = 0;
  }
}

// Today's puzzle at `level` (0 Easy, 1 Medium, 2 Hard) for a date written yyyymmdd. False only
// for a date that isn't one (<= 0): the caller shows "no date" then, as Daily maths does.
// The first attempt in range and hard enough is taken. If none of kAttempts is (or the work
// budget ran out), the best one is (Stats::fallback): the hardest, up to what the level needs,
// then one in range, then the fewest clues. So a fallback Hard that any attempt made need more
// than singles does; a fallback Medium is singles-only but at its fewest clues, 30 (test: rare).
inline bool daily(long yyyymmdd, int level, Puzzle &out, Stats *stats = nullptr) {
  Stats local;
  Stats &st = stats ? *stats : local;
  st = Stats();
  if (yyyymmdd <= 0) return false;
  if (level < 0) level = 0;
  if (level >= kLevels) level = kLevels - 1;
  const int lo = kMinClues[level], hi = kMaxClues[level];
  uint32_t workLeft = kWorkBudget;
  Puzzle p;
  p.level = (uint8_t)level;
  int bestScore = -1;
  bool good = false;
  for (int a = 0; a < kAttempts[level] && !good && workLeft; a++) {
    st.attempts = (uint8_t)(a + 1);
    Rng rng(daySeed(yyyymmdd, kSalt[level], a));
    int target = rng.range(lo, hi);
    uint8_t order[81 * 9];
    for (int c = 0; c < 81; c++) {
      uint8_t *o = order + c * 9;
      for (int i = 0; i < 9; i++) o[i] = (uint8_t)(i + 1);
      for (int i = 8; i > 0; i--) {
        int j = rng.range(0, i);
        uint8_t t = o[i];
        o[i] = o[j], o[j] = t;
      }
    }
    uint8_t empty[81] = {};
    uint32_t n = 0;
    bool cut = false;
    // An empty grid fills without a dead end in practice; the budget is only a safety net.
    int filled = countSolutions(empty, 1, kSearchBudget, p.solution, &n, &cut, order);
    st.nodes += n;
    if (n > st.maxNodes) st.maxNodes = n;
    spend(workLeft, n);
    if (filled != 1) continue;
    dig(p, rng, target, workLeft, st);
    int diff = difficulty(grade(p.grid, kMediumTechniques));
    bool inRange = p.clues >= lo && p.clues <= hi;
    good = inRange && diff >= kNeeds[level];
    int capped = diff < kNeeds[level] ? diff : kNeeds[level];
    int score = capped * 1000 + inRange * 100 + (81 - p.clues);
    if (good || score > bestScore) out = p, bestScore = score, st.technique = (uint8_t)diff;
  }
  if (bestScore < 0) return false;  // can't happen: the fill always succeeds
  st.fallback = !good;
  return true;
}

// ======================================================================== the hint

// The cell `game peek` names as "next" and `game right` fills: an empty (or wrong) cell that
// naked or hidden singles fill from the right digits on the board, else the first empty or wrong
// cell. Its digit, always the solution's, goes to *digit. -1 when the board is solved.
inline int next(const Puzzle &p, const uint8_t cells[81], uint8_t *digit = nullptr) {
  uint8_t known[81];
  int fallback = -1;
  for (int c = 0; c < 81; c++) {
    known[c] = cells[c] == p.solution[c] ? cells[c] : 0;
    if (!known[c] && fallback < 0) fallback = c;
  }
  if (fallback < 0) return -1;
  Grader s;
  int at = -1;
  if (s.start(known)) {
    for (int c = 0; c < 81 && at < 0; c++)
      if (!s.g[c] && bits(s.cand[c]) == 1) at = c;
    for (int u = 0; u < 27 && at < 0; u++)
      for (int d = 1; d <= 9 && at < 0; d++) {
        int n = 0, last = -1;
        bool placed = false;
        for (int i = 0; i < 9; i++) {
          int c = unitCell(u, i);
          placed |= s.g[c] == d;
          if (!s.g[c] && (s.cand[c] & bit(d))) n++, last = c;
        }
        if (!placed && n == 1) at = last;
      }
  }
  if (at < 0) at = fallback;
  if (digit) *digit = p.solution[at];
  return at;
}

}  // namespace sudokugrid
