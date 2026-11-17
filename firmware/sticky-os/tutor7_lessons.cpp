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

// The list, in the order the menu shows it: Unit 4 first, then Unit 7's Pythagorean theorem.
static const Lesson *const ALL[LESSON_COUNT] = {&U4_ANGLES, &U4_PARALLELOGRAM, &U4_MIDLINE, &U4_RECTANGLE, &U4_RHOMBUS,
                                                 &U4_TRAPEZOID, &U4_AREAS, &U4_REVIEW, &PITAGORA};

const Lesson &lesson(int i) { return *ALL[i < 0 ? 0 : i >= LESSON_COUNT ? LESSON_COUNT - 1 : i]; }

const char *lessonNumber(int slot) {
  static const char *const NUM[] = {"7.2", "4.1", "4.2", "4.3", "4.4", "4.5", "4.6", "4.7", "4.8"};  // slot 0 is Pitagora; 1..8 are Unit 4's lessons
  return slot >= 0 && slot < (int)(sizeof NUM / sizeof NUM[0]) ? NUM[slot] : "";
}

}  // namespace t7
