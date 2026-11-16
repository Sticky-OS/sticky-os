// HtmlToFlow (bookformats.h): HTML and XHTML, as books have it, to the flow.
//
// A forgiving tokenizer that keeps its place between any two bytes: the
// markup comes in whatever pieces the zip or the card gives. It doesn't build
// a tree. What it remembers is small: the tag being read (its first 1 KB),
// a stack of the open containers that change how their text looks (lists,
// quotes, centred or hidden parts; 32 deep, deeper ones are only counted),
// and counters for italic and bold.
//
// Blocks are opened late, at their first visible character, so empty
// paragraphs cost nothing and an id is always reported between two blocks or
// with the block it is in. With tocFromHeadings a heading is held back until
// its end (600 bytes at most), because its contents entry goes before it.
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "bookcharset.h"
#include "bookformats.h"

namespace {

enum Tag : uint8_t {
  T_NONE, T_A, T_B, T_BLOCK, T_BLOCKQUOTE, T_BODY, T_BR, T_CAPTION, T_CENTER, T_DD, T_H1, T_H2, T_H3, T_H4, T_H5, T_H6,
  T_HEAD, T_HR, T_I, T_IMG, T_LI, T_META, T_OL, T_P, T_PAGEBREAK, T_PRE, T_Q, T_RAW, T_SKIP, T_SPAN, T_TD, T_TITLE, T_TR, T_UL
};

// In strcmp order. T_BLOCK: a container that only ends the paragraph before
// and after it. T_RAW: its contents aren't markup. T_SKIP: nothing of it is text.
const struct { char name[11]; uint8_t tag; } TAGS[] = {
    {"a", T_A}, {"address", T_BLOCK}, {"article", T_BLOCK}, {"aside", T_BLOCK}, {"b", T_B}, {"blockquote", T_BLOCKQUOTE},
    {"body", T_BODY}, {"br", T_BR}, {"caption", T_CAPTION}, {"center", T_CENTER}, {"cite", T_I}, {"dd", T_DD},
    {"details", T_BLOCK}, {"dfn", T_I}, {"dir", T_UL}, {"div", T_BLOCK}, {"dl", T_BLOCK}, {"dt", T_DD},
    {"em", T_I}, {"fieldset", T_BLOCK}, {"figcaption", T_CAPTION}, {"figure", T_BLOCK}, {"footer", T_BLOCK}, {"form", T_BLOCK},
    {"h1", T_H1}, {"h2", T_H2}, {"h3", T_H3}, {"h4", T_H4}, {"h5", T_H5}, {"h6", T_H6},
    {"head", T_HEAD}, {"header", T_BLOCK}, {"hgroup", T_BLOCK}, {"hr", T_HR}, {"i", T_I}, {"img", T_IMG},
    {"li", T_LI}, {"main", T_BLOCK}, {"math", T_SKIP}, {"menu", T_UL}, {"meta", T_META}, {"nav", T_BLOCK},
    {"ol", T_OL}, {"p", T_P}, {"pagebreak", T_PAGEBREAK}, {"pre", T_PRE}, {"q", T_Q}, {"rp", T_SKIP},
    {"script", T_RAW}, {"section", T_BLOCK}, {"span", T_SPAN}, {"strong", T_B}, {"style", T_RAW}, {"summary", T_BLOCK},
    {"svg", T_SKIP}, {"table", T_BLOCK}, {"tbody", T_BLOCK}, {"td", T_TD}, {"template", T_SKIP}, {"tfoot", T_BLOCK},
    {"th", T_TD}, {"thead", T_BLOCK}, {"title", T_TITLE}, {"tr", T_TR}, {"ul", T_UL}, {"var", T_I},
};

Tag tagOf(const char *name) {
  int lo = 0, hi = (int)(sizeof TAGS / sizeof TAGS[0]) - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2, c = strcmp(name, TAGS[mid].name);
    if (!c) return (Tag)TAGS[mid].tag;
    if (c < 0) hi = mid - 1;
    else lo = mid + 1;
  }
  return T_NONE;
}

enum State : uint8_t { ST_TEXT, ST_LT, ST_TAG, ST_BANG, ST_COMMENT, ST_CDATA, ST_DECL, ST_PI, ST_ENTITY, ST_RAW, ST_RAW_LT, ST_RAW_END };

enum : uint8_t { F_CENTER = 1, F_QUOTE = 2 };            // of a container on the stack
enum : char { M_IT_ON = 1, M_IT_OFF, M_BD_ON, M_BD_OFF, M_BREAK };  // in a held-back heading

const size_t TAG_MAX = 1024;    // of a tag, what is kept (a picture written into its src can be megabytes)
const size_t HEADING_MAX = 600;
const size_t DEFER_MAX = 512;   // ids waiting for their block's end
const int STACK_MAX = 32, SPAN_MAX = 24;

bool isSpace(unsigned char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'; }

bool hasNoCase(const std::string &s, const char *what) {
  size_t n = strlen(what);
  for (size_t i = 0; i + n <= s.size(); i++) {
    size_t k = 0;
    while (k < n && tolower((unsigned char)s[i + k]) == what[k]) k++;
    if (k == n) return true;
  }
  return false;
}

// One attribute after another of a tag's inside ("a href='x' id=y"): false at the end.
bool nextAttr(const char *&p, const char *end, std::string &name, std::string &value) {
  name.clear(), value.clear();
  while (p < end && (isSpace(*p) || *p == '/')) p++;
  if (p >= end) return false;
  while (p < end && !isSpace(*p) && *p != '=' && *p != '/') name += (char)tolower((unsigned char)*p++);
  while (p < end && isSpace(*p)) p++;
  if (p >= end || *p != '=') return true;
  p++;
  while (p < end && isSpace(*p)) p++;
  if (p < end && (*p == '"' || *p == '\'')) {
    char q = *p++;
    while (p < end && *p != q) value += *p++;
    if (p < end) p++;
  } else {
    while (p < end && !isSpace(*p)) value += *p++;
  }
  return true;
}

// Does a picture's alt say something? Not when it's empty or the file's name.
bool altSaysSomething(const std::string &alt) {
  static const char *const EXT[] = {".jpg", ".jpeg", ".png", ".gif", ".svg", ".webp", ".bmp", ".tif", ".tiff"};
  bool letters = false;
  for (size_t i = 0; i < alt.size(); i++) {
    unsigned char c = alt[i];
    if (c == 0xC2 && i + 1 < alt.size() && (uint8_t)alt[i + 1] == 0xA0) i++;  // a no-break space is no word
    else if (c >= 0x80 || isalnum(c)) letters = true;
  }
  if (!letters) return false;
  // "cover", "Cover image": a cover page's picture, which says nothing.
  if (alt.size() < 20 && hasNoCase(alt, "cover")) return false;
  for (const char *e : EXT) {
    size_t n = strlen(e);
    if (alt.size() >= n && hasNoCase(alt.substr(alt.size() - n), e)) return false;
  }
  return true;
}

}  // namespace

struct HtmlToFlow::Impl {
  FlowWriter &out;
  explicit Impl(FlowWriter &o) : out(o) {}

  // The bytes' encoding.
  Charset cs = CS_UTF8, defaultCs = CS_UTF8;
  bool sniffed = false;    // the first bytes have been looked at for a byte order mark
  bool csLocked = false;   // the bytes themselves said (a mark, UTF-16): declarations don't count
  bool csChanged = false;  // a declaration just changed cs: the bytes after it are read anew
  uint8_t head[3];
  int headLen = 0;
  CharsetDecoder utf16;
  std::string tmp;
  uint32_t fed = 0;
  bool fresh = true;       // nothing of this document fed yet

  // The tokenizer.
  uint8_t st = ST_TEXT;
  std::string tag;         // the tag, entity or declaration being read
  char quote = 0, prev = 0;
  int match = 0;           // how much of "-->" or "]]>" has been seen; a declaration's [ ] depth
  const char *rawName = nullptr;  // "script" or "style": inside it

  // The document.
  struct Open { uint8_t tag, flags; uint16_t n; };  // n: a list's number so far; of plain containers, how many
  Open stack[STACK_MAX];
  int depth = 0, overflow = 0, quoteDepth = 0, centerDepth = 0;
  uint8_t spans[SPAN_MAX];
  int spanDepth = 0, spanOver = 0;
  int italic = 0, bold = 0, pre = 0, heading = 0;
  bool curIt = false, curBd = false;
  bool pCenter = false, itemPending = false, inHead = false, inTitle = false;
  int itemNumber = 0;
  char skipName[16] = "";
  int skipDepth = 0;
  bool blockOpen = false, anyBlock = false;  // anyBlock: this document has shown some text
  uint32_t blockStart = 0;
  int brRun = 0;
  bool tocHeadings = false, hbufOn = false, hIt = false, hBd = false;
  std::string hbuf;
  void (*anchorFn)(void *, const char *, uint32_t) = nullptr;
  void *anchorArg = nullptr;
  std::string deferIds;    // ids met inside an open block, each ended by a NUL
  std::string title;

  // ---- blocks ----

  void openBlock() {
    BlockKind kind = heading ? (heading == 1 ? BK_H1 : heading == 2 ? BK_H2 : BK_H3)
                   : itemPending ? BK_ITEM : quoteDepth ? BK_QUOTE : (pCenter || centerDepth) ? BK_CENTER : BK_PARA;
    blockStart = out.offset();
    out.beginBlock(kind);
    blockOpen = anyBlock = true;
    brRun = 0;
    if (kind == BK_ITEM) {
      itemPending = false;
      if (itemNumber > 0) {
        char num[16];
        snprintf(num, sizeof num, "%d. ", itemNumber);
        out.text(num);
      }
    }
    out.italic(curIt);
    out.bold(curBd);
  }

  // The end of a paragraph, whatever ends it.
  void boundary() {
    if (hbufOn) flushHeading();
    if (blockOpen) {
      out.endBlock();
      blockOpen = false;
      // The ids that were inside it: now, between two blocks, where a listener may write.
      if (deferIds.size()) {
        std::string ids;
        ids.swap(deferIds);
        for (size_t i = 0; i < ids.size(); i += strlen(ids.c_str() + i) + 1) anchorFn(anchorArg, ids.c_str() + i, blockStart);
      }
    }
    brRun = 0;
  }

  void anchor(const std::string &id) {
    if (!anchorFn || id.empty()) return;
    if (!blockOpen) anchorFn(anchorArg, id.c_str(), out.offset());
    else if (deferIds.size() + id.size() < DEFER_MAX) deferIds.append(id.c_str(), strlen(id.c_str()) + 1);
  }

  void setStyle() {
    bool it = italic > 0, bd = bold > 0;
    if (hbufOn) {
      if (it != curIt) hbuf += it ? M_IT_ON : M_IT_OFF;
      if (bd != curBd) hbuf += bd ? M_BD_ON : M_BD_OFF;
    }
    curIt = it, curBd = bd;
    if (blockOpen) out.italic(it), out.bold(bd);
  }

  void lineBreak() {
    if (hbufOn) {
      hbuf += M_BREAK;
    } else if (blockOpen) {
      // Two in a row with nothing between: the way careless HTML ends a paragraph.
      if (++brRun >= 2 && !pre) boundary();
      else out.lineBreak();
    }
  }

  // Text of the paragraph (never called with nothing).
  void plain(const char *p, size_t n) {
    if (hbufOn) {
      for (size_t i = 0; i < n; i++) hbuf += (unsigned char)p[i] < 0x20 ? ' ' : p[i];
      if (hbuf.size() > HEADING_MAX) flushHeading();  // no heading is this long: held back no more
      return;
    }
    if (blockOpen) {
      for (size_t i = 0; i < n && brRun; i++)
        if (!isSpace(p[i])) brRun = 0;
      out.text(p, n);
      return;
    }
    // Is there anything to see? Spaces, and no-break ones, don't start a paragraph.
    size_t i = 0;
    for (;;) {
      while (i < n && isSpace(p[i])) i++;
      if (i + 1 < n && (uint8_t)p[i] == 0xC2 && (uint8_t)p[i + 1] == 0xA0) i += 2;
      else break;
    }
    if (i == n) return;
    openBlock();
    out.text(p + i, n - i);
  }

  void text(const char *p, size_t n) {
    if (skipDepth || !n) return;
    if (inTitle) {
      if (title.size() < 300) title.append(p, n < 300 ? n : 300);
      return;
    }
    if (inHead) return;
    if (!pre) return plain(p, n);
    // Preformatted: the lines are kept.
    size_t from = 0;
    for (size_t i = 0; i < n; i++) {
      if (p[i] != '\n') continue;
      if (i > from) plain(p + from, i - from);
      lineBreak();
      from = i + 1;
    }
    if (n > from) plain(p + from, n - from);
  }

  void character(uint32_t cp) {
    char b[4];
    text(b, utf8Encode(cp, b));
  }

  // A heading that was held back: its contents entry, then the heading itself.
  void flushHeading() {
    hbufOn = false;
    std::string held, name;
    held.swap(hbuf);
    for (char c : held) {
      if (c == M_BREAK) name += ' ';
      else if ((unsigned char)c >= 0x20) name += c;
    }
    if (heading >= 1 && heading <= 3) out.tocEntry(name.c_str(), heading);
    bool endIt = curIt, endBd = curBd;
    curIt = hIt, curBd = hBd;
    size_t from = 0;
    for (size_t i = 0; i <= held.size(); i++) {
      if (i < held.size() && (unsigned char)held[i] >= 0x20) continue;
      if (i > from) plain(held.data() + from, i - from);
      from = i + 1;
      if (i == held.size()) break;
      switch (held[i]) {
        case M_IT_ON: curIt = true; break;
        case M_IT_OFF: curIt = false; break;
        case M_BD_ON: curBd = true; break;
        case M_BD_OFF: curBd = false; break;
        default: lineBreak(); break;
      }
      if (blockOpen) out.italic(curIt), out.bold(curBd);
    }
    curIt = endIt, curBd = endBd;
    if (blockOpen) out.italic(curIt), out.bold(curBd);
  }

  // ---- the containers that matter ----

  void count(const Open &o, int by) {
    if (o.flags & F_CENTER) centerDepth += by;
    if (o.flags & F_QUOTE) quoteDepth += by;
  }

  void push(uint8_t tag, uint8_t flags, uint16_t n) {
    if (tag == T_BLOCK && !flags && depth && stack[depth - 1].tag == T_BLOCK && !stack[depth - 1].flags && stack[depth - 1].n < 0xFFFF) {
      stack[depth - 1].n++;  // plain containers one in another: counted, not stacked
      return;
    }
    if (depth == STACK_MAX) {
      overflow++;
      return;
    }
    stack[depth] = {tag, flags, n};
    count(stack[depth++], 1);
  }

  void pop(uint8_t tag) {
    if (overflow) {
      overflow--;
      return;
    }
    int i = depth - 1;
    while (i >= 0 && stack[i].tag != tag) i--;
    if (i < 0) return;  // an end with no start
    while (depth > i + 1) count(stack[--depth], -1);  // what was left open inside it ends with it
    if (tag == T_BLOCK && !stack[i].flags && stack[i].n > 1) stack[i].n--;
    else count(stack[--depth], -1);
  }

  void pushSpan(uint8_t flags) {
    if (spanDepth == SPAN_MAX) {
      spanOver++;
      return;
    }
    spans[spanDepth++] = flags;
    if (flags & 1) italic++;
    if (flags & 2) bold++;
    if (flags) setStyle();
  }

  void popSpan() {
    if (spanOver) {
      spanOver--;
      return;
    }
    if (!spanDepth) return;
    uint8_t flags = spans[--spanDepth];
    if ((flags & 1) && italic > 0) italic--;
    if ((flags & 2) && bold > 0) bold--;
    if (flags) setStyle();
  }

  // ---- encodings ----

  void declare(const char *name, size_t n) {
    Charset want = cs;
    if (csLocked || !charsetByName(name, n, want) || want == CS_UTF16LE || want == CS_UTF16BE || want == cs) return;
    cs = want;
    csChanged = true;
  }

  // ---- tags ----

  void processingInstruction() {
    // <?xml version="1.0" encoding="iso-8859-1"?>
    if (tag.compare(0, 3, "xml") || tag.size() < 4 || !isSpace(tag[3])) return;
    const char *p = tag.c_str() + 3, *end = tag.c_str() + tag.size();
    std::string name, value;
    while (nextAttr(p, end, name, value))
      if (name == "encoding") declare(value.data(), value.size());
  }

  void endOfTag() {
    const char *p = tag.c_str(), *end = p + tag.size();
    bool closing = *p == '/';
    if (closing) p++;
    char name[16];
    size_t n = 0;
    for (; p < end && !isSpace(*p) && *p != '/'; p++) {
      if (*p == ':') n = 0;  // a namespace: "svg:svg", "mbp:pagebreak"
      else if (n < sizeof name - 1) name[n++] = (char)tolower((unsigned char)*p);
    }
    name[n] = 0;
    size_t last = tag.size();
    while (last > 0 && isSpace(tag[last - 1])) last--;
    bool selfClose = !closing && last > 0 && tag[last - 1] == '/';
    Tag t = tagOf(name);

    // What an opening tag says about itself.
    std::string id, aname, alt, content, charset;
    bool center = false, hidden = false, styleIt = false, styleBd = false, contentType = false;
    int start = 1;
    if (!closing) {
      std::string an, av;
      while (nextAttr(p, end, an, av)) {
        if (an == "id") id = av;
        else if (an == "name" && t == T_A) aname = av;
        else if (an == "alt" && t == T_IMG) alt = av;
        else if (an == "hidden") hidden = true;
        else if (an == "align") center = center || hasNoCase(av, "center");
        else if (an == "class") center = center || hasNoCase(av, "center") || hasNoCase(av, "centre");
        else if (an == "start" && t == T_OL) start = atoi(av.c_str());
        else if (an == "charset" && t == T_META) charset = av;
        else if (an == "content" && t == T_META) content = av;
        else if (an == "http-equiv" && t == T_META) contentType = hasNoCase(av, "content-type");
        else if (an == "style") {
          std::string s;  // without its spaces: "text-align : center" is "text-align:center"
          for (char c : av)
            if (!isSpace(c)) s += c;
          center = center || hasNoCase(s, "text-align:center");
          hidden = hidden || hasNoCase(s, "display:none") || hasNoCase(s, "visibility:hidden");
          styleIt = hasNoCase(s, "font-style:italic") || hasNoCase(s, "font-style:oblique");
          styleBd = hasNoCase(s, "font-weight:bold") || hasNoCase(s, "font-weight:6") || hasNoCase(s, "font-weight:7") ||
                    hasNoCase(s, "font-weight:8") || hasNoCase(s, "font-weight:9");
        }
      }
      decodeEntities(id);
      decodeEntities(aname);
    }

    // Inside something that isn't shown: only its own end (and what nests in it) counts.
    if (skipDepth) {
      if (!closing) anchor(id), anchor(aname);
      if (!strcmp(name, skipName)) {
        if (closing) skipDepth--;
        else if (!selfClose) skipDepth++;
      }
      return;
    }
    bool isVoid = t == T_BR || t == T_HR || t == T_IMG || t == T_META || t == T_PAGEBREAK || selfClose;
    if (!closing && !isVoid && (hidden || t == T_SKIP) && t != T_BODY && t != T_HEAD && n) {
      anchor(id);
      snprintf(skipName, sizeof skipName, "%s", name);
      skipDepth = 1;
      return;
    }

    if (closing) {
      switch (t) {
        case T_P: case T_DD: case T_CAPTION: case T_TR: boundary(); pCenter = false; break;
        case T_BLOCK: case T_BLOCKQUOTE: case T_CENTER: case T_UL: case T_OL: boundary(); pop(t); break;
        case T_BODY: boundary(); break;
        case T_LI: boundary(); itemPending = false; break;
        case T_H1: case T_H2: case T_H3: case T_H4: case T_H5: case T_H6: boundary(); heading = 0; break;
        case T_PRE: boundary(); if (pre) pre--; break;
        case T_I: if (italic) italic--; setStyle(); break;
        case T_B: if (bold) bold--; setStyle(); break;
        case T_SPAN: popSpan(); break;
        case T_Q: character(0x201D); break;
        case T_BR: lineBreak(); break;  // "</br>": a line break, as browsers have it
        case T_HEAD: inHead = false; break;
        case T_TITLE:
          if (inTitle) {
            // On one line, as a name is.
            std::string t;
            bool gap = false;
            for (char ch : title) {
              if (isSpace(ch)) gap = t.size() > 0;
              else t += gap ? " " : "", t += ch, gap = false;
            }
            title.swap(t);
          }
          inTitle = false;
          break;
        default: break;
      }
      return;
    }

    if (selfClose && t != T_BR && t != T_HR && t != T_IMG && t != T_META && t != T_PAGEBREAK) {
      // "<pre/>", "<p/>", "<a id='x'/>": nothing opens.
      bool inlineTag = t == T_A || t == T_I || t == T_B || t == T_SPAN || t == T_Q || t == T_TD || t == T_NONE || t == T_RAW || t == T_TITLE;
      if (!inlineTag) boundary();
      anchor(id);
      if (aname != id) anchor(aname);
      return;
    }
    switch (t) {
      case T_P: boundary(); anchor(id); pCenter = center; break;
      case T_BLOCK: boundary(); anchor(id); push(T_BLOCK, center ? F_CENTER : 0, 1); break;
      case T_BLOCKQUOTE: boundary(); anchor(id); push(T_BLOCKQUOTE, F_QUOTE, 0); break;
      case T_CENTER: boundary(); anchor(id); push(T_CENTER, F_CENTER, 0); break;
      case T_UL: boundary(); anchor(id); push(T_UL, 0, 0); break;
      case T_OL: boundary(); anchor(id); push(T_OL, 0, (uint16_t)(start > 0 && start < 60000 ? start - 1 : 0)); break;
      case T_LI: {
        boundary();
        anchor(id);
        itemPending = true, itemNumber = 0;
        int i = depth - 1;
        while (i >= 0 && stack[i].tag != T_OL && stack[i].tag != T_UL) i--;
        if (i >= 0 && stack[i].tag == T_OL && !overflow) itemNumber = stack[i].n < 0xFFFF ? ++stack[i].n : 0;
        break;
      }
      case T_DD: case T_TR: boundary(); anchor(id); pCenter = false; break;
      case T_CAPTION: boundary(); anchor(id); pCenter = true; break;
      case T_TD:
        anchor(id);
        if (blockOpen) out.text(" ");  // a row's cells: one line, a space between
        break;
      case T_BODY: inHead = inTitle = false; boundary(); anchor(id); break;
      case T_H1: case T_H2: case T_H3: case T_H4: case T_H5: case T_H6:
        boundary();
        anchor(id);
        heading = t - T_H1 + 1;
        if (tocHeadings && heading <= 3) hbufOn = true, hbuf.clear(), hIt = curIt, hBd = curBd;
        break;
      case T_HR: boundary(); anchor(id); out.rule(); break;
      case T_BR: anchor(id); lineBreak(); break;
      case T_PRE: boundary(); anchor(id); pre++; break;
      case T_PAGEBREAK: boundary(); out.pageBreak(); break;
      case T_IMG:
        anchor(id);
        decodeEntities(alt);
        if (altSaysSomething(alt)) {
          // In brackets, as a picture's words: not a picture of a letter (a
          // drop capital), of a heading's words, or words already in brackets.
          size_t chars = 0;
          for (unsigned char c : alt) chars += (c & 0xC0) != 0x80;
          if (chars > 1 && !heading && alt[0] != '[') alt = "[" + alt + "]";
          text(alt.data(), alt.size());
        }
        break;
      case T_I: anchor(id); italic++, setStyle(); break;
      case T_B: anchor(id); bold++, setStyle(); break;
      case T_SPAN: anchor(id); pushSpan((styleIt ? 1 : 0) | (styleBd ? 2 : 0)); break;
      case T_A: anchor(id); if (aname != id) anchor(aname); break;
      case T_Q: anchor(id); character(0x201C); break;
      case T_HEAD: inHead = true; break;
      case T_TITLE: if ((inHead || !anyBlock) && title.empty()) inTitle = true; break;
      case T_META:
        if (charset.size()) declare(charset.data(), charset.size());
        else if (contentType) {
          size_t at = 0;
          for (; at + 8 <= content.size(); at++)
            if (hasNoCase(content.substr(at, 8), "charset=")) break;
          if (at + 8 <= content.size()) {
            size_t from = at + 8, to = from;
            while (to < content.size() && content[to] != ';' && !isSpace(content[to])) to++;
            declare(content.data() + from, to - from);
          }
        }
        break;
      case T_RAW:
        rawName = name[1] == 'c' ? "script" : "style";
        break;
      default: anchor(id); break;
    }
  }

  // ---- the tokenizer ----

  // An entity's name is whole (ended is false when it wasn't closed by ';').
  void endOfEntity(bool ended) {
    uint32_t cp = 0;
    if (tag.size() > 1 && tag[0] == '#') {
      bool hex = tag[1] == 'x' || tag[1] == 'X', ok = tag.size() > (hex ? 2u : 1u);
      for (size_t i = hex ? 2 : 1; i < tag.size() && ok; i++) {
        unsigned char c = tag[i];
        if (hex ? !isxdigit(c) : !isdigit(c)) ok = false;
        else cp = hex ? cp * 16 + (isdigit(c) ? c - '0' : tolower(c) - 'a' + 10) : cp * 10 + (c - '0');
        if (cp > 0x10FFFF) cp = 0xFFFD;
      }
      if (!ok) cp = 0;
      else if (!cp) cp = 0xFFFD;
      else if (cp >= 0x80 && cp < 0xA0) cp = charsetHigh(CS_1252, (uint8_t)cp);  // "&#151;": what the writer saw on Windows
    } else if (ended || tag == "amp" || tag == "lt" || tag == "gt" || tag == "quot" || tag == "nbsp") {
      cp = htmlEntity(tag.data(), tag.size());  // (those few work without their ';' too, as in browsers)
    }
    if (cp) return character(cp);
    // Not an entity: the text it is.
    std::string lit = "&" + tag + (ended ? ";" : "");
    text(lit.data(), lit.size());
  }

  // Takes the markup (UTF-8 by now). Returns how much it took: all of it,
  // unless a declaration changed the encoding of what follows.
  size_t tokenize(const char *p, size_t n) {
    size_t i = 0;
    while (i < n) {
      unsigned char c = p[i];
      switch (st) {
        case ST_TEXT: {
          size_t j = i;
          while (j < n && p[j] != '<' && p[j] != '&') j++;
          if (j > i) text(p + i, j - i);
          i = j;
          if (i < n) {
            st = p[i] == '<' ? ST_LT : ST_ENTITY;
            tag.clear();
            i++;
          }
          break;
        }
        case ST_LT:
          if (c == '!') st = ST_BANG, i++;
          else if (c == '?') st = ST_PI, i++;
          else if (isalpha(c) || c == '/') st = ST_TAG, quote = 0, prev = 0;
          else text("<", 1), st = ST_TEXT;  // "a < b"
          break;
        case ST_TAG:
          i++;
          if (quote) {
            if (c == quote) quote = 0;
          } else if (c == '>') {
            st = ST_TEXT;
            endOfTag();
            if (rawName) st = ST_RAW;
            if (csChanged) return i;
            break;
          } else if ((c == '"' || c == '\'') && prev == '=') {
            quote = (char)c;
          }
          if (!isSpace(c)) prev = (char)c;
          if (tag.size() < TAG_MAX) tag += (char)c;
          break;
        case ST_BANG:
          tag += (char)c;
          if (tag == "--") st = ST_COMMENT, match = 0, i++;
          else if (tag == "[CDATA[") st = ST_CDATA, match = 0, i++;
          else if (!strncmp("--", tag.c_str(), tag.size()) || !strncmp("[CDATA[", tag.c_str(), tag.size())) i++;
          else st = ST_DECL, match = 0;  // <!DOCTYPE ...>: this byte is looked at again there
          break;
        case ST_COMMENT:
          i++;
          if (c == '-') match++;
          else if (c == '>' && match >= 2) st = ST_TEXT;
          else match = 0;
          break;
        case ST_CDATA:
          i++;
          if (c == ']') {
            if (++match > 2) text("]", 1), match = 2;
          } else if (c == '>' && match >= 2) {
            st = ST_TEXT;
          } else {
            for (; match > 0; match--) text("]", 1);
            text((const char *)&p[i - 1], 1);
          }
          break;
        case ST_DECL:
          i++;
          if (c == '[') match++;
          else if (c == ']') match--;
          else if (c == '>' && match <= 0) st = ST_TEXT;
          break;
        case ST_PI:
          i++;
          if (c == '>') {
            st = ST_TEXT;
            processingInstruction();
            if (csChanged) return i;
          } else if (tag.size() < 256) {
            tag += (char)c;
          }
          break;
        case ST_ENTITY:
          if (c == ';') {
            i++;
            st = ST_TEXT;
            endOfEntity(true);
          } else if ((isalnum(c) || c == '#') && tag.size() < 32) {
            tag += (char)c;
            i++;
          } else {
            st = ST_TEXT;  // and this byte is text, or the next tag's start
            endOfEntity(false);
          }
          break;
        case ST_RAW: {
          const char *lt = (const char *)memchr(p + i, '<', n - i);
          if (!lt) return n;
          i = (size_t)(lt - p) + 1;
          st = ST_RAW_LT;
          tag.clear();
          break;
        }
        case ST_RAW_LT: {
          // "</script" or "</style", in any case: its end. Anything else was script.
          size_t at = tag.size(), len = strlen(rawName);
          bool fits = at == 0 ? c == '/' : at <= len ? tolower(c) == rawName[at - 1] : !isalnum(c);
          if (!fits) {
            st = ST_RAW;
          } else if (at == len + 1) {
            st = ST_RAW_END;
            rawName = nullptr;
          } else {
            tag += (char)c;
            i++;
          }
          break;
        }
        case ST_RAW_END:
          i++;
          if (c == '>') st = ST_TEXT;
          break;
      }
    }
    return n;
  }

  void tokenizeAll(const char *p, size_t n) {
    while (n) {
      size_t k = tokenize(p, n);
      p += k, n -= k;
    }
  }

  // Bytes in the document's encoding.
  void run(const char *p, size_t n) {
    while (n) {
      csChanged = false;
      if (cs == CS_UTF8) {
        size_t k = tokenize(p, n);
        p += k, n -= k;
      } else if (cs == CS_UTF16LE || cs == CS_UTF16BE) {
        size_t k = n < 256 ? n : 256;
        tmp.clear();
        utf16.feed(p, k, tmp);
        tokenizeAll(tmp.data(), tmp.size());
        p += k, n -= k;
      } else {
        // One byte a character: the markup is ASCII as it is, the rest is looked up.
        size_t k = 0;
        while (k < n && (uint8_t)p[k] < 0x80) k++;
        if (k) {
          k = tokenize(p, k);
        } else {
          char b[4];
          tokenizeAll(b, utf8Encode(charsetHigh(cs, (uint8_t)p[0]), b));
          k = 1;
        }
        p += k, n -= k;
      }
    }
  }

  // The first bytes: a byte order mark, or UTF-16 without one ("<" and a zero).
  void sniff() {
    sniffed = true;
    int skip = 0;
    if (headLen >= 3 && head[0] == 0xEF && head[1] == 0xBB && head[2] == 0xBF) cs = CS_UTF8, csLocked = true, skip = 3;
    else if (headLen >= 2 && head[0] == 0xFF && head[1] == 0xFE) cs = CS_UTF16LE, skip = 2;
    else if (headLen >= 2 && head[0] == 0xFE && head[1] == 0xFF) cs = CS_UTF16BE, skip = 2;
    else if (headLen >= 2 && head[0] == '<' && head[1] == 0) cs = CS_UTF16LE;
    else if (headLen >= 2 && head[0] == 0 && head[1] == '<') cs = CS_UTF16BE;
    if (cs == CS_UTF16LE || cs == CS_UTF16BE) csLocked = true, utf16.reset(cs);
    run((const char *)head + skip, (size_t)(headLen - skip));
  }

  void feed(const char *p, size_t n) {
    if (fresh) fresh = false, fed = 0, title.clear();
    fed += (uint32_t)n;
    if (!sniffed) {
      while (n && headLen < 3) head[headLen++] = (uint8_t)*p++, n--;
      if (headLen < 3) return;
      sniff();
    }
    run(p, n);
  }

  void finish() {
    if (!sniffed && headLen) sniff();
    // What was cut off in the middle.
    if (st == ST_LT) text("<", 1);
    else if (st == ST_ENTITY) endOfEntity(false);
    boundary();
    deferIds.clear();
    cs = defaultCs;
    sniffed = csLocked = csChanged = false;
    headLen = 0;
    fresh = true;
    st = ST_TEXT;
    tag.clear();
    rawName = nullptr;
    depth = overflow = quoteDepth = centerDepth = spanDepth = spanOver = 0;
    italic = bold = pre = heading = brRun = skipDepth = itemNumber = 0;
    curIt = curBd = pCenter = itemPending = inHead = inTitle = anyBlock = false;
  }
};

HtmlToFlow::HtmlToFlow(FlowWriter &out) : d(new Impl(out)) {}
HtmlToFlow::~HtmlToFlow() { delete d; }

void HtmlToFlow::setCharset(const char *name) {
  Charset cs = CS_1252;  // a name we don't know: the commonest of the old ones
  charsetByName(name ? name : "", name ? strlen(name) : 0, cs);
  d->defaultCs = cs;
  if (!d->sniffed) d->cs = cs;
}

void HtmlToFlow::tocFromHeadings(bool on) { d->tocHeadings = on; }
void HtmlToFlow::onAnchor(void (*fn)(void *, const char *, uint32_t), void *arg) { d->anchorFn = fn, d->anchorArg = arg; }
uint32_t HtmlToFlow::bytesFed() const { return d->fed; }
void HtmlToFlow::feed(const char *data, size_t n) { d->feed(data, n); }
void HtmlToFlow::finish() { d->finish(); }
const std::string &HtmlToFlow::title() const { return d->title; }
