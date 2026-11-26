// Tutor7's lessons: see tutor7_lessons.h. The pages' words are in src/tutor/lessons_<code>.h, by slot.
#include "tutor7_lessons.h"

namespace t7 {

#define NOFIG -1
#define NOFIGS {NOFIG, NOFIG, NOFIG, NOFIG, NOFIG, NOFIG}

// Slot 0, 7.2: the Pythagorean theorem. Concepts, all from the 7th-grade textbooks' unit on metric
// relations in the right triangle: the right triangle and its parts, the theorem, the equivalent forms,
// the converse, the right isosceles triangle, the square's diagonal, the equilateral triangle's height.
// The worked examples use numbers of our own.
static const Lesson PITAGORA = {
    0,
    5, {FIG_RIGHT_NAMED, FIG_RIGHT_NAMED, NOFIG, NOFIG, NOFIG, NOFIG},
    3, NOFIGS,
    7,
    {
        {K_HYPOTENUSE, 1},
        {K_LEG, 1},
        {K_HYPOTENUSE, 2},
        {K_LEG, 2},
        {K_CONVERSE, 3},
        {K_SQUARE_DIAGONAL, 3},
        {K_EQUILATERAL_HEIGHT, 3},
    },
};

// ---- Unit 4, the quadrilateral. Concepts as the textbook's unit teaches them (convex quadrilateral and the
// 360 degrees, parallelogram, midline and centroid, rectangle, rhombus and square, trapezoid, perimeters and
// areas), in our own words. The worked examples use the textbook's kinds of numbers.

static const Lesson U4_ANGLES = {  // 4.1 the convex quadrilateral
    1,
    4, {FIG_QUAD, NOFIG, FIG_QUAD, NOFIG, NOFIG, NOFIG},
    3, NOFIGS,
    6,
    {{K_Q_FOURTH_ANGLE, 1}, {K_Q_FOURTH_ANGLE, 1}, {K_Q_PROPORTIONAL, 2}, {K_Q_FOURTH_ANGLE, 1}, {K_Q_PROPORTIONAL, 2}, {K_Q_PROPORTIONAL, 2}},
};

static const Lesson U4_PARALLELOGRAM = {  // 4.2
    2,
    5, {FIG_PARALLELOGRAM, FIG_PARALLELOGRAM, NOFIG, FIG_PARALLELOGRAM, NOFIG, NOFIG},
    3, NOFIGS,
    6,
    {{K_PAR_ANGLE, 1}, {K_PAR_PERIMETER, 1}, {K_PAR_DIAGONAL, 1}, {K_PAR_ANGLE, 1}, {K_PAR_PERIMETER, 1}, {K_PAR_DIAGONAL, 1}},
};

static const Lesson U4_MIDLINE = {  // 4.3 the midline (midsegment), the median, the centroid
    3,
    4, {FIG_MIDLINE, FIG_MIDLINE, FIG_MEDIANS, FIG_MEDIANS, NOFIG, NOFIG},
    3, NOFIGS,
    7,
    {{K_MIDLINE, 1}, {K_MEDIAN_TRIANGLE, 2}, {K_CENTROID, 2}, {K_MIDLINE, 1}, {K_CENTROID, 2}, {K_MEDIAN_TRIANGLE, 2}, {K_CENTROID, 2}},
};

static const Lesson U4_RECTANGLE = {  // 4.4
    4,
    4, {FIG_RECTANGLE, FIG_RECTANGLE, NOFIG, FIG_RECTANGLE, NOFIG, NOFIG},
    3, NOFIGS,
    6,
    {{K_RECT_DIAGONAL, 1}, {K_RECT_ANGLE, 2}, {K_RECT_DIAGONAL, 1}, {K_RECT_ANGLE, 2}, {K_RECT_DIAGONAL, 1}, {K_RECT_ANGLE, 2}},
};

static const Lesson U4_RHOMBUS = {  // 4.5 the rhombus and the square
    5,
    5, {FIG_RHOMBUS, FIG_RHOMBUS, NOFIG, NOFIG, NOFIG, NOFIG},
    3, NOFIGS,
    7,
    {{K_RHOMBUS_ANGLE, 2}, {K_RHOMBUS_SIDE, 1}, {K_CLASSIFY, 3}, {K_RHOMBUS_ANGLE, 2}, {K_SQUARE_SIDE, 3}, {K_CLASSIFY, 3}, {K_RHOMBUS_SIDE, 2}},
};

static const Lesson U4_TRAPEZOID = {  // 4.6
    6,
    5, {FIG_TRAPEZOID, FIG_TRAPEZOID, FIG_TRAPEZOID_ISOSCELES, FIG_TRAPEZOID, FIG_TRAPEZOID_ISOSCELES, NOFIG},
    3, NOFIGS,
    7,
    {{K_TRAP_ANGLE, 1}, {K_TRAP_MIDLINE, 2}, {K_TRAP_HEIGHT, 1}, {K_TRAP_ANGLE, 1}, {K_TRAP_MIDLINE, 2}, {K_TRAP_HEIGHT, 2}, {K_TRAP_ANGLE, 1}},
};

static const Lesson U4_AREAS = {  // 4.7 perimeters and areas
    7,
    5, {NOFIG, NOFIG, NOFIG, FIG_TRAPEZOID, NOFIG, NOFIG},
    3, NOFIGS,
    8,
    {{K_AREA, 2}, {K_PERIMETER, 1}, {K_AREA, 2}, {K_HEIGHT_FROM_AREA, 1}, {K_PERIMETER, 1}, {K_AREA, 2}, {K_HEIGHT_FROM_AREA, 1}, {K_HEIGHT_FROM_AREA, 2}},
};

static const Lesson U4_REVIEW = {  // 4.8 the unit's review
    8,
    3, NOFIGS,
    3, NOFIGS,
    8,
    {{K_CLASSIFY, 3}, {K_Q_FOURTH_ANGLE, 1}, {K_PAR_ANGLE, 1}, {K_CLASSIFY, 3}, {K_MIDLINE, 1}, {K_RECT_ANGLE, 2}, {K_TRAP_MIDLINE, 2}, {K_AREA, 2}},
};


// ---- Unit 5, the circle. Concepts as the textbook's unit teaches them (chords and arcs, the inscribed angle,
// tangents, regular polygons inscribed in a circle, the length of the circle and the area of the disc), in our
// own words. Slots 9..14. Pages: see the comment on each lesson (the words are in src/tutor/lessons_<code>_u5.h).

static const Lesson U5_CHORDS = {  // 5.1 chords and arcs
    9,
    5, {FIG_CIRCLE_ARC, FIG_CIRCLE_ARC, FIG_CHORD, FIG_CHORD, FIG_CHORD, NOFIG},
    3, NOFIGS,
    7,
    {{K_ARC_CENTRAL, 1}, {K_CHORD_DISTANCE, 1}, {K_ARC_CENTRAL, 1}, {K_CHORD_DISTANCE, 1}, {K_ARC_CENTRAL, 2}, {K_CHORD_DISTANCE, 2}, {K_CHORD_DISTANCE, 2}},
};

static const Lesson U5_INSCRIBED = {  // 5.2 the inscribed angle
    10,
    4, {FIG_INSCRIBED, FIG_INSCRIBED, FIG_CYCLIC, FIG_INSCRIBED, NOFIG, NOFIG},
    3, NOFIGS,
    7,
    {{K_INSCRIBED, 1}, {K_CYCLIC_QUAD, 1}, {K_INSCRIBED, 1}, {K_CYCLIC_QUAD, 1}, {K_INSCRIBED, 2}, {K_CYCLIC_QUAD, 2}, {K_INSCRIBED, 2}},
};

static const Lesson U5_TANGENTS = {  // 5.3 tangents from an outside point
    11,
    5, {NOFIG, FIG_TANGENT, FIG_TANGENT, NOFIG, NOFIG, NOFIG},
    3, NOFIGS,
    7,
    {{K_TANGENT_LENGTH, 1}, {K_CIRCUMSCRIBED, 1}, {K_INCIRCLE_AREA, 1}, {K_TANGENT_LENGTH, 1}, {K_CIRCUMSCRIBED, 1}, {K_TANGENT_LENGTH, 2}, {K_INCIRCLE_AREA, 1}},
};

static const Lesson U5_REGULAR = {  // 5.4 regular polygons inscribed in a circle
    12,
    4, {FIG_REGULAR, FIG_REGULAR, FIG_REGULAR, FIG_REGULAR, NOFIG, NOFIG},
    3, NOFIGS,
    6,
    {{K_REGULAR_POLYGON, 1}, {K_REGULAR_POLYGON, 1}, {K_REGULAR_POLYGON, 2}, {K_REGULAR_POLYGON, 2}, {K_REGULAR_POLYGON, 3}, {K_REGULAR_POLYGON, 3}},
};

static const Lesson U5_CIRCLE_AREA = {  // 5.5 the length of the circle and the area of the disc
    13,
    5, {NOFIG, NOFIG, FIG_SECTOR, FIG_SECTOR, FIG_ANNULUS, NOFIG},
    3, NOFIGS,
    8,
    {{K_CIRCUMFERENCE, 1}, {K_DISC_AREA, 1}, {K_ARC_SECTOR, 1}, {K_ANNULUS, 1}, {K_CIRCUMFERENCE, 2}, {K_DISC_AREA, 2}, {K_ARC_SECTOR, 2}, {K_ANNULUS, 1}},
};

static const Lesson U5_REVIEW = {  // 5.6 the unit's review
    14,
    3, NOFIGS,
    3, NOFIGS,
    8,
    {{K_ARC_CENTRAL, 1}, {K_INSCRIBED, 1}, {K_CHORD_DISTANCE, 1}, {K_TANGENT_LENGTH, 1}, {K_REGULAR_POLYGON, 2}, {K_CIRCUMFERENCE, 1}, {K_DISC_AREA, 1}, {K_ARC_SECTOR, 1}},
};

// ---- Unit 6, similarity (proportional segments, Thales's theorem and its converse, similar triangles and their
// criteria). Slots 15..20 (the words are in src/tutor/lessons_<code>_u6.h).

static const Lesson U6_SEGMENTS = {  // 6.1 proportional segments
    15,
    4, {FIG_SEGMENT, NOFIG, FIG_SEGMENT, NOFIG, NOFIG, NOFIG},
    3, NOFIGS,
    6,
    {{K_RATIO, 1}, {K_DIVIDE_SEGMENT, 1}, {K_RATIO, 1}, {K_DIVIDE_SEGMENT, 2}, {K_RATIO, 2}, {K_DIVIDE_SEGMENT, 2}},
};

static const Lesson U6_THALES = {  // 6.2 Thales's theorem, the bisector theorem
    16,
    4, {FIG_THALES, FIG_THALES, FIG_XFIG, FIG_BISECTOR, NOFIG, NOFIG},
    3, NOFIGS,
    7,
    {{K_THALES, 1}, {K_THALES_X, 1}, {K_THALES, 1}, {K_BISECTOR, 1}, {K_THALES, 2}, {K_THALES_X, 2}, {K_BISECTOR, 2}},
};

static const Lesson U6_CONVERSE = {  // 6.3 the converse of Thales's theorem
    17,
    3, {FIG_THALES, NOFIG, NOFIG, NOFIG, NOFIG, NOFIG},
    3, NOFIGS,
    6,
    {{K_THALES_CONVERSE, 1}, {K_THALES_CONVERSE, 1}, {K_THALES, 1}, {K_THALES_CONVERSE, 2}, {K_THALES_CONVERSE, 2}, {K_THALES, 2}},
};

static const Lesson U6_SIMILAR = {  // 6.4 similar triangles, the fundamental theorem
    18,
    4, {FIG_SIMILAR, FIG_THALES, FIG_SIMILAR, NOFIG, NOFIG, NOFIG},
    3, NOFIGS,
    6,
    {{K_SIM_SIDE, 1}, {K_TFA, 1}, {K_SIM_SIDE, 1}, {K_TFA, 1}, {K_SIM_SIDE, 2}, {K_TFA, 2}},
};

static const Lesson U6_CRITERIA = {  // 6.5 the criteria, heights, perimeters and areas
    19,
    5, {FIG_SIMILAR, FIG_SIMILAR, FIG_SIMILAR, FIG_SIMILAR, NOFIG, NOFIG},
    3, NOFIGS,
    8,
    {{K_SIM_CRITERION, 1}, {K_SIM_PERIMETER_AREA, 1}, {K_SIM_CRITERION, 2}, {K_SIM_PERIMETER_AREA, 2}, {K_SIM_CRITERION, 3}, {K_SIM_SIDE, 2}, {K_SIM_PERIMETER_AREA, 2}, {K_TFA, 2}},
};

static const Lesson U6_REVIEW = {  // 6.6 the unit's review
    20,
    3, NOFIGS,
    3, NOFIGS,
    8,
    {{K_RATIO, 1}, {K_THALES, 1}, {K_THALES_CONVERSE, 1}, {K_BISECTOR, 1}, {K_SIM_SIDE, 1}, {K_SIM_CRITERION, 1}, {K_SIM_PERIMETER_AREA, 1}, {K_TFA, 1}},
};

// The list, in the textbook's order: Unit 4, Unit 5, Unit 6, then Unit 7's Pythagorean theorem.
static const Lesson *const ALL[LESSON_COUNT] = {&U4_ANGLES, &U4_PARALLELOGRAM, &U4_MIDLINE, &U4_RECTANGLE, &U4_RHOMBUS,
                                                 &U4_TRAPEZOID, &U4_AREAS, &U4_REVIEW,
                                                 &U5_CHORDS, &U5_INSCRIBED, &U5_TANGENTS, &U5_REGULAR, &U5_CIRCLE_AREA, &U5_REVIEW,
                                                 &U6_SEGMENTS, &U6_THALES, &U6_CONVERSE, &U6_SIMILAR, &U6_CRITERIA, &U6_REVIEW,
                                                 &PITAGORA};

const Lesson &lesson(int i) { return *ALL[i < 0 ? 0 : i >= LESSON_COUNT ? LESSON_COUNT - 1 : i]; }

const char *lessonNumber(int slot) {
  static const char *const NUM[] = {"7.2", "4.1", "4.2", "4.3", "4.4", "4.5", "4.6", "4.7", "4.8",
                                       "5.1", "5.2", "5.3", "5.4", "5.5", "5.6", "6.1", "6.2", "6.3", "6.4", "6.5", "6.6"};  // slot 0 is Pitagora; 1..8 Unit 4; 9..14 Unit 5; 15..20 Unit 6
  return slot >= 0 && slot < (int)(sizeof NUM / sizeof NUM[0]) ? NUM[slot] : "";
}

}  // namespace t7
