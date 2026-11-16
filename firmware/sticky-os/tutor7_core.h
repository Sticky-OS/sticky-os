// Tutor7's logic, apart from how it looks (tutor7.ino draws it and takes the
// taps): exact numbers, and the exercise generators with their solvers.
//
// No Arduino here, so the same code runs on a computer (firmware/test/tutor7
// generates thousands of exercises and checks each one). Every exercise is
// made from numbers by a solver, never typed by hand: the key is computed,
// the wrong options are the typical slips (each with its reason), and
// tutor7Verify() re-derives the answer a second way before the exercise is
// shown. An exercise that can't be verified is never shown.
//
// Text is UTF-8, in Romanian. A square root is the character U+221A, which
// the fonts don't have: the screen draws it itself.
#pragma once
#include <stdint.h>

namespace t7 {

// A small random generator of our own, so a test can repeat any exercise.
struct Rng {
  uint32_t s;
  explicit Rng(uint32_t seed) : s(seed ? seed : 0x9E3779B9u) {}
  uint32_t next() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
  }
  int range(int lo, int hi) { return lo + (int)(next() % (uint32_t)(hi - lo + 1)); }  // both included
};

// ---------------------------------------------------------------- exact numbers

// p * sqrt(r) / q in lowest terms: r square-free (r == 1: a plain fraction),
// q > 0, gcd(p, q) = 1.
struct Exact {
  int32_t p, r, q;
};
Exact exactInt(int32_t n);
Exact exactFrac(int32_t p, int32_t q);
Exact exactSqrt(int64_t n);  // sqrt(n) simplified: sqrt(32) = 4 sqrt(2)
bool exactSame(const Exact &a, const Exact &b);
double exactValue(const Exact &e);
// "13", "5/2", "4√2", "9√3/2". Returns the length, or -1 when cap is too small.
int exactFormat(const Exact &e, char *out, int cap);

// ---------------------------------------------------------------- exercises

enum Kind : uint8_t {
  K_HYPOTENUSE,  // the legs are given, find the hypotenuse
  K_LEG,         // the hypotenuse and a leg are given, find the other leg
  K_CONVERSE,    // three sides: is the triangle right-angled?
  K_SQUARE_DIAGONAL,
  K_EQUILATERAL_HEIGHT,
  // Unit 4, the quadrilateral
  K_Q_FOURTH_ANGLE,   // three angles of a convex quadrilateral: the fourth
  K_Q_PROPORTIONAL,   // four angles in a given ratio: the largest
  K_PAR_ANGLE,        // a parallelogram's angles
  K_PAR_PERIMETER,    // a parallelogram's perimeter, or its other side from the perimeter
  K_PAR_DIAGONAL,     // the diagonals bisect each other
  K_CLASSIFY,         // which figure do these properties guarantee?
  K_MIDLINE,          // a triangle's midline
  K_MEDIAN_TRIANGLE,  // the perimeter of the triangle made by the three midlines
  K_CENTROID,         // the centroid divides a median 2 : 1
  K_RECT_DIAGONAL,    // a rectangle's diagonals
  K_RECT_ANGLE,       // the angle between a rectangle's diagonal and its side
  K_RHOMBUS_ANGLE,    // a rhombus's angles from the angle its diagonal makes with a side
  K_RHOMBUS_SIDE,     // a rhombus's side or perimeter from its diagonals
  K_SQUARE_SIDE,      // a square's side from its diagonal
  K_TRAP_ANGLE,       // a trapezoid's angles
  K_TRAP_MIDLINE,     // a trapezoid's midline and the segment between the diagonals
  K_TRAP_HEIGHT,      // an isosceles trapezoid's height
  K_AREA,             // areas of the parallelogram, triangle, rhombus, trapezoid, right triangle
  K_PERIMETER,        // perimeters
  K_HEIGHT_FROM_AREA, // a height from the area
  K_COUNT
};

// What the screen draws, from the exercise's own numbers. 0: the length is
// the unknown (drawn as ?). Right triangle: A = 90 degrees, v = AB, AC, BC.
// FIG_RIGHT_NAMED is for the theory pages: the sides are named (AB, AC, BC), not measured.
enum FigureKind : uint8_t {
  FIG_RIGHT_TRIANGLE, FIG_TRIANGLE, FIG_SQUARE_DIAGONAL, FIG_EQUILATERAL_HEIGHT, FIG_RIGHT_NAMED,
  // Unit 4: drawn with the vertices named, the numbers are in the question
  FIG_NONE, FIG_QUAD, FIG_PARALLELOGRAM, FIG_RECTANGLE, FIG_RHOMBUS, FIG_TRAPEZOID, FIG_TRAPEZOID_ISOSCELES, FIG_MIDLINE, FIG_MEDIANS,
  FIG_COUNT
};
struct Figure {
  FigureKind kind;
  int32_t v[4];  // the givens; what they mean depends on the kind (see u4Expect)
};

static const int MAX_OPTIONS = 4;
struct Exercise {
  Kind kind;
  uint8_t level;  // 1 integers, 2 radicals, 3 the converse and special cases
  Figure fig;
  char unit[8];    // after each option's number: " cm", "°", " cm²"
  char text[240];  // the question
  int nopt;
  int correct;                 // index into opt
  char opt[MAX_OPTIONS][64];   // what the buttons say
  Exact value[MAX_OPTIONS];    // the number behind an option (kind-specific for K_CONVERSE: see verify)
  char why[MAX_OPTIONS][200];  // why an option is wrong (empty for the right one)
  char clue[2][200];           // a nudge, then a stronger one; never the answer
  char solution[400];          // the worked solution, shown after the answer or on "give up"
};

// Fills ex for the kind and level; false when it could not make a valid one
// (never shown: the caller picks another seed).
bool generate(Kind kind, int level, Rng &rng, Exercise &ex);

// Re-derives everything a second way and checks the exercise is well-posed:
// the figure exists, there is exactly one right option, every wrong option
// differs from it, the strings match the numbers, the clues don't give the
// answer away. err gets the first problem found.
bool verify(const Exercise &ex, char *err, int cap);

}  // namespace t7
