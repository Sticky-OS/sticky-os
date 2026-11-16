// Inflate, written for this firmware (no code taken from zlib or miniz): a
// state machine that can stop anywhere, for more input or more room, and go
// on at the next call. Stored, fixed and dynamic blocks; codes of up to 9
// bits are read from a table at once, longer ones bit by bit.
//
// Everything it needs (the 32 KB window, the code tables: about 36 KB) is one
// block from psMalloc. zlib's Adler-32 is read but not checked: a book with
// a damaged byte is better shown than refused.
#include "bookinflate.h"

#include <stdlib.h>
#include <string.h>

#include "psalloc.h"

namespace {

const uint32_t WIN = 32768;
const int FAST = 9;  // bits of the direct table

struct Huff {
  uint16_t cnt[16];          // codes of each length
  uint16_t fast[1 << FAST];  // symbol << 4 | length, for codes of up to FAST bits; 0: longer
  uint16_t sym[288];         // symbols in the order of their codes
};

const uint16_t LEN_BASE[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
const uint8_t LEN_EXTRA[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const uint16_t DIST_BASE[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
const uint8_t DIST_EXTRA[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
const uint8_t CLEN_ORDER[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

// The decoding tables of n code lengths. False: more codes than the lengths
// leave room for. (Too few is let through: an unused code is an error only
// if the data uses it.)
bool build(Huff &h, const uint8_t *len, int n) {
  memset(h.cnt, 0, sizeof h.cnt);
  for (int i = 0; i < n; i++) h.cnt[len[i]]++;
  h.cnt[0] = 0;
  int left = 1;
  for (int l = 1; l <= 15; l++) {
    left = (left << 1) - h.cnt[l];
    if (left < 0) return false;
  }
  uint16_t offs[16];
  offs[1] = 0;
  for (int l = 1; l < 15; l++) offs[l + 1] = offs[l] + h.cnt[l];
  for (int i = 0; i < n; i++)
    if (len[i]) h.sym[offs[len[i]]++] = (uint16_t)i;
  memset(h.fast, 0, sizeof h.fast);
  unsigned code = 0;
  int idx = 0;
  for (int l = 1; l <= FAST; l++) {
    for (int k = 0; k < h.cnt[l]; k++, code++) {
      unsigned rev = 0;  // the stream has a code's first bit lowest
      for (int b = 0; b < l; b++) rev |= ((code >> b) & 1u) << (l - 1 - b);
      uint16_t e = (uint16_t)(h.sym[idx++] << 4 | l);
      for (unsigned j = rev; j < (1u << FAST); j += 1u << l) h.fast[j] = e;
    }
    code <<= 1;
  }
  return true;
}

enum { SYM_OK, SYM_MORE, SYM_BAD };

// The symbol at the front of the bits, and its length (nothing is taken).
inline int peekSym(const Huff &h, uint32_t bits, int nbits, int &sym, int &len) {
  uint16_t e = h.fast[bits & ((1u << FAST) - 1)];
  if (e) {
    len = e & 15;
    sym = e >> 4;
    return len <= nbits ? SYM_OK : SYM_MORE;
  }
  int code = 0, first = 0, index = 0;
  for (int l = 1; l <= 15; l++) {
    if (l > nbits) return SYM_MORE;
    code |= (bits >> (l - 1)) & 1;
    int count = h.cnt[l];
    if (code - count < first) {
      sym = h.sym[index + (code - first)];
      len = l;
      return SYM_OK;
    }
    index += count;
    first = (first + count) << 1;
    code <<= 1;
  }
  return SYM_BAD;
}

enum St : uint8_t {
  S_ZHDR, S_BLOCK, S_STORED_LEN, S_STORED_NLEN, S_STORED, S_DYN_HDR, S_DYN_CLEN, S_DYN_LENS, S_DYN_REP,
  S_CODES, S_LEN_EXTRA, S_DIST, S_DIST_EXTRA, S_MATCH, S_ZTRAILER, S_DONE, S_BAD
};

}  // namespace

struct Inflater::State {
  uint8_t win[WIN];
  Huff lit, dist;
  uint8_t lens[288 + 32];
  uint32_t bits;    // not yet used bits of the input, the next one lowest
  int nbits;
  uint32_t wpos;    // bytes unpacked so far (the place in the window is wpos % WIN)
  bool wrapped;     // more than 4 GB unpacked: wpos has gone round
  uint8_t st;
  bool last, zlib;
  uint32_t stored;  // bytes left of a stored block
  int nlen, ndist, ncode, idx, sym;
  int len, back;    // of the match being copied: its length and how far back
};

Inflater::Inflater() : s(nullptr) {}
Inflater::~Inflater() { free(s); }

bool Inflater::begin(bool zlibWrapper) {
  if (!s) s = (State *)psMalloc(sizeof(State));
  if (!s) return false;
  s->bits = 0, s->nbits = 0, s->wpos = 0, s->wrapped = false;
  s->st = zlibWrapper ? S_ZHDR : S_BLOCK;
  s->last = false, s->zlib = zlibWrapper;
  return true;
}

Inflater::Result Inflater::run(const uint8_t *in, size_t inLen, size_t &inUsed, uint8_t *out, size_t outCap, size_t &outMade) {
  inUsed = outMade = 0;
  if (!s || s->st == S_BAD) return BAD;
  uint32_t bits = s->bits, wpos = s->wpos;
  int nbits = s->nbits;
  Result res = BAD;
#define DROP(n) (bits >>= (n), nbits -= (n))
#define REFILL() while (nbits <= 24 && inUsed < inLen) bits |= (uint32_t)in[inUsed++] << nbits, nbits += 8
#define STOP(r) { res = (r); goto out; }
// After REFILL there are at least 25 bits unless the input has run out, and
// no step asks for more than 16: too few means "give more".
#define NEED(n) if (nbits < (n)) STOP(MORE_INPUT)
#define FAIL() { s->st = S_BAD; STOP(BAD) }
  for (;;) {
    REFILL();
    switch (s->st) {
      case S_ZHDR: {
        NEED(16);
        unsigned cmf = bits & 0xFF, flg = bits >> 8 & 0xFF;
        if ((cmf & 15) != 8 || (cmf >> 4) > 7 || (cmf << 8 | flg) % 31 || (flg & 0x20)) FAIL();
        DROP(16);
        s->st = S_BLOCK;
        break;
      }
      case S_BLOCK: {
        if (s->last) {
          DROP(nbits & 7);  // the rest of the last byte
          s->idx = 4;       // zlib: the Adler-32's bytes
          s->st = s->zlib ? S_ZTRAILER : S_DONE;
          break;
        }
        NEED(3);
        s->last = bits & 1;
        unsigned type = bits >> 1 & 3;
        DROP(3);
        if (type == 0) {
          DROP(nbits & 7);
          s->st = S_STORED_LEN;
        } else if (type == 1) {
          for (int i = 0; i < 288; i++) s->lens[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
          build(s->lit, s->lens, 288);
          memset(s->lens, 5, 30);
          build(s->dist, s->lens, 30);
          s->st = S_CODES;
        } else if (type == 2) {
          s->st = S_DYN_HDR;
        } else {
          FAIL();
        }
        break;
      }
      case S_STORED_LEN:
        NEED(16);
        s->stored = bits & 0xFFFF;
        DROP(16);
        s->st = S_STORED_NLEN;
        break;
      case S_STORED_NLEN:
        NEED(16);
        if ((bits & 0xFFFF) != (s->stored ^ 0xFFFF)) FAIL();
        DROP(16);
        s->st = S_STORED;
        break;
      case S_STORED: {
        // What the bit buffer already holds first, then straight from the input.
        while (s->stored && nbits >= 8) {
          if (outMade == outCap) STOP(MORE_OUTPUT);
          s->win[wpos & (WIN - 1)] = out[outMade++] = (uint8_t)bits;
          if (!++wpos) s->wrapped = true;
          DROP(8);
          s->stored--;
        }
        while (s->stored) {
          if (inUsed == inLen) STOP(MORE_INPUT);
          if (outMade == outCap) STOP(MORE_OUTPUT);
          size_t n = s->stored;
          if (n > inLen - inUsed) n = inLen - inUsed;
          if (n > outCap - outMade) n = outCap - outMade;
          uint32_t at = wpos & (WIN - 1);
          if (n > WIN - at) n = WIN - at;
          memcpy(out + outMade, in + inUsed, n);
          memcpy(s->win + at, in + inUsed, n);
          if (wpos + (uint32_t)n < wpos) s->wrapped = true;
          wpos += (uint32_t)n, inUsed += n, outMade += n, s->stored -= (uint32_t)n;
        }
        s->st = S_BLOCK;
        break;
      }
      case S_DYN_HDR:
        NEED(14);
        s->nlen = (bits & 31) + 257;
        s->ndist = (bits >> 5 & 31) + 1;
        s->ncode = (bits >> 10 & 15) + 4;
        DROP(14);
        if (s->nlen > 286 || s->ndist > 30) FAIL();
        memset(s->lens, 0, 19);
        s->idx = 0;
        s->st = S_DYN_CLEN;
        break;
      case S_DYN_CLEN:
        while (s->idx < s->ncode) {
          REFILL();
          NEED(3);
          s->lens[CLEN_ORDER[s->idx++]] = bits & 7;
          DROP(3);
        }
        if (!build(s->lit, s->lens, 19)) FAIL();  // the lengths' own code, kept in `lit` for now
        s->idx = 0;
        s->st = S_DYN_LENS;
        break;
      case S_DYN_LENS: {
        if (s->idx >= s->nlen + s->ndist) {
          if (!s->lens[256]) FAIL();  // a block that could never end
          if (!build(s->lit, s->lens, s->nlen) || !build(s->dist, s->lens + s->nlen, s->ndist)) FAIL();
          s->st = S_CODES;
          break;
        }
        int sym, len, r = peekSym(s->lit, bits, nbits, sym, len);
        if (r == SYM_MORE) STOP(MORE_INPUT);
        if (r == SYM_BAD || sym > 18) FAIL();
        DROP(len);
        if (sym < 16) s->lens[s->idx++] = (uint8_t)sym;
        else s->sym = sym, s->st = S_DYN_REP;
        break;
      }
      case S_DYN_REP: {
        int extra = s->sym == 16 ? 2 : s->sym == 17 ? 3 : 7;
        NEED(extra);
        int rep = (s->sym == 18 ? 11 : 3) + (int)(bits & ((1u << extra) - 1));
        DROP(extra);
        uint8_t v = 0;
        if (s->sym == 16) {
          if (!s->idx) FAIL();
          v = s->lens[s->idx - 1];
        }
        if (s->idx + rep > s->nlen + s->ndist) FAIL();
        while (rep--) s->lens[s->idx++] = v;
        s->st = S_DYN_LENS;
        break;
      }
      case S_CODES: {
        for (;;) {
          REFILL();
          int sym, len, r = peekSym(s->lit, bits, nbits, sym, len);
          if (r == SYM_MORE) STOP(MORE_INPUT);
          if (r == SYM_BAD) FAIL();
          if (sym < 256) {
            if (outMade == outCap) STOP(MORE_OUTPUT);
            DROP(len);
            s->win[wpos & (WIN - 1)] = out[outMade++] = (uint8_t)sym;
            if (!++wpos) s->wrapped = true;
            continue;
          }
          DROP(len);
          if (sym == 256) {
            s->st = S_BLOCK;
          } else {
            if (sym > 285) FAIL();
            s->sym = sym - 257;
            s->st = S_LEN_EXTRA;
          }
          break;
        }
        break;
      }
      case S_LEN_EXTRA: {
        int extra = LEN_EXTRA[s->sym];
        NEED(extra);
        s->len = LEN_BASE[s->sym] + (int)(bits & ((1u << extra) - 1));
        DROP(extra);
        s->st = S_DIST;
        break;
      }
      case S_DIST: {
        int sym, len, r = peekSym(s->dist, bits, nbits, sym, len);
        if (r == SYM_MORE) STOP(MORE_INPUT);
        if (r == SYM_BAD || sym > 29) FAIL();
        DROP(len);
        s->sym = sym;
        s->st = S_DIST_EXTRA;
        break;
      }
      case S_DIST_EXTRA: {
        int extra = DIST_EXTRA[s->sym];
        NEED(extra);
        s->back = DIST_BASE[s->sym] + (int)(bits & ((1u << extra) - 1));
        DROP(extra);
        if (!s->wrapped && (uint32_t)s->back > wpos) FAIL();  // from before the first byte
        s->st = S_MATCH;
        break;
      }
      case S_MATCH: {
        int len = s->len;
        uint32_t dist = (uint32_t)s->back;
        while (len && outMade < outCap) {
          s->win[wpos & (WIN - 1)] = out[outMade++] = s->win[(wpos - dist) & (WIN - 1)];
          if (!++wpos) s->wrapped = true;
          len--;
        }
        s->len = len;
        if (len) STOP(MORE_OUTPUT);
        s->st = S_CODES;
        break;
      }
      case S_ZTRAILER:
        while (s->idx) {
          REFILL();
          NEED(8);
          DROP(8);
          s->idx--;
        }
        s->st = S_DONE;
        break;
      case S_DONE: {
        // Whole bytes read ahead go back to the caller, as far as they came with this call.
        size_t back = (size_t)(nbits / 8);
        if (back > inUsed) back = inUsed;
        inUsed -= back;
        bits = 0, nbits = 0;
        STOP(DONE);
      }
      default: FAIL();
    }
  }
out:
#undef DROP
#undef REFILL
#undef STOP
#undef NEED
#undef FAIL
  s->bits = bits, s->nbits = nbits, s->wpos = wpos;
  return res;
}

long inflateAll(const uint8_t *in, size_t inLen, uint8_t *out, size_t outCap, bool zlibWrapper) {
  Inflater inf;
  if (!inf.begin(zlibWrapper)) return -1;
  size_t used = 0, made = 0;
  Inflater::Result r = inf.run(in, inLen, used, out, outCap, made);
  if (r == Inflater::MORE_OUTPUT) {
    // Full: fine if the stream ends just here.
    uint8_t spare;
    size_t used2 = 0, made2 = 0;
    r = inf.run(in + used, inLen - used, used2, &spare, 1, made2);
    if (made2) return -1;
  }
  return r == Inflater::DONE ? (long)made : -1;
}
