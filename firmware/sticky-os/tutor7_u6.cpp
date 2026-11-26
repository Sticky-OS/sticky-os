// Tutor7, Unit 6 (similarity of triangles): the exercise generators and the answers worked out a second way.
//
// Same rules as tutor7_u4.cpp. Every generator draws its numbers constructively (so every answer is a whole number of
// centimetres, or a simplified fraction for a ratio), the options are the typical slips with their reasons, and
// u6Expect() derives the answer again from ex.fig.v alone, by cross-multiplication in doubles, for verify() to compare.
// K_THALES_CONVERSE and K_SIM_CRITERION are choices: u6VerifyChoice() re-derives the verdict from ex.fig.v.
//
// fig.v layout (what the screen draws is documented in tutor7_core.h; the rest is for u6Expect). v[5] is the variant.
//   K_RATIO           variant 0 and 2 (same unit, different units): FIG_NONE v0, v1 = the two lengths in mm, v2, v3 = their units
//                     (0 mm, 1 cm, 2 dm, 3 m), v4 = 1 when the ratio asked is the second length over the first.
//                     1: FIG_SEGMENT v0 = AM, v1 = MB, v2 = what is asked (0 AM/MB, 1 MB/AM, 2 AM/AB, 3 MB/AB).
//                     3: FIG_SEGMENT v0 = AM, v1 = MB, v2 = AB (given), v4 = what is asked (0 AM/MB, 1 MB/AM).
//   K_DIVIDE_SEGMENT  0: FIG_SEGMENT v0 = AM, v1 = MB, v2, v3 = a, b (AM/MB = a/b), v4 = ask (0 AM, 1 MB, 2 AM/AB).
//                     1: FIG_NONE v0..2 = a, b, c (three parts proportional to them), v3 = AB, v4 = ask (0..2 the part, 3 the longest).
//                     2: FIG_SEGMENT v0 = AM, v1 = MB, v2, v3 = a, b (AM/AB = a/b), v4 = ask (0 AM, 1 MB).
//   K_THALES          FIG_THALES v0 = AD, v1 = DB, v2 = the third given length, v3 = ask. Variant 0: given AD, DB and AE (ask 0, find EC)
//                     or EC (ask 1, find AE). 1: given AB, AD and AE (ask 0: EC), AC (ask 1: AE, ask 2: EC). 2: given AD, DB, AC
//                     (ask 0: AE, ask 1: EC).
//   K_THALES_X        FIG_XFIG v0..3 = OA, OB, OC, OD (all four, the unknown included), v4 = the unknown (0..3), v5 = 0: three
//                     given, 1..4: a whole line is given with the part (v5 - 1), 5: the whole line of the unknown is asked.
//   K_THALES_CONVERSE FIG_NONE (the figure would show DE parallel to BC): v0..3 = AD, DB, AE, EC in mm, v4 = variant (0 the parts
//                     in cm, 1 the whole sides, with cm and dm). Options: value[i].p = a code (1..6), see u6VerifyChoice.
//   K_BISECTOR        FIG_BISECTOR v0 = AB, v1 = AC. Variant 0: v2 = BC, v4 = ask (0 BD, 1 DC). 1: v2, v3 = BD, DC, v4 = ask (0 the
//                     unknown is AC, AB given; 1 the unknown is AB, AC given). 2: v2, v3 = BD, DC, v4 = which is given (0 BD, 1 DC),
//                     BC asked.
//   K_SIM_SIDE        v0..2 = AB, BC, CA, v3 = the given side of the second triangle (the one corresponding to side i), v4 = i * 3 + j
//                     (side i given, side j asked, in the order AB, BC, CA), v5 = the order the second triangle is named in
//                     (0 DEF, 1 DFE, 2 EDF, 3 EFD, 4 FDE, 5 FED). FIG_SIMILAR for the order DEF when the second triangle is smaller,
//                     else FIG_NONE.
//   K_SIM_CRITERION   FIG_NONE. v5 = 1000 * type + extra. type 1 (angles): v0, v1 = two angles of ABC, v2, v3 = two angles of DEF,
//                     extra = which vertices. 2 (sides): v0..2 = sides of ABC, v3, v4 = two sides of DEF, v5 % 1000 = the third.
//                     3 (SAS): v0, v1 = AB, AC, v2 = angle A, v3, v4 = DE, DF, extra = angle D. 4 (the angle is not between the
//                     sides): v0, v1 = AB, BC, v2 = angle A, v3, v4 = DE, EF, extra = angle D. Options: value[i].p = 1 AA, 2 SAS, 3 SSS, 4 no.
//   K_SIM_PERIMETER_AREA  FIG_NONE. AB/DE = v0/v1 (k). Variant 0, 1: v2 = the perimeter of ABC (0: DEF asked), of DEF (1: ABC asked).
//                     2: v0, v1 = the perimeters, k asked. 3, 4: v2 = the area of ABC (3) or DEF (4), the other asked. 5: v0, v1 = the
//                     areas of ABC and DEF, k asked.
//   K_TFA             FIG_THALES v0 = AD, v1 = DB, v2 = the given, v3 = ask. Variant 0: ask 0 given BC, find DE; ask 1 given DE, find BC.
//                     1: given BC, find DE. 2: v2 = the perimeter of ABC, the perimeter of ADE asked.
#include "tutor7_internal.h"

#include <utility>

namespace t7 {

#define U(x) TX_U6_##x
#define CM2 " cm" SQ

static int64_t gcd64(int64_t a, int64_t b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b) {
    int64_t t = a % b;
    a = b;
    b = t;
  }
  return a;
}

// Two different numbers with no common factor (a few draws: the draws do not depend on the language).
static bool coprimePair(Rng &rng, int lo, int hi, int &a, int &b) {
  for (int tries = 0; tries < 12; tries++) {
    a = rng.range(lo, hi);
    b = rng.range(lo, hi);
    if (a != b && gcd64(a, b) == 1) return true;
  }
  return false;
}

static const int MMPER[4] = {1, 10, 100, 1000};
static const char *UNAME[4] = {"mm", "cm", "dm", "m"};

// The slips of an exercise: every value is checked (a length must be whole, a ratio positive).
struct Slips {
  Exercise &ex;
  int n;
  bool bad;
  explicit Slips(Exercise &e) : ex(e), n(0), bad(false) {}
  void add(const Exact &v, int id) {
    if (n >= MAX_SLIPS) return;
    ex.slip[n].v = v;
    if (PR(ex.slip[n].why, "%s", tx(ex.lang, id))) bad = true;
    n++;
  }
  void ratio(int64_t p, int64_t q, int id) {
    if (p > 0 && q > 0 && p < 100000 && q < 100000) add(exactFrac((int32_t)p, (int32_t)q), id);
  }
  void length(int64_t num, int64_t den, int id) {
    if (den > 0 && num > 0 && num % den == 0 && num / den < 100000) add(exactInt((int32_t)(num / den)), id);
  }
};

static bool finish(Exercise &ex, Rng &rng, Slips &s, bool bad, const Exact &right, const char *unit) {
  if (bad || s.bad || !setOptions(ex, rng, right, ex.slip, s.n, unit)) return false;
  char tok[48];
  if (exactFormat(right, tok, sizeof tok) < 0) return false;
  for (int i = 0; i < ex.nopt; i++)
    if (i != ex.correct && hasToken(ex.why[i], tok)) return false;
  for (int k = 0; k < 2; k++)
    if (hasToken(ex.clue[k], tok)) return false;
  return hasToken(ex.solution, tok);
}

// "6/9 = 2/3", or "2/3" when it is in lowest terms already, or "6/2 = 3"
static void ratioExpr(char *out, int cap, int64_t p, int64_t q) {
  int64_t g = gcd64(p, q);
  if (q / g == 1) snprintf(out, cap, "%lld/%lld = %lld", (long long)p, (long long)q, (long long)(p / g));
  else if (g == 1) snprintf(out, cap, "%lld/%lld", (long long)p, (long long)q);
  else snprintf(out, cap, "%lld/%lld = %lld/%lld", (long long)p, (long long)q, (long long)(p / g), (long long)(q / g));
}

// A length in a unit, with at most one decimal ("0.3 dm", in the language's separator). False when it cannot be written so.
static bool qty(const Exercise &ex, int64_t mm, int unit, char *out, int cap) {
  int64_t per = MMPER[unit];
  int64_t step = unit == 0 ? 1 : (unit == 3 ? per : per / 10);
  if (mm <= 0 || mm % step) return false;
  int64_t whole = mm / per, rest = unit == 0 ? 0 : (mm % per) / step;
  char num[24];
  if (rest) {
    if (PR(num, TX(U(DEC)), (int)whole, (int)rest)) return false;
  } else {
    snprintf(num, sizeof num, "%d", (int)whole);
  }
  return snprintf(out, cap, "%s %s", num, UNAME[unit]) < cap;
}

static bool join3(const Exercise &ex, char *out, int cap, const char *a, const char *b, const char *c) {
  return snprintf(out, cap, "%s%s%s%s%s", a, TX(TX_LIST_COMMA), b, TX(TX_LIST_AND), c) < cap;
}

static int lvl(int level, int max) { return level < 1 ? 1 : (level > max ? max : level); }

// ================================================================ K_RATIO

static const char *RATIO_NAME[4] = {"AM/MB", "MB/AM", "AM/AB", "MB/AB"};

static bool genRatio(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? rng.range(0, 1) : (rng.range(0, 2) < 2 ? 2 : 3);
  Slips s(ex);
  bool bad = false;
  ex.kind = K_RATIO;
  ex.level = lvl(level, 2);
  Exact right;
  if (var == 0 || var == 2) {
    int a, b;
    if (!coprimePair(rng, 1, 9, a, b)) return false;
    int flip = rng.range(0, 1);
    int u1 = 1, u2 = 1;
    int64_t mm1, mm2;
    if (var == 0) {
      int m = rng.range(1, 5);
      mm1 = 10 * a * m;
      mm2 = 10 * b * m;
    } else {
      static const int PAIRS[6][2] = {{1, 2}, {2, 1}, {0, 1}, {1, 0}, {3, 1}, {1, 3}};
      int p = rng.range(0, 5);
      u1 = PAIRS[p][0];
      u2 = PAIRS[p][1];
      int small = MMPER[u1] < MMPER[u2] ? u1 : u2, big = small == u1 ? u2 : u1;
      int mx = a > b ? a : b;
      int64_t sc;
      if (big == 3) sc = 1000 * rng.range(1, 2);
      else if (small == 0) sc = rng.range(1, 40 / mx > 0 ? 40 / mx : 1);
      else sc = 10 * rng.range(1, 36 / mx > 0 ? 36 / mx : 1);
      mm1 = a * sc;
      mm2 = b * sc;
    }
    char q1[40], q2[40];
    if (!qty(ex, mm1, u1, q1, sizeof q1) || !qty(ex, mm2, u2, q2, sizeof q2)) return false;
    ex.fig = Figure{FIG_NONE, {(int32_t)mm1, (int32_t)mm2, u1, u2, flip, var}};
    const char *rn = flip ? "CD/AB" : "AB/CD";
    bad |= PR(ex.text, TX(U(RAT_Q0)), q1, q2, rn);
    int64_t N = flip ? mm2 : mm1, D = flip ? mm1 : mm2;
    right = exactFrac((int32_t)N, (int32_t)D);
    if (var == 0) {
      int64_t n = N / 10, d = D / 10;
      s.ratio(D, N, U(RAT_SLIP_INV));
      s.ratio(n, n + d, U(RAT_SLIP_SUM));
      s.ratio(d, n + d, U(RAT_SLIP_SUM));
      s.length(n > d ? n - d : d - n, 1, U(RAT_SLIP_DIFF));
      s.ratio(n + d, d, U(RAT_SLIP_PARTS));
      char ex1[64];
      ratioExpr(ex1, sizeof ex1, n, d);
      bad |= PR(ex.solution, TX(U(RAT_SOL0)), rn, ex1);
      bad |= PR(ex.clue[0], TX(U(RAT_H1_0)));
      bad |= PR(ex.clue[1], TX(U(RAT_H2_0)));
    } else {
      int64_t TN = flip ? mm2 * 10 / MMPER[u2] : mm1 * 10 / MMPER[u1];  // the displayed numbers, in tenths
      int64_t TD = flip ? mm1 * 10 / MMPER[u1] : mm2 * 10 / MMPER[u2];
      int fN = MMPER[flip ? u2 : u1], fD = MMPER[flip ? u1 : u2];
      s.ratio(TN, TD, U(RAT_SLIP_RAW));
      s.ratio(D, N, U(RAT_SLIP_INV));
      s.ratio(N, 10 * D, U(RAT_SLIP_POW10));
      s.ratio(10 * N, D, U(RAT_SLIP_POW10));
      s.ratio(TN * fD, TD * fN, U(RAT_SLIP_SWAP));
      int sm = MMPER[u1] < MMPER[u2] ? u1 : u2;
      char c1[80], c2[80], ex1[64];
      if (u1 == sm) snprintf(c1, sizeof c1, "%s", q1);
      else snprintf(c1, sizeof c1, "%s = %d %s", q1, (int)(mm1 / MMPER[sm]), UNAME[sm]);
      if (u2 == sm) snprintf(c2, sizeof c2, "%s", q2);
      else snprintf(c2, sizeof c2, "%s = %d %s", q2, (int)(mm2 / MMPER[sm]), UNAME[sm]);
      ratioExpr(ex1, sizeof ex1, N / MMPER[sm], D / MMPER[sm]);
      bad |= PR(ex.solution, TX(U(RAT_SOL2)), c1, c2, rn, ex1);
      bad |= PR(ex.clue[0], TX(U(RAT_H1_2)));
      bad |= PR(ex.clue[1], TX(U(RAT_H2_2)));
    }
  } else if (var == 1) {
    int a, b;
    if (!coprimePair(rng, 1, 8, a, b)) return false;
    int m = rng.range(1, 5), ask = rng.range(0, 3);
    int AM = a * m, MB = b * m, AB = AM + MB;
    ex.fig = Figure{FIG_SEGMENT, {AM, MB, ask, 0, 0, 1}};
    bad |= PR(ex.text, TX(U(RAT_Q1)), AM, MB, RATIO_NAME[ask]);
    int N = (ask == 0 || ask == 2) ? AM : MB;
    int D = ask < 2 ? (ask == 0 ? MB : AM) : AB;
    right = exactFrac(N, D);
    if (ask < 2) {
      s.ratio(D, N, U(RAT_SLIP_INV));
      s.ratio(N, AB, U(RAT_SLIP_PARTS));
      s.ratio(D, AB, U(RAT_SLIP_PARTS));
      s.ratio(AB, D, U(RAT_SLIP_PARTS));
      s.length(AM > MB ? AM - MB : MB - AM, 1, U(RAT_SLIP_DIFF));
    } else {
      int O = AB - N;  // the other part
      s.ratio(N, O, U(RAT_SLIP_PARTS));
      s.ratio(O, AB, U(RAT_SLIP_PARTS));
      s.ratio(O, N, U(RAT_SLIP_PARTS));
      s.ratio(AB, N, U(RAT_SLIP_INV));
      s.length(O, 1, U(RAT_SLIP_LEN));
    }
    char ex1[64];
    ratioExpr(ex1, sizeof ex1, N, D);
    if (ask < 2) bad |= PR(ex.solution, TX(U(RAT_SOL0)), RATIO_NAME[ask], ex1);
    else bad |= PR(ex.solution, TX(U(RAT_SOL1)), AM, MB, AB, RATIO_NAME[ask], ex1);
    bad |= PR(ex.clue[0], TX(U(RAT_H1_1)));
    bad |= PR(ex.clue[1], TX(U(RAT_H2_1)));
  } else {
    int a, b;
    if (!coprimePair(rng, 1, 8, a, b)) return false;
    int m = rng.range(1, 5), ask = rng.range(0, 1);
    int AM = a * m, MB = b * m, AB = AM + MB;
    ex.fig = Figure{FIG_SEGMENT, {AM, MB, AB, 0, ask, 3}};
    bad |= PR(ex.text, TX(U(RAT_Q3)), AB, AM, RATIO_NAME[ask]);
    int N = ask == 0 ? AM : MB, D = ask == 0 ? MB : AM;
    right = exactFrac(N, D);
    s.ratio(D, N, U(RAT_SLIP_INV));
    s.ratio(N, AB, U(RAT_SLIP_PARTS));
    s.ratio(AB, N, U(RAT_SLIP_PARTS));
    s.length(MB, 1, U(RAT_SLIP_LEN));
    s.length(AB, 1, U(RAT_SLIP_LEN));
    char ex1[64];
    ratioExpr(ex1, sizeof ex1, N, D);
    bad |= PR(ex.solution, TX(U(RAT_SOL3)), AB, AM, MB, RATIO_NAME[ask], ex1);
    bad |= PR(ex.clue[0], TX(U(RAT_H1_3)));
    bad |= PR(ex.clue[1], TX(U(RAT_H2_3)));
  }
  return finish(ex, rng, s, bad, right, "");
}

// ================================================================ K_DIVIDE_SEGMENT

static bool genDivide(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? 0 : (rng.range(0, 1) ? 1 : 2);
  Slips s(ex);
  bool bad = false;
  ex.kind = K_DIVIDE_SEGMENT;
  ex.level = lvl(level, 2);
  Exact right;
  const char *unit = " cm";
  static const uint16_t ASK[3] = {U(DIV_ASK_AM), U(DIV_ASK_MB), U(DIV_ASK_RATIO)};
  if (var == 0) {
    int a, b;
    if (!coprimePair(rng, 1, 6, a, b)) return false;
    int m = rng.range(1, 6), r = rng.range(0, 4);
    int ask = r < 2 ? 0 : (r < 4 ? 1 : 2);
    int AM = a * m, MB = b * m, AB = AM + MB;
    ex.fig = Figure{FIG_SEGMENT, {AM, MB, a, b, ask, 0}};
    bad |= PR(ex.text, TX(U(DIV_Q0)), a, b, AB, TX(ASK[ask]));
    if (ask == 0) {
      right = exactInt(AM);
      s.length(MB, 1, U(SLIP_OTHER));
      s.length(m, 1, U(DIV_SLIP_UNIT));
      s.length(AB, 2, U(DIV_SLIP_HALF));
      s.length(a, 1, U(DIV_SLIP_TERM));
      s.length((int64_t)AB * a, b, U(DIV_SLIP_PARTWHOLE));
      bad |= PR(ex.solution, TX(U(DIV_SOL_AM)), a, b, a + b, AB, a + b, m, a, a, m, AM);
    } else if (ask == 1) {
      right = exactInt(MB);
      s.length(AM, 1, U(SLIP_OTHER));
      s.length(m, 1, U(DIV_SLIP_UNIT));
      s.length(AB, 2, U(DIV_SLIP_HALF));
      s.length(b, 1, U(DIV_SLIP_TERM));
      s.length((int64_t)AB * b, a, U(DIV_SLIP_PARTWHOLE));
      bad |= PR(ex.solution, TX(U(DIV_SOL_MB)), a, b, a + b, AB, a + b, m, b, b, m, MB);
    } else {
      unit = "";
      right = exactFrac(a, a + b);
      s.ratio(a, b, U(DIV_SLIP_PARTS));
      s.ratio(b, a + b, U(DIV_SLIP_OTHERRATIO));
      s.ratio(a + b, a, U(RAT_SLIP_INV));
      s.ratio(b, a, U(DIV_SLIP_PARTS));
      char ex1[64];
      ratioExpr(ex1, sizeof ex1, a, a + b);
      bad |= PR(ex.solution, TX(U(DIV_SOL_RATIO)), a, b, a + b, a, a + b, ex1);
    }
    bad |= PR(ex.clue[0], TX(U(DIV_H1_0)));
    bad |= PR(ex.clue[1], TX(U(DIV_H2_0)));
  } else if (var == 1) {
    int a = 0, b = 0, c = 0;
    for (int tries = 0; tries < 12 && !(a < b && b < c); tries++) {
      a = rng.range(1, 5), b = rng.range(1, 6), c = rng.range(1, 7);
    }
    if (!(a < b && b < c)) return false;
    int S = a + b + c, m = rng.range(1, 6), AB = S * m, ask = rng.range(0, 3);
    int part[3] = {a * m, b * m, c * m};
    int t = ask < 3 ? (ask == 0 ? a : (ask == 1 ? b : c)) : c;
    ex.fig = Figure{FIG_NONE, {a, b, c, AB, ask, 1}};
    static const uint16_t ASK3[4] = {U(DIV_ASK3_AM), U(DIV_ASK3_MN), U(DIV_ASK3_NB), U(DIV_ASK3_MAX)};
    bad |= PR(ex.text, TX(U(DIV_Q1)), AB, a, b, c, TX(ASK3[ask]));
    int ans = t * m;
    right = exactInt(ans);
    for (int i = 0; i < 3; i++)
      if (part[i] != ans) s.length(part[i], 1, U(SLIP_OTHER));
    s.length(m, 1, U(DIV_SLIP_UNIT));
    s.length(AB, 3, U(DIV_SLIP_THIRDS));
    s.length(t, 1, U(DIV_SLIP_TERM));
    bad |= PR(ex.solution, TX(U(DIV_SOL3)), a, b, c, a, b, c, S, AB, S, m, t, t, m, ans);
    bad |= PR(ex.clue[0], TX(U(DIV_H1_1)));
    bad |= PR(ex.clue[1], TX(U(DIV_H2_1)));
  } else {
    int b = rng.range(3, 9), a = rng.range(1, b - 1);
    for (int tries = 0; tries < 12 && gcd64(a, b) != 1; tries++) b = rng.range(3, 9), a = rng.range(1, b - 1);
    if (gcd64(a, b) != 1) return false;
    int m = rng.range(1, 6), ask = rng.range(0, 1);
    int AB = b * m, AM = a * m, MB = AB - AM;
    ex.fig = Figure{FIG_SEGMENT, {AM, MB, a, b, ask, 2}};
    bad |= PR(ex.text, TX(U(DIV_Q2)), AB, a, b, TX(ASK[ask]));
    if (ask == 0) {
      right = exactInt(AM);
      s.length(MB, 1, U(SLIP_OTHER));
      s.length(m, 1, U(DIV_SLIP_UNIT));
      s.length(AB, 2, U(DIV_SLIP_HALF));
      s.length(a, 1, U(DIV_SLIP_TERM));
      s.length((int64_t)AB * a, b - a, U(DIV_SLIP_PARTWHOLE2));
      bad |= PR(ex.solution, TX(U(DIV_SOL2_AM)), b, a, AB, b, m, a, m, AM);
    } else {
      right = exactInt(MB);
      s.length(AM, 1, U(SLIP_OTHER));
      s.length(m, 1, U(DIV_SLIP_UNIT));
      s.length(AB, 2, U(DIV_SLIP_HALF));
      s.length(b - a, 1, U(DIV_SLIP_TERM));
      s.length((int64_t)AB * b, a + b, U(DIV_SLIP_PARTWHOLE2));
      bad |= PR(ex.solution, TX(U(DIV_SOL2_MB)), b, a, AB, b, m, b, a, b - a, b - a, m, MB);
    }
    bad |= PR(ex.clue[0], TX(U(DIV_H1_2)));
    bad |= PR(ex.clue[1], TX(U(DIV_H2_2)));
  }
  return finish(ex, rng, s, bad, right, unit);
}

// ================================================================ K_THALES

static bool genThales(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? 0 : (rng.range(0, 1) ? 1 : 2);
  int a, b;
  if (!coprimePair(rng, 1, 6, a, b)) return false;
  int m1 = rng.range(1, 6), m2 = rng.range(1, 6);
  if (m1 == m2) m2 = m2 % 6 + 1;
  int AD = a * m1, DB = b * m1, AE = a * m2, EC = b * m2, AB = AD + DB, AC = AE + EC;
  Slips s(ex);
  bool bad = false;
  ex.kind = K_THALES;
  ex.level = lvl(level, 2);
  int ask, g, ans;
  if (var == 0) {
    ask = rng.range(0, 1);
    g = ask == 0 ? AE : EC;
    ans = ask == 0 ? EC : AE;
    ex.fig = Figure{FIG_THALES, {AD, DB, g, ask, 0, 0}};
    bad |= PR(ex.text, TX(U(THA_Q0)), AD, DB, ask == 0 ? "AE" : "EC", g, ask == 0 ? "EC" : "AE");
    s.length(AC, 1, U(THA_SLIP_WHOLE));
    if (ask == 0) {
      s.length(AE + DB - AD, 1, U(SLIP_ADD));
      s.length((int64_t)AE * AD, DB, U(SLIP_INV));
      s.length(DB, 1, U(SLIP_COPY));
      s.length(AE, 1, U(SLIP_COPY));
      bad |= PR(ex.solution, TX(U(THA_SOL0A)), AE, DB, AD, EC);
    } else {
      s.length(EC + AD - DB, 1, U(SLIP_ADD));
      s.length((int64_t)EC * DB, AD, U(SLIP_INV));
      s.length(AD, 1, U(SLIP_COPY));
      s.length(EC, 1, U(SLIP_COPY));
      bad |= PR(ex.solution, TX(U(THA_SOL0B)), EC, AD, DB, AE);
    }
  } else if (var == 1) {
    ask = rng.range(0, 2);
    g = ask == 0 ? AE : AC;
    ans = ask == 1 ? AE : EC;
    ex.fig = Figure{FIG_THALES, {AD, DB, g, ask, 0, 1}};
    bad |= PR(ex.text, TX(U(THA_Q1)), AB, AD, ask == 0 ? "AE" : "AC", g, ask == 1 ? "AE" : "EC");
    if (ask == 0) {
      s.length(DB, 1, U(SLIP_STOP));
      s.length(AC, 1, U(SLIP_MIX));
      s.length(AE + DB - AD, 1, U(SLIP_ADD));
      s.length((int64_t)AE * AD, DB, U(SLIP_INV));
      s.length(AE, 1, U(SLIP_COPY));
      bad |= PR(ex.solution, TX(U(THA_SOL1A)), AB, AD, DB, AE, DB, AD, EC);
    } else if (ask == 1) {
      s.length(EC, 1, U(SLIP_OTHER));
      s.length(AC - DB, 1, U(SLIP_ADD));
      s.length(AD, 1, U(SLIP_COPY));
      s.length((int64_t)AC * AD, DB, U(SLIP_MIX));
      s.length(AC, 1, U(SLIP_COPY));
      bad |= PR(ex.solution, TX(U(THA_SOL1B)), AC, AD, AB, AE);
    } else {
      s.length(AE, 1, U(SLIP_OTHER));
      s.length(AC - AD, 1, U(SLIP_ADD));
      s.length(DB, 1, U(SLIP_COPY));
      s.length((int64_t)AC * DB, AD, U(SLIP_MIX));
      s.length(AC, 1, U(SLIP_COPY));
      bad |= PR(ex.solution, TX(U(THA_SOL1C)), AB, AD, DB, AC, DB, AB, EC);
    }
  } else {
    ask = rng.range(0, 1);
    g = AC;
    ans = ask == 0 ? AE : EC;
    ex.fig = Figure{FIG_THALES, {AD, DB, g, ask, 0, 2}};
    bad |= PR(ex.text, TX(U(THA_Q2)), AD, DB, AC, ask == 0 ? "AE" : "EC");
    s.length(ask == 0 ? EC : AE, 1, U(SLIP_OTHER));
    s.length(AB, 1, U(SLIP_STOP));
    s.length(AC - (ask == 0 ? AD : DB), 1, U(SLIP_ADD));
    s.length(ask == 0 ? (int64_t)AC * AD : (int64_t)AC * DB, ask == 0 ? DB : AD, U(SLIP_MIX));
    s.length(ask == 0 ? AD : DB, 1, U(SLIP_COPY));
    if (ask == 0) bad |= PR(ex.solution, TX(U(THA_SOL2A)), AD, DB, AB, AC, AD, AB, AE);
    else bad |= PR(ex.solution, TX(U(THA_SOL2B)), AD, DB, AB, AC, DB, AB, EC);
  }
  bad |= PR(ex.clue[0], TX(var == 0 ? U(THA_H1_0) : (var == 1 ? U(THA_H1_1) : U(THA_H1_2))));
  bad |= PR(ex.clue[1], TX(var == 0 ? U(THA_H2_0) : (var == 1 ? U(THA_H2_1) : U(THA_H2_2))));
  return finish(ex, rng, s, bad, exactInt(ans), " cm");
}

// ================================================================ K_THALES_X

static const char *OLAB[4] = {"OA", "OB", "OC", "OD"};

static bool genThalesX(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? 0 : (rng.range(0, 2) < 2 ? 1 + rng.range(0, 3) : 5);
  int a, b;
  if (!coprimePair(rng, 1, 6, a, b)) return false;
  int m1 = rng.range(1, 6), m2 = rng.range(1, 6);
  if (m1 == m2) m2 = m2 % 6 + 1;
  int w[4] = {a * m1, b * m1, a * m2, b * m2};
  int u, gi = 0;
  if (var >= 1 && var <= 4) {
    gi = var - 1;
    u = 2 * (1 - (gi >> 1)) + rng.range(0, 1);
  } else {
    u = rng.range(0, 3);
  }
  int p = u ^ 1, c = u ^ 2, d = u ^ 3;
  Slips s(ex);
  bool bad = false;
  ex.kind = K_THALES_X;
  ex.level = lvl(level, 2);
  ex.fig = Figure{FIG_XFIG, {w[0], w[1], w[2], w[3], u, var}};
  char it[3][40], list[140];
  const char *asked = OLAB[u];
  int nIt = 0;
  if (var == 0 || var == 5) {
    for (int i = 0; i < 4; i++)
      if (i != u) snprintf(it[nIt++], sizeof it[0], "%s = %d cm", OLAB[i], w[i]);
    if (var == 5) asked = u < 2 ? "AB" : "CD";
  } else {
    snprintf(it[0], sizeof it[0], "%s = %d cm", (gi >> 1) ? "CD" : "AB", w[gi] + w[gi ^ 1]);
    snprintf(it[1], sizeof it[1], "%s = %d cm", OLAB[gi], w[gi]);
    snprintf(it[2], sizeof it[2], "%s = %d cm", OLAB[p], w[p]);
  }
  if (!join3(ex, list, sizeof list, it[0], it[1], it[2])) return false;
  bad |= PR(ex.text, TX(U(THX_Q)), list, asked);
  int ans = var == 5 ? w[u] + w[p] : w[u];
  if (var == 5) {
    s.length(w[u], 1, U(THX_SLIP_PARTONLY));
    s.length(w[c] + w[d], 1, U(THX_SLIP_OTHERLINE));
    s.length(w[p] + w[c] - w[d] + w[p], 1, U(SLIP_ADD));
    s.length(w[p], 1, U(SLIP_COPY));
    s.length(w[c], 1, U(SLIP_COPY));
    s.length((int64_t)w[p] * w[d], w[c], U(SLIP_INV));
    bad |= PR(ex.solution, TX(U(THX_SOL5)), OLAB[u], OLAB[p], OLAB[c], OLAB[d], w[p], w[c], w[d], w[u], asked, OLAB[u], OLAB[p], w[u], w[p],
              w[u] + w[p]);
    bad |= PR(ex.clue[0], TX(U(THX_H1_5)));
  } else {
    s.length(w[u] + w[p], 1, U(THX_SLIP_WHOLE));
    s.length(w[p] + w[c] - w[d], 1, U(SLIP_ADD));
    s.length((int64_t)w[p] * w[d], w[c], U(SLIP_INV));
    s.length(w[c], 1, U(SLIP_COPY));
    s.length(w[p], 1, U(SLIP_COPY));
    if (var == 0) {
      bad |= PR(ex.solution, TX(U(THX_SOL0)), OLAB[u], OLAB[p], OLAB[c], OLAB[d], w[p], w[c], w[d], w[u]);
      bad |= PR(ex.clue[0], TX(U(THX_H1_0)));
    } else {
      bad |= PR(ex.solution, TX(U(THX_SOL1)), (gi >> 1) ? "CD" : "AB", OLAB[gi], OLAB[gi ^ 1], OLAB[gi ^ 1], w[gi] + w[gi ^ 1], w[gi], w[gi ^ 1],
                OLAB[u], OLAB[p], OLAB[c], OLAB[d], w[p], w[c], w[d], w[u]);
      bad |= PR(ex.clue[0], TX(U(THX_H1_1)));
    }
  }
  bad |= PR(ex.clue[1], TX(U(THX_H2)));
  return finish(ex, rng, s, bad, exactInt(ans), " cm");
}

// ================================================================ K_THALES_CONVERSE (a choice)

// Option codes (value[i].p): 1 "yes, AD/DB = AE/EC" (variant 1: AD/AB = AE/AC), 2 "no, the ratios differ", 3 "yes, AD/AB = AE/EC",
// 4 "yes, AD/DB = AE/AC", 5 "no, AD is not equal to AE", 6 "no, AD/AB differs from AE/EC". Codes 1, 3, 4 say yes; 2, 5, 6 say no.
static bool tcVerdictYes(int code) { return code == 1 || code == 3 || code == 4; }
static const uint16_t TC_OPT[2][7] = {{0, U(TCV_O1A), U(TCV_O2A), U(TCV_O3), U(TCV_O4), U(TCV_O5), U(TCV_O6)},
                                      {0, U(TCV_O1B), U(TCV_O2B), U(TCV_O3), U(TCV_O4), U(TCV_O5), U(TCV_O6)}};

static bool genConverse(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? 0 : 1;
  int a, b;
  if (!coprimePair(rng, 1, 6, a, b)) return false;
  int m1 = rng.range(1, 5), m2 = rng.range(1, 5);
  if (m1 == m2) m2 = m2 % 5 + 1;
  int AD = a * m1, DB = b * m1, AE = a * m2, EC = b * m2;
  bool yes = rng.range(0, 1) == 1;
  if (!yes) {
    int which = rng.range(0, 3), delta = rng.range(1, 2) * (rng.range(0, 1) ? 1 : -1);
    int *t[4] = {&AD, &DB, &AE, &EC};
    *t[which] += delta;
    if (*t[which] < 1 || AD == AE || (int64_t)AD * EC == (int64_t)AE * DB) return false;
  }
  int AB = AD + DB, AC = AE + EC;
  bool bad = false;
  ex.kind = K_THALES_CONVERSE;
  ex.level = lvl(level, 2);
  ex.fig = Figure{FIG_NONE, {AD * 10, DB * 10, AE * 10, EC * 10, var, 0}};
  char q[4][40];
  if (var == 0) {
    if (!qty(ex, AD * 10, 1, q[0], sizeof q[0]) || !qty(ex, DB * 10, 1, q[1], sizeof q[1]) || !qty(ex, AE * 10, 1, q[2], sizeof q[2]) ||
        !qty(ex, EC * 10, 1, q[3], sizeof q[3]))
      return false;
    bad |= PR(ex.text, TX(U(TCV_Q0)), q[0], q[1], q[2], q[3]);
  } else {
    int mixed = rng.range(0, 2);  // 1: AB in dm, AD in cm; 2: AC in dm, AE in cm
    if (!qty(ex, AB * 10, mixed == 1 ? 2 : 1, q[0], sizeof q[0]) || !qty(ex, AD * 10, 1, q[1], sizeof q[1]) ||
        !qty(ex, AC * 10, mixed == 2 ? 2 : 1, q[2], sizeof q[2]) || !qty(ex, AE * 10, 1, q[3], sizeof q[3]))
      return false;
    bad |= PR(ex.text, TX(U(TCV_Q1)), q[0], q[1], q[2], q[3]);
  }
  int codes[4], right = yes ? 1 : 2;
  if (yes) {
    codes[0] = 1, codes[1] = 2, codes[2] = 5, codes[3] = 6;
  } else {
    codes[0] = 2, codes[1] = 1, codes[2] = 3, codes[3] = 4;
  }
  for (int i = 3; i > 0; i--) {
    int j = rng.range(0, i), t = codes[i];
    codes[i] = codes[j];
    codes[j] = t;
  }
  ex.nopt = 4;
  for (int i = 0; i < 4; i++) {
    int cd = codes[i];
    ex.value[i] = exactInt(cd);
    bad |= PR(ex.opt[i], "%s", TX(TC_OPT[var][cd]));
    ex.why[i][0] = 0;
    if (cd == right) {
      ex.correct = i;
      continue;
    }
    int id = (cd == 1 || cd == 2) ? U(TCV_WHY_COMPARE) : (cd == 5 ? U(TCV_WHY_EQUAL) : U(TCV_WHY_MIX));
    bad |= PR(ex.why[i], "%s", TX(id));
  }
  char r1[64], r2[64];
  if (var == 0) {
    ratioExpr(r1, sizeof r1, AD, DB);
    ratioExpr(r2, sizeof r2, AE, EC);
    bad |= PR(ex.solution, TX(yes ? U(TCV_SOL_YES_A) : U(TCV_SOL_NO_A)), r1, r2);
  } else {
    ratioExpr(r1, sizeof r1, AD, AB);
    ratioExpr(r2, sizeof r2, AE, AC);
    bad |= PR(ex.solution, TX(yes ? U(TCV_SOL_YES_B) : U(TCV_SOL_NO_B)), r1, r2);
  }
  bad |= PR(ex.clue[0], TX(var == 0 ? U(TCV_H1_A) : U(TCV_H1_B)));
  bad |= PR(ex.clue[1], TX(U(TCV_H2)));
  return !bad;
}

// ================================================================ K_BISECTOR

static bool genBisector(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? 0 : (rng.range(0, 1) ? 1 : 2);
  int a, b;
  if (!coprimePair(rng, 1, 6, a, b)) return false;
  int amb = a > b ? a - b : b - a;
  int m = 0, tlo = 1, thi = 0;
  for (int tries = 0; tries < 12 && tlo > thi; tries++) {
    m = rng.range(2, 8);
    tlo = (m * amb) / (a + b) + 1;
    thi = m - 1;
  }
  if (tlo > thi) return false;
  int t = rng.range(tlo, thi);
  int AB = a * m, AC = b * m, BD = a * t, DC = b * t, BC = BD + DC;
  Slips s(ex);
  bool bad = false;
  ex.kind = K_BISECTOR;
  ex.level = lvl(level, 2);
  int ans;
  if (var == 0) {
    int ask = rng.range(0, 1);
    ex.fig = Figure{FIG_BISECTOR, {AB, AC, BC, 0, ask, 0}};
    bad |= PR(ex.text, TX(U(BIS_Q0)), AB, AC, BC, ask == 0 ? "BD" : "DC");
    ans = ask == 0 ? BD : DC;
    s.length(ask == 0 ? DC : BD, 1, U(SLIP_OTHER));
    s.length(BC, 2, U(BIS_SLIP_HALF));
    s.length(ask == 0 ? (int64_t)BC * AB : (int64_t)BC * AC, ask == 0 ? AC : AB, U(SLIP_MIX));
    s.length(ask == 0 ? AB : AC, 1, U(SLIP_COPY));
    s.length(BC - (ask == 0 ? AB : AC), 1, U(SLIP_ADD));
    bad |= PR(ex.solution, TX(U(BIS_SOL0)), ask == 0 ? "BD" : "DC", ask == 0 ? "AB" : "AC", BC, ask == 0 ? AB : AC, AB, AC, ans);
    bad |= PR(ex.clue[0], TX(U(BIS_H1_0)));
    bad |= PR(ex.clue[1], TX(U(BIS_H2_0)));
  } else if (var == 1) {
    int ask = rng.range(0, 1);  // 0: AB given, AC asked; 1: AC given, AB asked
    ex.fig = Figure{FIG_BISECTOR, {AB, AC, BD, DC, ask, 1}};
    bad |= PR(ex.text, TX(U(BIS_Q1)), BD, DC, ask == 0 ? "AB" : "AC", ask == 0 ? AB : AC, ask == 0 ? "AC" : "AB");
    ans = ask == 0 ? AC : AB;
    int g = ask == 0 ? AB : AC;
    s.length(ask == 0 ? (int64_t)g * BD : (int64_t)g * DC, ask == 0 ? DC : BD, U(SLIP_INV));
    s.length(g + (ask == 0 ? DC - BD : BD - DC), 1, U(SLIP_ADD));
    s.length(BC, 1, U(SLIP_STOP));
    s.length(g, 1, U(SLIP_COPY));
    s.length(ask == 0 ? DC : BD, 1, U(SLIP_OTHER));
    if (ask == 0) bad |= PR(ex.solution, TX(U(BIS_SOL1)), "AC", "AB", "DC", "BD", AB, DC, BD, AC);
    else bad |= PR(ex.solution, TX(U(BIS_SOL1)), "AB", "AC", "BD", "DC", AC, BD, DC, AB);
    bad |= PR(ex.clue[0], TX(U(BIS_H1_1)));
    bad |= PR(ex.clue[1], TX(U(BIS_H2_1)));
  } else {
    int given = rng.range(0, 1);  // 0 BD given, 1 DC given
    ex.fig = Figure{FIG_BISECTOR, {AB, AC, BD, DC, given, 2}};
    bad |= PR(ex.text, TX(U(BIS_Q2)), AB, AC, given == 0 ? "BD" : "DC", given == 0 ? BD : DC);
    ans = BC;
    s.length(given == 0 ? DC : BD, 1, U(SLIP_STOP));
    s.length((given == 0 ? (int64_t)BD * (AB + AC) : (int64_t)DC * (AB + AC)), given == 0 ? AC : AB, U(SLIP_INV));
    s.length((given == 0 ? BD : DC) + (given == 0 ? AC : AB), 1, U(SLIP_ADD));
    s.length(AB + AC, 1, U(SLIP_ADD));
    s.length(given == 0 ? BD : DC, 1, U(SLIP_COPY));
    if (given == 0) bad |= PR(ex.solution, TX(U(BIS_SOL2)), "DC", "BD", "AC", "AB", BD, AC, AB, DC, BD, DC, BC);
    else bad |= PR(ex.solution, TX(U(BIS_SOL2)), "BD", "DC", "AB", "AC", DC, AB, AC, BD, BD, DC, BC);
    bad |= PR(ex.clue[0], TX(U(BIS_H1_2)));
    bad |= PR(ex.clue[1], TX(U(BIS_H2_2)));
  }
  return finish(ex, rng, s, bad, exactInt(ans), " cm");
}

// ================================================================ K_SIM_SIDE

static const char *SIDEN[3] = {"AB", "BC", "CA"};
static const char *PERM3[6] = {"DEF", "DFE", "EDF", "EFD", "FDE", "FED"};
// The second triangle's side that corresponds to side k of ABC: the k-th and (k+1)-th letters of its name.
static void sideName(int perm, int k, char *out) {
  out[0] = PERM3[perm][k];
  out[1] = PERM3[perm][(k + 1) % 3];
  out[2] = 0;
}
static int alphaIndex(const char *n) {  // DE 0, EF 1, DF 2, whatever the order of the letters
  char x = n[0] < n[1] ? n[0] : n[1], y = n[0] < n[1] ? n[1] : n[0];
  return x == 'D' && y == 'E' ? 0 : (x == 'E' && y == 'F' ? 1 : 2);
}

static bool genSimSide(int level, Rng &rng, Exercise &ex) {
  static const int K[9][2] = {{2, 1}, {3, 1}, {4, 1}, {3, 2}, {5, 2}, {1, 2}, {1, 3}, {2, 3}, {3, 4}};
  static const int U3[10][3] = {{3, 4, 5}, {4, 5, 6}, {2, 3, 4}, {5, 6, 7}, {3, 5, 7}, {4, 6, 7}, {5, 7, 8}, {3, 4, 6}, {4, 5, 7}, {5, 6, 8}};
  const int *kk = K[rng.range(0, 8)];
  int p = kk[0], q = kk[1];
  const int *uu = U3[rng.range(0, 9)];
  int u[3] = {uu[0], uu[1], uu[2]};
  for (int i = 2; i > 0; i--) {
    int j = rng.range(0, i), t = u[i];
    u[i] = u[j];
    u[j] = t;
  }
  int F[3], T[3];
  for (int i = 0; i < 3; i++) F[i] = q * u[i], T[i] = p * u[i];
  int perm = level <= 1 ? 0 : rng.range(1, 5);
  int i = rng.range(0, 2), j = (i + rng.range(1, 2)) % 3, m = 3 - i - j;
  Slips s(ex);
  bool bad = false;
  ex.kind = K_SIM_SIDE;
  ex.level = lvl(level, 2);
  ex.fig = Figure{(perm == 0 && p < q) ? FIG_SIMILAR : FIG_NONE, {F[0], F[1], F[2], T[i], i * 3 + j, perm}};
  char ni[3], nj[3], nm[3];
  sideName(perm, i, ni);
  sideName(perm, j, nj);
  sideName(perm, m, nm);
  bad |= PR(ex.text, TX(U(SS_Q)), PERM3[perm], SIDEN[i], F[i], SIDEN[j], F[j], PERM3[perm], ni, T[i], nj);
  int ans = T[j];
  s.length((int64_t)T[i] * F[i], F[j], U(SLIP_INV));
  s.length(T[i] + F[j] - F[i], 1, U(SLIP_ADD));
  s.length((int64_t)T[i] * F[m], F[i], U(SS_SLIP_PAIR));
  s.length((int64_t)T[i] * F[j], F[m], U(SS_SLIP_PAIR));
  s.length(F[j], 1, U(SLIP_COPY));
  s.length(T[i], 1, U(SLIP_COPY));
  if (perm != 0) {
    int ai = alphaIndex(ni), aj = alphaIndex(nj);
    s.length((int64_t)T[i] * F[aj], F[ai], U(SS_SLIP_ORDER));
  }
  char l0[2] = {PERM3[perm][0], 0}, l1[2] = {PERM3[perm][1], 0}, l2[2] = {PERM3[perm][2], 0};
  bad |= PR(ex.solution, TX(U(SS_SOL)), l0, l1, l2, SIDEN[i], ni, SIDEN[j], nj, ni, SIDEN[i], nj, SIDEN[j], nj, T[i], F[j], F[i], ans);
  bad |= PR(ex.clue[0], TX(U(SS_H1)));
  bad |= PR(ex.clue[1], TX(U(SS_H2)));
  return finish(ex, rng, s, bad, exactInt(ans), " cm");
}

// ================================================================ K_SIM_CRITERION (a choice)

// Option codes (value[i].p): 1 AA, 2 SAS, 3 SSS, 4 no / not enough data.
static const uint16_t CRI_LABEL[5] = {0, U(CRI_L_UU), U(CRI_L_LUL), U(CRI_L_LLL), U(CRI_L_NO)};

static bool sortedSimilar(const int *s3, const int *t3) {
  int s[3] = {s3[0], s3[1], s3[2]}, t[3] = {t3[0], t3[1], t3[2]};
  for (int i = 0; i < 3; i++)
    for (int j = i + 1; j < 3; j++) {
      if (s[j] < s[i]) {
        int x = s[i];
        s[i] = s[j];
        s[j] = x;
      }
      if (t[j] < t[i]) {
        int x = t[i];
        t[i] = t[j];
        t[j] = x;
      }
    }
  return (int64_t)s[0] * t[1] == (int64_t)s[1] * t[0] && (int64_t)s[1] * t[2] == (int64_t)s[2] * t[1];
}

static bool genCriterion(int level, Rng &rng, Exercise &ex) {
  int type, nearMiss = 0, yes = rng.range(0, 1);
  if (level <= 1) {
    type = 1;
  } else if (level == 2) {
    type = rng.range(0, 1) ? 2 : 3;
  } else {
    int r = rng.range(0, 19);
    type = r < 7 ? 4 : (r < 11 ? 2 : (r < 14 ? 3 : (r < 17 ? 2 : 1)));
    if (r < 7) yes = 0;
    if (r >= 7 && r < 11) nearMiss = 1, yes = 0;
    if (r >= 14 && r < 17) yes = 1;
  }
  bool bad = false;
  ex.kind = K_SIM_CRITERION;
  ex.level = lvl(level, 3);
  int want;
  const int hintGroup = type == 1 ? 0 : (type == 2 ? 1 : 2);
  if (type == 1) {
    int x = 5 * rng.range(6, 20), y = 5 * rng.range(6, 20), z = 180 - x - y;
    if (z < 20 || z > 120 || x == y || y == z || x == z) return false;
    int ang[3] = {x, y, z}, sec[3];
    for (int i = 0; i < 3; i++) sec[i] = ang[i];
    int shared = ang[rng.range(0, 2)];
    if (yes) {
      for (int i = 2; i > 0; i--) {
        int j = rng.range(0, i), t = sec[i];
        sec[i] = sec[j];
        sec[j] = t;
      }
    } else {
      int y2 = 5 * rng.range(6, 20);
      if (y2 == x || y2 == y || y2 == z) return false;
      int z2 = 180 - shared - y2;
      if (z2 < 20 || z2 > 120) return false;
      sec[0] = shared, sec[1] = y2, sec[2] = z2;
      for (int i = 2; i > 0; i--) {
        int j = rng.range(0, i), t = sec[i];
        sec[i] = sec[j];
        sec[j] = t;
      }
    }
    static const int PAIR[3][2] = {{0, 1}, {0, 2}, {1, 2}};
    int p1 = rng.range(0, 2), p2 = rng.range(0, 2);
    if (!yes) {  // the equal angle must be visible among the two given
      int pos = 0;
      for (int i = 0; i < 3; i++)
        if (sec[i] == shared) pos = i;
      int tries = 0;
      while ((PAIR[p2][0] != pos && PAIR[p2][1] != pos) && tries++ < 8) p2 = rng.range(0, 2);
      if (PAIR[p2][0] != pos && PAIR[p2][1] != pos) return false;
    }
    const char *L1 = "ABC", *L2 = "DEF";
    char la[2][2] = {{L1[PAIR[p1][0]], 0}, {L1[PAIR[p1][1]], 0}}, lb[2][2] = {{L2[PAIR[p2][0]], 0}, {L2[PAIR[p2][1]], 0}};
    int a1 = ang[PAIR[p1][0]], a2 = ang[PAIR[p1][1]], b1 = sec[PAIR[p2][0]], b2 = sec[PAIR[p2][1]];
    ex.fig = Figure{FIG_NONE, {a1, a2, b1, b2, 0, 1000 + p1 * 3 + p2}};
    bad |= PR(ex.text, TX(U(CRI_Q_UU)), la[0], a1, la[1], a2, lb[0], b1, lb[1], b2);
    want = yes ? 1 : 4;
    int t1 = 180 - a1 - a2, t2 = 180 - b1 - b2;
    int sa[3] = {a1, a2, t1}, sb[3] = {b1, b2, t2};
    for (int i = 0; i < 3; i++)
      for (int j = i + 1; j < 3; j++) {
        if (sa[j] < sa[i]) std::swap(sa[i], sa[j]);
        if (sb[j] < sb[i]) std::swap(sb[i], sb[j]);
      }
    bad |= PR(ex.solution, TX(yes ? U(CRI_SOL_UU_YES) : U(CRI_SOL_UU_NO)), t1, t2, sa[0], sa[1], sa[2], sb[0], sb[1], sb[2]);
  } else if (type == 2) {
    static const int U3[12][3] = {{3, 4, 5}, {4, 5, 6}, {2, 3, 4}, {5, 6, 7}, {3, 5, 7}, {4, 6, 7}, {5, 7, 8}, {3, 4, 6}, {4, 5, 7}, {5, 6, 8}, {6, 7, 9}, {4, 7, 9}};
    const int *u = U3[rng.range(0, 11)];
    int f1 = rng.range(1, 3), f2 = rng.range(1, 3);
    if (f1 == f2 || u[2] * (f1 > f2 ? f1 : f2) > 28) return false;
    int s1[3], s2[3];
    for (int i = 0; i < 3; i++) s1[i] = f1 * u[i], s2[i] = f2 * u[i];
    if (!yes) {
      int k = rng.range(0, 2), d = nearMiss ? 1 : rng.range(2, 3);
      s2[k] += rng.range(0, 1) ? d : -d;
      int mx = s2[0] > s2[1] ? (s2[0] > s2[2] ? s2[0] : s2[2]) : (s2[1] > s2[2] ? s2[1] : s2[2]);
      if (s2[k] < 1 || s2[0] + s2[1] + s2[2] <= 2 * mx || sortedSimilar(s1, s2)) return false;
    }
    int srtA[3] = {s1[0], s1[1], s1[2]}, srtB[3] = {s2[0], s2[1], s2[2]};
    for (int i = 0; i < 3; i++)
      for (int j = i + 1; j < 3; j++) {
        if (srtA[j] < srtA[i]) std::swap(srtA[i], srtA[j]);
        if (srtB[j] < srtB[i]) std::swap(srtB[i], srtB[j]);
      }
    for (int i = 2; i > 0; i--) {
      int j = rng.range(0, i), t = s1[i];
      s1[i] = s1[j];
      s1[j] = t;
      j = rng.range(0, i);
      t = s2[i];
      s2[i] = s2[j];
      s2[j] = t;
    }
    ex.fig = Figure{FIG_NONE, {s1[0], s1[1], s1[2], s2[0], s2[1], 2000 + s2[2]}};
    bad |= PR(ex.text, TX(U(CRI_Q_LLL)), s1[0], s1[1], s1[2], s2[0], s2[1], s2[2]);
    want = yes ? 3 : 4;
    char red[64];
    {
      int64_t gg = gcd64(srtA[0], srtB[0]);
      if (srtB[0] / gg == 1) snprintf(red, sizeof red, "%lld", (long long)(srtA[0] / gg));
      else snprintf(red, sizeof red, "%lld/%lld", (long long)(srtA[0] / gg), (long long)(srtB[0] / gg));
    }
    if (yes) bad |= PR(ex.solution, TX(U(CRI_SOL_LLL_YES)), srtA[0], srtA[1], srtA[2], srtB[0], srtB[1], srtB[2], srtA[0], srtB[0], srtA[1], srtB[1], srtA[2], srtB[2], red);
    else bad |= PR(ex.solution, TX(U(CRI_SOL_LLL_NO)), srtA[0], srtA[1], srtA[2], srtB[0], srtB[1], srtB[2], srtA[0], srtB[0], srtA[1], srtB[1], srtA[2], srtB[2]);
  } else {
    // type 3: the angle is between the sides; type 4: it is not (AB and BC with angle A)
    int x = rng.range(2, 8), y = rng.range(2, 9);
    if (x == y || gcd64(x, y) != 1) return false;
    if (type == 4 && y > x) std::swap(x, y);  // the side opposite the angle is the shorter one
    int f1 = rng.range(1, 3), f2 = rng.range(1, 3);
    if (f1 == f2) return false;
    int AB = f1 * x, AC = f1 * y, DE = f2 * x, DF = f2 * y;
    int angA = type == 4 ? 5 * rng.range(6, 9) : 5 * rng.range(8, 16), angD = angA;
    bool angleDiffers = false;
    if (type == 4) {
      want = 4;
      if (rng.range(0, 4) == 0) DF += rng.range(0, 1) ? 1 : -1;  // sometimes the sides are not even proportional
      if (DF < 1 || AC >= AB || DF >= DE) return false;  // the side opposite the angle is the shorter: the data really do not decide
      double sn = sin(angA * 3.14159265358979 / 180.0);
      if (!(AC > AB * sn * 1.05) || !(DF > DE * sn * 1.05)) return false;  // such a triangle exists
    } else if (yes) {
      want = 2;
    } else {
      want = 4;
      if (rng.range(0, 1)) {
        DF += rng.range(0, 1) ? rng.range(1, 3) : -rng.range(1, 3);
        if (DF < 1 || (int64_t)AB * DF == (int64_t)AC * DE) return false;
      } else {
        angD = angA + (rng.range(0, 1) ? 1 : -1) * 5 * rng.range(1, 4);
        angleDiffers = true;
      }
    }
    ex.fig = Figure{FIG_NONE, {AB, AC, angA, DE, DF, type * 1000 + angD}};
    bad |= PR(ex.text, TX(type == 3 ? U(CRI_Q_SAS) : U(CRI_Q_TRAP)), AB, AC, angA, DE, DF, angD);
    char r1[64], r2[64];
    ratioExpr(r1, sizeof r1, AB, DE);
    ratioExpr(r2, sizeof r2, AC, DF);
    if (type == 4) bad |= PR(ex.solution, "%s", TX(U(CRI_SOL_TRAP)));
    else if (yes) bad |= PR(ex.solution, TX(U(CRI_SOL_SAS_YES)), r1, r2, angA);
    else if (angleDiffers) bad |= PR(ex.solution, TX(U(CRI_SOL_SAS_NOANG)), r1, r2, angA, angD);
    else bad |= PR(ex.solution, TX(U(CRI_SOL_SAS_NOSIDES)), r1, r2);
  }
  // the options: the four answers, in a random order
  int codes[4] = {1, 2, 3, 4};
  for (int i = 3; i > 0; i--) {
    int j = rng.range(0, i), t = codes[i];
    codes[i] = codes[j];
    codes[j] = t;
  }
  ex.nopt = 4;
  for (int i = 0; i < 4; i++) {
    int cd = codes[i];
    ex.value[i] = exactInt(cd);
    bad |= PR(ex.opt[i], "%s", TX(CRI_LABEL[cd]));
    ex.why[i][0] = 0;
    if (cd == want) {
      ex.correct = i;
      continue;
    }
    // the reason depends on what was given (angles, three sides, two sides and an angle) and on what is right
    int id;
    if (type == 1) id = cd == 2 ? U(CRI_W_ANG_SAS) : (cd == 3 ? U(CRI_W_ANG_SSS) : (cd == 4 ? U(CRI_W_ANG_NO) : U(CRI_W_ANG_UU)));
    else if (type == 2) id = cd == 1 ? U(CRI_W_SSS_UU) : (cd == 2 ? U(CRI_W_SSS_SAS) : (cd == 3 ? U(CRI_W_SSS_LLL) : U(CRI_W_SSS_NO)));
    else id = cd == 1 ? U(CRI_W_SAS_UU) : (cd == 3 ? U(CRI_W_SAS_SSS) : (cd == 2 ? U(CRI_W_SAS_SAS) : U(CRI_W_SAS_NO)));
    bad |= PR(ex.why[i], "%s", TX(id));
  }
  static const uint16_t H1[3] = {U(CRI_H1_UU), U(CRI_H1_LLL), U(CRI_H1_LUL)};
  static const uint16_t H2[3] = {U(CRI_H2_UU), U(CRI_H2_LLL), U(CRI_H2_LUL)};
  bad |= PR(ex.clue[0], "%s", TX(H1[hintGroup]));
  bad |= PR(ex.clue[1], "%s", TX(H2[hintGroup]));
  return !bad;
}

// ================================================================ K_SIM_PERIMETER_AREA

static bool genSimPA(int level, Rng &rng, Exercise &ex) {
  static const int K[10][2] = {{1, 2}, {1, 3}, {2, 3}, {3, 2}, {2, 1}, {3, 1}, {3, 4}, {4, 3}, {2, 5}, {5, 2}};
  int var = level <= 1 ? rng.range(0, 2) : 3 + rng.range(0, 2);
  const int *k = K[rng.range(0, 9)];
  int kn = k[0], kd = k[1];
  Slips s(ex);
  bool bad = false;
  ex.kind = K_SIM_PERIMETER_AREA;
  ex.level = lvl(level, 2);
  Exact right;
  const char *unit = " cm";
  if (var <= 1) {
    int base = kn * kn * kd, tmin = (12 + base - 1) / base, tmax = 60 / base;
    if (tmax < tmin) return false;
    int t = rng.range(tmin, tmax), s0 = kd * kn * t;
    int P1 = kn * s0, P2 = kd * s0;
    int g = var == 0 ? P1 : P2, ans = var == 0 ? P2 : P1;
    ex.fig = Figure{FIG_NONE, {kn, kd, g, 0, 0, var}};
    bad |= PR(ex.text, TX(var == 0 ? U(SPA_Q0) : U(SPA_Q1)), kn, kd, g);
    right = exactInt(ans);
    if (var == 0) {
      s.length((int64_t)g * kd * kd, (int64_t)kn * kn, U(SPA_SLIP_SQUARE));
      s.length((int64_t)g * kn, kd, U(SPA_SLIP_WAY));
    } else {
      s.length((int64_t)g * kn * kn, (int64_t)kd * kd, U(SPA_SLIP_SQUARE));
      s.length((int64_t)g * kd, kn, U(SPA_SLIP_WAY));
    }
    s.length(g, 1, U(SLIP_COPY));
    s.length(var == 0 ? g + kd - kn : g + kn - kd, 1, U(SLIP_ADD));
    if (var == 0) bad |= PR(ex.solution, TX(U(SPA_SOL0)), kn, kd, g, kd, kn, ans);
    else bad |= PR(ex.solution, TX(U(SPA_SOL1)), kn, kd, g, kn, kd, ans);
    bad |= PR(ex.clue[0], TX(U(SPA_H1_P)));
    bad |= PR(ex.clue[1], TX(U(SPA_H2_P)));
  } else if (var == 2) {
    int s0 = rng.range(2, 9);
    int P1 = kn * s0, P2 = kd * s0;
    ex.fig = Figure{FIG_NONE, {P1, P2, 0, 0, 0, 2}};
    bad |= PR(ex.text, TX(U(SPA_Q2)), P1, P2);
    unit = "";
    right = exactFrac(kn, kd);
    s.ratio(kd, kn, U(RAT_SLIP_INV));
    s.ratio((int64_t)kn * kn, (int64_t)kd * kd, U(SPA_SLIP_SQUARERATIO));
    s.ratio((int64_t)kd * kd, (int64_t)kn * kn, U(SPA_SLIP_SQUARERATIO));
    s.length(P1 > P2 ? P1 - P2 : P2 - P1, 1, U(RAT_SLIP_DIFF));
    char ex1[64];
    ratioExpr(ex1, sizeof ex1, P1, P2);
    bad |= PR(ex.solution, TX(U(SPA_SOL2)), ex1);
    bad |= PR(ex.clue[0], TX(U(SPA_H1_K)));
    bad |= PR(ex.clue[1], TX(U(SPA_H2_K)));
  } else if (var <= 4) {
    int c = rng.range(1, 6);
    int S1 = kn * kn * c, S2 = kd * kd * c;
    int g = var == 3 ? S1 : S2, ans = var == 3 ? S2 : S1;
    ex.fig = Figure{FIG_NONE, {kn, kd, g, 0, 0, var}};
    bad |= PR(ex.text, TX(var == 3 ? U(SPA_Q3) : U(SPA_Q4)), kn, kd, g);
    unit = CM2;
    right = exactInt(ans);
    if (var == 3) {
      s.length((int64_t)g * kd, kn, U(SPA_SLIP_NOSQUARE));
      s.length((int64_t)g * kn, kd, U(SPA_SLIP_NOSQUARE));
      s.length((int64_t)g * kn * kn, (int64_t)kd * kd, U(SPA_SLIP_WAY));
    } else {
      s.length((int64_t)g * kn, kd, U(SPA_SLIP_NOSQUARE));
      s.length((int64_t)g * kd, kn, U(SPA_SLIP_NOSQUARE));
      s.length((int64_t)g * kd * kd, (int64_t)kn * kn, U(SPA_SLIP_WAY));
    }
    s.length(g, 1, U(SLIP_COPY));
    s.length(var == 3 ? g + kd * kd - kn * kn : g + kn * kn - kd * kd, 1, U(SLIP_ADD));
    if (var == 3) bad |= PR(ex.solution, TX(U(SPA_SOL3)), kn, kd, kn * kn, kd * kd, g, kd * kd, kn * kn, ans);
    else bad |= PR(ex.solution, TX(U(SPA_SOL4)), kn, kd, kn * kn, kd * kd, g, kn * kn, kd * kd, ans);
    bad |= PR(ex.clue[0], TX(U(SPA_H1_A)));
    bad |= PR(ex.clue[1], TX(U(SPA_H2_A)));
  } else {
    int c = rng.range(1, 6);
    int S1 = kn * kn * c, S2 = kd * kd * c;
    ex.fig = Figure{FIG_NONE, {S1, S2, 0, 0, 0, 5}};
    bad |= PR(ex.text, TX(U(SPA_Q5)), S1, S2);
    unit = "";
    right = exactFrac(kn, kd);
    s.ratio((int64_t)kn * kn, (int64_t)kd * kd, U(SPA_SLIP_AREARATIO));
    s.ratio(kd, kn, U(RAT_SLIP_INV));
    s.ratio((int64_t)kd * kd, (int64_t)kn * kn, U(SPA_SLIP_AREARATIO));
    s.length(S1 > S2 ? S1 - S2 : S2 - S1, 1, U(RAT_SLIP_DIFF));
    char ex1[64];
    ratioExpr(ex1, sizeof ex1, S1, S2);
    char kk[32];
    if (kd == 1) snprintf(kk, sizeof kk, "%d", kn);
    else snprintf(kk, sizeof kk, "%d/%d", kn, kd);
    bad |= PR(ex.solution, TX(U(SPA_SOL5)), ex1, kk);
    bad |= PR(ex.clue[0], TX(U(SPA_H1_AK)));
    bad |= PR(ex.clue[1], TX(U(SPA_H2_AK)));
  }
  return finish(ex, rng, s, bad, right, unit);
}

// ================================================================ K_TFA

static bool genTfa(int level, Rng &rng, Exercise &ex) {
  int var = level <= 1 ? 0 : (rng.range(0, 1) ? 1 : 2);
  int a = 0, c = 0;
  for (int tries = 0; tries < 12 && !(c > a && gcd64(a, c) == 1); tries++) a = rng.range(1, 5), c = rng.range(2, 7);
  if (!(c > a && gcd64(a, c) == 1)) return false;
  int m1 = rng.range(1, 6), m2 = rng.range(1, 6);
  if (m1 == m2) m2 = m2 % 6 + 1;
  int AD = a * m1, AB = c * m1, DB = AB - AD, BC = c * m2, DE = a * m2;
  Slips s(ex);
  bool bad = false;
  ex.kind = K_TFA;
  ex.level = lvl(level, 2);
  int ans, ask = 0, g;
  if (var == 0) {
    ask = rng.range(0, 1);
    g = ask == 0 ? BC : DE;
    ans = ask == 0 ? DE : BC;
    ex.fig = Figure{FIG_THALES, {AD, DB, g, ask, 0, 0}};
    bad |= PR(ex.text, TX(U(TFA_Q0)), AD, AB, ask == 0 ? "BC" : "DE", g, ask == 0 ? "DE" : "BC");
    if (ask == 0) {
      s.length((int64_t)BC * AD, DB, U(SLIP_MIX));
      s.length(BC - DE, 1, U(SLIP_OTHER));
      s.length(BC - DB, 1, U(SLIP_ADD));
      s.length((int64_t)BC * AB, AD, U(SLIP_INV));
      s.length(AD, 1, U(SLIP_COPY));
      bad |= PR(ex.solution, TX(U(TFA_SOL0A)), BC, AD, AB, DE);
    } else {
      s.length((int64_t)DE * DB, AD, U(SLIP_MIX));
      s.length(DE + DB, 1, U(SLIP_ADD));
      s.length((int64_t)DE * AD, AB, U(SLIP_INV));
      s.length(AB, 1, U(SLIP_COPY));
      s.length(DE, 1, U(SLIP_COPY));
      bad |= PR(ex.solution, TX(U(TFA_SOL0B)), DE, AB, AD, BC);
    }
  } else if (var == 1) {
    g = BC;
    ans = DE;
    ex.fig = Figure{FIG_THALES, {AD, DB, g, 0, 0, 1}};
    bad |= PR(ex.text, TX(U(TFA_Q1)), AD, DB, BC);
    s.length((int64_t)BC * AD, DB, U(SLIP_MIX));
    s.length(AB, 1, U(SLIP_STOP));
    s.length(BC - DB, 1, U(SLIP_ADD));
    s.length((int64_t)BC * AB, AD, U(SLIP_INV));
    s.length(BC - DE, 1, U(SLIP_OTHER));
    bad |= PR(ex.solution, TX(U(TFA_SOL1)), AD, DB, AB, BC, AD, AB, DE);
  } else {
    int s0 = rng.range(2, 6);
    int P = AB * s0;
    ans = AD * s0;
    g = P;
    ex.fig = Figure{FIG_THALES, {AD, DB, g, 0, 0, 2}};
    bad |= PR(ex.text, TX(U(TFA_Q2)), P, AD, DB);
    s.length((int64_t)P * AD, DB, U(SLIP_MIX));
    s.length(P - DB, 1, U(SLIP_ADD));
    s.length((int64_t)P * DB, AB, U(SLIP_OTHER));
    s.length(AB, 1, U(SLIP_STOP));
    s.length(AD, 1, U(SLIP_COPY));
    s.length((int64_t)P * AB, AD, U(SLIP_INV));
    bad |= PR(ex.solution, TX(U(TFA_SOL2)), AD, DB, AB, P, AD, AB, ans);
  }
  bad |= PR(ex.clue[0], TX(var == 2 ? U(TFA_H1_2) : U(TFA_H1)));
  bad |= PR(ex.clue[1], TX(var == 2 ? U(TFA_H2_2) : U(TFA_H2)));
  return finish(ex, rng, s, bad, exactInt(ans), " cm");
}

// ================================================================ entry points

bool u6Generate(Kind kind, int level, Rng &rng, Exercise &ex) {
  switch (kind) {
    case K_RATIO: return genRatio(level, rng, ex);
    case K_DIVIDE_SEGMENT: return genDivide(level, rng, ex);
    case K_THALES: return genThales(level, rng, ex);
    case K_THALES_X: return genThalesX(level, rng, ex);
    case K_THALES_CONVERSE: return genConverse(level, rng, ex);
    case K_BISECTOR: return genBisector(level, rng, ex);
    case K_SIM_SIDE: return genSimSide(level, rng, ex);
    case K_SIM_CRITERION: return genCriterion(level, rng, ex);
    case K_SIM_PERIMETER_AREA: return genSimPA(level, rng, ex);
    case K_TFA: return genTfa(level, rng, ex);
    default: return false;
  }
}

// ================================================================ the second derivation

static bool bad(char *err, int cap, const char *msg) {
  if (err && cap > 0) snprintf(err, cap, "%s", msg);
  return false;
}

bool u6Expect(const Exercise &ex, double &expect, const char *&unit, char *err, int cap) {
  const int32_t *v = ex.fig.v;
  const FigureKind fk = ex.fig.kind;
  unit = " cm";
  switch (ex.kind) {
    case K_RATIO: {
      unit = "";
      int var = v[5];
      if (var == 0 || var == 2) {
        if (fk != FIG_NONE || v[0] <= 0 || v[1] <= 0 || (v[4] != 0 && v[4] != 1)) return bad(err, cap, "ratio figure");
        if (v[2] < 0 || v[2] > 3 || v[3] < 0 || v[3] > 3 || (var == 0) != (v[2] == v[3])) return bad(err, cap, "ratio units");
        expect = v[4] ? (double)v[1] / v[0] : (double)v[0] / v[1];
        return true;
      }
      if (fk != FIG_SEGMENT || v[0] <= 0 || v[1] <= 0) return bad(err, cap, "ratio segment figure");
      if (var == 1) {
        double am = v[0], mb = v[1];
        switch (v[2]) {
          case 0: expect = am / mb; return true;
          case 1: expect = mb / am; return true;
          case 2: expect = am / (am + mb); return true;
          case 3: expect = mb / (am + mb); return true;
          default: return bad(err, cap, "ratio ask");
        }
      }
      if (var == 3) {
        double am = v[0], mb = (double)v[2] - v[0];
        if (mb <= 0 || fabs(mb - v[1]) > 1e-9) return bad(err, cap, "AB must be more than AM and MB = AB - AM");
        if (v[4] == 0) expect = am / mb;
        else if (v[4] == 1) expect = mb / am;
        else return bad(err, cap, "ratio ask");
        return true;
      }
      return bad(err, cap, "ratio variant");
    }
    case K_DIVIDE_SEGMENT: {
      int var = v[5];
      if (var == 0 || var == 2) {
        if (fk != FIG_SEGMENT || v[0] <= 0 || v[1] <= 0 || v[2] <= 0 || v[3] <= 0) return bad(err, cap, "divide figure");
        double AB = (double)v[0] + v[1], a = v[2], b = v[3];
        if (var == 0) {
          if (fabs(v[0] * b - v[1] * a) > 1e-9) return bad(err, cap, "AM/MB is not a/b");
          if (v[4] == 0) expect = AB * a / (a + b);
          else if (v[4] == 1) expect = AB * b / (a + b);
          else if (v[4] == 2) {
            unit = "";
            expect = a / (a + b);
          } else return bad(err, cap, "divide ask");
        } else {
          if (b <= a || fabs(v[0] * b - AB * a) > 1e-9) return bad(err, cap, "AM/AB is not a/b");
          if (v[4] == 0) expect = AB * a / b;
          else if (v[4] == 1) expect = AB * (b - a) / b;
          else return bad(err, cap, "divide ask");
        }
        return true;
      }
      if (var == 1) {
        if (fk != FIG_NONE || v[0] <= 0 || v[1] <= v[0] || v[2] <= v[1] || v[3] <= 0) return bad(err, cap, "three parts");
        double S = (double)v[0] + v[1] + v[2], AB = v[3];
        if (v[4] >= 0 && v[4] <= 2) expect = AB * v[v[4]] / S;
        else if (v[4] == 3) expect = AB * v[2] / S;
        else return bad(err, cap, "divide ask");
        return true;
      }
      return bad(err, cap, "divide variant");
    }
    case K_THALES: {
      if (fk != FIG_THALES || v[0] <= 0 || v[1] <= 0 || v[2] <= 0) return bad(err, cap, "Thales figure");
      double AD = v[0], DB = v[1], AB = AD + DB, g = v[2];
      switch (v[5] * 10 + v[3]) {
        case 0: expect = g * DB / AD; return true;   // AE given: EC
        case 1: expect = g * AD / DB; return true;   // EC given: AE
        case 10: expect = g * (AB - AD) / AD; return true;
        case 11: expect = g * AD / AB; return true;  // AC given: AE
        case 12: expect = g * (AB - AD) / AB; return true;  // AC given: EC
        case 20: expect = g * AD / AB; return true;
        case 21: expect = g * DB / AB; return true;
        default: return bad(err, cap, "Thales variant");
      }
    }
    case K_THALES_X: {
      if (fk != FIG_XFIG) return bad(err, cap, "X figure");
      for (int i = 0; i < 4; i++)
        if (v[i] <= 0) return bad(err, cap, "X length");
      if (v[4] < 0 || v[4] > 3 || v[5] < 0 || v[5] > 5) return bad(err, cap, "X variant");
      if ((int64_t)v[0] * v[3] != (int64_t)v[1] * v[2]) return bad(err, cap, "OA/OB is not OC/OD");
      // the unknown from the other three: u * d = p * c (u opposite d, p the other part of u's line, c the corresponding segment)
      int u = v[4];
      double p = v[u ^ 1], c = v[u ^ 2], d = v[u ^ 3];
      expect = p * c / d;
      if (v[5] >= 1 && v[5] <= 4) {
        int gi = v[5] - 1;
        if ((gi >> 1) == (u >> 1)) return bad(err, cap, "the given whole line must be the other one");
      }
      if (v[5] == 5) expect += p;
      return true;
    }
    case K_BISECTOR: {
      if (fk != FIG_BISECTOR || v[0] <= 0 || v[1] <= 0) return bad(err, cap, "bisector figure");
      double AB = v[0], AC = v[1];
      switch (v[5]) {
        case 0: {
          double BC = v[2];
          if (BC <= fabs(AB - AC) || BC >= AB + AC) return bad(err, cap, "not a triangle");
          if (v[4] == 0) expect = BC * AB / (AB + AC);
          else if (v[4] == 1) expect = BC * AC / (AB + AC);
          else return bad(err, cap, "bisector ask");
          return true;
        }
        case 1: {
          double BD = v[2], DC = v[3];
          if (BD <= 0 || DC <= 0 || fabs(BD * AC - DC * AB) > 1e-9) return bad(err, cap, "BD/DC is not AB/AC");
          if (BD + DC <= fabs(AB - AC) || BD + DC >= AB + AC) return bad(err, cap, "not a triangle");
          if (v[4] == 0) expect = AB * DC / BD;
          else if (v[4] == 1) expect = AC * BD / DC;
          else return bad(err, cap, "bisector ask");
          return true;
        }
        case 2: {
          double BD = v[2], DC = v[3];
          if (BD <= 0 || DC <= 0 || fabs(BD * AC - DC * AB) > 1e-9) return bad(err, cap, "BD/DC is not AB/AC");
          if (v[4] == 0) expect = BD * (AB + AC) / AB;
          else if (v[4] == 1) expect = DC * (AB + AC) / AC;
          else return bad(err, cap, "bisector ask");
          return true;
        }
        default: return bad(err, cap, "bisector variant");
      }
    }
    case K_SIM_SIDE: {
      if (fk != FIG_NONE && fk != FIG_SIMILAR) return bad(err, cap, "similar figure");
      for (int i = 0; i < 4; i++)
        if (v[i] <= 0) return bad(err, cap, "side");
      int i = v[4] / 3, j = v[4] % 3;
      if (v[4] < 0 || v[4] > 8 || i == j || v[5] < 0 || v[5] > 5) return bad(err, cap, "similar sides");
      if (fk == FIG_SIMILAR && v[5] != 0) return bad(err, cap, "the figure has the letters in order");
      if (v[0] + v[1] <= v[2] || v[1] + v[2] <= v[0] || v[0] + v[2] <= v[1]) return bad(err, cap, "not a triangle");
      expect = (double)v[3] * v[j] / v[i];
      return true;
    }
    case K_SIM_PERIMETER_AREA: {
      if (fk != FIG_NONE) return bad(err, cap, "perimeter/area figure");
      for (int i = 0; i < 2; i++)
        if (v[i] <= 0) return bad(err, cap, "ratio number");
      double k = (double)v[0] / v[1];
      switch (v[5]) {
        case 0: if (v[2] <= 0) return bad(err, cap, "perimeter"); expect = v[2] / k; return true;
        case 1: if (v[2] <= 0) return bad(err, cap, "perimeter"); expect = v[2] * k; return true;
        case 2: unit = ""; expect = (double)v[0] / v[1]; return true;
        case 3: unit = " cm" SQ; if (v[2] <= 0) return bad(err, cap, "area"); expect = v[2] / (k * k); return true;
        case 4: unit = " cm" SQ; if (v[2] <= 0) return bad(err, cap, "area"); expect = v[2] * (k * k); return true;
        case 5: {
          unit = "";
          expect = sqrt((double)v[0] / v[1]);
          // the ratio of the areas must be the square of a fraction
          double q = round(expect * 1000.0) / 1000.0;
          bool ok = false;
          for (int d = 1; d <= 12 && !ok; d++) {
            double n = expect * d;
            if (fabs(n - round(n)) < 1e-9) ok = true;
          }
          (void)q;
          return ok ? true : bad(err, cap, "the areas are not in the ratio of a square");
        }
        default: return bad(err, cap, "perimeter/area variant");
      }
    }
    case K_TFA: {
      if (fk != FIG_THALES || v[0] <= 0 || v[1] <= 0 || v[2] <= 0) return bad(err, cap, "T.F.A. figure");
      double AD = v[0], AB = AD + v[1], g = v[2];
      switch (v[5] * 10 + v[3]) {
        case 0: expect = g * AD / AB; return true;  // BC given: DE
        case 1: expect = g * AB / AD; return true;  // DE given: BC
        case 10: expect = g * AD / AB; return true;
        case 20: expect = g * AD / AB; return true;  // perimeter of ABC given: perimeter of ADE
        default: return bad(err, cap, "T.F.A. variant");
      }
    }
    default: return bad(err, cap, "unknown Unit 6 kind");
  }
}

bool u6VerifyChoice(const Exercise &ex, char *err, int cap) {
  const int32_t *v = ex.fig.v;
  if (ex.fig.kind != FIG_NONE) return bad(err, cap, "choice figure");
  int found = 0, want = 0;
  if (ex.kind == K_THALES_CONVERSE) {
    int var = v[4];
    if (var != 0 && var != 1) return bad(err, cap, "converse variant");
    for (int i = 0; i < 4; i++)
      if (v[i] <= 0) return bad(err, cap, "converse length");
    bool parallel = (int64_t)v[0] * v[3] == (int64_t)v[2] * v[1];  // AD * EC = AE * DB
    want = parallel ? 1 : 2;
    if (ex.nopt != 4) return bad(err, cap, "converse options");
    for (int i = 0; i < ex.nopt; i++) {
      int code = ex.value[i].p;
      if (code < 1 || code > 6) return bad(err, cap, "option code");
      if (strcmp(ex.opt[i], TX(TC_OPT[var][code]))) return bad(err, cap, "option text does not match its code");
      for (int j = 0; j < i; j++)
        if (ex.value[j].p == code) return bad(err, cap, "two options with the same code");
      bool yes = tcVerdictYes(code);
      if (code == want) {
        found++;
        if (i != ex.correct) return bad(err, cap, "the right option is marked wrong");
      } else if (yes == parallel) {
        return bad(err, cap, "a wrong option has the right verdict");
      }
    }
    if (var == 0 && (v[0] % 10 || v[1] % 10 || v[2] % 10 || v[3] % 10)) return bad(err, cap, "parts are in whole cm");
    if (v[0] == v[2]) return bad(err, cap, "AD equals AE");
    return found == 1 ? true : bad(err, cap, "no unique right option");
  }
  // K_SIM_CRITERION
  int type = v[5] / 1000, extra = v[5] % 1000;
  switch (type) {
    case 1: {
      for (int i = 0; i < 4; i++)
        if (v[i] <= 0 || v[i] >= 180) return bad(err, cap, "angle");
      if (v[0] + v[1] >= 180 || v[2] + v[3] >= 180) return bad(err, cap, "angles add up to 180 or more");
      int a[3] = {v[0], v[1], 180 - v[0] - v[1]}, b[3] = {v[2], v[3], 180 - v[2] - v[3]};
      for (int i = 0; i < 3; i++)
        for (int j = i + 1; j < 3; j++) {
          if (a[j] < a[i]) std::swap(a[i], a[j]);
          if (b[j] < b[i]) std::swap(b[i], b[j]);
        }
      want = (a[0] == b[0] && a[1] == b[1] && a[2] == b[2]) ? 1 : 4;
      break;
    }
    case 2: {
      int s[3] = {v[0], v[1], v[2]}, t[3] = {v[3], v[4], extra};
      for (int i = 0; i < 3; i++)
        if (s[i] <= 0 || t[i] <= 0) return bad(err, cap, "side");
      int ms = s[0] > s[1] ? (s[0] > s[2] ? s[0] : s[2]) : (s[1] > s[2] ? s[1] : s[2]);
      int mt = t[0] > t[1] ? (t[0] > t[2] ? t[0] : t[2]) : (t[1] > t[2] ? t[1] : t[2]);
      if (s[0] + s[1] + s[2] <= 2 * ms || t[0] + t[1] + t[2] <= 2 * mt) return bad(err, cap, "not a triangle");
      want = sortedSimilar(s, t) ? 3 : 4;
      break;
    }
    case 3:
      if (v[0] <= 0 || v[1] <= 0 || v[3] <= 0 || v[4] <= 0 || v[2] <= 0 || v[2] >= 180 || extra <= 0 || extra >= 180) return bad(err, cap, "SAS numbers");
      want = ((int64_t)v[0] * v[4] == (int64_t)v[1] * v[3] && v[2] == extra) ? 2 : 4;
      break;
    case 4:
      if (v[0] <= 0 || v[1] <= 0 || v[3] <= 0 || v[4] <= 0 || v[2] <= 0 || v[2] >= 180 || extra != v[2]) return bad(err, cap, "trap numbers");
      want = 4;  // the angle is not between the sides: nothing follows
      break;
    default: return bad(err, cap, "criterion type");
  }
  for (int i = 0; i < ex.nopt; i++) {
    int code = ex.value[i].p;
    if (code < 1 || code > 4) return bad(err, cap, "option code");
    if (strcmp(ex.opt[i], TX(CRI_LABEL[code]))) return bad(err, cap, "option text does not match its code");
    for (int j = 0; j < i; j++)
      if (ex.value[j].p == code) return bad(err, cap, "two options with the same code");
    if (code == want) {
      found++;
      if (i != ex.correct) return bad(err, cap, "the right option is marked wrong");
    }
  }
  return found == 1 ? true : bad(err, cap, "no unique right option");
}

}  // namespace t7
