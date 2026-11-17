// Tutor7, Unit 4 (the quadrilateral): the exercise generators and the answers worked out a second way.
//
// Same rules as tutor7_core.cpp. The question, the options and the explanations come from numbers, the key
// from a solver; u4Expect() then derives the answer again from the figure's givens (ex.fig.v) alone, in a
// different form, and verify() compares the two. The clues and the explanations of wrong answers hold no
// numbers (only formulas in words), so they cannot give the answer away.
//
// The fonts have no angle sign, parallel sign or arrows: angles are written "unghiul A", parallel lines
// "laturile AB și CD sunt paralele".
#include "tutor7_internal.h"

namespace t7 {

static int mult5(Rng &r, int lo, int hi) { return 5 * r.range((lo + 4) / 5, hi / 5); }
static int evenIn(Rng &r, int lo, int hi) { return 2 * r.range((lo + 1) / 2, hi / 2); }

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

static const char *V4[4] = {"A", "B", "C", "D"};
#define ANG DEG
#define CM2 " cm" SQ

// ---------------------------------------------------------------- Lesson 1: the sum of the angles

static bool genFourthAngle(Rng &rng, Exercise &ex) {
  int a[4];
  int sum = 0;
  for (int i = 0; i < 4; i++) a[i] = mult5(rng, 50, 130), sum += a[i];
  // make the four add up to 360: the last one is whatever is left
  int rest = 360 - (a[0] + a[1] + a[2]);
  if (rest < 40 || rest > 140) return false;
  a[3] = rest;
  int u = rng.range(0, 3);  // the unknown vertex
  int g[3], gi = 0;
  const char *gl[3];
  for (int i = 0; i < 4; i++)
    if (i != u) g[gi] = a[i], gl[gi++] = V4[i];
  int S = g[0] + g[1] + g[2];
  bool bad = false;
  ex.kind = K_Q_FOURTH_ANGLE;
  ex.level = 1;
  ex.fig = Figure{FIG_QUAD, {g[0], g[1], g[2], 0}};
  bad |= PR(ex.text, TX(TX_Q4_Q), gl[0],
            g[0], gl[1], g[1], gl[2], g[2], V4[u]);
  Cand *c = ex.slip;
  int nc = 0;
  SLIP(c, nc, exactInt(S), TX(TX_Q4_SLIP1));
  SLIP(c, nc, exactInt(abs(180 - S)), TX(TX_Q4_SLIP2));
  SLIP(c, nc, exactInt(360 - (g[0] + g[1])), TX(TX_Q4_SLIP3));
  SLIP(c, nc, exactInt(360 - (g[1] + g[2])), TX(TX_Q4_SLIP3));
  SLIP(c, nc, exactInt(360 - (g[0] + g[2])), TX(TX_Q4_SLIP3));
  bad |= PR(ex.clue[0], TX(TX_Q4_HINT1));
  bad |= PR(ex.clue[1], TX(TX_Q4_HINT2));
  bad |= PR(ex.solution, TX(TX_Q4_SOL),
            V4[u], g[0], g[1], g[2], S, a[u]);
  return finish(ex, rng, bad, exactInt(a[u]), c, nc, ANG);
}

static bool genProportional(Rng &rng, Exercise &ex) {
  static const int SETS[][4] = {{1, 2, 3, 4}, {2, 4, 6, 8}, {3, 4, 5, 6}, {2, 3, 4, 6}, {1, 2, 4, 5}, {2, 3, 5, 8},
                                {3, 4, 6, 7}, {8, 9, 11, 12}, {1, 3, 4, 4}, {3, 5, 6, 6}, {2, 3, 3, 4}};
  const int *s = SETS[rng.range(0, (int)(sizeof SETS / sizeof SETS[0]) - 1)];
  int r[4] = {s[0], s[1], s[2], s[3]};
  for (int i = 3; i > 0; i--) {
    int j = rng.range(0, i), t = r[i];
    r[i] = r[j];
    r[j] = t;
  }
  int S = r[0] + r[1] + r[2] + r[3], m = 0;
  for (int i = 0; i < 4; i++) m = r[i] > m ? r[i] : m;
  if (360 % S) return false;
  int part = 360 / S, big = part * m;
  if (big >= 180) return false;
  bool bad = false;
  ex.kind = K_Q_PROPORTIONAL;
  ex.level = 2;
  ex.fig = Figure{FIG_QUAD, {r[0], r[1], r[2], r[3]}};
  bad |= PR(ex.text, TX(TX_QPROP_Q), r[0], r[1], r[2], r[3]);
  Cand *c = ex.slip;
  int nc = 0;
  SLIP(c, nc, exactInt(90), TX(TX_QPROP_SLIP1));
  if (180 * m % S == 0)  // the angle "with 180°" is a whole number of degrees, or the slip is not offered
    SLIP(c, nc, exactInt(180 * m / S), TX(TX_QPROP_SLIP2));
  SLIP(c, nc, exactInt(m), TX(TX_QPROP_SLIP3));
  SLIP(c, nc, exactInt(part), TX(TX_QPROP_SLIP4));
  SLIP(c, nc, exactInt(360 - big), TX(TX_QPROP_SLIP5));
  bad |= PR(ex.clue[0], TX(TX_QPROP_HINT1));
  bad |= PR(ex.clue[1], TX(TX_QPROP_HINT2));
  bad |= PR(ex.solution, TX(TX_QPROP_SOL),
            r[0], r[1], r[2], r[3], S, S, part, m, part, big);
  return finish(ex, rng, bad, exactInt(big), c, nc, ANG);
}

// ---------------------------------------------------------------- Lesson 2: the parallelogram

static bool genParAngle(Rng &rng, Exercise &ex) {
  int A = rng.range(0, 1) ? mult5(rng, 40, 85) : mult5(rng, 95, 140);
  int which = rng.range(0, 2);  // 0 B, 1 C, 2 D
  const char *name[3] = {"B", "C", "D"};
  int ans = which == 1 ? A : 180 - A;
  bool bad = false;
  ex.kind = K_PAR_ANGLE;
  ex.level = 1;
  ex.fig = Figure{FIG_PARALLELOGRAM, {A, which, 0, 0}};
  bad |= PR(ex.text, TX(TX_PANG_Q), A, name[which]);
  Cand *c = ex.slip;
  int nc = 0;
  if (which == 1) {
    SLIP(c, nc, exactInt(180 - A), TX(TX_PANG_SLIP1));
    if (A < 90)  // only an acute angle can be taken from 90°
      SLIP(c, nc, exactInt(90 - A), TX(TX_PANG_SLIP2));
    SLIP(c, nc, exactInt(360 - A), TX(TX_PANG_SLIP3));
    SLIP(c, nc, exactInt(360 - 2 * A), TX(TX_PANG_SLIP4));
  } else {
    SLIP(c, nc, exactInt(A), TX(TX_PANG_SLIP5), name[which]);
    SLIP(c, nc, exactInt(abs(90 - A)), TX(TX_PANG_SLIP6));
    SLIP(c, nc, exactInt(360 - A), TX(TX_PANG_SLIP7));
    SLIP(c, nc, exactInt(360 - 2 * A), TX(TX_PANG_SLIP4));
  }
  bad |= PR(ex.clue[0], TX(TX_PANG_HINT1));
  bad |= PR(ex.clue[1], TX(TX_PANG_HINT2));
  if (which == 1)
    bad |= PR(ex.solution, TX(TX_PANG_SOL1), A);
  else
    bad |= PR(ex.solution, TX(TX_PANG_SOL2), name[which], name[which], A, ans);
  return finish(ex, rng, bad, exactInt(ans), c, nc, ANG);
}

static bool genParPerimeter(Rng &rng, Exercise &ex) {
  bool bad = false;
  ex.kind = K_PAR_PERIMETER;
  ex.level = 1;
  ex.fig = Figure{FIG_PARALLELOGRAM, {0, 0, 0, 0}};
  Cand *c = ex.slip;
  int nc = 0;
  if (rng.range(0, 1)) {  // the sides are given: the perimeter
    int a = rng.range(3, 15), b = rng.range(3, 15);
    if (a == b) return false;
    int P = 2 * (a + b);
    ex.fig.v[0] = a, ex.fig.v[1] = b, ex.fig.v[2] = 0;
    bad |= PR(ex.text, TX(TX_PPER_Q1), a, b);
    SLIP(c, nc, exactInt(a + b), TX(TX_PPER_SLIP1));
    SLIP(c, nc, exactInt(a * b), TX(TX_PPER_SLIP2));
    SLIP(c, nc, exactInt(2 * a + b), TX(TX_PPER_SLIP3));
    SLIP(c, nc, exactInt(a + 2 * b), TX(TX_PPER_SLIP3));
    SLIP(c, nc, exactInt(4 * (a > b ? a : b)), TX(TX_PPER_SLIP4));
    bad |= PR(ex.clue[0], TX(TX_PPER_HINT1_1));
    bad |= PR(ex.clue[1], TX(TX_PPER_HINT2_1));
    bad |= PR(ex.solution, TX(TX_PPER_SOL1), a, b, a + b, P);
    return finish(ex, rng, bad, exactInt(P), c, nc, " cm");
  }
  int a = rng.range(3, 12), b = rng.range(3, 12);  // the perimeter and one side: the other
  if (a == b) return false;
  int P = 2 * (a + b);
  ex.fig.v[0] = P, ex.fig.v[1] = a, ex.fig.v[2] = 1;
  bad |= PR(ex.text, TX(TX_PPER_Q2), P, a);
  SLIP(c, nc, exactInt(P / 2), TX(TX_PPER_SLIP5));
  SLIP(c, nc, exactInt(P - a), TX(TX_PPER_SLIP6));
  SLIP(c, nc, exactInt(P - 2 * a), TX(TX_PPER_SLIP7));
  SLIP(c, nc, exactFrac(P, 4), TX(TX_PPER_SLIP8));
  bad |= PR(ex.clue[0], TX(TX_PPER_HINT1_2));
  bad |= PR(ex.clue[1], TX(TX_PPER_HINT2_2));
  bad |= PR(ex.solution, TX(TX_PPER_SOL2), P, P / 2, P / 2, a, b);
  return finish(ex, rng, bad, exactInt(b), c, nc, " cm");
}

static bool genParDiagonal(Rng &rng, Exercise &ex) {
  int ao = rng.range(3, 12), bo = rng.range(3, 12);
  if (ao == bo) return false;
  int ask = rng.range(0, 1);  // 0 AC, 1 BD
  int ans = ask == 0 ? 2 * ao : 2 * bo;
  bool bad = false;
  ex.kind = K_PAR_DIAGONAL;
  ex.level = 1;
  ex.fig = Figure{FIG_PARALLELOGRAM, {ao, bo, ask, 0}};
  bad |= PR(ex.text, TX(TX_PDIAG_Q), ao, bo, ask == 0 ? "AC" : "BD");
  Cand *c = ex.slip;
  int nc = 0;
  int mine = ask == 0 ? ao : bo, other = ask == 0 ? bo : ao;
  SLIP(c, nc, exactInt(mine), TX(TX_PDIAG_SLIP1));
  SLIP(c, nc, exactInt(ao + bo), TX(TX_PDIAG_SLIP2));
  SLIP(c, nc, exactInt(2 * other), TX(TX_PDIAG_SLIP3));
  SLIP(c, nc, exactInt(2 * (ao + bo)), TX(TX_PDIAG_SLIP4));
  bad |= PR(ex.clue[0], TX(TX_PDIAG_HINT1));
  bad |= PR(ex.clue[1], TX(TX_PDIAG_HINT2));
  bad |= PR(ex.solution, TX(TX_PDIAG_SOL), ask == 0 ? "AC" : "BD", ask == 0 ? "AC" : "BD",
            ask == 0 ? "AO" : "BO", mine, ans);
  return finish(ex, rng, bad, exactInt(ans), c, nc, " cm");
}

// Which figure do these properties guarantee? The curated cases below are the characterisations and the
// non-characterisations of the textbook; classifyRule() is the same knowledge as rules, used by verify().
enum : int {
  F_P = 1, F_OC2 = 2, F_DB = 4, F_PC = 8, F_RA = 16, F_DC = 32, F_DP = 64, F_CS = 128, F_R3 = 256, F_S4 = 512, F_R2 = 1024, F_PAR1 = 2048
};
struct Case {
  int flags;
  int answer;  // 0 only a convex quadrilateral, 1 parallelogram, 2 rectangle, 3 rhombus, 4 square
};
static const Case CASES[] = {
    {F_P | F_RA, 2},        {F_P | F_DC, 2},         {F_P | F_CS, 3},         {F_P | F_DP, 3},        {F_P | F_RA | F_CS, 4},
    {F_P | F_DC | F_DP, 4}, {F_R3, 2},               {F_S4, 3},               {F_S4 | F_RA, 4},       {F_OC2, 1},
    {F_DB, 1},              {F_PC, 1},               {F_DB | F_DC, 2},        {F_DB | F_DP, 3},       {F_RA, 0},
    {F_R2, 0},              {F_DC, 0},               {F_DP, 0},               {F_CS, 0},              {F_PAR1, 0},
    {F_PAR1 | F_RA, 0},     {F_OC2 | F_RA, 2},       {F_DB | F_CS, 3},        {F_PC | F_DP, 3},       {F_OC2 | F_DC, 2},
    {F_P | F_RA | F_DP, 4}, {F_DB | F_RA | F_CS, 4},
};
static const int NCASES = sizeof CASES / sizeof CASES[0];
// The answers' names, by code (0 only a convex quadrilateral ... 4 square).
static const uint16_t CLASS_LABEL[5] = {TX_CLS_LABEL_QUAD, TX_CLS_LABEL_PAR, TX_CLS_LABEL_RECT, TX_CLS_LABEL_RHOMBUS, TX_CLS_LABEL_SQUARE};
// When only "a convex quadrilateral" is guaranteed: why each other name is wrong (by code; 0 unused).
static const uint16_t CLASS_NOT_ENOUGH[5] = {TX_CLS_WHY_NOT_PAR, TX_CLS_WHY_NOT_PAR, TX_CLS_WHY_NOT_RECT, TX_CLS_WHY_NOT_RHOMBUS, TX_CLS_WHY_NOT_SQUARE};

static int classifyRule(int f) {
  bool par = f & (F_P | F_OC2 | F_DB | F_PC | F_S4 | F_R3);
  bool rect = (par && (f & (F_RA | F_DC))) || (f & F_R3);
  bool rho = (par && (f & (F_CS | F_DP))) || (f & F_S4);
  if (rect && rho) return 4;
  if (rect) return 2;
  if (rho) return 3;
  return par ? 1 : 0;
}

static bool genClassify(Rng &rng, Exercise &ex) {
  const Case &cs = CASES[rng.range(0, NCASES - 1)];
  int f = cs.flags, ans = cs.answer;
  // the properties, in words
  char list[200] = "";
  const char *parts[8];
  int np = 0;
  bool dbMerged = false;
  if ((f & F_DB) && (f & F_DC)) parts[np++] = TX(TX_CLS_P_DIAG_CONG_BISECT), dbMerged = true;
  else if ((f & F_DB) && (f & F_DP)) parts[np++] = TX(TX_CLS_P_DIAG_PERP_BISECT), dbMerged = true;
  else if (f & F_DB) parts[np++] = TX(TX_CLS_P_DIAG_BISECT);
  if (f & F_OC2) parts[np++] = TX(TX_CLS_P_OPP_SIDES_CONG);
  if (f & F_PC) parts[np++] = TX(TX_CLS_P_PAIR_PAR_CONG);
  if (f & F_PAR1) parts[np++] = TX(TX_CLS_P_PAIR_PAR);
  if (f & F_S4) parts[np++] = TX(TX_CLS_P_ALL_SIDES_CONG);
  if (f & F_CS) parts[np++] = TX(TX_CLS_P_ADJ_SIDES_CONG);
  if (f & F_R3) parts[np++] = TX(TX_CLS_P_THREE_RIGHT);
  if (f & F_R2) parts[np++] = TX(TX_CLS_P_TWO_RIGHT);
  if (f & F_RA) parts[np++] = TX(TX_CLS_P_ONE_RIGHT);
  if (!dbMerged) {
    if ((f & F_DC) && (f & F_DP)) parts[np++] = TX(TX_CLS_P_DIAG_CONG_PERP);
    else if (f & F_DC) parts[np++] = TX(TX_CLS_P_DIAG_CONG);
    else if (f & F_DP) parts[np++] = TX(TX_CLS_P_DIAG_PERP);
  }
  if (!np) return false;
  for (int i = 0; i < np; i++) {
    size_t used = strlen(list);
    const char *sep = i == 0 ? "" : (i == np - 1 ? TX(TX_LIST_AND) : TX(TX_LIST_COMMA));
    if (snprintf(list + used, sizeof list - used, "%s%s", sep, parts[i]) >= (int)(sizeof list - used)) return false;
  }
  bool bad = false;
  ex.kind = K_CLASSIFY;
  ex.level = 3;
  ex.fig = Figure{FIG_QUAD, {f, 0, 0, 0}};
  bad |= PR(ex.text, (f & F_P) ? TX(TX_CLS_Q_PAR) : TX(TX_CLS_Q_QUAD), list);
  // the options: the right label and three others, shuffled
  int codes[5] = {0, 1, 2, 3, 4}, pick[4], npick = 0;
  pick[npick++] = ans;
  for (int i = 4; i > 0; i--) {
    int j = rng.range(0, i), t = codes[i];
    codes[i] = codes[j];
    codes[j] = t;
  }
  for (int i = 0; i < 5 && npick < 4; i++)
    if (codes[i] != ans) pick[npick++] = codes[i];
  for (int i = 3; i > 0; i--) {
    int j = rng.range(0, i), t = pick[i];
    pick[i] = pick[j];
    pick[j] = t;
  }
  ex.nopt = 4;
  for (int i = 0; i < 4; i++) {
    int cd = pick[i];
    ex.value[i] = exactInt(cd + 1);  // 1..5, so that a code is never zero
    bad |= PR(ex.opt[i], "%s", TX(CLASS_LABEL[cd]));
    ex.why[i][0] = 0;
    if (cd == ans) {
      ex.correct = i;
      continue;
    }
    if (ans == 0)
      bad |= PR(ex.why[i], "%s", TX(CLASS_NOT_ENOUGH[cd]));
    else if (cd == 0)
      bad |= PR(ex.why[i], TX(TX_CLS_WHY2));
    else if (cd < ans && !(cd == 2 && ans == 3) && !(cd == 3 && ans == 2))  // less than the properties give
      bad |= PR(ex.why[i], TX(TX_CLS_WHY3));
    else if (cd == 4 && ans == 2)
      bad |= PR(ex.why[i], TX(TX_CLS_WHY4));
    else if (cd == 4 && ans == 3)
      bad |= PR(ex.why[i], TX(TX_CLS_WHY5));
    else if (cd == 4 && ans == 1)
      bad |= PR(ex.why[i], TX(TX_CLS_WHY6));
    else if (cd == 2 && ans == 1)
      bad |= PR(ex.why[i], TX(TX_CLS_WHY7));
    else if (cd == 3 && ans == 1)
      bad |= PR(ex.why[i], TX(TX_CLS_WHY8));
    else if (cd == 2 && ans == 3)
      bad |= PR(ex.why[i], TX(TX_CLS_WHY9));
    else if (cd == 3 && ans == 2)
      bad |= PR(ex.why[i], TX(TX_CLS_WHY10));
    else
      bad |= PR(ex.why[i], TX(TX_CLS_WHY11));
  }
  bad |= PR(ex.clue[0], TX(TX_CLS_HINT1));
  bad |= PR(ex.clue[1], TX(TX_CLS_HINT2));
  // every case of answer 0, the two with a pair of parallel sides included: some trapezoid has them all
  static const uint16_t SOL[5] = {TX_CLS_SOL_QUAD, TX_CLS_SOL_PAR, TX_CLS_SOL_RECT, TX_CLS_SOL_RHOMBUS, TX_CLS_SOL_SQUARE};
  bad |= PR(ex.solution, "%s", TX(SOL[ans]));
  return !bad;
}

// ---------------------------------------------------------------- Lesson 3: midline and centroid

static bool genMidline(Rng &rng, Exercise &ex) {
  int mode = rng.range(0, 1);
  bool bad = false;
  ex.kind = K_MIDLINE;
  ex.level = 1;
  Cand *c = ex.slip;
  int nc = 0;
  int ans;
  if (mode == 0) {
    int bc = evenIn(rng, 6, 40);
    ans = bc / 2;
    ex.fig = Figure{FIG_MIDLINE, {bc, 0, 0, 0}};
    bad |= PR(ex.text, TX(TX_MID_Q1), bc);
    SLIP(c, nc, exactInt(bc), TX(TX_MID_SLIP1));
    SLIP(c, nc, exactInt(2 * bc), TX(TX_MID_SLIP2));
    SLIP(c, nc, exactFrac(bc, 4), TX(TX_MID_SLIP3));
    SLIP(c, nc, exactFrac(bc, 3), TX(TX_MID_SLIP4));
    bad |= PR(ex.solution, TX(TX_MID_SOL1), bc, ans);
  } else {
    int mn = rng.range(3, 20);
    ans = 2 * mn;
    ex.fig = Figure{FIG_MIDLINE, {mn, 1, 0, 0}};
    bad |= PR(ex.text, TX(TX_MID_Q2), mn);
    SLIP(c, nc, exactInt(mn), TX(TX_MID_SLIP5));
    SLIP(c, nc, exactFrac(mn, 2), TX(TX_MID_SLIP6));
    SLIP(c, nc, exactInt(3 * mn), TX(TX_MID_SLIP7));
    SLIP(c, nc, exactInt(4 * mn), TX(TX_MID_SLIP8));
    bad |= PR(ex.solution, TX(TX_MID_SOL2), mn, ans);
  }
  bad |= PR(ex.clue[0], TX(TX_MID_HINT1));
  bad |= PR(ex.clue[1], TX(TX_MID_HINT2));
  return finish(ex, rng, bad, exactInt(ans), c, nc, " cm");
}

static bool genMedianTriangle(Rng &rng, Exercise &ex) {
  int a = rng.range(5, 20), b = rng.range(5, 20), cc = rng.range(5, 20);
  if ((a + b + cc) % 2 || a + b <= cc || a + cc <= b || b + cc <= a || (a == b && b == cc)) return false;
  int P = a + b + cc, ans = P / 2;
  bool bad = false;
  ex.kind = K_MEDIAN_TRIANGLE;
  ex.level = 2;
  ex.fig = Figure{FIG_MIDLINE, {a, b, cc, 0}};
  bad |= PR(ex.text, TX(TX_MEDT_Q), a, b, cc);
  Cand *c = ex.slip;
  int nc = 0;
  SLIP(c, nc, exactInt(P), TX(TX_MEDT_SLIP1));
  SLIP(c, nc, exactFrac(P, 4), TX(TX_MEDT_SLIP2));
  SLIP(c, nc, exactFrac(P, 3), TX(TX_MEDT_SLIP3));
  SLIP(c, nc, exactFrac(a, 2), TX(TX_MEDT_SLIP4));
  SLIP(c, nc, exactInt(2 * P), TX(TX_MEDT_SLIP5));
  bad |= PR(ex.clue[0], TX(TX_MEDT_HINT1));
  bad |= PR(ex.clue[1], TX(TX_MEDT_HINT2));
  bad |= PR(ex.solution, TX(TX_MEDT_SOL), a, b, cc, P, ans);
  return finish(ex, rng, bad, exactInt(ans), c, nc, " cm");
}

static bool genCentroid(Rng &rng, Exercise &ex) {
  int mode = rng.range(0, 3);
  bool bad = false;
  ex.kind = K_CENTROID;
  ex.level = 2;
  Cand *c = ex.slip;
  int nc = 0;
  int ans, given;
  if (mode <= 1) {
    given = 3 * rng.range(2, 10);
    ans = mode == 0 ? 2 * given / 3 : given / 3;
    ex.fig = Figure{FIG_MEDIANS, {given, mode, 0, 0}};
    bad |= PR(ex.text, TX(TX_CEN_Q1), given, mode == 0 ? "GA" : "GM");
    if (mode == 0) {
      SLIP(c, nc, exactInt(given / 3), TX(TX_CEN_SLIP1));
      SLIP(c, nc, exactFrac(given, 2), TX(TX_CEN_SLIP2));
      SLIP(c, nc, exactInt(given), TX(TX_CEN_SLIP3));
      SLIP(c, nc, exactInt(given / 3 * 4), TX(TX_CEN_SLIP4));
      bad |= PR(ex.solution, TX(TX_CEN_SOL1), given, ans);
    } else {
      SLIP(c, nc, exactInt(2 * given / 3), TX(TX_CEN_SLIP5));
      SLIP(c, nc, exactFrac(given, 2), TX(TX_CEN_SLIP2));
      SLIP(c, nc, exactInt(given), TX(TX_CEN_SLIP6));
      SLIP(c, nc, exactInt(given / 3 + 2), TX(TX_CEN_SLIP7));
      bad |= PR(ex.solution, TX(TX_CEN_SOL2), given, ans);
    }
  } else {
    given = rng.range(2, 10);
    ans = mode == 2 ? 3 * given : 2 * given;
    ex.fig = Figure{FIG_MEDIANS, {given, mode, 0, 0}};
    bad |= PR(ex.text, TX(TX_CEN_Q2), given, mode == 2 ? "AM" : "GA");
    if (mode == 2) {
      SLIP(c, nc, exactInt(2 * given), TX(TX_CEN_SLIP8));
      SLIP(c, nc, exactInt(given), TX(TX_CEN_SLIP9));
      SLIP(c, nc, exactInt(4 * given), TX(TX_CEN_SLIP10));
      SLIP(c, nc, exactInt(given + 2), TX(TX_CEN_SLIP11));
      bad |= PR(ex.solution, TX(TX_CEN_SOL3), given, ans);
    } else {
      SLIP(c, nc, exactInt(3 * given), TX(TX_CEN_SLIP12));
      SLIP(c, nc, exactInt(given), TX(TX_CEN_SLIP13));
      SLIP(c, nc, exactFrac(given, 2), TX(TX_CEN_SLIP14));
      SLIP(c, nc, exactInt(given + 2), TX(TX_CEN_SLIP15));
      bad |= PR(ex.solution, TX(TX_CEN_SOL4), given, ans);
    }
  }
  bad |= PR(ex.clue[0], TX(TX_CEN_HINT1));
  bad |= PR(ex.clue[1], TX(TX_CEN_HINT2));
  return finish(ex, rng, bad, exactInt(ans), c, nc, " cm");
}

// ---------------------------------------------------------------- Lesson 4: the rectangle

static bool genRectDiagonal(Rng &rng, Exercise &ex) {
  int mode = rng.range(0, 2);
  bool bad = false;
  ex.kind = K_RECT_DIAGONAL;
  ex.level = 1;
  Cand *c = ex.slip;
  int nc = 0;
  int ans, x;
  if (mode == 0) {
    x = evenIn(rng, 6, 30);
    ans = x;
    ex.fig = Figure{FIG_RECTANGLE, {x, 0, 0, 0}};
    bad |= PR(ex.text, TX(TX_RDIAG_Q1), x);
    SLIP(c, nc, exactInt(x / 2), TX(TX_RDIAG_SLIP1));
    SLIP(c, nc, exactInt(2 * x), TX(TX_RDIAG_SLIP2));
    SLIP(c, nc, exactInt(x / 2 * 3), TX(TX_RDIAG_SLIP3));
    SLIP(c, nc, exactInt(x / 4), TX(TX_RDIAG_SLIP4));
    bad |= PR(ex.solution, TX(TX_RDIAG_SOL1), x);
  } else if (mode == 1) {
    x = rng.range(3, 15);
    ans = 2 * x;
    ex.fig = Figure{FIG_RECTANGLE, {x, 1, 0, 0}};
    bad |= PR(ex.text, TX(TX_RDIAG_Q2), x);
    SLIP(c, nc, exactInt(x), TX(TX_RDIAG_SLIP5));
    SLIP(c, nc, exactInt(4 * x), TX(TX_RDIAG_SLIP6));
    SLIP(c, nc, exactInt(3 * x), TX(TX_RDIAG_SLIP7));
    SLIP(c, nc, exactFrac(x, 2), TX(TX_RDIAG_SLIP8));
    bad |= PR(ex.solution, TX(TX_RDIAG_SOL2), x, ans, ans);
  } else {
    x = rng.range(3, 15);
    ans = 4 * x;
    ex.fig = Figure{FIG_RECTANGLE, {x, 2, 0, 0}};
    bad |= PR(ex.text, TX(TX_RDIAG_Q3), x);
    SLIP(c, nc, exactInt(2 * x), TX(TX_RDIAG_SLIP9));
    SLIP(c, nc, exactInt(x), TX(TX_RDIAG_SLIP10));
    SLIP(c, nc, exactInt(8 * x), TX(TX_RDIAG_SLIP11));
    SLIP(c, nc, exactInt(3 * x), TX(TX_RDIAG_SLIP7));
    bad |= PR(ex.solution, TX(TX_RDIAG_SOL3), 2 * x, 2 * x, 2 * x, ans);
  }
  bad |= PR(ex.clue[0], TX(TX_RDIAG_HINT1));
  bad |= PR(ex.clue[1], TX(TX_RDIAG_HINT2));
  return finish(ex, rng, bad, exactInt(ans), c, nc, " cm");
}

static bool genRectAngle(Rng &rng, Exercise &ex) {
  int al = evenIn(rng, 20, 80);
  int ans = (180 - al) / 2;
  bool bad = false;
  ex.kind = K_RECT_ANGLE;
  ex.level = 2;
  ex.fig = Figure{FIG_RECTANGLE, {al, 0, 0, 0}};
  bad |= PR(ex.text, TX(TX_RANG_Q), al);
  Cand *c = ex.slip;
  int nc = 0;
  SLIP(c, nc, exactInt(90 - al), TX(TX_RANG_SLIP1));
  SLIP(c, nc, exactInt(al / 2), TX(TX_RANG_SLIP2));
  SLIP(c, nc, exactInt(180 - al), TX(TX_RANG_SLIP3));
  SLIP(c, nc, exactInt(al), TX(TX_RANG_SLIP4));
  SLIP(c, nc, exactInt((180 - al) / 2 + 10), TX(TX_RANG_SLIP5));
  bad |= PR(ex.clue[0], TX(TX_RANG_HINT1));
  bad |= PR(ex.clue[1], TX(TX_RANG_HINT2));
  bad |= PR(ex.solution, TX(TX_RANG_SOL), al,
            180 - al, ans);
  return finish(ex, rng, bad, exactInt(ans), c, nc, ANG);
}

// ---------------------------------------------------------------- Lesson 5: the rhombus and the square

static bool genRhombusAngle(Rng &rng, Exercise &ex) {
  int x = rng.range(15, 44);
  if (2 * x == 90) return false;
  int mode = rng.range(0, 1);  // 0 the angle BAD, 1 the angle ABC
  int ans = mode == 0 ? 2 * x : 180 - 2 * x;
  bool bad = false;
  ex.kind = K_RHOMBUS_ANGLE;
  ex.level = 2;
  ex.fig = Figure{FIG_RHOMBUS, {x, mode, 0, 0}};
  bad |= PR(ex.text, TX(TX_RHA_Q), x, mode == 0 ? "BAD" : "ABC");
  Cand *c = ex.slip;
  int nc = 0;
  if (mode == 0) {
    SLIP(c, nc, exactInt(x), TX(TX_RHA_SLIP1));
    SLIP(c, nc, exactInt(90 - x), TX(TX_RHA_SLIP2));
    SLIP(c, nc, exactInt(180 - x), TX(TX_RHA_SLIP3));
    SLIP(c, nc, exactInt(180 - 2 * x), TX(TX_RHA_SLIP4));
    bad |= PR(ex.solution, TX(TX_RHA_SOL1), x, ans);
  } else {
    SLIP(c, nc, exactInt(2 * x), TX(TX_RHA_SLIP5));
    SLIP(c, nc, exactInt(x), TX(TX_RHA_SLIP6));
    SLIP(c, nc, exactInt(90 - x), TX(TX_RHA_SLIP7));
    SLIP(c, nc, exactInt(180 - x), TX(TX_RHA_SLIP8));
    bad |= PR(ex.solution, TX(TX_RHA_SOL2), x, 2 * x,
              2 * x, ans);
  }
  bad |= PR(ex.clue[0], TX(TX_RHA_HINT1));
  bad |= PR(ex.clue[1], TX(TX_RHA_HINT2));
  return finish(ex, rng, bad, exactInt(ans), c, nc, ANG);
}

static bool genRhombusSide(int level, Rng &rng, Exercise &ex) {
  int a, b;  // half of each diagonal
  if (level <= 1) {
    static const int T[][3] = {{3, 4, 5}, {5, 12, 13}, {8, 15, 17}, {6, 8, 10}, {9, 12, 15}, {12, 16, 20}, {7, 24, 25}, {20, 21, 29}};
    const int *t = T[rng.range(0, (int)(sizeof T / sizeof T[0]) - 1)];
    a = t[0], b = t[1];
    if (rng.range(0, 1)) {
      int tmp = a;
      a = b;
      b = tmp;
    }
  } else {
    a = rng.range(2, 9), b = rng.range(2, 9);
    if (a == b) return false;
  }
  int64_t n = sq(a) + sq(b);
  Exact side = exactSqrt(n);
  if (level <= 1 ? side.r != 1 : side.r == 1) return false;
  int mode = rng.range(0, 1);  // 0 the side, 1 the perimeter
  int d1 = 2 * a, d2 = 2 * b;
  Exact right = mode == 0 ? side : exactNorm(4LL * side.p, side.r, side.q);
  bool bad = false;
  ex.kind = K_RHOMBUS_SIDE;
  ex.level = level <= 1 ? 1 : 2;
  ex.fig = Figure{FIG_RHOMBUS, {d1, d2, mode, 0}};
  bad |= PR(ex.text, mode == 0 ? TX(TX_RHS_Q_SIDE) : TX(TX_RHS_Q_PERIMETER), d1, d2);
  Cand *c = ex.slip;
  int nc = 0;
  char f[32];
  if (mode == 0) {
    SLIP(c, nc, exactInt(a + b), TX(TX_RHS_SLIP1));
    SLIP(c, nc, exactSqrt(sq(d1) + sq(d2)), TX(TX_RHS_SLIP2));
    SLIP(c, nc, exactInt(d1 * d2 / 2), TX(TX_RHS_SLIP3));
    SLIP(c, nc, exactInt(a > b ? a : b), TX(TX_RHS_SLIP4));
    SLIP(c, nc, exactInt((int32_t)n), TX(TX_RHS_SLIP5));
  } else {
    SLIP(c, nc, side, TX(TX_RHS_SLIP6));
    SLIP(c, nc, exactNorm(2LL * side.p, side.r, side.q), TX(TX_RHS_SLIP7));
    SLIP(c, nc, exactInt(2 * (a + b)), TX(TX_RHS_SLIP8));
    SLIP(c, nc, exactNorm(4LL * exactSqrt(sq(d1) + sq(d2)).p, exactSqrt(sq(d1) + sq(d2)).r, 1), TX(TX_RHS_SLIP9));
    SLIP(c, nc, exactInt(d1 * d2 / 2), TX(TX_RHS_SLIP10));
  }
  bad |= PR(ex.clue[0], TX(TX_RHS_HINT1));
  bad |= PR(ex.clue[1], mode == 0 ? TX(TX_RHS_HINT2_SIDE) : TX(TX_RHS_HINT2_PERIMETER));
  exactFormat(side, f, sizeof f);
  if (mode == 0)
    bad |= PR(ex.solution, TX(TX_RHS_SOL1), a, b, a, b,
              (int)n, f);
  else {
    char g[32];
    exactFormat(right, g, sizeof g);
    bad |= PR(ex.solution, TX(TX_RHS_SOL2), a, b, a, b,
              (int)n, f, f, g);
  }
  return finish(ex, rng, bad, right, c, nc, " cm");
}

static bool genSquareSide(Rng &rng, Exercise &ex) {
  int d = rng.range(2, 20);
  Exact right = exactNorm(d, 2, 2);  // d / sqrt(2) = d sqrt(2) / 2
  bool bad = false;
  ex.kind = K_SQUARE_SIDE;
  ex.level = 3;
  ex.fig = Figure{FIG_SQUARE_DIAGONAL, {d, 1, 0, 0}};
  bad |= PR(ex.text, TX(TX_SQS_Q), d);
  Cand *c = ex.slip;
  int nc = 0;
  SLIP(c, nc, exactInt(d), TX(TX_SQS_SLIP1));
  SLIP(c, nc, exactFrac(d, 2), TX(TX_SQS_SLIP2));
  SLIP(c, nc, exactNorm(d, 2, 1), TX(TX_SQS_SLIP3), d);
  SLIP(c, nc, exactFrac((int32_t)(d * d), 2), TX(TX_SQS_SLIP4));
  SLIP(c, nc, exactNorm(d, 3, 2), TX(TX_SQS_SLIP5));
  char f[32];
  exactFormat(right, f, sizeof f);
  bad |= PR(ex.clue[0], TX(TX_SQS_HINT1));
  bad |= PR(ex.clue[1], TX(TX_SQS_HINT2));
  bad |= PR(ex.solution, TX(TX_SQS_SOL), d, d * d, d, f);
  return finish(ex, rng, bad, right, c, nc, " cm");
}

// ---------------------------------------------------------------- Lesson 6: the trapezoid

static bool genTrapAngle(Rng &rng, Exercise &ex) {
  int mode = rng.range(0, 3);
  bool iso = mode >= 2;
  int x = iso ? mult5(rng, 50, 85) : (rng.range(0, 1) ? mult5(rng, 50, 85) : mult5(rng, 95, 130));
  int ans = mode == 3 ? x : 180 - x;
  const char *given[4] = {"A", "B", "A", "A"}, *ask[4] = {"D", "C", "C", "B"};
  bool bad = false;
  ex.kind = K_TRAP_ANGLE;
  ex.level = 1;
  ex.fig = Figure{iso ? FIG_TRAPEZOID_ISOSCELES : FIG_TRAPEZOID, {x, mode, 0, 0}};
  bad |= PR(ex.text, iso ? TX(TX_TANG_Q_ISO) : TX(TX_TANG_Q), given[mode], x, ask[mode]);
  Cand *c = ex.slip;
  int nc = 0;
  if (mode <= 1) {
    SLIP(c, nc, exactInt(x), TX(TX_TANG_SLIP1), given[mode], ask[mode]);
    SLIP(c, nc, exactInt(abs(90 - x)), TX(TX_TANG_SLIP2));
    SLIP(c, nc, exactInt(360 - x), TX(TX_TANG_SLIP3));
    SLIP(c, nc, exactFrac(180 - x, 2), TX(TX_TANG_SLIP4));
    bad |= PR(ex.clue[0], TX(TX_TANG_HINT1_1));
    bad |= PR(ex.clue[1], TX(TX_TANG_HINT2_1));
    bad |= PR(ex.solution, TX(TX_TANG_SOL1), mode == 0 ? "AD" : "BC", ask[mode], x, ans);
  } else if (mode == 2) {
    SLIP(c, nc, exactInt(x), TX(TX_TANG_SLIP5));
    SLIP(c, nc, exactInt(abs(90 - x)), TX(TX_TANG_SLIP6));
    SLIP(c, nc, exactInt(360 - x), TX(TX_TANG_SLIP7));
    SLIP(c, nc, exactInt(360 - 2 * x), TX(TX_TANG_SLIP8));
    bad |= PR(ex.clue[0], TX(TX_TANG_HINT1_2));
    bad |= PR(ex.clue[1], TX(TX_TANG_HINT2_2));
    bad |= PR(ex.solution, TX(TX_TANG_SOL2), x, ans, ans);
  } else {
    SLIP(c, nc, exactInt(180 - x), TX(TX_TANG_SLIP9));
    SLIP(c, nc, exactInt(abs(90 - x)), TX(TX_TANG_SLIP6));
    SLIP(c, nc, exactInt(360 - x), TX(TX_TANG_SLIP10));
    SLIP(c, nc, exactInt(360 - 2 * x), TX(TX_TANG_SLIP11));
    bad |= PR(ex.clue[0], TX(TX_TANG_HINT1_3));
    bad |= PR(ex.clue[1], TX(TX_TANG_HINT2_3));
    bad |= PR(ex.solution, TX(TX_TANG_SOL3), x);
  }
  return finish(ex, rng, bad, exactInt(ans), c, nc, ANG);
}

static bool genTrapMidline(Rng &rng, Exercise &ex) {
  int mode = rng.range(0, 2);
  int B = rng.range(8, 26), b = rng.range(3, B - 2);
  if ((B + b) % 2 || B <= b) return false;
  bool bad = false;
  ex.kind = K_TRAP_MIDLINE;
  ex.level = 2;
  Cand *c = ex.slip;
  int nc = 0;
  int ans;
  if (mode == 0) {
    ans = (B + b) / 2;
    ex.fig = Figure{FIG_TRAPEZOID, {B, b, 0, 0}};
    bad |= PR(ex.text, TX(TX_TMID_Q1), B, b);
    SLIP(c, nc, exactInt(B + b), TX(TX_TMID_SLIP1));
    SLIP(c, nc, exactInt((B - b) / 2), TX(TX_TMID_SLIP2));
    SLIP(c, nc, exactInt(B - b), TX(TX_TMID_SLIP3));
    SLIP(c, nc, exactFrac(B + b, 4), TX(TX_TMID_SLIP4));
    SLIP(c, nc, exactFrac(B, 2), TX(TX_TMID_SLIP5));
    bad |= PR(ex.solution, TX(TX_TMID_SOL1), B, b, B + b, ans);
  } else if (mode == 1) {
    int m = (B + b) / 2;
    ans = B;
    ex.fig = Figure{FIG_TRAPEZOID, {m, b, 1, 0}};
    bad |= PR(ex.text, TX(TX_TMID_Q2), m, b);
    SLIP(c, nc, exactInt(m + b), TX(TX_TMID_SLIP6));
    SLIP(c, nc, exactInt(2 * m), TX(TX_TMID_SLIP7));
    SLIP(c, nc, exactInt(m - b), TX(TX_TMID_SLIP8));
    SLIP(c, nc, exactInt(2 * m + b), TX(TX_TMID_SLIP9));
    bad |= PR(ex.solution, TX(TX_TMID_SOL2), m, 2 * m, 2 * m, b, ans);
  } else {
    ans = (B - b) / 2;
    ex.fig = Figure{FIG_TRAPEZOID, {B, b, 2, 0}};
    bad |= PR(ex.text, TX(TX_TMID_Q3), B, b);
    SLIP(c, nc, exactInt((B + b) / 2), TX(TX_TMID_SLIP10));
    SLIP(c, nc, exactInt(B - b), TX(TX_TMID_SLIP11));
    SLIP(c, nc, exactFrac(b, 2), TX(TX_TMID_SLIP12));
    SLIP(c, nc, exactFrac(B, 2), TX(TX_TMID_SLIP13));
    bad |= PR(ex.solution, TX(TX_TMID_SOL3), B, b, B - b, ans);
  }
  bad |= PR(ex.clue[0], mode == 2 ? TX(TX_TMID_HINT1_PQ) : TX(TX_TMID_HINT1_MN));
  bad |= PR(ex.clue[1], mode == 2 ? TX(TX_TMID_HINT2_PQ) : TX(TX_TMID_HINT2_MN));
  return finish(ex, rng, bad, exactInt(ans), c, nc, " cm");
}

static bool genTrapHeight(int level, Rng &rng, Exercise &ex) {
  int p, c0, b;  // projection of the leg, the leg, the small base
  if (level <= 1) {
    static const int T[][3] = {{3, 4, 5}, {5, 12, 13}, {8, 15, 17}, {6, 8, 10}, {9, 12, 15}, {7, 24, 25}, {12, 16, 20}};
    const int *t = T[rng.range(0, (int)(sizeof T / sizeof T[0]) - 1)];
    p = t[rng.range(0, 1)];
    c0 = t[2];
    b = rng.range(3, 10);
  } else {
    c0 = rng.range(4, 13);
    p = rng.range(1, c0 - 1);
    b = rng.range(3, 10);
  }
  int64_t n = sq(c0) - sq(p);
  Exact right = exactSqrt(n);
  if (n <= 0 || (level <= 1 ? right.r != 1 : right.r == 1) || right.p == p) return false;
  int B = b + 2 * p;
  bool bad = false;
  ex.kind = K_TRAP_HEIGHT;
  ex.level = level <= 1 ? 1 : 2;
  ex.fig = Figure{FIG_TRAPEZOID_ISOSCELES, {B, b, c0, 0}};
  bad |= PR(ex.text, TX(TX_THGT_Q), B, b, c0);
  Cand *c = ex.slip;
  int nc = 0;
  SLIP(c, nc, exactInt(c0), TX(TX_THGT_SLIP1));
  SLIP(c, nc, exactSqrt(sq(c0) + sq(p)), TX(TX_THGT_SLIP2));
  SLIP(c, nc, exactInt(c0 - p), TX(TX_THGT_SLIP3));
  SLIP(c, nc, exactInt(p), TX(TX_THGT_SLIP4));
  SLIP(c, nc, exactInt((int32_t)n), TX(TX_THGT_SLIP5));
  if (sq(c0) > sq(2 * p)) SLIP(c, nc, exactSqrt(sq(c0) - sq(2 * p)), TX(TX_THGT_SLIP6));
  char f[32];
  exactFormat(right, f, sizeof f);
  bad |= PR(ex.clue[0], TX(TX_THGT_HINT1));
  bad |= PR(ex.clue[1], TX(TX_THGT_HINT2));
  bad |= PR(ex.solution, TX(TX_THGT_SOL), B, b, p, c0, p, (int)n, f);
  return finish(ex, rng, bad, right, c, nc, " cm");
}

// ---------------------------------------------------------------- Lesson 7: perimeters and areas

static bool genArea(Rng &rng, Exercise &ex) {
  int type = rng.range(0, 4);
  bool bad = false;
  ex.kind = K_AREA;
  ex.level = 2;
  Cand *c = ex.slip;
  int nc = 0;
  int ans;
  if (type == 0) {  // parallelogram: base, side, height
    int a = rng.range(6, 14), h = rng.range(3, 9), s = h + rng.range(1, 6);
    ans = a * h;
    ex.fig = Figure{FIG_PARALLELOGRAM, {0, a, h, s}};
    bad |= PR(ex.text, TX(TX_AREA_Q1), a, s, h);
    SLIP(c, nc, exactInt(a * s), TX(TX_AREA_SLIP1));
    SLIP(c, nc, exactFrac(a * h, 2), TX(TX_AREA_SLIP2));
    SLIP(c, nc, exactInt(2 * (a + s)), TX(TX_AREA_SLIP3));
    SLIP(c, nc, exactInt(a + h), TX(TX_AREA_SLIP4));
    bad |= PR(ex.clue[0], TX(TX_AREA_HINT1_1));
    bad |= PR(ex.clue[1], TX(TX_AREA_HINT2_1));
    bad |= PR(ex.solution, TX(TX_AREA_SOL1), a, h, ans);
  } else if (type == 1) {  // triangle
    int b = rng.range(6, 16), h = rng.range(4, 12);
    if ((b * h) % 2) return false;
    ans = b * h / 2;
    ex.fig = Figure{FIG_NONE, {1, b, h, 0}};
    bad |= PR(ex.text, TX(TX_AREA_Q2), b, h);
    SLIP(c, nc, exactInt(b * h), TX(TX_AREA_SLIP5));
    SLIP(c, nc, exactInt(b + h), TX(TX_AREA_SLIP6));
    SLIP(c, nc, exactFrac(b * h, 4), TX(TX_AREA_SLIP7));
    SLIP(c, nc, exactFrac(b + h, 2), TX(TX_AREA_SLIP8));
    bad |= PR(ex.clue[0], TX(TX_AREA_HINT1_2));
    bad |= PR(ex.clue[1], TX(TX_AREA_HINT2_2));
    bad |= PR(ex.solution, TX(TX_AREA_SOL2), b, h, b * h, ans);
  } else if (type == 2) {  // rhombus from the diagonals
    int d1 = rng.range(4, 16), d2 = rng.range(4, 16);
    if ((d1 * d2) % 2 || d1 == d2) return false;
    ans = d1 * d2 / 2;
    ex.fig = Figure{FIG_RHOMBUS, {2, d1, d2, 0}};
    bad |= PR(ex.text, TX(TX_AREA_Q3), d1, d2);
    SLIP(c, nc, exactInt(d1 * d2), TX(TX_AREA_SLIP9));
    SLIP(c, nc, exactInt(d1 + d2), TX(TX_AREA_SLIP10));
    SLIP(c, nc, exactFrac(d1 * d2, 4), TX(TX_AREA_SLIP11));
    SLIP(c, nc, exactFrac(d1 + d2, 2), TX(TX_AREA_SLIP12));
    bad |= PR(ex.clue[0], TX(TX_AREA_HINT1_3));
    bad |= PR(ex.clue[1], TX(TX_AREA_HINT2_3));
    bad |= PR(ex.solution, TX(TX_AREA_SOL3), d1, d2, d1 * d2, ans);
  } else if (type == 3) {  // trapezoid
    int B = rng.range(8, 20), b = rng.range(3, B - 2), h = rng.range(3, 10);
    if (((B + b) * h) % 2 || B <= b) return false;
    ans = (B + b) * h / 2;
    ex.fig = Figure{FIG_TRAPEZOID, {3, B, b, h}};
    bad |= PR(ex.text, TX(TX_AREA_Q4), B, b, h);
    SLIP(c, nc, exactInt((B + b) * h), TX(TX_AREA_SLIP13));
    SLIP(c, nc, exactInt(B * h), TX(TX_AREA_SLIP14));
    SLIP(c, nc, exactFrac((B - b) * h, 2), TX(TX_AREA_SLIP15));
    SLIP(c, nc, exactFrac(B + b, 2), TX(TX_AREA_SLIP16));
    bad |= PR(ex.clue[0], TX(TX_AREA_HINT1_4));
    bad |= PR(ex.clue[1], TX(TX_AREA_HINT2_4));
    bad |= PR(ex.solution, TX(TX_AREA_SOL4), B, b, h, (B + b) * h, ans);
  } else {  // right triangle from the legs (Pythagorean, so the hypotenuse is a whole number too)
    static const int T[][3] = {{3, 4, 5}, {6, 8, 10}, {5, 12, 13}, {9, 12, 15}, {8, 15, 17}, {12, 16, 20}, {7, 24, 25}, {10, 24, 26}};
    const int *t = T[rng.range(0, (int)(sizeof T / sizeof T[0]) - 1)];
    int c1 = t[0], c2 = t[1], hy = t[2];
    if (rng.range(0, 1)) {
      int tmp = c1;
      c1 = c2;
      c2 = tmp;
    }
    ans = c1 * c2 / 2;
    ex.fig = Figure{FIG_RIGHT_TRIANGLE, {c1, c2, hy, 4}};  // v[3] 4: the area is asked (t7FigRight marks it)
    bad |= PR(ex.text, TX(TX_AREA_Q5), c1, c2, hy);
    SLIP(c, nc, exactInt(c1 * c2), TX(TX_AREA_SLIP17));
    SLIP(c, nc, exactInt(c1 + c2), TX(TX_AREA_SLIP18));
    SLIP(c, nc, exactFrac(hy * c1, 2), TX(TX_AREA_SLIP19));
    SLIP(c, nc, exactInt(c1 + c2 + hy), TX(TX_AREA_SLIP20));
    bad |= PR(ex.clue[0], TX(TX_AREA_HINT1_5));
    bad |= PR(ex.clue[1], TX(TX_AREA_HINT2_5));
    bad |= PR(ex.solution, TX(TX_AREA_SOL5), c1, c2, c1 * c2, ans);
  }
  return finish(ex, rng, bad, exactInt(ans), c, nc, CM2);
}

static bool genPerimeter(Rng &rng, Exercise &ex) {
  int type = rng.range(0, 4);
  bool bad = false;
  ex.kind = K_PERIMETER;
  ex.level = 1;
  Cand *c = ex.slip;
  int nc = 0;
  int ans;
  if (type == 0 || type == 3) {
    int a = rng.range(3, 18), b = rng.range(3, 18);
    if (a == b) return false;
    ans = 2 * (a + b);
    ex.fig = Figure{type == 0 ? FIG_RECTANGLE : FIG_PARALLELOGRAM, {type, a, b, 0}};
    if (type == 0) bad |= PR(ex.text, TX(TX_PER_Q1), a, b);
    else bad |= PR(ex.text, TX(TX_PER_Q2), a, b);
    SLIP(c, nc, exactInt(a + b), TX(TX_PER_SLIP1));
    SLIP(c, nc, exactInt(a * b), TX(TX_PER_SLIP2));
    SLIP(c, nc, exactInt(2 * a + b), TX(TX_PER_SLIP3));
    SLIP(c, nc, exactInt(a + 2 * b), TX(TX_PER_SLIP3));
    bad |= PR(ex.clue[0], TX(TX_PER_HINT1_1));
    bad |= PR(ex.clue[1], TX(TX_PER_HINT2_1));
    bad |= PR(ex.solution, TX(TX_PER_SOL1), a, b, a + b, ans);
  } else if (type == 1 || type == 2) {
    int l = rng.range(3, 20);
    ans = 4 * l;
    ex.fig = Figure{type == 1 ? FIG_NONE : FIG_RHOMBUS, {type, l, 0, 0}};
    bad |= PR(ex.text, type == 1 ? TX(TX_PER_Q_SQUARE) : TX(TX_PER_Q_RHOMBUS), l);
    SLIP(c, nc, exactInt(l * l), TX(TX_PER_SLIP4));
    SLIP(c, nc, exactInt(2 * l), TX(TX_PER_SLIP5));
    SLIP(c, nc, exactInt(3 * l), TX(TX_PER_SLIP6));
    SLIP(c, nc, exactInt(8 * l), TX(TX_PER_SLIP7));
    bad |= PR(ex.clue[0], TX(TX_PER_HINT1_2));
    bad |= PR(ex.clue[1], TX(TX_PER_HINT2_2));
    bad |= PR(ex.solution, TX(TX_PER_SOL2), l, ans);
  } else {  // mixed units: dm to cm
    int a = rng.range(6, 20), y = 5 * rng.range(1, 4);
    ans = 2 * (a + y);
    char dm[16];
    if (y % 10) snprintf(dm, sizeof dm, TX(TX_DM_HALF), y / 10);  // "2,5" or "2.5": the language's decimal separator
    else snprintf(dm, sizeof dm, "%d", y / 10);
    ex.fig = Figure{FIG_RECTANGLE, {4, a, y, 0}};
    bad |= PR(ex.text, TX(TX_PER_Q4), a, dm);
    // the width's dm number taken as cm: 2 · (a + y/10) = 2a + y/5, a whole number since y is a multiple of 5
    SLIP(c, nc, exactInt(2 * a + y / 5), TX(TX_PER_SLIP8));
    SLIP(c, nc, exactInt(a + y), TX(TX_PER_SLIP1));
    SLIP(c, nc, exactInt(a * y), TX(TX_PER_SLIP9));
    SLIP(c, nc, exactInt(2 * a + y), TX(TX_PER_SLIP10));
    bad |= PR(ex.clue[0], TX(TX_PER_HINT1_3));
    bad |= PR(ex.clue[1], TX(TX_PER_HINT2_3));
    bad |= PR(ex.solution, TX(TX_PER_SOL3), dm, y, a, y, a + y, ans);
  }
  return finish(ex, rng, bad, exactInt(ans), c, nc, " cm");
}

static bool genHeightFromArea(int level, Rng &rng, Exercise &ex) {
  int type = level <= 1 ? rng.range(0, 1) : rng.range(0, 2);
  bool bad = false;
  ex.kind = K_HEIGHT_FROM_AREA;
  ex.level = level <= 1 ? 1 : 2;
  Cand *c = ex.slip;
  int nc = 0;
  Exact right;
  if (type == 0) {
    int a = rng.range(4, 12), h = rng.range(3, 9);
    int A = a * h;
    right = exactInt(h);
    ex.fig = Figure{FIG_NONE, {0, A, a, 0}};
    bad |= PR(ex.text, TX(TX_HFA_Q1), A, a);
    SLIP(c, nc, exactInt(A - a), TX(TX_HFA_SLIP1));
    SLIP(c, nc, exactInt(A * a), TX(TX_HFA_SLIP2));
    SLIP(c, nc, exactFrac(A, 2 * a), TX(TX_HFA_SLIP3));
    SLIP(c, nc, exactFrac(2 * A, a), TX(TX_HFA_SLIP4));
    bad |= PR(ex.clue[0], TX(TX_HFA_HINT1_1));
    bad |= PR(ex.clue[1], TX(TX_HFA_HINT2_1));
    bad |= PR(ex.solution, TX(TX_HFA_SOL1), A, a, h);
  } else if (type == 1) {
    int b = rng.range(4, 14), h = rng.range(3, 12);
    if ((b * h) % 2) return false;
    int A = b * h / 2;
    right = exactInt(h);
    ex.fig = Figure{FIG_NONE, {1, A, b, 0}};
    bad |= PR(ex.text, TX(TX_HFA_Q2), A, b);
    SLIP(c, nc, exactFrac(A, b), TX(TX_HFA_SLIP5));
    SLIP(c, nc, exactInt(A - b), TX(TX_HFA_SLIP6));
    SLIP(c, nc, exactInt(A * b), TX(TX_HFA_SLIP7));
    SLIP(c, nc, exactFrac(4 * A, b), TX(TX_HFA_SLIP8));
    bad |= PR(ex.clue[0], TX(TX_HFA_HINT1_2));
    bad |= PR(ex.clue[1], TX(TX_HFA_HINT2_2));
    bad |= PR(ex.solution, TX(TX_HFA_SOL2), A, b, h);
  } else {
    static const int T[][3] = {{3, 4, 5}, {5, 12, 13}, {6, 8, 10}, {8, 15, 17}, {9, 12, 15}, {7, 24, 25}, {12, 16, 20}};
    const int *t = T[rng.range(0, (int)(sizeof T / sizeof T[0]) - 1)];
    int c1 = t[0], c2 = t[1], hy = t[2];
    right = exactFrac(c1 * c2, hy);
    if (right.q == 1) return false;
    ex.fig = Figure{FIG_RIGHT_TRIANGLE, {c1, c2, hy, 2}};  // v[3] 2: the height from A is asked (t7FigRight draws it)
    bad |= PR(ex.text, TX(TX_HFA_Q3), c1, c2, hy);
    SLIP(c, nc, exactFrac(c1 * c2, 2), TX(TX_HFA_SLIP9));
    SLIP(c, nc, exactFrac(hy, 2), TX(TX_HFA_SLIP10));
    SLIP(c, nc, exactInt(c1 * c2), TX(TX_HFA_SLIP11));
    SLIP(c, nc, exactFrac(c1 * c2, c1 + c2), TX(TX_HFA_SLIP12));
    SLIP(c, nc, exactFrac(c1 + c2, 2), TX(TX_HFA_SLIP13));
    bad |= PR(ex.clue[0], TX(TX_HFA_HINT1_3));
    bad |= PR(ex.clue[1], TX(TX_HFA_HINT2_3));
    char f[32];
    exactFormat(right, f, sizeof f);
    bad |= PR(ex.solution, TX(TX_HFA_SOL3), c1, c2, hy, c1 * c2, hy, f);
  }
  return finish(ex, rng, bad, right, c, nc, " cm");
}

// ---------------------------------------------------------------- entry points

bool u4Generate(Kind kind, int level, Rng &rng, Exercise &ex) {
  switch (kind) {
    case K_Q_FOURTH_ANGLE: return genFourthAngle(rng, ex);
    case K_Q_PROPORTIONAL: return genProportional(rng, ex);
    case K_PAR_ANGLE: return genParAngle(rng, ex);
    case K_PAR_PERIMETER: return genParPerimeter(rng, ex);
    case K_PAR_DIAGONAL: return genParDiagonal(rng, ex);
    case K_CLASSIFY: return genClassify(rng, ex);
    case K_MIDLINE: return genMidline(rng, ex);
    case K_MEDIAN_TRIANGLE: return genMedianTriangle(rng, ex);
    case K_CENTROID: return genCentroid(rng, ex);
    case K_RECT_DIAGONAL: return genRectDiagonal(rng, ex);
    case K_RECT_ANGLE: return genRectAngle(rng, ex);
    case K_RHOMBUS_ANGLE: return genRhombusAngle(rng, ex);
    case K_RHOMBUS_SIDE: return genRhombusSide(level, rng, ex);
    case K_SQUARE_SIDE: return genSquareSide(rng, ex);
    case K_TRAP_ANGLE: return genTrapAngle(rng, ex);
    case K_TRAP_MIDLINE: return genTrapMidline(rng, ex);
    case K_TRAP_HEIGHT: return genTrapHeight(level, rng, ex);
    case K_AREA: return genArea(rng, ex);
    case K_PERIMETER: return genPerimeter(rng, ex);
    case K_HEIGHT_FROM_AREA: return genHeightFromArea(level, rng, ex);
    default: return false;
  }
}

// ---------------------------------------------------------------- the second derivation

static bool bad(char *err, int cap, const char *msg) {
  if (err && cap > 0) snprintf(err, cap, "%s", msg);
  return false;
}

bool u4Expect(const Exercise &ex, double &expect, const char *&unit, char *err, int cap) {
  const int32_t *v = ex.fig.v;
  const double PI = 3.14159265358979323846;
  unit = " cm";
  switch (ex.kind) {
    case K_Q_FOURTH_ANGLE: {
      unit = DEG;
      for (int i = 0; i < 3; i++)
        if (v[i] <= 0 || v[i] >= 180) return bad(err, cap, "angle out of range");
      expect = 360.0 - v[0] - v[1] - v[2];
      return expect > 0 && expect < 180 ? true : bad(err, cap, "the fourth angle must be between 0 and 180");
    }
    case K_Q_PROPORTIONAL: {
      unit = DEG;
      int S = 0, m = 0;
      for (int i = 0; i < 4; i++) {
        if (v[i] <= 0) return bad(err, cap, "ratio not positive");
        S += v[i];
        m = v[i] > m ? v[i] : m;
      }
      if ((360 * m) % S) return bad(err, cap, "angles are not whole degrees");
      expect = 360.0 * m / S;
      return expect < 180 ? true : bad(err, cap, "an angle of 180 or more is not convex");
    }
    case K_PAR_ANGLE: {
      unit = DEG;
      if (v[0] <= 0 || v[0] >= 180 || v[1] < 0 || v[1] > 2) return bad(err, cap, "parallelogram angle givens");
      expect = v[1] == 1 ? v[0] : 180.0 - v[0];  // opposite: equal; consecutive: supplementary
      return true;
    }
    case K_PAR_PERIMETER:
      if (v[2] == 0) {
        if (v[0] <= 0 || v[1] <= 0) return bad(err, cap, "side not positive");
        expect = 2.0 * (v[0] + v[1]);
      } else {
        if (v[0] <= 0 || v[0] % 2 || v[1] <= 0 || v[0] / 2 <= v[1]) return bad(err, cap, "perimeter and side do not fit");
        expect = v[0] / 2.0 - v[1];
      }
      return true;
    case K_PAR_DIAGONAL:
      if (v[0] <= 0 || v[1] <= 0 || v[2] < 0 || v[2] > 1) return bad(err, cap, "diagonal givens");
      expect = 2.0 * (v[2] == 0 ? v[0] : v[1]);
      return true;
    case K_MIDLINE:
      if (v[0] <= 0 || v[1] < 0 || v[1] > 1) return bad(err, cap, "midline givens");
      if (v[1] == 0) {
        if (v[0] % 2) return bad(err, cap, "BC must be even");
        expect = v[0] / 2.0;
      } else {
        expect = 2.0 * v[0];
      }
      return true;
    case K_MEDIAN_TRIANGLE: {
      int a = v[0], b = v[1], c = v[2];
      if (a <= 0 || b <= 0 || c <= 0 || a + b <= c || a + c <= b || b + c <= a) return bad(err, cap, "not a triangle");
      if ((a + b + c) % 2) return bad(err, cap, "the perimeter must be even");
      expect = (a + b + c) / 2.0;
      return true;
    }
    case K_CENTROID:
      if (v[0] <= 0 || v[1] < 0 || v[1] > 3) return bad(err, cap, "centroid givens");
      if (v[1] <= 1) {
        if (v[0] % 3) return bad(err, cap, "AM must be a multiple of three");
        expect = v[1] == 0 ? 2.0 * v[0] / 3 : v[0] / 3.0;
      } else {
        expect = v[1] == 2 ? 3.0 * v[0] : 2.0 * v[0];
      }
      return true;
    case K_RECT_DIAGONAL:
      if (v[0] <= 0 || v[1] < 0 || v[1] > 2) return bad(err, cap, "rectangle diagonal givens");
      expect = v[1] == 0 ? v[0] : v[1] == 1 ? 2.0 * v[0] : 4.0 * v[0];  // BD = AC; 2 AO; AC + BD = 4 AO
      return true;
    case K_RECT_ANGLE: {
      unit = DEG;
      if (v[0] <= 0 || v[0] >= 90) return bad(err, cap, "the acute angle between the diagonals");
      // a rectangle with sides w < h whose diagonals meet at alpha: tan(alpha / 2) = w / h; the angle at A with the short side is atan(h / w)
      double w = tan(v[0] * PI / 360.0), h = 1.0;
      expect = atan2(h, w) * 180.0 / PI;
      return true;
    }
    case K_RHOMBUS_ANGLE: {
      unit = DEG;
      if (v[0] <= 0 || v[0] >= 45 || v[1] < 0 || v[1] > 1) return bad(err, cap, "rhombus angle givens");
      // half diagonals p (along AC) and q (along BD): tan(x) = q / p; the angle at B is twice atan(p / q)
      double x = v[0] * PI / 180.0;
      double p = 1.0, q = tan(x);
      expect = v[1] == 0 ? 2.0 * atan2(q, p) * 180.0 / PI : 2.0 * atan2(p, q) * 180.0 / PI;
      return true;
    }
    case K_RHOMBUS_SIDE: {
      if (v[0] <= 0 || v[1] <= 0 || v[0] % 2 || v[1] % 2 || v[2] < 0 || v[2] > 1) return bad(err, cap, "rhombus diagonal givens");
      double side = sqrt((v[0] / 2.0) * (v[0] / 2.0) + (v[1] / 2.0) * (v[1] / 2.0));
      expect = v[2] == 0 ? side : 4.0 * side;
      return true;
    }
    case K_SQUARE_SIDE:
      if (v[0] <= 0) return bad(err, cap, "square diagonal");
      expect = v[0] / sqrt(2.0);
      return true;
    case K_TRAP_ANGLE: {
      unit = DEG;
      if (v[0] <= 0 || v[0] >= 180 || v[1] < 0 || v[1] > 3) return bad(err, cap, "trapezoid angle givens");
      if (v[1] >= 2 && v[0] >= 90) return bad(err, cap, "an isosceles trapezoid's angle at the large base is acute");
      expect = v[1] == 3 ? v[0] : 180.0 - v[0];
      return true;
    }
    case K_TRAP_MIDLINE: {
      int B, b, mode = v[2];
      if (mode == 1) {
        int m = v[0];
        b = v[1];
        B = 2 * m - b;
      } else {
        B = v[0], b = v[1];
      }
      if (b <= 0 || B <= b || (B + b) % 2) return bad(err, cap, "trapezoid bases");
      expect = mode == 0 ? (B + b) / 2.0 : mode == 1 ? B : (B - b) / 2.0;
      return true;
    }
    case K_TRAP_HEIGHT: {
      int B = v[0], b = v[1], c = v[2];
      if (b <= 0 || B <= b || (B - b) % 2 || c <= 0) return bad(err, cap, "isosceles trapezoid givens");
      double p = (B - b) / 2.0;
      if (c <= p) return bad(err, cap, "the leg must be longer than its projection");
      expect = sqrt((double)c * c - p * p);
      return true;
    }
    case K_AREA: {
      unit = " cm" SQ;
      if (ex.fig.kind == FIG_RIGHT_TRIANGLE) {  // the right-triangle figure keeps AB and AC in v[0], v[1]; the type is v[3]
        if (v[3] != 4 || v[0] <= 0 || v[1] <= 0) return bad(err, cap, "right triangle givens");
        expect = v[0] * v[1] / 2.0;
        return true;
      }
      int t = v[0];
      switch (t) {
        case 0:
          if (v[1] <= 0 || v[2] <= 0 || v[3] <= v[2]) return bad(err, cap, "parallelogram: the side must be longer than the height");
          expect = (double)v[1] * v[2];
          return true;
        case 1: expect = v[1] * v[2] / 2.0; return v[1] > 0 && v[2] > 0 ? true : bad(err, cap, "triangle givens");
        case 2: expect = v[1] * v[2] / 2.0; return v[1] > 0 && v[2] > 0 ? true : bad(err, cap, "rhombus givens");
        case 3:
          if (v[2] <= 0 || v[1] <= v[2] || v[3] <= 0) return bad(err, cap, "trapezoid givens");
          expect = (v[1] + v[2]) * v[3] / 2.0;
          return true;
        case 4: expect = v[1] * v[2] / 2.0; return v[1] > 0 && v[2] > 0 ? true : bad(err, cap, "right triangle givens");
        default: return bad(err, cap, "area type");
      }
    }
    case K_PERIMETER: {
      int t = v[0];
      switch (t) {
        case 0:
        case 3:
          if (v[1] <= 0 || v[2] <= 0) return bad(err, cap, "side not positive");
          expect = 2.0 * (v[1] + v[2]);
          return true;
        case 1:
        case 2:
          if (v[1] <= 0) return bad(err, cap, "side not positive");
          expect = 4.0 * v[1];
          return true;
        case 4:
          if (v[1] <= 0 || v[2] <= 0 || v[2] % 5) return bad(err, cap, "mixed units: the width is in half decimetres");
          expect = 2.0 * (v[1] + v[2]);
          return true;
        default: return bad(err, cap, "perimeter type");
      }
    }
    case K_HEIGHT_FROM_AREA:
      if (ex.fig.kind == FIG_RIGHT_TRIANGLE) {  // legs in v[0], v[1]; the type is v[3]
        if (v[3] != 2 || v[0] <= 0 || v[1] <= 0) return bad(err, cap, "leg givens");
        expect = (double)v[0] * v[1] / sqrt((double)v[0] * v[0] + (double)v[1] * v[1]);
        return true;
      }
      if (v[0] == 0) {
        if (v[1] <= 0 || v[2] <= 0) return bad(err, cap, "area givens");
        expect = (double)v[1] / v[2];
      } else if (v[0] == 1) {
        if (v[1] <= 0 || v[2] <= 0) return bad(err, cap, "area givens");
        expect = 2.0 * v[1] / v[2];
      } else if (v[0] == 2) {
        if (v[1] <= 0 || v[2] <= 0) return bad(err, cap, "leg givens");
        expect = (double)v[1] * v[2] / sqrt((double)v[1] * v[1] + (double)v[2] * v[2]);
      } else {
        return bad(err, cap, "height type");
      }
      return true;
    default: return bad(err, cap, "unknown Unit 4 kind");
  }
}

bool u4VerifyClassify(const Exercise &ex, char *err, int cap) {
  if (ex.fig.kind != FIG_QUAD) return bad(err, cap, "classify figure");
  int want = classifyRule(ex.fig.v[0]);
  int found = 0;
  for (int i = 0; i < ex.nopt; i++) {
    int code = ex.value[i].p - 1;
    if (code < 0 || code > 4) return bad(err, cap, "option code");
    if (strcmp(ex.opt[i], TX(CLASS_LABEL[code]))) return bad(err, cap, "option text does not match its code");
    for (int j = 0; j < i; j++)
      if (ex.value[j].p == ex.value[i].p) return bad(err, cap, "two options with the same figure");
    if (code == want) {
      found++;
      if (i != ex.correct) return bad(err, cap, "the right option is marked wrong");
    }
  }
  return found == 1 ? true : bad(err, cap, "no unique right option");
}

}  // namespace t7
