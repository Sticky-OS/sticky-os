// Daily Sudoku (firmware/sticky-os/sudokugrid.h): ten years of puzzles at each level, each with
// one solution, its clues in the level's range, solvable by the level's techniques and (Medium,
// Hard) not by the level below's, with the rare fallbacks counted; fixed dates give fixed puzzles (the golden strings: a generator change shows up in
// review, and then kSalt's version digit goes up with it); the budgets hold; and the helpers the
// game will use (clashes, solved, the "next" hint). Prints the host's times and node counts.
#include <chrono>
#include <cstdio>
#include <cstring>

#include "sudokugrid.h"

static int checks = 0, failures = 0;
#define CHECK(c)                                                    \
  do {                                                              \
    checks++;                                                       \
    if (!(c)) failures++, std::printf("FAIL line %d: %s\n", __LINE__, #c); \
  } while (0)

namespace sg = sudokugrid;

// The day after a yyyymmdd date.
static long nextDay(long d) {
  int y = (int)(d / 10000), m = (int)(d / 100 % 100), day = (int)(d % 100);
  static const int len[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int n = len[m - 1] + (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0));
  if (++day > n) day = 1, m++;
  if (m > 12) m = 1, y++;
  return (long)y * 10000 + m * 100 + day;
}

static void text(const uint8_t g[81], char out[82]) {
  for (int c = 0; c < 81; c++) out[c] = g[c] ? (char)('0' + g[c]) : '.';
  out[81] = 0;
}

static bool validFull(const uint8_t g[81]) {
  for (int c = 0; c < 81; c++)
    if (g[c] < 1 || g[c] > 9) return false;
  return sg::clashCount(g) == 0;
}

// games.cpp's gameSeed / gameRnd, copied as they are (with a fixed seed: no esp_random here).
static uint32_t rngState;
static void gameSeed(uint32_t seed) { rngState = seed ? seed : 1; }
static int gameRnd(int lo, int hi) {
  rngState ^= rngState << 13;  // xorshift32
  rngState ^= rngState >> 17;
  rngState ^= rngState << 5;
  return hi <= lo ? lo : lo + (int)(rngState % (uint32_t)(hi - lo + 1));
}

static void testRngIsGameRnd() {
  for (uint32_t seed : {1u, 20260101u * 2654435761u, 0xDEADBEEFu}) {
    gameSeed(seed);
    sg::Rng r(seed);
    int differ = 0;
    for (int i = 0; i < 10000; i++) {
      int lo = i % 7, hi = lo + i % 23;
      differ += gameRnd(lo, hi) != r.range(lo, hi);
    }
    CHECK(differ == 0);
  }
  // The issue's seed, attempt 0: date * 2654435761u ^ salt.
  CHECK(sg::daySeed(20260101, sg::kSalt[0], 0) == ((uint32_t)20260101 * 2654435761u ^ sg::kSalt[0]));
  CHECK(sg::kSalt[0] != sg::kSalt[1] && sg::kSalt[1] != sg::kSalt[2]);
}

// What a run of days at one level came to.
struct Tally {
  int days = 0, fallbacks = 0, retried = 0, grades[5] = {}, minClues = 81, maxClues = 0, maxAttempts = 0;
  long attempts = 0;
  uint32_t maxNodes = 0, maxTotal = 0, maxSteps = 0, maxWork = 0, cuts = 0;
  double maxUs = 0, sumUs = 0;
  void print(const char *name, const char *span) const {
    std::printf(
        "  %-6s %s, %d days: clues %d-%d; fallback %d (%.2f %%); attempts avg %.2f, max %d, retried %d days; "
        "needs singles %d, locked %d, pairs %d, beyond %d\n"
        "         host %.0f us avg, %.0f us max; nodes max %u in one search (budget %u), %u in one day; grader passes "
        "max %u; work max %u (budget %u); cuts %u\n"
        "         board estimate at 40x this host: %.0f ms avg, %.0f ms max\n",
        name, span, days, minClues, maxClues, fallbacks, 100.0 * fallbacks / days, (double)attempts / days,
        maxAttempts, retried, grades[sudokugrid::kSingles], grades[sudokugrid::kLocked],
        grades[sudokugrid::kPairs], grades[sudokugrid::kBeyond], sumUs / days, maxUs, maxNodes,
        sudokugrid::kSearchBudget, maxTotal, maxSteps, maxWork, sudokugrid::kWorkBudget, cuts, sumUs / days * 40 / 1000,
        maxUs * 40 / 1000);
  }
};

// Ten years from 2026-01-01 at each level: everything the issue asks of a puzzle, every day; the
// first year and the ten printed apart.
static void testDays() {
  static const char *kName[3] = {"easy", "medium", "hard"};
  for (int level = 0; level < sg::kLevels; level++) {
    Tally year, decade;
    for (long d = 20260101; d < 20360101; d = nextDay(d)) {
      sg::Puzzle p;
      sg::Stats st;
      auto t0 = std::chrono::steady_clock::now();
      bool ok = sg::daily(d, level, p, &st);
      double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
      CHECK(ok);
      if (!ok) continue;
      CHECK(p.level == level);
      CHECK(validFull(p.solution));
      int clues = 0;
      for (int c = 0; c < 81; c++) {
        CHECK(p.grid[c] == 0 || p.grid[c] == p.solution[c]);
        clues += p.grid[c] != 0;
      }
      CHECK(clues == p.clues);
      // Exactly one solution, by a count with a budget it never needs.
      bool cut = true;
      uint8_t sol[81];
      CHECK(sg::countSolutions(p.grid, 2, 50000000, sol, nullptr, &cut) == 1);
      CHECK(!cut);
      CHECK(std::memcmp(sol, p.solution, 81) == 0);
      // Each level both ways: what it allows solves it, what the level below allows doesn't. A
      // fallback (rare: counted) is still within its level's techniques, and in range.
      uint8_t out[81];
      int singles = sg::grade(p.grid, sg::kSingles, out);
      int medium = sg::grade(p.grid, sg::kMediumTechniques);
      if (level == 0) CHECK(singles == sg::kSingles && !std::memcmp(out, p.solution, 81) && !st.fallback);
      if (level == 1) CHECK(medium != sg::kUnsolved && (st.fallback || singles == sg::kUnsolved));
      if (level == 2) CHECK(st.fallback || medium == sg::kUnsolved);
      CHECK(st.technique == sg::difficulty(medium));
      CHECK(p.clues >= sg::kMinClues[level]);
      CHECK(p.clues <= sg::kMaxClues[level]);
      // The budgets: no search past its own, no day past the work budget (but for the one grade
      // or search under way when it ran out), no more attempts than allowed.
      uint32_t work = st.nodes + st.graderSteps * sg::kPassWork;
      CHECK(st.maxNodes <= sg::kSearchBudget);
      CHECK(work <= sg::kWorkBudget + sg::kSearchBudget + 400 * sg::kPassWork);
      CHECK(st.attempts >= 1 && st.attempts <= sg::kAttempts[level]);
      for (Tally *t : {&year, &decade}) {
        if (t == &year && d >= 20270101) continue;
        t->days++, t->sumUs += us, t->attempts += st.attempts;
        if (us > t->maxUs) t->maxUs = us;
        t->fallbacks += st.fallback;
        t->retried += st.attempts > 1;
        t->grades[st.technique]++;
        t->cuts += st.cuts;
        if (st.attempts > t->maxAttempts) t->maxAttempts = st.attempts;
        if (p.clues < t->minClues) t->minClues = p.clues;
        if (p.clues > t->maxClues) t->maxClues = p.clues;
        if (st.maxNodes > t->maxNodes) t->maxNodes = st.maxNodes;
        if (st.nodes > t->maxTotal) t->maxTotal = st.nodes;
        if (st.graderSteps > t->maxSteps) t->maxSteps = st.graderSteps;
        if (work > t->maxWork) t->maxWork = work;
      }
    }
    CHECK(year.days == 365 && decade.days == 3652);
    CHECK(year.fallbacks * 100 < year.days && decade.fallbacks * 100 < decade.days);  // rare: under 1 %
    year.print(kName[level], "2026");
    decade.print(kName[level], "2026-35");
  }
}

// The grader's techniques are sound: run to where it's stuck on Hard puzzles (where it gets
// stuck) and Medium ones, every digit it placed and every candidate it kept agrees with the
// solution. And Medium's puzzles use both of its tiers.
static void testGraderSound() {
  int stuck = 0, wrong = 0, tiers[5] = {};
  for (long d = 20260101; d < 20260401; d = nextDay(d))
    for (int level = 1; level <= 2; level++) {
      sg::Puzzle p;
      sg::Stats st;
      sg::daily(d, level, p, &st);
      sg::Grader s;
      CHECK(s.start(p.grid));
      for (;;) {
        int r = s.singles();
        CHECK(r >= 0);
        if (r < 0) break;
        if (r > 0) continue;
        if (s.locked() || s.pairs()) continue;
        break;
      }
      stuck += s.left > 0;
      for (int c = 0; c < 81; c++)
        wrong += s.g[c] ? s.g[c] != p.solution[c] : !(s.cand[c] & sg::digitBit(p.solution[c]));
      if (level == 1 && !st.fallback) tiers[sg::grade(p.grid, sg::kMediumTechniques)]++;
    }
  CHECK(wrong == 0);
  CHECK(stuck >= 85);  // the Hard ones
  CHECK(tiers[sg::kLocked] > 0 && tiers[sg::kPairs] > 0 && tiers[sg::kSingles] == 0);
}

// Fixed dates, fixed puzzles. If one changes on purpose, bump kSalt's version digit and paste
// the new strings this prints.
static void testGolden() {
  struct Golden {
    long date;
    int level;
    const char *grid;
  } golden[] = {
      {20260101, 0, "9...15.2....7.3.....3....81.791.42364...6719.26.9.8.5..1.4.....3.857.419.9..8.5.."},
      {20260704, 0, ".76...3..4.93.5167.3271..897..8..5..2..5.9.....5.3.24..2..9..51.....187.6.127...4"},
      {20261225, 0, "93.1.7...5.2.8..9.6..5.28.14.6..12.82..6.9.1....2..96..69..3...8..9.63.4..4.15.79"},
      {20260101, 1, ".5......6.......72...867459.956..24.7....9..5...153..8....829.48.6..5.2.4....6..."},
      {20260704, 1, "8...9.4...4.8.3792.....7.865......4.62.4..5.948..3..21.6.3......1...695.2...89.6."},
      {20261225, 1, ".....7.3.6...9.27...8...649....3.5..9..6..7.25..2.89134.....1.6.15..64.....9.4..."},
      {20260101, 2, ".46.81......4..19..51.3..8.3..57....4...1.97...7..4...5.9..78...8.1.92.........1."},
      {20260704, 2, "4...8..23.....265......5..9....2.....6.......34...621..16.9..4.23.74.5.68........"},
      {20261225, 2, "2.1...9...4.16..7.8..3....5..6...5.21..8...69..9..4........72.8......6....82.6.1."},
  };
  for (const Golden &g : golden) {
    sg::Puzzle p, again;
    sg::daily(g.date, g.level, p);
    sg::daily(g.date, g.level, again);
    CHECK(std::memcmp(&p, &again, sizeof p) == 0);  // the same twice
    char s[82];
    text(p.grid, s);
    bool same = std::strcmp(s, g.grid) == 0;
    CHECK(same);
    if (!same) std::printf("  golden %ld level %d is now \"%s\"\n", g.date, g.level, s);
  }
  // Levels and days differ.
  sg::Puzzle a, b, c;
  sg::daily(20260101, 0, a), sg::daily(20260101, 1, b), sg::daily(20260102, 0, c);
  CHECK(std::memcmp(a.solution, b.solution, 81) != 0);
  CHECK(std::memcmp(a.solution, c.solution, 81) != 0);
  // No date, no puzzle.
  CHECK(!sg::daily(0, 0, a));
  CHECK(!sg::daily(-1, 2, a));
}

// The helpers: clashes, solved and next, by playing every level of a few days through the hints.
static void testPlay() {
  int plays = 0;
  for (long d = 20260301; d < 20260311; d = nextDay(d))
    for (int level = 0; level < sg::kLevels; level++) {
      sg::Puzzle p;
      sg::daily(d, level, p);
      uint8_t cells[81];
      std::memcpy(cells, p.grid, 81);
      CHECK(sg::clashCount(cells) == 0 && !sg::solved(cells));
      // A wrong digit beside a given in its row clashes; the hint still leads to the solution.
      int empty = 0;
      while (cells[empty]) empty++;
      int row = empty / 9, given = -1;
      for (int i = 0; i < 9 && given < 0; i++)
        if (cells[row * 9 + i]) given = row * 9 + i;
      if (given >= 0) {
        cells[empty] = cells[given];
        CHECK(sg::clashes(cells, empty) >= 1 && sg::clashes(cells, given) >= 1);
        CHECK(sg::clashCount(cells) >= 2);
      }
      int moves = 0;
      for (;;) {
        uint8_t digit = 0;
        int at = sg::next(p, cells, &digit);
        if (at < 0) break;
        CHECK(at < 81 && cells[at] != p.solution[at] && digit == p.solution[at]);
        cells[at] = digit;
        if (++moves > 81) break;
      }
      CHECK(sg::solved(cells) && std::memcmp(cells, p.solution, 81) == 0);
      CHECK(sg::next(p, cells) == -1);
      plays++;
    }
  CHECK(plays == 30);
  // From the start of an Easy puzzle the first hint is a single: placing it keeps singles going.
  sg::Puzzle p;
  sg::daily(20260101, 0, p);
  uint8_t digit;
  int at = sg::next(p, p.grid, &digit);
  CHECK(at >= 0 && p.grid[at] == 0 && digit == p.solution[at]);
  CHECK(sg::clashes(p.grid, -1) == 0 && sg::clashes(p.grid, at) == 0);
}

int main() {
  testRngIsGameRnd();
  testDays();
  testGraderSound();
  testGolden();
  testPlay();
  std::printf("sudokugrid: %d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
