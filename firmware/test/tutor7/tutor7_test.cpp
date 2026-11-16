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
#include "tutor7_lessons.h"
#define RTS "\xE2\x88\x9A"

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
    CHECK(ex.nslip >= 0 && ex.nslip <= MAX_SLIPS, "%s seed %d: %d slips", r.name, seed, ex.nslip);
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

// A wrong option's explanation must describe how its number is reached. Two slips giving the same number
// are often both right ("2 · 2 = 2²"), so this can't be checked in general: these are the slips that once
// carried a number their reason does not give.
static void testSlipReasons() {
  auto withReason = [](const Exercise &ex, const char *words) {
    for (int i = 0; i < ex.nopt; i++)
      if (i != ex.correct && strstr(ex.why[i], words)) return i;
    return -1;
  };
  for (int seed = 1; seed <= 20000; seed++) {
    Exercise ex;
    Rng r1(seed * 2654435761u);
    if (generate(K_PAR_ANGLE, 1, r1, ex)) {  // "taken from 90°" is 90 - A, so only for an acute A
      int A = ex.fig.v[0], i = withReason(ex, "Ai scăzut din 90");
      if (ex.fig.v[1] == 1 && i >= 0) CHECK(A < 90 && ex.value[i].p == 90 - A, "parallelogram angle seed %d: A = %d, \"from 90\" on %s", seed, A, ex.opt[i]);
    }
    Rng r2(seed * 2654435761u);
    if (generate(K_Q_PROPORTIONAL, 2, r2, ex)) {  // "with 180°": 180 · m / S exactly, never truncated
      int S = 0, m = 0, i = withReason(ex, "Ai folosit suma de 180");
      for (int k = 0; k < 4; k++) S += ex.fig.v[k], m = ex.fig.v[k] > m ? ex.fig.v[k] : m;
      if (i >= 0) CHECK(ex.value[i].q == 1 && ex.value[i].p * S == 180 * m, "proportional seed %d: \"180°\" on %s", seed, ex.opt[i]);
    }
    Rng r3(seed * 2654435761u);
    if (generate(K_PERIMETER, 1, r3, ex) && ex.fig.v[0] == 4) {  // dm not converted: 2 · (a + y/10), y in cm
      int a = ex.fig.v[1], y = ex.fig.v[2], i = withReason(ex, "fără să transformi");
      if (i >= 0) CHECK(fabs(exactValue(ex.value[i]) - 2 * (a + y / 10.0)) < 1e-9, "perimeter seed %d: \"not converted\" on %s", seed, ex.opt[i]);
    }
  }
}

static bool utf8ok(const char *s) {
  const unsigned char *p = (const unsigned char *)s;
  while (*p) {
    int n = *p < 0x80 ? 0 : (*p >> 5) == 6 ? 1 : (*p >> 4) == 14 ? 2 : (*p >> 3) == 30 ? 3 : -1;
    if (n < 0) return false;
    p++;
    for (int i = 0; i < n; i++, p++)
      if ((*p & 0xC0) != 0x80) return false;
  }
  return true;
}

// The lessons' text: well-formed, short enough for a screen, and the worked examples' arithmetic is right.
static void testLessons() {
  for (int li = 0; li < LESSON_COUNT; li++) {
    const Lesson &l = lesson(li);
    CHECK(l.name && l.subject && utf8ok(l.name) && utf8ok(l.subject), "lesson %d name", li);
    CHECK(l.nSummary >= 1 && l.nSummary <= MAX_PAGES && l.nExample >= 1 && l.nExample <= MAX_PAGES, "lesson %d page counts", li);
    CHECK(l.nSteps >= 1 && l.nSteps <= MAX_STEPS, "lesson %d step count", li);
    auto page = [&](const Page &p, const char *what, int i) {
      CHECK(p.title && p.body && p.title[0] && p.body[0], "lesson %d %s %d is empty", li, what, i);
      CHECK(utf8ok(p.title) && utf8ok(p.body), "lesson %d %s %d: bad UTF-8", li, what, i);
      CHECK(strlen(p.body) < 620, "lesson %d %s %d: body too long for a screen (%zu bytes)", li, what, i, strlen(p.body));
      CHECK(p.figure >= -1 && p.figure < FIG_COUNT, "lesson %d %s %d: figure kind", li, what, i);
    };
    for (int i = 0; i < l.nSummary; i++) page(l.summary[i], "summary", i);
    for (int i = 0; i < l.nExample; i++) page(l.example[i], "example", i);
    // every planned exercise can be made, and verifies
    for (int i = 0; i < l.nSteps; i++) {
      int made = 0;
      for (int seed = 1; seed <= 300; seed++) {
        Rng rng(seed * 40503u + li);
        Exercise ex;
        char err[120];
        if (generate(l.steps[i].kind, l.steps[i].level, rng, ex)) {
          made++;
          CHECK(verify(ex, err, sizeof err), "lesson %d step %d: %s", li, i, err);
        }
      }
      CHECK(made > 100, "lesson %d step %d: only %d of 300 seeds made an exercise", li, i, made);
    }
  }
  // The Pythagoras lesson's worked examples, recomputed here and looked for in the text.
  const Lesson &p = lesson(LESSON_COUNT - 1);  // Pitagora, the last in the list
  auto has = [](const char *text, const char *needle) { return strstr(text, needle) != nullptr; };
  CHECK(9 * 9 + 40 * 40 == 1681 && 41 * 41 == 1681, "example 1 arithmetic");
  CHECK(has(p.example[0].body, "1681") && has(p.example[0].body, "41 cm"), "example 1 text");
  CHECK(9 * 9 - 7 * 7 == 32 && fmt(exactSqrt(32)) == "4\xE2\x88\x9A" "2", "example 2 arithmetic");
  CHECK(has(p.example[1].body, "= 32") && has(p.example[1].body, "4\xE2\x88\x9A" "2"), "example 2 text");
  CHECK(20 * 20 + 21 * 21 == 841 && 29 * 29 == 841, "example 3 arithmetic");
  CHECK(has(p.example[2].body, "841") && has(p.example[2].body, "29 cm"), "example 3 text");
  CHECK(16 * 2 == 32 && has(p.summary[2].body, "16\xC2\xB7" "2"), "the simplification shown on page 3 is right");
  // Unit 4's worked examples: the numbers in the text are recomputed here and looked for in it.
  auto lessonNamed = [&](const char *name) -> const Lesson * {
    for (int i = 0; i < LESSON_COUNT; i++)
      if (!strcmp(lesson(i).name, name)) return &lesson(i);
    return nullptr;
  };
  const Lesson *q = lessonNamed("Patrulaterul convex");
  CHECK(q && 360 - (70 + 100 + 80) == 110 && has(q->example[0].body, "= 110°") && has(q->example[0].body, "250°"), "U4 L1 example 1");
  CHECK(q && 8 + 9 + 11 + 12 == 40 && 360 / 40 == 9 && 8 * 9 == 72 && 9 * 9 == 81 && 11 * 9 == 99 && 12 * 9 == 108 && has(q->example[1].body, "72°, 81°, 99° și 108°"), "U4 L1 example 2");
  CHECK(q && 60 + 105 + 90 + 105 == 360 && has(q->example[2].body, "= 360°"), "U4 L1 example 3");
  const Lesson *pl = lessonNamed("Paralelogramul");
  CHECK(pl && 180 - 70 == 110 && has(pl->example[0].body, "B = 180° − 70° = 110°") && has(pl->example[0].body, "D = 110°"), "U4 L2 example 1");
  CHECK(pl && 2 * 5 == 10 && has(pl->example[1].body, "BD = 2 · 5 = 10 cm") && has(pl->example[1].body, "CO = AO = 3 cm"), "U4 L2 example 2");
  CHECK(pl && 30 - 2 * 6 == 18 && 18 / 2 == 9 && has(pl->example[2].body, "30 − 2 · 6 = 18 cm") && has(pl->example[2].body, "câte 9 cm"), "U4 L2 example 3");
  const Lesson *ml = lessonNamed("Linia mijlocie");
  CHECK(ml && 18 / 2 == 9 && has(ml->example[0].body, "MN = BC : 2 = 18 : 2 = 9 cm"), "U4 L3 example 1");
  CHECK(ml && 10 + 14 + 12 == 36 && 36 / 2 == 18 && 10 + 12 > 14 && has(ml->example[1].body, "36 : 2 = 18 cm"), "U4 L3 example 2");
  CHECK(ml && 2 * 18 / 3 == 12 && 18 / 3 == 6 && 12 + 6 == 18 && has(ml->example[2].body, "(2/3) · 18 = 12 cm") && has(ml->example[2].body, "(1/3) · 18 = 6 cm"), "U4 L3 example 3");
  const Lesson *re = lessonNamed("Dreptunghiul");
  CHECK(re && 2 * 7 == 14 && has(re->example[0].body, "AC = 2 · AO = 14 cm") && has(re->example[0].body, "BD = AC = 14 cm"), "U4 L4 example 1");
  CHECK(re && 180 - 68 == 112 && 112 / 2 == 56 && has(re->example[1].body, "180° − 68° = 112°") && has(re->example[1].body, "= 56°"), "U4 L4 example 2");
  CHECK(re && 18 / 2 == 9 && 3 * 9 == 27 && has(re->example[2].body, "perimetrul este 3 · 9 = 27 cm"), "U4 L4 example 3");
  const Lesson *ro = lessonNamed("Rombul și pătratul");
  CHECK(ro && 2 * 35 == 70 && 180 - 70 == 110 && has(ro->example[0].body, "2 · 35° = 70°") && has(ro->example[0].body, "180° − 70° = 110°"), "U4 L5 example 1");
  CHECK(ro && 6 * 6 + 8 * 8 == 100 && 4 * 10 == 40 && has(ro->example[1].body, "36 + 64 = 100") && has(ro->example[1].body, "4 · 10 = 40 cm"), "U4 L5 example 2");
  CHECK(ro && fmt(exactSqrt(50)) == "5\xE2\x88\x9A" "2" && has(ro->example[2].body, "l² = 50") && has(ro->example[2].body, "5" RTS "2 cm"), "U4 L5 example 3");
  const Lesson *tr = lessonNamed("Trapezul");
  CHECK(tr && 180 - 70 == 110 && 180 - 100 == 80 && has(tr->example[0].body, "D = 180° − 70° = 110°") && has(tr->example[0].body, "B = 180° − 100° = 80°"), "U4 L6 example 1");
  CHECK(tr && (12 + 6) / 2 == 9 && (12 - 6) / 2 == 3 && has(tr->example[1].body, "MN = (12 + 6) : 2 = 9 cm") && has(tr->example[1].body, "PQ = (12 − 6) : 2 = 3 cm"), "U4 L6 example 2");
  CHECK(tr && (20 - 8) / 2 == 6 && 10 * 10 - 6 * 6 == 64 && 8 * 8 == 64 && has(tr->example[2].body, "100 − 36 = 64") && has(tr->example[2].body, "h = 8 cm"), "U4 L6 example 3");
  const Lesson *ar = lessonNamed("Perimetre și arii");
  CHECK(ar && 10 * 6 == 60 && 2 * (10 + 7) == 34 && has(ar->example[0].body, "A = 10 · 6 = 60 cm²") && has(ar->example[0].body, "P = 2 · (10 + 7) = 34 cm"), "U4 L7 example 1");
  CHECK(ar && 6 * 6 + 8 * 8 == 100 && 12 * 16 / 2 == 96 && has(ar->example[1].body, "40 cm") && has(ar->example[1].body, "96 cm²"), "U4 L7 example 2");
  CHECK(ar && (10 + 7) * 4 / 2 == 34 && has(ar->example[2].body, "0,7 dm = 7 cm") && has(ar->example[2].body, "68 : 2 = 34 cm²"), "U4 L7 example 3");
  const Lesson *rv = lessonNamed("Recapitulare U4");
  CHECK(rv && 3 * 60 == 180 && has(rv->example[1].body, "B = 60° și A = 120°"), "U4 review example 2");
  CHECK(rv && 6 * 8 / 2 == 24 && 48 * 10 == 480 && 48.0 / 10 == 4.8 && has(rv->example[2].body, "h = 48 : 10 = 4,8 cm"), "U4 review example 3");
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
      {K_Q_FOURTH_ANGLE, 1, "fourth angle"},        {K_Q_PROPORTIONAL, 2, "proportional angles"}, {K_PAR_ANGLE, 1, "parallelogram angle"},
      {K_PAR_PERIMETER, 1, "parallelogram perimeter"}, {K_PAR_DIAGONAL, 1, "parallelogram diagonal"}, {K_CLASSIFY, 3, "classify"},
      {K_MIDLINE, 1, "midline"},                    {K_MEDIAN_TRIANGLE, 2, "median triangle"},    {K_CENTROID, 2, "centroid"},
      {K_RECT_DIAGONAL, 1, "rectangle diagonal"},   {K_RECT_ANGLE, 2, "rectangle angle"},         {K_RHOMBUS_ANGLE, 2, "rhombus angle"},
      {K_RHOMBUS_SIDE, 1, "rhombus side, ints"},    {K_RHOMBUS_SIDE, 2, "rhombus side, radicals"}, {K_SQUARE_SIDE, 3, "square side"},
      {K_TRAP_ANGLE, 1, "trapezoid angle"},         {K_TRAP_MIDLINE, 2, "trapezoid midline"},     {K_TRAP_HEIGHT, 1, "trapezoid height, ints"},
      {K_TRAP_HEIGHT, 2, "trapezoid height, radicals"}, {K_AREA, 2, "area"},                      {K_PERIMETER, 1, "perimeter"},
      {K_HEIGHT_FROM_AREA, 1, "height from area"},  {K_HEIGHT_FROM_AREA, 2, "height, right triangle"},
  };
  for (const Run &r : runs) testKind(r, samples);
  testVerifyCatches();
  testSlipReasons();
  testLessons();
  if (samples) {
    fclose(samples);
    printf("samples written to build/samples.txt\n");
  }
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
