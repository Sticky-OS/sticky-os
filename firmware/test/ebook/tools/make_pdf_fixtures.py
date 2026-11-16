#!/usr/bin/env python3
"""Makes the PDF reader's test files (fixtures/pdf-*.pdf), byte by byte: no
third-party packages, zlib for Flate. Each file shows one thing the reader
(firmware/sticky-os/bookpdf.cpp) must handle; pdf_test.cpp says what must come out.

    python3 tools/make_pdf_fixtures.py fixtures
"""
import os
import sys
import zlib

# Helvetica's widths for the characters 32..126, to wrap and justify the lines as a real maker would.
HELV = [278, 278, 355, 556, 556, 889, 667, 191, 333, 333, 389, 584, 278, 333, 278, 278] + [556] * 10 + \
       [278, 278, 584, 584, 584, 556, 1015,
        667, 667, 722, 722, 667, 611, 778, 722, 278, 500, 667, 556, 833, 722, 778, 667, 778, 722, 667, 611, 722,
        667, 944, 667, 667, 611, 278, 278, 278, 469, 556, 333,
        556, 556, 500, 556, 556, 278, 556, 556, 222, 222, 500, 222, 833, 556, 556, 556, 556, 333, 500, 278, 556,
        500, 722, 500, 500, 500, 334, 260, 334, 584]


def width(s, size):
    return sum(HELV[ord(c) - 32] if 32 <= ord(c) < 127 else 556 for c in s) * size / 1000


def pstr(s):
    return '(' + s.replace('\\', '\\\\').replace('(', '\\(').replace(')', '\\)') + ')'


class Pdf:
    """Objects in order; written with a classic table, or a cross-reference stream and object streams."""

    def __init__(self):
        self.objs = []  # (bytes of the value, stream data or None, may go in an object stream)

    def add(self, value, stream=None, packable=True):
        self.objs.append([value.encode('latin-1') if isinstance(value, str) else value, stream, packable])
        return len(self.objs)

    def set(self, num, value, stream=None):
        self.objs[num - 1][0] = value.encode('latin-1') if isinstance(value, str) else value
        self.objs[num - 1][1] = stream

    def body(self, n, value, stream):
        out = b'%d 0 obj\n' % n + value
        if stream is not None:
            out += b'\nstream\n' + stream + b'\nendstream'
        return out + b'\nendobj\n'

    def classic(self, trailer, header=b'%PDF-1.4\n%\xe2\xe3\xcf\xd3\n', shift=0, startxref=None):
        out = bytearray(header)
        offs = []
        for i, (v, s, _) in enumerate(self.objs):
            offs.append(len(out))
            out += self.body(i + 1, v, s)
        x = len(out)
        out += b'xref\n0 %d\n0000000000 65535 f \n' % (len(self.objs) + 1)
        for o in offs:
            out += b'%010d 00000 n \n' % max(0, o + shift)
        out += b'trailer\n<< /Size %d %s >>\nstartxref\n%d\n%%%%EOF\n' % (len(self.objs) + 1, trailer.encode(), x if startxref is None else startxref)
        return bytes(out)

    def modern(self, trailer):
        """Packable objects in one object stream; the table a stream with PNG Up prediction."""
        out = bytearray(b'%PDF-1.5\n%\xe2\xe3\xcf\xd3\n')
        n = len(self.objs)
        stm = n + 1
        xref = n + 2
        where = {}
        packed = []
        for i, (v, s, p) in enumerate(self.objs):
            if p and s is None:
                packed.append((i + 1, v))
        head, data = b'', b''
        for k, (num, v) in enumerate(packed):
            head += b'%d %d ' % (num, len(data))
            data += v + b'\n'
            where[num] = (2, stm, k)
        for i, (v, s, p) in enumerate(self.objs):
            if i + 1 not in where:
                where[i + 1] = (1, len(out), 0)
                out += self.body(i + 1, v, s)
        z = zlib.compress(head + data)
        where[stm] = (1, len(out), 0)
        out += self.body(stm, b'<< /Type /ObjStm /N %d /First %d /Filter /FlateDecode /Length %d >>' % (len(packed), len(head), len(z)), z)
        where[xref] = (1, len(out), 0)
        rows = []
        for num in range(0, xref + 1):
            t, a, b = where.get(num, (0, 0, 255 if num == 0 else 0))
            rows.append(bytes([t]) + a.to_bytes(4, 'big') + bytes([b]))
        # PNG "Up": each row less the one above, a tag byte in front.
        pred, prev = b'', bytes(6)
        for r in rows:
            pred += b'\x02' + bytes((r[i] - prev[i]) & 255 for i in range(6))
            prev = r
        z = zlib.compress(pred)
        d = b'<< /Type /XRef /Size %d /W [1 4 1] /Filter /FlateDecode /DecodeParms << /Predictor 12 /Columns 6 >> /Length %d %s >>' % (
            xref + 1, len(z), trailer.encode())
        x = len(out)
        out += self.body(xref, d, z)
        out += b'startxref\n%d\n%%%%EOF\n' % x
        return bytes(out)


def helv(pdf, name='Helvetica'):
    return pdf.add('<< /Type /Font /Subtype /Type1 /BaseFont /%s /Encoding /WinAnsiEncoding >>' % name)


def page(pdf, parent, content, resources, extra=''):
    c = pdf.add('<< /Length %d >>' % len(content), content, packable=False)
    return pdf.add('<< /Type /Page /Parent %d 0 R /MediaBox [0 0 400 600] /Contents %d 0 R /Resources %s %s>>' % (parent, c, resources, extra))


def para(text, x, y, w, size=10, lead=12, indent=18, font='/F1'):
    """A paragraph, wrapped and justified (Tw), the first line indented. Returns (content, next y)."""
    words, lines, cur = text.split(), [], []
    for wd in words:
        avail = w - (indent if not lines else 0)
        if cur and width(' '.join(cur + [wd]), size) > avail:
            lines.append(cur)
            cur = []
        cur.append(wd)
    lines.append(cur)
    out = ''
    for i, ln in enumerate(lines):
        s = ' '.join(ln)
        lx = x + (indent if i == 0 else 0)
        avail = w - (indent if i == 0 else 0)
        tw = (avail - width(s, size)) / max(1, len(ln) - 1) if i < len(lines) - 1 else 0
        out += 'BT %s %g Tf %.3f Tw %.2f %.2f Td %s Tj ET\n' % (font, size, tw, lx, y, pstr(s))
        y -= lead
    return out, y


LOREM = ('The quick brown fox jumps over the lazy dog while the small cat watches from the garden wall and '
         'thinks about the long afternoon ahead of them both.')


def classic_pdf():
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    f1 = helv(p)
    info = p.add('<< /Title <FEFF00430061006600E9> /Author (Ann \\(the\\) Writer) /Producer (hand) '
                 '/Subject (A short <b>story</b> about\\na fox.) >>')
    c1, y = para(LOREM, 50, 550, 300)
    c2, y = para('A second paragraph starts here and it ends right away.', 50, y, 300)
    c3 = 'BT /F1 10 Tf 50 %g Td (\225 A list item, its bullet in WinAnsi.) Tj ET\n' % y
    pg = page(p, pages, (c1 + c2 + c3).encode('latin-1'), '<< /Font << /F1 %d 0 R >> >>' % f1)
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R /Lang (en-GB) >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R] /Count 1 >>' % pg)
    return p.classic('/Root %d 0 R /Info %d 0 R' % (cat, info))


def modern_pdf():
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    f1 = helv(p, 'Times-Roman')
    f2 = helv(p, 'Times-Italic')
    c1, y = para('Packed objects and a predicted table still read well.', 50, 550, 300, indent=0)
    content = c1 + 'BT /F2 10 Tf 68 %g Td (In italics.) Tj ET\n' % y
    z = zlib.compress(content.encode())
    c = p.add('<< /Length %d /Filter /FlateDecode >>' % len(z), z, packable=False)
    pg = p.add('<< /Type /Page /Parent %d 0 R /MediaBox [0 0 400 600] /Contents %d 0 R /Resources << /Font << /F1 %d 0 R /F2 %d 0 R >> >> >>' % (pages, c, f1, f2))
    info = p.add('<< /Title (Packed) /Subject (packed) >>')  # the title again: no description
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R] /Count 1 >>' % pg)
    return p.modern('/Root %d 0 R /Info %d 0 R' % (cat, info))


def tj_pdf():
    # Words with no space characters: TJ kerning says where they part (small: a kern, big: a space),
    # and so do the gaps between strings placed apart.
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    f1 = helv(p)
    content = ('BT /F1 10 Tf 50 550 Td [(Kern)-40(ing)-300(words)20(here)-280(and)] TJ ET\n'
               'BT /F1 10 Tf 50 538 Td (Two) Tj 22 0 Td (strings) Tj 31.2 0 Td (touch) Tj ET\n'
               'BT /F1 10 Tf 2 Tc 50 526 Td (Spaced) Tj ET\n')
    pg = page(p, pages, content.encode(), '<< /Font << /F1 %d 0 R >> >>' % f1)
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R] /Count 1 >>' % pg)
    return p.classic('/Root %d 0 R' % cat)


def cmap_pdf():
    # A two-byte Identity-H font: its codes mean nothing without the /ToUnicode map.
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    cmap = ('/CIDInit /ProcSet findresource begin 12 dict begin begincmap\n'
            '1 begincodespacerange <0000> <FFFF> endcodespacerange\n'
            '2 beginbfchar <0001> <0020> <0050> <00660069> endbfchar\n'
            '3 beginbfrange <0010> <0019> <0041> <0030> <0032> [<0068> <00E9> <D834DD1E>] <0040> <0041> <006E> endbfrange\n'
            'endcmap CMapName currentdict /CMap defineresource pop end end\n').encode()
    tu = p.add('<< /Length %d >>' % len(cmap), cmap, packable=False)
    desc = p.add('<< /Type /Font /Subtype /CIDFontType2 /BaseFont /Demo-Regular /CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) /Supplement 0 >> /DW 600 /W [16 [700 700] 48 50 500] >>')
    f0 = p.add('<< /Type /Font /Subtype /Type0 /BaseFont /Demo-Regular /Encoding /Identity-H /DescendantFonts [%d 0 R] /ToUnicode %d 0 R >>' % (desc, tu))
    # Another Identity-H font with no map: its text can't be read.
    desc2 = p.add('<< /Type /Font /Subtype /CIDFontType2 /BaseFont /Secret /CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) /Supplement 0 >> >>')
    f9 = p.add('<< /Type /Font /Subtype /Type0 /BaseFont /Secret /Encoding /Identity-H /DescendantFonts [%d 0 R] >>' % desc2)
    # "ABC fine hé\U0001D11E" then "PAn": codes 0010.. are A.., 0050 is "fi", 0040.. "no"
    content = ('BT /F0 10 Tf 50 550 Td <001000110012 0001 0050 00400041 0040> Tj ET\n'
               'BT /F0 10 Tf 50 538 Td <003000310032> Tj ET\n'
               'BT /F9 10 Tf 50 526 Td <0101 0102 0103> Tj ET\n')
    pg = page(p, pages, content.encode(), '<< /Font << /F0 %d 0 R /F9 %d 0 R >> >>' % (f0, f9))
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R] /Count 1 >>' % pg)
    return p.classic('/Root %d 0 R' % cat)


def diffs_pdf():
    # A simple font's own glyph names: /Differences over WinAnsi.
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    enc = p.add('<< /Type /Encoding /BaseEncoding /WinAnsiEncoding /Differences [1 /f_i /quoteright /eacute /uni0161 /T_h /a.sc /zero.oldstyle 39 /quotesingle 128 /Euro] >>')
    f1 = p.add('<< /Type /Font /Subtype /Type1 /BaseFont /ABCDEF+Garamond-Italic /FirstChar 1 /LastChar 7 /Widths [556 278 556 556 600 500 556] /Encoding %d 0 R >>' % enc)
    f2 = p.add('<< /Type /Font /Subtype /Type1 /BaseFont /Garamond /Encoding /MacRomanEncoding >>')
    content = ('BT /F1 10 Tf 50 550 Td (\\001nd caf\\003 \\005ere \\004 it\\002s \\2005) Tj ET\n'
               'BT /F2 10 Tf 50 538 Td (Mac \\216t\\216 \\322quoted\\323) Tj ET\n')
    pg = page(p, pages, content.encode('latin-1'), '<< /Font << /F1 %d 0 R /F2 %d 0 R >> >>' % (f1, f2))
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R] /Count 1 >>' % pg)
    return p.classic('/Root %d 0 R' % cat)


def pages_pdf():
    # Three pages with a running header and page numbers; a paragraph goes on
    # from one page to the next, and a word is broken at a line's end.
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    f1 = helv(p)
    kids = []
    texts = [
        ('The first page begins a story that goes on for a while, longer than one page can hold, and so it '
         'reaches the bottom of this page and goes on'),
        ('over the top of the next page without a break, as books do. Here a word is broken: it is extra-',
         'ordinarily long. Then the paragraph ends.'),
        ('A last page with a new paragraph of its own, which is also the end of the book.',),
    ]
    for i in range(3):
        c = 'BT /F1 8 Tf 50 570 Td (THE RUNNING HEADER) Tj ET\n'
        c += 'BT /F1 8 Tf 195 30 Td (%d) Tj ET\n' % (i + 1)
        if i == 0:
            body, _ = para(texts[0], 50, 540, 300)
            # The last line runs to the right edge: the paragraph isn't over.
            c += body
        elif i == 1:
            c += 'BT /F1 10 Tf 50 540 Td %s Tj ET\n' % pstr('over the top of the next page without a break, as')
            c += 'BT /F1 10 Tf 50 528 Td %s Tj ET\n' % pstr('books do. Here a word is broken: it is extra-')
            c += 'BT /F1 10 Tf 50 516 Td %s Tj ET\n' % pstr('ordinarily long. Then the paragraph ends.')
        else:
            body, _ = para(texts[2][0], 50, 540, 300)
            c += body
        kids.append(page(p, pages, c.encode(), '<< /Font << /F1 %d 0 R >> >>' % f1))
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%s] /Count 3 >>' % ' '.join('%d 0 R' % k for k in kids))
    return p.classic('/Root %d 0 R' % cat)


def outline_pdf():
    # Two chapters in the outline: one by a page reference, one by a named
    # destination in the names tree through a GoTo action. Inner nodes of the page tree.
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    mid = p.add('')
    f1 = helv(p)
    f2 = helv(p, 'Helvetica-Bold')
    kids = []
    for i, title in enumerate(['Chapter One', 'Chapter Two']):
        c = 'BT /F2 20 Tf 50 540 Td %s Tj ET\n' % pstr(title)
        body, _ = para('This is the text of %s, which has a few words in it.' % title.lower(), 50, 500, 300)
        kids.append(page(p, mid, (c + body).encode(), '<< /Font << /F1 %d 0 R /F2 %d 0 R >> >>' % (f1, f2)))
    p.set(mid, '<< /Type /Pages /Parent %d 0 R /Kids [%s] /Count 2 >>' % (pages, ' '.join('%d 0 R' % k for k in kids)))
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R] /Count 2 >>' % mid)
    root = p.add('')
    o1 = p.add('')
    o2 = p.add('')
    p.set(root, '<< /Type /Outlines /First %d 0 R /Last %d 0 R /Count 2 >>' % (o1, o2))
    p.set(o1, '<< /Title (Chapter One) /Parent %d 0 R /Next %d 0 R /Dest [%d 0 R /XYZ 0 600 0] >>' % (root, o2, kids[0]))
    p.set(o2, '<< /Title <FEFF004300680061007000740065007200200054007700F6> /Parent %d 0 R /Prev %d 0 R /A << /S /GoTo /D (two) >> >>' % (root, o1))
    names = p.add('<< /Kids [%d 0 R] >>' % p.add('<< /Limits [(one) (two)] /Names [(one) [%d 0 R /Fit] (two) << /D [%d 0 R /Fit] >>] >>' % (kids[0], kids[1])))
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R /Outlines %d 0 R /Names << /Dests %d 0 R >> >>' % (pages, root, names))
    return p.classic('/Root %d 0 R' % cat)


def encrypted_pdf():
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    f1 = helv(p)
    pg = page(p, pages, b'BT /F1 10 Tf 50 550 Td (\x8a\x12\xff\x01) Tj ET', '<< /Font << /F1 %d 0 R >> >>' % f1)
    enc = p.add('<< /Filter /Standard /V 2 /R 3 /Length 128 /O <00> /U <00> /P -4 >>')
    info = p.add('<< /Title (\x9c\x13\x88) >>')
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R] /Count 1 >>' % pg)
    return p.classic('/Root %d 0 R /Info %d 0 R /Encrypt %d 0 R /ID [<01> <01>]' % (cat, info, enc))


def lzw(data):
    # LZW as PDF wants it: 9 to 12 bit codes, early change, a clear code first.
    dic = {bytes([i]): i for i in range(256)}
    nxt, bits, out, acc, n = 258, 9, [], 0, 0

    def emit(code):
        nonlocal acc, n
        acc = acc << bits | code
        n += bits
        while n >= 8:
            out.append(acc >> (n - 8) & 255)
            n -= 8
    emit(256)
    w = b''
    for c in data:
        wc = w + bytes([c])
        if wc in dic:
            w = wc
            continue
        emit(dic[w])
        if nxt < 4095:
            dic[wc] = nxt
            nxt += 1
            if nxt + 1 > (1 << bits) and bits < 12:
                bits += 1
        else:
            emit(256)
            dic = {bytes([i]): i for i in range(256)}
            nxt, bits = 258, 9
        w = bytes([c])
    if w:
        emit(dic[w])
    emit(257)
    if n:
        out.append(acc << (8 - n) & 255)
    return bytes(out)


def a85(data):
    out = b''
    for i in range(0, len(data), 4):
        chunk = data[i:i + 4]
        pad = 4 - len(chunk)
        v = int.from_bytes(chunk + b'\0' * pad, 'big')
        if v == 0 and not pad:
            out += b'z'
            continue
        digits = []
        for _ in range(5):
            digits.append(33 + v % 85)
            v //= 85
        out += bytes(reversed(digits))[:5 - pad]
    return out + b'~>'


def rle(data):
    out = b''
    i = 0
    while i < len(data):
        j = i
        while j < len(data) and j - i < 128 and data[j] == data[i]:
            j += 1
        if j - i >= 3:
            out += bytes([257 - (j - i), data[i]])
            i = j
        else:
            k = i
            while k < len(data) and k - i < 128 and not (k + 2 < len(data) and data[k] == data[k + 1] == data[k + 2]):
                k += 1
            out += bytes([k - i - 1]) + data[i:k]
            i = k
    return out + b'\x80'


def filters_pdf():
    # Contents as an array of streams: LZW in ASCII85, then RunLength in ASCIIHex; a form
    # XObject called with Do; an inline image whose bytes hold "EI" without spaces around it.
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    f1 = helv(p)
    s1 = b'BT /F1 10 Tf 50 550 Td (Old filters, LZW in ASCII85, work: aaaaaaaaaaaaaaaaaaaa.) Tj ET\n'
    s2 = b'q 1 0 0 1 0 -12 cm /X1 Do Q\nBI /W 4 /H 1 /BPC 8 /CS /G ID \x00EI\xff\nEI\nBT /F1 10 Tf 50 526 Td (After the picture.) Tj ET\n'
    e1 = a85(lzw(s1))
    e2 = rle(s2).hex().encode() + b'>'
    c1 = p.add('<< /Length %d /Filter [/ASCII85Decode /LZWDecode] >>' % len(e1), e1, packable=False)
    c2 = p.add('<< /Length %d /Filter [/AHx /RL] >>' % len(e2), e2, packable=False)
    form = b'BT /F1 10 Tf 50 550 Td (From a form, RunLength in hex.) Tj ET'
    x1 = p.add('<< /Type /XObject /Subtype /Form /BBox [0 0 400 600] /Length %d >>' % len(form), form, packable=False)
    pg = p.add('<< /Type /Page /Parent %d 0 R /MediaBox [0 0 400 600] /Contents [%d 0 R %d 0 R] /Resources << /Font << /F1 %d 0 R >> /XObject << /X1 %d 0 R >> >> >>' % (pages, c1, c2, f1, x1))
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R] /Count 1 >>' % pg)
    return p.classic('/Root %d 0 R' % cat)


def broken_pdf():
    # Every offset in the table is wrong, and the stream's /Length too: the file is scanned instead.
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    f1 = helv(p)
    content = b'BT /F1 10 Tf 50 550 Td (Found by looking for the objects.) Tj ET'
    c = p.add('<< /Length 9 >>', content, packable=False)
    pg = p.add('<< /Type /Page /Parent %d 0 R /MediaBox [0 0 400 600] /Contents %d 0 R /Resources << /Font << /F1 %d 0 R >> >> >>' % (pages, c, f1))
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R] /Count 1 >>' % pg)
    return p.classic('/Root %d 0 R' % cat, shift=37)


def scan_pdf():
    # Pictures and no text: what a scanner makes.
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    jpeg = b'\xff\xd8\xff\xe0' + bytes(60) + b'\xff\xd9'
    img = p.add('<< /Type /XObject /Subtype /Image /Width 8 /Height 8 /BitsPerComponent 8 /ColorSpace /DeviceGray /Filter /DCTDecode /Length %d >>' % len(jpeg), jpeg, packable=False)
    kids = [page(p, pages, b'q 400 0 0 600 0 0 cm /Im1 Do Q', '<< /XObject << /Im1 %d 0 R >> >>' % img) for _ in range(3)]
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%s] /Count 3 >>' % ' '.join('%d 0 R' % k for k in kids))
    return p.classic('/Root %d 0 R' % cat)


def columns_pdf():
    # Two columns: the left is read to its end before the right; the title spans both.
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    f1 = helv(p)
    f2 = helv(p, 'Helvetica-Bold')
    c = 'BT /F2 16 Tf 120 560 Td (A Paper In Two Columns) Tj ET\n'
    left = ' '.join('Left column words number %d go here.' % i for i in range(1, 15))
    right = ' '.join('Right side text item %d sits there.' % i for i in range(1, 15))
    body, _ = para(left, 30, 520, 160, size=9, lead=11, indent=0)
    c += body
    body, _ = para(right, 210, 520, 160, size=9, lead=11, indent=0)
    c += body
    pg = page(p, pages, c.encode(), '<< /Font << /F1 %d 0 R /F2 %d 0 R >> >>' % (f1, f2))
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R] /Count 1 >>' % pg)
    return p.classic('/Root %d 0 R' % cat)


def cover_pdf():
    # The first page one JPEG picture the page's shape: the cover. (Its bytes only look like a JPEG's.)
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    f1 = helv(p)
    jpeg = b'\xff\xd8\xff\xe0' + bytes(range(256)) * 8 + b'\xff\xd9'
    img = p.add('<< /Type /XObject /Subtype /Image /Width 400 /Height 600 /BitsPerComponent 8 /ColorSpace /DeviceRGB /Filter [/DCTDecode] /Length %d >>' % len(jpeg), jpeg, packable=False)
    first = page(p, pages, b'q 400 0 0 600 0 0 cm /Im1 Do Q', '<< /XObject << /Im1 %d 0 R >> >>' % img)
    second = page(p, pages, b'BT /F1 10 Tf 50 550 Td (The text after the cover, long enough for a book of two pages to count as text.) Tj ET', '<< /Font << /F1 %d 0 R >> >>' % f1)
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R >>' % pages)
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R %d 0 R] /Count 2 >>' % (first, second))
    return p.classic('/Root %d 0 R' % cat)


def cycles_pdf():
    # Loops everywhere a hostile file can put them: a page tree node that is its own kid, a form
    # that calls itself, outline items that point back, q without Q, a reference to itself.
    p = Pdf()
    cat = p.add('')
    pages = p.add('')
    f1 = helv(p)
    form = p.add('')
    content = b'q ' * 200 + b'BT /F1 10 Tf 50 550 Td (Still here after all that.) Tj ET /X1 Do'
    c = p.add('<< /Length %d >>' % len(content), content, packable=False)
    res = '<< /Font << /F1 %d 0 R >> /XObject << /X1 %d 0 R >> >>' % (f1, form)
    fbody = b'/X1 Do BT /F1 10 Tf 50 520 Td (Once from the form.) Tj ET'
    p.set(form, '<< /Type /XObject /Subtype /Form /Resources %s /Length %d >>' % (res, len(fbody)), fbody)
    pg = p.add('<< /Type /Page /Parent %d 0 R /MediaBox [0 0 400 600] /Contents %d 0 R /Resources %s >>' % (pages, c, res))
    p.set(pages, '<< /Type /Pages /Kids [%d 0 R %d 0 R %d 0 R] /Count 3 >>' % (pg, pages, pg))
    self_ref = p.add('')
    p.set(self_ref, '%d 0 R' % self_ref)
    root = p.add('')
    o1 = p.add('')
    p.set(root, '<< /First %d 0 R >>' % o1)
    p.set(o1, '<< /Title (Loop) /Dest [%d 0 R /Fit] /Next %d 0 R /First %d 0 R >>' % (pg, o1, o1))
    p.set(cat, '<< /Type /Catalog /Pages %d 0 R /Outlines %d 0 R /Lang %d 0 R >>' % (pages, root, self_ref))
    return p.classic('/Root %d 0 R' % cat)


FIXTURES = {
    'pdf-classic.pdf': classic_pdf, 'pdf-modern.pdf': modern_pdf, 'pdf-tj.pdf': tj_pdf, 'pdf-cmap.pdf': cmap_pdf,
    'pdf-diffs.pdf': diffs_pdf, 'pdf-pages.pdf': pages_pdf, 'pdf-outline.pdf': outline_pdf,
    'pdf-encrypted.pdf': encrypted_pdf, 'pdf-filters.pdf': filters_pdf, 'pdf-broken.pdf': broken_pdf,
    'pdf-scan.pdf': scan_pdf, 'pdf-columns.pdf': columns_pdf, 'pdf-cycles.pdf': cycles_pdf,
    'pdf-cover.pdf': cover_pdf,
}

if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', 'fixtures')
    for name, make in FIXTURES.items():
        with open(os.path.join(out, name), 'wb') as f:
            f.write(make())
    print('%d PDF fixtures written to %s' % (len(FIXTURES), out))
