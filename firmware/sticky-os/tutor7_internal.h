// Shared by tutor7_core.cpp and tutor7_u4.cpp (not for the sketch's other tabs).
#pragma once
#include "tutor7_core.h"
#include "tutor7_text.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SQ "\xC2\xB2"  // the superscript two
#define RT "\xE2\x88\x9A"
#define DEG "\xC2\xB0"
#define PI_ "\xCF\x80"  // the Greek small letter pi: the screen draws it itself (the fonts have none)

// An exercise's words, in its language: PR(ex.text, TX(TX_HYP_Q), ab, ac). Every generator's exercise is `ex`.
#define TX(id) tx(ex.lang, (id))

#include <stdarg.h>

// snprintf into a fixed buffer; true when it did not fit. The format is a table's (TX), not a literal: a
// function of our own, so the compiler doesn't flag every format without arguments.
inline bool prFits(char *dst, size_t cap, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(dst, cap, fmt, ap);
  va_end(ap);
  return n < 0 || n >= (int)cap;
}
#define PR(dst, ...) prFits((dst), sizeof(dst), __VA_ARGS__)

namespace t7 {

inline int64_t sq(int64_t x) { return x * x; }

// A generator gathers its slips in ex.slip (Cand *c = ex.slip), never in a local array: see Exercise.
typedef Slip Cand;

// Adds a typical slip: its value, and why it is wrong.
#define SLIP(list, n, value, ...)                      \
  do {                                                 \
    if ((n) >= MAX_SLIPS) {                            \
      bad = true;                                      \
      break;                                           \
    }                                                  \
    (list)[n].v = (value);                             \
    if (PR((list)[n].why, __VA_ARGS__)) bad = true;    \
    (n)++;                                             \
  } while (0)

Exact exactNorm(int64_t p, int64_t r, int64_t q);
bool setOptions(Exercise &ex, Rng &rng, const Exact &right, Cand *c, int nc, const char *unit);
bool hasToken(const char *text, const char *token);

// Unit 4
bool u4Generate(Kind kind, int level, Rng &rng, Exercise &ex);
// The answer a kind's givens lead to, worked out again from ex.fig.v alone. False (and err) when they make no sense.
bool u4Expect(const Exercise &ex, double &expect, const char *&unit, char *err, int cap);
bool u4VerifyClassify(const Exercise &ex, char *err, int cap);

// Unit 5 (tutor7_u5.cpp) and Unit 6 (tutor7_u6.cpp): the same contract as Unit 4's. A generator fails (false)
// when the numbers it drew do not make a good exercise; the caller draws again.
bool u5Generate(Kind kind, int level, Rng &rng, Exercise &ex);
bool u5Expect(const Exercise &ex, double &expect, const char *&unit, char *err, int cap);
bool u6Generate(Kind kind, int level, Rng &rng, Exercise &ex);
// K_THALES_CONVERSE and K_SIM_CRITERION are choices (the options are statements, not numbers): they check
// themselves in u6VerifyChoice(); the others give their answer through u6Expect() like Unit 4's.
bool u6Expect(const Exercise &ex, double &expect, const char *&unit, char *err, int cap);
bool u6VerifyChoice(const Exercise &ex, char *err, int cap);

}  // namespace t7
