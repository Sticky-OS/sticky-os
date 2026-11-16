// Inflate (RFC 1951), and zlib's wrapper around it (RFC 1950): what a zip's
// files (EPUB) and a PDF's streams are packed with. A streaming decoder: it
// is given the packed bytes in pieces and gives back the unpacked ones in
// pieces, so neither has to fit in memory. Its 32 KB window is allocated in
// PSRAM (psalloc.h).
//
// Plain C++, no Arduino.
#pragma once
#include <stddef.h>
#include <stdint.h>

class Inflater {
 public:
  Inflater();
  ~Inflater();
  Inflater(const Inflater &) = delete;
  Inflater &operator=(const Inflater &) = delete;

  // Starts a new stream. zlibWrapper: the stream has zlib's two-byte header
  // and Adler-32 after it (a PDF's FlateDecode, a PNG); false: raw deflate
  // (a zip's files). False: out of memory.
  bool begin(bool zlibWrapper);

  // Unpacks from in[0..inLen) into out[0..outCap). On return inUsed and
  // outMade say how much of each was taken and made. Call again with the
  // rest of the input (and more room) until it returns DONE.
  enum Result {
    MORE_INPUT,   // everything given was taken: give more
    MORE_OUTPUT,  // out is full: take it and call again
    DONE,         // the stream's end (bytes after it in `in` are not taken)
    BAD,          // not a deflate stream, or damaged
  };
  Result run(const uint8_t *in, size_t inLen, size_t &inUsed, uint8_t *out, size_t outCap, size_t &outMade);

 private:
  struct State;
  State *s;
};

// A whole stream at once, for small things: out gets at most outCap bytes.
// Returns the unpacked size, or -1 if it is damaged or doesn't fit.
long inflateAll(const uint8_t *in, size_t inLen, uint8_t *out, size_t outCap, bool zlibWrapper);
