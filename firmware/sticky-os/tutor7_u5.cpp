// Tutor7, Unit 5 (the circle): the exercise generators and the answers worked out a second way.
//
// Same rules as tutor7_u4.cpp. Every generator draws numbers constructively (so the answer is whole, or a clean
// radical), the options are the typical slips, and u5Expect() derives the answer again from ex.fig.v alone, in a
// different form (doubles, sqrt, Heron's formula, the arcs of a cyclic quadrilateral) for verify() to compare.
// Answers with pi are integer multiples of pi: the option "12π cm" is the number 12 with the unit "π cm".
//
// fig.v layout (what the screen draws is documented in tutor7_core.h; the rest is for u5Expect):
//   K_ARC_CENTRAL     v1 = variant. 0, 1: FIG_CIRCLE_ARC v0 = the central angle. 2: FIG_NONE v0, v2, v3 = three arcs.
//                     3: FIG_NONE v0, v2, v3 = the ratio, v4 = which arc (0..2). 4: FIG_NONE v0 = an arc of a semicircle.
//   K_CHORD_DISTANCE  FIG_CHORD v0 = radius, v1 = distance (both times 1 for integer exercises, times 100 and rounded when the
//                     radius or the distance is a radical: only the ratio matters), v2 = variant (0 chord, 1 distance,
//                     2 radius), v3 = r, v4 = d, v5 = chord (the true integer givens, 0 for the unknown).
//   K_INSCRIBED       FIG_INSCRIBED v0 = the arc BC the angle at A looks at, v1 = variant, v2 = the other given.
//   K_CYCLIC_QUAD     FIG_CYCLIC v0..3 = arcs AB, BC, CD, DA, v4 = variant, v5 = asked vertex + 4 * first given vertex.
//   K_TANGENT_LENGTH  FIG_TANGENT v0 = radius, v1 = OA (scaled like the chord's), v2 = variant (0 AC, 1 OA, 2 radius),
//                     v3 = r, v4 = OA, v5 = AC (true givens, 0 for the unknown).
//   K_CIRCUMSCRIBED   FIG_NONE v0..3 = AB, BC, CD, DA (all four, the unknown included), v4 = variant, v5 = the unknown side.
//   K_INCIRCLE_AREA   FIG_NONE v0..2 = the sides, v3 = variant, v4 = the radius.
//   K_REGULAR_POLYGON FIG_REGULAR v0 = n, v1 = variant, v2, v3 = the other givens (see u5Expect).
//   K_CIRCUMFERENCE, K_DISC_AREA   FIG_NONE v0 = variant, v1, v2 = givens.
//   K_ARC_SECTOR      FIG_SECTOR v0 = the angle, v1 = variant, v2 = the radius, v3 = the given arc length or sector area.
//   K_ANNULUS         FIG_ANNULUS v0 = R, v1 = r, v2 = variant.
#include "tutor7_internal.h"

namespace t7 {

static int mult(Rng &r, int step, int lo, int hi) { return step * r.range((lo + step - 1) / step, hi / step); }

// Finishes an exercise: the options, and a last look that nothing but the solution states the answer.
static bool finish(Exercise &ex, Rng &rng, bool bad, const Exact &right, Cand *c, int nc, const char *unit) {
  if (bad || !setOptions(ex, rng, right, c, nc, unit)) return false;
  char tok[48];
  if (exactFormat(right, tok, sizeof tok) < 0) return false;
  for (int i = 0; i < ex.nopt; i++)
    if (i != ex.correct && hasToken(ex.why[i], tok)) return false;
  for (int k = 0; k < 2; k++)
    if (hasToken(ex.clue[k], tok)) return false;
  return hasToken(ex.solution, tok);
}

// "√80 = 4√5", or just "√7" when the root is already as simple as it gets, or "√16 = 4"
static void rootText(const Exact &e, int64_t n, char *out, int cap) {
  char f[32];
  exactFormat(e, f, sizeof f);
  if (e.r == n && e.p == 1 && e.q == 1) snprintf(out, cap, RT "%lld", (long long)n);
  else snprintf(out, cap, RT "%lld = %s", (long long)n, f);
}

static Exact twice(const Exact &e) { return exactNorm(2LL * e.p, e.r, e.q); }

// A Pythagorean triple (legs a, b in random order, hypotenuse c, c up to about 30).
static void pickTriple(Rng &rng, int &a, int &b, int &c) {
  static const int T[5][3] = {{3, 4, 5}, {5, 12, 13}, {8, 15, 17}, {7, 24, 25}, {20, 21, 29}};
  int i = rng.range(0, 4);
  int k = rng.range(1, 30 / T[i][2]);
  a = T[i][0] * k, b = T[i][1] * k, c = T[i][2] * k;
  if (rng.range(0, 1)) {
    int t = a;
    a = b;
    b = t;
  }
}

static const char *VN[4] = {"A", "B", "C", "D"};
static const char *ARCN[4] = {"AB", "BC", "CD", "DA"};
// A slip that is a radical is left out of an integer exercise (level 1 shows whole numbers only); a coefficient 1 of pi
// ("1π cm") is never offered.
#define SLIPR(list, n, value, lvl, ...)                      \
  do {                                                       \
    Exact e_ = (value);                                      \
    if ((lvl) > 1 || e_.r == 1) SLIP(list, n, e_, __VA_ARGS__); \
  } while (0)
#define SLIP1(list, n, value, ...)                              \
  do {                                                          \
    Exact e_ = (value);                                         \
    if (!(e_.r == 1 && e_.q == 1 && e_.p == 1)) SLIP(list, n, e_, __VA_ARGS__); \
  } while (0)
#define CM " cm"
#define CM2 " cm" SQ
#define PCM PI_ " cm"
#define PCM2 PI_ " cm" SQ

// ---------------------------------------------------------------- 5.1: arcs

static bool genArc(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? rng.range(0, 2) : rng.range(3, 4);
  bool bad = false;
  ex.kind = K_ARC_CENTRAL;
  ex.level = level <= 1 ? 1 : 2;
  Cand *c = ex.slip;
  int nc = 0;
  int ans = 0;
  if (var <= 1) {
    int x = mult(rng, 10, 20, 170);
    ex.fig = Figure{FIG_CIRCLE_ARC, {x, var, 0, 0, 0, 0}};
    if (var == 0) {
      ans = x;
      bad |= PR(ex.text, TX(TX_ARC_Q_MINOR), x);
      SLIP(c, nc, exactInt(x / 2), TX(TX_ARC_SLIP_HALF));
      SLIP(c, nc, exactInt(2 * x), TX(TX_ARC_SLIP_DOUBLE));
      SLIP(c, nc, exactInt(360 - x), TX(TX_ARC_SLIP_MAJOR));
      SLIP(c, nc, exactInt(180 - x), TX(TX_ARC_SLIP_SUPP));
      bad |= PR(ex.clue[0], TX(TX_ARC_HINT1));
      bad |= PR(ex.clue[1], TX(TX_ARC_HINT2_MINOR));
      bad |= PR(ex.solution, TX(TX_ARC_SOL_MINOR), x);
    } else {
      ans = 360 - x;
      bad |= PR(ex.text, TX(TX_ARC_Q_MAJOR), x);
      SLIP(c, nc, exactInt(x), TX(TX_ARC_SLIP_MINOR));
      SLIP(c, nc, exactInt(180 - x), TX(TX_ARC_SLIP_SUPP));
      SLIP(c, nc, exactInt(360 - 2 * x), TX(TX_ARC_SLIP_TWICE));
      SLIP(c, nc, exactInt(x / 2), TX(TX_ARC_SLIP_HALF));
      bad |= PR(ex.clue[0], TX(TX_ARC_HINT1));
      bad |= PR(ex.clue[1], TX(TX_ARC_HINT2_MAJOR));
      bad |= PR(ex.solution, TX(TX_ARC_SOL_MAJOR), x, x, ans);
    }
  } else if (var == 2) {
    int a = mult(rng, 10, 40, 140), b = mult(rng, 10, 40, 140), d = mult(rng, 10, 40, 140);
    int S = a + b + d;
    ans = 360 - S;
    if (ans < 30 || ans > 170) return false;
    ex.fig = Figure{FIG_NONE, {a, 2, b, d, 0, 0}};
    bad |= PR(ex.text, TX(TX_ARC_Q_REST), a, b, d);
    SLIP(c, nc, exactInt(S), TX(TX_ARC_SLIP_SUM));
    SLIP(c, nc, exactInt(S - 180), TX(TX_ARC_SLIP_180));
    SLIP(c, nc, exactInt(360 - a - b), TX(TX_ARC_SLIP_TWO));
    SLIP(c, nc, exactInt(360 - b - d), TX(TX_ARC_SLIP_TWO));
    bad |= PR(ex.clue[0], TX(TX_ARC_HINT1_REST));
    bad |= PR(ex.clue[1], TX(TX_ARC_HINT2_REST));
    bad |= PR(ex.solution, TX(TX_ARC_SOL_REST), a, b, d, S, S, ans);
  } else if (var == 3) {
    static const int SETS[][3] = {{1, 2, 3}, {1, 2, 6}, {2, 3, 4}, {1, 3, 5}, {3, 4, 5}, {1, 4, 7}, {2, 5, 5}, {1, 1, 2},
                                  {2, 3, 5}, {1, 2, 7}, {1, 5, 9}, {2, 2, 5}, {1, 3, 8}, {3, 3, 6}};
    const int *s = SETS[rng.range(0, (int)(sizeof SETS / sizeof SETS[0]) - 1)];
    int r[3] = {s[0], s[1], s[2]};
    for (int i = 2; i > 0; i--) {
      int j = rng.range(0, i), t = r[i];
      r[i] = r[j];
      r[j] = t;
    }
    int S = r[0] + r[1] + r[2];
    if (360 % S) return false;
    int part = 360 / S, which = rng.range(0, 2);
    ans = part * r[which];
    const char *names[3] = {"AB", "BC", "CA"};
    ex.fig = Figure{FIG_NONE, {r[0], 3, r[1], r[2], which, 0}};
    bad |= PR(ex.text, TX(TX_ARC_Q_RATIO), r[0], r[1], r[2], names[which]);
    SLIP(c, nc, exactInt(120), TX(TX_ARC_SLIP_EQUAL));
    SLIP(c, nc, exactInt(part), TX(TX_ARC_SLIP_PART));
    SLIP(c, nc, exactInt(360 - ans), TX(TX_ARC_SLIP_REST));
    SLIP(c, nc, exactInt(part * r[(which + 1) % 3]), TX(TX_ARC_SLIP_OTHER));
    SLIP(c, nc, exactInt(r[which]), TX(TX_ARC_SLIP_NUMBER));
    bad |= PR(ex.clue[0], TX(TX_ARC_HINT1_RATIO));
    bad |= PR(ex.clue[1], TX(TX_ARC_HINT2_RATIO));
    bad |= PR(ex.solution, TX(TX_ARC_SOL_RATIO), r[0], r[1], r[2], S, S, part, names[which], r[which], part, ans);
  } else {
    int x = mult(rng, 10, 20, 160);
    if (x == 90) return false;
    ans = 180 - x;
    ex.fig = Figure{FIG_NONE, {x, 4, 0, 0, 0, 0}};
    bad |= PR(ex.text, TX(TX_ARC_Q_SEMI), x);
    SLIP(c, nc, exactInt(360 - x), TX(TX_ARC_SLIP_360));
    SLIP(c, nc, exactInt(x), TX(TX_ARC_SLIP_SAME));
    if (x < 90) SLIP(c, nc, exactInt(90 - x), TX(TX_ARC_SLIP_90));
    SLIP(c, nc, exactInt(x / 2), TX(TX_ARC_SLIP_HALFX));
    SLIP(c, nc, exactInt(x + 180), TX(TX_ARC_SLIP_PLUS));
    bad |= PR(ex.clue[0], TX(TX_ARC_HINT1_SEMI));
    bad |= PR(ex.clue[1], TX(TX_ARC_HINT2_SEMI));
    bad |= PR(ex.solution, TX(TX_ARC_SOL_SEMI), x, ans);
  }
  return finish(ex, rng, bad, exactInt(ans), c, nc, DEG);
}

// ---------------------------------------------------------------- 5.1: the chord and its distance

static bool genChord(int level, Rng &rng, Exercise &ex) {
  int var = rng.range(0, 2);  // 0 the chord, 1 the distance, 2 the radius
  int r, d, h;                // radius, distance, half the chord (the true values, when integers)
  int64_t n;
  if (level <= 1) {
    int a, b, cc;
    pickTriple(rng, a, b, cc);
    r = cc, d = a, h = b;
    n = var == 0 ? sq(r) - sq(d) : var == 1 ? sq(r) - sq(h) : sq(d) + sq(h);
  } else if (var == 0) {
    r = rng.range(4, 13), d = rng.range(1, r - 1), h = 0;
    n = sq(r) - sq(d);
  } else if (var == 1) {
    r = rng.range(4, 13), h = rng.range(1, r - 1), d = 0;
    n = sq(r) - sq(h);
  } else {
    d = rng.range(2, 9), h = rng.range(2, 9), r = 0;
    n = sq(d) + sq(h);
  }
  Exact s = exactSqrt(n);  // the unknown half-chord, distance or radius
  if (level <= 1 ? s.r != 1 : s.r == 1) return false;
  double rt = var == 2 ? sqrt((double)n) : r, dt = var == 1 ? sqrt((double)n) : d;
  if (level <= 1) rt = r, dt = d;
  int scale = level <= 1 ? 1 : 100;
  bool bad = false;
  ex.kind = K_CHORD_DISTANCE;
  ex.level = level <= 1 ? 1 : 2;
  int chordGiven = var == 0 ? 0 : 2 * h;
  ex.fig = Figure{FIG_CHORD, {(int)lround(rt * scale), (int)lround(dt * scale), var, var == 2 && level > 1 ? 0 : r, var == 1 && level > 1 ? 0 : d, chordGiven}};
  if (level <= 1) {
    ex.fig.v[3] = r, ex.fig.v[4] = d, ex.fig.v[5] = 2 * h;
    if (var == 0) ex.fig.v[5] = 0;
    if (var == 1) ex.fig.v[4] = 0;
    if (var == 2) ex.fig.v[3] = 0;
  }
  Cand *c = ex.slip;
  int nc = 0;
  char rtx[64], f[40];
  rootText(s, n, rtx, sizeof rtx);
  Exact right;
  if (var == 0) {
    right = twice(s);
    exactFormat(right, f, sizeof f);
    bad |= PR(ex.text, TX(TX_CHD_Q_CHORD), r, d);
    SLIP(c, nc, s, TX(TX_CHD_SLIP_HALF));
    SLIP(c, nc, exactInt(2 * (r - d)), TX(TX_CHD_SLIP_DIFF));
    SLIPR(c, nc, twice(exactSqrt(sq(r) + sq(d))), level, TX(TX_CHD_SLIP_ADD));
    SLIP(c, nc, exactInt((int32_t)n), TX(TX_CHD_SLIP_NOROOT));
    SLIP(c, nc, exactInt(2 * r), TX(TX_CHD_SLIP_DIAM));
    bad |= PR(ex.clue[0], TX(TX_CHD_HINT1));
    bad |= PR(ex.clue[1], TX(TX_CHD_HINT2_CHORD));
    bad |= PR(ex.solution, TX(TX_CHD_SOL_CHORD), r, d, (long long)sq(r), (long long)sq(d), (long long)n, rtx, f);
  } else if (var == 1) {
    right = s;
    bad |= PR(ex.text, TX(TX_CHD_Q_DIST), r, 2 * h);
    SLIP(c, nc, exactInt(h), TX(TX_CHD_SLIP_D_HALF));
    SLIP(c, nc, exactInt(r - h), TX(TX_CHD_SLIP_D_DIFF));
    SLIPR(c, nc, exactSqrt(sq(r) + sq(h)), level, TX(TX_CHD_SLIP_D_ADD));
    SLIP(c, nc, exactInt((int32_t)n), TX(TX_CHD_SLIP_D_NOROOT));
    if (sq(r) > 4 * sq(h)) SLIPR(c, nc, exactSqrt(sq(r) - 4 * sq(h)), level, TX(TX_CHD_SLIP_D_WHOLE));
    bad |= PR(ex.clue[0], TX(TX_CHD_HINT1));
    bad |= PR(ex.clue[1], TX(TX_CHD_HINT2_DIST));
    bad |= PR(ex.solution, TX(TX_CHD_SOL_DIST), 2 * h, h, r, h, (long long)sq(r), (long long)sq(h), (long long)n, rtx);
  } else {
    right = s;
    bad |= PR(ex.text, TX(TX_CHD_Q_RADIUS), 2 * h, d);
    SLIP(c, nc, exactInt(d + h), TX(TX_CHD_SLIP_R_SUM));
    SLIP(c, nc, exactInt((int32_t)n), TX(TX_CHD_SLIP_R_NOROOT));
    SLIPR(c, nc, exactSqrt(sq(d) + 4 * sq(h)), level, TX(TX_CHD_SLIP_R_WHOLE));
    SLIP(c, nc, exactInt(h), TX(TX_CHD_SLIP_R_HALF));
    if (d != h) SLIPR(c, nc, exactSqrt(sq(d) > sq(h) ? sq(d) - sq(h) : sq(h) - sq(d)), level, TX(TX_CHD_SLIP_R_DIFF));
    bad |= PR(ex.clue[0], TX(TX_CHD_HINT1));
    bad |= PR(ex.clue[1], TX(TX_CHD_HINT2_RADIUS));
    bad |= PR(ex.solution, TX(TX_CHD_SOL_RADIUS), 2 * h, h, d, h, (long long)sq(d), (long long)sq(h), (long long)n, rtx);
  }
  return finish(ex, rng, bad, right, c, nc, CM);
}

// ---------------------------------------------------------------- 5.2: the inscribed angle

static bool genInscribed(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? rng.range(0, 3) : rng.range(4, 7);
  if (level <= 1 && var == 2 && rng.range(0, 2)) var = rng.range(0, 1);  // the plain "90 degrees" is rarer
  bool bad = false;
  ex.kind = K_INSCRIBED;
  ex.level = level <= 1 ? 1 : 2;
  Cand *c = ex.slip;
  int nc = 0, ans = 0;
  const char *unit = DEG;
  switch (var) {
    case 0: {  // the arc -> the angle
      int x = mult(rng, 10, 40, 180);
      ans = x / 2;
      ex.fig = Figure{FIG_INSCRIBED, {x, 0, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_INS_Q_ANGLE), x);
      SLIP(c, nc, exactInt(x), TX(TX_INS_SLIP_FULL));
      SLIP(c, nc, exactInt(2 * x), TX(TX_INS_SLIP_DBL));
      SLIP(c, nc, exactInt(180 - x / 2), TX(TX_INS_SLIP_SUPP));
      if (x % 4 == 0) SLIP(c, nc, exactInt(x / 4), TX(TX_INS_SLIP_QUARTER));
      bad |= PR(ex.clue[0], TX(TX_INS_HINT1));
      bad |= PR(ex.clue[1], TX(TX_INS_HINT2_ANGLE));
      bad |= PR(ex.solution, TX(TX_INS_SOL_ANGLE), x, ans);
      break;
    }
    case 1: {  // the angle -> the arc
      int y = mult(rng, 5, 20, 85);
      ans = 2 * y;
      ex.fig = Figure{FIG_INSCRIBED, {2 * y, 1, y, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_INS_Q_ARC), y);
      SLIP(c, nc, exactInt(y), TX(TX_INS_SLIP_SAME));
      SLIP(c, nc, exactInt(360 - 2 * y), TX(TX_INS_SLIP_OTHERARC));
      if (y % 2 == 0) SLIP(c, nc, exactInt(y / 2), TX(TX_INS_SLIP_HALFANG));
      SLIP(c, nc, exactInt(180 - y), TX(TX_INS_SLIP_180Y));
      SLIP(c, nc, exactInt(4 * y), TX(TX_INS_SLIP_FOUR));
      bad |= PR(ex.clue[0], TX(TX_INS_HINT1));
      bad |= PR(ex.clue[1], TX(TX_INS_HINT2_ARC));
      bad |= PR(ex.solution, TX(TX_INS_SOL_ARC), y, ans);
      break;
    }
    case 2: {  // the angle on a diameter
      ans = 90;
      ex.fig = Figure{FIG_INSCRIBED, {180, 2, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_INS_Q_DIAM));
      SLIP(c, nc, exactInt(180), TX(TX_INS_SLIP_D180));
      SLIP(c, nc, exactInt(45), TX(TX_INS_SLIP_D45));
      SLIP(c, nc, exactInt(60), TX(TX_INS_SLIP_D60));
      SLIP(c, nc, exactInt(360), TX(TX_INS_SLIP_D360));
      bad |= PR(ex.clue[0], TX(TX_INS_HINT1_DIAM));
      bad |= PR(ex.clue[1], TX(TX_INS_HINT2_DIAM));
      bad |= PR(ex.solution, TX(TX_INS_SOL_DIAM));
      break;
    }
    case 3: {  // a diameter and one angle: the other
      int y = mult(rng, 5, 15, 75);
      if (y == 45) return false;
      ans = 90 - y;
      ex.fig = Figure{FIG_INSCRIBED, {180, 3, y, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_INS_Q_DIAMTRI), y);
      SLIP(c, nc, exactInt(180 - y), TX(TX_INS_SLIP_T180));
      SLIP(c, nc, exactInt(90 + y), TX(TX_INS_SLIP_TADD));
      SLIP(c, nc, exactInt(y), TX(TX_INS_SLIP_TSAME));
      SLIP(c, nc, exactInt(180 - 2 * y), TX(TX_INS_SLIP_TTWICE));
      bad |= PR(ex.clue[0], TX(TX_INS_HINT1_DIAM));
      bad |= PR(ex.clue[1], TX(TX_INS_HINT2_DIAMTRI));
      bad |= PR(ex.solution, TX(TX_INS_SOL_DIAMTRI), y, ans);
      break;
    }
    case 4: {  // the arcs round the circle -> the angle at A
      int a = mult(rng, 10, 40, 160), b = mult(rng, 10, 40, 160);
      int arc = 360 - a - b;
      if (arc < 40 || arc > 200) return false;
      ans = arc / 2;
      ex.fig = Figure{FIG_INSCRIBED, {arc, 4, a, b, 0, 0}};
      bad |= PR(ex.text, TX(TX_INS_Q_THREE), a, b);
      SLIP(c, nc, exactInt((a + b) / 2), TX(TX_INS_SLIP_GIVEN));
      SLIP(c, nc, exactInt(arc), TX(TX_INS_SLIP_FULL));
      SLIP(c, nc, exactInt(a / 2), TX(TX_INS_SLIP_ADJ));
      SLIP(c, nc, exactInt(b / 2), TX(TX_INS_SLIP_ADJ));
      bad |= PR(ex.clue[0], TX(TX_INS_HINT1));
      bad |= PR(ex.clue[1], TX(TX_INS_HINT2_THREE));
      bad |= PR(ex.solution, TX(TX_INS_SOL_THREE), a, b, arc, arc, ans);
      break;
    }
    case 5: {  // the arc BC and the angle B: the angle C
      int x = mult(rng, 10, 20, 200), y = mult(rng, 5, 20, 80);
      ans = 180 - x / 2 - y;
      if (ans < 20) return false;
      ex.fig = Figure{FIG_INSCRIBED, {x, 5, y, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_INS_Q_TRI), x, y);
      if (180 - x - y > 0) SLIP(c, nc, exactInt(180 - x - y), TX(TX_INS_SLIP_WHOLEARC));
      SLIP(c, nc, exactInt(180 - y), TX(TX_INS_SLIP_NOA));
      SLIP(c, nc, exactInt(x / 2 + y), TX(TX_INS_SLIP_NOREST));
      SLIP(c, nc, exactInt(180 - x / 2), TX(TX_INS_SLIP_NOB));
      bad |= PR(ex.clue[0], TX(TX_INS_HINT1));
      bad |= PR(ex.clue[1], TX(TX_INS_HINT2_TRI));
      bad |= PR(ex.solution, TX(TX_INS_SOL_TRI), x, x / 2, x / 2, y, ans);
      break;
    }
    case 6: {  // a vertex on the other side of the chord
      int x = mult(rng, 5, 25, 85);
      ans = 180 - x;
      ex.fig = Figure{FIG_INSCRIBED, {2 * x, 6, x, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_INS_Q_OTHER), x);
      SLIP(c, nc, exactInt(x), TX(TX_INS_SLIP_O_SAME));
      SLIP(c, nc, exactInt(360 - x), TX(TX_INS_SLIP_O_360));
      if (x < 90) SLIP(c, nc, exactInt(90 - x), TX(TX_INS_SLIP_O_90));
      SLIP(c, nc, exactInt(2 * x), TX(TX_INS_SLIP_O_ARC));
      bad |= PR(ex.clue[0], TX(TX_INS_HINT1));
      bad |= PR(ex.clue[1], TX(TX_INS_HINT2_OTHER));
      bad |= PR(ex.solution, TX(TX_INS_SOL_OTHER), x, ans);
      break;
    }
    default: {  // the vertex on the minor arc
      int x = mult(rng, 10, 40, 160);
      ans = 180 - x / 2;
      ex.fig = Figure{FIG_INSCRIBED, {360 - x, 7, x, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_INS_Q_MINOR), x);
      SLIP(c, nc, exactInt(x / 2), TX(TX_INS_SLIP_M_HALF));
      SLIP(c, nc, exactInt(x), TX(TX_INS_SLIP_FULL));
      SLIP(c, nc, exactInt(360 - x), TX(TX_INS_SLIP_M_WHOLE));
      SLIP(c, nc, exactInt(180 - x), TX(TX_INS_SLIP_M_180));
      bad |= PR(ex.clue[0], TX(TX_INS_HINT1));
      bad |= PR(ex.clue[1], TX(TX_INS_HINT2_MINOR));
      bad |= PR(ex.solution, TX(TX_INS_SOL_MINOR), x, 360 - x, 360 - x, ans);
      break;
    }
  }
  return finish(ex, rng, bad, exactInt(ans), c, nc, unit);
}

// ---------------------------------------------------------------- 5.2: the cyclic quadrilateral

static bool genCyclic(int level, Rng &rng, Exercise &ex) {
  int arcs[4];
  arcs[0] = mult(rng, 10, 30, 170), arcs[1] = mult(rng, 10, 30, 170), arcs[2] = mult(rng, 10, 30, 170);
  arcs[3] = 360 - arcs[0] - arcs[1] - arcs[2];
  if (arcs[3] < 30 || arcs[3] > 170) return false;
  int ang[4];
  for (int k = 0; k < 4; k++) ang[k] = (arcs[(k + 1) % 4] + arcs[(k + 2) % 4]) / 2;
  int var = level <= 1 ? rng.range(0, 1) : rng.range(2, 3);
  bool bad = false;
  ex.kind = K_CYCLIC_QUAD;
  ex.level = level <= 1 ? 1 : 2;
  Cand *c = ex.slip;
  int nc = 0, asked = 0, g = 0, ans = 0;
  if (var == 0) {
    g = rng.range(0, 3), asked = (g + 2) % 4;
    int x = ang[g];
    ans = ang[asked];
    bad |= PR(ex.text, TX(TX_CYC_Q_OPP), VN[g], x, VN[asked]);
    SLIP(c, nc, exactInt(x), TX(TX_CYC_SLIP_SAME));
    SLIP(c, nc, exactInt(360 - x), TX(TX_CYC_SLIP_360));
    if (x != 90) SLIP(c, nc, exactInt(x > 90 ? x - 90 : 90 - x), TX(TX_CYC_SLIP_90));
    SLIP(c, nc, exactInt(2 * x), TX(TX_CYC_SLIP_DBL));
    bad |= PR(ex.clue[0], TX(TX_CYC_HINT1));
    bad |= PR(ex.clue[1], TX(TX_CYC_HINT2_OPP));
    bad |= PR(ex.solution, TX(TX_CYC_SOL_OPP), VN[asked], VN[g], x, ans);
  } else if (var == 1) {
    g = rng.range(0, 3), asked = g;
    int a1 = arcs[(g + 1) % 4], a2 = arcs[(g + 2) % 4];
    ans = ang[g];
    bad |= PR(ex.text, TX(TX_CYC_Q_ARCS), ARCN[(g + 1) % 4], a1, ARCN[(g + 2) % 4], a2, VN[g], VN[g]);
    SLIP(c, nc, exactInt(a1 + a2), TX(TX_CYC_SLIP_ARC));
    SLIP(c, nc, exactInt(180 - ans), TX(TX_CYC_SLIP_OPPOSITE));
    SLIP(c, nc, exactInt(a1 / 2), TX(TX_CYC_SLIP_ONE));
    SLIP(c, nc, exactInt(a2 / 2), TX(TX_CYC_SLIP_ONE));
    bad |= PR(ex.clue[0], TX(TX_CYC_HINT1_ARCS));
    bad |= PR(ex.clue[1], TX(TX_CYC_HINT2_ARCS));
    bad |= PR(ex.solution, TX(TX_CYC_SOL_ARCS), VN[g], ARCN[(g + 1) % 4], ARCN[(g + 2) % 4], a1, a2, a1 + a2, a1 + a2, ans);
  } else if (var == 2) {
    g = rng.range(0, 3);
    int g2 = (g + 1) % 4;
    asked = rng.range(0, 1) ? (g + 2) % 4 : (g + 3) % 4;  // the opposite of the first or of the second
    int ga = ang[g], gb = ang[g2];
    ans = ang[asked];
    bool oppFirst = asked == (g + 2) % 4;
    int same = oppFirst ? ga : gb, other = oppFirst ? gb : ga;
    bad |= PR(ex.text, TX(TX_CYC_Q_TWO), VN[g], ga, VN[g2], gb, VN[asked]);
    SLIP(c, nc, exactInt(360 - ga - gb), TX(TX_CYC_SLIP_BOTH));
    SLIP(c, nc, exactInt(180 - other), TX(TX_CYC_SLIP_WRONGPAIR));
    SLIP(c, nc, exactInt(same), TX(TX_CYC_SLIP_SAME));
    if (ga + gb < 180) SLIP(c, nc, exactInt(180 - ga - gb), TX(TX_CYC_SLIP_BOTH180));
    SLIP(c, nc, exactInt(other), TX(TX_CYC_SLIP_OTHERGIVEN));
    bad |= PR(ex.clue[0], TX(TX_CYC_HINT1_TWO));
    bad |= PR(ex.clue[1], TX(TX_CYC_HINT2_TWO));
    bad |= PR(ex.solution, TX(TX_CYC_SOL_TWO), VN[asked], VN[oppFirst ? g : g2], VN[asked], same, ans);
  } else {
    asked = rng.range(0, 3);
    ans = ang[asked];
    bad |= PR(ex.text, TX(TX_CYC_Q_THREE), arcs[0], arcs[1], arcs[2], VN[asked]);
    SLIP(c, nc, exactInt(2 * ans), TX(TX_CYC_SLIP_ARC));
    SLIP(c, nc, exactInt(180 - ans), TX(TX_CYC_SLIP_OPPOSITE));
    SLIP(c, nc, exactInt((arcs[0] + arcs[1] + arcs[2]) / 2), TX(TX_CYC_SLIP_ALLTHREE));
    if (ans % 2 == 0) SLIP(c, nc, exactInt(ans / 2), TX(TX_CYC_SLIP_TWICEHALF));
    bad |= PR(ex.clue[0], TX(TX_CYC_HINT1_ARCS));
    bad |= PR(ex.clue[1], TX(TX_CYC_HINT2_THREE));
    int s1 = arcs[(asked + 1) % 4], s2 = arcs[(asked + 2) % 4];
    bad |= PR(ex.solution, TX(TX_CYC_SOL_THREE), arcs[0], arcs[1], arcs[2], arcs[0] + arcs[1] + arcs[2], arcs[3], VN[asked], s1, s2, s1 + s2, ans);
  }
  ex.fig = Figure{FIG_CYCLIC, {arcs[0], arcs[1], arcs[2], arcs[3], var, asked + 4 * g}};
  return finish(ex, rng, bad, exactInt(ans), c, nc, DEG);
}

// ---------------------------------------------------------------- 5.3: tangents

static bool genTangent(int level, Rng &rng, Exercise &ex) {
  int var = rng.range(0, 2);  // 0 AC, 1 OA, 2 the radius
  int r = 0, oa = 0, ac = 0;
  int64_t n;
  if (level <= 1) {
    int a, b, cc;
    pickTriple(rng, a, b, cc);
    r = a, ac = b, oa = cc;
    n = var == 0 ? sq(oa) - sq(r) : var == 1 ? sq(r) + sq(ac) : sq(oa) - sq(ac);
  } else if (var == 0) {
    r = rng.range(2, 9), oa = rng.range(r + 1, 15);
    n = sq(oa) - sq(r);
  } else if (var == 1) {
    r = rng.range(2, 9), ac = rng.range(2, 9);
    if (r == ac) return false;
    n = sq(r) + sq(ac);
  } else {
    oa = rng.range(4, 13), ac = rng.range(2, oa - 1);
    n = sq(oa) - sq(ac);
  }
  Exact s = exactSqrt(n);
  if (level <= 1 ? s.r != 1 : s.r == 1) return false;
  double un = sqrt((double)n);
  int scale = level <= 1 ? 1 : 100;
  double rt = var == 2 ? un : r, ot = var == 1 ? un : oa;
  int tr = var == 2 ? 0 : r, to = var == 1 ? 0 : oa, ta = var == 0 ? 0 : ac;
  bool bad = false;
  ex.kind = K_TANGENT_LENGTH;
  ex.level = level <= 1 ? 1 : 2;
  ex.fig = Figure{FIG_TANGENT, {(int)lround(rt * scale), (int)lround(ot * scale), var, tr, to, ta}};
  Cand *c = ex.slip;
  int nc = 0;
  char rtx[64];
  rootText(s, n, rtx, sizeof rtx);
  if (var == 0) {
    bad |= PR(ex.text, TX(TX_TAN_Q_AC), r, oa);
    SLIP(c, nc, exactInt(oa - r), TX(TX_TAN_SLIP_DIFF));
    SLIPR(c, nc, exactSqrt(sq(oa) + sq(r)), level, TX(TX_TAN_SLIP_ADD));
    SLIP(c, nc, exactInt((int32_t)n), TX(TX_TAN_SLIP_NOROOT));
    SLIP(c, nc, exactInt(oa), TX(TX_TAN_SLIP_HYP));
    bad |= PR(ex.clue[0], TX(TX_TAN_HINT1));
    bad |= PR(ex.clue[1], TX(TX_TAN_HINT2_AC));
    bad |= PR(ex.solution, TX(TX_TAN_SOL_AC), oa, r, (long long)sq(oa), (long long)sq(r), (long long)n, rtx);
  } else if (var == 1) {
    bad |= PR(ex.text, TX(TX_TAN_Q_OA), r, ac);
    SLIP(c, nc, exactInt(r + ac), TX(TX_TAN_SLIP_SUM));
    SLIP(c, nc, exactInt((int32_t)n), TX(TX_TAN_SLIP_NOROOT2));
    SLIPR(c, nc, exactSqrt(sq(r) > sq(ac) ? sq(r) - sq(ac) : sq(ac) - sq(r)), level, TX(TX_TAN_SLIP_WRONGSUB));
    SLIP(c, nc, exactInt(r > ac ? r : ac), TX(TX_TAN_SLIP_LONGERLEG));
    bad |= PR(ex.clue[0], TX(TX_TAN_HINT1));
    bad |= PR(ex.clue[1], TX(TX_TAN_HINT2_OA));
    bad |= PR(ex.solution, TX(TX_TAN_SOL_OA), r, ac, (long long)sq(r), (long long)sq(ac), (long long)n, rtx);
  } else {
    bad |= PR(ex.text, TX(TX_TAN_Q_R), oa, ac);
    SLIP(c, nc, exactInt(oa - ac), TX(TX_TAN_SLIP_DIFF2));
    SLIP(c, nc, exactInt((int32_t)n), TX(TX_TAN_SLIP_NOROOT3));
    SLIPR(c, nc, exactSqrt(sq(oa) + sq(ac)), level, TX(TX_TAN_SLIP_ADD2));
    SLIP(c, nc, exactInt(ac), TX(TX_TAN_SLIP_TANASR));
    bad |= PR(ex.clue[0], TX(TX_TAN_HINT1));
    bad |= PR(ex.clue[1], TX(TX_TAN_HINT2_R));
    bad |= PR(ex.solution, TX(TX_TAN_SOL_R), oa, ac, (long long)sq(oa), (long long)sq(ac), (long long)n, rtx);
  }
  return finish(ex, rng, bad, s, c, nc, CM);
}

// ---------------------------------------------------------------- 5.3: the circumscribed quadrilateral

static bool genCircumscribed(Rng &rng, Exercise &ex) {
  int s[4];
  s[0] = rng.range(4, 15), s[1] = rng.range(4, 15), s[2] = rng.range(4, 15);
  s[3] = s[0] + s[2] - s[1];
  if (s[3] < 4 || s[3] > 18) return false;
  if (s[0] == s[1] && s[1] == s[2]) return false;
  int m = rng.range(0, 3), var = rng.range(0, 1);
  int p1 = s[(m + 1) % 4], p2 = s[(m + 2) % 4], p3 = s[(m + 3) % 4];  // the three given sides, in order
  int sm = s[m], S = p1 + p3;
  const char *nm = ARCN[m], *n1 = ARCN[(m + 1) % 4], *n2 = ARCN[(m + 2) % 4], *n3 = ARCN[(m + 3) % 4];
  bool bad = false;
  ex.kind = K_CIRCUMSCRIBED;
  ex.level = 1;
  ex.fig = Figure{FIG_NONE, {s[0], s[1], s[2], s[3], var, m}};
  Cand *c = ex.slip;
  int nc = 0;
  if (var == 0) {
    bad |= PR(ex.text, TX(TX_CIR_Q_SIDE), n1, p1, n2, p2, n3, p3, nm);
    SLIP(c, nc, exactInt(p1 + p2 + p3), TX(TX_CIR_SLIP_SUM));
    if (p1 + p2 - p3 > 0) SLIP(c, nc, exactInt(p1 + p2 - p3), TX(TX_CIR_SLIP_NEIGHBOURS));
    if (p2 + p3 - p1 > 0) SLIP(c, nc, exactInt(p2 + p3 - p1), TX(TX_CIR_SLIP_NEIGHBOURS));
    SLIP(c, nc, exactInt(p2), TX(TX_CIR_SLIP_OPPEQUAL));
    bad |= PR(ex.clue[0], TX(TX_CIR_HINT1));
    bad |= PR(ex.clue[1], TX(TX_CIR_HINT2_SIDE));
    bad |= PR(ex.solution, TX(TX_CIR_SOL_SIDE), nm, n2, n1, n3, p1, p3, S, nm, S, p2, sm);
    return finish(ex, rng, bad, exactInt(sm), c, nc, CM);
  }
  int P = 2 * S;
  bad |= PR(ex.text, TX(TX_CIR_Q_PERIM), n1, p1, n2, p2, n3, p3);
  SLIP(c, nc, exactInt(p1 + p2 + p3), TX(TX_CIR_SLIP_PSUM));
  SLIP(c, nc, exactInt(S), TX(TX_CIR_SLIP_PHALF));
  SLIP(c, nc, exactInt(2 * (p1 + p2)), TX(TX_CIR_SLIP_POPP));
  SLIP(c, nc, exactInt(4 * p2), TX(TX_CIR_SLIP_PFOUR));
  bad |= PR(ex.clue[0], TX(TX_CIR_HINT1));
  bad |= PR(ex.clue[1], TX(TX_CIR_HINT2_PERIM));
  bad |= PR(ex.solution, TX(TX_CIR_SOL_PERIM), n1, n3, p1, p3, S, S, P);
  return finish(ex, rng, bad, exactInt(P), c, nc, CM);
}

// ---------------------------------------------------------------- 5.3: the area of a triangle with its inscribed circle

// Triangles with integer sides and an integer inradius (checked with Heron's formula before use).
static const int INC[][4] = {{3, 4, 5, 1},    {6, 8, 10, 2},   {5, 12, 13, 2},  {9, 12, 15, 3},  {8, 15, 17, 3}, {12, 16, 20, 4},
                             {15, 20, 25, 5}, {7, 24, 25, 3},  {10, 24, 26, 4}, {13, 14, 15, 4}, {9, 10, 17, 2}, {10, 10, 12, 3},
                             {15, 15, 24, 4}, {7, 15, 20, 2},  {6, 25, 29, 2},  {20, 21, 29, 6}, {18, 24, 30, 6}};

static bool genIncircle(Rng &rng, Exercise &ex) {
  const int *t = INC[rng.range(0, (int)(sizeof INC / sizeof INC[0]) - 1)];
  int sd[3] = {t[0], t[1], t[2]};
  for (int i = 2; i > 0; i--) {
    int j = rng.range(0, i), x = sd[i];
    sd[i] = sd[j];
    sd[j] = x;
  }
  int r = t[3], P = sd[0] + sd[1] + sd[2];
  if (P % 2) return false;
  int p = P / 2;
  int A = p * r;
  double hero = sqrt((double)p * (p - sd[0]) * (p - sd[1]) * (p - sd[2]));
  if (fabs(hero - A) > 1e-9) return false;
  int var = rng.range(0, 2);  // 0 sides and r, 1 the area and the perimeter, 2 the perimeter and r
  bool bad = false;
  ex.kind = K_INCIRCLE_AREA;
  ex.level = 1;
  ex.fig = Figure{FIG_NONE, {sd[0], sd[1], sd[2], var, r, 0}};
  Cand *c = ex.slip;
  int nc = 0;
  if (var == 1) {
    bad |= PR(ex.text, TX(TX_INC_Q_R), A, P);
    SLIP(c, nc, exactInt(A * p), TX(TX_INC_SLIP_R_MULT));
    SLIP(c, nc, exactInt(p), TX(TX_INC_SLIP_R_SEMI));
    SLIP(c, nc, exactInt(2 * r), TX(TX_INC_SLIP_R_DIAM));
    SLIP(c, nc, exactInt(A), TX(TX_INC_SLIP_R_AREA));
    if (A % P == 0) SLIP(c, nc, exactInt(A / P), TX(TX_INC_SLIP_R_PERIM));
    if (A > p) SLIP(c, nc, exactInt(A - p), TX(TX_INC_SLIP_R_SUB));
    bad |= PR(ex.clue[0], TX(TX_INC_HINT1));
    bad |= PR(ex.clue[1], TX(TX_INC_HINT2_R));
    bad |= PR(ex.solution, TX(TX_INC_SOL_R), P, p, A, p, r);
    return finish(ex, rng, bad, exactInt(r), c, nc, CM);
  }
  if (var == 0) bad |= PR(ex.text, TX(TX_INC_Q_AREA), r, sd[0], sd[1], sd[2]);
  else bad |= PR(ex.text, TX(TX_INC_Q_AREA_P), r, P);
  SLIP(c, nc, exactInt(P * r), TX(TX_INC_SLIP_A_PERIM));
  SLIP(c, nc, exactInt(p + r), TX(TX_INC_SLIP_A_ADD));
  SLIP(c, nc, exactInt(P + r), TX(TX_INC_SLIP_A_ADDP));
  if (A % 2 == 0) SLIP(c, nc, exactInt(A / 2), TX(TX_INC_SLIP_A_TWICE));
  bad |= PR(ex.clue[0], TX(TX_INC_HINT1));
  bad |= PR(ex.clue[1], TX(TX_INC_HINT2_AREA));
  if (var == 0) bad |= PR(ex.solution, TX(TX_INC_SOL_AREA), sd[0], sd[1], sd[2], P, P, p, p, r, A);
  else bad |= PR(ex.solution, TX(TX_INC_SOL_AREA_P), P, p, p, r, A);
  return finish(ex, rng, bad, exactInt(A), c, nc, CM2);
}

// ---------------------------------------------------------------- 5.4: regular polygons

static const int NS5[8] = {3, 4, 5, 6, 8, 9, 10, 12};

static bool genRegular(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? rng.range(0, 2) : level == 2 ? rng.range(3, 4) : rng.range(5, 8);
  bool bad = false;
  ex.kind = K_REGULAR_POLYGON;
  ex.level = level <= 1 ? 1 : level == 2 ? 2 : 3;
  Cand *c = ex.slip;
  int nc = 0, ans = 0;
  const char *unit = DEG;
  int n = NS5[rng.range(0, 7)];
  int cen = 360 / n, ang = (n - 2) * 180 / n, sum = (n - 2) * 180;
  switch (var) {
    case 0:
      ans = cen;
      ex.fig = Figure{FIG_REGULAR, {n, 0, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_RP_Q_CENTRAL), n);
      SLIP(c, nc, exactInt(ang), TX(TX_RP_SLIP_C_POLY));
      if (180 % n == 0) SLIP(c, nc, exactInt(180 / n), TX(TX_RP_SLIP_C_HALF));
      if (360 % (n - 2) == 0) SLIP(c, nc, exactInt(360 / (n - 2)), TX(TX_RP_SLIP_C_NM2));
      SLIP(c, nc, exactInt(n), TX(TX_RP_SLIP_C_N));
      bad |= PR(ex.clue[0], TX(TX_RP_HINT1_C));
      bad |= PR(ex.clue[1], TX(TX_RP_HINT2_C));
      bad |= PR(ex.solution, TX(TX_RP_SOL_C), n, ans);
      break;
    case 1:
      ans = ang;
      ex.fig = Figure{FIG_REGULAR, {n, 1, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_RP_Q_ANGLE), n);
      SLIP(c, nc, exactInt(cen), TX(TX_RP_SLIP_A_CENTRAL));
      SLIP(c, nc, exactInt(sum), TX(TX_RP_SLIP_A_SUM));
      if ((n - 1) * 180 % n == 0) SLIP(c, nc, exactInt((n - 1) * 180 / n), TX(TX_RP_SLIP_A_NM1));
      SLIP(c, nc, exactInt(2 * ang), TX(TX_RP_SLIP_A_360));
      bad |= PR(ex.clue[0], TX(TX_RP_HINT1_A));
      bad |= PR(ex.clue[1], TX(TX_RP_HINT2_A));
      bad |= PR(ex.solution, TX(TX_RP_SOL_A), n, sum, n, sum, n, ans);
      break;
    case 2:
      ans = sum;
      ex.fig = Figure{FIG_REGULAR, {n, 2, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_RP_Q_SUM), n);
      SLIP(c, nc, exactInt((n - 2) * 360), TX(TX_RP_SLIP_S_360));
      SLIP(c, nc, exactInt(n * 180), TX(TX_RP_SLIP_S_N));
      SLIP(c, nc, exactInt((n - 1) * 180), TX(TX_RP_SLIP_S_NM1));
      SLIP(c, nc, exactInt(ang), TX(TX_RP_SLIP_S_ONE));
      bad |= PR(ex.clue[0], TX(TX_RP_HINT1_S));
      bad |= PR(ex.clue[1], TX(TX_RP_HINT2_S));
      bad |= PR(ex.solution, TX(TX_RP_SOL_S), n, n - 2, sum);
      break;
    case 3:
      ans = n;
      unit = "";
      ex.fig = Figure{FIG_REGULAR, {n, 3, cen, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_RP_Q_N_CENTRAL), cen);
      if (n % 2 == 0) SLIP(c, nc, exactInt(n / 2), TX(TX_RP_SLIP_N_HALF));
      SLIP(c, nc, exactInt(n - 2), TX(TX_RP_SLIP_N_NM2));
      SLIP(c, nc, exactInt(n + 2), TX(TX_RP_SLIP_N_NP2));
      SLIP(c, nc, exactInt(n - 1), TX(TX_RP_SLIP_N_NM1));
      bad |= PR(ex.clue[0], TX(TX_RP_HINT1_NC));
      bad |= PR(ex.clue[1], TX(TX_RP_HINT2_NC));
      bad |= PR(ex.solution, TX(TX_RP_SOL_NC), cen, n);
      break;
    case 4:
      ans = n;
      unit = "";
      ex.fig = Figure{FIG_REGULAR, {n, 4, ang, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_RP_Q_N_ANGLE), ang);
      SLIP(c, nc, exactInt(180 - ang), TX(TX_RP_SLIP_N_CENTRAL));
      SLIP(c, nc, exactInt(n - 2), TX(TX_RP_SLIP_NA_NM2));
      SLIP(c, nc, exactInt(n + 2), TX(TX_RP_SLIP_NA_NP2));
      SLIP(c, nc, exactInt(n - 1), TX(TX_RP_SLIP_NA_NM1));
      bad |= PR(ex.clue[0], TX(TX_RP_HINT1_NA));
      bad |= PR(ex.clue[1], TX(TX_RP_HINT2_NA));
      bad |= PR(ex.solution, TX(TX_RP_SOL_NA), ang, ang, ang, 180 - ang, n);
      break;
    case 5: {  // the hexagon's perimeter from the radius
      int r = rng.range(2, 20);
      ans = 6 * r;
      unit = CM;
      ex.fig = Figure{FIG_REGULAR, {6, 5, r, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_RP_Q_HEXPER), r);
      SLIP(c, nc, exactInt(12 * r), TX(TX_RP_SLIP_H_DIAM));
      SLIP(c, nc, exactInt(5 * r), TX(TX_RP_SLIP_H_FIVE));
      SLIP(c, nc, exactInt(r), TX(TX_RP_SLIP_H_ONE));
      SLIP(c, nc, exactInt(3 * r), TX(TX_RP_SLIP_H_THREE));
      bad |= PR(ex.clue[0], TX(TX_RP_HINT1_HEX));
      bad |= PR(ex.clue[1], TX(TX_RP_HINT2_HEXPER));
      bad |= PR(ex.solution, TX(TX_RP_SOL_HEXPER), r, r, ans);
      break;
    }
    case 6: {  // the radius from the hexagon's perimeter
      int r = rng.range(2, 20), P = 6 * r;
      ans = r;
      unit = CM;
      ex.fig = Figure{FIG_REGULAR, {6, 6, P, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_RP_Q_HEXR), P);
      if (r % 2 == 0) SLIP(c, nc, exactInt(P / 12), TX(TX_RP_SLIP_HR_12));
      SLIP(c, nc, exactInt(P / 3), TX(TX_RP_SLIP_HR_3));
      SLIP(c, nc, exactInt(P / 2), TX(TX_RP_SLIP_HR_2));
      SLIP(c, nc, exactInt(P), TX(TX_RP_SLIP_HR_P));
      bad |= PR(ex.clue[0], TX(TX_RP_HINT1_HEX));
      bad |= PR(ex.clue[1], TX(TX_RP_HINT2_HEXR));
      bad |= PR(ex.solution, TX(TX_RP_SOL_HEXR), P, P, ans);
      break;
    }
    default: {  // the area from the perimeter and the apothem (the apothem is what a real polygon would have, rounded)
      static const int NP[6] = {4, 5, 6, 8, 10, 12};
      int k = NP[rng.range(0, 5)], s = rng.range(2, 20);
      double real = s / (2.0 * tan(3.14159265358979323846 / k));
      int a = (int)lround(real);
      if (a < 2 || fabs(real - a) > 0.04 * a) return false;
      int P = k * s;
      if ((P * a) % 2) return false;
      ans = P * a / 2;
      unit = CM2;
      if (var == 7) {
        ex.fig = Figure{FIG_REGULAR, {k, 7, P, a, 0, 0}};
        bad |= PR(ex.text, TX(TX_RP_Q_AREA_PA), P, a);
        SLIP(c, nc, exactInt(P * a), TX(TX_RP_SLIP_P_FULL));
        SLIP(c, nc, exactInt(P + a), TX(TX_RP_SLIP_P_ADD));
        SLIP(c, nc, exactInt(P / 2), TX(TX_RP_SLIP_P_HALFP));
        if (P * a % 4 == 0) SLIP(c, nc, exactInt(P * a / 4), TX(TX_RP_SLIP_P_QUARTER));
        bad |= PR(ex.clue[0], TX(TX_RP_HINT1_AREA));
        bad |= PR(ex.clue[1], TX(TX_RP_HINT2_AREA));
        bad |= PR(ex.solution, TX(TX_RP_SOL_AREA_PA), P, a, ans);
      } else {
        ex.fig = Figure{FIG_REGULAR, {k, 8, s, a, 0, 0}};
        bad |= PR(ex.text, TX(TX_RP_Q_AREA_NSA), k, s, a);
        SLIP(c, nc, exactInt(P * a), TX(TX_RP_SLIP_P_FULL));
        if (s * a % 2 == 0) SLIP(c, nc, exactInt(s * a / 2), TX(TX_RP_SLIP_P_ONE));
        SLIP(c, nc, exactInt(P), TX(TX_RP_SLIP_P_PERIM));
        SLIP(c, nc, exactInt(k + s + a), TX(TX_RP_SLIP_P_NUMS));
        bad |= PR(ex.clue[0], TX(TX_RP_HINT1_AREA));
        bad |= PR(ex.clue[1], TX(TX_RP_HINT2_AREA_NSA));
        bad |= PR(ex.solution, TX(TX_RP_SOL_AREA_NSA), k, s, P, P, a, ans);
      }
      break;
    }
  }
  return finish(ex, rng, bad, exactInt(ans), c, nc, unit);
}

// ---------------------------------------------------------------- 5.5: the circle's length

static bool genCircumference(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? rng.range(0, 2) : rng.range(3, 4);
  bool bad = false;
  ex.kind = K_CIRCUMFERENCE;
  ex.level = level <= 1 ? 1 : 2;
  Cand *c = ex.slip;
  int nc = 0, ans = 0;
  const char *unit = PCM;
  switch (var) {
    case 0: {
      int r = rng.range(2, 25);
      ans = 2 * r;
      ex.fig = Figure{FIG_NONE, {0, r, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_CF_Q_R), r);
      SLIP(c, nc, exactInt(r), TX(TX_CF_SLIP_NO2));
      SLIP(c, nc, exactInt(r * r), TX(TX_CF_SLIP_AREA));
      SLIP(c, nc, exactInt(4 * r), TX(TX_CF_SLIP_4R));
      SLIP(c, nc, exactInt(r + 2), TX(TX_CF_SLIP_ADD));
      bad |= PR(ex.clue[0], TX(TX_CF_HINT1));
      bad |= PR(ex.clue[1], TX(TX_CF_HINT2_R));
      bad |= PR(ex.solution, TX(TX_CF_SOL_R), r, ans);
      break;
    }
    case 1: {
      int d = mult(rng, 2, 4, 40);
      ans = d;
      ex.fig = Figure{FIG_NONE, {1, d, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_CF_Q_D), d);
      SLIP(c, nc, exactInt(2 * d), TX(TX_CF_SLIP_2D));
      SLIP(c, nc, exactInt(d / 2), TX(TX_CF_SLIP_RADIUS));
      SLIP(c, nc, exactInt(d / 2 * (d / 2)), TX(TX_CF_SLIP_AREA));
      SLIP(c, nc, exactInt(d * d), TX(TX_CF_SLIP_DSQ));
      bad |= PR(ex.clue[0], TX(TX_CF_HINT1));
      bad |= PR(ex.clue[1], TX(TX_CF_HINT2_D));
      bad |= PR(ex.solution, TX(TX_CF_SOL_D), d, d / 2, d / 2, ans);
      break;
    }
    case 2: {
      int r = rng.range(3, 25), L = 2 * r;
      ans = r;
      unit = CM;
      ex.fig = Figure{FIG_NONE, {2, L, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_CF_Q_FROMLEN), L);
      SLIP(c, nc, exactInt(L), TX(TX_CF_SLIP_L_AS_R));
      SLIP(c, nc, exactInt(2 * L), TX(TX_CF_SLIP_L_TWICE));
      SLIP(c, nc, exactInt(r * r), TX(TX_CF_SLIP_L_SQ));
      if (L % 4 == 0) SLIP(c, nc, exactInt(L / 4), TX(TX_CF_SLIP_L_QUARTER));
      bad |= PR(ex.clue[0], TX(TX_CF_HINT1_L));
      bad |= PR(ex.clue[1], TX(TX_CF_HINT2_L));
      bad |= PR(ex.solution, TX(TX_CF_SOL_L), L, L, ans);
      break;
    }
    case 3: {
      int r = rng.range(3, 25), t = rng.range(2, 12);
      ans = 2 * r * t;
      ex.fig = Figure{FIG_NONE, {3, r, t, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_CF_Q_WHEEL), r, t);
      SLIP(c, nc, exactInt(r * t), TX(TX_CF_SLIP_W_NO2));
      SLIP(c, nc, exactInt(2 * r), TX(TX_CF_SLIP_W_ONE));
      SLIP(c, nc, exactInt(t * r * r), TX(TX_CF_SLIP_W_AREA));
      SLIP(c, nc, exactInt(2 * r + t), TX(TX_CF_SLIP_W_ADD));
      bad |= PR(ex.clue[0], TX(TX_CF_HINT1_W));
      bad |= PR(ex.clue[1], TX(TX_CF_HINT2_W));
      bad |= PR(ex.solution, TX(TX_CF_SOL_W), r, 2 * r, t, t, 2 * r, ans);
      break;
    }
    default: {
      int R = rng.range(5, 24), r = rng.range(2, R - 2);
      ans = 2 * (R - r);
      ex.fig = Figure{FIG_NONE, {4, R, r, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_CF_Q_TWO), R, r);
      SLIP(c, nc, exactInt(R - r), TX(TX_CF_SLIP_T_NO2));
      SLIP(c, nc, exactInt(2 * (R + r)), TX(TX_CF_SLIP_T_SUM));
      SLIP(c, nc, exactInt(R * R - r * r), TX(TX_CF_SLIP_T_AREA));
      SLIP(c, nc, exactInt(2 * R - r), TX(TX_CF_SLIP_T_MIX));
      bad |= PR(ex.clue[0], TX(TX_CF_HINT1_T));
      bad |= PR(ex.clue[1], TX(TX_CF_HINT2_T));
      bad |= PR(ex.solution, TX(TX_CF_SOL_T), R, 2 * R, r, 2 * r, 2 * R, 2 * r, ans);
      break;
    }
  }
  return finish(ex, rng, bad, exactInt(ans), c, nc, unit);
}

// ---------------------------------------------------------------- 5.5: the disc's area

static bool genDisc(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? rng.range(0, 2) : rng.range(3, 4);
  bool bad = false;
  ex.kind = K_DISC_AREA;
  ex.level = level <= 1 ? 1 : 2;
  Cand *c = ex.slip;
  int nc = 0, ans = 0;
  const char *unit = PCM2;
  switch (var) {
    case 0: {
      int r = rng.range(2, 20);
      ans = r * r;
      ex.fig = Figure{FIG_NONE, {0, r, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_DA_Q_R), r);
      SLIP(c, nc, exactInt(2 * r), TX(TX_DA_SLIP_LEN));
      SLIP(c, nc, exactInt(r), TX(TX_DA_SLIP_NOSQ));
      SLIP(c, nc, exactInt(4 * r * r), TX(TX_DA_SLIP_DSQ));
      SLIP(c, nc, exactInt(2 * r * r), TX(TX_DA_SLIP_2R2));
      bad |= PR(ex.clue[0], TX(TX_DA_HINT1));
      bad |= PR(ex.clue[1], TX(TX_DA_HINT2_R));
      bad |= PR(ex.solution, TX(TX_DA_SOL_R), r, ans);
      break;
    }
    case 1: {
      int d = mult(rng, 2, 4, 40), r = d / 2;
      ans = r * r;
      ex.fig = Figure{FIG_NONE, {1, d, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_DA_Q_D), d);
      SLIP(c, nc, exactInt(d * d), TX(TX_DA_SLIP_D_ASR));
      SLIP(c, nc, exactInt(d), TX(TX_DA_SLIP_D_NOSQ));
      SLIP(c, nc, exactInt(r), TX(TX_DA_SLIP_NOSQ));
      SLIP(c, nc, exactInt(2 * d), TX(TX_DA_SLIP_D_LEN));
      bad |= PR(ex.clue[0], TX(TX_DA_HINT1));
      bad |= PR(ex.clue[1], TX(TX_DA_HINT2_D));
      bad |= PR(ex.solution, TX(TX_DA_SOL_D), d, r, r, ans);
      break;
    }
    case 2: {
      int r = rng.range(2, 20), A = r * r;
      ans = r;
      unit = CM;
      ex.fig = Figure{FIG_NONE, {2, A, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_DA_Q_FROMAREA), A);
      SLIP(c, nc, exactInt(A), TX(TX_DA_SLIP_A_AS_R));
      SLIP(c, nc, exactInt(2 * A), TX(TX_DA_SLIP_A_TWICE));
      if (A % 2 == 0) SLIP(c, nc, exactInt(A / 2), TX(TX_DA_SLIP_A_HALF));
      if (A % 4 == 0) SLIP(c, nc, exactInt(A / 4), TX(TX_DA_SLIP_A_QUARTER));
      SLIP(c, nc, exactInt(2 * r), TX(TX_DA_SLIP_A_DIAM));
      bad |= PR(ex.clue[0], TX(TX_DA_HINT1_A));
      bad |= PR(ex.clue[1], TX(TX_DA_HINT2_A));
      bad |= PR(ex.solution, TX(TX_DA_SOL_A), A, A, ans, ans, A);
      break;
    }
    case 3: {
      int r = rng.range(2, 20), L = 2 * r;
      ans = r * r;
      ex.fig = Figure{FIG_NONE, {3, L, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_DA_Q_FROMLEN), L);
      SLIP(c, nc, exactInt(r), TX(TX_DA_SLIP_NOSQ));
      SLIP(c, nc, exactInt(L * L), TX(TX_DA_SLIP_L_SQ));
      SLIP(c, nc, exactInt(2 * r * r), TX(TX_DA_SLIP_2R2));
      SLIP(c, nc, exactInt(L), TX(TX_DA_SLIP_L_AS_AREA));
      bad |= PR(ex.clue[0], TX(TX_DA_HINT1_L));
      bad |= PR(ex.clue[1], TX(TX_DA_HINT2_L));
      bad |= PR(ex.solution, TX(TX_DA_SOL_L), L, L, r, r, ans);
      break;
    }
    default: {
      int s = mult(rng, 2, 4, 30), r = s / 2;
      ans = r * r;
      ex.fig = Figure{FIG_NONE, {4, s, 0, 0, 0, 0}};
      bad |= PR(ex.text, TX(TX_DA_Q_SQUARE), s);
      SLIP(c, nc, exactInt(s * s), TX(TX_DA_SLIP_S_ASR));
      SLIP(c, nc, exactInt(r), TX(TX_DA_SLIP_NOSQ));
      SLIP(c, nc, exactInt(s), TX(TX_DA_SLIP_S_SIDE));
      if (s * s % 2 == 0) SLIP(c, nc, exactInt(s * s / 2), TX(TX_DA_SLIP_S_HALF));
      bad |= PR(ex.clue[0], TX(TX_DA_HINT1_S));
      bad |= PR(ex.clue[1], TX(TX_DA_HINT2_S));
      bad |= PR(ex.solution, TX(TX_DA_SOL_S), s, r, r, ans);
      break;
    }
  }
  return finish(ex, rng, bad, exactInt(ans), c, nc, unit);
}

// ---------------------------------------------------------------- 5.5: arcs and sectors

static bool genArcSector(int level, Rng &rng, Exercise &ex) {
  static const int NS[] = {20, 30, 36, 40, 45, 60, 72, 90, 120, 150, 180, 240, 270};
  int var = level <= 1 ? rng.range(0, 1) : rng.range(2, 5);
  int n = NS[rng.range(0, (int)(sizeof NS / sizeof NS[0]) - 1)];
  bool needArea = var == 1 || var == 3 || var == 5;
  int r = 0, start = rng.range(3, 24);
  for (int i = 0; i < 22 && !r; i++) {
    int t = 3 + (start - 3 + i) % 22;
    if (needArea ? (n * t * t) % 360 == 0 : (n * t) % 180 == 0) r = t;
  }
  if (!r) return false;
  int arcC = n * r / 180, F = 2 * r, D = r * r;
  int secS = needArea ? n * r * r / 360 : 0;
  bool bad = false;
  ex.kind = K_ARC_SECTOR;
  ex.level = level <= 1 ? 1 : 2;
  Cand *c = ex.slip;
  int nc = 0, ans = 0;
  const char *unit = PCM;
  if (arcC == 1 && !needArea) return false;
  if (needArea && secS == 1) return false;
  int given = var == 0 || var == 1 ? 0 : (var == 2 || var == 4) ? arcC : secS;
  ex.fig = Figure{FIG_SECTOR, {n, var, r, given, 0, 0}};
  switch (var) {
    case 0:
      ans = arcC;
      bad |= PR(ex.text, TX(TX_AS_Q_ARC), r, n);
      if (n * r * r % 360 == 0) SLIP1(c, nc, exactInt(n * r * r / 360), TX(TX_AS_SLIP_AREAFORM));
      if (arcC % 2 == 0) SLIP1(c, nc, exactInt(arcC / 2), TX(TX_AS_SLIP_NO2));
      SLIP1(c, nc, exactInt(2 * arcC), TX(TX_AS_SLIP_DBL));
      SLIP1(c, nc, exactInt(F), TX(TX_AS_SLIP_WHOLE));
      bad |= PR(ex.clue[0], TX(TX_AS_HINT1_ARC));
      bad |= PR(ex.clue[1], TX(TX_AS_HINT2_ARC));
      bad |= PR(ex.solution, TX(TX_AS_SOL_ARC), n, n, r, ans);
      break;
    case 1:
      ans = secS;
      unit = PCM2;
      bad |= PR(ex.text, TX(TX_AS_Q_SECTOR), r, n);
      SLIP1(c, nc, exactInt(D), TX(TX_AS_SLIP_S_DISC));
      if ((n * r) % 180 == 0) SLIP1(c, nc, exactInt(arcC), TX(TX_AS_SLIP_S_LENGTH));
      if ((n * r) % 360 == 0) SLIP1(c, nc, exactInt(n * r / 360), TX(TX_AS_SLIP_S_NOSQ));
      SLIP1(c, nc, exactInt(2 * secS), TX(TX_AS_SLIP_S_DBL));
      if (secS % 2 == 0) SLIP1(c, nc, exactInt(secS / 2), TX(TX_AS_SLIP_S_HALF));
      bad |= PR(ex.clue[0], TX(TX_AS_HINT1_SECTOR));
      bad |= PR(ex.clue[1], TX(TX_AS_HINT2_SECTOR));
      bad |= PR(ex.solution, TX(TX_AS_SOL_SECTOR), n, n, r, ans);
      break;
    case 2:
      ans = F;
      bad |= PR(ex.text, TX(TX_AS_Q_FULL), n, arcC);
      SLIP1(c, nc, exactInt(arcC), TX(TX_AS_SLIP_F_ARC));
      SLIP1(c, nc, exactInt(F - arcC), TX(TX_AS_SLIP_F_REST));
      SLIP1(c, nc, exactInt(r), TX(TX_AS_SLIP_F_HALF));
      if (arcC * n % 360 == 0) SLIP1(c, nc, exactInt(arcC * n / 360), TX(TX_AS_SLIP_MULT));
      bad |= PR(ex.clue[0], TX(TX_AS_HINT1_FULL));
      bad |= PR(ex.clue[1], TX(TX_AS_HINT2_FULL));
      bad |= PR(ex.solution, TX(TX_AS_SOL_FULL), n, n, arcC, n, ans);
      break;
    case 3:
      ans = D;
      unit = PCM2;
      bad |= PR(ex.text, TX(TX_AS_Q_DISC), n, secS);
      SLIP1(c, nc, exactInt(secS), TX(TX_AS_SLIP_D_SECTOR));
      SLIP1(c, nc, exactInt(D - secS), TX(TX_AS_SLIP_D_REST));
      if (r != secS) SLIP1(c, nc, exactInt(r), TX(TX_AS_SLIP_D_RADIUS));
      if (D % 2 == 0) SLIP1(c, nc, exactInt(D / 2), TX(TX_AS_SLIP_D_HALF));
      if (secS * n % 360 == 0) SLIP1(c, nc, exactInt(secS * n / 360), TX(TX_AS_SLIP_MULT));
      bad |= PR(ex.clue[0], TX(TX_AS_HINT1_FULL));
      bad |= PR(ex.clue[1], TX(TX_AS_HINT2_DISC));
      bad |= PR(ex.solution, TX(TX_AS_SOL_DISC), n, n, secS, n, ans);
      break;
    case 4:
      ans = r;
      unit = CM;
      bad |= PR(ex.text, TX(TX_AS_Q_RFROMARC), n, arcC);
      SLIP1(c, nc, exactInt(arcC), TX(TX_AS_SLIP_R_AS_R));
      SLIP1(c, nc, exactInt(F), TX(TX_AS_SLIP_R_WHOLE));
      SLIP1(c, nc, exactInt(r * r), TX(TX_AS_SLIP_R_SQ));
      if (arcC * n % 180 == 0) SLIP1(c, nc, exactInt(arcC * n / 180), TX(TX_AS_SLIP_MULT));
      bad |= PR(ex.clue[0], TX(TX_AS_HINT1_FULL));
      bad |= PR(ex.clue[1], TX(TX_AS_HINT2_RARC));
      bad |= PR(ex.solution, TX(TX_AS_SOL_RARC), n, n, arcC, n, F, F, F, ans);
      break;
    default:
      ans = r;
      unit = CM;
      bad |= PR(ex.text, TX(TX_AS_Q_RFROMSECTOR), n, secS);
      SLIP1(c, nc, exactInt(D), TX(TX_AS_SLIP_RS_DISC));
      SLIP1(c, nc, exactInt(secS), TX(TX_AS_SLIP_RS_SECTOR));
      SLIP1(c, nc, exactInt(2 * r), TX(TX_AS_SLIP_RS_DIAM));
      if (secS * n % 360 == 0) SLIP1(c, nc, exactInt(secS * n / 360), TX(TX_AS_SLIP_MULT));
      bad |= PR(ex.clue[0], TX(TX_AS_HINT1_FULL));
      bad |= PR(ex.clue[1], TX(TX_AS_HINT2_RSEC));
      bad |= PR(ex.solution, TX(TX_AS_SOL_RSEC), n, n, secS, n, D, D, D, r, r, D);
      break;
  }
  return finish(ex, rng, bad, exactInt(ans), c, nc, unit);
}

// ---------------------------------------------------------------- 5.5: the ring

static bool genAnnulus(Rng &rng, Exercise &ex) {
  int var = rng.range(0, 2);
  int R = rng.range(5, 15), r = rng.range(2, R - 2);
  if (R - r < 2) return false;
  int w = R - r, ans = R * R - r * r;
  bool bad = false;
  ex.kind = K_ANNULUS;
  ex.level = 1;
  ex.fig = Figure{FIG_ANNULUS, {R, r, var, 0, 0, 0}};
  Cand *c = ex.slip;
  int nc = 0;
  if (var == 0) {
    bad |= PR(ex.text, TX(TX_AN_Q_RADII), R, r);
    SLIP(c, nc, exactInt(w * w), TX(TX_AN_SLIP_DIFFSQ));
    SLIP(c, nc, exactInt(R * R + r * r), TX(TX_AN_SLIP_SUM));
    SLIP(c, nc, exactInt(R * R - r), TX(TX_AN_SLIP_ONESQ));
    SLIP(c, nc, exactInt(w), TX(TX_AN_SLIP_NOSQ));
    SLIP(c, nc, exactInt(R * R), TX(TX_AN_SLIP_BIGDISC));
  } else if (var == 1) {
    bad |= PR(ex.text, TX(TX_AN_Q_DIAMS), 2 * R, 2 * r);
    SLIP(c, nc, exactInt(4 * ans), TX(TX_AN_SLIP_D_AS_R));
    SLIP(c, nc, exactInt(w * w), TX(TX_AN_SLIP_DIFFSQ));
    SLIP(c, nc, exactInt(R * R + r * r), TX(TX_AN_SLIP_SUM));
    SLIP(c, nc, exactInt(w), TX(TX_AN_SLIP_NOSQ));
    SLIP(c, nc, exactInt(R * R - r), TX(TX_AN_SLIP_ONESQ));
  } else {
    bad |= PR(ex.text, TX(TX_AN_Q_WIDTH), R, w);
    SLIP(c, nc, exactInt(w * w), TX(TX_AN_SLIP_W_ONLY));
    SLIP(c, nc, exactInt(R * R - w * w), TX(TX_AN_SLIP_W_AS_r));
    SLIP(c, nc, exactInt(R * R + r * r), TX(TX_AN_SLIP_SUM));
    SLIP(c, nc, exactInt(R * R), TX(TX_AN_SLIP_BIGDISC));
    SLIP(c, nc, exactInt(R * w), TX(TX_AN_SLIP_W_PROD));
  }
  bad |= PR(ex.clue[0], TX(TX_AN_HINT1));
  bad |= PR(ex.clue[1], var == 2 ? TX(TX_AN_HINT2_W) : var == 1 ? TX(TX_AN_HINT2_D) : TX(TX_AN_HINT2));
  if (var == 0) bad |= PR(ex.solution, TX(TX_AN_SOL_RADII), R, R * R, r, r * r, R * R, r * r, ans);
  else if (var == 1) bad |= PR(ex.solution, TX(TX_AN_SOL_DIAMS), 2 * R, R, 2 * r, r, R, r, R * R, r * r, ans);
  else bad |= PR(ex.solution, TX(TX_AN_SOL_WIDTH), R, w, r, R, r, R * R, r * r, ans);
  return finish(ex, rng, bad, exactInt(ans), c, nc, PCM2);
}

// ---------------------------------------------------------------- dispatch

bool u5Generate(Kind kind, int level, Rng &rng, Exercise &ex) {
  ex.fig = Figure{FIG_NONE, {0, 0, 0, 0, 0, 0}};
  switch (kind) {
    case K_ARC_CENTRAL: return genArc(level, rng, ex);
    case K_CHORD_DISTANCE: return genChord(level, rng, ex);
    case K_INSCRIBED: return genInscribed(level, rng, ex);
    case K_CYCLIC_QUAD: return genCyclic(level, rng, ex);
    case K_TANGENT_LENGTH: return genTangent(level, rng, ex);
    case K_CIRCUMSCRIBED: return genCircumscribed(rng, ex);
    case K_INCIRCLE_AREA: return genIncircle(rng, ex);
    case K_REGULAR_POLYGON: return genRegular(level, rng, ex);
    case K_CIRCUMFERENCE: return genCircumference(level, rng, ex);
    case K_DISC_AREA: return genDisc(level, rng, ex);
    case K_ARC_SECTOR: return genArcSector(level, rng, ex);
    case K_ANNULUS: return genAnnulus(rng, ex);
    default: return false;
  }
}

// ---------------------------------------------------------------- the second derivation

static bool bad(char *err, int cap, const char *msg) {
  if (err && cap > 0) snprintf(err, cap, "%s", msg);
  return false;
}

static bool whole(double x) { return fabs(x - floor(x + 0.5)) < 1e-9; }

bool u5Expect(const Exercise &ex, double &expect, const char *&unit, char *err, int cap) {
  const int32_t *v = ex.fig.v;
  unit = DEG;
  switch (ex.kind) {
    case K_ARC_CENTRAL: {
      int var = v[1];
      if (var == 0 || var == 1) {
        if (ex.fig.kind != FIG_CIRCLE_ARC || v[0] < 10 || v[0] > 170) return bad(err, cap, "central angle givens");
        expect = var == 0 ? v[0] : 360.0 - v[0];
      } else if (var == 2) {
        if (v[0] <= 0 || v[2] <= 0 || v[3] <= 0 || v[0] + v[2] + v[3] >= 360) return bad(err, cap, "arcs givens");
        expect = 360.0 - v[0] - v[2] - v[3];
      } else if (var == 3) {
        int S = v[0] + v[2] + v[3];
        if (v[0] <= 0 || v[2] <= 0 || v[3] <= 0 || v[4] < 0 || v[4] > 2) return bad(err, cap, "ratio givens");
        int pick = v[4] == 0 ? v[0] : v[4] == 1 ? v[2] : v[3];
        if (!whole(360.0 * pick / S)) return bad(err, cap, "arc is not whole degrees");
        expect = 360.0 * pick / S;
      } else if (var == 4) {
        if (v[0] <= 0 || v[0] >= 180) return bad(err, cap, "semicircle givens");
        expect = 180.0 - v[0];
      } else {
        return bad(err, cap, "arc variant");
      }
      return true;
    }
    case K_CHORD_DISTANCE: {
      unit = " cm";
      if (ex.fig.kind != FIG_CHORD || v[2] < 0 || v[2] > 2) return bad(err, cap, "chord givens");
      double r = v[3], d = v[4], h = v[5] / 2.0, dTrue, rTrue;
      if (v[2] == 0) {
        if (r <= 0 || d <= 0 || d >= r) return bad(err, cap, "need 0 < d < r");
        expect = 2.0 * sqrt(r * r - d * d), dTrue = d, rTrue = r;
      } else if (v[2] == 1) {
        if (r <= 0 || h <= 0 || h >= r) return bad(err, cap, "need 0 < half chord < r");
        expect = sqrt(r * r - h * h), dTrue = expect, rTrue = r;
      } else {
        if (d <= 0 || h <= 0) return bad(err, cap, "distance and chord must be positive");
        expect = hypot(d, h), dTrue = d, rTrue = expect;
      }
      if (fabs((double)v[1] / v[0] - dTrue / rTrue) > 0.011) return bad(err, cap, "the figure does not show the givens");
      return true;
    }
    case K_INSCRIBED: {
      int var = v[1];
      if (ex.fig.kind != FIG_INSCRIBED || v[0] < 20 || v[0] > 340) return bad(err, cap, "inscribed figure");
      if (var == 0) {
        if (v[0] % 2) return bad(err, cap, "odd arc");
        expect = v[0] / 2.0;
      } else if (var == 1) {
        if (v[0] != 2 * v[2]) return bad(err, cap, "arc is not twice the angle");
        expect = v[0];
      } else if (var == 2) {
        expect = v[0] / 2.0;  // the arc is a semicircle
        if (v[0] != 180) return bad(err, cap, "a diameter's arc is 180");
      } else if (var == 3) {
        if (v[0] != 180 || v[2] <= 0 || v[2] >= 90) return bad(err, cap, "diameter givens");
        expect = 180.0 - v[0] / 2.0 - v[2];
      } else if (var == 4) {
        if (v[0] != 360 - v[2] - v[3] || v[2] <= 0 || v[3] <= 0) return bad(err, cap, "arcs do not fit the circle");
        expect = (360.0 - v[2] - v[3]) / 2.0;
      } else if (var == 5) {
        if (v[2] <= 0) return bad(err, cap, "angle givens");
        expect = 180.0 - v[0] / 2.0 - v[2];
        if (expect <= 0) return bad(err, cap, "triangle angles too large");
      } else if (var == 6) {
        if (v[0] != 2 * v[2] || v[2] >= 180) return bad(err, cap, "other-side givens");
        expect = (360.0 - v[0]) / 2.0;
      } else if (var == 7) {
        if (v[0] != 360 - v[2] || v[2] >= 180) return bad(err, cap, "minor arc givens");
        expect = v[0] / 2.0;
      } else {
        return bad(err, cap, "inscribed variant");
      }
      return true;
    }
    case K_CYCLIC_QUAD: {
      if (ex.fig.kind != FIG_CYCLIC) return bad(err, cap, "cyclic figure");
      if (v[0] <= 0 || v[1] <= 0 || v[2] <= 0 || v[3] <= 0 || v[0] + v[1] + v[2] + v[3] != 360) return bad(err, cap, "arcs must add up to 360");
      int asked = v[5] & 3, g = v[5] >> 2, var = v[4];
      if (var == 0 && asked != (g + 2) % 4) return bad(err, cap, "opposite angle expected");
      if (var == 2 && asked != (g + 2) % 4 && asked != (g + 3) % 4) return bad(err, cap, "third or fourth angle expected");
      if (var < 0 || var > 3) return bad(err, cap, "cyclic variant");
      // the angle at a vertex is half of the arc that does not contain it: the next two arcs round the circle
      double arcSeen = v[(asked + 1) % 4] + v[(asked + 2) % 4];
      expect = arcSeen / 2.0;
      return true;
    }
    case K_TANGENT_LENGTH: {
      unit = " cm";
      if (ex.fig.kind != FIG_TANGENT || v[2] < 0 || v[2] > 2) return bad(err, cap, "tangent givens");
      double r = v[3], oa = v[4], ac = v[5], rTrue, oTrue;
      if (v[2] == 0) {
        if (r <= 0 || oa <= r) return bad(err, cap, "need OA > r");
        expect = sqrt(oa * oa - r * r), rTrue = r, oTrue = oa;
      } else if (v[2] == 1) {
        if (r <= 0 || ac <= 0) return bad(err, cap, "r and AC must be positive");
        expect = hypot(r, ac), rTrue = r, oTrue = expect;
      } else {
        if (ac <= 0 || oa <= ac) return bad(err, cap, "need OA > AC");
        expect = sqrt(oa * oa - ac * ac), rTrue = expect, oTrue = oa;
      }
      if (fabs((double)v[0] / v[1] - rTrue / oTrue) > 0.011) return bad(err, cap, "the figure does not show the givens");
      return true;
    }
    case K_CIRCUMSCRIBED: {
      unit = " cm";
      int m = v[5];
      if (m < 0 || m > 3 || v[4] < 0 || v[4] > 1) return bad(err, cap, "circumscribed givens");
      for (int i = 0; i < 4; i++)
        if (v[i] <= 0) return bad(err, cap, "side not positive");
      if (v[0] + v[2] != v[1] + v[3]) return bad(err, cap, "sums of opposite sides differ");
      if (v[4] == 0) {
        expect = (double)v[(m + 1) % 4] + v[(m + 3) % 4] - v[(m + 2) % 4];  // from the three given sides only
        if (expect != v[m]) return bad(err, cap, "the unknown side does not match");
      } else {
        expect = 2.0 * (v[(m + 1) % 4] + v[(m + 3) % 4]);  // twice the sum of a known opposite pair
      }
      return true;
    }
    case K_INCIRCLE_AREA: {
      double a = v[0], b = v[1], c = v[2];
      if (a <= 0 || b <= 0 || c <= 0 || a + b <= c || a + c <= b || b + c <= a) return bad(err, cap, "not a triangle");
      double p = (a + b + c) / 2.0, area = sqrt(p * (p - a) * (p - b) * (p - c));  // Heron
      if (fabs(area / p - v[4]) > 1e-9) return bad(err, cap, "the radius does not fit the triangle");
      if (v[3] == 1) unit = " cm", expect = area / p;
      else if (v[3] == 0 || v[3] == 2) unit = " cm" SQ, expect = area;
      else return bad(err, cap, "incircle variant");
      return true;
    }
    case K_REGULAR_POLYGON: {
      int n = v[0], var = v[1];
      if (ex.fig.kind != FIG_REGULAR || n < 3 || n > 12) return bad(err, cap, "regular figure");
      if (var == 0) expect = 360.0 / n;
      else if (var == 1) expect = (n - 2) * 180.0 / n;
      else if (var == 2) expect = (n - 2) * 180.0;
      else if (var == 3) {
        unit = "";
        if (v[2] <= 0) return bad(err, cap, "central angle");
        expect = 360.0 / v[2];
        if (expect != n) return bad(err, cap, "central angle does not match n");
      } else if (var == 4) {
        unit = "";
        if (v[2] <= 0 || v[2] >= 180) return bad(err, cap, "polygon angle");
        expect = 360.0 / (180.0 - v[2]);
        if (expect != n) return bad(err, cap, "polygon angle does not match n");
      } else if (var == 5) {
        unit = " cm";
        if (n != 6 || v[2] <= 0) return bad(err, cap, "hexagon givens");
        expect = 6.0 * v[2];
      } else if (var == 6) {
        unit = " cm";
        if (n != 6 || v[2] <= 0 || v[2] % 6) return bad(err, cap, "hexagon perimeter");
        expect = v[2] / 6.0;
      } else if (var == 7) {
        unit = " cm" SQ;
        if (v[2] <= 0 || v[3] <= 0 || (v[2] * v[3]) % 2) return bad(err, cap, "perimeter and apothem");
        expect = v[2] * v[3] / 2.0;
      } else if (var == 8) {
        unit = " cm" SQ;
        if (v[2] <= 0 || v[3] <= 0 || ((int64_t)n * v[2] * v[3]) % 2) return bad(err, cap, "side and apothem");
        expect = n * (double)v[2] * v[3] / 2.0;
      } else {
        return bad(err, cap, "regular variant");
      }
      return true;
    }
    case K_CIRCUMFERENCE: {
      unit = PI_ " cm";
      int var = v[0];
      if (var == 0) expect = 2.0 * v[1];
      else if (var == 1) {
        if (v[1] % 2) return bad(err, cap, "odd diameter");
        expect = v[1];
      } else if (var == 2) {
        unit = " cm";
        if (v[1] % 2) return bad(err, cap, "odd length");
        expect = v[1] / 2.0;
      } else if (var == 3) {
        if (v[1] <= 0 || v[2] <= 0) return bad(err, cap, "wheel givens");
        expect = 2.0 * v[1] * v[2];
      } else if (var == 4) {
        if (v[1] <= v[2] || v[2] <= 0) return bad(err, cap, "need R > r");
        expect = 2.0 * (v[1] - v[2]);
      } else {
        return bad(err, cap, "circumference variant");
      }
      return v[1] > 0 ? true : bad(err, cap, "givens must be positive");
    }
    case K_DISC_AREA: {
      unit = PI_ " cm" SQ;
      int var = v[0];
      if (v[1] <= 0) return bad(err, cap, "givens must be positive");
      if (var == 0) expect = (double)v[1] * v[1];
      else if (var == 1 || var == 4) {
        if (v[1] % 2) return bad(err, cap, "odd diameter or side");
        expect = (v[1] / 2.0) * (v[1] / 2.0);
      } else if (var == 2) {
        unit = " cm";
        expect = sqrt((double)v[1]);
        if (!whole(expect)) return bad(err, cap, "the area is not a perfect square");
      } else if (var == 3) {
        if (v[1] % 2) return bad(err, cap, "odd length");
        expect = (v[1] / 2.0) * (v[1] / 2.0);
      } else {
        return bad(err, cap, "disc variant");
      }
      return true;
    }
    case K_ARC_SECTOR: {
      unit = PI_ " cm";
      double n = v[0];
      int var = v[1];
      if (ex.fig.kind != FIG_SECTOR || n < 10 || n > 350 || v[2] <= 0) return bad(err, cap, "sector givens");
      double r = v[2];
      if (var == 0) expect = n / 360.0 * 2.0 * r;
      else if (var == 1) unit = PI_ " cm" SQ, expect = n / 360.0 * r * r;
      else if (var == 2) {
        expect = 360.0 * v[3] / n;  // the whole circle: the arc over its fraction
        if (fabs(expect - 2.0 * r) > 1e-9) return bad(err, cap, "arc does not fit the radius");
      } else if (var == 3) {
        unit = PI_ " cm" SQ;
        expect = 360.0 * v[3] / n;
        if (fabs(expect - r * r) > 1e-9) return bad(err, cap, "sector does not fit the radius");
      } else if (var == 4) {
        unit = " cm";
        expect = 180.0 * v[3] / n;
        if (fabs(expect - r) > 1e-9) return bad(err, cap, "arc does not fit the radius");
      } else if (var == 5) {
        unit = " cm";
        expect = sqrt(360.0 * v[3] / n);
        if (fabs(expect - r) > 1e-9) return bad(err, cap, "sector does not fit the radius");
      } else {
        return bad(err, cap, "arc and sector variant");
      }
      return whole(expect) ? true : bad(err, cap, "the coefficient of pi is not whole");
    }
    case K_ANNULUS: {
      unit = PI_ " cm" SQ;
      if (ex.fig.kind != FIG_ANNULUS || v[1] <= 0 || v[0] <= v[1]) return bad(err, cap, "need R > r > 0");
      expect = ((double)v[0] - v[1]) * ((double)v[0] + v[1]);  // R squared minus r squared, as a product
      return true;
    }
    default:
      return bad(err, cap, "not a unit 5 kind");
  }
}

}  // namespace t7
