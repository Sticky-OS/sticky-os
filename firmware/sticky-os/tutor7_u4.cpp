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
  bad |= PR(ex.text, "Un patrulater convex ABCD are unghiul %s = %d" ANG ", unghiul %s = %d" ANG " și unghiul %s = %d" ANG ". Cât măsoară unghiul %s?", gl[0],
            g[0], gl[1], g[1], gl[2], g[2], V4[u]);
  Cand c[6];
  int nc = 0;
  SLIP(c, nc, exactInt(S), "Ai adunat cele trei unghiuri, dar suma lor nu este răspunsul. Cele patru unghiuri împreună au suma de 360" ANG ", deci unghiul cerut completează suma până la 360" ANG ".");
  SLIP(c, nc, exactInt(abs(180 - S)), "Ai folosit suma de 180" ANG ", cea de la triunghi. Într-un patrulater convex suma unghiurilor este 360" ANG ".");
  SLIP(c, nc, exactInt(360 - (g[0] + g[1])), "Ai uitat unul dintre unghiurile date. Scade din 360" ANG " toate cele trei unghiuri cunoscute.");
  SLIP(c, nc, exactInt(360 - (g[1] + g[2])), "Ai uitat unul dintre unghiurile date. Scade din 360" ANG " toate cele trei unghiuri cunoscute.");
  SLIP(c, nc, exactInt(360 - (g[0] + g[2])), "Ai uitat unul dintre unghiurile date. Scade din 360" ANG " toate cele trei unghiuri cunoscute.");
  bad |= PR(ex.clue[0], "Suma măsurilor celor patru unghiuri ale unui patrulater convex este aceeași pentru orice patrulater convex. Amintește-ți cât este.");
  bad |= PR(ex.clue[1], "Adună cele trei unghiuri date, apoi scade suma lor din totalul celor patru unghiuri.");
  bad |= PR(ex.solution, "Suma unghiurilor unui patrulater convex este 360" ANG ". Unghiul %s = 360" ANG " − (%d" ANG " + %d" ANG " + %d" ANG ") = 360" ANG " − %d" ANG " = %d" ANG ".",
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
  bad |= PR(ex.text, "Unghiurile unui patrulater convex sunt direct proporționale cu numerele %d, %d, %d și %d. Cât măsoară cel mai mare unghi?", r[0], r[1], r[2], r[3]);
  Cand c[5];
  int nc = 0;
  SLIP(c, nc, exactInt(90), "Ai împărțit suma în patru părți egale, dar unghiurile nu sunt egale: ele sunt proporționale cu numerele date.");
  SLIP(c, nc, exactInt(180 / S * m + (180 % S ? 0 : 0)), "Ai folosit suma de 180" ANG " în loc de suma unghiurilor unui patrulater convex.");
  SLIP(c, nc, exactInt(m), "Numerele date sunt doar proporțiile, nu măsurile în grade. Află mai întâi cât valorează o parte.");
  SLIP(c, nc, exactInt(part), "Aceasta este măsura unei singure părți. Cel mai mare unghi este format din mai multe astfel de părți.");
  SLIP(c, nc, exactInt(360 - big), "Aceasta este suma celorlalte trei unghiuri, nu măsura celui mai mare.");
  bad |= PR(ex.clue[0], "Suma celor patru unghiuri este aceeași pentru orice patrulater convex. Împarte-o în atâtea părți egale câte indică numerele date, luate împreună.");
  bad |= PR(ex.clue[1], "Află cât valorează o parte (suma unghiurilor împărțită la suma numerelor date), apoi înmulțește cu cel mai mare dintre numere.");
  bad |= PR(ex.solution, "Suma unghiurilor este 360" ANG ". Suma numerelor: %d + %d + %d + %d = %d, deci o parte are 360" ANG " : %d = %d" ANG ". Cel mai mare unghi: %d · %d" ANG " = %d" ANG ".",
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
  bad |= PR(ex.text, "În paralelogramul ABCD, unghiul A măsoară %d" ANG ". Cât măsoară unghiul %s?", A, name[which]);
  Cand c[6];
  int nc = 0;
  if (which == 1) {
    SLIP(c, nc, exactInt(180 - A), "Unghiul C este opus unghiului A, nu consecutiv cu el. Unghiurile opuse ale paralelogramului sunt congruente.");
    SLIP(c, nc, exactInt(90 - A > 0 ? 90 - A : 180 - 2 * A + A), "Ai scăzut din 90" ANG ". Nu există o astfel de relație între unghiurile paralelogramului.");
    SLIP(c, nc, exactInt(360 - A), "Ai scăzut din 360" ANG ". Suma de 360" ANG " este a tuturor celor patru unghiuri, iar unghiul C este opus lui A.");
    SLIP(c, nc, exactInt(360 - 2 * A), "Aceasta este suma dintre unghiul B și unghiul D, nu măsura unui singur unghi.");
  } else {
    SLIP(c, nc, exactInt(A), "Unghiul %s este consecutiv cu unghiul A (au o latură comună). Unghiurile consecutive ale paralelogramului sunt suplementare, nu egale.", name[which]);
    SLIP(c, nc, exactInt(abs(90 - A)), "Ai scăzut din 90" ANG ". Unghiurile consecutive ale paralelogramului însumează alt număr de grade.");
    SLIP(c, nc, exactInt(360 - A), "Ai scăzut din 360" ANG ". Suma de 360" ANG " este a tuturor celor patru unghiuri; două unghiuri consecutive au o sumă mai mică.");
    SLIP(c, nc, exactInt(360 - 2 * A), "Aceasta este suma dintre unghiul B și unghiul D, nu măsura unui singur unghi.");
  }
  bad |= PR(ex.clue[0], "Privește unde se află unghiul cerut față de unghiul A: este consecutiv (are o latură comună cu el) sau opus?");
  bad |= PR(ex.clue[1], "Unghiurile opuse ale unui paralelogram sunt congruente, iar două unghiuri consecutive sunt suplementare (suma lor este 180" ANG ").");
  if (which == 1)
    bad |= PR(ex.solution, "Unghiul C este opus unghiului A, iar unghiurile opuse ale paralelogramului sunt congruente: unghiul C = unghiul A = %d" ANG ".", A);
  else
    bad |= PR(ex.solution, "Unghiul %s este consecutiv cu unghiul A, deci cele două sunt suplementare: unghiul %s = 180" ANG " − %d" ANG " = %d" ANG ".", name[which], name[which], A, ans);
  return finish(ex, rng, bad, exactInt(ans), c, nc, ANG);
}

static bool genParPerimeter(Rng &rng, Exercise &ex) {
  bool bad = false;
  ex.kind = K_PAR_PERIMETER;
  ex.level = 1;
  ex.fig = Figure{FIG_PARALLELOGRAM, {0, 0, 0, 0}};
  Cand c[6];
  int nc = 0;
  if (rng.range(0, 1)) {  // the sides are given: the perimeter
    int a = rng.range(3, 15), b = rng.range(3, 15);
    if (a == b) return false;
    int P = 2 * (a + b);
    ex.fig.v[0] = a, ex.fig.v[1] = b, ex.fig.v[2] = 0;
    bad |= PR(ex.text, "Paralelogramul ABCD are AB = %d cm și BC = %d cm. Care este perimetrul lui?", a, b);
    SLIP(c, nc, exactInt(a + b), "Aceasta este suma a două laturi consecutive. Perimetrul însumează toate cele patru laturi, iar laturile opuse sunt congruente.");
    SLIP(c, nc, exactInt(a * b), "Ai înmulțit lungimile laturilor. Perimetrul este suma lungimilor laturilor, nu produsul lor.");
    SLIP(c, nc, exactInt(2 * a + b), "Ai socotit o latură o singură dată. Fiecare lungime apare de două ori, fiindcă laturile opuse sunt congruente.");
    SLIP(c, nc, exactInt(a + 2 * b), "Ai socotit o latură o singură dată. Fiecare lungime apare de două ori, fiindcă laturile opuse sunt congruente.");
    SLIP(c, nc, exactInt(4 * (a > b ? a : b)), "Ai considerat că toate laturile sunt egale cu cea mai lungă. Într-un paralelogram oarecare doar laturile opuse sunt congruente.");
    bad |= PR(ex.clue[0], "Într-un paralelogram laturile opuse sunt congruente. Câte laturi are perimetrul de adunat și ce lungimi au ele?");
    bad |= PR(ex.clue[1], "Perimetrul este de două ori suma a două laturi consecutive.");
    bad |= PR(ex.solution, "Laturile opuse sunt congruente, deci perimetrul este 2 · (AB + BC) = 2 · (%d + %d) = 2 · %d = %d cm.", a, b, a + b, P);
    return finish(ex, rng, bad, exactInt(P), c, nc, " cm");
  }
  int a = rng.range(3, 12), b = rng.range(3, 12);  // the perimeter and one side: the other
  if (a == b) return false;
  int P = 2 * (a + b);
  ex.fig.v[0] = P, ex.fig.v[1] = a, ex.fig.v[2] = 1;
  bad |= PR(ex.text, "Perimetrul paralelogramului ABCD este %d cm, iar latura AB are %d cm. Cât are latura BC?", P, a);
  SLIP(c, nc, exactInt(P / 2), "Aceasta este suma dintre AB și BC (jumătate din perimetru), nu lungimea lui BC.");
  SLIP(c, nc, exactInt(P - a), "Ai scăzut o singură latură din perimetru, dar în perimetru AB apare de două ori.");
  SLIP(c, nc, exactInt(P - 2 * a), "Ai scăzut cele două laturi egale cu AB, dar au rămas două laturi egale cu BC: împarte rezultatul la doi.");
  SLIP(c, nc, exactFrac(P, 4), "Ai împărțit perimetrul la patru, ca la un romb. Într-un paralelogram oarecare laturile consecutive nu sunt egale.");
  bad |= PR(ex.clue[0], "Perimetrul este de două ori suma a două laturi consecutive. Cât este această sumă?");
  bad |= PR(ex.clue[1], "Scade din suma a două laturi consecutive latura cunoscută.");
  bad |= PR(ex.solution, "Perimetrul este 2 · (AB + BC), deci AB + BC = %d : 2 = %d cm. Atunci BC = %d − %d = %d cm.", P, P / 2, P / 2, a, b);
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
  bad |= PR(ex.text, "Diagonalele paralelogramului ABCD se intersectează în punctul O. Se știe că AO = %d cm și BO = %d cm. Cât este %s?", ao, bo, ask == 0 ? "AC" : "BD");
  Cand c[6];
  int nc = 0;
  int mine = ask == 0 ? ao : bo, other = ask == 0 ? bo : ao;
  SLIP(c, nc, exactInt(mine), "Aceasta este doar jumătate din diagonală. Diagonala întreagă are două astfel de segmente.");
  SLIP(c, nc, exactInt(ao + bo), "Ai adunat segmentele de pe două diagonale diferite. Diagonala cerută are două segmente congruente.");
  SLIP(c, nc, exactInt(2 * other), "Aceasta este lungimea celeilalte diagonale. Fiecare diagonală are propriul ei punct O la mijloc.");
  SLIP(c, nc, exactInt(2 * (ao + bo)), "Ai adunat toate segmentele de la O la vârfuri. Diagonala cerută cuprinde doar două dintre ele.");
  bad |= PR(ex.clue[0], "Ce spune teorema despre punctul în care se intersectează diagonalele unui paralelogram?");
  bad |= PR(ex.clue[1], "Punctul O este mijlocul fiecărei diagonale, deci diagonala are lungimea egală cu de două ori un segment al ei.");
  bad |= PR(ex.solution, "Diagonalele unui paralelogram se înjumătățesc, deci O este mijlocul lui %s: %s = 2 · %s = 2 · %d = %d cm.", ask == 0 ? "AC" : "BD", ask == 0 ? "AC" : "BD",
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
static const char *CLASS_LABEL[5] = {"Doar patrulater convex", "Paralelogram", "Dreptunghi", "Romb", "Pătrat"};
static const char *CLASS_PLURAL[5] = {"patrulatere", "paralelograme", "dreptunghiuri", "romburi", "pătrate"};

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
  if ((f & F_DB) && (f & F_DC)) parts[np++] = "diagonalele congruente care se înjumătățesc", dbMerged = true;
  else if ((f & F_DB) && (f & F_DP)) parts[np++] = "diagonalele perpendiculare care se înjumătățesc", dbMerged = true;
  else if (f & F_DB) parts[np++] = "diagonalele care se înjumătățesc";
  if (f & F_OC2) parts[np++] = "laturile opuse congruente două câte două";
  if (f & F_PC) parts[np++] = "două laturi opuse paralele și congruente";
  if (f & F_PAR1) parts[np++] = "două laturi opuse paralele";
  if (f & F_S4) parts[np++] = "toate laturile congruente";
  if (f & F_CS) parts[np++] = "două laturi consecutive congruente";
  if (f & F_R3) parts[np++] = "trei unghiuri drepte";
  if (f & F_R2) parts[np++] = "două unghiuri drepte";
  if (f & F_RA) parts[np++] = "un unghi drept";
  if (!dbMerged) {
    if ((f & F_DC) && (f & F_DP)) parts[np++] = "diagonalele congruente și perpendiculare";
    else if (f & F_DC) parts[np++] = "diagonalele congruente";
    else if (f & F_DP) parts[np++] = "diagonalele perpendiculare";
  }
  if (!np) return false;
  for (int i = 0; i < np; i++) {
    size_t used = strlen(list);
    const char *sep = i == 0 ? "" : (i == np - 1 ? " și " : ", ");
    if (snprintf(list + used, sizeof list - used, "%s%s", sep, parts[i]) >= (int)(sizeof list - used)) return false;
  }
  bool bad = false;
  ex.kind = K_CLASSIFY;
  ex.level = 3;
  ex.fig = Figure{FIG_QUAD, {f, 0, 0, 0}};
  bad |= PR(ex.text, "%s ABCD are %s. Care este cea mai precisă denumire pe care o garantează aceste proprietăți?", (f & F_P) ? "Paralelogramul" : "Patrulaterul convex", list);
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
    bad |= PR(ex.opt[i], "%s", CLASS_LABEL[cd]);
    ex.why[i][0] = 0;
    if (cd == ans) {
      ex.correct = i;
      continue;
    }
    if (ans == 0)
      bad |= PR(ex.why[i], "Proprietățile date nu sunt suficiente: există patrulatere convexe care le respectă și nu sunt %s.", CLASS_PLURAL[cd]);
    else if (cd == 0)
      bad |= PR(ex.why[i], "Proprietățile date ajung ca să recunoști o figură particulară. Gândește-te ce tip de patrulater caracterizează fiecare dintre ele.");
    else if (cd < ans && !(cd == 2 && ans == 3) && !(cd == 3 && ans == 2))  // less than the properties give
      bad |= PR(ex.why[i], "Este adevărat, dar proprietățile date spun mai mult decât atât. Caută cea mai precisă denumire.");
    else if (cd == 4 && ans == 2)
      bad |= PR(ex.why[i], "Un dreptunghi cu laturile diferite respectă proprietățile date și nu este pătrat.");
    else if (cd == 4 && ans == 3)
      bad |= PR(ex.why[i], "Un romb fără unghi drept respectă proprietățile date și nu este pătrat.");
    else if (cd == 4 && ans == 1)
      bad |= PR(ex.why[i], "Există paralelograme care respectă proprietățile date și nu sunt pătrate.");
    else if (cd == 2 && ans == 1)
      bad |= PR(ex.why[i], "Există paralelograme care respectă proprietățile date și nu sunt dreptunghiuri.");
    else if (cd == 3 && ans == 1)
      bad |= PR(ex.why[i], "Există paralelograme care respectă proprietățile date și nu sunt romburi.");
    else if (cd == 2 && ans == 3)
      bad |= PR(ex.why[i], "Un romb fără unghi drept respectă proprietățile date și nu este dreptunghi.");
    else if (cd == 3 && ans == 2)
      bad |= PR(ex.why[i], "Un dreptunghi cu laturile diferite respectă proprietățile date și nu este romb.");
    else
      bad |= PR(ex.why[i], "Verifică din nou ce garantează fiecare proprietate.");
  }
  bad |= PR(ex.clue[0], "Mai întâi află dacă proprietățile garantează că laturile opuse sunt paralele. Apoi vezi ce adaugă celelalte: unghiuri, diagonale, laturi.");
  bad |= PR(ex.clue[1], "Alege doar ce rezultă sigur din toate proprietățile luate împreună, nu ce ar putea fi uneori adevărat.");
  static const char *SOL[5] = {
      "Proprietățile date nu garantează nici măcar că laturile opuse sunt paralele (există și trapeze care le respectă), deci rămâne doar denumirea de patrulater convex.",
      "Proprietățile date garantează că ABCD este paralelogram, dar nu mai mult: nu rezultă nici unghi drept, nici laturi consecutive congruente.",
      "ABCD este paralelogram, iar un paralelogram cu un unghi drept sau cu diagonalele congruente este dreptunghi (trei unghiuri drepte duc tot la dreptunghi).",
      "ABCD este paralelogram, iar un paralelogram cu două laturi consecutive congruente sau cu diagonalele perpendiculare este romb (toate laturile congruente duc tot la romb).",
      "ABCD este și dreptunghi (un unghi drept sau diagonale congruente) și romb (laturi consecutive congruente sau diagonale perpendiculare), deci este pătrat."};
  bad |= PR(ex.solution, "%s", SOL[ans]);
  return !bad;
}

// ---------------------------------------------------------------- Lesson 3: midline and centroid

static bool genMidline(Rng &rng, Exercise &ex) {
  int mode = rng.range(0, 1);
  bool bad = false;
  ex.kind = K_MIDLINE;
  ex.level = 1;
  Cand c[6];
  int nc = 0;
  int ans;
  if (mode == 0) {
    int bc = evenIn(rng, 6, 40);
    ans = bc / 2;
    ex.fig = Figure{FIG_MIDLINE, {bc, 0, 0, 0}};
    bad |= PR(ex.text, "În triunghiul ABC, M este mijlocul laturii AB, iar N este mijlocul laturii AC. Dacă BC = %d cm, cât este MN?", bc);
    SLIP(c, nc, exactInt(bc), "Linia mijlocie MN este paralelă cu BC, dar nu este la fel de lungă: este mai scurtă decât latura pe care o însoțește.");
    SLIP(c, nc, exactInt(2 * bc), "Ai dublat lungimea laturii. Linia mijlocie este mai scurtă decât latura paralelă cu ea.");
    SLIP(c, nc, exactFrac(bc, 4), "Ai împărțit la patru. Linia mijlocie este o jumătate din latura paralelă, nu un sfert din ea.");
    SLIP(c, nc, exactFrac(bc, 3), "Ai împărțit la trei, ca la centrul de greutate. Aici lucrezi cu mijloacele a două laturi, adică cu linia mijlocie.");
    bad |= PR(ex.solution, "MN este linia mijlocie a triunghiului ABC, deci MN = BC : 2 = %d : 2 = %d cm.", bc, ans);
  } else {
    int mn = rng.range(3, 20);
    ans = 2 * mn;
    ex.fig = Figure{FIG_MIDLINE, {mn, 1, 0, 0}};
    bad |= PR(ex.text, "În triunghiul ABC, M este mijlocul laturii AB, iar N este mijlocul laturii AC. Dacă MN = %d cm, cât este BC?", mn);
    SLIP(c, nc, exactInt(mn), "Latura BC este mai lungă decât linia mijlocie MN, care este paralelă cu ea.");
    SLIP(c, nc, exactFrac(mn, 2), "Ai înjumătățit în loc să dublezi. Linia mijlocie este jumătate din BC, deci BC este mai lungă decât MN.");
    SLIP(c, nc, exactInt(3 * mn), "Ai înmulțit cu trei, ca la centrul de greutate. Aici ai o linie mijlocie, care este jumătate din latura paralelă.");
    SLIP(c, nc, exactInt(4 * mn), "Ai înmulțit cu patru. Linia mijlocie este jumătate din latura paralelă, nu un sfert din ea.");
    bad |= PR(ex.solution, "MN este linia mijlocie, deci MN = BC : 2. Atunci BC = 2 · MN = 2 · %d = %d cm.", mn, ans);
  }
  bad |= PR(ex.clue[0], "Gândește-te la teorema liniei mijlocii: linia mijlocie este paralelă cu a treia latură și lungimea ei se leagă simplu de lungimea laturii.");
  bad |= PR(ex.clue[1], "Linia mijlocie are jumătate din lungimea laturii paralele cu ea. Alege între a înjumătăți și a dubla, după ce cunoști.");
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
  bad |= PR(ex.text, "Triunghiul ABC are laturile AB = %d cm, BC = %d cm și AC = %d cm. Se unesc mijloacele laturilor și se obține triunghiul format de cele trei linii mijlocii. Care este perimetrul lui?", a, b, cc);
  Cand c[6];
  int nc = 0;
  SLIP(c, nc, exactInt(P), "Acesta este perimetrul triunghiului ABC. Liniile mijlocii sunt mai scurte decât laturile.");
  SLIP(c, nc, exactFrac(P, 4), "Ai împărțit perimetrul la patru. Fiecare linie mijlocie este jumătate din latura ei, deci perimetrul se înjumătățește.");
  SLIP(c, nc, exactFrac(P, 3), "Ai împărțit la trei. Fiecare linie mijlocie este jumătate din latura paralelă cu ea.");
  SLIP(c, nc, exactFrac(a, 2), "Aceasta este o singură linie mijlocie. Perimetrul însumează cele trei linii mijlocii.");
  SLIP(c, nc, exactInt(2 * P), "Ai dublat perimetrul. Liniile mijlocii sunt mai scurte decât laturile cu care sunt paralele.");
  bad |= PR(ex.clue[0], "Fiecare latură a triunghiului format de liniile mijlocii este o linie mijlocie. Cum se leagă ea de latura paralelă din triunghiul ABC?");
  bad |= PR(ex.clue[1], "Cele trei linii mijlocii sunt jumătăți ale celor trei laturi. Adună-le sau înjumătățește perimetrul.");
  bad |= PR(ex.solution, "Fiecare linie mijlocie este jumătate din latura paralelă cu ea, deci perimetrul triunghiului format de ele este jumătate din perimetrul lui ABC: (%d + %d + %d) : 2 = %d : 2 = %d cm.", a, b, cc, P, ans);
  return finish(ex, rng, bad, exactInt(ans), c, nc, " cm");
}

static bool genCentroid(Rng &rng, Exercise &ex) {
  int mode = rng.range(0, 3);
  bool bad = false;
  ex.kind = K_CENTROID;
  ex.level = 2;
  Cand c[6];
  int nc = 0;
  int ans, given;
  if (mode <= 1) {
    given = 3 * rng.range(2, 10);
    ans = mode == 0 ? 2 * given / 3 : given / 3;
    ex.fig = Figure{FIG_MEDIANS, {given, mode, 0, 0}};
    bad |= PR(ex.text, "În triunghiul ABC, AM este mediană, iar G este centrul de greutate. Dacă AM = %d cm, cât este %s?", given, mode == 0 ? "GA" : "GM");
    if (mode == 0) {
      SLIP(c, nc, exactInt(given / 3), "Aceasta este lungimea lui GM. Centrul de greutate este mai aproape de mijlocul laturii decât de vârf, deci GA este segmentul mai lung.");
      SLIP(c, nc, exactFrac(given, 2), "Ai luat G drept mijlocul medianei. Centrul de greutate nu înjumătățește mediana.");
      SLIP(c, nc, exactInt(given), "Aceasta este lungimea întregii mediane AM, iar GA este doar o parte din ea.");
      SLIP(c, nc, exactInt(given / 3 * 4), "Aceasta depășește lungimea medianei. GA este o parte din AM, deci este mai scurt decât ea.");
      bad |= PR(ex.solution, "Centrul de greutate împarte mediana în raportul 2 : 1, începând de la vârf: GA = (2/3) · AM = (2/3) · %d = %d cm.", given, ans);
    } else {
      SLIP(c, nc, exactInt(2 * given / 3), "Aceasta este lungimea lui GA. Segmentul GM, dinspre mijlocul laturii, este cel mai scurt dintre cele două.");
      SLIP(c, nc, exactFrac(given, 2), "Ai luat G drept mijlocul medianei. Centrul de greutate nu înjumătățește mediana.");
      SLIP(c, nc, exactInt(given), "Aceasta este lungimea întregii mediane AM, iar GM este doar o parte din ea.");
      SLIP(c, nc, exactInt(given / 3 + 2), "Ai adăugat un număr la o treime din mediană, fără nicio regulă din teoremă. GM este exact o parte din mediană.");
      bad |= PR(ex.solution, "Centrul de greutate se află la o treime de latură: GM = (1/3) · AM = (1/3) · %d = %d cm.", given, ans);
    }
  } else {
    given = rng.range(2, 10);
    ans = mode == 2 ? 3 * given : 2 * given;
    ex.fig = Figure{FIG_MEDIANS, {given, mode, 0, 0}};
    bad |= PR(ex.text, "În triunghiul ABC, AM este mediană, iar G este centrul de greutate. Dacă GM = %d cm, cât este %s?", given, mode == 2 ? "AM" : "GA");
    if (mode == 2) {
      SLIP(c, nc, exactInt(2 * given), "Aceasta este lungimea lui GA. Mediana AM cuprinde și segmentul GM.");
      SLIP(c, nc, exactInt(given), "Aceasta este doar lungimea lui GM, o parte din mediană. Întreaga mediană este mai lungă.");
      SLIP(c, nc, exactInt(4 * given), "Ai socotit prea multe părți. Mediana are exact trei părți egale: două pentru GA și una pentru GM.");
      SLIP(c, nc, exactInt(given + 2), "Ai adăugat un număr la GM, fără nicio regulă din teoremă. Mediana are trei părți egale cu GM.");
      bad |= PR(ex.solution, "GM este o treime din mediană, deci AM = 3 · GM = 3 · %d = %d cm.", given, ans);
    } else {
      SLIP(c, nc, exactInt(3 * given), "Aceasta este lungimea întregii mediane AM, nu a lui GA. GA este mai scurt decât AM.");
      SLIP(c, nc, exactInt(given), "Aceasta este lungimea lui GM. GA este segmentul dinspre vârf, mai lung decât GM.");
      SLIP(c, nc, exactFrac(given, 2), "Ai înjumătățit lungimea lui GM. GA este mai lung decât GM, nu mai scurt.");
      SLIP(c, nc, exactInt(given + 2), "Ai adăugat un număr la GM, fără nicio regulă din teoremă. GA este un multiplu al lui GM.");
      bad |= PR(ex.solution, "GA este de două ori cât GM (G împarte mediana în raportul 2 : 1 de la vârf): GA = 2 · GM = 2 · %d = %d cm.", given, ans);
    }
  }
  bad |= PR(ex.clue[0], "Centrul de greutate împarte fiecare mediană în două segmente. Care este mai lung: cel dinspre vârf sau cel dinspre mijlocul laturii?");
  bad |= PR(ex.clue[1], "Mediana se împarte în trei părți egale: unul dintre segmente are o parte, celălalt are două părți.");
  return finish(ex, rng, bad, exactInt(ans), c, nc, " cm");
}

// ---------------------------------------------------------------- Lesson 4: the rectangle

static bool genRectDiagonal(Rng &rng, Exercise &ex) {
  int mode = rng.range(0, 2);
  bool bad = false;
  ex.kind = K_RECT_DIAGONAL;
  ex.level = 1;
  Cand c[6];
  int nc = 0;
  int ans, x;
  if (mode == 0) {
    x = evenIn(rng, 6, 30);
    ans = x;
    ex.fig = Figure{FIG_RECTANGLE, {x, 0, 0, 0}};
    bad |= PR(ex.text, "ABCD este dreptunghi, iar O este punctul de intersecție al diagonalelor. Dacă AC = %d cm, cât este BD?", x);
    SLIP(c, nc, exactInt(x / 2), "Aceasta este lungimea lui OB, o jumătate de diagonală. BD este o diagonală întreagă; compară-o cu AC: diagonalele dreptunghiului au o proprietate.");
    SLIP(c, nc, exactInt(2 * x), "Ai dublat lungimea unei diagonale. Fiecare diagonală este dată de la vârf la vârf, deci nu mai e nimic de dublat.");
    SLIP(c, nc, exactInt(x / 2 * 3), "Această valoare nu rezultă din nicio proprietate a dreptunghiului. Diagonalele lui sunt congruente.");
    SLIP(c, nc, exactInt(x / 4), "Aceasta este mult mai mică. BD este o diagonală întreagă, nu un sfert din alta.");
    bad |= PR(ex.solution, "Diagonalele unui dreptunghi sunt congruente, deci BD = AC = %d cm.", x);
  } else if (mode == 1) {
    x = rng.range(3, 15);
    ans = 2 * x;
    ex.fig = Figure{FIG_RECTANGLE, {x, 1, 0, 0}};
    bad |= PR(ex.text, "ABCD este dreptunghi, iar O este punctul de intersecție al diagonalelor. Dacă AO = %d cm, cât este BD?", x);
    SLIP(c, nc, exactInt(x), "Diagonalele dreptunghiului sunt congruente și se înjumătățesc, deci OB = OA. Aceasta este OB, o jumătate de diagonală, nu BD.");
    SLIP(c, nc, exactInt(4 * x), "Aceasta este suma dintre cele două diagonale (AC + BD). Se cere doar BD.");
    SLIP(c, nc, exactInt(3 * x), "Această valoare nu rezultă din proprietățile diagonalelor dreptunghiului.");
    SLIP(c, nc, exactFrac(x, 2), "Ai înjumătățit lungimea lui AO. Diagonala întreagă este mai lungă decât jumătatea ei.");
    bad |= PR(ex.solution, "Diagonalele dreptunghiului sunt congruente și se înjumătățesc, deci AC = 2 · AO = 2 · %d = %d cm. Atunci BD = AC = %d cm.", x, ans, ans);
  } else {
    x = rng.range(3, 15);
    ans = 4 * x;
    ex.fig = Figure{FIG_RECTANGLE, {x, 2, 0, 0}};
    bad |= PR(ex.text, "ABCD este dreptunghi, iar O este punctul de intersecție al diagonalelor. Dacă AO = %d cm, cât este AC + BD?", x);
    SLIP(c, nc, exactInt(2 * x), "Aceasta este lungimea unei singure diagonale. Se cere suma ambelor diagonale.");
    SLIP(c, nc, exactInt(x), "Aceasta este doar AO, o jumătate de diagonală. Se cer ambele diagonale întregi.");
    SLIP(c, nc, exactInt(8 * x), "Ai dublat o dată în plus. Fiecare diagonală este dublul lui AO, iar sunt două diagonale.");
    SLIP(c, nc, exactInt(3 * x), "Această valoare nu rezultă din proprietățile diagonalelor dreptunghiului.");
    bad |= PR(ex.solution, "Diagonalele dreptunghiului sunt congruente și se înjumătățesc: AC = 2 · AO = %d cm și BD = AC = %d cm. Suma lor: AC + BD = 2 · %d = %d cm.", 2 * x, 2 * x, 2 * x, ans);
  }
  bad |= PR(ex.clue[0], "Amintește-ți proprietatea diagonalelor unui dreptunghi: cum sunt ele între ele și cum se taie în punctul O?");
  bad |= PR(ex.clue[1], "Diagonalele dreptunghiului sunt congruente și fiecare are punctul O la mijloc. Folosește ambele proprietăți.");
  return finish(ex, rng, bad, exactInt(ans), c, nc, " cm");
}

static bool genRectAngle(Rng &rng, Exercise &ex) {
  int al = evenIn(rng, 20, 80);
  int ans = (180 - al) / 2;
  bool bad = false;
  ex.kind = K_RECT_ANGLE;
  ex.level = 2;
  ex.fig = Figure{FIG_RECTANGLE, {al, 0, 0, 0}};
  bad |= PR(ex.text, "ABCD este dreptunghi, iar diagonalele lui se intersectează în O. Unghiul ascuțit dintre diagonale, adică unghiul AOB, măsoară %d" ANG ". Cât măsoară unghiul OAB?", al);
  Cand c[6];
  int nc = 0;
  SLIP(c, nc, exactInt(90 - al), "Ai scăzut unghiul AOB din 90" ANG ". Triunghiul AOB nu este dreptunghic; suma unghiurilor lui este alta.");
  SLIP(c, nc, exactInt(al / 2), "Acesta este unghiul dintre diagonală și latura AD, nu unghiul OAB. Cele două unghiuri împreună formează unghiul drept din A.");
  SLIP(c, nc, exactInt(180 - al), "Acesta este unghiul obtuz dintre diagonale (unghiul AOD), nu un unghi de la baza triunghiului AOB.");
  SLIP(c, nc, exactInt(al), "Acesta este chiar unghiul AOB, de la vârful O. Se cere unghiul OAB, de la baza triunghiului isoscel.");
  SLIP(c, nc, exactInt((180 - al) / 2 + 10), "Valoarea nu rezultă din calcul: în triunghiul isoscel AOB cele două unghiuri de la bază sunt egale și completează suma de 180" ANG ".");
  bad |= PR(ex.clue[0], "Diagonalele dreptunghiului sunt congruente și se înjumătățesc, deci OA = OB. Ce fel de triunghi este AOB?");
  bad |= PR(ex.clue[1], "Într-un triunghi isoscel unghiurile de la bază sunt congruente, iar suma celor trei unghiuri este 180" ANG ".");
  bad |= PR(ex.solution, "OA = OB (diagonalele dreptunghiului sunt congruente și se înjumătățesc), deci triunghiul AOB este isoscel: unghiul OAB = (180" ANG " − %d" ANG ") : 2 = %d" ANG " : 2 = %d" ANG ".", al,
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
  bad |= PR(ex.text, "ABCD este romb, iar diagonala AC formează cu latura AB un unghi de %d" ANG ". Cât măsoară unghiul %s?", x, mode == 0 ? "BAD" : "ABC");
  Cand c[6];
  int nc = 0;
  if (mode == 0) {
    SLIP(c, nc, exactInt(x), "Acesta este doar unghiul dintre diagonală și latura AB. Diagonala este bisectoarea unghiului BAD, deci unghiul BAD cuprinde două astfel de unghiuri.");
    SLIP(c, nc, exactInt(90 - x), "Aceasta este măsura unghiului ABD (dintre diagonala BD și latura AB), nu a unghiului BAD.");
    SLIP(c, nc, exactInt(180 - x), "Ai scăzut din 180" ANG " unghiul dintre diagonală și latură. Relația corectă folosește faptul că diagonala este bisectoare.");
    SLIP(c, nc, exactInt(180 - 2 * x), "Acesta este unghiul ABC, consecutiv cu unghiul BAD. Se cere unghiul BAD.");
    bad |= PR(ex.solution, "Diagonalele rombului sunt bisectoarele unghiurilor, deci unghiul BAD = 2 · unghiul BAC = 2 · %d" ANG " = %d" ANG ".", x, ans);
  } else {
    SLIP(c, nc, exactInt(2 * x), "Acesta este unghiul BAD, consecutiv cu unghiul ABC. Două unghiuri consecutive ale rombului sunt suplementare, nu egale.");
    SLIP(c, nc, exactInt(x), "Acesta este doar unghiul dintre diagonală și latura AB. Mai întâi află unghiul BAD, apoi folosește unghiurile consecutive.");
    SLIP(c, nc, exactInt(90 - x), "Aceasta este măsura unghiului ABD, jumătate din unghiul ABC. Se cere unghiul ABC întreg.");
    SLIP(c, nc, exactInt(180 - x), "Ai scăzut din 180" ANG " unghiul dintre diagonală și latură. Mai întâi află unghiul BAD, apoi unghiul consecutiv cu el.");
    bad |= PR(ex.solution, "Diagonala AC este bisectoare, deci unghiul BAD = 2 · %d" ANG " = %d" ANG ". Unghiurile BAD și ABC sunt consecutive, deci suplementare: unghiul ABC = 180" ANG " − %d" ANG " = %d" ANG ".", x, 2 * x,
              2 * x, ans);
  }
  bad |= PR(ex.clue[0], "Într-un romb, diagonalele sunt bisectoarele unghiurilor. Ce înseamnă asta pentru unghiul pe care îl face diagonala AC cu latura AB?");
  bad |= PR(ex.clue[1], "Rombul este paralelogram: unghiurile consecutive sunt suplementare, iar o bisectoare împarte unghiul în două părți egale.");
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
  int mode = level <= 1 ? rng.range(0, 1) : rng.range(0, 1);
  int d1 = 2 * a, d2 = 2 * b;
  Exact right = mode == 0 ? side : exactNorm(4LL * side.p, side.r, side.q);
  bool bad = false;
  ex.kind = K_RHOMBUS_SIDE;
  ex.level = level <= 1 ? 1 : 2;
  ex.fig = Figure{FIG_RHOMBUS, {d1, d2, mode, 0}};
  bad |= PR(ex.text, "Diagonalele rombului ABCD măsoară %d cm și %d cm. Cât este %s?", d1, d2, mode == 0 ? "latura rombului" : "perimetrul rombului");
  Cand c[7];
  int nc = 0;
  char f[32];
  if (mode == 0) {
    SLIP(c, nc, exactInt(a + b), "Ai adunat jumătățile diagonalelor. În triunghiul dreptunghic AOB acestea sunt catetele, iar latura rombului este ipotenuza: se folosește teorema lui Pitagora.");
    SLIP(c, nc, exactSqrt(sq(d1) + sq(d2)), "Ai folosit diagonalele întregi. Diagonalele rombului se taie în O, iar catetele triunghiului dreptunghic AOB sunt jumătățile lor.");
    SLIP(c, nc, exactInt(d1 * d2 / 2), "Aceasta este aria rombului, nu latura lui.");
    SLIP(c, nc, exactInt(a > b ? a : b), "Ai luat cea mai lungă jumătate de diagonală. Latura rombului este ipotenuza triunghiului AOB, deci este mai lungă decât orice jumătate de diagonală.");
    SLIP(c, nc, exactInt((int32_t)n), "Ai uitat să extragi rădăcina pătrată: suma pătratelor catetelor este pătratul laturii, nu latura.");
  } else {
    SLIP(c, nc, side, "Aceasta este lungimea unei laturi. Perimetrul însumează cele patru laturi congruente.");
    SLIP(c, nc, exactNorm(2LL * side.p, side.r, side.q), "Ai socotit doar două laturi. Rombul are patru laturi congruente.");
    SLIP(c, nc, exactInt(2 * (a + b)), "Ai calculat 2 · (suma jumătăților de diagonală), fără teorema lui Pitagora. Mai întâi află latura, ca ipotenuză în triunghiul AOB.");
    SLIP(c, nc, exactNorm(4LL * exactSqrt(sq(d1) + sq(d2)).p, exactSqrt(sq(d1) + sq(d2)).r, 1), "Ai folosit diagonalele întregi la calculul laturii. Catetele triunghiului AOB sunt jumătățile lor.");
    SLIP(c, nc, exactInt(d1 * d2 / 2), "Aceasta este aria rombului, nu perimetrul lui.");
  }
  bad |= PR(ex.clue[0], "Diagonalele rombului sunt perpendiculare și se înjumătățesc. Privește triunghiul AOB: ce fel de triunghi este și care sunt catetele lui?");
  bad |= PR(ex.clue[1], "Catetele triunghiului AOB sunt jumătățile diagonalelor, iar latura rombului este ipotenuza: folosește teorema lui Pitagora%s.", mode == 0 ? "" : ", apoi înmulțește cu numărul de laturi");
  exactFormat(side, f, sizeof f);
  if (mode == 0)
    bad |= PR(ex.solution, "Diagonalele se taie perpendicular în mijlocul lor: AO = %d cm și BO = %d cm. În triunghiul dreptunghic AOB: AB" SQ " = %d" SQ " + %d" SQ " = %d, deci latura este %s cm.", a, b, a, b,
              (int)n, f);
  else {
    char g[32];
    exactFormat(right, g, sizeof g);
    bad |= PR(ex.solution, "Diagonalele se taie perpendicular în mijlocul lor: AO = %d cm și BO = %d cm. În triunghiul AOB: AB" SQ " = %d" SQ " + %d" SQ " = %d, deci AB = %s cm. Perimetrul: 4 · %s = %s cm.", a, b, a, b,
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
  bad |= PR(ex.text, "Diagonala unui pătrat măsoară %d cm. Cât este latura pătratului?", d);
  Cand c[6];
  int nc = 0;
  SLIP(c, nc, exactInt(d), "Latura pătratului este mai scurtă decât diagonala: diagonala este ipotenuza unui triunghi dreptunghic isoscel, iar latura este una dintre catete.");
  SLIP(c, nc, exactFrac(d, 2), "Ai înjumătățit diagonala. Din teorema lui Pitagora, pătratul diagonalei este dublul pătratului laturii, nu de patru ori mai mare.");
  SLIP(c, nc, exactNorm(d, 2, 1), "Aceasta ar fi diagonala unui pătrat cu latura de %d cm. Aici diagonala este cea dată, iar latura se caută.", d);
  SLIP(c, nc, exactFrac((int32_t)(d * d), 2), "Ai uitat să extragi rădăcina pătrată: aceasta este pătratul laturii, nu latura.");
  SLIP(c, nc, exactNorm(d, 3, 2), "Ai folosit formula de la triunghiul echilateral. Aici triunghiul dreptunghic este isoscel, iar diagonala este ipotenuza lui.");
  char f[32];
  exactFormat(right, f, sizeof f);
  bad |= PR(ex.clue[0], "Diagonala împarte pătratul în două triunghiuri dreptunghice isoscele. Care este ipotenuza și care sunt catetele?");
  bad |= PR(ex.clue[1], "Fie l latura. Scrie relația lui Pitagora: diagonala la pătrat este suma pătratelor celor două catete, adică de două ori l la pătrat.");
  bad |= PR(ex.solution, "Fie l latura. Din teorema lui Pitagora: %d" SQ " = l" SQ " + l" SQ " = 2 · l" SQ ", deci l" SQ " = %d : 2 și l = %d : " RT "2 = %s cm.", d, d * d, d, f);
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
  bad |= PR(ex.text, "ABCD este un trapez%s, cu laturile AB și CD paralele, iar unghiul %s măsoară %d" ANG ". Cât măsoară unghiul %s?", iso ? " isoscel" : "", given[mode], x, ask[mode]);
  Cand c[6];
  int nc = 0;
  if (mode <= 1) {
    SLIP(c, nc, exactInt(x), "Unghiul %s și unghiul %s sunt alăturate aceleiași laturi neparalele, deci sunt suplementare, nu egale.", given[mode], ask[mode]);
    SLIP(c, nc, exactInt(abs(90 - x)), "Ai scăzut din 90" ANG ". Unghiurile alăturate unei laturi neparalele au altă sumă.");
    SLIP(c, nc, exactInt(360 - x), "Ai scăzut din 360" ANG ", suma tuturor celor patru unghiuri. Aici folosești doar două unghiuri alăturate unei laturi neparalele.");
    SLIP(c, nc, exactFrac(180 - x, 2), "Ai înjumătățit diferența până la 180" ANG ". Cele două unghiuri sunt suplementare, nu una jumătate din cealaltă.");
    bad |= PR(ex.clue[0], "Unghiul cerut și unghiul dat sunt alăturate aceleiași laturi neparalele a trapezului. Ce relație au astfel de unghiuri?");
    bad |= PR(ex.clue[1], "În orice trapez, unghiurile alăturate unei laturi neparalele sunt suplementare: suma lor este 180" ANG ".");
    bad |= PR(ex.solution, "Laturile AB și CD sunt paralele, deci unghiurile alăturate laturii neparalele %s sunt suplementare: unghiul %s = 180" ANG " − %d" ANG " = %d" ANG ".", mode == 0 ? "AD" : "BC", ask[mode], x, ans);
  } else if (mode == 2) {
    SLIP(c, nc, exactInt(x), "Ai crezut că unghiul C este congruent cu unghiul A. În trapezul isoscel sunt congruente unghiurile alăturate aceleiași baze: A cu B și C cu D.");
    SLIP(c, nc, exactInt(abs(90 - x)), "Ai scăzut din 90" ANG ". Nu există o astfel de relație în trapez.");
    SLIP(c, nc, exactInt(360 - x), "Ai scăzut din 360" ANG ", suma tuturor celor patru unghiuri. Folosești doar două unghiuri.");
    SLIP(c, nc, exactInt(360 - 2 * x), "Aceasta este suma dintre unghiul C și unghiul D, nu măsura unghiului C singur.");
    bad |= PR(ex.clue[0], "Mai întâi află unghiul D, alăturat aceleiași laturi neparalele AD ca unghiul A. Apoi folosește proprietatea trapezului isoscel.");
    bad |= PR(ex.clue[1], "Unghiurile A și D sunt suplementare, iar în trapezul isoscel unghiurile alăturate bazei mici sunt congruente.");
    bad |= PR(ex.solution, "Unghiurile A și D sunt alăturate laturii neparalele AD, deci unghiul D = 180" ANG " − %d" ANG " = %d" ANG ". În trapezul isoscel unghiurile alăturate bazei mici sunt congruente: unghiul C = unghiul D = %d" ANG ".", x, ans, ans);
  } else {
    SLIP(c, nc, exactInt(180 - x), "Aceasta este măsura unghiurilor de la baza mică. Unghiul B este alăturat aceleiași baze AB ca unghiul A.");
    SLIP(c, nc, exactInt(abs(90 - x)), "Ai scăzut din 90" ANG ". Nu există o astfel de relație în trapez.");
    SLIP(c, nc, exactInt(360 - x), "Ai scăzut din 360" ANG ", suma tuturor celor patru unghiuri. Folosești doar un unghi.");
    SLIP(c, nc, exactInt(360 - 2 * x), "Aceasta este suma dintre unghiurile C și D, nu măsura unghiului B.");
    bad |= PR(ex.clue[0], "Unghiurile A și B sunt alăturate aceleiași baze a trapezului. Ce proprietate au ele în trapezul isoscel?");
    bad |= PR(ex.clue[1], "Într-un trapez isoscel, unghiurile alăturate aceleiași baze sunt congruente.");
    bad |= PR(ex.solution, "Trapezul este isoscel, iar unghiurile A și B sunt alăturate bazei AB, deci sunt congruente: unghiul B = unghiul A = %d" ANG ".", x);
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
  Cand c[6];
  int nc = 0;
  int ans;
  if (mode == 0) {
    ans = (B + b) / 2;
    ex.fig = Figure{FIG_TRAPEZOID, {B, b, 0, 0}};
    bad |= PR(ex.text, "Trapezul ABCD are bazele AB = %d cm și CD = %d cm. M și N sunt mijloacele laturilor neparalele AD și BC. Cât este MN?", B, b);
    SLIP(c, nc, exactInt(B + b), "Aceasta este suma bazelor. Linia mijlocie este jumătate din această sumă.");
    SLIP(c, nc, exactInt((B - b) / 2), "Aceasta este lungimea segmentului dintre diagonale, nu a liniei mijlocii. Linia mijlocie folosește suma bazelor.");
    SLIP(c, nc, exactInt(B - b), "Ai scăzut bazele. Linia mijlocie a trapezului folosește suma bazelor.");
    SLIP(c, nc, exactFrac(B + b, 4), "Ai împărțit suma bazelor la patru. Linia mijlocie este jumătate din suma bazelor.");
    SLIP(c, nc, exactFrac(B, 2), "Ai luat jumătate din baza mare. Linia mijlocie depinde de ambele baze.");
    bad |= PR(ex.solution, "MN este linia mijlocie a trapezului, deci MN = (AB + CD) : 2 = (%d + %d) : 2 = %d : 2 = %d cm.", B, b, B + b, ans);
  } else if (mode == 1) {
    int m = (B + b) / 2;
    ans = B;
    ex.fig = Figure{FIG_TRAPEZOID, {m, b, 1, 0}};
    bad |= PR(ex.text, "Linia mijlocie MN a trapezului ABCD are %d cm, iar baza mică CD are %d cm. Cât are baza mare AB?", m, b);
    SLIP(c, nc, exactInt(m + b), "Ai adunat linia mijlocie cu baza mică. Linia mijlocie este jumătate din suma bazelor, deci suma bazelor este dublul ei.");
    SLIP(c, nc, exactInt(2 * m), "Aceasta este suma bazelor. Din ea trebuie scăzută baza mică ca să rămână baza mare.");
    SLIP(c, nc, exactInt(m - b), "Ai scăzut baza mică din linia mijlocie. Mai întâi dublează linia mijlocie, ca să obții suma bazelor.");
    SLIP(c, nc, exactInt(2 * m + b), "Ai adunat baza mică la suma bazelor. Baza mică trebuie scăzută din suma bazelor.");
    bad |= PR(ex.solution, "MN = (AB + CD) : 2, deci AB + CD = 2 · MN = 2 · %d = %d cm. Atunci AB = %d − %d = %d cm.", m, 2 * m, 2 * m, b, ans);
  } else {
    ans = (B - b) / 2;
    ex.fig = Figure{FIG_TRAPEZOID, {B, b, 2, 0}};
    bad |= PR(ex.text, "Trapezul ABCD are bazele AB = %d cm și CD = %d cm. Linia lui mijlocie MN intersectează diagonalele AC și BD în P și în Q. Cât este PQ?", B, b);
    SLIP(c, nc, exactInt((B + b) / 2), "Aceasta este lungimea întregii linii mijlocii MN. PQ este doar segmentul ei dintre diagonale.");
    SLIP(c, nc, exactInt(B - b), "Ai scăzut bazele, dar segmentul dintre diagonale este jumătate din această diferență.");
    SLIP(c, nc, exactFrac(b, 2), "Aceasta este jumătate din baza mică, adică lungimea lui MP, nu a lui PQ.");
    SLIP(c, nc, exactFrac(B, 2), "Aceasta este jumătate din baza mare, adică lungimea lui MQ, nu a lui PQ.");
    bad |= PR(ex.solution, "Segmentul de pe linia mijlocie cuprins între diagonale are lungimea jumătate din diferența bazelor: PQ = (AB − CD) : 2 = (%d − %d) : 2 = %d : 2 = %d cm.", B, b, B - b, ans);
  }
  bad |= PR(ex.clue[0], mode == 2 ? "Pe linia mijlocie a trapezului, segmentul dintre diagonale are o lungime legată de diferența bazelor. Care este relația?"
                                  : "Linia mijlocie a unui trapez unește mijloacele laturilor neparalele. Cum se leagă lungimea ei de lungimile bazelor?");
  bad |= PR(ex.clue[1], mode == 2 ? "Segmentul dintre diagonale este jumătate din diferența bazelor (baza mare minus baza mică)."
                                  : "Linia mijlocie este jumătate din suma bazelor. Folosește relația în sensul potrivit pentru ce cunoști.");
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
  bad |= PR(ex.text, "Trapezul isoscel ABCD, cu bazele AB = %d cm și CD = %d cm, are laturile neparalele AD = BC = %d cm. Cât este înălțimea trapezului?", B, b, c0);
  Cand c[7];
  int nc = 0;
  SLIP(c, nc, exactInt(c0), "Aceasta este lungimea laturii neparalele, care este oblică. Înălțimea este perpendiculară pe baze, deci este mai scurtă decât latura.");
  SLIP(c, nc, exactSqrt(sq(c0) + sq(p)), "Ai adunat pătratele. Latura neparalelă este ipotenuza triunghiului dreptunghic format de înălțime, deci pătratul înălțimii este o diferență.");
  SLIP(c, nc, exactInt(c0 - p), "Ai scăzut lungimile. Teorema lui Pitagora lucrează cu pătratele: înălțimea la pătrat este diferența pătratelor.");
  SLIP(c, nc, exactInt(p), "Aceasta este proiecția laturii neparalele pe baza mare, adică jumătate din diferența bazelor, nu înălțimea.");
  SLIP(c, nc, exactInt((int32_t)n), "Ai uitat să extragi rădăcina pătrată: diferența pătratelor este pătratul înălțimii, nu înălțimea.");
  if (sq(c0) > sq(2 * p)) SLIP(c, nc, exactSqrt(sq(c0) - sq(2 * p)), "Proiecția laturii pe baza mare este jumătate din diferența bazelor, nu întreaga diferență.");
  char f[32];
  exactFormat(right, f, sizeof f);
  bad |= PR(ex.clue[0], "Ducând înălțimile din vârfurile bazei mici se formează două triunghiuri dreptunghice congruente. Cât este proiecția laturii neparalele pe baza mare?");
  bad |= PR(ex.clue[1], "Proiecția este jumătate din diferența bazelor. Apoi aplică teorema lui Pitagora: latura neparalelă este ipotenuza, iar înălțimea este o catetă.");
  bad |= PR(ex.solution, "Proiecția laturii pe baza mare este (AB − CD) : 2 = (%d − %d) : 2 = %d cm. În triunghiul dreptunghic format: h" SQ " = %d" SQ " − %d" SQ " = %d, deci h = %s cm.", B, b, p, c0, p, (int)n, f);
  return finish(ex, rng, bad, right, c, nc, " cm");
}

// ---------------------------------------------------------------- Lesson 7: perimeters and areas

static bool genArea(Rng &rng, Exercise &ex) {
  int type = rng.range(0, 4);
  bool bad = false;
  ex.kind = K_AREA;
  ex.level = 2;
  Cand c[7];
  int nc = 0;
  int ans;
  if (type == 0) {  // parallelogram: base, side, height
    int a = rng.range(6, 14), h = rng.range(3, 9), s = h + rng.range(1, 6);
    ans = a * h;
    ex.fig = Figure{FIG_PARALLELOGRAM, {0, a, h, s}};
    ex.fig.v[0] = 0;
    bad |= PR(ex.text, "Paralelogramul ABCD are baza AB = %d cm, latura BC = %d cm și înălțimea corespunzătoare bazei AB egală cu %d cm. Care este aria lui?", a, s, h);
    SLIP(c, nc, exactInt(a * s), "Ai înmulțit baza cu latura alăturată. Aria este baza înmulțită cu înălțimea, iar înălțimea este perpendiculară pe bază, nu oblică.");
    SLIP(c, nc, exactFrac(a * h, 2), "Ai împărțit la doi, ca la triunghi. Aria paralelogramului este baza înmulțită cu înălțimea, fără împărțire.");
    SLIP(c, nc, exactInt(2 * (a + s)), "Aceasta este perimetrul, nu aria.");
    SLIP(c, nc, exactInt(a + h), "Ai adunat baza cu înălțimea. Aria se obține prin înmulțire.");
    bad |= PR(ex.clue[0], "Aria unui paralelogram se calculează cu o bază și cu înălțimea corespunzătoare ei. Care dintre numerele date sunt acestea?");
    bad |= PR(ex.clue[1], "Înmulțește baza cu înălțimea corespunzătoare. Latura oblică nu intră în formulă.");
    bad |= PR(ex.solution, "A = AB · h = %d · %d = %d cm" SQ ".", a, h, ans);
    ex.fig.v[0] = 0;
  } else if (type == 1) {  // triangle
    int b = rng.range(6, 16), h = rng.range(4, 12);
    if ((b * h) % 2) return false;
    ans = b * h / 2;
    ex.fig = Figure{FIG_NONE, {1, b, h, 0}};
    bad |= PR(ex.text, "Un triunghi are o latură de %d cm, iar înălțimea corespunzătoare ei are %d cm. Care este aria triunghiului?", b, h);
    SLIP(c, nc, exactInt(b * h), "Ai uitat să împarți la doi. Aria triunghiului este jumătate din produsul dintre o latură și înălțimea ei.");
    SLIP(c, nc, exactInt(b + h), "Ai adunat latura cu înălțimea. Aria se obține prin înmulțire și împărțire la doi.");
    SLIP(c, nc, exactFrac(b * h, 4), "Ai împărțit de două ori la doi. Formula are o singură împărțire la doi.");
    SLIP(c, nc, exactFrac(b + h, 2), "Ai făcut media dintre latură și înălțime. Aria triunghiului este jumătate din produsul lor.");
    bad |= PR(ex.clue[0], "Aria triunghiului este legată de aria unui dreptunghi sau paralelogram cu aceeași bază și înălțime. Care este legătura?");
    bad |= PR(ex.clue[1], "Înmulțește latura cu înălțimea corespunzătoare, apoi împarte rezultatul la doi.");
    bad |= PR(ex.solution, "A = (latura · înălțimea) : 2 = (%d · %d) : 2 = %d : 2 = %d cm" SQ ".", b, h, b * h, ans);
  } else if (type == 2) {  // rhombus from the diagonals
    int d1 = rng.range(4, 16), d2 = rng.range(4, 16);
    if ((d1 * d2) % 2 || d1 == d2) return false;
    ans = d1 * d2 / 2;
    ex.fig = Figure{FIG_RHOMBUS, {2, d1, d2, 0}};
    bad |= PR(ex.text, "Un romb are diagonalele de %d cm și de %d cm. Care este aria lui?", d1, d2);
    SLIP(c, nc, exactInt(d1 * d2), "Ai uitat să împarți la doi. Aria rombului este jumătate din produsul diagonalelor.");
    SLIP(c, nc, exactInt(d1 + d2), "Ai adunat diagonalele. Aria se obține prin înmulțire și împărțire la doi.");
    SLIP(c, nc, exactFrac(d1 * d2, 4), "Ai împărțit la patru. Aria rombului este jumătate din produsul diagonalelor, nu un sfert.");
    SLIP(c, nc, exactFrac(d1 + d2, 2), "Ai făcut media diagonalelor. Aria este jumătate din produsul lor.");
    bad |= PR(ex.clue[0], "Diagonalele împart rombul în patru triunghiuri dreptunghice congruente. Care sunt catetele fiecăruia?");
    bad |= PR(ex.clue[1], "Aria rombului este jumătate din produsul diagonalelor.");
    bad |= PR(ex.solution, "A = (d" "1" " · d" "2" ") : 2 = (%d · %d) : 2 = %d : 2 = %d cm" SQ ".", d1, d2, d1 * d2, ans);
  } else if (type == 3) {  // trapezoid
    int B = rng.range(8, 20), b = rng.range(3, B - 2), h = rng.range(3, 10);
    if (((B + b) * h) % 2 || B <= b) return false;
    ans = (B + b) * h / 2;
    ex.fig = Figure{FIG_TRAPEZOID, {3, B, b, h}};
    bad |= PR(ex.text, "Trapezul ABCD are baza mare AB = %d cm, baza mică CD = %d cm și înălțimea de %d cm. Care este aria lui?", B, b, h);
    SLIP(c, nc, exactInt((B + b) * h), "Ai uitat să împarți la doi. Aria trapezului este jumătate din suma bazelor înmulțită cu înălțimea.");
    SLIP(c, nc, exactInt(B * h), "Ai folosit doar baza mare. Aria trapezului depinde de ambele baze.");
    SLIP(c, nc, exactFrac((B - b) * h, 2), "Ai scăzut bazele. În formula ariei apare suma bazelor.");
    SLIP(c, nc, exactFrac(B + b, 2), "Aceasta este lungimea liniei mijlocii. Pentru arie trebuie să o înmulțești cu înălțimea.");
    bad |= PR(ex.clue[0], "Aria trapezului se poate calcula ca linia mijlocie înmulțită cu înălțimea. Cum se află linia mijlocie din baze?");
    bad |= PR(ex.clue[1], "Adună bazele, înmulțește suma cu înălțimea, apoi împarte la doi.");
    bad |= PR(ex.solution, "A = ((AB + CD) · h) : 2 = ((%d + %d) · %d) : 2 = %d : 2 = %d cm" SQ ".", B, b, h, (B + b) * h, ans);
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
    ex.fig = Figure{FIG_RIGHT_TRIANGLE, {c1, c2, 0, 4}};
    bad |= PR(ex.text, "Triunghiul ABC are unghiul A = 90°, cateta AB = %d cm și cateta AC = %d cm. Ipotenuza BC are %d cm. Care este aria triunghiului?", c1, c2, hy);
    SLIP(c, nc, exactInt(c1 * c2), "Ai uitat să împarți la doi. Aria triunghiului dreptunghic este jumătate din produsul catetelor.");
    SLIP(c, nc, exactInt(c1 + c2), "Ai adunat catetele. Aria se obține prin înmulțire și împărțire la doi.");
    SLIP(c, nc, exactFrac(hy * c1, 2), "Ai înmulțit ipotenuza cu o catetă. Înălțimea corespunzătoare ipotenuzei nu este o catetă; catetele sunt perpendiculare una pe alta, deci una este înălțimea celeilalte.");
    SLIP(c, nc, exactInt(c1 + c2 + hy), "Aceasta este perimetrul triunghiului (suma laturilor), nu aria lui.");
    bad |= PR(ex.clue[0], "În triunghiul dreptunghic, o catetă este înălțimea corespunzătoare celeilalte catete. Ce rezultă pentru arie?");
    bad |= PR(ex.clue[1], "Înmulțește cele două catete, apoi împarte la doi. Ipotenuza nu este necesară.");
    bad |= PR(ex.solution, "Catetele sunt perpendiculare, deci A = (AB · AC) : 2 = (%d · %d) : 2 = %d : 2 = %d cm" SQ ".", c1, c2, c1 * c2, ans);
  }
  return finish(ex, rng, bad, exactInt(ans), c, nc, CM2);
}

static bool genPerimeter(Rng &rng, Exercise &ex) {
  int type = rng.range(0, 4);
  bool bad = false;
  ex.kind = K_PERIMETER;
  ex.level = 1;
  Cand c[7];
  int nc = 0;
  int ans;
  if (type == 0 || type == 3) {
    int a = rng.range(3, 18), b = rng.range(3, 18);
    if (a == b) return false;
    ans = 2 * (a + b);
    ex.fig = Figure{type == 0 ? FIG_RECTANGLE : FIG_PARALLELOGRAM, {type, a, b, 0}};
    ex.fig.v[0] = type;
    if (type == 0) bad |= PR(ex.text, "Un dreptunghi are lungimea de %d cm și lățimea de %d cm. Care este perimetrul lui?", a, b);
    else bad |= PR(ex.text, "Un paralelogram are două laturi consecutive de %d cm și de %d cm. Care este perimetrul lui?", a, b);
    SLIP(c, nc, exactInt(a + b), "Aceasta este suma a două laturi consecutive. Perimetrul însumează toate cele patru laturi.");
    SLIP(c, nc, exactInt(a * b), "Aceasta este produsul lungimilor (aria unui dreptunghi cu aceste laturi), nu perimetrul.");
    SLIP(c, nc, exactInt(2 * a + b), "Ai socotit o latură o singură dată. Laturile opuse sunt congruente, deci fiecare lungime apare de două ori.");
    SLIP(c, nc, exactInt(a + 2 * b), "Ai socotit o latură o singură dată. Laturile opuse sunt congruente, deci fiecare lungime apare de două ori.");
    bad |= PR(ex.clue[0], "Perimetrul este suma lungimilor tuturor laturilor. Câte laturi sunt și care dintre ele sunt congruente?");
    bad |= PR(ex.clue[1], "Perimetrul este de două ori suma a două laturi consecutive.");
    bad |= PR(ex.solution, "P = 2 · (%d + %d) = 2 · %d = %d cm.", a, b, a + b, ans);
  } else if (type == 1 || type == 2) {
    int l = rng.range(3, 20);
    ans = 4 * l;
    ex.fig = Figure{type == 1 ? FIG_NONE : FIG_RHOMBUS, {type, l, 0, 0}};
    bad |= PR(ex.text, "Un %s are latura de %d cm. Care este perimetrul lui?", type == 1 ? "pătrat" : "romb", l);
    SLIP(c, nc, exactInt(l * l), "Aceasta este aria pătratului cu această latură, nu perimetrul.");
    SLIP(c, nc, exactInt(2 * l), "Ai socotit doar două laturi. Figura are patru laturi congruente.");
    SLIP(c, nc, exactInt(3 * l), "Ai socotit doar trei laturi. Figura are patru laturi congruente.");
    SLIP(c, nc, exactInt(8 * l), "Ai socotit fiecare latură de două ori. Figura are doar patru laturi.");
    bad |= PR(ex.clue[0], "Toate laturile figurii sunt congruente. Câte laturi are ea?");
    bad |= PR(ex.clue[1], "Perimetrul este suma celor patru laturi congruente.");
    bad |= PR(ex.solution, "P = 4 · l = 4 · %d = %d cm.", l, ans);
  } else {  // mixed units: dm to cm
    int a = rng.range(6, 20), y = 5 * rng.range(1, 4);
    ans = 2 * (a + y);
    char dm[16];
    if (y % 10) snprintf(dm, sizeof dm, "%d,5", y / 10);
    else snprintf(dm, sizeof dm, "%d", y / 10);
    ex.fig = Figure{FIG_RECTANGLE, {4, a, y, 0}};
    bad |= PR(ex.text, "Un dreptunghi are lungimea de %d cm și lățimea de %s dm. Care este perimetrul lui, în centimetri?", a, dm);
    SLIP(c, nc, exactInt(2 * (a + y / 10)), "Ai adunat centimetri cu decimetri fără să transformi unitățile. Mai întâi exprimă lățimea în centimetri (1 dm = 10 cm).");
    SLIP(c, nc, exactInt(a + y), "Aceasta este suma a două laturi consecutive. Perimetrul însumează toate cele patru laturi.");
    SLIP(c, nc, exactInt(a * y), "Aceasta este produsul lungimilor, nu perimetrul.");
    SLIP(c, nc, exactInt(2 * a + y), "Ai socotit lățimea o singură dată. Laturile opuse sunt congruente, deci fiecare lungime apare de două ori.");
    bad |= PR(ex.clue[0], "Cele două laturi nu sunt exprimate în aceeași unitate de măsură. Ce faci înainte de a calcula?");
    bad |= PR(ex.clue[1], "Transformă decimetrii în centimetri, apoi adună de două ori suma celor două laturi consecutive.");
    bad |= PR(ex.solution, "Lățimea: %s dm = %d cm. P = 2 · (%d + %d) = 2 · %d = %d cm.", dm, y, a, y, a + y, ans);
  }
  return finish(ex, rng, bad, exactInt(ans), c, nc, " cm");
}

static bool genHeightFromArea(int level, Rng &rng, Exercise &ex) {
  int type = level <= 1 ? rng.range(0, 1) : rng.range(0, 2);
  bool bad = false;
  ex.kind = K_HEIGHT_FROM_AREA;
  ex.level = level <= 1 ? 1 : 2;
  Cand c[7];
  int nc = 0;
  Exact right;
  if (type == 0) {
    int a = rng.range(4, 12), h = rng.range(3, 9);
    int A = a * h;
    right = exactInt(h);
    ex.fig = Figure{FIG_NONE, {0, A, a, 0}};
    bad |= PR(ex.text, "Aria unui paralelogram este %d cm" SQ ", iar una dintre laturile lui are %d cm. Care este înălțimea corespunzătoare acestei laturi?", A, a);
    SLIP(c, nc, exactInt(A - a), "Ai scăzut latura din arie. Aria este produsul dintre latură și înălțime, deci înălțimea se află prin împărțire.");
    SLIP(c, nc, exactInt(A * a), "Ai înmulțit aria cu latura. Din A = latura · înălțimea, înălțimea se obține împărțind aria la latură.");
    SLIP(c, nc, exactFrac(A, 2 * a), "Ai împărțit și la doi, ca la triunghi. La paralelogram aria este baza înmulțită cu înălțimea, fără împărțire.");
    SLIP(c, nc, exactFrac(2 * A, a), "Ai înmulțit cu doi, ca la triunghi. La paralelogram aria este baza înmulțită cu înălțimea, fără alte factori.");
    bad |= PR(ex.clue[0], "Aria unui paralelogram este produsul dintre o latură și înălțimea corespunzătoare ei. Care factor lipsește?");
    bad |= PR(ex.clue[1], "Împarte aria la latura dată.");
    bad |= PR(ex.solution, "A = latura · h, deci h = A : latura = %d : %d = %d cm.", A, a, h);
  } else if (type == 1) {
    int b = rng.range(4, 14), h = rng.range(3, 12);
    if ((b * h) % 2) return false;
    int A = b * h / 2;
    right = exactInt(h);
    ex.fig = Figure{FIG_NONE, {1, A, b, 0}};
    bad |= PR(ex.text, "Aria unui triunghi este %d cm" SQ ", iar una dintre laturile lui are %d cm. Care este înălțimea corespunzătoare acestei laturi?", A, b);
    SLIP(c, nc, exactFrac(A, b), "Ai împărțit aria la latură, dar în triunghi aria este jumătate din produsul laturii cu înălțimea, deci înălțimea este mai mare.");
    SLIP(c, nc, exactInt(A - b), "Ai scăzut latura din arie. Aria este jumătate din produsul laturii cu înălțimea.");
    SLIP(c, nc, exactInt(A * b), "Ai înmulțit aria cu latura. Înălțimea se află împărțind, nu înmulțind.");
    SLIP(c, nc, exactFrac(4 * A, b), "Ai înmulțit cu patru. Din A = (latura · înălțimea) : 2 se obține dublul ariei împărțit la latură.");
    bad |= PR(ex.clue[0], "Aria triunghiului este jumătate din produsul laturii cu înălțimea corespunzătoare. Cum scoți înălțimea din această relație?");
    bad |= PR(ex.clue[1], "Dublează aria, apoi împarte la latura dată.");
    bad |= PR(ex.solution, "A = (latura · h) : 2, deci h = 2 · A : latura = 2 · %d : %d = %d cm.", A, b, h);
  } else {
    static const int T[][3] = {{3, 4, 5}, {5, 12, 13}, {6, 8, 10}, {8, 15, 17}, {9, 12, 15}, {7, 24, 25}, {12, 16, 20}};
    const int *t = T[rng.range(0, (int)(sizeof T / sizeof T[0]) - 1)];
    int c1 = t[0], c2 = t[1], hy = t[2];
    right = exactFrac(c1 * c2, hy);
    if (right.q == 1) return false;
    ex.fig = Figure{FIG_RIGHT_TRIANGLE, {c1, c2, 0, 2}};
    bad |= PR(ex.text, "Triunghiul ABC are unghiul A = 90°, cateta AB = %d cm, cateta AC = %d cm și ipotenuza BC = %d cm. Care este înălțimea dusă din A pe ipotenuză?", c1, c2, hy);
    SLIP(c, nc, exactFrac(c1 * c2, 2), "Aceasta este aria triunghiului, nu înălțimea.");
    SLIP(c, nc, exactFrac(hy, 2), "Aceasta este lungimea medianei duse din A (jumătate din ipotenuză), nu a înălțimii.");
    SLIP(c, nc, exactInt(c1 * c2), "Aceasta este produsul catetelor. Înălțimea se obține împărțind acest produs la ipotenuză.");
    SLIP(c, nc, exactFrac(c1 * c2, c1 + c2), "Ai împărțit produsul catetelor la suma lor. Înălțimea corespunzătoare ipotenuzei se obține împărțind la ipotenuză.");
    SLIP(c, nc, exactFrac(c1 + c2, 2), "Ai făcut media catetelor, care nu este înălțimea dusă pe ipotenuză.");
    bad |= PR(ex.clue[0], "Aria triunghiului dreptunghic se poate calcula în două feluri: cu catetele, sau cu ipotenuza și înălțimea dusă pe ea. Scrie-le egale.");
    bad |= PR(ex.clue[1], "Aria cu catetele este jumătate din produsul lor. Aria cu ipotenuza este jumătate din produsul dintre ipotenuză și înălțime.");
    char f[32];
    exactFormat(right, f, sizeof f);
    bad |= PR(ex.solution, "Aria se scrie în două feluri: (AB · AC) : 2 = (BC · h) : 2, deci h = (AB · AC) : BC = (%d · %d) : %d = %d : %d = %s cm.", c1, c2, hy, c1 * c2, hy, f);
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
    if (strcmp(ex.opt[i], CLASS_LABEL[code])) return bad(err, cap, "option text does not match its code");
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
