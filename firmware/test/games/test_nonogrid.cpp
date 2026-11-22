// Daily nonogram (firmware/sticky-os/nonogrid.h): the line logic against brute force; a year of
// puzzles at each size, each 50-60 % filled with no empty line, its clues matching its grid, the
// line solver finishing it and an independent search finding no second solution; fixed dates
// give fixed puzzles (golden strings); and the helpers the game will use (lineDone, solved, the
// "next" hint). Prints the host's times and step counts.
#include <chrono>
#include <cstdio>
#include <cstring>

#include "nonogrid.h"

static int checks = 0, failures = 0;
#define CHECK(c)                                                    \
  do {                                                              \
    checks++;                                                       \
    if (!(c)) failures++, std::printf("FAIL line %d: %s\n", __LINE__, #c); \
  } while (0)

namespace ng = nonogrid;

static long nextDay(long d) {
  int y = (int)(d / 10000), m = (int)(d / 100 % 100), day = (int)(d % 100);
  static const int len[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int n = len[m - 1] + (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0));
  if (++day > n) day = 1, m++;
  if (m > 12) m = 1, y++;
  return (long)y * 10000 + m * 100 + day;
}

// The blocks of a line given as bits, cell 0 the lowest.
static int blocks(uint32_t bitsIn, int n, uint8_t out[10]) {
  int k = 0, run = 0;
  for (int i = 0; i <= n; i++) {
    if (i < n && (bitsIn >> i & 1)) run++;
    else if (run) out[k++] = (uint8_t)run, run = 0;
  }
  return k;
}

// solveLine against every one of the 2^n lines.
static void testLineAgainstBruteForce() {
  ng::Rng rng(12345);
  int cases = 0, contradictions = 0;
  for (int t = 0; t < 20000; t++) {
    int n = t % 2 ? 10 : 8;
    uint32_t line = rng.next() & ((1u << n) - 1);
    uint8_t clue[10];
    int k = blocks(line, n, clue);
    if (k > ng::kMaxClues) continue;
    // Known cells: mostly from the line itself, sometimes a wrong one, to see contradictions.
    uint16_t f = 0, e = 0;
    for (int i = 0; i < n; i++) {
      int r = rng.range(0, 9);
      bool on = line >> i & 1;
      if (r == 9) on = !on;
      if (r >= 6) (on ? f : e) |= (uint16_t)(1u << i);
    }
    uint16_t always = (uint16_t)((1u << n) - 1), ever = 0;
    int fits = 0;
    for (uint32_t x = 0; x < (1u << n); x++) {
      if ((x & f) != f || (x & e)) continue;
      uint8_t b[10];
      int kb = blocks(x, n, b);
      if (kb != k || std::memcmp(b, clue, k)) continue;
      fits++, always &= (uint16_t)x, ever |= (uint16_t)x;
    }
    uint16_t mf = 0, me = 0;
    bool ok = ng::solveLine(clue, k, n, f, e, &mf, &me);
    CHECK(ok == (fits > 0));
    if (ok && fits) CHECK(mf == always && me == (uint16_t)(((1u << n) - 1) & ~ever));
    cases++, contradictions += !fits;
  }
  CHECK(cases > 19000 && contradictions > 100);
  // An empty clue: every cell empty.
  uint16_t mf, me;
  CHECK(ng::solveLine(nullptr, 0, 8, 0, 0, &mf, &me) && mf == 0 && me == 0xFF);
}

// Solutions of a puzzle's clues, up to 2, row by row through every placement of each row's clue,
// columns checked as they grow. Independent of the line solver.
struct Counter {
  const ng::Puzzle *p;
  uint16_t rowsOf[10][64];
  int rowCount[10];
  uint16_t grid[10];
  int found;

  void rowPlacements(int r, int k, int from, uint16_t sofar) {
    const ng::Puzzle &q = *p;
    if (k == q.clues[r]) {
      rowsOf[r][rowCount[r]++] = sofar;
      return;
    }
    int need = 0;
    for (int j = k; j < q.clues[r]; j++) need += q.clue[r][j] + (j > k);
    for (int s = from; s + need <= q.side; s++)
      rowPlacements(r, k + 1, s + q.clue[r][k] + 1, (uint16_t)(sofar | (((1u << q.clue[r][k]) - 1) << s)));
  }
  // Column c after rows 0..r: its finished blocks are the clue's first ones, the open one no longer.
  bool columnOk(int c, int r, bool last) const {
    const ng::Puzzle &q = *p;
    const uint8_t *clue = q.clue[q.side + c];
    int n = q.clues[q.side + c], k = 0, run = 0;
    for (int i = 0; i <= r; i++) {
      if (grid[i] >> c & 1) {
        run++;
        if (k >= n || run > clue[k]) return false;
      } else if (run) {
        if (run != clue[k]) return false;
        k++, run = 0;
      }
    }
    if (last) return k + (run > 0) == n && (!run || run == clue[k]);
    return true;
  }
  void rows(int r) {
    if (found >= 2) return;
    if (r == p->side) {
      found++;
      return;
    }
    for (int i = 0; i < rowCount[r] && found < 2; i++) {
      grid[r] = rowsOf[r][i];
      bool ok = true;
      for (int c = 0; c < p->side && ok; c++) ok = columnOk(c, r, r == p->side - 1);
      if (ok) rows(r + 1);
    }
  }
  int count(const ng::Puzzle &q) {
    p = &q, found = 0;
    for (int r = 0; r < q.side; r++) rowCount[r] = 0, rowPlacements(r, 0, 0, 0);
    rows(0);
    return found;
  }
};

static void text(const ng::Puzzle &p, char out[101]) {
  for (int c = 0; c < ng::cells(p); c++) out[c] = p.filled[c] ? '#' : '.';
  out[ng::cells(p)] = 0;
}

static void testYear() {
  static const char *kName[2] = {"8x8", "10x10"};
  for (int size = 0; size < ng::kSizes; size++) {
    double maxUs = 0, sumUs = 0;
    int days = 0, maxAttempts = 0, minFill = 100, maxFill = 0;
    long attempts = 0, emptyLines = 0, stuck = 0;
    uint32_t maxSteps = 0, maxLast = 0, maxTried = 0;
    for (long d = 20260101; d < 20270101; d = nextDay(d)) {
      ng::Puzzle p;
      ng::Stats st;
      auto t0 = std::chrono::steady_clock::now();
      bool ok = ng::daily(d, size, p, &st);
      double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
      CHECK(ok);
      if (!ok) continue;
      days++, sumUs += us;
      if (us > maxUs) maxUs = us;
      CHECK(p.size == size && p.side == ng::kSide[size]);
      int n = ng::cells(p), fill = 0;
      for (int c = 0; c < n; c++) fill += p.filled[c] == 1;
      CHECK(fill == p.filledCount);
      CHECK(fill * 100 >= n * 50 && fill * 100 <= n * 60);  // 50-60 %
      if (fill < minFill) minFill = fill;
      if (fill > maxFill) maxFill = fill;
      // The clues are the grid's, and no line is empty.
      for (int l = 0; l < ng::lines(p); l++) {
        uint32_t bitsIn = 0;
        for (int i = 0; i < p.side; i++) bitsIn |= (uint32_t)p.filled[ng::lineCell(p, l, i)] << i;
        uint8_t b[10];
        int k = blocks(bitsIn, p.side, b);
        CHECK(k > 0 && k == p.clues[l] && !std::memcmp(b, p.clue[l], k));
      }
      CHECK(ng::clueCount(p, true, 0) == p.clues[p.side] && ng::clueAt(p, true, 0, 0) == p.clue[p.side][0]);
      // The line solver finishes it from blank, at the solution; the search finds no other.
      uint8_t marks[ng::kMaxCells] = {};
      CHECK(ng::solve(p, marks) == ng::kSolved);
      for (int c = 0; c < n; c++) CHECK((marks[c] == ng::kFilled) == (p.filled[c] == 1));
      Counter counter;
      CHECK(counter.count(p) == 1);
      attempts += st.attempts, emptyLines += st.emptyLine, stuck += st.stuck;
      if (st.attempts > maxAttempts) maxAttempts = st.attempts;
      if (st.steps > maxSteps) maxSteps = st.steps;
      if (st.lastSteps > maxLast) maxLast = st.lastSteps;
      if (st.placements > maxTried) maxTried = st.placements;
    }
    CHECK(days == 365);
    CHECK(maxAttempts < ng::kAttempts / 2);
    std::printf(
        "  %-6s %d days: filled %d-%d of %d; attempts avg %.2f, max %d (of %d); thrown away: %ld empty line, %ld "
        "stuck\n"
        "         host %.1f us avg, %.0f us max; lines solved max %u a day (%u for the puzzle kept), placements "
        "tried max %u\n"
        "         board estimate at 40x this host: %.1f ms max\n",
        kName[size], days, minFill, maxFill, ng::kSide[size] * ng::kSide[size], (double)attempts / days, maxAttempts,
        ng::kAttempts, emptyLines, stuck, sumUs / days, maxUs, maxSteps, maxLast, maxTried, maxUs * 40 / 1000);
  }
}

// Fixed dates, fixed puzzles ('#' filled, row by row). If one changes on purpose, bump kSalt's
// version digit and paste the new strings this prints.
static void testGolden() {
  struct Golden {
    long date;
    int size;
    const char *grid;
  } golden[] = {
      {20260101, 0, ".#####...##.######..#......##...#.#.##.#.###..#..#....####...###"},
      {20260704, 0, "###.....#..#.#.#..#..##.#..###....##..###.####..###...##..##.##."},
      {20261225, 0, ".##.###.#.##.##..###..#....#.##..####....#.#.##.#...####.#..####"},
      {20260101, 1, ".#..###.#...####..#..##.####..##..#...###.##.#.###.....###..#..##..##.#..##..##.#.##.###....##.#...."},
      {20260704, 1, "##.##.#....####.##.##.##.#..##.#####...#..#.##.#########..####...#..###..###.....#..#.#.##..###.#.#."},
      {20261225, 1, ".#.#.#.###.##.#.###.#.##....###.###.#..#..#..####.#.###..#..#...###....###..#.##.#.#.#.#.##.####.###"},
  };
  for (const Golden &g : golden) {
    ng::Puzzle p, again;
    ng::daily(g.date, g.size, p);
    ng::daily(g.date, g.size, again);
    CHECK(std::memcmp(p.filled, again.filled, sizeof p.filled) == 0);
    CHECK(std::memcmp(p.clue, again.clue, sizeof p.clue) == 0);
    char s[101];
    text(p, s);
    bool same = std::strcmp(s, g.grid) == 0;
    CHECK(same);
    if (!same) std::printf("  golden %ld size %d is now \"%s\"\n", g.date, g.size, s);
  }
  ng::Puzzle a, b;
  CHECK(!ng::daily(0, 0, a));
  ng::daily(20260101, 0, a), ng::daily(20260102, 0, b);
  CHECK(std::memcmp(a.filled, b.filled, sizeof a.filled) != 0);
  // The issue's seed, attempt 0.
  CHECK(ng::daySeed(20260101, ng::kSalt[1], 0) == ((uint32_t)20260101 * 2654435761u ^ ng::kSalt[1]));
}

// lineDone, solved and next, by playing a few days through the hints, starting from a wrong mark.
static void testPlay() {
  int plays = 0;
  for (long d = 20260301; d < 20260311; d = nextDay(d))
    for (int size = 0; size < ng::kSizes; size++) {
      ng::Puzzle p;
      ng::daily(d, size, p);
      int n = ng::cells(p);
      uint8_t marks[ng::kMaxCells] = {};
      CHECK(!ng::solved(p, marks) && !ng::lineDone(p, marks, false, 0));
      int wrong = 0;  // a fill where the solution is empty, and a cross where it's filled
      while (p.filled[wrong]) wrong++;
      marks[wrong] = ng::kFilled;
      int wrongCross = 0;
      while (!p.filled[wrongCross]) wrongCross++;
      marks[wrongCross] = ng::kCrossed;
      int moves = 0, fills = 0;
      for (;;) {
        uint8_t mark = 0;
        int at = ng::next(p, marks, &mark);
        if (at < 0) break;
        CHECK(at < n && marks[at] != mark);
        CHECK(mark == (p.filled[at] ? ng::kFilled : ng::kCrossed));
        fills += mark == ng::kFilled;
        marks[at] = mark;
        if (++moves > n) break;
      }
      CHECK(ng::solved(p, marks));
      for (int c = 0; c < n; c++) CHECK((marks[c] == ng::kFilled) == (p.filled[c] == 1));
      for (int i = 0; i < p.side; i++) CHECK(ng::lineDone(p, marks, false, i) && ng::lineDone(p, marks, true, i));
      CHECK(marks[wrong] == ng::kCrossed);  // the hint put the wrong fill right
      CHECK(fills == p.filledCount);
      // A line one fill short isn't done; its clue stays.
      int c0 = 0;
      while (!p.filled[c0]) c0++;
      marks[c0] = ng::kUnknown;
      CHECK(!ng::lineDone(p, marks, false, c0 / p.side) && !ng::lineDone(p, marks, true, c0 % p.side));
      CHECK(!ng::solved(p, marks));
      uint8_t mark;
      CHECK(ng::next(p, marks, &mark) == c0 && mark == ng::kFilled);
      plays++;
    }
  CHECK(plays == 20);
}

int main() {
  testLineAgainstBruteForce();
  testYear();
  testGolden();
  testPlay();
  std::printf("nonogrid: %d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
