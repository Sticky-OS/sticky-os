#!/usr/bin/env python3
"""Makes the MOBI reader's test books (../fixtures/mobi-*), small and made
from nothing (no real book in them), with the encoders written here from the
format's description: PalmDOC's LZ77, the HUFF/CDIC Huffman scheme, the
trailing entries of text records, EXTH, the INDX indexes (NCX, and KF8's
skeleton and fragment tables), FDST and the MOBI 7 + KF8 "BOUNDARY" file.
The generated files are committed; run this again only to change them:

    python3 tools/make_mobi_fixtures.py

Beside each book, mobi-<name>.html is the exact text its records hold, for
the test to compare the reader's output with byte for byte.
"""
import os
import random
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, '..', 'fixtures')
REC = 4096

# ---------------------------------------------------------------- the texts

WORDS = ('the a of and to in that it was he she said for on with as his her at by not but from they '
         'all were which this had you one there or an so if out up into when no what more over could '
         'little time like very then down about only well back again now through way ').split()


def prose(rnd, n, extra=()):
    """n words or so of made-up sentences, with some of extra mixed in."""
    out, words = [], list(WORDS) + list(extra)
    while len(out) < n:
        s = [rnd.choice(words) for _ in range(rnd.randint(6, 16))]
        s[0] = s[0].capitalize()
        out.append(' '.join(s) + rnd.choice('...?!'))
    return ' '.join(out)


def mobi7_html(rnd, chapters, accents=(), toc=False, encoding='utf-8'):
    """An old-MOBI book: <mbp:pagebreak/> between chapters, an optional
    contents page of filepos links reached through the <guide>. Returns the
    bytes (as str, filled) and the chapter positions."""
    def build(pos):
        h = '<html><head><guide>'
        if toc:
            h += '<reference type="toc" title="Contents" filepos=%010d />' % pos.get('toc', 0)
        h += '</guide></head><body>'
        h += '<p align="center"><font size="7">A Made-Up Book</font></p><mbp:pagebreak/>'
        if toc:
            h += '\0TOC\0<p><b>Contents</b></p><blockquote>'
            for i, t in enumerate(chapters):
                h += '<a filepos=%010d>%s</a><br/>' % (pos.get(i, 0), t.replace('&', '&amp;'))
            h += '</blockquote><mbp:pagebreak/>'
        for i, t in enumerate(chapters):
            h += '\0C%d\0<h2>%s</h2>' % (i, t.replace('&', '&amp;'))
            for k in range(2):
                h += '<p>%s <i>%s</i> %s</p>' % (prose(rnd, 24, accents), rnd.choice(WORDS), prose(rnd, 16, accents))
            h += '<mbp:pagebreak/>'
        return h + '</body></html>'
    # The marks \0..\0 say where things are; a second pass puts the real numbers in.
    state = rnd.getstate()
    pos = {}
    for _ in range(2):
        rnd.setstate(state)
        h = build(pos)
        pos, out, i = {}, '', 0
        while i < len(h):
            if h[i] == '\0':
                j = h.index('\0', i + 1)
                m = h[i + 1:j]
                pos['toc' if m == 'TOC' else int(m[1:])] = len(out.encode(encoding))  # filepos counts the book's own bytes
                i = j + 1
            else:
                out += h[i]
                i += 1
    return out, pos


# ---------------------------------------------------------------- compression

def lz77(data):
    """PalmDOC compression: greedy, copies of 3..10 bytes up to 2047 back
    (overlapping ones too), the space+character byte, escaped literals."""
    out, i, n = bytearray(), 0, len(data)
    while i < n:
        best, bestd = 0, 0
        for d in range(1, min(i, 2047) + 1):
            k = 0
            while k < 10 and i + k < n and data[i + k - d] == data[i + k]:
                k += 1
            if k > best:
                best, bestd = k, d
                if k == 10:
                    break
        if best >= 3:
            v = 0x8000 | bestd << 3 | (best - 3)
            out += bytes([v >> 8, v & 0xFF])
            i += best
            continue
        c = data[i]
        if c == 0x20 and i + 1 < n and 0x40 <= data[i + 1] <= 0x7F:
            out.append(data[i + 1] ^ 0x80)
            i += 2
            continue
        if c == 0 or 0x09 <= c <= 0x7F:
            out.append(c)
            i += 1
            continue
        j = i
        while j < n and j - i < 8 and not (data[j] == 0 or 0x09 <= data[j] <= 0x7F):
            j += 1
        out.append(j - i)
        out += data[i:j]
        i = j
    return bytes(out)


def unlz77(d):
    o, i = bytearray(), 0
    while i < len(d):
        c = d[i]
        i += 1
        if 1 <= c <= 8:
            o += d[i:i + c]
            i += c
        elif c < 0x80:
            o.append(c)
        elif c >= 0xC0:
            o += bytes([0x20, c ^ 0x80])
        else:
            v = c << 8 | d[i]
            i += 1
            dist, n = v >> 3 & 0x7FF, (v & 7) + 3
            for _ in range(n):
                o.append(o[-dist])
    return bytes(o)


class Huff:
    """HUFF/CDIC: phrases (every byte, then common pieces of the text, some
    of them stored coded with the others: the recursion the format allows),
    canonical codes where longer codes are smaller numbers, as readers expect."""

    def __init__(self, text):
        phrases = [(bytes([b]), True) for b in range(256)]
        pieces = {}
        for w in text.split(b' '):
            for p in (w, b' ' + w, w + b' '):
                if 2 <= len(p) <= 12:
                    pieces[p] = pieces.get(p, 0) + 1
        common = sorted(pieces, key=lambda p: (-pieces[p] * len(p), p))[:120]
        phrases += [(p, True) for p in common]
        # Coded phrases: pairs of common pieces, kept as codes of the phrases above.
        self.pairs = [(common[i] + common[i + 1]) for i in range(0, 40, 2)]
        phrases += [(p, False) for p in self.pairs]
        self.phrases = phrases
        # Frequencies from a greedy parse of the text, for the code lengths.
        index = {p: k for k, (p, _) in enumerate(phrases)}
        freq = [1] * len(phrases)
        for k in self.parse(text, index):
            freq[k] += 4
        self.lengths = self.code_lengths(freq)
        self.assign()

    @staticmethod
    def parse(text, index):
        i, out = 0, []
        while i < len(text):
            for n in range(min(24, len(text) - i), 0, -1):
                k = index.get(text[i:i + n])
                if k is not None:
                    out.append(k)
                    i += n
                    break
        return out

    @staticmethod
    def code_lengths(freq):
        import heapq
        heap = [(f, i, [i]) for i, f in enumerate(freq)]
        heapq.heapify(heap)
        depth = [0] * len(freq)
        uid = len(freq)
        while len(heap) > 1:
            f1, _, a = heapq.heappop(heap)
            f2, _, b = heapq.heappop(heap)
            for s in a + b:
                depth[s] += 1
            heapq.heappush(heap, (f1 + f2, uid, a + b))
            uid += 1
        return [max(1, min(d, 24)) if max(depth) <= 24 else d for d in depth]

    def assign(self):
        # Canonical codes, then every bit flipped: longer codes become the smaller numbers.
        order = sorted(range(len(self.phrases)), key=lambda k: (self.lengths[k], k))
        code, prev, codes = 0, self.lengths[order[0]], {}
        for k in order:
            code <<= self.lengths[k] - prev
            prev = self.lengths[k]
            codes[k] = code
            code += 1
        assert code <= 1 << prev, 'not a prefix code'
        self.code = {k: (1 << self.lengths[k]) - 1 - c for k, c in codes.items()}
        # A reader finds a phrase's number as (stored max of its length) - code:
        # the phrases of a length must be numbered from its largest code down.
        # Renumber the phrases so that holds, base by base.
        bylen = {}
        for k in range(len(self.phrases)):
            bylen.setdefault(self.lengths[k], []).append(k)
        newnum, base, self.maxc, self.minc = {}, 0, {}, {}
        for L in sorted(bylen):
            ks = sorted(bylen[L], key=lambda k: -self.code[k])
            hi, lo = self.code[ks[0]], self.code[ks[-1]]
            assert hi - lo + 1 == len(ks)
            for k in ks:
                newnum[k] = base + (hi - self.code[k])
            self.maxc[L], self.minc[L] = hi + base, lo
            base += len(ks)
        self.num = newnum

    def bits(self, ks):
        s = ''.join(format(self.code[k], '0%db' % self.lengths[k]) for k in ks)
        s += '0' * (-len(s) % 8)  # zeros: the longest code's start, longer than what's left
        return bytes(int(s[i:i + 8], 2) for i in range(0, len(s), 8))

    def encode(self, text):
        index = {p: k for k, (p, lit) in enumerate(self.phrases)}
        return self.bits(self.parse(text, index))

    def records(self):
        assert max(self.lengths) >= 8
        t1 = []
        for b in range(256):
            # The codes that start with this byte: one short code, or the shortest of the long ones.
            hits = [k for k in self.code if self.lengths[k] <= 8 and self.code[k] >> 0 == b >> (8 - self.lengths[k])]
            if hits:
                L = self.lengths[hits[0]]
                t1.append(L | 0x80 | self.maxc[L] << 8)
            else:
                L = min(self.lengths[k] for k in self.code if self.lengths[k] > 8 and self.code[k] >> (self.lengths[k] - 8) == b)
                t1.append(L)
        t2 = []
        for L in range(1, 33):
            if L in self.minc:
                t2 += [self.minc[L], self.maxc[L]]
            else:
                t2 += [(1 << L) if L < 32 else 0xFFFFFFFF, 0]
        huff = b'HUFF' + struct.pack('>LLL', 24, 24, 24 + 1024) + b'\0' * 8 + struct.pack('>256L', *t1) + struct.pack('>64L', *t2)
        # Phrases by their new numbers, 256 a CDIC record (bits = 8).
        index = {p: k for k, (p, lit) in enumerate(self.phrases)}
        byNum = [None] * len(self.phrases)
        for k, (p, lit) in enumerate(self.phrases):
            if lit:
                data, flag = p, 0x8000
            else:  # coded with the phrases that make it (not with itself)
                halves = self.parse(p, {q: j for q, j in index.items() if q != p})
                data, flag = self.bits(halves), 0
            byNum[self.num[k]] = struct.pack('>H', flag | len(data)) + data
        cdics = []
        for s in range(0, len(byNum), 256):
            part = byNum[s:s + 256]
            offs, body = [], b''
            for e in part:
                offs.append(2 * len(part) + len(body))
                body += e
            cdics.append(b'CDIC' + struct.pack('>LLL', 16, len(byNum), 8) + struct.pack('>%dH' % len(part), *offs) + body)
        return [huff] + cdics


def unhuff(h, data, depth=0):
    """A second decoder, here, to check the encoder."""
    t1, mn, mx, phrases = h
    bits, out, pos = len(data) * 8, bytearray(), 0
    d = data + b'\0' * 8
    x, n = struct.unpack_from('>Q', d, 0)[0], 32
    while True:
        if n <= 0:
            pos += 4
            x, n = struct.unpack_from('>Q', d, pos)[0], n + 32
        code = x >> n & 0xFFFFFFFF
        v = t1[code >> 24]
        L, mxc = v & 0x1F, (((v >> 8) + 1) << (32 - (v & 0x1F))) - 1
        if not v & 0x80:
            while code < mn[L]:
                L += 1
            mxc = mx[L]
        n -= L
        bits -= L
        if bits < 0:
            break
        p, lit = phrases[(mxc - code) >> (32 - L)]
        out += p if lit else unhuff(h, p, depth + 1)
    return bytes(out)


def huff_tables(recs):
    huff = recs[0]
    t1 = struct.unpack_from('>256L', huff, 24)
    t2 = struct.unpack_from('>64L', huff, 24 + 1024)
    mn = [0] + [t2[2 * i] << (32 - (i + 1)) for i in range(32)]
    mx = [0] + [((t2[2 * i + 1] + 1) << (32 - (i + 1))) - 1 for i in range(32)]
    phrases = []
    for c in recs[1:]:
        total, b = struct.unpack_from('>LL', c, 8)
        cnt = min(1 << b, total - len(phrases))
        for off in struct.unpack_from('>%dH' % cnt, c, 16):
            ln = struct.unpack_from('>H', c, 16 + off)[0]
            phrases.append((c[18 + off:18 + off + (ln & 0x7FFF)], bool(ln & 0x8000)))
    return t1, mn, mx, phrases


# ---------------------------------------------------------------- records

def size_entry(n):
    """A trailing entry's size, written so it reads backwards: the first byte has the high bit."""
    out = [n & 0x7F]
    n >>= 7
    while n:
        out.insert(0, n & 0x7F)
        n >>= 7
    out[0] |= 0x80
    return bytes(out)


def trailers(flags, overlap, rnd):
    t = b''
    if flags & 1:
        t += overlap + bytes([len(overlap)])
    for bit in range(1, 16):
        if flags >> bit & 1:
            body = bytes(rnd.randrange(256) for _ in range(rnd.randint(0, 5)))
            n = len(body) + 1
            while len(size_entry(n)) + len(body) != n:
                n += 1
            t += body + size_entry(n)
    return t


def text_records(html, compression, flags, rnd, huff=None):
    recs = []
    for s in range(0, len(html), REC):
        chunk = html[s:s + REC]
        overlap = b''
        if flags & 1:  # the rest of a character cut by the record's end
            nxt = html[s + REC:s + REC + 3]
            while len(overlap) < len(nxt) and (nxt[len(overlap)] & 0xC0) == 0x80:
                overlap += nxt[len(overlap):len(overlap) + 1]
        body = lz77(chunk) if compression == 2 else huff.encode(chunk) if compression == 17480 else chunk
        if compression == 2:
            assert unlz77(body) == chunk
        recs.append(body + trailers(flags, overlap, rnd))
    return recs


def exth(items, encoding='utf-8'):
    body = b''
    for t, v in items:
        if isinstance(v, int):
            v = struct.pack('>L', v)
        elif isinstance(v, str):
            v = v.encode(encoding)  # a book's EXTH strings are in its own encoding
        body += struct.pack('>LL', t, len(v) + 8) + v
    pad = -len(body) % 4
    return b'EXTH' + struct.pack('>LL', 12 + len(body) + pad, len(items)) + body + b'\0' * pad


def header(html_len, nrecs, compression, encoding, version, title, ex, first_image, extra_flags,
           ncx=0xFFFFFFFF, huff=(0xFFFFFFFF, 0), kf8=None, encryption=0, codepage=None):
    """Record 0: PalmDOC header, MOBI header (232 bytes, 264 for KF8), EXTH, the full name."""
    hlen = 264 if kf8 else 232
    m = bytearray(hlen)
    m[0:4] = b'MOBI'
    struct.pack_into('>LLL', m, 4, hlen, 2, codepage or (65001 if encoding == 'utf-8' else 1252))
    struct.pack_into('>LL', m, 16, 0, 0)  # unique id, file version: set below
    struct.pack_into('>L', m, 20, version)
    for off in range(24, 64, 4):
        struct.pack_into('>L', m, off, 0xFFFFFFFF)
    struct.pack_into('>L', m, 64, nrecs + 1)  # first non-book record
    struct.pack_into('>L', m, 76, 0x40C)  # locale: French, the language when EXTH 524 doesn't say
    struct.pack_into('>L', m, 80, version)
    struct.pack_into('>L', m, 92, first_image)
    struct.pack_into('>LL', m, 96, *huff)
    struct.pack_into('>L', m, 112, 0x50 if ex is not None else 0)
    struct.pack_into('>L', m, 176, 0xFFFFFFFF)
    struct.pack_into('>L', m, 180, 0xFFFFFFFF)
    struct.pack_into('>H', m, 226, extra_flags)
    struct.pack_into('>L', m, 228, ncx)
    if kf8:
        struct.pack_into('>LL', m, 176, kf8['fdst'], 1)
        struct.pack_into('>LLLL', m, 232, kf8['frag'], kf8['skel'], 0xFFFFFFFF, 0xFFFFFFFF)
    eh = exth(ex, 'utf-8' if encoding == 'utf-8' else 'cp1252') if ex is not None else b''
    name = title.encode(encoding if encoding == 'utf-8' else 'cp1252')
    palm = struct.pack('>HHLHHHH', compression, 0, html_len, nrecs, REC, encryption, 0)
    r = bytearray(palm) + m + eh
    name_at = len(r)
    r += name + b'\0' * (4 - len(name) % 4)
    struct.pack_into('>LL', r, 16 + 68, name_at, len(name))
    return bytes(r)


def palmdb(name, kind, records):
    n = len(records)
    h = bytearray(78)
    h[:len(name)] = name.encode('latin-1')[:31]
    h[60:68] = kind
    struct.pack_into('>H', h, 76, n)
    off = 78 + 8 * n + 2
    table = b''
    for i, r in enumerate(records):
        table += struct.pack('>LL', off, 2 * i)
        off += len(r)
    return bytes(h) + table + b'\0\0' + b''.join(records)


# ---------------------------------------------------------------- indexes

def varint(v):
    out = [v & 0x7F | 0x80]
    v >>= 7
    while v:
        out.insert(0, v & 0x7F)
        v >>= 7
    return bytes(out)


def index(tags, entries, strings=None):
    """An INDX: its header record (with TAGX), one record of entries, CNCX.
    tags: (tag, values a time, mask); entries: (label, {tag: [values]})."""
    ctrl = 1
    tagx = b'TAGX' + struct.pack('>LL', 12 + 4 * (len(tags) + 1), ctrl)
    for t, each, mask in tags:
        tagx += bytes([t, each, mask, 0])
    tagx += bytes([0, 0, 0, 1])
    body, offs = b'', []
    for label, vals in entries:
        offs.append(192 + len(body))
        cb, data = 0, b''
        for t, each, mask in tags:
            if t not in vals:
                continue
            count = len(vals[t]) // each
            shift = (mask & -mask).bit_length() - 1
            if count << shift & ~mask or count << shift == mask and bin(mask).count('1') > 1:
                raise ValueError('count does not fit the mask')
            cb |= count << shift
            for v in vals[t]:
                data += varint(v)
        body += bytes([len(label)]) + label + bytes([cb]) + data
    idxt = b'IDXT' + b''.join(struct.pack('>H', o) for o in offs)
    rec = bytearray(192)
    rec[0:4] = b'INDX'
    struct.pack_into('>LLLLLL', rec, 4, 192, 0, 0, 0, 192 + len(body), len(entries))
    rec = bytes(rec) + body + idxt
    head = bytearray(192)
    head[0:4] = b'INDX'
    struct.pack_into('>L', head, 4, 192)
    struct.pack_into('>L', head, 24, 1)
    struct.pack_into('>L', head, 28, 65001)
    struct.pack_into('>L', head, 36, len(entries))
    struct.pack_into('>L', head, 52, 1 if strings is not None else 0)
    out = [bytes(head) + tagx, rec]
    if strings is not None:
        out.append(strings)
    return out


def cncx(titles):
    data, offs = b'', []
    for t in titles:
        offs.append(len(data))
        b = t.encode('utf-8')
        data += varint(len(b)) + b
    return data, offs


# ---------------------------------------------------------------- images

def jpeg(rnd, n):
    return b'\xff\xd8\xff\xe0' + bytes(rnd.randrange(256) for _ in range(n)) + b'\xff\xd9'


PNG = b'\x89PNG\r\n\x1a\n' + b'\0' * 40
GIF = b'GIF89a' + b'\0' * 40


# ---------------------------------------------------------------- the books

def mobi7(rnd, title, html, compression=2, encoding='utf-8', flags=0x3, ex=(), images=(), ncx_entries=None, encryption=0):
    """records 0..: header, text, (index), images. Returns the records."""
    huff = Huff(html) if compression == 17480 else None
    text = text_records(html, compression, flags, rnd, huff)
    after = []
    ncx_at = 0xFFFFFFFF
    if ncx_entries:
        data, offs = cncx([t for _, t in ncx_entries])
        ncx_at = len(text) + 1
        after += index([(1, 1, 1), (2, 1, 2), (3, 1, 4), (4, 1, 8)],
                       [(b'%02X' % i, {1: [p], 2: [10], 3: [offs[i]], 4: [0]}) for i, (p, _) in enumerate(ncx_entries)], data)
    huff_at = (0xFFFFFFFF, 0)
    if huff:
        hr = huff.records()
        for s in range(0, len(html), REC):  # the second decoder agrees
            assert unhuff(huff_tables(hr), huff.encode(html[s:s + REC])) == html[s:s + REC]
        huff_at = (len(text) + 1 + len(after), len(hr))
        after += hr
    first_image = len(text) + 1 + len(after) if images else 0xFFFFFFFF
    h = header(len(html), len(text), compression, encoding, 6, title, list(ex), first_image, flags, ncx=ncx_at, huff=huff_at, encryption=encryption)
    return [h] + text + after + list(images)


def kf8_part(rnd, title, files, ex=()):
    """A KF8 book: each file is (skeleton, [(where in the skeleton, piece)]).
    The text is each skeleton followed by its pieces; the tables say where
    they go. Returns the records and the text as the reader puts it together."""
    raw, put, skels, frags = b'', b'', [], []
    for skel, pieces in files:
        start = len(raw)
        raw += skel
        whole = skel
        shift = 0
        for at, piece in pieces:
            whole = whole[:at + shift] + piece + whole[at + shift:]
            frags.append((start + at + shift, len(piece), len(skels)))
            shift += len(piece)
            raw += piece
        skels.append((start, len(skel), len(pieces)))
        put += whole
    text = text_records(raw, 2, 0x3, rnd)
    first = len(text) + 1
    recs = []
    frag_idx = index([(2, 1, 1), (3, 1, 2), (4, 1, 4), (6, 2, 8)],
                     [(b'%010d' % ins, {2: [0], 3: [f], 4: [i], 6: [0, ln]}) for i, (ins, ln, f) in enumerate(frags)])
    skel_idx = index([(1, 1, 3), (6, 2, 12)],
                     [(b'SKEL%010d' % i, {1: [n, n], 6: [p, ln, p, ln]}) for i, (p, ln, n) in enumerate(skels)])
    titles = ['File %d' % (i + 1) for i in range(len(files))]
    data, offs = cncx(titles)
    ncx_idx = index([(1, 1, 1), (2, 1, 2), (3, 1, 4), (4, 1, 8), (6, 2, 128)],
                    [(b'%02X' % i, {1: [frags[sum(len(f[1]) for f in files[:i])][0]], 2: [10], 3: [offs[i]], 4: [0], 6: [sum(len(f[1]) for f in files[:i]), 0]})
                     for i in range(len(files))], data)
    frag_at = first
    recs += frag_idx
    skel_at = first + len(recs)
    recs += skel_idx
    ncx_at = first + len(recs)
    recs += ncx_idx
    css = b'p { margin: 0 }'
    fdst_at = first + len(recs)
    recs.append(b'FDST' + struct.pack('>LL', 12, 2) + struct.pack('>LLLL', 0, len(raw), len(raw), len(raw) + len(css)))
    # The style sheet is a second flow at the text's end: in the last text record.
    text = text_records(raw + css, 2, 0x3, rnd)
    assert len(text) + 1 == first
    h = header(len(raw) + len(css), len(text), 2, 'utf-8', 8, title, list(ex), 0xFFFFFFFF, 0x3, ncx=ncx_at,
               kf8={'fdst': fdst_at, 'frag': frag_at, 'skel': skel_at})
    return [h] + text + recs, put


def write(name, data):
    with open(os.path.join(OUT, name), 'wb') as f:
        f.write(data)
    print('%-28s %6d bytes' % (name, len(data)))


def main():
    rnd = random.Random(1859)
    os.makedirs(OUT, exist_ok=True)

    # Uncompressed, windows-1252, a filepos contents page, a GIF cover with a JPEG thumbnail.
    chapters = ['The First Chapter', 'Café & Crème', 'The “Last” One']
    s, pos = mobi7_html(rnd, chapters, accents=['café', 'naïve', 'déjà', '—', '’s'], toc=True, encoding='cp1252')
    html = s.encode('cp1252')
    write('mobi-plain-1252.mobi', palmdb('A_Made-Up_Book', b'BOOKMOBI',
          mobi7(rnd, 'A Made-Up Book', html, compression=1, encoding='cp1252', flags=0x2,
                ex=[(100, 'Ann Écrivain'), (100, 'Second Author'), (503, 'Café Stories'), (524, 'fr'), (201, 0), (202, 1),
                    (103, '<p>Une histoire <b>café</b> &amp; crème.</p><p>Second &eacute;té.</p>')],
                images=[GIF, jpeg(rnd, 300)])))
    write('mobi-plain-1252.html', html)

    # PalmDOC LZ77, UTF-8 with characters cut by the records' ends, an NCX, a PNG cover.
    accents = ['été', 'über', 'жизнь', '日本', '\U0001F600', '—']
    chapters = ['Chapter One', 'Chapter Два', 'Chapter Three', 'Chapter Four', 'Chapter Five']
    while True:  # until every record's end cuts a character: what the trailing entry is for
        s, pos = mobi7_html(rnd, chapters, accents=accents * 6)
        html = s.encode('utf-8')
        if len(html) > 2 * REC and all((html[k] & 0xC0) == 0x80 for k in range(REC, len(html), REC)):
            break
    write('mobi-lz77-utf8.mobi', palmdb('Made_Up_Two', b'BOOKMOBI',
          mobi7(rnd, 'Made Up Two', html, compression=2, flags=0x7,
                ex=[(100, 'Bo Writer'), (524, 'en'), (201, 0), (103, 'First line жизнь.\nSecond line.')], images=[PNG],
                ncx_entries=[(pos[i], t) for i, t in enumerate(chapters)])))
    write('mobi-lz77-utf8.html', html)

    # HUFF/CDIC, windows-1252, no EXTH title (the full name), no usable cover (a GIF).
    chapters = ['Opening', 'Middle', 'Ending']
    s, pos = mobi7_html(rnd, chapters, accents=['café', 'garçon'], toc=True, encoding='cp1252')
    html = s.encode('cp1252')
    write('mobi-huff.mobi', palmdb('HUFF_BOOK', b'BOOKMOBI',
          mobi7(rnd, 'Huffman Book', html, compression=17480, encoding='cp1252', flags=0x3,
                ex=[(100, 'Cy Coder'), (201, 0)], images=[GIF])))
    write('mobi-huff.html', html)

    # DRM: the probe works, the conversion says BE_DRM.
    s, _ = mobi7_html(rnd, ['Locked'])
    write('mobi-drm.mobi', palmdb('Locked_Book', b'BOOKMOBI',
          mobi7(rnd, 'Locked Book', s.encode('utf-8'), ex=[(503, 'A Locked Book'), (100, 'Di Lock')], encryption=2)))

    # Plain PalmDOC text (TEXtREAd): no MOBI header, the name from the database.
    text = ('\n'.join(prose(rnd, 30, ['naïve']) for _ in range(14))).encode('cp1252')
    recs = text_records(text, 2, 0, rnd)
    head = struct.pack('>HHLHHHH', 2, 0, len(text), len(recs), REC, 0, 0)
    write('mobi-palmdoc.prc', palmdb('Plain_Palm_Text', b'TEXtREAd', [head] + recs))
    write('mobi-palmdoc.html', text)

    # KF8 and a MOBI 7 + KF8 file: two files, the pieces inside their skeletons.
    def kf8_file(n, words):
        skel = (b'<?xml version="1.0" encoding="utf-8"?><html xmlns="http://www.w3.org/1999/xhtml"><head><title>x</title></head>'
                b'<body aid="%d"><div aid="%d"></div></body></html>' % (2 * n, 2 * n + 1))
        at = skel.index(b'</div>')
        piece = ('<h2>EIGHT File %d</h2><p>%s <i>leaning</i> %s</p>' % (n + 1, prose(rnd, 80, words), prose(rnd, 40))).encode('utf-8')
        return skel, [(at, piece)]
    files = [kf8_file(i, ['été', '—']) for i in range(3)]
    k8, put = kf8_part(rnd, 'Made Up Eight', files, ex=[(100, 'Ed Eight'), (503, 'Made Up Eight'), (524, 'en')])
    write('mobi-kf8.azw3', palmdb('Made_Up_Eight', b'BOOKMOBI', k8))
    write('mobi-kf8.html', put)

    s, _ = mobi7_html(rnd, ['SEVEN only'])
    m7 = mobi7(rnd, 'Made Up Eight', s.encode('utf-8'), ex=[(100, 'Ed Eight'), (503, 'Made Up Eight'), (121, 0), (201, 0)],
               images=[jpeg(rnd, 200)])
    boundary = len(m7) + 1
    m7[0] = header(len(s.encode('utf-8')), len(m7) - 2, 2, 'utf-8', 6, 'Made Up Eight',
                   [(100, 'Ed Eight'), (503, 'Made Up Eight'), (121, boundary), (201, 0)], len(m7) - 1, 0x3)
    write('mobi-combo.mobi', palmdb('Made_Up_Eight', b'BOOKMOBI', m7 + [b'BOUNDARY'] + k8))
    write('mobi-combo-seven.html', s.encode('utf-8'))


if __name__ == '__main__':
    main()
