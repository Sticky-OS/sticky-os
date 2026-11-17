// Tutor's words, by language: the exercises' formats (src/tutor/t7_strings.h, English, and a file a
// language beside it: src/tutor/ro.h) and the lessons' pages (src/tutor/lessons_<code>.h). Plain C++,
// so firmware/test/tutor7 builds it on a computer; the tables are const, in flash.
//
// The languages are the board's (Lang, i18n.h). English is complete and is the fallback. Tutor shows
// its content in the board's language when that language has all of it (every exercise string and
// every lesson), else in English: an exercise or a page is never half in one language and half in
// another. The buttons and titles around it are the board's own strings (S_G_T7_*, src/i18n).
#pragma once
#include <stdint.h>
// Languages are ints here, a Lang of i18n.h (not included: on the board it brings Arduino.h, whose sq()
// macro the generators' sq() would clash with).

namespace t7 {

enum TxId : uint16_t {
#define T7S(id, en) id,
#include "src/tutor/t7_strings.h"
#undef T7S
  TX_COUNT
};

static const int MAX_PAGES = 6;  // a lesson's summary or examples

struct PageText {
  const char *title;
  const char *body;  // \n starts a paragraph; "√" then digits or a parenthesised expression is a root
};
// A lesson's words in one language. The pages' number and figures are the lesson's plan
// (tutor7_lessons.cpp), the same in every language: summary[i] is shown for plan page i.
struct LessonText {
  const char *name;     // in the lesson list and the lesson's header
  const char *subject;  // "Geometry": the lesson list's header
  PageText summary[MAX_PAGES];
  PageText example[MAX_PAGES];
};

// The string in `lang`; English when that language hasn't got it (or lang is out of range).
const char *tx(int lang, int id);
// Lesson `slot`'s words (slot as in Lesson::slot) in `lang`; English when that language hasn't got the lessons.
const LessonText &lessonText(int lang, int slot);
// Every exercise string and every lesson is there in `lang`.
bool complete(int lang);
// The language Tutor's content is shown in on a board set to boardLang: boardLang if complete, else English.
int contentLang(int boardLang);

}  // namespace t7
