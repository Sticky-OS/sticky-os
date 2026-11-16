#!/usr/bin/env python3
"""Generates firmware/sticky-os/bookpdf.h: the constant tables of the PDF reader
(glyph names, the standard encodings, the widths of Helvetica and Times).

    python3 tools/make_pdf_tables.py ../../sticky-os/bookpdf.h
"""
import sys
import unicodedata

G = {}

def add(text):
    for item in text.split():
        name, cp = item.split(':')
        G[name] = int(cp, 16)

# ASCII punctuation and digits (letters are a rule in the code: a one-letter name is itself)
add("""space:20 exclam:21 quotedbl:22 numbersign:23 dollar:24 percent:25 ampersand:26 quotesingle:27
parenleft:28 parenright:29 asterisk:2A plus:2B comma:2C hyphen:2D period:2E slash:2F
zero:30 one:31 two:32 three:33 four:34 five:35 six:36 seven:37 eight:38 nine:39
colon:3A semicolon:3B less:3C equal:3D greater:3E question:3F at:40
bracketleft:5B backslash:5C bracketright:5D asciicircum:5E underscore:5F grave:60
braceleft:7B bar:7C braceright:7D asciitilde:7E""")
# Latin-1
add("""nbspace:A0 nonbreakingspace:A0 exclamdown:A1 cent:A2 sterling:A3 currency:A4 yen:A5 brokenbar:A6 section:A7
dieresis:A8 copyright:A9 ordfeminine:AA guillemotleft:AB logicalnot:AC sfthyphen:AD softhyphen:AD registered:AE
macron:AF degree:B0 plusminus:B1 twosuperior:B2 threesuperior:B3 acute:B4 mu:B5 paragraph:B6
periodcentered:B7 middot:B7 cedilla:B8 onesuperior:B9 ordmasculine:BA guillemotright:BB onequarter:BC
onehalf:BD threequarters:BE questiondown:BF AE:C6 Eth:D0 multiply:D7 Oslash:D8 Thorn:DE germandbls:DF
ae:E6 eth:F0 divide:F7 oslash:F8 thorn:FE""")
# Latin Extended and friends that have no decomposition
add("""Dcroat:110 dcroat:111 Hbar:126 hbar:127 dotlessi:131 IJ:132 ij:133 kgreenlandic:138 Ldot:13F ldot:140
Lslash:141 lslash:142 napostrophe:149 Eng:14A eng:14B OE:152 oe:153 Tbar:166 tbar:167 longs:17F florin:192
dotlessj:237 Gcommaaccent:122 gcommaaccent:123 Kcommaaccent:136 kcommaaccent:137 Lcommaaccent:13B
lcommaaccent:13C Ncommaaccent:145 ncommaaccent:146 Rcommaaccent:156 rcommaaccent:157 Scommaaccent:218
scommaaccent:219 Tcommaaccent:162 tcommaaccent:163 Scedilla:15E scedilla:15F Cdot:10A cdot:10B Gdot:120
gdot:121 Zdot:17B zdot:17C Edot:116 edot:117 Idot:130 Ohungarumlaut:150 ohungarumlaut:151
Uhungarumlaut:170 uhungarumlaut:171 dcaron:10F lcaron:13E Lcaron:13D tcaron:165""")
# Spacing accents
add("""circumflex:2C6 caron:2C7 breve:2D8 dotaccent:2D9 ring:2DA ogonek:2DB tilde:2DC hungarumlaut:2DD""")
# Punctuation and symbols
add("""endash:2013 emdash:2014 figuredash:2012 afii00208:2015 quoteleft:2018 quoteright:2019 quotesinglbase:201A
quotereversed:201B quotedblleft:201C quotedblright:201D quotedblbase:201E dagger:2020 daggerdbl:2021
bullet:2022 ellipsis:2026 perthousand:2030 minute:2032 second:2033 guilsinglleft:2039 guilsinglright:203A
fraction:2044 Euro:20AC euro:20AC trademark:2122 minus:2212 ff:FB00 fi:FB01 fl:FB02 ffi:FB03 ffl:FB04
onethird:2153 twothirds:2154 oneeighth:215B threeeighths:215C fiveeighths:215D seveneighths:215E
arrowleft:2190 arrowup:2191 arrowright:2192 arrowdown:2193 arrowboth:2194 arrowdblleft:21D0
arrowdblright:21D2 arrowdblboth:21D4 universal:2200 partialdiff:2202 existential:2203 emptyset:2205
gradient:2207 element:2208 notelement:2209 suchthat:220B product:220F summation:2211 asteriskmath:2217
radical:221A proportional:221D infinity:221E angle:2220 logicaland:2227 logicalor:2228 intersection:2229
union:222A integral:222B therefore:2234 similar:223C congruent:2245 approxequal:2248 notequal:2260
equivalence:2261 lessequal:2264 greaterequal:2265 propersubset:2282 propersuperset:2283 notsubset:2284
reflexsubset:2286 reflexsuperset:2287 circleplus:2295 circlemultiply:2297 perpendicular:22A5 dotmath:22C5
lozenge:25CA spade:2660 club:2663 heart:2665 diamond:2666 aleph:2135 Ifraktur:2111 Rfraktur:211C
weierstrass:2118 angleleft:2329 angleright:232A carriagereturn:21B5 arrowdblup:21D1 arrowdbldown:21D3
openbullet:25E6 filledbox:25A0 H18533:25CF H22073:25A1 triagrt:25BA visiblespace:2423
registerserif:AE copyrightserif:A9 trademarkserif:2122 registersans:AE copyrightsans:A9 trademarksans:2122
periodcentered.alt:B7 quotedblleft.alt:201C""")
# Greek
add("""Alpha:391 Beta:392 Gamma:393 Delta:394 Epsilon:395 Zeta:396 Eta:397 Theta:398 Iota:399 Kappa:39A
Lambda:39B Mu:39C Nu:39D Xi:39E Omicron:39F Pi:3A0 Rho:3A1 Sigma:3A3 Tau:3A4 Upsilon:3A5 Phi:3A6 Chi:3A7
Psi:3A8 Omega:3A9 alpha:3B1 beta:3B2 gamma:3B3 delta:3B4 epsilon:3B5 zeta:3B6 eta:3B7 theta:3B8 iota:3B9
kappa:3BA lambda:3BB nu:3BD xi:3BE omicron:3BF pi:3C0 rho:3C1 sigma1:3C2 sigma:3C3 tau:3C4 upsilon:3C5
phi:3C6 chi:3C7 psi:3C8 omega:3C9 theta1:3D1 Upsilon1:3D2 phi1:3D5 omega1:3D6""")
G = {k: v for k, v in G.items() if '.' not in k}

# Accented letters, by their decomposition
ACC = {0x300: 'grave', 0x301: 'acute', 0x302: 'circumflex', 0x303: 'tilde', 0x304: 'macron', 0x306: 'breve',
       0x307: 'dotaccent', 0x308: 'dieresis', 0x30A: 'ring', 0x30B: 'hungarumlaut', 0x30C: 'caron',
       0x327: 'cedilla', 0x328: 'ogonek'}
for cp in list(range(0xC0, 0x180)) + [0x1E80, 0x1E81, 0x1E82, 0x1E83, 0x1E84, 0x1E85, 0x1EF2, 0x1EF3]:
    d = unicodedata.normalize('NFD', chr(cp))
    if len(d) == 2 and ord(d[1]) in ACC and d[0].isascii():
        name = d[0] + ACC[ord(d[1])]
        if ord(d[1]) == 0x327 and d[0] in 'GgKkLlNnRrTt':
            continue  # the "commaaccent" ones, by hand above
        G.setdefault(name, cp)

names = sorted(G, key=lambda s: s.encode())
assert all(len(n) > 1 for n in names)

STD = {}
for i in range(0x20, 0x7F):
    STD[i] = i
STD[0x27] = 0x2019
STD[0x60] = 0x2018
std_hi = """A1:exclamdown A2:cent A3:sterling A4:fraction A5:yen A6:florin A7:section A8:currency A9:quotesingle
AA:quotedblleft AB:guillemotleft AC:guilsinglleft AD:guilsinglright AE:fi AF:fl B1:endash B2:dagger B3:daggerdbl
B4:periodcentered B6:paragraph B7:bullet B8:quotesinglbase B9:quotedblbase BA:quotedblright BB:guillemotright
BC:ellipsis BD:perthousand BF:questiondown C1:grave C2:acute C3:circumflex C4:tilde C5:macron C6:breve
C7:dotaccent C8:dieresis CA:ring CB:cedilla CD:hungarumlaut CE:ogonek CF:caron D0:emdash E1:AE E3:ordfeminine
E8:Lslash E9:Oslash EA:OE EB:ordmasculine F1:ae F5:dotlessi F8:lslash F9:oslash FA:oe FB:germandbls"""
for item in std_hi.split():
    c, n = item.split(':')
    STD[int(c, 16)] = G[n]

WIN = {}
for i in range(0x20, 0x100):
    try:
        WIN[i] = ord(bytes([i]).decode('cp1252'))
    except UnicodeDecodeError:
        WIN[i] = 0x2022
WIN[0x7F] = 0x2022
WIN[0xA0] = 0x20
WIN[0xAD] = 0x2D

MAC = {}
for i in range(0x20, 0x100):
    MAC[i] = ord(bytes([i]).decode('mac_roman'))
MAC[0x7F] = 0
MAC[0xCA] = 0x20
MAC[0xF0] = 0

SYM = {}
sym = """20:space 21:exclam 22:universal 23:numbersign 24:existential 25:percent 26:ampersand 27:suchthat
28:parenleft 29:parenright 2A:asteriskmath 2B:plus 2C:comma 2D:minus 2E:period 2F:slash 30:zero 31:one 32:two
33:three 34:four 35:five 36:six 37:seven 38:eight 39:nine 3A:colon 3B:semicolon 3C:less 3D:equal 3E:greater
3F:question 40:congruent 41:Alpha 42:Beta 43:Chi 44:Delta 45:Epsilon 46:Phi 47:Gamma 48:Eta 49:Iota 4A:theta1
4B:Kappa 4C:Lambda 4D:Mu 4E:Nu 4F:Omicron 50:Pi 51:Theta 52:Rho 53:Sigma 54:Tau 55:Upsilon 56:sigma1 57:Omega
58:Xi 59:Psi 5A:Zeta 5B:bracketleft 5C:therefore 5D:bracketright 5E:perpendicular 5F:underscore 61:alpha 62:beta
63:chi 64:delta 65:epsilon 66:phi 67:gamma 68:eta 69:iota 6A:phi1 6B:kappa 6C:lambda 6D:mu 6E:nu 6F:omicron
70:pi 71:theta 72:rho 73:sigma 74:tau 75:upsilon 76:omega1 77:omega 78:xi 79:psi 7A:zeta 7B:braceleft 7C:bar
7D:braceright 7E:similar A0:Euro A1:Upsilon1 A2:minute A3:lessequal A4:fraction A5:infinity A6:florin A7:club
A8:diamond A9:heart AA:spade AB:arrowboth AC:arrowleft AD:arrowup AE:arrowright AF:arrowdown B0:degree
B1:plusminus B2:second B3:greaterequal B4:multiply B5:proportional B6:partialdiff B7:bullet B8:divide
B9:notequal BA:equivalence BB:approxequal BC:ellipsis BF:carriagereturn C0:aleph C1:Ifraktur C2:Rfraktur
C3:weierstrass C4:circlemultiply C5:circleplus C6:emptyset C7:intersection C8:union C9:propersuperset
CA:reflexsuperset CB:notsubset CC:propersubset CD:reflexsubset CE:element CF:notelement D0:angle D1:gradient
D2:registerserif D3:copyrightserif D4:trademarkserif D5:product D6:radical D7:dotmath D8:logicalnot
D9:logicaland DA:logicalor DB:arrowdblboth DC:arrowdblleft DD:arrowdblup DE:arrowdblright DF:arrowdbldown
E0:lozenge E1:angleleft E2:registersans E3:copyrightsans E4:trademarksans E5:summation F1:angleright F2:integral"""
for item in sym.split():
    c, n = item.split(':')
    SYM[int(c, 16)] = 0x3BC if n == 'mu' else G[n]

DOC = {i: i for i in range(0x20, 0x100)}
DOC[0x7F] = 0
DOC[0xAD] = 0
for i, cp in enumerate([0x2D8, 0x2C7, 0x2C6, 0x2D9, 0x2DD, 0x2DB, 0x2DA, 0x2DC]):
    DOC[0x18 + i] = cp
for i, cp in enumerate([0x2022, 0x2020, 0x2021, 0x2026, 0x2014, 0x2013, 0x192, 0x2044, 0x2039, 0x203A, 0x2212,
                        0x2030, 0x201E, 0x201C, 0x201D, 0x2018, 0x2019, 0x201A, 0x2122, 0xFB01, 0xFB02, 0x141,
                        0x152, 0x160, 0x178, 0x17D, 0x131, 0x142, 0x153, 0x161, 0x17E, 0, 0x20AC]):
    DOC[0x80 + i] = cp
for i in (9, 10, 13):
    DOC[i] = 0x20

HELV = [278, 278, 355, 556, 556, 889, 667, 191, 333, 333, 389, 584, 278, 333, 278, 278] + [556] * 10 + \
       [278, 278, 584, 584, 584, 556, 1015,
        667, 667, 722, 722, 667, 611, 778, 722, 278, 500, 667, 556, 833, 722, 778, 667, 778, 722, 667, 611, 722,
        667, 944, 667, 667, 611, 278, 278, 278, 469, 556, 333,
        556, 556, 500, 556, 556, 278, 556, 556, 222, 222, 500, 222, 833, 556, 556, 556, 556, 333, 500, 278, 556,
        500, 722, 500, 500, 500, 334, 260, 334, 584]
TIMES = [250, 333, 408, 500, 500, 833, 778, 180, 333, 333, 500, 564, 250, 333, 250, 278] + [500] * 10 + \
        [278, 278, 564, 564, 564, 444, 921,
         722, 667, 667, 722, 611, 556, 722, 722, 333, 389, 722, 611, 889, 722, 722, 556, 722, 667, 556, 611, 722,
         722, 944, 722, 722, 611, 333, 278, 333, 469, 500, 333,
         444, 500, 444, 500, 444, 333, 500, 500, 278, 278, 500, 278, 778, 500, 500, 500, 500, 333, 389, 278, 500,
         500, 722, 500, 500, 444, 480, 200, 480, 541]
assert len(HELV) == 95 and len(TIMES) == 95


def table(name, d, typ='uint16_t', n=256):
    out = ['static const %s %s[%d] = {' % (typ, name, n)]
    for row in range(0, n, 16):
        out.append('    ' + ' '.join('0x%X,' % d.get(i, 0) for i in range(row, min(n, row + 16))))
    out.append('};')
    return '\n'.join(out)


o = []
o.append("""// The PDF reader's tables (bookpdf.cpp): the names fonts give their glyphs,
// the encodings a font without a /ToUnicode map is read with, and the widths
// of the two families a PDF may use without saying how wide their letters
// are. All constants: they live in flash.
//
// Generated (firmware/test/ebook/tools/make_pdf_tables.py): don't edit by hand.
#pragma once
#include <stdint.h>

// Adobe's glyph names (the Latin, punctuation, Greek and common symbols of
// the Adobe Glyph List), sorted by name as strcmp does. One-letter names
// ("a", "Z") and the "uniXXXX" forms are worked out in the code.
struct PdfGlyph {
  const char *name;
  uint16_t cp;
};
static const PdfGlyph PDF_GLYPHS[] = {""")
line = '   '
for n in names:
    item = ' {"%s", 0x%X},' % (n, G[n])
    if len(line) + len(item) > 118:
        o.append(line)
        line = '   '
    line += item
o.append(line)
o.append('};')
o.append('static const int PDF_GLYPH_COUNT = %d;' % len(names))
o.append('')
o.append('// A character code to Unicode in each of the encodings a font may name (0: nothing there).')
o.append(table('PDF_ENC_STANDARD', STD))
o.append(table('PDF_ENC_WINANSI', WIN))
o.append(table('PDF_ENC_MACROMAN', MAC))
o.append(table('PDF_ENC_SYMBOL', SYM))
o.append('// PDFDocEncoding: what a text string outside the pages (a title, a bookmark) is in when it has no byte order mark.')
o.append(table('PDF_ENC_DOC', DOC))
o.append('')
o.append('// Widths of the characters 32 to 126 in quarter-thousandths of the font size: the standard')
o.append('// fonts come without /Widths, and other fonts without them are guessed from these.')
o.append(table('PDF_W_HELVETICA', {i: round(w / 4) for i, w in enumerate(HELV)}, 'uint8_t', 95))
o.append(table('PDF_W_TIMES', {i: round(w / 4) for i, w in enumerate(TIMES)}, 'uint8_t', 95))
open(sys.argv[1], 'w').write('\n'.join(o) + '\n')
print(len(names), 'glyph names')
