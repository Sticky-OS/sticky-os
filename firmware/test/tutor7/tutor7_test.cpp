// Host tests for Tutor7's exercise generators (firmware/sticky-os/tutor7_core.cpp) and its words
// (tutor7_text.cpp, src/tutor/).
//   ./run.sh              checks the exact numbers, then makes and verifies thousands of exercises
//                         per kind in every language Tutor has whole, checks that a seed makes the
//                         same exercise in every language, that verify() catches broken ones, and
//                         the tables (placeholders, UTF-8, the fonts' letters) and the lessons
//   ./run.sh samples      also writes build/samples.txt (Romanian) and build/samples_<code>.txt
//                         for the other languages: a few exercises of each kind, to read
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>
#include <vector>
#include <string>
#include "tutor7_core.h"
#include "tutor7_lessons.h"
#include "tutor7_text.h"
#include "i18n.h"
#include "src/fonts/ExtCodes.h"
#define RTS "\xE2\x88\x9A"

using namespace t7;

struct Needle {  // a worked example's text must hold this (Romanian, English): the numbers recomputed by the test
  int slot, page;
  const char *ro, *en;
};
static int failures = 0, checks = 0;
// T7_UNIT=5 or 6 (or 4, 7): only that unit's kinds and lessons, so one unit can be built while another is unfinished.
static int onlyUnit() { const char *e = getenv("T7_UNIT"); return e ? atoi(e) : 0; }
static int kindUnit(int k) { return k >= K_U6_FIRST ? 6 : k >= K_U5_FIRST ? 5 : k >= K_U4_FIRST ? 4 : 7; }
static int slotUnit(int slot) { return slot == 0 ? 7 : slot <= 8 ? 4 : slot <= 14 ? 5 : 6; }
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

// The languages Tutor has whole: the exercises are made and checked in each.
static std::vector<int> contentLangs() {
  std::vector<int> v;
  for (int l = 0; l < LANG_COUNT; l++)
    if (complete(l)) v.push_back(l);
  return v;
}

static void testKind(const Run &r, int lang, FILE *samples) {
  const int SEEDS = 20000;
  int made = 0, rejected = 0, shown = 0;
  std::set<std::string> distinct;
  int correctAt[4] = {0, 0, 0, 0};
  for (int seed = 1; seed <= SEEDS; seed++) {
    Rng rng(seed * 2654435761u);
    Exercise ex;
    if (!generate(r.kind, r.level, rng, ex, lang)) {
      rejected++;
      continue;
    }
    made++;
    char err[120] = "";
    bool ok = verify(ex, err, sizeof err);
    CHECK(ok, "%s %s seed %d: %s | %s", r.name, LANGS[lang].code, seed, err, ex.text);
    CHECK(ex.lang == lang, "%s seed %d: made in %d, asked for %d", r.name, seed, ex.lang, lang);
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
  printf("%s %-26s made %5d, rejected %5d, distinct questions %4zu, right answer at slot 0..3: %d %d %d %d\n", LANGS[lang].code, r.name, made, rejected, distinct.size(),
         correctAt[0], correctAt[1], correctAt[2], correctAt[3]);
  CHECK(made > SEEDS / 20, "%s: only %d of %d seeds gave an exercise", r.name, made, SEEDS);
  CHECK(distinct.size() >= 12, "%s: only %zu different questions", r.name, distinct.size());
  // the right answer should not favour a slot
  if (made > 400 && (r.kind != K_CONVERSE))
    for (int i = 0; i < 4; i++) CHECK(correctAt[i] > made / 8, "%s: the right answer is rarely at slot %d (%d of %d)", r.name, i, correctAt[i], made);
}

// verify() must refuse each of these broken exercises
// verify() must refuse each of these broken exercises
static void testVerifyCatches(int lang) {
  Rng rng(12345);
  Exercise ex;
  while (!generate(K_HYPOTENUSE, 1, rng, ex, lang)) {}
  char err[120];
  CHECK(verify(ex, err, sizeof err), "sample must verify: %s", err);

  Exercise a = ex;
  a.correct = (a.correct + 1) % a.nopt;  // mark a wrong option as right
  CHECK(!verify(a, err, sizeof err), "a wrong key must be caught");

  a = ex;
  a.fig.v[0] += 1;  // the figure no longer matches the key
  CHECK(!verify(a, err, sizeof err), "a figure that disagrees with the key must be caught");

  a = ex;
  char num[32];  // the bare number, without the unit
  exactFormat(ex.value[ex.correct], num, sizeof num);
  snprintf(a.clue[1], sizeof a.clue[1], "%s %s.", tx(lang, TX_HYP_HINT1), num);
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
    snprintf(a.why[wrong], sizeof a.why[wrong], "%s = %s.", ex.why[wrong], num);  // an explanation that gives the answer
    CHECK(!verify(a, err, sizeof err), "a wrong-answer explanation that gives the answer must be caught");
  }

  a = ex;
  a.opt[0][0] = (char)0xC3;  // a broken UTF-8 sequence
  a.opt[0][1] = 0;
  CHECK(!verify(a, err, sizeof err), "broken UTF-8 must be caught");

  // a leg exercise whose 'hypotenuse' is not the longest side
  Rng r2(777);
  Exercise leg;
  while (!generate(K_LEG, 1, r2, leg, lang)) {}
  CHECK(verify(leg, err, sizeof err), "leg sample must verify: %s", err);
  leg.fig.v[2] = leg.fig.v[0];
  CHECK(!verify(leg, err, sizeof err), "a hypotenuse that is not the longest side must be caught");

  // a converse exercise that is not a triangle
  Rng r3(99);
  Exercise cv;
  while (!generate(K_CONVERSE, 3, r3, cv, lang)) {}
  CHECK(verify(cv, err, sizeof err), "converse sample must verify: %s", err);
  cv.fig.v[0] = 1000;  // sides no longer form a triangle
  CHECK(!verify(cv, err, sizeof err), "sides that do not make a triangle must be caught");

  // a classify exercise whose button names another figure than its code
  Rng r4(4242);
  Exercise cl;
  while (!generate(K_CLASSIFY, 3, r4, cl, lang)) {}
  CHECK(verify(cl, err, sizeof err), "classify sample must verify: %s", err);
  strcpy(cl.opt[0], cl.opt[1]);
  CHECK(!verify(cl, err, sizeof err), "a classify option that names the wrong figure must be caught");
}

// A wrong option's explanation must describe how its number is reached. Two slips giving the same number
// are often both right ("2 · 2 = 2²"), so this can't be checked in general: these are the slips that once
// carried a number their reason does not give. They are found by their string (in every language).
static void testSlipReasons(int lang) {
  auto withReason = [lang](const Exercise &ex, int id) {
    for (int i = 0; i < ex.nopt; i++)
      if (i != ex.correct && !strcmp(ex.why[i], tx(lang, id))) return i;
    return -1;
  };
  int seen[3] = {0, 0, 0};
  for (int seed = 1; seed <= 20000; seed++) {
    Exercise ex;
    Rng r1(seed * 2654435761u);
    if (generate(K_PAR_ANGLE, 1, r1, ex, lang)) {  // "taken from 90°" is 90 - A, so only for an acute A
      int A = ex.fig.v[0], i = withReason(ex, TX_PANG_SLIP2);
      if (i >= 0) seen[0]++;
      if (i >= 0) CHECK(ex.fig.v[1] == 1 && A < 90 && ex.value[i].p == 90 - A, "parallelogram angle seed %d: A = %d, \"from 90\" on %s", seed, A, ex.opt[i]);
    }
    Rng r2(seed * 2654435761u);
    if (generate(K_Q_PROPORTIONAL, 2, r2, ex, lang)) {  // "with 180°": 180 · m / S exactly, never truncated
      int S = 0, m = 0, i = withReason(ex, TX_QPROP_SLIP2);
      for (int k = 0; k < 4; k++) S += ex.fig.v[k], m = ex.fig.v[k] > m ? ex.fig.v[k] : m;
      if (i >= 0) seen[1]++;
      if (i >= 0) CHECK(ex.value[i].q == 1 && ex.value[i].p * S == 180 * m, "proportional seed %d: \"180°\" on %s", seed, ex.opt[i]);
    }
    Rng r3(seed * 2654435761u);
    if (generate(K_PERIMETER, 1, r3, ex, lang) && ex.fig.v[0] == 4) {  // dm not converted: 2 · (a + y/10), y in cm
      int a = ex.fig.v[1], y = ex.fig.v[2], i = withReason(ex, TX_PER_SLIP8);
      if (i >= 0) seen[2]++;
      if (i >= 0) CHECK(fabs(exactValue(ex.value[i]) - 2 * (a + y / 10.0)) < 1e-9, "perimeter seed %d: \"not converted\" on %s", seed, ex.opt[i]);
    }
  }
  for (int k = 0; k < 3; k++) CHECK(seen[k] > 50, "%s: slip %d found only %d times (is its string still the one the generator uses?)", LANGS[lang].code, k, seen[k]);
}

// The same seed makes the same exercise in every language: the same success, figure, options, key and
// values. Only the words differ. This is what lets the board switch languages under an open exercise.
static void testSameInEveryLanguage(const Run &r, const std::vector<int> &langs) {
  int differ = 0;
  for (int seed = 1; seed <= 20000; seed++) {
    Exercise a, b;
    Rng ra(seed * 2654435761u);
    bool okA = generate(r.kind, r.level, ra, a, langs[0]);
    for (size_t li = 1; li < langs.size(); li++) {
      Rng rb(seed * 2654435761u);
      bool okB = generate(r.kind, r.level, rb, b, langs[li]);
      bool same = okA == okB && ra.s == rb.s;
      if (same && okA) {
        same = a.kind == b.kind && a.level == b.level && a.fig.kind == b.fig.kind && a.nopt == b.nopt && a.correct == b.correct && a.nslip == b.nslip &&
               !strcmp(a.unit, b.unit);
        for (int k = 0; k < 4; k++) same = same && a.fig.v[k] == b.fig.v[k];
        for (int i = 0; i < a.nopt; i++) same = same && exactSame(a.value[i], b.value[i]);
        // the answers say the same numbers (the converse's and the classify's are words)
        if (r.kind != K_CONVERSE && r.kind != K_CLASSIFY && r.kind != K_THALES_CONVERSE && r.kind != K_SIM_CRITERION)
          for (int i = 0; i < a.nopt; i++) same = same && !strcmp(a.opt[i], b.opt[i]);
      }
      if (!same && differ++ < 5)
        printf("  %s seed %d: %s %s, %s %s\n", r.name, seed, LANGS[langs[0]].code, okA ? "made" : "failed", LANGS[langs[li]].code, okB ? "made" : "failed");
    }
  }
  CHECK(differ == 0, "%s: %d seeds make a different exercise (or none) in another language", r.name, differ);
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

// The first code point the fonts can't draw (ASCII, src/fonts' extra letters, and √ and π, which the screen
// draws itself), or 0. Newlines are paragraph breaks.
static uint32_t missingGlyph(const char *s) {
  const unsigned char *p = (const unsigned char *)s;
  while (*p) {
    uint32_t c;
    int n;
    if (*p < 0x80) c = *p, n = 0;
    else if ((*p >> 5) == 6) c = *p & 0x1F, n = 1;
    else if ((*p >> 4) == 14) c = *p & 0x0F, n = 2;
    else c = *p & 0x07, n = 3;
    p++;
    for (int i = 0; i < n && *p; i++, p++) c = (c << 6) | (*p & 0x3F);
    if (c == '\n' || (c >= 0x20 && c < 0x7F) || c == 0x221A || c == 0x3C0) continue;  // √ and π: the screen draws them
    bool found = false;
    for (uint16_t e : EXT_CODES) found |= e == c;
    if (!found) return c;
  }
  return 0;
}

// The printf conversions of a format, in order: "%d%s%lld".
static std::string specs(const char *s) {
  std::string out;
  for (const char *p = s; *p; p++) {
    if (*p != '%') continue;
    const char *q = p + 1;
    if (*q == '%') {
      p = q;
      continue;
    }
    while (*q && strchr("-+ #0123456789.", *q)) q++;
    while (*q == 'l' || *q == 'h' || *q == 'z') q++;
    out.append(p, q + 1 - p);
    p = q;
  }
  return out;
}

// Every string of every language: UTF-8, letters the fonts have, the English placeholders in order,
// and no "/" used as a division in English-style text (verify reads it as part of a number).
static void testTables() {
  for (int lang = 0; lang < LANG_COUNT; lang++) {
    if (!complete(lang) && lang != L_EN) continue;
    for (int id = 0; id < TX_COUNT; id++) {
      const char *s = tx(lang, id), *en = tx(L_EN, id);
      CHECK(s && utf8ok(s), "%s string %d: bad UTF-8", LANGS[lang].code, id);
      uint32_t g = missingGlyph(s);
      CHECK(!g, "%s string %d: U+%04X is not in the fonts: %s", LANGS[lang].code, id, (unsigned)g, s);
      CHECK(specs(s) == specs(en), "%s string %d: placeholders %s, English has %s: %s", LANGS[lang].code, id, specs(s).c_str(), specs(en).c_str(), s);
    }
  }
}

// Unit 5's and Unit 6's worked examples: needles (NEEDLES_U5/6) and the arithmetic behind them (arithmeticU5/6).
// The files are written with the lessons' words; CHECK and fmt are as above.
#include "needles_u5.h"
#include "needles_u6.h"

// The lessons: every language's words fit the plan (as many pages, none empty, short enough for a
// screen), the plan's exercises can be made, and the worked examples' arithmetic is right.
static void testLessons(const std::vector<int> &langs) {
  for (int li = 0; li < LESSON_COUNT; li++) {
    const Lesson &l = lesson(li);
    if (onlyUnit() && slotUnit(l.slot) != onlyUnit()) continue;
    CHECK(l.slot >= 0 && l.slot < LESSON_COUNT, "lesson %d slot", li);
    for (int lj = 0; lj < li; lj++) CHECK(lesson(lj).slot != l.slot, "lessons %d and %d share slot %d", lj, li, l.slot);
    CHECK(l.nSummary >= 1 && l.nSummary <= MAX_PAGES && l.nExample >= 1 && l.nExample <= MAX_PAGES, "lesson %d page counts", li);
    CHECK(l.nSteps >= 1 && l.nSteps <= MAX_STEPS, "lesson %d step count", li);
    for (int i = 0; i < MAX_PAGES; i++)
      CHECK(l.summaryFigure[i] >= -1 && l.summaryFigure[i] < FIG_COUNT && l.exampleFigure[i] >= -1 && l.exampleFigure[i] < FIG_COUNT, "lesson %d page %d: figure kind", li, i);
    for (int lang : langs) {
      const LessonText &t = lessonText(lang, l.slot);
      const char *code = LANGS[lang].code;
      CHECK(t.name && t.subject && utf8ok(t.name) && utf8ok(t.subject) && !missingGlyph(t.name) && !missingGlyph(t.subject), "%s lesson %d name", code, li);
      auto pages = [&](const PageText *p, int n, const char *what) {
        for (int i = 0; i < MAX_PAGES; i++) {
          if (i >= n) {
            CHECK(!p[i].title && !p[i].body, "%s lesson slot %d: %s page %d is not in the plan (%d pages)", code, l.slot, what, i + 1, n);
            continue;
          }
          CHECK(p[i].title && p[i].body && p[i].title[0] && p[i].body[0], "%s lesson slot %d %s %d is missing or empty", code, l.slot, what, i);
          if (!p[i].title || !p[i].body) continue;
          CHECK(utf8ok(p[i].title) && utf8ok(p[i].body), "%s lesson slot %d %s %d: bad UTF-8", code, l.slot, what, i);
          uint32_t g = missingGlyph(p[i].title) ? missingGlyph(p[i].title) : missingGlyph(p[i].body);
          CHECK(!g, "%s lesson slot %d %s %d: U+%04X is not in the fonts", code, l.slot, what, i, (unsigned)g);
          CHECK(strlen(p[i].body) < 620, "%s lesson slot %d %s %d: body too long for a screen (%zu bytes)", code, l.slot, what, i, strlen(p[i].body));
        }
      };
      pages(t.summary, l.nSummary, "summary");
      pages(t.example, l.nExample, "example");
    }
    // every planned exercise can be made, and verifies, in every language
    for (int i = 0; i < l.nSteps; i++)
      for (int lang : langs) {
        int made = 0;
        for (int seed = 1; seed <= 300; seed++) {
          Rng rng(seed * 40503u + li);
          Exercise ex;
          char err[120];
          if (generate(l.steps[i].kind, l.steps[i].level, rng, ex, lang)) {
            made++;
            CHECK(verify(ex, err, sizeof err), "%s lesson %d step %d: %s", LANGS[lang].code, li, i, err);
          }
        }
        CHECK(made > 100, "%s lesson %d step %d: only %d of 300 seeds made an exercise", LANGS[lang].code, li, i, made);
      }
  }

  // The worked examples: the numbers in the text are recomputed here and looked for in it, in each
  // language's notation. {slot, example page, Romanian, English}
  static const Needle NEEDLES[] = {
      {0, 0, "1681", "1681"}, {0, 0, "41 cm", "41 cm"},
      {0, 1, "= 32", "= 32"}, {0, 1, "4" RTS "2", "4" RTS "2"},
      {0, 2, "841", "841"}, {0, 2, "29 cm", "29 cm"},
      {1, 0, "= 110°", "= 110°"}, {1, 0, "250°", "250°"},
      {1, 1, "72°, 81°, 99° și 108°", "72°, 81°, 99° and 108°"},
      {1, 2, "= 360°", "= 360°"},
      {2, 0, "B = 180° − 70° = 110°", "B = 180° − 70° = 110°"}, {2, 0, "D = 110°", "D = 110°"},
      {2, 1, "BD = 2 · 5 = 10 cm", "BD = 2 × 5 = 10 cm"}, {2, 1, "CO = AO = 3 cm", "CO = AO = 3 cm"},
      {2, 2, "30 − 2 · 6 = 18 cm", "30 − 2 × 6 = 18 cm"}, {2, 2, "câte 9 cm", "each is 9 cm"},
      {3, 0, "MN = BC : 2 = 18 : 2 = 9 cm", "MN = BC ÷ 2 = 18 ÷ 2 = 9 cm"},
      {3, 1, "36 : 2 = 18 cm", "36 ÷ 2 = 18 cm"},
      {3, 2, "(2/3) · 18 = 12 cm", "(2/3) × 18 = 12 cm"}, {3, 2, "(1/3) · 18 = 6 cm", "(1/3) × 18 = 6 cm"},
      {4, 0, "AC = 2 · AO = 14 cm", "AC = 2 × AO = 14 cm"}, {4, 0, "BD = AC = 14 cm", "BD = AC = 14 cm"},
      {4, 1, "180° − 68° = 112°", "180° − 68° = 112°"}, {4, 1, "= 56°", "= 56°"},
      {4, 2, "3 · 9 = 27 cm", "3 × 9 = 27 cm"},
      {5, 0, "2 · 35° = 70°", "2 × 35° = 70°"}, {5, 0, "180° − 70° = 110°", "180° − 70° = 110°"},
      {5, 1, "36 + 64 = 100", "36 + 64 = 100"}, {5, 1, "4 · 10 = 40 cm", "4 × 10 = 40 cm"},
      {5, 2, "l² = 50", "s² = 50"}, {5, 2, "5" RTS "2 cm", "5" RTS "2 cm"},
      {6, 0, "D = 180° − 70° = 110°", "D = 180° − 70° = 110°"}, {6, 0, "B = 180° − 100° = 80°", "B = 180° − 100° = 80°"},
      {6, 1, "MN = (12 + 6) : 2 = 9 cm", "MN = (12 + 6) ÷ 2 = 9 cm"}, {6, 1, "PQ = (12 − 6) : 2 = 3 cm", "PQ = (12 − 6) ÷ 2 = 3 cm"},
      {6, 2, "100 − 36 = 64", "100 − 36 = 64"}, {6, 2, "h = 8 cm", "h = 8 cm"},
      {7, 0, "A = 10 · 6 = 60 cm²", "A = 10 × 6 = 60 cm²"}, {7, 0, "P = 2 · (10 + 7) = 34 cm", "P = 2 × (10 + 7) = 34 cm"},
      {7, 1, "40 cm", "40 cm"}, {7, 1, "96 cm²", "96 cm²"},
      {7, 2, "0,7 dm = 7 cm", "0.7 dm = 7 cm"}, {7, 2, "68 : 2 = 34 cm²", "68 ÷ 2 = 34 cm²"},
      {8, 1, "B = 60° și A = 120°", "B = 60° and A = 120°"},
      {8, 2, "h = 48 : 10 = 4,8 cm", "h = 48 ÷ 10 = 4.8 cm"},
  };
  // the arithmetic behind them
  CHECK(9 * 9 + 40 * 40 == 1681 && 41 * 41 == 1681 && 9 * 9 - 7 * 7 == 32 && fmt(exactSqrt(32)) == "4" RTS "2", "Pythagoras examples 1-2");
  CHECK(20 * 20 + 21 * 21 == 841 && 29 * 29 == 841, "Pythagoras example 3");
  CHECK(360 - (70 + 100 + 80) == 110 && 8 + 9 + 11 + 12 == 40 && 360 / 40 == 9 && 8 * 9 == 72 && 11 * 9 == 99 && 12 * 9 == 108 && 60 + 105 + 90 + 105 == 360, "4.1");
  CHECK(180 - 70 == 110 && 2 * 5 == 10 && 30 - 2 * 6 == 18 && 18 / 2 == 9, "4.2");
  CHECK(10 + 14 + 12 == 36 && 10 + 12 > 14 && 2 * 18 / 3 == 12 && 18 / 3 == 6, "4.3");
  CHECK(2 * 7 == 14 && 180 - 68 == 112 && 112 / 2 == 56 && 3 * 9 == 27, "4.4");
  CHECK(2 * 35 == 70 && 6 * 6 + 8 * 8 == 100 && fmt(exactSqrt(50)) == "5" RTS "2", "4.5");
  CHECK(180 - 100 == 80 && (12 + 6) / 2 == 9 && (12 - 6) / 2 == 3 && (20 - 8) / 2 == 6 && 10 * 10 - 6 * 6 == 64, "4.6");
  CHECK(10 * 6 == 60 && 2 * (10 + 7) == 34 && 12 * 16 / 2 == 96 && (10 + 7) * 4 / 2 == 34, "4.7");
  CHECK(3 * 60 == 180 && 6 * 8 / 2 == 24 && 48.0 / 10 == 4.8, "4.8");
  if (!onlyUnit() || onlyUnit() == 4 || onlyUnit() == 7) {
  for (const Needle &n : NEEDLES)
    for (int lang : langs) {
      const char *want = lang == L_RO ? n.ro : lang == L_EN ? n.en : nullptr;
      if (!want) continue;  // a new language: add its column
      const char *body = lessonText(lang, n.slot).example[n.page].body;
      CHECK(body && strstr(body, want), "%s slot %d example %d: \"%s\" not found", LANGS[lang].code, n.slot, n.page + 1, want);
    }
  }
  // Units 5 and 6: each has its own file with the needles and the arithmetic behind them
  auto needles = [&](const Needle *ns, size_t n, int unit) {
    if (onlyUnit() && onlyUnit() != unit) return;
    for (size_t i = 0; i < n; i++)
      for (int lang : langs) {
        const char *want = lang == L_RO ? ns[i].ro : lang == L_EN ? ns[i].en : nullptr;
        if (!want) continue;
        const char *body = lessonText(lang, ns[i].slot).example[ns[i].page].body;
        CHECK(body && strstr(body, want), "%s slot %d example %d: \"%s\" not found", LANGS[lang].code, ns[i].slot, ns[i].page + 1, want);
      }
  };
  needles(NEEDLES_U5, sizeof NEEDLES_U5 / sizeof NEEDLES_U5[0], 5);
  needles(NEEDLES_U6, sizeof NEEDLES_U6 / sizeof NEEDLES_U6[0], 6);
  if (!onlyUnit() || onlyUnit() == 5) arithmeticU5();
  if (!onlyUnit() || onlyUnit() == 6) arithmeticU6();
  CHECK(strstr(lessonText(L_RO, 0).summary[2].body, "16·2") && strstr(lessonText(L_EN, 0).summary[2].body, "16 × 2"), "the simplification shown on Pythagoras page 3");
  // the content language: a complete language is itself, any other falls back to English
  CHECK(contentLang(L_RO) == L_RO && contentLang(L_EN) == L_EN && contentLang(L_DE) == L_EN && contentLang(-1) == L_EN, "contentLang");
}

int main(int argc, char **argv) {
  setvbuf(stdout, nullptr, _IONBF, 0);
  bool wantSamples = argc > 1 && !strcmp(argv[1], "samples");
  testExact();
  std::vector<int> langs = contentLangs();
  CHECK(langs.size() >= 2 && complete(L_EN) && complete(L_RO), "English and Romanian are complete");
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
#include "runs_u5.h"
#include "runs_u6.h"
  };
  std::vector<Run> chosen;
  for (const Run &r : runs)
    if (!onlyUnit() || kindUnit(r.kind) == onlyUnit()) chosen.push_back(r);
  for (int lang : langs) {
    FILE *samples = nullptr;
    if (wantSamples) {
      char path[64];
      if (lang == L_RO) snprintf(path, sizeof path, "%s/samples.txt", getenv("BUILD") ? getenv("BUILD") : "build");  // as before the other languages came
      else snprintf(path, sizeof path, "%s/samples_%s.txt", getenv("BUILD") ? getenv("BUILD") : "build", LANGS[lang].code);
      samples = fopen(path, "w");
    }
    for (const Run &r : chosen) testKind(r, lang, samples);
    if (samples) fclose(samples);
    testVerifyCatches(lang);
    testSlipReasons(lang);
  }
  for (const Run &r : chosen) testSameInEveryLanguage(r, langs);
  testTables();
  testLessons(langs);
  if (wantSamples) printf("samples written to build/samples*.txt\n");
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
