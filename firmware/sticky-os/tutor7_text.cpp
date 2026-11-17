// Tutor's tables (tutor7_text.h): plain C++, so firmware/test/tutor7 builds it on a computer. Built the
// same way as the board's own strings (i18n_tables.cpp): const, put in id order at compile time, in flash.
#include "tutor7_text.h"

#include <array>
#include <stddef.h>

#include "i18n.h"
#include "tutor7_lessons.h"

namespace t7 {
// Not constexpr, never defined: reached at compile time, it stops the build.
const char *tutorIdTwiceOrOutOfRange();
namespace {

struct Tx {
  uint16_t id;
  const char *s;
};
using Table = std::array<const char *, TX_COUNT>;

constexpr Table EN = {{
#define T7S(id, en) en,
#include "src/tutor/t7_strings.h"
#undef T7S
}};

// A language's pairs, put in id order at compile time. A pair twice is an error.
template <size_t N>
constexpr Table build(const Tx (&pairs)[N]) {
  Table t{};
  for (const Tx &p : pairs) {
    if (p.id >= TX_COUNT || t[p.id]) t[0] = tutorIdTwiceOrOutOfRange();
    t[p.id] = p.s;
  }
  return t;
}

constexpr bool allThere(const Table &t) {
  for (const char *s : t)
    if (!s) return false;
  return true;
}

constexpr Tx RO_[] = {
#include "src/tutor/ro.h"
};
constexpr Table RO = build(RO_);

// The lessons, by slot.
constexpr LessonText LESSONS_EN[LESSON_COUNT] = {
#include "src/tutor/lessons_en.h"
};
constexpr LessonText LESSONS_RO[LESSON_COUNT] = {
#include "src/tutor/lessons_ro.h"
};

// Every lesson there, with as many pages as its plan has (the plan is checked by firmware/test/tutor7).
constexpr bool lessonsThere(const LessonText *l) {
  for (int i = 0; i < LESSON_COUNT; i++)
    if (!l[i].name || !l[i].subject || !l[i].summary[0].body || !l[i].example[0].body) return false;
  return true;
}

struct Content {
  const Table *strings;      // nullptr: no Tutor strings in this language
  const LessonText *lessons;  // nullptr: no lessons
  bool complete;
};

constexpr std::array<Content, LANG_COUNT> makeContent() {
  std::array<Content, LANG_COUNT> c{};
  c[L_EN] = {&EN, LESSONS_EN, allThere(EN) && lessonsThere(LESSONS_EN)};
  c[L_RO] = {&RO, LESSONS_RO, allThere(RO) && lessonsThere(LESSONS_RO)};
  return c;
}
constexpr std::array<Content, LANG_COUNT> CONTENT = makeContent();
static_assert(CONTENT[L_EN].complete, "English is Tutor's fallback: it must have every string and lesson");

}  // namespace

const char *tx(int lang, int id) {
  if (id < 0 || id >= TX_COUNT) return "";
  const Table *t = lang >= 0 && lang < LANG_COUNT ? CONTENT[lang].strings : nullptr;
  const char *s = t ? (*t)[id] : nullptr;
  return s ? s : EN[id];
}

const LessonText &lessonText(int lang, int slot) {
  const LessonText *l = lang >= 0 && lang < LANG_COUNT && CONTENT[lang].lessons ? CONTENT[lang].lessons : LESSONS_EN;
  return l[slot < 0 ? 0 : slot >= LESSON_COUNT ? LESSON_COUNT - 1 : slot];
}

bool complete(int lang) { return lang >= 0 && lang < LANG_COUNT && CONTENT[lang].complete; }

int contentLang(int boardLang) { return complete(boardLang) ? boardLang : L_EN; }

}  // namespace t7
