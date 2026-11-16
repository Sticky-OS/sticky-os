// Tutor7's lessons: what each one says, in Romanian, and which exercises
// follow. Plain data, no Arduino, so a test can check it. The theory is only
// what the 7th-grade textbooks teach (nothing new), in our own words.
//
// Text is UTF-8. In the page text: \n starts a new paragraph; a square root is
// the character U+221A followed by its radicand (digits or letters), or by a
// parenthesised expression; the screen draws it with a bar.
#pragma once
#include "tutor7_core.h"

namespace t7 {

struct Page {
  const char *title;
  const char *body;
  int figure;  // a FigureKind to draw beside the text, or -1
};

struct Step {  // one exercise of a lesson's practice, in order
  Kind kind;
  uint8_t level;
};

static const int MAX_STEPS = 8;
static const int MAX_PAGES = 6;

struct Lesson {
  const char *name;      // in the lesson list
  const char *subject;   // "Geometrie"
  int nSummary;
  Page summary[MAX_PAGES];
  int nExample;
  Page example[MAX_PAGES];
  int nSteps;
  Step steps[MAX_STEPS];
};

static const int LESSON_COUNT = 1;
const Lesson &lesson(int i);

}  // namespace t7
