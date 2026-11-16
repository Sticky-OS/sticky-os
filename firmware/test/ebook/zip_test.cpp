// The zip reader (bookzip.h): the list behind a comment, lookups, stored and
// deflated files read in pieces, and lying sizes, cut-short and damaged copies.
#include <stdio.h>
#include <string.h>
#include <string>

#include "bookzip.h"
#include "flowcheck.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static uint32_t lcg(uint32_t &x) { return x = (x * 1103515245u + 12345u) & 0x7FFFFFFFu; }

// gen() and noise() of tools/make_fixtures.py.
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

// A whole entry read with pieces of `step` bytes; false if the reader failed.
static bool readEntry(ZipFile &z, int i, size_t step, std::string &out) {
  out.clear();
  ZipReader r;
  if (!r.open(z, i)) return false;
  std::string buf(step, 0);
  for (int guard = 0; guard < 10000000; guard++) {
    long n = r.read((uint8_t *)&buf[0], step);
    if (n < 0) return false;
    if (n == 0) return true;
    out.append(buf.data(), (size_t)n);
  }
  return false;
}

static void writeFile(const char *path, const std::string &s) {
  FILE *f = fopen(path, "wb");
  fwrite(s.data(), 1, s.size(), f);
  fclose(f);
}

int main(int argc, char **argv) {
  std::string dir = argc > 1 ? argv[1] : "fixtures", path = dir + "/zip-mixed.zip";
  ZipFile z;
  CHECK(z.open(path.c_str()));
  CHECK(z.count() == 4);
  int st = z.find("stored.txt"), de = z.find("Dir/Deflated.TXT"), em = z.find("empty"), no = z.find("noise.bin");
  CHECK(st >= 0 && de >= 0 && em >= 0 && no >= 0);
  CHECK(z.find("dir/deflated.txt") == de);  // whatever the case
  CHECK(z.find("missing") == -1 && z.find("") == -1);
  CHECK(z.size(de) == 70000);
  std::string got;
  for (size_t step : {1, 7, 4096, 100000}) {
    CHECK(readEntry(z, st, step, got) && got == gen(0, 11, 5000));
    CHECK(readEntry(z, de, step, got) && got == gen(0, 12, 70000));
    CHECK(readEntry(z, em, step, got) && got.empty());
    CHECK(readEntry(z, no, step, got) && got == gen(1, 13, 3000));
  }
  PsVec<char> all;
  CHECK(zipReadAll(z, de, all, 70000) && all.size() == 70001 && all[70000] == 0 && !memcmp(all.data(), gen(0, 12, 70000).data(), 70000));
  CHECK(!zipReadAll(z, de, all, 69999));
  z.close();
  // Not a zip.
  {
    bool notZip = false;
    writeFile("build/zip_test.zip", "plain text, not a zip at all");
    CHECK(!z.open("build/zip_test.zip", &notZip) && notZip);
    CHECK(!z.open("build/no-such-file.zip"));
  }
  std::string good = flowSlurp(path);
  // Lying sizes in the list: never more than the list says, never past the packed bytes.
  {
    size_t cd = good.rfind("PK\1\2");
    while (cd != std::string::npos && cd > 0 && good.compare(cd + 46, 16, "Dir/Deflated.TXT")) cd = good.rfind("PK\1\2", cd - 1);
    CHECK(cd != std::string::npos);
    std::string bad = good;
    bad[cd + 24] = 0x10, bad[cd + 25] = 0, bad[cd + 26] = 0, bad[cd + 27] = 0;  // says 16 bytes unpacked
    writeFile("build/zip_test.zip", bad);
    CHECK(z.open("build/zip_test.zip"));
    readEntry(z, z.find("Dir/Deflated.TXT"), 5, got);
    CHECK(got == gen(0, 12, 16));
    bad = good;
    bad[cd + 27] = 0x7F;  // says 2 GB
    writeFile("build/zip_test.zip", bad);
    CHECK(z.open("build/zip_test.zip"));
    CHECK(readEntry(z, z.find("Dir/Deflated.TXT"), 4096, got) && got == gen(0, 12, 70000));  // the stream's end ends it
    CHECK(!zipReadAll(z, z.find("Dir/Deflated.TXT"), all, 1 << 20));
    bad = good;
    bad[cd + 23] = 0x7F;  // packed size beyond the file
    writeFile("build/zip_test.zip", bad);
    CHECK(z.open("build/zip_test.zip"));
    readEntry(z, z.find("Dir/Deflated.TXT"), 4096, got);
    z.close();
  }
  // Cut short and damaged: no crash, no hang.
  uint32_t seed = 5;
  int opened = 0;
  for (int k = 0; k < 1500; k++) {
    std::string bad = good;
    if (k < 500) {
      bad.resize(good.size() * k / 500);
    } else {
      int flips = 1 + lcg(seed) % 6;
      for (int j = 0; j < flips; j++) bad[lcg(seed) % bad.size()] ^= (char)(1 << (lcg(seed) % 8));
    }
    writeFile("build/zip_test.zip", bad);
    if (!z.open("build/zip_test.zip")) continue;
    opened++;
    for (int i = 0; i < z.count(); i++) {
      readEntry(z, i, 1 + lcg(seed) % 9000, got);
      CHECK(got.size() <= z.size(i));
    }
  }
  z.close();
  remove("build/zip_test.zip");
  printf("zip: %d damaged copies opened, %s\n", opened, fails ? "FAILED" : "ok");
  return fails ? 1 : 0;
}
