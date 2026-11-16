// Tutor7's logic: see tutor7_core.h.
#include "tutor7_internal.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

namespace t7 {

// ---------------------------------------------------------------- exact numbers

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

static Exact norm(int64_t p, int64_t r, int64_t q) {
  if (q < 0) {
    p = -p;
    q = -q;
  }
  int64_t g = gcd64(p, q);
  if (g > 1) {
    p /= g;
    q /= g;
  }
  if (p == 0) {
    r = 1;
    q = 1;
  }
  return Exact{(int32_t)p, (int32_t)r, (int32_t)q};
}

Exact exactNorm(int64_t p, int64_t r, int64_t q) { return norm(p, r, q); }
Exact exactInt(int32_t n) { return norm(n, 1, 1); }
Exact exactFrac(int32_t p, int32_t q) { return norm(p, 1, q); }

Exact exactSqrt(int64_t n) {
  if (n <= 0) return norm(0, 1, 1);
  int64_t out = 1;
  for (int64_t i = 2; i * i <= n; i++)
    while (n % (i * i) == 0) {
      n /= i * i;
      out *= i;
    }
  return norm(out, n, 1);
}

bool exactSame(const Exact &a, const Exact &b) { return a.p == b.p && a.r == b.r && a.q == b.q; }
double exactValue(const Exact &e) { return (double)e.p * sqrt((double)e.r) / (double)e.q; }

int exactFormat(const Exact &e, char *out, int cap) {
  int n;
  if (e.r == 1) {
    n = e.q == 1 ? snprintf(out, cap, "%d", e.p) : snprintf(out, cap, "%d/%d", e.p, e.q);
  } else {
    n = e.p == 1 ? snprintf(out, cap, "√%d", e.r) : snprintf(out, cap, "%d√%d", e.p, e.r);
    if (n >= 0 && n < cap && e.q != 1) {
      int m = snprintf(out + n, cap - n, "/%d", e.q);
      n = m < 0 ? m : n + m;
    }
  }
  return (n < 0 || n >= cap) ? -1 : n;
}

// ---------------------------------------------------------------- building blocks



// The right answer plus three of the slips, in a random order. False when
// there are not three distinct slips (then the caller tries another seed).
bool setOptions(Exercise &ex, Rng &rng, const Exact &right, Cand *c, int nc, const char *unit) {
  if (PR(ex.unit, "%s", unit)) return false;
  // shuffle the slips, keep the first three that differ from the answer and each other
  for (int i = nc - 1; i > 0; i--) {
    int j = rng.range(0, i);
    Cand t = c[i];
    c[i] = c[j];
    c[j] = t;
  }
  // A radical answer must not be the only radical on screen, or it could be found by its look alone:
  // keep at least one radical among the wrong options when there is one.
  if (right.r != 1) {
    for (int i = 0; i < nc; i++)
      if (c[i].v.r != 1 && fabs(exactValue(c[i].v) - exactValue(right)) >= 1e-9) {
        Cand t = c[0];
        c[0] = c[i];
        c[i] = t;
        break;
      }
  }
  Cand pick[3];
  int np = 0;
  for (int i = 0; i < nc && np < 3; i++) {
    bool dup = c[i].v.p <= 0 || fabs(exactValue(c[i].v) - exactValue(right)) < 1e-9;  // a length or an angle is positive
    for (int k = 0; k < np && !dup; k++) dup = fabs(exactValue(c[i].v) - exactValue(pick[k].v)) < 1e-9;
    if (!dup) pick[np++] = c[i];
  }
  if (np < 3) return false;
  Exact val[4] = {right, pick[0].v, pick[1].v, pick[2].v};
  const char *why[4] = {"", pick[0].why, pick[1].why, pick[2].why};
  int order[4] = {0, 1, 2, 3};
  for (int i = 3; i > 0; i--) {
    int j = rng.range(0, i);
    int t = order[i];
    order[i] = order[j];
    order[j] = t;
  }
  ex.nopt = 4;
  for (int i = 0; i < 4; i++) {
    int s = order[i];
    ex.value[i] = val[s];
    if (s == 0) ex.correct = i;
    char num[32];
    if (exactFormat(val[s], num, sizeof num) < 0) return false;
    if (PR(ex.opt[i], "%s%s", num, unit)) return false;
    if (PR(ex.why[i], "%s", why[s])) return false;
  }
  return true;
}

// Primitive Pythagorean triples with a hypotenuse up to 85: the legs a, b and the hypotenuse c.
static const int PRIMS[][3] = {{3, 4, 5},   {5, 12, 13}, {8, 15, 17},  {7, 24, 25},  {20, 21, 29},
                               {12, 35, 37}, {9, 40, 41}, {28, 45, 53}, {11, 60, 61}, {16, 63, 65},
                               {33, 56, 65}, {48, 55, 73}, {13, 84, 85}, {36, 77, 85}};
static const int NPRIMS = sizeof(PRIMS) / sizeof(PRIMS[0]);

static void rootText(const Exact &e, int64_t n, char *out, int cap) {
  // "√80 = 4√5" or just "√7" when the root is already as simple as it gets
  char f[32];
  exactFormat(e, f, sizeof f);
  if (e.r == n && e.p == 1 && e.q == 1) snprintf(out, cap, RT "%lld", (long long)n);
  else snprintf(out, cap, RT "%lld = %s", (long long)n, f);
}

// ---------------------------------------------------------------- generators

static bool genHypotenuse(int level, Rng &rng, Exercise &ex) {
  int ab, ac;
  if (level <= 1) {
    const int *t = PRIMS[rng.range(0, NPRIMS - 1)];
    int k = rng.range(1, t[2] <= 13 ? 4 : (t[2] <= 29 ? 2 : 1));
    ab = t[0] * k;
    ac = t[1] * k;
  } else {
    ab = rng.range(2, 15);
    ac = rng.range(2, 15);
    if (ab == ac) return false;
  }
  if (rng.range(0, 1)) {
    int t = ab;
    ab = ac;
    ac = t;
  }
  int64_t n = sq(ab) + sq(ac);
  Exact right = exactSqrt(n);
  bool integer = right.r == 1;
  if (level <= 1 ? !integer : integer) return false;

  bool bad = false;
  ex.kind = K_HYPOTENUSE;
  ex.level = level <= 1 ? 1 : 2;
  ex.fig = Figure{FIG_RIGHT_TRIANGLE, {ab, ac, 0}};
  bad |= PR(ex.text, "Î" "ntr-un triunghi dreptunghic ABC, cu unghiul A = 90°, avem AB = %d cm și AC = %d cm. "
                     "Cât este lungimea ipotenuzei BC?", ab, ac);
  Cand c[7];
  int nc = 0;
  SLIP(c, nc, exactSqrt(ab + ac),
       "Ai adunat lungimile catetelor și ai extras rădăcina. Teorema lui Pitagora adună pătratele catetelor, nu lungimile lor: BC" SQ " = AB" SQ " + AC" SQ ".");
  SLIP(c, nc, exactSqrt(sq(ab) + ac),
       "Ai ridicat la pătrat o singură catetă. Ambele catete se ridică la pătrat: BC" SQ " = AB" SQ " + AC" SQ ".");
  if (ab != ac)
    SLIP(c, nc, exactSqrt(llabs(sq(ab) - sq(ac))),
         "Ai scăzut pătratele. La ipotenuză se adună pătratele catetelor: BC" SQ " = AB" SQ " + AC" SQ ".");
  SLIP(c, nc, exactInt((int32_t)n),
       "Ai uitat să extragi rădăcina pătrată. Relația dă BC" SQ ", nu BC: BC = " RT "(AB" SQ " + AC" SQ ").");
  SLIP(c, nc, exactInt(ab + ac),
       "Ai adunat lungimile catetelor. Teorema lui Pitagora folosește pătratele lor: BC" SQ " = AB" SQ " + AC" SQ ".");
  SLIP(c, nc, exactInt(ab > ac ? ab : ac),
       "Ai luat cateta mai mare. Ipotenuza este mereu mai lungă decât oricare dintre catete.");
  bad |= PR(ex.clue[0], "Ipotenuza este latura opusă unghiului drept. Care latură este aceasta în triunghiul ABC?");
  bad |= PR(ex.clue[1], "Scrie relația lui Pitagora pentru acest triunghi: BC" SQ " = AB" SQ " + AC" SQ ". Înlocuiește AB și AC cu valorile date și calculează.");
  char rt[64], f[32];
  rootText(right, n, rt, sizeof rt);
  exactFormat(right, f, sizeof f);
  bad |= PR(ex.solution, "BC" SQ " = AB" SQ " + AC" SQ " = %d" SQ " + %d" SQ " = %d + %d = %lld, deci BC = %s cm.", ab, ac, ab * ab, ac * ac,
            (long long)n, rt);
  return !bad && setOptions(ex, rng, right, c, nc, " cm");
}

static bool genLeg(int level, Rng &rng, Exercise &ex) {
  int bc, ab;
  if (level <= 1) {
    const int *t = PRIMS[rng.range(0, NPRIMS - 1)];
    int k = rng.range(1, t[2] <= 13 ? 4 : (t[2] <= 29 ? 2 : 1));
    bc = t[2] * k;
    ab = t[rng.range(0, 1)] * k;
  } else {
    bc = rng.range(5, 20);
    ab = rng.range(2, bc - 1);
  }
  int64_t n = sq(bc) - sq(ab);
  if (n <= 0) return false;
  Exact right = exactSqrt(n);
  bool integer = right.r == 1;
  if (level <= 1 ? !integer : integer) return false;
  if (level <= 1 && right.p == ab) return false;  // the unknown leg must differ from the given one

  bool bad = false;
  ex.kind = K_LEG;
  ex.level = level <= 1 ? 1 : 2;
  ex.fig = Figure{FIG_RIGHT_TRIANGLE, {ab, 0, bc}};
  bad |= PR(ex.text, "Î" "ntr-un triunghi dreptunghic ABC, cu unghiul A = 90°, ipotenuza BC = %d cm și cateta AB = %d cm. "
                     "Cât este lungimea catetei AC?", bc, ab);
  Cand c[7];
  int nc = 0;
  SLIP(c, nc, exactSqrt(bc - ab),
       "Ai scăzut lungimile și ai extras rădăcina. Teorema lui Pitagora lucrează cu pătratele: AC" SQ " = BC" SQ " - AB" SQ ".");
  SLIP(c, nc, exactSqrt(sq(bc) - ab),
       "Ai ridicat la pătrat doar ipotenuza. Ambele laturi se ridică la pătrat: AC" SQ " = BC" SQ " - AB" SQ ".");
  SLIP(c, nc, exactSqrt(sq(bc) + sq(ab)),
       "Ai adunat pătratele, ca la ipotenuză. Ipotenuza este cea mai lungă latură, iar BC" SQ " = AB" SQ " + AC" SQ ", deci AC" SQ " = BC" SQ " - AB" SQ ".");
  SLIP(c, nc, exactInt((int32_t)n),
       "Ai uitat să extragi rădăcina pătrată. Diferența pătratelor este AC" SQ ", nu AC.");
  SLIP(c, nc, exactInt(bc - ab),
       "Ai scăzut lungimile. Teorema lui Pitagora folosește pătratele: AC" SQ " = BC" SQ " - AB" SQ ".");
  SLIP(c, nc, exactInt(bc),
       "Aceasta este lungimea ipotenuzei. Cateta AC este mai scurtă decât ipotenuza.");
  bad |= PR(ex.clue[0], "Ipotenuza este BC, latura opusă unghiului drept. Din relația BC" SQ " = AB" SQ " + AC" SQ " scoate AC" SQ ".");
  bad |= PR(ex.clue[1], "AC" SQ " = BC" SQ " - AB" SQ ". Înlocuiește valorile, calculează diferența, apoi extrage rădăcina pătrată.");
  char rt[64];
  rootText(right, n, rt, sizeof rt);
  bad |= PR(ex.solution, "AC" SQ " = BC" SQ " - AB" SQ " = %d" SQ " - %d" SQ " = %d - %d = %lld, deci AC = %s cm.", bc, ab, bc * bc, ab * ab,
            (long long)n, rt);
  return !bad && setOptions(ex, rng, right, c, nc, " cm");
}

// Three sides: right-angled or not. Options carry a code in value[].p:
// 0 "no", 1 "yes, the right angle is opposite the longest side", 2 "yes, opposite the shortest".
static bool genConverse(Rng &rng, Exercise &ex) {
  const int *t = PRIMS[rng.range(0, NPRIMS - 1)];
  int k = rng.range(1, t[2] <= 13 ? 4 : (t[2] <= 29 ? 2 : 1));
  int x = t[0] * k, y = t[1] * k, z = t[2] * k;
  if (x > y) {
    int tmp = x;
    x = y;
    y = tmp;
  }
  bool right = rng.range(0, 1) == 1;
  if (!right) {
    int d = rng.range(1, 3);
    z += rng.range(0, 1) ? d : -d;
    if (z <= y || x + y <= z || sq(x) + sq(y) == sq(z)) return false;  // still a triangle, still not right
  }
  int sides[3] = {x, y, z};  // shown in a random order
  for (int i = 2; i > 0; i--) {
    int j = rng.range(0, i);
    int tmp = sides[i];
    sides[i] = sides[j];
    sides[j] = tmp;
  }
  bool bad = false;
  ex.kind = K_CONVERSE;
  ex.level = 3;
  ex.fig = Figure{FIG_TRIANGLE, {sides[0], sides[1], sides[2]}};
  bad |= PR(ex.text, "Un triunghi are laturile de %d cm, %d cm și %d cm. Este triunghiul dreptunghic?", sides[0], sides[1], sides[2]);

  char yes1[80], yes2[80], no[80];
  bad |= PR(yes1, "Da, unghiul drept este opus laturii de %d cm", z);
  bad |= PR(yes2, "Da, unghiul drept este opus laturii de %d cm", x);
  bad |= PR(no, "Nu, nu este dreptunghic");
  const char *label[3] = {no, yes1, yes2};
  int code[3] = {0, 1, 2};
  for (int i = 2; i > 0; i--) {
    int j = rng.range(0, i);
    int tmp = code[i];
    code[i] = code[j];
    code[j] = tmp;
  }
  ex.nopt = 3;
  for (int i = 0; i < 3; i++) {
    int cd = code[i];
    ex.value[i] = exactInt(cd);
    bad |= PR(ex.opt[i], "%s", label[cd]);
    ex.why[i][0] = 0;
    bool isRight = cd == (right ? 1 : 0);
    if (isRight) {
      ex.correct = i;
      continue;
    }
    if (right) {
      if (cd == 2)
        bad |= PR(ex.why[i], "Unghiul drept este mereu opus celei mai lungi laturi (ipotenuza), nu celei mai scurte.");
      else
        bad |= PR(ex.why[i], "Verifică: %d" SQ " + %d" SQ " = %d + %d = %d și %d" SQ " = %d. Compară cele două rezultate: ce spune reciproca teoremei lui Pitagora într-un astfel de caz?",
                  x, y, x * x, y * y, x * x + y * y, z, z * z);
    } else {
      if (cd == 1)
        bad |= PR(ex.why[i], "Verifică: %d" SQ " + %d" SQ " = %d, dar %d" SQ " = %d. Valorile nu sunt egale, deci unghiul nu este drept.", x, y,
                  x * x + y * y, z, z * z);
      else
        bad |= PR(ex.why[i], "Unghiul drept ar fi opus celei mai lungi laturi, nu celei mai scurte. Iar %d" SQ " + %d" SQ " = %d, dar %d" SQ " = %d: valorile nu sunt egale.",
                  x, y, x * x + y * y, z, z * z);
    }
  }
  bad |= PR(ex.clue[0], "Pentru reciproca teoremei lui Pitagora, încearcă cu latura cea mai lungă drept ipotenuză. Care este ea?");
  bad |= PR(ex.clue[1], "Calculează suma pătratelor celorlalte două laturi și compară-o cu pătratul laturii celei mai lungi.");
  if (right)
    bad |= PR(ex.solution, "Cea mai lungă latură este %d cm. %d" SQ " + %d" SQ " = %d + %d = %d și %d" SQ " = %d. Valorile sunt egale, deci triunghiul este dreptunghic (unghiul drept este opus laturii de %d cm).",
              z, x, y, x * x, y * y, x * x + y * y, z, z * z, z);
  else
    bad |= PR(ex.solution, "Cea mai lungă latură este %d cm. %d" SQ " + %d" SQ " = %d + %d = %d, dar %d" SQ " = %d. Valorile sunt diferite, deci triunghiul nu este dreptunghic.",
              z, x, y, x * x, y * y, x * x + y * y, z, z * z);
  return !bad;
}

static bool genSquareDiagonal(Rng &rng, Exercise &ex) {
  int l = rng.range(2, 15);
  Exact right = exactSqrt(2 * sq(l));
  bool bad = false;
  ex.kind = K_SQUARE_DIAGONAL;
  ex.level = 3;
  ex.fig = Figure{FIG_SQUARE_DIAGONAL, {l, 0, 0}};
  bad |= PR(ex.text, "Latura unui pătrat este de %d cm. Cât este lungimea diagonalei pătratului?", l);
  Cand c[4];
  int nc = 0;
  SLIP(c, nc, exactInt(2 * l),
       "Ai dublat latura. Diagonala este ipotenuza unui triunghi dreptunghic isoscel cu catetele de %d cm: d" SQ " = %d" SQ " + %d" SQ ", deci nu se obține prin dublarea laturii.", l, l, l);
  {
    Exact e = norm(l, 3, 1);
    SLIP(c, nc, e,
         RT "3 apare la înălțimea triunghiului echilateral, nu aici. Diagonala este ipotenuza unui triunghi dreptunghic isoscel cu catetele de %d cm: d" SQ " = %d" SQ " + %d" SQ ".", l, l, l);
  }
  SLIP(c, nc, norm(l, 2, 2),
       "Aceasta este jumătate din diagonală. Diagonala întreagă este ipotenuza: d" SQ " = %d" SQ " + %d" SQ ", nu jumătate din ea.", l, l);
  SLIP(c, nc, exactInt(l * l),
       "Aceasta este aria pătratului, nu lungimea diagonalei. Diagonala se află din teorema lui Pitagora: d" SQ " = %d" SQ " + %d" SQ ".", l, l);
  bad |= PR(ex.clue[0], "Diagonala împarte pătratul în două triunghiuri dreptunghice. Care sunt catetele și care este ipotenuza într-unul dintre ele?");
  bad |= PR(ex.clue[1], "Catetele sunt laturile pătratului, de %d cm fiecare, iar diagonala d este ipotenuza. Aplică teorema lui Pitagora: d" SQ " = %d" SQ " + %d" SQ ".", l, l, l);
  char rt[64];
  rootText(right, 2 * sq(l), rt, sizeof rt);
  bad |= PR(ex.solution, "Diagonala este ipotenuza unui triunghi dreptunghic isoscel cu catetele de %d cm: d" SQ " = %d" SQ " + %d" SQ " = %d + %d = %d, deci d = %s cm.",
            l, l, l, l * l, l * l, 2 * l * l, rt);
  return !bad && setOptions(ex, rng, right, c, nc, " cm");
}

static bool genEquilateralHeight(Rng &rng, Exercise &ex) {
  int h2 = rng.range(2, 16);  // half the side
  int l = 2 * h2;
  Exact right = exactSqrt(3 * sq(h2));
  bool bad = false;
  ex.kind = K_EQUILATERAL_HEIGHT;
  ex.level = 3;
  ex.fig = Figure{FIG_EQUILATERAL_HEIGHT, {l, 0, 0}};
  bad |= PR(ex.text, "Un triunghi echilateral ABC are latura de %d cm. Cât este lungimea înălțimii AD?", l);
  Cand c[4];
  int nc = 0;
  SLIP(c, nc, norm(l, 3, 1),
       "Ai uitat să împarți latura la 2. În triunghiul dreptunghic ABD, cateta BD este jumătate din BC: BD = %d cm, apoi AD" SQ " = AB" SQ " - BD" SQ ".", h2);
  SLIP(c, nc, exactInt(h2),
       "Aceasta este lungimea lui BD, jumătate din latură, nu a înălțimii AD. Înălțimea AD este cealaltă catetă a triunghiului ABD.");
  SLIP(c, nc, norm(l, 3, 4),
       "Ai împărțit la 2 de două ori. Latura se înjumătățește o singură dată (BD = %d cm); apoi AD" SQ " = AB" SQ " - BD" SQ " = %d" SQ " - %d" SQ ".", h2, l, h2);
  SLIP(c, nc, norm(h2, 2, 1),
       "Aici triunghiul este echilateral, nu pătrat. Calculează cu atenție AD" SQ " = AB" SQ " - BD" SQ " = %d" SQ " - %d" SQ ", apoi extrage rădăcina pătrată.", l, h2);
  bad |= PR(ex.clue[0], "Înălțimea AD este perpendiculară pe BC, iar D este mijlocul lui BC. Ce fel de triunghi este ABD?");
  bad |= PR(ex.clue[1], "În triunghiul dreptunghic ABD, ipotenuza este AB = %d cm și o catetă este BD = %d cm. Aplică teorema lui Pitagora pentru a afla AD.", l, h2);
  char rt[64];
  rootText(right, 3 * sq(h2), rt, sizeof rt);
  bad |= PR(ex.solution, "D este mijlocul lui BC, deci BD = %d cm. În triunghiul dreptunghic ABD: AD" SQ " = AB" SQ " - BD" SQ " = %d" SQ " - %d" SQ " = %d - %d = %d, deci AD = %s cm.",
            h2, l, h2, l * l, h2 * h2, 3 * h2 * h2, rt);
  return !bad && setOptions(ex, rng, right, c, nc, " cm");
}

bool generate(Kind kind, int level, Rng &rng, Exercise &ex) {
  memset(&ex, 0, sizeof ex);
  switch (kind) {
    case K_HYPOTENUSE: return genHypotenuse(level, rng, ex);
    case K_LEG: return genLeg(level, rng, ex);
    case K_CONVERSE: return genConverse(rng, ex);
    case K_SQUARE_DIAGONAL: return genSquareDiagonal(rng, ex);
    case K_EQUILATERAL_HEIGHT: return genEquilateralHeight(rng, ex);
    default: return u4Generate(kind, level, rng, ex);
  }
}

// ---------------------------------------------------------------- verification

static bool validUtf8(const char *s) {
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

// token occurs in text and is not just a piece of a longer number
bool hasToken(const char *text, const char *token) {
  size_t n = strlen(token);
  if (!n) return false;
  for (const char *p = text; (p = strstr(p, token)); p++) {
    char before = p == text ? ' ' : p[-1];
    char after = p[n];
    bool bDigit = (before >= '0' && before <= '9') || before == '/';
    bool aDigit = (after >= '0' && after <= '9') || after == '/';
    if (!bDigit && !aDigit) return true;
  }
  return false;
}

static bool fail(char *err, int cap, const char *msg) {
  if (err && cap > 0) snprintf(err, cap, "%s", msg);
  return false;
}

bool verify(const Exercise &ex, char *err, int cap) {
  if (ex.nopt < 3 || ex.nopt > MAX_OPTIONS) return fail(err, cap, "number of options");
  if (ex.correct < 0 || ex.correct >= ex.nopt) return fail(err, cap, "correct index");
  if (!ex.text[0] || !ex.clue[0][0] || !ex.clue[1][0] || !ex.solution[0]) return fail(err, cap, "empty text, clue or solution");
  if (!validUtf8(ex.text) || !validUtf8(ex.clue[0]) || !validUtf8(ex.clue[1]) || !validUtf8(ex.solution)) return fail(err, cap, "bad UTF-8");
  for (int i = 0; i < ex.nopt; i++) {
    if (!validUtf8(ex.opt[i]) || !validUtf8(ex.why[i])) return fail(err, cap, "bad UTF-8 in an option");
    if (!ex.opt[i][0]) return fail(err, cap, "empty option");
    if ((i == ex.correct) != (ex.why[i][0] == 0)) return fail(err, cap, "reason missing on a wrong option, or present on the right one");
    for (int j = 0; j < i; j++)
      if (!strcmp(ex.opt[i], ex.opt[j])) return fail(err, cap, "two options say the same");
  }

  const Figure &f = ex.fig;
  char token[48] = "";
  char tokens2[2][48] = {"", ""};  // numbers a clue must not give away (converse)
  if (ex.kind == K_CONVERSE) {
    int s[3] = {f.v[0], f.v[1], f.v[2]};
    for (int i = 0; i < 3; i++)
      if (s[i] <= 0) return fail(err, cap, "side not positive");
    // sort
    for (int i = 0; i < 3; i++)
      for (int j = i + 1; j < 3; j++)
        if (s[j] < s[i]) {
          int t = s[i];
          s[i] = s[j];
          s[j] = t;
        }
    if (s[0] + s[1] <= s[2]) return fail(err, cap, "not a triangle");
    bool isRight = (int64_t)s[0] * s[0] + (int64_t)s[1] * s[1] == (int64_t)s[2] * s[2];
    int want = isRight ? 1 : 0;
    int found = 0;
    for (int i = 0; i < ex.nopt; i++) {
      if (ex.value[i].p == want) {
        found++;
        if (i != ex.correct) return fail(err, cap, "the right option is marked wrong");
      }
    }
    if (found != 1) return fail(err, cap, "no unique right option");
    // an option that says "yes, opposite side N" must name the longest side (code 1) or the shortest (code 2)
    for (int i = 0; i < ex.nopt; i++) {
      char want1[40];
      int code = ex.value[i].p;
      if (code == 1) snprintf(want1, sizeof want1, "%d cm", s[2]);
      else if (code == 2) snprintf(want1, sizeof want1, "%d cm", s[0]);
      else continue;
      if (!strstr(ex.opt[i], want1)) return fail(err, cap, "option names the wrong side");
    }
    snprintf(tokens2[0], sizeof tokens2[0], "%d", s[0] * s[0] + s[1] * s[1]);
    snprintf(tokens2[1], sizeof tokens2[1], "%d", s[2] * s[2]);
  } else if (ex.kind == K_CLASSIFY) {
    if (!u4VerifyClassify(ex, err, cap)) return false;
  } else {
    double expect = 0;
    const char *unit = ex.unit;
    if (ex.kind >= K_Q_FOURTH_ANGLE && ex.kind < K_COUNT) {
      const char *want = "";
      if (!u4Expect(ex, expect, want, err, cap)) return false;
      if (strcmp(want, ex.unit)) return fail(err, cap, "the unit of the options is not the kind's unit");
    } else if (ex.kind == K_HYPOTENUSE) {
      if (f.kind != FIG_RIGHT_TRIANGLE || f.v[0] <= 0 || f.v[1] <= 0 || f.v[2] != 0) return fail(err, cap, "hypotenuse figure");
      expect = sqrt((double)f.v[0] * f.v[0] + (double)f.v[1] * f.v[1]);
    } else if (ex.kind == K_LEG) {
      if (f.kind != FIG_RIGHT_TRIANGLE || f.v[0] <= 0 || f.v[1] != 0 || f.v[2] <= f.v[0]) return fail(err, cap, "leg figure: the hypotenuse must be the longest");
      expect = sqrt((double)f.v[2] * f.v[2] - (double)f.v[0] * f.v[0]);
    } else if (ex.kind == K_SQUARE_DIAGONAL) {
      if (f.kind != FIG_SQUARE_DIAGONAL || f.v[0] <= 0) return fail(err, cap, "square figure");
      expect = f.v[0] * sqrt(2.0);
    } else if (ex.kind == K_EQUILATERAL_HEIGHT) {
      if (f.kind != FIG_EQUILATERAL_HEIGHT || f.v[0] <= 0) return fail(err, cap, "equilateral figure");
      expect = f.v[0] * sqrt(3.0) / 2;
    } else {
      return fail(err, cap, "unknown kind");
    }
    if (fabs(exactValue(ex.value[ex.correct]) - expect) > 1e-9) return fail(err, cap, "the key does not match the independent value");
    for (int i = 0; i < ex.nopt; i++) {
      char num[40], full[48];
      if (exactFormat(ex.value[i], num, sizeof num) < 0) return fail(err, cap, "format");
      snprintf(full, sizeof full, "%s%s", num, unit);
      if (strcmp(full, ex.opt[i])) return fail(err, cap, "option text does not match its number");
      if (i != ex.correct && fabs(exactValue(ex.value[i]) - expect) < 1e-9) return fail(err, cap, "a wrong option is also right");
      if (ex.value[i].r < 1 || ex.value[i].q < 1) return fail(err, cap, "not in lowest terms");
    }
    exactFormat(ex.value[ex.correct], token, sizeof token);
    if (!hasToken(ex.solution, token)) return fail(err, cap, "the solution does not state the answer");
    // the answer must not already be a given number, or a clue naming that number would look like a leak
    // (only the Pythagoras kinds: Unit 4's clues and explanations carry no numbers, and a rectangle's BD is its AC)
    for (int i = 0; i < 3 && ex.kind <= K_EQUILATERAL_HEIGHT; i++) {
      char g[24];
      snprintf(g, sizeof g, "%d", f.v[i]);
      if (f.v[i] && !strcmp(g, token)) return fail(err, cap, "the answer equals a given length");
    }
  }
  // an explanation of a wrong choice must not hand over the right answer: the student may try again
  for (int i = 0; i < ex.nopt; i++)
    if (i != ex.correct && token[0] && hasToken(ex.why[i], token)) return fail(err, cap, "a wrong-answer explanation gives the answer away");
  for (int c = 0; c < 2; c++) {
    if (token[0] && hasToken(ex.clue[c], token)) return fail(err, cap, "a clue gives the answer away");
    for (int k = 0; k < 2; k++)
      if (tokens2[k][0] && hasToken(ex.clue[c], tokens2[k])) return fail(err, cap, "a clue gives the calculation away");
  }
  return true;
}

}  // namespace t7
