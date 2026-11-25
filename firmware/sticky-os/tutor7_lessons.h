// Tutor's lessons (7th grade maths, geometry, for now): the plan of each one, the same in every
// language: how many summary and example pages it has, the figure beside each, and which exercises
// its practice asks. Plain data, no Arduino, so a test can check it. The words of the pages are in
// src/tutor/lessons_<code>.h (tutor7_text.h: lessonText()). The theory is only what the 7th-grade
// textbooks teach (nothing new), in our own words.
#pragma once
#include "tutor7_core.h"
#include "tutor7_text.h"

namespace t7 {

struct Step {  // one exercise of a lesson's practice, in order
  Kind kind;
  uint8_t level;
};

static const int MAX_STEPS = 8;

struct Lesson {
  int slot;  // where its best score is kept (GameProgress.tutor7Best), whatever its place in the list; its words (lessonText)
  int nSummary;
  int8_t summaryFigure[MAX_PAGES];  // a FigureKind to draw beside each summary page, or -1
  int nExample;
  int8_t exampleFigure[MAX_PAGES];
  int nSteps;
  Step steps[MAX_STEPS];
};

static const int LESSON_COUNT = 21;  // Unit 4 (eight lessons), Unit 5 (six), Unit 6 (six), then the Pythagorean theorem of Unit 7
const Lesson &lesson(int i);  // in the order of the lesson list
// The textbook's number for a lesson (by its slot): "4.1" is Unit 4, lesson 1; "7.2" is Unit 7, lesson 2.
// The last lesson of Unit 4, 5 and 6 (x.8 / 5.6 / 6.6) is that unit's own review.
// The numbers follow the Romanian textbook, in every language.
const char *lessonNumber(int slot);

}  // namespace t7
