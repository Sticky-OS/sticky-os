// Inflate (bookinflate.h): streams packed by zlib at several levels, fed and
// taken in pieces of every size, and cut-short and damaged ones.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "bookinflate.h"

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

static uint32_t lcg(uint32_t &x) { return x = (x * 1103515245u + 12345u) & 0x7FFFFFFFu; }

// The same text as gen() and noise() in tools/make_fixtures.py.
static std::string gen(int kind, uint32_t seed, size_t n) {
  static const char *const WORDS[16] = {"the", "ink", "page", "quiet", "river", "lantern", "a", "of", "winter", "reads", "slowly", "and", "paper", "boat", "night", "stays"};
  std::string out;
  uint32_t x = seed;
  if (kind == 1) {
    while (out.size() < n) out += (char)(lcg(x) >> 16 & 0xFF);
    return out;
  }
  while (out.size() < n) out += WORDS[lcg(x) >> 16 & 15], out += ' ';
  out.resize(n);
  return out;
}

// Unpacks with input pieces of inStep bytes and room for outStep at a time.
// Returns the result that ended it (MORE_INPUT: the input ran out first).
static Inflater::Result unpack(const std::string &packed, bool wrapper, size_t inStep, size_t outStep, std::string &out, size_t *consumed = nullptr) {
  Inflater inf;
  out.clear();
  if (!inf.begin(wrapper)) return Inflater::BAD;
  std::string room(outStep, 0);
  size_t at = 0, guard = 0;
  for (;;) {
    size_t give = packed.size() - at < inStep ? packed.size() - at : inStep, used = 0, made = 0;
    Inflater::Result r = inf.run((const uint8_t *)packed.data() + at, give, used, (uint8_t *)&room[0], outStep, made);
    CHECK(used <= give && made <= outStep);
    at += used;
    out.append(room.data(), made);
    if (consumed) *consumed = at;
    if (r == Inflater::DONE || r == Inflater::BAD) return r;
    if (r == Inflater::MORE_INPUT && at == packed.size()) return r;
    if (!used && !made && ++guard > 4) {
      CHECK(!"no progress");
      return Inflater::BAD;
    }
    if (used || made) guard = 0;
    if (out.size() > (64u << 20)) return Inflater::BAD;  // a damaged stream may unpack to anything, but not for ever
  }
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : "fixtures";
  std::string all = slurp(dir + "/inflate-cases.bin");
  CHECK(all.size() > 0);
  size_t at = 0;
  int cases = 0;
  uint32_t rnd = 12345;
  while (at + 18 <= all.size()) {
    const uint8_t *h = (const uint8_t *)all.data() + at;
    bool wrapper = h[0];
    int kind = h[1];
    uint32_t seed, n, reps, plen;
    memcpy(&seed, h + 2, 4), memcpy(&n, h + 6, 4), memcpy(&reps, h + 10, 4), memcpy(&plen, h + 14, 4);
    at += 18;
    if (at + plen > all.size()) { CHECK(!"fixture cut short"); break; }
    std::string packed = all.substr(at, plen), want, got;
    at += plen;
    cases++;
    std::string one = gen(kind, seed, n);
    for (uint32_t i = 0; i < reps; i++) want += one;

    // Every mix of tiny and large pieces.
    static const size_t STEPS[][2] = {{1, 1}, {1, 4096}, {4096, 1}, {3, 7}, {7, 3}, {100000, 100000}, {64, 300}, {1000, 33000}};
    for (auto &st : STEPS) {
      size_t consumed = 0;
      Inflater::Result r = unpack(packed + "tail", wrapper, st[0], st[1], got, &consumed);
      CHECK(r == Inflater::DONE);
      CHECK(got == want);
      // Bytes after the stream are left alone (as far as the last piece given allows).
      CHECK(consumed <= packed.size() + 4);
      if (st[0] >= 100000) CHECK(consumed == packed.size());
    }
    // All at once.
    {
      std::string room(want.size() + 1, 0);
      CHECK(inflateAll((const uint8_t *)packed.data(), packed.size(), (uint8_t *)&room[0], want.size(), wrapper) == (long)want.size());
      CHECK(!memcmp(room.data(), want.data(), want.size()));
      if (want.size()) CHECK(inflateAll((const uint8_t *)packed.data(), packed.size(), (uint8_t *)&room[0], want.size() - 1, wrapper) == -1);
    }
    // Cut short anywhere: never DONE with the whole text, never a crash.
    for (size_t cut = 0; cut < packed.size(); cut += (packed.size() > 600 ? 37 : 1)) {
      Inflater::Result r = unpack(packed.substr(0, cut), wrapper, 50, 500, got);
      CHECK(r == Inflater::MORE_INPUT || r == Inflater::BAD);
      CHECK(got.size() <= want.size() && !memcmp(got.data(), want.data(), got.size()));
    }
    // Damaged: any result, but it ends.
    for (int k = 0; k < 300 && packed.size(); k++) {
      std::string bad = packed;
      int flips = 1 + lcg(rnd) % 3;
      for (int j = 0; j < flips; j++) bad[lcg(rnd) % bad.size()] ^= (char)(1 << (lcg(rnd) >> 8) % 8);
      unpack(bad, wrapper, 1 + lcg(rnd) % 500, 1 + lcg(rnd) % 5000, got);
    }
  }
  CHECK(cases >= 12);
  // Not deflate at all.
  {
    std::string got;
    CHECK(unpack(std::string("\x07garbage", 8), false, 10, 10, got) == Inflater::BAD);       // block type 3
    CHECK(unpack(std::string("PK\3\4zzzzzzzz", 12), true, 10, 10, got) == Inflater::BAD);    // no zlib header
    uint32_t x = 99;
    for (int k = 0; k < 2000; k++) {
      std::string junk;
      size_t len = lcg(x) % 400;
      while (junk.size() < len) junk += (char)(lcg(x) >> 16);
      unpack(junk, k & 1, 1 + lcg(x) % 64, 1 + lcg(x) % 2000, got);
    }
  }
  printf("inflate: %d cases, %s\n", cases, fails ? "FAILED" : "ok");
  return fails ? 1 : 0;
}
