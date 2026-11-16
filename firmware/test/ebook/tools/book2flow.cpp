// book2flow: any book file to a flow, its contents and its cover, through
// the firmware's own converter (ebook.h), with the meta, numbers and, with
// -d, a readable dump. For trying the readers on real books.
//
//   book2flow [-d] [-n bytes] book.epub outdir
//
// Writes outdir/<name>.flow, .flow.toc, .cover; -d prints the flow as text
// (-n: only its first bytes).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string>

#include "ebook.h"
#include "flowcheck.h"

static double now() {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
  bool dump = false;
  size_t maxDump = (size_t)-1;
  int i = 1;
  for (; i < argc && argv[i][0] == '-'; i++) {
    if (!strcmp(argv[i], "-d")) dump = true;
    else if (!strcmp(argv[i], "-n") && i + 1 < argc) dump = true, maxDump = strtoul(argv[++i], nullptr, 10);
  }
  if (argc - i != 2) {
    fprintf(stderr, "usage: book2flow [-d] [-n bytes] book outdir\n");
    return 2;
  }
  const char *book = argv[i], *dir = argv[i + 1];
  const char *slash = strrchr(book, '/');
  std::string base = std::string(dir) + "/" + (slash ? slash + 1 : book);
  std::string flowPath = base + ".flow", coverPath = base + ".cover";

  BookMeta pm;
  BookErr perr;
  double t0 = now();
  bool probed = bookProbe(book, pm, perr);
  double probeTime = now() - t0;
  printf("probe: %s (err %d) title=\"%s\" author=\"%s\" lang=\"%s\" format=%s  %.1f ms\n", probed ? "ok" : "FAILED", perr, pm.title.c_str(), pm.author.c_str(),
         pm.language.c_str(), bookFormatLabel(pm.format), probeTime * 1000);

  BookConverter c;
  t0 = now();
  if (!c.begin(book, flowPath.c_str(), coverPath.c_str())) {
    printf("begin failed: err %d\n", c.error());
    return 1;
  }
  double beginTime = now() - t0;
  int p, steps = 0, last = -1;
  double maxStep = 0;
  for (;;) {
    double s = now();
    p = c.step();
    double d = now() - s;
    if (d > maxStep) maxStep = d;
    steps++;
    if (p < 0 || p == BOOK_DONE) break;
    if (p < last) printf("progress went back: %d after %d\n", p, last);
    last = p;
  }
  double total = now() - t0;
  if (p < 0) {
    printf("convert failed: err %d after %d steps\n", c.error(), steps);
    return 1;
  }
  const BookMeta &m = c.meta();
  std::string flow = flowSlurp(flowPath), toc = flowSlurp(flowPath + ".toc"), why;
  FILE *bf = fopen(book, "rb");
  long srcSize = 0;
  if (bf) fseek(bf, 0, SEEK_END), srcSize = ftell(bf), fclose(bf);
  printf("meta: title=\"%s\" author=\"%s\" lang=\"%s\" cover=%c\n", m.title.c_str(), m.author.c_str(), m.language.c_str(), m.cover ? m.cover : '-');
  printf("stats: source %ld bytes, flow %zu bytes, toc %zu entries, begin %.2f ms, %d steps, max step %.2f ms, total %.1f ms\n", srcSize, flow.size(),
         tocParse(toc).size(), beginTime * 1000, steps, maxStep * 1000, total * 1000);
  bool ok = flowValidate(flow, &why);
  printf("flow: %s%s\n", ok ? "valid" : "INVALID: ", ok ? "" : why.c_str());
  bool tok = tocValidate(toc, flow, &why);
  printf("toc: %s%s\n", tok ? "valid" : "INVALID: ", tok ? "" : why.c_str());
  for (auto &e : tocParse(toc)) printf("  %8u %d %s\n", e.offset, e.level, e.title.c_str());
  if (dump) printf("----\n%s\n", flowDump(flow, maxDump).c_str());
  return ok && tok ? 0 : 1;
}
