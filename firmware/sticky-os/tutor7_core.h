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
// Text is UTF-8, in the language the exercise is made in (ex.lang, a Lang of
// i18n.h): the words come from tutor7_text.h's tables, the numbers from the
// generator. Nothing a generator decides depends on the language (the random
// numbers, the options and their order, the key, the figure): the same seed
// makes the same exercise in every language. A square root is the character
// U+221A, which the fonts don't have: the screen draws it itself.
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
  // Unit 5, the circle (tutor7_u5.cpp). Lengths of circles are in multiples of pi: the option "12π cm" is the number 12 with unit "π cm".
  K_ARC_CENTRAL,      // arcs and central angles: minor and major arc, the arcs round the circle
  K_CHORD_DISTANCE,   // a chord, its distance from the centre and the radius (d² + (c/2)² = r²)
  K_INSCRIBED,        // an inscribed angle and its arc
  K_CYCLIC_QUAD,      // the angles of a quadrilateral inscribed in a circle
  K_TANGENT_LENGTH,   // the tangent segment from an outside point (right angle at the point of tangency)
  K_CIRCUMSCRIBED,    // a quadrilateral round a circle: the sums of opposite sides are equal
  K_INCIRCLE_AREA,    // a triangle's area from its semiperimeter and the radius of the circle inside it
  K_REGULAR_POLYGON,  // a regular polygon's central angle, its angle, its number of sides
  K_CIRCUMFERENCE,    // the length of a circle
  K_DISC_AREA,        // the area of a disc
  K_ARC_SECTOR,       // the length of an arc, the area of a sector
  K_ANNULUS,          // the area of a ring between two circles
  // Unit 6, similarity (tutor7_u6.cpp)
  K_RATIO,            // the ratio of two segments (units!)
  K_DIVIDE_SEGMENT,   // a point dividing a segment in a given ratio; parts proportional to numbers
  K_THALES,           // Thales's theorem in a triangle: a segment from the proportion
  K_THALES_X,         // Thales's theorem in the X figure (two crossing lines, two parallels)
  K_THALES_CONVERSE,  // are these lines parallel? (a choice: yes / no, with the reason)
  K_BISECTOR,         // the angle bisector theorem
  K_SIM_SIDE,         // similar triangles: a side from the ratio
  K_SIM_CRITERION,    // are two triangles similar, and by which criterion? (a choice)
  K_SIM_PERIMETER_AREA, // perimeter and area of similar triangles (k and k squared)
  K_TFA,              // the fundamental theorem of similarity: DE parallel to BC, a side of the small triangle
  K_COUNT
};

// What the screen draws, from the exercise's own numbers. 0: the length is
// the unknown (drawn as ?). Right triangle: A = 90 degrees, v = AB, AC, BC, and v[3] what else is
// asked: 0 nothing (a side is), 2 the height from A to BC (K_HEIGHT_FROM_AREA), 4 the area (K_AREA).
// FIG_RIGHT_NAMED is for the theory pages: the sides are named (AB, AC, BC), not measured.
enum FigureKind : uint8_t {
  FIG_RIGHT_TRIANGLE, FIG_TRIANGLE, FIG_SQUARE_DIAGONAL, FIG_EQUILATERAL_HEIGHT, FIG_RIGHT_NAMED,
  // Unit 4: drawn with the vertices named, the numbers are in the question
  FIG_NONE, FIG_QUAD, FIG_PARALLELOGRAM, FIG_RECTANGLE, FIG_RHOMBUS, FIG_TRAPEZOID, FIG_TRAPEZOID_ISOSCELES, FIG_MIDLINE, FIG_MEDIANS,
  // Unit 5, the circle: drawn to scale from v[] (the numbers asked about are in the question, the figure shows the names)
  FIG_CIRCLE_ARC,  // centre O, points A and B, the chord AB, the arc: v[0] = the central angle AOB in degrees (the minor arc, 10..170)
  FIG_CHORD,       // centre O, chord AB, M its midpoint, OM: v[0] = radius, v[1] = OM, in the same unit (0 < v[1] < v[0])
  FIG_INSCRIBED,   // an inscribed angle BAC: v[0] = the arc BC not containing A, in degrees (20..340)
  FIG_CYCLIC,      // quadrilateral ABCD inscribed: v[0..3] = the arcs AB, BC, CD, DA in degrees (they add up to 360)
  FIG_TANGENT,     // outside point A, tangents AC and AD, the radii OC and OD: v[0] = radius, v[1] = OA (same unit, v[1] > v[0])
  FIG_REGULAR,     // a regular polygon inscribed, with its centre: v[0] = the number of sides (3..12)
  FIG_ANNULUS,     // two concentric circles: v[0] = R, v[1] = r (R > r)
  FIG_SECTOR,      // a sector: v[0] = its angle in degrees (10..350)
  // Unit 6, similarity
  FIG_SEGMENT,     // segment AB with a point M on it: v[0] = AM, v[1] = MB (same unit)
  FIG_THALES,      // triangle ABC with D on AB, E on AC and DE parallel to BC: v[0] = AD, v[1] = DB (E divides AC in the same ratio)
  FIG_XFIG,        // lines AB and CD cross at O, AC parallel to BD: v[0..3] = OA, OB, OC, OD (OA/OB = OC/OD)
  FIG_BISECTOR,    // triangle ABC with the bisector AD, D on BC: v[0] = AB, v[1] = AC (D divides BC in the ratio AB : AC)
  FIG_SIMILAR,     // two triangles ABC and DEF, the second a smaller copy turned a little: no numbers
  FIG_COUNT
};
struct Figure {
  FigureKind kind;
  int32_t v[6];  // the givens; what they mean depends on the kind (see u4Expect, u5Expect, u6Expect)
};

// The kinds of each unit, for the dispatch in generate() and verify().
static const Kind K_U4_FIRST = K_Q_FOURTH_ANGLE, K_U5_FIRST = K_ARC_CENTRAL, K_U6_FIRST = K_RATIO;

static const int MAX_OPTIONS = 4;
static const int MAX_SLIPS = 8;
// The buffers have room for the longest language's words (formatted): firmware/test/tutor7 makes
// every exercise in every language and fails when one does not fit.
static const int WHY_CAP = 280;
// A typical slip: the number it gives, and why it is wrong.
struct Slip {
  Exact v;
  char why[WHY_CAP];
};
struct Exercise {
  Kind kind;
  uint8_t level;  // 1 integers, 2 radicals, 3 the converse and special cases
  uint8_t lang;   // the language of the words (Lang)
  Figure fig;
  char unit[8];    // after each option's number: " cm", "°", " cm²"
  char text[320];  // the question
  int nopt;
  int correct;                     // index into opt
  char opt[MAX_OPTIONS][80];       // what the buttons say
  Exact value[MAX_OPTIONS];        // the number behind an option (kind-specific for K_CONVERSE: see verify)
  char why[MAX_OPTIONS][WHY_CAP];  // why an option is wrong (empty for the right one)
  char clue[2][WHY_CAP];           // a nudge, then a stronger one; never the answer
  char solution[560];              // the worked solution, shown after the answer or on "give up"
  // The generator's scratch: every slip it thought of, three of which become the wrong options. They
  // are here and not on the stack because they take 2.3 KB and the board's loop task has 8 KB: the
  // board keeps its Exercise in PSRAM and makes it there.
  int nslip;
  Slip slip[MAX_SLIPS];
};

// Fills ex for the kind and level, its words in `lang` (a Lang; one without Tutor's words gives
// English); false when it could not make a valid one (never shown: the caller picks another seed).
bool generate(Kind kind, int level, Rng &rng, Exercise &ex, int lang);

// Re-derives everything a second way and checks the exercise is well-posed:
// the figure exists, there is exactly one right option, every wrong option
// differs from it, the strings match the numbers, the clues don't give the
// answer away. err gets the first problem found.
bool verify(const Exercise &ex, char *err, int cap);

}  // namespace t7
