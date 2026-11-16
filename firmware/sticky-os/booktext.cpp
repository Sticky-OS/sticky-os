// Plain text, Markdown and HTML files (bookformats.h).
//
// HTML goes through HtmlToFlow, with its headings as the contents.
//
// Text is read 16 KB a step, turned into UTF-8 (bookcharset.h) and taken a
// line at a time; lines gather into a paragraph (32 KB at most) that is
// written when it ends. The file's first 64 KB decide how:
// - The encoding: a byte order mark; else UTF-8 if the bytes are valid
//   UTF-8; else windows-1251 if most letters are beyond ASCII (Russian);
//   else windows-1252.
// - The shape: if a tenth or more of the lines are longer than 110
//   characters, each line is a paragraph (text written by a word processor);
//   otherwise the lines are hard-wrapped (Project Gutenberg) and a blank
//   line ends a paragraph, or, in a file with almost no blank lines, an
//   indented line starts one. In the wrapped shape a paragraph whose lines
//   are all short (verse, a list of contents) or whose later lines are
//   indented (verse again) keeps its lines; one indented 8 or more all
//   through is centred.
// - Chapters: a paragraph of one or two short lines that starts like
//   "CHAPTER XII." / "Chapter 3: A Title" / "BOOK ONE" / "PART II", or is
//   "PROLOGUE" and the like, or a numeral alone between blank lines,
//   becomes a heading on a new page with a contents entry; a title-like line
//   right after it joins it. The words must be capitalised, short and alone
//   in their paragraph: a sentence that starts with "Chapter" stays prose.
// - "_word_" is italic when the closing mark is in the same paragraph.
// - Project Gutenberg's "*** START OF ..." and "*** END OF ..." lines stay
//   (with the licence: it belongs with the book), with a rule after the
//   first and before the second.
//
// Markdown (.md, .markdown): # and underlined headings (in the contents;
// a page break before each level-one heading), *, _, ** and __ emphasis,
// lists, > quotes, --- rules, code as plain lines, links as their text,
// pictures as their alt text.
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "bookcharset.h"
#include "bookformats.h"
#include "psalloc.h"

namespace {

const size_t READ = 16384;         // bytes of the file a step
const size_t SAMPLE = 65536;       // looked at to choose the encoding and shape
const size_t PARA_MAX = 32768;     // a paragraph longer than this is cut
const size_t TEXT_LINE_MAX = 16384;     // and a line
const size_t HEADING_CHARS = 70;   // a heading's lines are no longer

bool isSpace(char c) { return c == ' ' || c == '\t'; }

size_t chars(const char *p, size_t n) {
  size_t k = 0;
  for (size_t i = 0; i < n; i++) k += ((uint8_t)p[i] & 0xC0) != 0x80;
  return k;
}

// The encoding of a file that starts with these bytes; skip: its byte order mark.
Charset detect(const uint8_t *p, size_t n, size_t &skip) {
  skip = 0;
  if (n >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) return skip = 3, CS_UTF8;
  if (n >= 2 && p[0] == 0xFF && p[1] == 0xFE) return skip = 2, CS_UTF16LE;
  if (n >= 2 && p[0] == 0xFE && p[1] == 0xFF) return skip = 2, CS_UTF16BE;
  // UTF-16 without a mark: ASCII's zero halves.
  size_t zeroEven = 0, zeroOdd = 0, look = n < 4096 ? n : 4096;
  for (size_t i = 0; i < look; i++)
    if (!p[i]) (i & 1 ? zeroOdd : zeroEven)++;
  if (look >= 16 && zeroOdd > look / 4 && zeroEven < look / 50) return CS_UTF16LE;
  if (look >= 16 && zeroEven > look / 4 && zeroOdd < look / 50) return CS_UTF16BE;
  if (utf8Valid(p, n)) return CS_UTF8;
  size_t letters = 0, high = 0;
  for (size_t i = 0; i < n; i++) {
    if (p[i] >= 0xC0) high++;
    else if (isalpha(p[i])) letters++;
  }
  return high * 10 > (high + letters) * 4 ? CS_1251 : CS_1252;
}

// "Language: English" in a Project Gutenberg header, as a code.
const char *languageCode(const std::string &name) {
  static const struct { const char *name, *code; } LANGS[] = {
      {"english", "en"},   {"french", "fr"},    {"spanish", "es"}, {"german", "de"},  {"italian", "it"},   {"portuguese", "pt"},
      {"dutch", "nl"},     {"russian", "ru"},   {"polish", "pl"},  {"romanian", "ro"}, {"latin", "la"},    {"finnish", "fi"},
      {"swedish", "sv"},   {"danish", "da"},    {"norwegian", "no"}, {"greek", "el"},  {"hungarian", "hu"}, {"czech", "cs"},
      {"catalan", "ca"},   {"esperanto", "eo"}, {"chinese", "zh"}, {"japanese", "ja"},
  };
  std::string low;
  for (char c : name) low += (char)tolower((unsigned char)c);
  for (auto &l : LANGS)
    if (low == l.name) return l.code;
  return "";
}

// Title:, Author:, Language: lines of a Project Gutenberg header (UTF-8 text).
void gutenbergMeta(const std::string &head, BookMeta &meta) {
  size_t i = 0;
  while (i < head.size()) {
    size_t nl = head.find('\n', i);
    if (nl == std::string::npos) break;  // a line cut off by the sample's end: not trusted
    std::string line = head.substr(i, nl - i);
    i = nl + 1;
    while (line.size() && (line.back() == '\r' || isSpace(line.back()))) line.pop_back();
    size_t colon = line.find(": ");
    if (colon == std::string::npos || colon > 10) continue;
    std::string key = line.substr(0, colon), value = line.substr(colon + 2);
    while (value.size() && isSpace(value[0])) value.erase(0, 1);
    if (value.empty() || value.size() > 300) continue;
    if (key == "Title" && meta.title.empty()) meta.title = value;
    else if (key == "Author" && meta.author.empty()) meta.author = value;
    else if (key == "Language" && meta.language.empty()) meta.language = languageCode(value);
  }
}

bool startsWith(const char *p, size_t n, const char *s) {
  size_t k = strlen(s);
  return n >= k && !memcmp(p, s, k);
}

// <meta name="description" content="..."> in an HTML file's head (UTF-8 text).
void htmlDescription(const std::string &text, std::string &out) {
  for (size_t at = 0; (at = text.find('<', at)) != std::string::npos; at++) {
    size_t e = text.find('>', at);
    if (e == std::string::npos) return;
    std::string tag;
    for (size_t i = at + 1; i < e; i++) tag += (char)tolower((unsigned char)text[i]);
    if (!tag.compare(0, 4, "body")) return;  // past the head
    if (tag.compare(0, 5, "meta ")) continue;
    // Its name and content, quoted ' or ".
    std::string name, content;
    for (const char *key : {"name", "content"}) {
      size_t k = tag.find(std::string(key) + "=");
      while (k != std::string::npos && k > 0 && !isspace((unsigned char)tag[k - 1])) k = tag.find(std::string(key) + "=", k + 1);
      if (k == std::string::npos) continue;
      size_t v = k + strlen(key) + 1;
      char q = v < tag.size() ? tag[v] : 0;
      size_t ve = q == '"' || q == '\'' ? tag.find(q, ++v) : tag.find(' ', v);
      if (ve == std::string::npos) ve = tag.size();
      (*key == 'n' ? name : content) = text.substr(at + 1 + v, ve - v);  // the content as written, not lowered
    }
    for (char &c : name) c = (char)tolower((unsigned char)c);
    if (name == "description" && content.size()) {
      bookCleanDescription(content.data(), content.size(), out, BOOK_DESCRIPTION_MAX);
      return;
    }
  }
}

// The words a chapter's heading starts with, and the numbers after them.
bool isRoman(const char *p, size_t n) {
  if (!n || n > 9) return false;
  for (size_t i = 0; i < n; i++)
    if (!strchr("IVXLCDM", p[i]) && !strchr("ivxlcdm", p[i])) return false;
  // All one case.
  bool up = isupper((unsigned char)p[0]);
  for (size_t i = 0; i < n; i++)
    if ((bool)isupper((unsigned char)p[i]) != up) return false;
  return true;
}

bool isNumberWord(const char *p, size_t n) {
  static const char *const WORDS[] = {"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve",
                                      "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty", "first", "second",
                                      "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth", "eleventh", "twelfth", "last", "the"};
  for (const char *w : WORDS) {
    size_t k = strlen(w);
    if (k != n) continue;
    size_t i = 0;
    while (i < n && tolower((unsigned char)p[i]) == w[i]) i++;
    if (i == n) return strcmp(w, "the") != 0;
  }
  return false;
}

bool isNumber(const char *p, size_t n) {
  if (!n) return false;
  size_t i = 0;
  while (i < n && isdigit((unsigned char)p[i])) i++;
  if (i == n && n <= 4) return true;
  return isRoman(p, n) || isNumberWord(p, n);
}

// Does a capital (or a digit) start at p[i]? ASCII, Latin-1's accented
// letters, Latin Extended-A's even ones, Greek and Cyrillic.
bool capitalAt(const char *p, size_t n, size_t i) {
  unsigned char c = p[i];
  if (c < 0x80) return isupper(c) || isdigit(c);
  if (i + 1 >= n) return false;
  uint32_t cp = c >= 0xC0 && c < 0xE0 ? (uint32_t)(c & 0x1F) << 6 | ((uint8_t)p[i + 1] & 0x3F) : 0;
  return (cp >= 0xC0 && cp <= 0xDE && cp != 0xD7) || (cp >= 0x100 && cp < 0x180 && !(cp & 1)) || (cp >= 0x391 && cp <= 0x3A9) || (cp >= 0x400 && cp <= 0x42F);
}

// Most of its words start with a capital (a title), and it is short.
bool titleLike(const char *p, size_t n) {
  if (!n || chars(p, n) > HEADING_CHARS) return false;
  if (p[0] == '"' || p[0] == '\'' || p[0] == '-' || (uint8_t)p[0] == 0xE2) return false;  // dialogue: quotes, dashes
  int words = 0, caps = 0;
  for (size_t i = 0; i < n; i++) {
    if (isSpace(p[i]) || (i && !isSpace(p[i - 1]))) continue;
    words++;
    if (capitalAt(p, n, i)) caps++;
  }
  return words && caps * 2 >= words;
}

enum Heading { H_NONE, H_PART, H_CHAPTER, H_NUMBER };

// Is this line a chapter's (or part's) heading?
Heading headingOf(const char *p, size_t n) {
  while (n && isSpace(*p)) p++, n--;
  if (!n || chars(p, n) > HEADING_CHARS) return H_NONE;
  size_t w = 0;
  while (w < n && isalpha((unsigned char)p[w])) w++;
  std::string word(p, w), low;
  for (char c : word) low += (char)tolower((unsigned char)c);
  bool capital = w && isupper((unsigned char)p[0]);
  bool allCaps = capital;
  for (size_t i = 0; i < w; i++)
    if (islower((unsigned char)p[i])) allCaps = false;
  // A word alone: "PROLOGUE", "Epilogue."
  static const char *const ALONE[] = {"prologue", "epilogue", "preface", "introduction", "foreword", "afterword", "conclusion"};
  size_t rest = w;
  while (rest < n && (p[rest] == '.' || isSpace(p[rest]))) rest++;
  if (capital && rest == n)
    for (const char *a : ALONE)
      if (low == a) return H_CHAPTER;
  // A numeral alone ("XII.", "12"): the caller wants blank lines around it.
  {
    size_t k = 0;
    while (k < n && (isalnum((unsigned char)p[k]))) k++;
    size_t e = k;
    while (e < n && (p[e] == '.' || isSpace(p[e]))) e++;
    if (k && e == n && (isRoman(p, k) || (isdigit((unsigned char)p[0]) && isNumber(p, k)))) return H_NUMBER;
  }
  static const struct { const char *word; Heading h; } WORDS[] = {
      {"chapter", H_CHAPTER}, {"book", H_PART}, {"part", H_PART}, {"volume", H_PART}, {"canto", H_CHAPTER}, {"letter", H_CHAPTER},
      {"chapitre", H_CHAPTER}, {"capitulo", H_CHAPTER}, {"kapitel", H_CHAPTER}, {"livre", H_PART}, {"partie", H_PART}, {"stave", H_CHAPTER},
  };
  Heading h = H_NONE;
  for (auto &e : WORDS)
    if (low == e.word) h = e.h;
  if (h != H_NONE && !(capital && (allCaps || islower((unsigned char)p[1])))) h = H_NONE;
  // The words beyond ASCII, as they are written (capitalised or in capitals).
  static const struct { const char *word; Heading h; } UTF8[] = {
      {"\xD0\x93\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB0", H_CHAPTER}, {"\xD0\x93\xD0\x9B\xD0\x90\xD0\x92\xD0\x90", H_CHAPTER},  // Глава ГЛАВА
      {"\xD0\xA7\xD0\xB0\xD1\x81\xD1\x82\xD1\x8C", H_PART}, {"\xD0\xA7\xD0\x90\xD0\xA1\xD0\xA2\xD0\xAC", H_PART},          // Часть ЧАСТЬ
      {"Cap\xC3\xADtulo", H_CHAPTER}, {"CAP\xC3\x8DTULO", H_CHAPTER}, {"Rozdzia\xC5\x82", H_CHAPTER}, {"ROZDZIA\xC5\x81", H_CHAPTER},
  };
  for (auto &e : UTF8)
    if (h == H_NONE && startsWith(p, n, e.word)) h = e.h, w = strlen(e.word);
  if (h == H_NONE || w >= n || !isSpace(p[w])) return H_NONE;
  size_t a = w;
  while (a < n && isSpace(p[a])) a++;
  size_t b = a;
  while (b < n && isalnum((unsigned char)p[b])) b++;
  if (!isNumber(p + a, b - a)) return H_NONE;
  // After the number: nothing, or a separator and a title.
  size_t c = b;
  while (c < n && isSpace(p[c])) c++;
  if (c == n) return h;
  bool sep = p[c] == '.' || p[c] == ':' || p[c] == '-' || ((uint8_t)p[c] == 0xE2 && c + 2 < n && (uint8_t)p[c + 1] == 0x80 && ((uint8_t)p[c + 2] == 0x94 || (uint8_t)p[c + 2] == 0x93));
  if (sep) {
    c += (uint8_t)p[c] == 0xE2 ? 3 : 1;
    while (c < n && (isSpace(p[c]) || p[c] == '.')) c++;
    if (c == n || titleLike(p + c, n - c) || (c < n && isupper((unsigned char)p[c]))) return h;
    return H_NONE;
  }
  return titleLike(p + c, n - c) && c > b ? h : H_NONE;
}


class TextJob : public BookJob {
 public:
  TextJob(FlowWriter &out, BookMeta &meta) : out_(out), meta_(meta) {}
  ~TextJob() override {
    if (f_) fclose(f_);
    free(buf_);
    delete html_;
  }

  bool start(const char *path, BookFormat format, BookErr &err) {
    f_ = fopen(path, "rb");
    if (!f_) return err = BE_OPEN, false;
    buf_ = (uint8_t *)psMalloc(SAMPLE);
    if (!buf_) return err = BE_MEMORY, false;
    if (fseek(f_, 0, SEEK_END) == 0) size_ = ftell(f_);
    if (size_ < 0 || fseek(f_, 0, SEEK_SET) != 0) return err = BE_OPEN, false;
    size_t n = fread(buf_, 1, SAMPLE, f_);
    if (fseek(f_, 0, SEEK_SET) != 0) return err = BE_OPEN, false;
    size_t skip;
    Charset cs = detect(buf_, n, skip);
    const char *dot = strrchr(path, '.');
    std::string ext = dot ? dot : "";
    for (char &c : ext) c = (char)tolower((unsigned char)c);
    if (format == BF_HTML) {
      html_ = new HtmlToFlow(out_);
      html_->tocFromHeadings(true);
      if (cs == CS_1252 || cs == CS_1251) html_->setCharset(cs == CS_1252 ? "windows-1252" : "windows-1251");  // unless the file says
      CharsetDecoder dec(cs);
      std::string head;
      dec.feed((const char *)buf_ + skip, (n - skip < 8192 ? n - skip : 8192), head);
      htmlDescription(head, meta_.description);
      return true;
    }
    markdown_ = ext == ".md" || ext == ".markdown";
    dec_.reset(cs);
    skip_ = skip;
    // The shape, and the header, from the sample.
    std::string text;
    dec_.feed((const char *)buf_ + skip, n - skip, text);
    dec_.reset(cs);
    gutenbergMeta(text.substr(0, 4096), meta_);
    size_t lines = 0, longOnes = 0, blank = 0, indented = 0;
    for (size_t i = 0; i < text.size();) {
      size_t nl = text.find('\n', i);
      if (nl == std::string::npos) break;
      size_t len = nl - i;
      if (len && text[nl - 1] == '\r') len--;
      size_t k = 0;
      while (k < len && isSpace(text[i + k])) k++;
      if (k == len) {
        blank++;
      } else {
        lines++;
        size_t c = chars(text.data() + i, len);
        if (c > 110) longOnes++;
        if (k >= 2) indented++;
        if (c > 40) widths_[c > 99 ? 99 : c]++;
      }
      i = nl + 1;
    }
    perLine_ = lines && longOnes * 10 >= lines;
    indentParas_ = !perLine_ && blank * 33 < lines && indented * 10 > lines;  // paragraphs that start indented, with no blank lines
    // The typical full line: what 90 % of the long-ish lines don't pass.
    size_t total = 0, seen = 0;
    for (size_t w : widths_) total += w;
    width_ = 72;
    for (int c = 0; c < 100 && total; c++)
      if ((seen += widths_[c]) * 10 >= total * 9) {
        width_ = (size_t)c;
        break;
      }
    return true;
  }

  int step() override {
    if (!f_) return BOOK_DONE;
    size_t n = fread(buf_, 1, READ, f_);
    if (n == 0) {
      if (ferror(f_)) return -(int)BE_OPEN;
      finish();
      return BOOK_DONE;
    }
    done_ += (long)n;
    if (html_) {
      html_->feed((const char *)buf_, n);
    } else {
      size_t from = skip_ < n ? skip_ : n;
      skip_ = 0;
      for (size_t i = from; i < n; i += 256) {
        tmp_.clear();
        dec_.feed((const char *)buf_ + i, n - i < 256 ? n - i : 256, tmp_);
        take(tmp_.data(), tmp_.size());
      }
    }
    int p = size_ > 0 ? (int)((double)done_ * 999 / (double)size_) : 0;
    return p > 999 ? 999 : p;
  }

 private:
  void finish() {
    if (html_) {
      html_->finish();
      if (meta_.title.empty()) meta_.title = html_->title();
    } else {
      if (line_.size() || lastCR_) endLine();
      blankLine();
      flushHeading();
    }
    fclose(f_);
    f_ = nullptr;
  }

  // ---- lines ----

  void take(const char *p, size_t n) {
    for (size_t i = 0; i < n; i++) {
      char c = p[i];
      if (c == '\n') {
        if (lastCR_) {
          lastCR_ = false;  // "\r\n": one end
          continue;
        }
        endLine();
      } else if (c == '\r') {
        endLine();
        lastCR_ = true;
      } else {
        lastCR_ = false;
        if (line_.size() >= TEXT_LINE_MAX && ((uint8_t)c & 0xC0) != 0x80) endLine();
        line_.push_back(c);
      }
    }
  }

  void endLine() {
    // Without the spaces after it (Markdown's two are a line break).
    size_t n = line_.size();
    bool hard = false;
    if (markdown_ && n >= 2 && line_[n - 1] == ' ' && line_[n - 2] == ' ') hard = true;
    while (n && (isSpace(line_[n - 1]) || line_[n - 1] == '\f')) n--;
    if (markdown_ && n && line_[n - 1] == '\\') hard = true, n--;
    size_t indent = 0;
    while (indent < n && isSpace(line_[indent])) indent += line_[indent] == '\t' ? 4 : 1;
    size_t at = 0;
    while (at < n && isSpace(line_[at])) at++;
    if (at == n) blankLine();
    else if (markdown_) markdownLine(line_.data() + at, n - at, indent, hard);
    else textLine(line_.data() + at, n - at, indent);
    line_.clear();
  }

  void blankLine() {
    if (markdown_ && fence_) {
      if (inBlock_) out_.lineBreak();
      return;
    }
    endPara(true);
    blank_ = true;
  }

  // ---- plain text ----

  void textLine(const char *p, size_t n, size_t indent) {
    if (perLine_) {
      endPara(false);  // the line before: a paragraph (ended here, so a blank line after it can tell)
      addLine(p, n, indent);
      return;
    }
    if (indentParas_ && indent >= 2 && lines_) endPara(false);
    addLine(p, n, indent);
  }

  void addLine(const char *p, size_t n, size_t indent, char sep = '\n') {
    if (para_.size() + n + 1 > PARA_MAX) endPara(false);
    if (!lines_) startedAfterBlank_ = blank_, minIndent_ = indent, allShort_ = true, laterIndented_ = false;
    else para_.push_back(sep);
    blank_ = false;
    if (indent < minIndent_) minIndent_ = indent;
    if (lines_ && indent >= 2) laterIndented_ = true;
    if (chars(p, n) > width_ * 3 / 4) allShort_ = false;
    lineStarts_[lines_ < 2 ? lines_ : 2] = para_.size();
    para_.insert(para_.end(), p, p + n);
    lines_++;
  }

  void endPara(bool byBlank) {
    if (!lines_) return;
    int lines = lines_;
    lines_ = 0;
    paragraph(para_.data(), para_.size(), lines, byBlank);
    para_.clear();
  }

  // The paragraph is whole: what is it?
  void paragraph(const char *p, size_t n, int lines, bool byBlank) {
    if (markdown_) return markdownPara(p, n, lines);
    size_t first = lines > 1 ? lineStarts_[1] - 1 : n;  // the first line's end
    // A heading?
    Heading h = lines <= 2 ? headingOf(p, first) : H_NONE;
    if (h == H_NUMBER && !(lines == 1 && startedAfterBlank_ && byBlank)) h = H_NONE;
    if (h != H_NONE && lines == 2 && (!titleLike(p + first + 1, n - first - 1) || headingOf(p + first + 1, n - first - 1) != H_NONE)) h = H_NONE;  // (two chapters' lines: a list of contents)
    if (h != H_NONE) {
      flushHeading();
      heading_.assign(p, n);
      headingKind_ = h;
      headingLines_ = lines;
      if (lines == 1) return;  // its title may follow
      return flushHeading();
    }
    if (heading_.size()) {
      // A title just after a heading of one line joins it.
      if (lines == 1 && headingLines_ == 1 && titleLike(p, n) && headingOf(p, n) == H_NONE) {
        heading_ += '\n';
        heading_.append(p, n);
        return flushHeading();
      }
      flushHeading();
    }
    // "*  *  *  *": a break in the text.
    size_t stars = 0, k = 0;
    for (; k < n && (p[k] == '*' || p[k] == ' ' || p[k] == '\n'); k++) stars += p[k] == '*';
    if (k == n && stars >= 3) return out_.rule();
    bool start = startsWith(p, n, "*** START OF") || startsWith(p, n, "***START OF");
    bool end = startsWith(p, n, "*** END OF") || startsWith(p, n, "***END OF");
    if (end) out_.rule();
    bool keep = lines >= 2 && (allShort_ || laterIndented_);
    out_.beginBlock(minIndent_ >= 8 && !perLine_ ? BK_CENTER : BK_PARA);
    styled(p, n, keep, false);
    out_.endBlock();
    if (start) out_.rule();
  }

  void flushHeading() {
    if (heading_.empty()) return;
    std::string h;
    h.swap(heading_);
    // The contents' line: on one line, without "_italic_" marks.
    std::string title;
    for (size_t i = 0; i < h.size(); i++) {
      char c = h[i];
      bool edge = i == 0 || i + 1 == h.size() || !isalnum((unsigned char)h[i - 1]) || !isalnum((unsigned char)h[i + 1]);
      if (c == '_' && edge) continue;
      title += c == '\n' ? ' ' : c;
    }
    bool part = headingKind_ == H_PART;
    if (part) sawPart_ = true;
    out_.pageBreak();
    out_.tocEntry(title.c_str(), part || !sawPart_ ? 1 : 2);
    out_.beginBlock(part ? BK_H1 : BK_H2);
    styled(h.data(), h.size(), true, false);
    out_.endBlock();
  }

  // Text with "_italic_" (and in Markdown, all of its inline marks); line
  // ends ('\n') become line breaks (keep) or spaces, '\x0B' always a break.
  void styled(const char *p, size_t n, bool keep, bool md) {
    size_t run = 0;  // start of the plain run not yet written
    size_t closeIt = (size_t)-1, closeBd = (size_t)-1;
    auto flush = [&](size_t i) {
      if (i > run) out_.text(p + run, i - run);
    };
    for (size_t i = 0; i < n; i++) {
      char c = p[i];
      if (c == '\n' || c == '\x0B') {
        flush(i);
        if (c == '\x0B' || keep) out_.lineBreak();
        else out_.text(" ", 1);
        run = i + 1;
        continue;
      }
      if (i == closeIt || i == closeBd) {
        flush(i);
        bool bd = i == closeBd;
        size_t len = bd ? 2 : 1;
        if (bd) out_.bold(false), closeBd = (size_t)-1;
        else out_.italic(false), closeIt = (size_t)-1;
        i += len - 1;
        run = i + 1;
        continue;
      }
      if (md && c == '\\' && i + 1 < n && ispunct((unsigned char)p[i + 1])) {
        flush(i);
        run = i + 1;
        i++;
        continue;
      }
      if (md && c == '`') {
        size_t e = i + 1;
        while (e < n && p[e] != '`') e++;
        if (e < n) {
          flush(i);
          out_.text(p + i + 1, e - i - 1);
          i = e;
          run = e + 1;
        }
        continue;
      }
      if (md && (c == '[' || (c == '!' && i + 1 < n && p[i + 1] == '['))) {
        // [text](url) and ![alt](url): the text.
        size_t o = c == '!' ? i + 1 : i, e = o + 1;
        int depth = 1;
        while (e < n && depth) depth += p[e] == '[' ? 1 : p[e] == ']' ? -1 : 0, e += depth ? 1 : 0;
        if (e < n && e + 1 < n && p[e + 1] == '(') {
          const char *close = (const char *)memchr(p + e + 2, ')', n - e - 2);
          if (close) {
            size_t u = (size_t)(close - p);
            flush(i);
            if (c == '!') {
              if (e > o + 1) out_.text("["), out_.text(p + o + 1, e - o - 1), out_.text("]");
              i = u;
              run = u + 1;
            } else {
              // The link's text may hold marks of its own: go on inside it, and skip "](url)" at its end.
              linkEnd_ = e, linkSkip_ = u;
              run = i + 1;
            }
            continue;
          }
        }
      }
      if (md && i == linkEnd_) {
        flush(i);
        i = linkSkip_;
        run = i + 1;
        linkEnd_ = (size_t)-1;
        continue;
      }
      if (md && c == '<') {
        // Inline HTML: <br> a break, other tags left out.
        size_t e = i + 1;
        while (e < n && e - i < 200 && p[e] != '>' && p[e] != '<') e++;
        if (e < n && p[e] == '>' && e > i + 1 && (isalpha((unsigned char)p[i + 1]) || p[i + 1] == '/')) {
          flush(i);
          if ((e - i == 3 || e - i == 4 || e - i == 5) && tolower((unsigned char)p[i + 1]) == 'b' && tolower((unsigned char)p[i + 2]) == 'r') out_.lineBreak();
          i = e;
          run = e + 1;
          continue;
        }
      }
      if (md && c == '&') {
        size_t e = i + 1;
        while (e < n && e - i < 12 && isalnum((unsigned char)p[e])) e++;
        uint32_t cp = e < n && p[e] == ';' ? htmlEntity(p + i + 1, e - i - 1) : 0;
        if (cp) {
          flush(i);
          char b[4];
          out_.text(b, (size_t)utf8Encode(cp, b));
          i = e;
          run = e + 1;
          continue;
        }
      }
      bool star = md && c == '*';
      if (c != '_' && !star) continue;
      // An opening mark: after a space or a punctuation, before a letter;
      // its close: the same mark before a space or a punctuation.
      bool dbl = md && i + 1 < n && p[i + 1] == c;
      size_t len = dbl ? 2 : 1;
      unsigned char before = i ? p[i - 1] : ' ', after = i + len < n ? p[i + len] : ' ';
      bool canOpen = !isalnum(before) && after != ' ' && after != '\n' && after != c;
      if (!canOpen || (dbl ? closeBd != (size_t)-1 : closeIt != (size_t)-1)) continue;
      size_t limit = n < i + 4000 ? n : i + 4000;
      for (size_t k = i + len + 1; k + len <= limit; k++) {
        if (p[k] != c || (dbl && p[k + 1] != c) || (!dbl && k + 1 < n && p[k + 1] == c && md)) continue;
        unsigned char pb = p[k - 1], na = k + len < n ? p[k + len] : ' ';
        if (pb == ' ' || pb == '\n' || isalnum(na)) continue;
        flush(i);
        if (dbl) out_.bold(true), closeBd = k;
        else out_.italic(true), closeIt = k;
        run = i + len;
        i += len - 1;
        break;
      }
    }
    flush(n);
    out_.italic(false), out_.bold(false);
    linkEnd_ = (size_t)-1;
  }

  // ---- Markdown ----

  void markdownLine(const char *p, size_t n, size_t indent, bool hard) {
    bool fenceMark = (startsWith(p, n, "```") || startsWith(p, n, "~~~")) && indent < 4;
    if (fence_) {
      if (fenceMark) {
        fence_ = false;
        out_.endBlock();
        inBlock_ = false;
        return;
      }
      if (!inBlock_) out_.beginBlock(BK_PARA), inBlock_ = true;
      else out_.lineBreak();
      for (size_t k = 0; k < indent; k++) out_.text("\xC2\xA0", 2);  // the code's own indent, kept
      out_.text(p, n);
      return;
    }
    if (fenceMark) {
      endPara(false);
      fence_ = true;
      return;
    }
    // A heading.
    if (p[0] == '#' && indent < 4) {
      size_t level = 0;
      while (level < n && p[level] == '#') level++;
      if (level <= 6 && (level == n || isSpace(p[level]))) {
        endPara(false);
        size_t a = level, e = n;
        while (a < e && isSpace(p[a])) a++;
        while (e > a && p[e - 1] == '#') e--;
        while (e > a && isSpace(p[e - 1])) e--;
        mdHeading(p + a, e - a, (int)level);
        return;
      }
    }
    // A rule, or the line under a heading.
    char c0 = p[0];
    if ((c0 == '-' || c0 == '*' || c0 == '_' || c0 == '=') && indent < 4) {
      size_t marks = 0, k = 0;
      for (; k < n && (p[k] == c0 || isSpace(p[k])); k++) marks += p[k] == c0;
      if (k == n && (marks >= 3 || (c0 == '=' && marks >= 1) || (c0 == '-' && lines_ == 1 && marks >= 1))) {
        if ((c0 == '=' || c0 == '-') && lines_ == 1 && mdKind_ == BK_PARA) {
          std::string h(para_.data(), para_.size());
          lines_ = 0;
          para_.clear();
          mdHeading(h.data(), h.size(), c0 == '=' ? 1 : 2);
          return;
        }
        if (marks >= 3 && c0 != '=') {
          endPara(false);
          out_.rule();
          return;
        }
      }
    }
    // A quote.
    if (c0 == '>') {
      size_t a = 1;
      if (a < n && isSpace(p[a])) a++;
      if (mdKind_ != BK_QUOTE) endPara(false);
      mdKind_ = BK_QUOTE;
      if (a == n) {  // ">" alone: the quote's paragraphs part
        endPara(false);
        mdKind_ = BK_QUOTE;
        return;
      }
      addLine(p + a, n - a, 0, lastHard_ ? '\x0B' : '\n');
      lastHard_ = hard;
      return;
    }
    // A list's item.
    size_t m = 0;
    bool bullet = (c0 == '-' || c0 == '*' || c0 == '+') && n > 1 && isSpace(p[1]);
    if (!bullet) {
      while (m < n && m < 9 && isdigit((unsigned char)p[m])) m++;
    }
    if (bullet || (m && m + 1 < n && (p[m] == '.' || p[m] == ')') && isSpace(p[m + 1]))) {
      endPara(false);
      mdKind_ = BK_ITEM;
      size_t a = bullet ? 2 : m + 2;
      while (a < n && isSpace(p[a])) a++;
      std::string item;
      if (!bullet) item.assign(p, m), item += ". ";
      item.append(p + a, n - a);
      addLine(item.data(), item.size(), 0);
      lastHard_ = hard;
      return;
    }
    if (mdKind_ == BK_QUOTE && lines_ == 0) mdKind_ = BK_PARA;
    if (mdKind_ != BK_PARA && blank_) endPara(false), mdKind_ = BK_PARA;
    addLine(p, n, 0, lastHard_ ? '\x0B' : '\n');
    lastHard_ = hard;
  }

  void mdHeading(const char *p, size_t n, int level) {
    std::string title;
    for (size_t i = 0; i < n; i++)
      if (p[i] != '*' && p[i] != '_' && p[i] != '`') title += p[i];
    decodeEntities(title);
    if (level == 1) out_.pageBreak();
    out_.tocEntry(title.c_str(), level > 3 ? 3 : level);
    out_.beginBlock(level == 1 ? BK_H1 : level == 2 ? BK_H2 : BK_H3);
    styled(p, n, false, true);
    out_.endBlock();
    mdKind_ = BK_PARA;
  }

  void markdownPara(const char *p, size_t n, int) {
    out_.beginBlock((BlockKind)mdKind_);
    styled(p, n, false, true);
    out_.endBlock();
    if (mdKind_ == BK_ITEM) mdKind_ = BK_PARA;
    lastHard_ = false;
  }

  FlowWriter &out_;
  BookMeta &meta_;
  FILE *f_ = nullptr;
  uint8_t *buf_ = nullptr;
  long size_ = 0, done_ = 0;
  size_t skip_ = 0;
  HtmlToFlow *html_ = nullptr;
  CharsetDecoder dec_;
  std::string tmp_;
  PsVec<char> line_, para_;
  bool lastCR_ = false, markdown_ = false;
  // The shape.
  bool perLine_ = false, indentParas_ = false;
  size_t width_ = 72;
  uint32_t widths_[100] = {0};
  // The paragraph being gathered.
  int lines_ = 0;
  size_t lineStarts_[3] = {0, 0, 0};
  size_t minIndent_ = 0;
  bool allShort_ = true, laterIndented_ = false, startedAfterBlank_ = true, blank_ = true;
  // A heading waiting for the line after it.
  std::string heading_;
  Heading headingKind_ = H_NONE;
  int headingLines_ = 0;
  bool sawPart_ = false;
  // Markdown.
  uint8_t mdKind_ = BK_PARA;
  bool fence_ = false, inBlock_ = false, lastHard_ = false;
  size_t linkEnd_ = (size_t)-1, linkSkip_ = 0;
};

}  // namespace

BookJob *textJob(const char *path, FlowWriter &out, BookFormat format, BookMeta &meta, BookErr &err) {
  TextJob *job = new TextJob(out, meta);
  if (!job->start(path, format, err)) {
    delete job;
    return nullptr;
  }
  return job;
}

bool textProbe(const char *path, BookFormat format, BookMeta &meta, BookErr &err) {
  FILE *f = fopen(path, "rb");
  if (!f) return err = BE_OPEN, false;
  uint8_t *head = (uint8_t *)psMalloc(8192);
  if (!head) return fclose(f), err = BE_MEMORY, false;
  size_t n = fread(head, 1, format == BF_HTML ? 8192 : 4096, f);
  fclose(f);
  size_t skip;
  Charset cs = detect(head, n, skip);
  CharsetDecoder dec(cs);
  std::string text;
  dec.feed((const char *)head + skip, n - skip, text);
  free(head);
  if (format != BF_HTML) {
    gutenbergMeta(text, meta);
    return true;
  }
  htmlDescription(text, meta.description);
  // <title> of an HTML file (in its first 8 KB).
  std::string low;
  for (char c : text) low += (char)tolower((unsigned char)c);
  size_t a = low.find("<title"), b = a == std::string::npos ? a : low.find('>', a), e = b == std::string::npos ? b : low.find("</title", b);
  if (e != std::string::npos) {
    std::string t = text.substr(b + 1, e - b - 1), s;
    decodeEntities(t);
    for (char c : t) {
      if (isspace((unsigned char)c)) {
        if (s.size() && s.back() != ' ') s += ' ';
      } else {
        s += c;
      }
    }
    while (s.size() && s.back() == ' ') s.pop_back();
    meta.title = s.substr(0, 300);
  }
  return true;
}
