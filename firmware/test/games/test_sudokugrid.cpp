// Sudoku (firmware/sticky-os/sudokugrid.h, issue #18): for a year of dates at every level the
// puzzle of the day has exactly one solution, its clues in the level's range, and the techniques
// its level allows solve it (Easy singles, Medium singles and pairs); it's the same every time
// (and the same as the board's: fixed dates give fixed puzzles, checked character for character);
// the node counts stay within the budget; the solver, grader and clash check.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "sudokugrid.h"

static int checks = 0, failures = 0;
#define CHECK(c)                                                         \
  do {                                                                   \
    checks++;                                                            \
    if (!(c)) failures++, std::printf("FAIL line %d: %s\n", __LINE__, #c); \
  } while (0)

namespace sg = sudokugrid;
static sg::Work work;
static sg::Puzzle spare;

// A copy of games.cpp's gameSeed / gameRnd (xorshift32), which the board passes to make().
static uint32_t rngState;
static void gameSeed(uint32_t seed) { rngState = seed ? seed : 0x12345u; }
static int gameRnd(int lo, int hi) {
  rngState ^= rngState << 13;
  rngState ^= rngState >> 17;
  rngState ^= rngState << 5;
  return hi <= lo ? lo : lo + (int)(rngState % (uint32_t)(hi - lo + 1));
}

// The board's way (games_sudoku.ino): seed, make, then gameSeed(0).
static void makeDay(sg::Puzzle &p, long date, int level) {
  gameSeed(sg::seed(date, level));
  sg::make(p, date, level, work, spare, gameRnd);
}

static void text(const uint8_t g[81], char out[82]) {
  for (int i = 0; i < 81; i++) out[i] = g[i] ? char('0' + g[i]) : '.';
  out[81] = 0;
}
static void fromText(const char *s, uint8_t g[81]) {
  for (int i = 0; i < 81; i++) g[i] = s[i] == '.' ? 0 : s[i] - '0';
}

static bool validFull(const uint8_t g[81]) {
  for (int i = 0; i < 81; i++)
    if (g[i] < 1 || g[i] > 9 || sg::clashes(g, i)) return false;
  return true;
}

// Dates as the board has them (20261122), a day apart.
static long dateAfter(long date, int days) {
  struct tm d = {};
  d.tm_year = date / 10000 - 1900, d.tm_mon = date / 100 % 100 - 1, d.tm_mday = date % 100 + days, d.tm_hour = 12;
  mktime(&d);
  return (d.tm_year + 1900) * 10000L + (d.tm_mon + 1) * 100 + d.tm_mday;
}

static void testSolverAndGrader() {
  // A well-known easy puzzle and its solution.
  const char *q = "53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79";
  const char *a = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";
  uint8_t g[81];
  fromText(q, g);
  CHECK(sg::countSolutions(g, work) == 1);
  CHECK(sg::grade(g) == sg::SINGLES);
  uint8_t s[81];
  memcpy(s, g, 81);
  CHECK(sg::solve(s, work));
  char t[82];
  text(s, t);
  CHECK(strcmp(t, a) == 0);
  // Arto Inkala's "world's hardest": one solution, beyond singles and pairs.
  fromText("8..........36......7..9.2...5...7.......457.....1...3...1....68..85...1..9....4..", g);
  CHECK(sg::countSolutions(g, work, 2) == 1);
  CHECK(sg::grade(g) == sg::MORE);
  // An empty grid has many solutions; the count stops at the limit; a budget gives up.
  uint8_t empty[81] = {};
  CHECK(sg::countSolutions(empty, work, 2) == 2);
  CHECK(sg::countSolutions(empty, work, 1000, 50) == 1000 && work.nodes > 50);
  // Two of a number in a row: no solution.
  uint8_t bad[81] = {};
  bad[0] = bad[5] = 4;
  CHECK(sg::countSolutions(bad, work) == 0);
  CHECK(!sg::solve(bad, work));
  CHECK(sg::grade(bad) == sg::MORE);
}

static void testClashes() {
  uint8_t v[81] = {};
  v[0] = 5, v[8] = 5;  // the same row
  v[80] = 5;           // the same column as 8
  v[40] = 3;           // alone
  CHECK(sg::clashes(v, 0) && sg::clashes(v, 8) && sg::clashes(v, 80));
  CHECK(!sg::clashes(v, 40) && !sg::clashes(v, 1));
  CHECK(sg::countClashes(v) == 3);
  v[8] = 0;
  CHECK(!sg::clashes(v, 0) && !sg::clashes(v, 80));
  v[10] = 5;  // the same box as cell 0
  CHECK(sg::clashes(v, 0) && sg::clashes(v, 10));
  // Placing a number takes its pencil mark off the row, column and box, nowhere else.
  uint16_t pencil[81];
  for (int i = 0; i < 81; i++) pencil[i] = 0x3FE;
  sg::clearPeersMark(pencil, 40, 7);
  int off = 0;
  for (int i = 0; i < 81; i++) off += !(pencil[i] & (1 << 7));
  CHECK(off == 20);  // 8 in the row, 8 in the column, 4 more in the box
  CHECK(pencil[40] & (1 << 7));
}

static void testYear() {
  static const char *const NAMES[] = {"easy", "medium", "hard"};
  double total[3] = {}, worst[3] = {};
  int minClues[3] = {81, 81, 81}, maxClues[3] = {}, needs[3][3] = {}, maxTries[3] = {};
  uint32_t maxNodes = 0;
  long date = 20260101;
  const int DAYS = 365;
  for (int day = 0; day < DAYS; day++, date = dateAfter(date, 1))
    for (int level = 0; level < 3; level++) {
      sg::Puzzle p;
      auto t0 = std::chrono::steady_clock::now();
      makeDay(p, date, level);
      double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
      total[level] += ms, worst[level] = ms > worst[level] ? ms : worst[level];
      CHECK(p.date == date && p.level == level);
      CHECK(validFull(p.solution));
      for (int i = 0; i < 81; i++) CHECK(!p.given[i] || p.given[i] == p.solution[i]);
      CHECK(sg::countClues(p.given) == p.clues);
      CHECK(p.clues >= sg::kFewest[level] && p.clues <= sg::kMost[level]);
      CHECK(sg::countSolutions(p.given, work, 2) == 1);
      maxNodes = work.nodes > maxNodes ? work.nodes : maxNodes;
      CHECK(work.nodes < sg::kNodeBudget);
      CHECK(sg::grade(p.given) == p.needs);
      if (level == 0) CHECK(p.needs == sg::SINGLES);
      if (level == 1) CHECK(p.needs <= sg::PAIRS);
      needs[level][p.needs]++;
      maxTries[level] = p.tries > maxTries[level] ? p.tries : maxTries[level];
      uint8_t s[81];
      memcpy(s, p.given, 81);
      CHECK(sg::solve(s, work) && memcmp(s, p.solution, 81) == 0);
      CHECK(sg::fits(p.given, p) && sg::fits(p.solution, p) && sg::solved(p.solution, p) && !sg::solved(p.given, p));
      CHECK(sg::nextCell(p.given, p) >= 0 && sg::nextCell(p.solution, p) == -1);
      for (int i = 0; i < 81; i++) CHECK(!p.given[i] == !p.given[80 - i]);  // symmetric
      minClues[level] = p.clues < minClues[level] ? p.clues : minClues[level];
      maxClues[level] = p.clues > maxClues[level] ? p.clues : maxClues[level];
      if (day % 30 == 0) {  // the same again: every board makes the same puzzle that day
        sg::Puzzle again;
        makeDay(again, date, level);
        CHECK(memcmp(again.given, p.given, 81) == 0 && memcmp(again.solution, p.solution, 81) == 0);
      }
    }
  sg::Puzzle a, b, c;
  makeDay(a, 20261122, 0), makeDay(b, 20261123, 0), makeDay(c, 20261122, 1);
  CHECK(memcmp(a.solution, b.solution, 81) != 0 && memcmp(a.solution, c.solution, 81) != 0);
  for (int l = 0; l < 3; l++)
    std::printf("sudoku %-6s %d days: clues %d..%d, needs singles %d / pairs %d / more %d, up to %d tries, %.2f ms on average, %.2f ms at most\n",
                NAMES[l], DAYS, minClues[l], maxClues[l], needs[l][0], needs[l][1], needs[l][2], maxTries[l], total[l] / DAYS, worst[l]);
  std::printf("sudoku: the most nodes to count a day's solutions: %u (budget %u)\n", (unsigned)maxNodes, (unsigned)sg::kNodeBudget);
  CHECK(needs[1][sg::PAIRS] > DAYS / 3);  // Medium often needs a pair (else the closest of its tries: fewer clues than Easy)
  CHECK(needs[2][sg::MORE] > DAYS * 8 / 10);   // Hard nearly always needs more
}

// Fixed dates give fixed puzzles (and the board shows the same: `game peek`). A change here
// means a changed generator: change kSalt's version digit too, on purpose.
static void testGolden() {
  struct Golden {
    long date;
    int level;
    const char *given;
  };
  static const Golden WANT[] = {
      {20261122, 0, "1438..697.89.4.5.22..7..1.....5.1..9.........9..4.3.....2..7..55.1.6.27.374..5961"},
      {20261122, 1, "....6.3.226..18.97.....7.8....87..4....1.5....4..93....5.7.....73.42..611.9.5...."},
      {20261122, 2, "9....15.4...9......4.....29..8.5.3.2.2.7.8.5.4.5.2.7..86.....9......7...7.43....1"},
      {20270101, 0, "..3179..84..68.9..8..2.563.....5...47.8...3.29...2.....594.8..6..2.91..33..5621.."},
      {20270101, 2, "8...6...4.5.8...9..36.15...64...8..9.........1..3...48...45.97..6...9.8.7...8...1"},
      {20280229, 1, "..523..611.......5....8.9.4.6...2.5.5..7.8..3.3.4...8.3.2.7....8.......245..291.."},
  };
  for (const Golden &g : WANT) {
    sg::Puzzle p;
    makeDay(p, g.date, g.level);
    char t[82];
    text(p.given, t);
    if (strcmp(t, g.given)) std::printf("      {%ld, %d, \"%s\"},\n", g.date, g.level, t);
    CHECK(strcmp(t, g.given) == 0);
  }
}

int main() {
  testSolverAndGrader();
  testClashes();
  testYear();
  testGolden();
  std::printf("sudokugrid: %d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
