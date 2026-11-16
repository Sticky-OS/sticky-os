// Host tests for Tutor7's exercise generators (firmware/sticky-os/tutor7_core.cpp).
//   ./run.sh              checks the exact numbers, then makes and verifies thousands of
//                         exercises per kind, then checks that verify() catches broken ones
//   ./run.sh samples      also writes build/samples.txt: a few exercises of each kind, to read
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <string>
#include "tutor7_core.h"

using namespace t7;

static int failures = 0, checks = 0;
#define CHECK(cond, ...)                                     \
  do {                                                       \
    checks++;                                                \
    if (!(cond)) {                                           \
      failures++;                                            \
      if (failures <= 40) {                                  \
        printf("FAIL %s:%d: %s  ", __FILE__, __LINE__, #cond); \
        printf(__VA_ARGS__);                                 \
        printf("\n");                                        \
      }                                                      \
    }                                                        \
  } while (0)

static std::string fmt(const Exact &e) {
  char b[48];
  return exactFormat(e, b, sizeof b) < 0 ? "?" : b;
}

static void testExact() {
  struct { int64_t n; const char *want; } sqrts[] = {
      {1, "1"},    {4, "2"},    {169, "13"},  {2, "\xE2\x88\x9A" "2"},  {8, "2\xE2\x88\x9A" "2"},
      {12, "2\xE2\x88\x9A" "3"}, {32, "4\xE2\x88\x9A" "2"}, {80, "4\xE2\x88\x9A" "5"}, {75, "5\xE2\x88\x9A" "3"},
      {50, "5\xE2\x88\x9A" "2"}, {0, "0"},    {7, "\xE2\x88\x9A" "7"},  {1681, "41"},
  };
  for (auto &t : sqrts) CHECK(fmt(exactSqrt(t.n)) == t.want, "sqrt(%lld) gave %s, want %s", (long long)t.n, fmt(exactSqrt(t.n)).c_str(), t.want);
  CHECK(fmt(exactFrac(6, 4)) == "3/2", "6/4");
  CHECK(fmt(exactFrac(-6, -4)) == "3/2", "-6/-4");
  CHECK(fmt(exactFrac(8, 4)) == "2", "8/4");
  Exact h = exactSqrt(27);  // 3 sqrt(3)
  CHECK(h.p == 3 && h.r == 3 && h.q == 1, "sqrt 27");
  // every square-free reduction must satisfy p^2 * r == n * q^2 (here q = 1), and r must be square-free
  for (int64_t n = 1; n <= 20000; n++) {
    Exact e = exactSqrt(n);
    CHECK((int64_t)e.p * e.p * e.r == n && e.q == 1, "sqrt(%lld) = %s does not square back", (long long)n, fmt(e).c_str());
    for (int64_t d = 2; d * d <= e.r; d++) CHECK(e.r % (d * d) != 0, "sqrt(%lld): r = %d is not square-free", (long long)n, e.r);
  }
  char tiny[3];
  CHECK(exactFormat(exactSqrt(80), tiny, sizeof tiny) == -1, "a too-small buffer must be reported");
}

struct Run {
  Kind kind;
  int level;
  const char *name;
};

static void testKind(const Run &r, FILE *samples) {
  const int SEEDS = 20000;
  int made = 0, rejected = 0, shown = 0;
  std::set<std::string> distinct;
  int correctAt[4] = {0, 0, 0, 0};
  for (int seed = 1; seed <= SEEDS; seed++) {
    Rng rng(seed * 2654435761u);
    Exercise ex;
    if (!generate(r.kind, r.level, rng, ex)) {
      rejected++;
      continue;
    }
    made++;
    char err[120] = "";
    bool ok = verify(ex, err, sizeof err);
    CHECK(ok, "%s seed %d: %s | %s", r.name, seed, err, ex.text);
    distinct.insert(ex.text);
    if (ex.correct >= 0 && ex.correct < 4) correctAt[ex.correct]++;
    if (samples && shown < 3 && seed > shown * 997) {
      shown++;
      fprintf(samples, "==== %s (seed %d)\n%s\n", r.name, seed, ex.text);
      for (int i = 0; i < ex.nopt; i++) fprintf(samples, "  %c %s%s\n", i == ex.correct ? '*' : ' ', ex.opt[i], i == ex.correct ? "   <- corect" : "");
      for (int i = 0; i < ex.nopt; i++)
        if (i != ex.correct) fprintf(samples, "    daca alege \"%s\": %s\n", ex.opt[i], ex.why[i]);
      fprintf(samples, "  indiciu 1: %s\n  indiciu 2: %s\n  rezolvare: %s\n\n", ex.clue[0], ex.clue[1], ex.solution);
    }
  }
  printf("%-26s made %5d, rejected %5d, distinct questions %4zu, right answer at slot 0..3: %d %d %d %d\n", r.name, made, rejected, distinct.size(),
         correctAt[0], correctAt[1], correctAt[2], correctAt[3]);
  CHECK(made > SEEDS / 20, "%s: only %d of %d seeds gave an exercise", r.name, made, SEEDS);
  CHECK(distinct.size() >= 12, "%s: only %zu different questions", r.name, distinct.size());
  // the right answer should not favour a slot
  if (made > 400 && (r.kind != K_CONVERSE))
    for (int i = 0; i < 4; i++) CHECK(correctAt[i] > made / 8, "%s: the right answer is rarely at slot %d (%d of %d)", r.name, i, correctAt[i], made);
}

// verify() must refuse each of these broken exercises
static void testVerifyCatches() {
  Rng rng(12345);
  Exercise ex;
  CHECK(generate(K_HYPOTENUSE, 1, rng, ex) || generate(K_HYPOTENUSE, 1, rng, ex) || generate(K_HYPOTENUSE, 1, rng, ex), "need a sample");
  while (!generate(K_HYPOTENUSE, 1, rng, ex)) {}
  char err[120];
  CHECK(verify(ex, err, sizeof err), "sample must verify: %s", err);

  Exercise a = ex;
  a.correct = (a.correct + 1) % a.nopt;  // mark a wrong option as right
  CHECK(!verify(a, err, sizeof err), "a wrong key must be caught");

  a = ex;
  a.fig.v[0] += 1;  // the figure no longer matches the key
  CHECK(!verify(a, err, sizeof err), "a figure that disagrees with the key must be caught");

  a = ex;
  snprintf(a.clue[1], sizeof a.clue[1], "Raspunsul este %s.", ex.opt[ex.correct]);
  // opt has a unit; take the bare number
  char num[32];
  exactFormat(ex.value[ex.correct], num, sizeof num);
  snprintf(a.clue[1], sizeof a.clue[1], "Rezultatul este %s.", num);
  CHECK(!verify(a, err, sizeof err), "a clue that gives the answer must be caught");

  a = ex;
  int w = (a.correct + 1) % a.nopt;
  strcpy(a.opt[w], a.opt[a.correct]);  // two identical buttons
  CHECK(!verify(a, err, sizeof err), "two identical options must be caught");

  a = ex;
  a.why[a.correct][0] = 'x';  // a reason on the right answer
  CHECK(!verify(a, err, sizeof err), "a reason on the right option must be caught");

  a = ex;
  {
    int wrong = (a.correct + 1) % a.nopt;
    char ans[40];
    exactFormat(ex.value[ex.correct], ans, sizeof ans);
    snprintf(a.why[wrong], sizeof a.why[wrong], "Gresit. Corect este %s.", ans);  // an explanation that gives the answer
    CHECK(!verify(a, err, sizeof err), "a wrong-answer explanation that gives the answer must be caught");
  }

  a = ex;
  a.opt[0][0] = (char)0xC3;  // a broken UTF-8 sequence
  a.opt[0][1] = 0;
  CHECK(!verify(a, err, sizeof err), "broken UTF-8 must be caught");

  // a leg exercise whose 'hypotenuse' is not the longest side
  Rng r2(777);
  Exercise leg;
  while (!generate(K_LEG, 1, r2, leg)) {}
  CHECK(verify(leg, err, sizeof err), "leg sample must verify: %s", err);
  leg.fig.v[2] = leg.fig.v[0];
  CHECK(!verify(leg, err, sizeof err), "a hypotenuse that is not the longest side must be caught");

  // a converse exercise that is not a triangle
  Rng r3(99);
  Exercise cv;
  while (!generate(K_CONVERSE, 3, r3, cv)) {}
  CHECK(verify(cv, err, sizeof err), "converse sample must verify: %s", err);
  cv.fig.v[0] = 1000;  // sides no longer form a triangle
  CHECK(!verify(cv, err, sizeof err), "sides that do not make a triangle must be caught");
  cv = ex;  // reset
}

int main(int argc, char **argv) {
  setvbuf(stdout, nullptr, _IONBF, 0);
  bool wantSamples = argc > 1 && !strcmp(argv[1], "samples");
  testExact();
  FILE *samples = wantSamples ? fopen("build/samples.txt", "w") : nullptr;
  const Run runs[] = {
      {K_HYPOTENUSE, 1, "hypotenuse, integers"}, {K_HYPOTENUSE, 2, "hypotenuse, radicals"}, {K_LEG, 1, "leg, integers"},
      {K_LEG, 2, "leg, radicals"},               {K_CONVERSE, 3, "converse"},               {K_SQUARE_DIAGONAL, 3, "square diagonal"},
      {K_EQUILATERAL_HEIGHT, 3, "equilateral height"},
  };
  for (const Run &r : runs) testKind(r, samples);
  testVerifyCatches();
  if (samples) {
    fclose(samples);
    printf("samples written to build/samples.txt\n");
  }
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
