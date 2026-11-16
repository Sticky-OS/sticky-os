// For the tests: is a flow (bookflow.h) what its grammar says, and a
// readable dump of one. Header only, so any *_test.cpp can include it.
//
//   std::string why;
//   if (!flowValidate(flow, &why)) printf("bad flow: %s\n", why.c_str());
//   if (!tocValidate(toc, flow, &why)) ...
//   puts(flowDump(flow).c_str());   // <H1>Chapter</H1>, <i>..</i>, [PAGE], ...
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "bookflow.h"

inline std::string flowSlurp(const std::string &path) {
  std::string s;
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) return s;
  char buf[16384];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) s.append(buf, n);
  fclose(f);
  return s;
}

// The whole grammar: blocks end with '\n'; [FL_PAGE] [kind] inline*; a rule
// has no inline; no empty block; valid UTF-8 (no surrogates, no overlongs),
// no control byte other than the inline ones, none inside a character;
// italic/bold toggles alternate (on, off) and are closed at the block's end;
// no leading, trailing or double spaces.
inline bool flowValidate(const std::string &f, std::string *why = nullptr) {
  char msg[160];
#define FLOW_BAD(...) do { snprintf(msg, sizeof msg, __VA_ARGS__); if (why) *why = msg; return false; } while (0)
  size_t i = 0;
  while (i < f.size()) {
    size_t start = i;
    if ((uint8_t)f[i] == FL_PAGE) i++;
    uint8_t kind = 0;
    if (i < f.size() && (uint8_t)f[i] >= FL_H1 && (uint8_t)f[i] <= FL_RULE) kind = f[i++];
    if (kind == FL_RULE) {
      if (i >= f.size() || f[i] != '\n') FLOW_BAD("rule with text at %zu", start);
      i++;
      continue;
    }
    bool it = false, bd = false, any = false, lastSpace = false, first = true;
    for (;;) {
      if (i >= f.size()) FLOW_BAD("block at %zu has no end", start);
      uint8_t c = f[i];
      if (c == '\n') break;
      if (c == FL_ITALIC_ON) { if (it) FLOW_BAD("italic on twice at %zu", i); it = true, i++; continue; }
      if (c == FL_ITALIC_OFF) { if (!it) FLOW_BAD("italic off unopened at %zu", i); it = false, i++; continue; }
      if (c == FL_BOLD_ON) { if (bd) FLOW_BAD("bold on twice at %zu", i); bd = true, i++; continue; }
      if (c == FL_BOLD_OFF) { if (!bd) FLOW_BAD("bold off unopened at %zu", i); bd = false, i++; continue; }
      if (c == FL_BREAK) { if (first) FLOW_BAD("break at block start %zu", i); lastSpace = false, i++; continue; }
      if (c < 0x20 || c == 0x7F) FLOW_BAD("control byte 0x%02X at %zu", c, i);
      if (c == ' ') {
        if (first) FLOW_BAD("block starts with a space at %zu", i);
        if (lastSpace) FLOW_BAD("double space at %zu", i);
        lastSpace = true, i++;
        continue;
      }
      first = false, any = true, lastSpace = false;
      if (c < 0x80) { i++; continue; }
      int need = c >= 0xC2 && c <= 0xDF ? 1 : c >= 0xE0 && c <= 0xEF ? 2 : c >= 0xF0 && c <= 0xF4 ? 3 : -1;
      if (need < 0 || i + need >= f.size()) FLOW_BAD("bad UTF-8 lead 0x%02X at %zu", c, i);
      uint32_t cp = c & (0x3F >> need);
      for (int k = 1; k <= need; k++) {
        uint8_t b = f[i + k];
        if ((b & 0xC0) != 0x80) FLOW_BAD("bad UTF-8 continuation at %zu", i + k);
        cp = cp << 6 | (b & 0x3F);
      }
      static const uint32_t least[] = {0, 0x80, 0x800, 0x10000};
      if (cp < least[need] || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) FLOW_BAD("bad code point at %zu", i);
      i += need + 1;
    }
    if (it || bd) FLOW_BAD("style left on at block end %zu", i);
    if (lastSpace) FLOW_BAD("block ends with a space at %zu", i);
    if (!any) FLOW_BAD("empty block at %zu", start);
    i++;
  }
  return true;
#undef FLOW_BAD
}

struct FlowTocLine { uint32_t offset; int level; std::string title; };

inline std::vector<FlowTocLine> tocParse(const std::string &toc) {
  std::vector<FlowTocLine> v;
  size_t i = 0;
  while (i < toc.size()) {
    size_t nl = toc.find('\n', i);
    if (nl == std::string::npos) nl = toc.size();
    std::string line = toc.substr(i, nl - i);
    size_t t1 = line.find('\t'), t2 = t1 == std::string::npos ? t1 : line.find('\t', t1 + 1);
    FlowTocLine e{0, 0, ""};
    if (t2 != std::string::npos) {
      e.offset = (uint32_t)strtoul(line.c_str(), nullptr, 10);
      e.level = atoi(line.c_str() + t1 + 1);
      e.title = line.substr(t2 + 1);
    } else {
      e.level = -1;
    }
    v.push_back(e);
    i = nl + 1;
  }
  return v;
}

// Every line well formed, levels 1..3, titles not empty, offsets ascending
// and each at a block's start in the flow.
inline bool tocValidate(const std::string &toc, const std::string &flow, std::string *why = nullptr) {
  uint32_t last = 0;
  char msg[160];
  for (auto &e : tocParse(toc)) {
    if (e.level < 1 || e.level > 3) { snprintf(msg, sizeof msg, "bad toc line"); if (why) *why = msg; return false; }
    if (e.title.empty()) { if (why) *why = "empty toc title"; return false; }
    if (e.offset < last) { snprintf(msg, sizeof msg, "toc offset goes back: %u after %u (%s)", e.offset, last, e.title.c_str()); if (why) *why = msg; return false; }
    if (e.offset > flow.size() || (e.offset > 0 && e.offset < flow.size() && flow[e.offset - 1] != '\n') || (e.offset == flow.size() && e.offset)) {
      snprintf(msg, sizeof msg, "toc offset %u not at a block start (%s)", e.offset, e.title.c_str());
      if (why) *why = msg;
      return false;
    }
    last = e.offset;
  }
  return true;
}

// The flow as text: a line a block, markup in angle brackets.
inline std::string flowDump(const std::string &f, size_t maxBytes = (size_t)-1) {
  std::string o;
  bool bol = true;
  for (size_t i = 0; i < f.size() && o.size() < maxBytes; i++) {
    uint8_t c = f[i];
    if (bol && c == FL_PAGE) { o += "\n[PAGE] "; continue; }
    if (bol && c >= FL_H1 && c <= FL_RULE) {
      static const char *const K[] = {"", "<H1> ", "<H2> ", "<H3> ", "<QUOTE> ", "<CENTER> ", "<ITEM> ", "* * *"};
      o += K[c];
      continue;
    }
    bol = c == '\n';
    switch (c) {
      case FL_BREAK: o += "<br>"; break;
      case FL_ITALIC_ON: o += "<i>"; break;
      case FL_ITALIC_OFF: o += "</i>"; break;
      case FL_BOLD_ON: o += "<b>"; break;
      case FL_BOLD_OFF: o += "</b>"; break;
      default: o += (char)c;
    }
  }
  return o;
}
