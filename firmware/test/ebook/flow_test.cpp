// The flow writer (bookflow.h): white space, styles, broken UTF-8, the contents file.
#include <stdio.h>
#include <string.h>
#include <string>

#include "bookflow.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static std::string slurp(const std::string &path) {
  std::string s;
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) return s;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) s.append(buf, n);
  fclose(f);
  return s;
}

int main() {
  std::string path = "build/flow_test.flow";
  FlowWriter w;
  CHECK(w.open(path.c_str()));
  w.rule();  // before any text: dropped
  w.tocEntry("Chapter  One\n", 1);
  w.pageBreak();
  w.beginBlock(BK_H1);
  w.text("  Chapter   One ");
  w.beginBlock();
  w.beginBlock();  // empty: dropped
  w.text("Hello ");
  w.italic(true);
  w.text("world \xC3");  // a character split between two calls
  w.text("\xA9 x");
  w.italic(false);
  w.text(" end.  ");
  w.lineBreak();
  w.text("next");
  w.lineBreak();  // at the block's end: dropped
  w.rule();
  w.rule();
  uint32_t at = w.offset();
  w.text("auto \xFF bad");  // no block open: a paragraph; a bad byte: U+FFFD
  w.bold(true);
  w.text(" bold");
  CHECK(w.close());
  std::string flow = slurp(path), toc = slurp(path + ".toc");
  std::string want = "\x0C\x01" "Chapter One\n"
                     "Hello \x0Eworld \xC3\xA9 x\x0F end.\x0Bnext\n"
                     "\x07\n"
                     "auto \xEF\xBF\xBD bad \x10" "bold\x11\n";
  CHECK(flow == want);
  CHECK(toc == "0\t1\tChapter One\n");
  CHECK(at == flow.find("auto"));
  {  // several entries waiting for one block are all kept; dropped ones aren't
    FlowWriter q;
    CHECK(q.open("build/flow_test2.flow"));
    q.tocEntry("Cover", 1);
    q.tocDropPending();  // the cover page had no text
    q.tocEntry("Part One", 1);
    q.tocEntry("Chapter 1", 2);
    q.text("Text.");
    for (int i = 0; i < 10; i++) q.tocEntry(i == 9 ? "Last" : "x", 3);  // more than TOC_PENDING_MAX: the last one waiting is replaced
    CHECK(q.tocPending() == FlowWriter::TOC_PENDING_MAX);
    q.beginBlock();
    q.text("More.");
    CHECK(q.close());
    std::string toc2 = slurp("build/flow_test2.flow.toc");
    CHECK(toc2.find("Cover") == std::string::npos);
    CHECK(toc2.rfind("0\t1\tPart One\n0\t2\tChapter 1\n", 0) == 0);
    CHECK(toc2.find("6\t3\tLast\n") != std::string::npos);
  }
  printf("flow: %s\n", fails ? "FAILED" : "ok");
  return fails ? 1 : 0;
}
