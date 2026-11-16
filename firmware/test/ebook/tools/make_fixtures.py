#!/usr/bin/env python3
"""Makes the small test books in ../fixtures (inflate-*, zip-*, epub-*, fb2-*,
text-*, html-*). Everything is written here, by us: no text is taken from a
real book. The generated files are committed, so the tests need no Python;
run this again only to change a fixture:

    python3 tools/make_fixtures.py
"""
import base64
import os
import struct
import sys
import zipfile
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "fixtures")

WORDS = [b"the", b"ink", b"page", b"quiet", b"river", b"lantern", b"a", b"of", b"winter", b"reads", b"slowly", b"and", b"paper", b"boat", b"night", b"stays"]


def gen(seed, n):
    """Low-entropy text, the same as gen() in inflate_test.cpp."""
    x = seed
    out = bytearray()
    while len(out) < n:
        x = (x * 1103515245 + 12345) & 0x7FFFFFFF
        out += WORDS[(x >> 16) & 15] + b" "
    return bytes(out[:n])


def noise(seed, n):
    x = seed
    out = bytearray()
    for _ in range(n):
        x = (x * 1103515245 + 12345) & 0x7FFFFFFF
        out.append((x >> 16) & 0xFF)
    return bytes(out)


def deflate(data, level, wrapper, strategy=zlib.Z_DEFAULT_STRATEGY):
    c = zlib.compressobj(level, zlib.DEFLATED, 15 if wrapper else -15, 9, strategy)
    return c.compress(data) + c.flush()


def write(name, data):
    with open(os.path.join(OUT, name), "wb") as f:
        f.write(data)
    print("%-28s %6d bytes" % (name, len(data)))


def inflate_cases():
    # One file: cases of  u8 wrapper, u8 kind, u32 seed, u32 length, u32 repeats, u32 packed length, packed bytes.
    # kind 0: gen(seed, length) * repeats; 1: noise(seed, length) * repeats.
    out = bytearray()

    def case(kind, seed, n, reps, level, wrapper, strategy=zlib.Z_DEFAULT_STRATEGY):
        raw = (gen(seed, n) if kind == 0 else noise(seed, n)) * reps
        packed = deflate(raw, level, wrapper, strategy)
        out.extend(struct.pack("<BBIIII", 1 if wrapper else 0, kind, seed, n, reps, len(packed)) + packed)

    for level in (1, 6, 9):
        case(0, 1, 3000, 1, level, False)
        case(0, 1, 3000, 1, level, True)
    case(0, 3, 700, 1, 0, True)                      # stored
    case(0, 3, 700, 1, 0, False)
    case(0, 4, 2000, 1, 6, False, zlib.Z_FIXED)       # fixed codes
    case(0, 5, 2000, 1, 6, True, zlib.Z_HUFFMAN_ONLY)
    case(0, 2, 30000, 3, 9, False)                    # matches from 30000 bytes back: the window goes round
    case(1, 6, 600, 1, 6, True)                       # noise: mostly literals, long codes
    case(1, 7, 300, 40, 9, False)                     # long matches
    case(0, 8, 0, 1, 6, True)                         # nothing at all
    case(0, 8, 0, 1, 6, False)
    write("inflate-cases.bin", bytes(out))



# A tiny picture of each kind (they are only copied, never drawn).
JPEG = b"\xFF\xD8\xFF\xE0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00" + bytes(range(64)) + b"\xFF\xD9"
PNG = b"\x89PNG\r\n\x1A\n" + b"\x00\x00\x00\x0DIHDR" + bytes(17) + b"\x00\x00\x00\x00IEND\xAE\x42\x60\x82"

CONTAINER = b"""<?xml version="1.0"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles><rootfile full-path="%s" media-type="application/oebps-package+xml"/></rootfiles>
</container>"""


def epub(name, files, opf_path="OEBPS/content.opf", stored=()):
    """files: (path, bytes) in order; mimetype first, stored, as the spec wants."""
    path = os.path.join(OUT, name)
    with zipfile.ZipFile(path, "w") as z:
        z.writestr(zipfile.ZipInfo("mimetype"), b"application/epub+zip", zipfile.ZIP_STORED)
        z.writestr("META-INF/container.xml", CONTAINER % opf_path.encode(), zipfile.ZIP_DEFLATED)
        for p, data in files:
            z.writestr(p, data, zipfile.ZIP_STORED if p in stored else zipfile.ZIP_DEFLATED)
    print("%-28s %6d bytes" % (name, os.path.getsize(path)))


def xhtml(body, title="x"):
    return ("""<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:epub="http://www.idpf.org/2007/ops">
<head><title>%s</title><link rel="stylesheet" href="s.css"/><style>p { margin: 0 }</style></head>
<body>%s</body></html>""" % (title, body)).encode()


def epub2():
    opf = b"""<?xml version="1.0" encoding="UTF-8"?>
<package xmlns="http://www.idpf.org/2007/opf" version="2.0" unique-identifier="id">
  <metadata xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:opf="http://www.idpf.org/2007/opf">
    <dc:title>The Quiet River</dc:title>
    <dc:creator opf:role="aut">Ana Ink</dc:creator>
    <dc:creator opf:role="aut">Second Author</dc:creator>
    <dc:language>en</dc:language>
    <dc:description>&lt;p&gt;A short book about &lt;i&gt;ink&lt;/i&gt; &amp;amp; rivers.&lt;/p&gt;&lt;p&gt;Second   paragraph.&lt;/p&gt;</dc:description>
    <meta name="cover" content="cover-img"/>
  </metadata>
  <manifest>
    <item id="ncx" href="toc.ncx" media-type="application/x-dtbncx+xml"/>
    <item id="cover-img" href="images/front.jpg" media-type="image/jpeg"/>
    <item id="coverpage" href="cover.xhtml" media-type="application/xhtml+xml"/>
    <item id="c1" href="text/one.xhtml" media-type="application/xhtml+xml"/>
    <item id="c2" href="text/two.xhtml" media-type="application/xhtml+xml"/>
    <item id="notes" href="text/notes.xhtml" media-type="application/xhtml+xml"/>
  </manifest>
  <spine toc="ncx">
    <itemref idref="coverpage"/>
    <itemref idref="c1"/>
    <itemref idref="notes" linear="no"/>
    <itemref idref="c2"/>
  </spine>
</package>"""
    ncx = b"""<?xml version="1.0" encoding="UTF-8"?>
<ncx xmlns="http://www.daisy.org/z3986/2005/ncx/" version="2005-1">
  <navMap>
    <navPoint id="n0" playOrder="1"><navLabel><text>Cover</text></navLabel><content src="cover.xhtml"/></navPoint>
    <navPoint id="n1" playOrder="2"><navLabel><text>Part One</text></navLabel><content src="text/one.xhtml"/>
      <navPoint id="n2" playOrder="3"><navLabel><text>Chapter 1</text></navLabel><content src="text/one.xhtml#c1"/></navPoint>
      <navPoint id="n3" playOrder="4"><navLabel><text>Chapter 2 &amp; more</text></navLabel><content src="text/two.xhtml"/>
        <navPoint id="n4" playOrder="5"><navLabel><text>A deep part</text></navLabel><content src="text/two.xhtml#deep"/>
          <navPoint id="n5" playOrder="6"><navLabel><text>Deeper</text></navLabel><content src="text/two.xhtml#deeper"/></navPoint>
        </navPoint>
      </navPoint>
    </navPoint>
    <navPoint id="n6" playOrder="7"><navLabel><text>Missing</text></navLabel><content src="text/gone.xhtml"/></navPoint>
  </navMap>
</ncx>"""
    cover = xhtml('<div><img src="images/front.jpg" alt="cover"/></div>')
    one = xhtml('<h1 id="c1">Chapter 1</h1><p>It was <i>winter</i> on the river.</p><p class="center">* * *</p>'
                '<p>The ink stayed&#160;dark &mdash; and quiet&hellip;</p>')
    two = xhtml('<h2>Chapter 2</h2><p>A <b>paper</b> boat.</p><h3 id="deep">Deep</h3><p>down</p><p id="deeper">deeper</p>')
    notes = xhtml("<p>A note.</p>")
    epub("epub-two.epub", [("OEBPS/content.opf", opf), ("OEBPS/toc.ncx", ncx), ("OEBPS/images/front.jpg", JPEG),
                           ("OEBPS/cover.xhtml", cover), ("OEBPS/text/one.xhtml", one), ("OEBPS/text/two.xhtml", two),
                           ("OEBPS/text/notes.xhtml", notes)])


def epub3():
    opf = b"""<?xml version="1.0" encoding="UTF-8"?>
<opf:package xmlns:opf="http://www.idpf.org/2007/opf" version="3.0" unique-identifier="id">
  <opf:metadata xmlns:dc="http://purl.org/dc/elements/1.1/">
    <dc:title id="t">L&#x2019;Encre &amp; la Rivi\xc3\xa8re</dc:title>
    <dc:creator id="a">Ana Ink</dc:creator>
    <dc:language>fr</dc:language>
    <dc:description><![CDATA[Un livre <b>court</b>.]]> Fin &#233;t&#xE9;.</dc:description>
  </opf:metadata>
  <opf:manifest>
    <opf:item id="nav" href="nav.xhtml" media-type="application/xhtml+xml" properties="nav"/>
    <opf:item id="pic" href="pics/c.png" media-type="image/png" properties="cover-image"/>
    <opf:item id="a" href="Chapter%20One.xhtml" media-type="application/xhtml+xml"/>
    <opf:item id="b" href="sub/b.xhtml" media-type="application/xhtml+xml"/>
  </opf:manifest>
  <opf:spine>
    <opf:itemref idref="a"/>
    <opf:itemref idref="b"/>
  </opf:spine>
</opf:package>"""
    nav = xhtml("""<nav epub:type="landmarks"><ol><li><a href="Chapter%20One.xhtml">Start</a></li></ol></nav>
<nav epub:type="toc" id="toc"><h1>Contents</h1><ol>
  <li><a href="Chapter%20One.xhtml">Premier <em>chapitre</em></a></li>
  <li><a href="sub/b.xhtml#s2">Deuxième</a><ol><li><a href="../book/sub/b.xhtml#s3">Trois</a></li></ol></li>
</ol></nav>""")
    a = xhtml('<section><h1>Premier chapitre</h1><p>L&#8217;été à Paris.</p></section>')
    b = xhtml('<p>Avant.</p><div id="s2"><h2>Deuxième</h2><p>Texte.</p></div><p>Milieu <span id="s3">trois</span> fin.</p><p>Après.</p>')
    epub("epub-three.epub", [("book/package.opf", opf), ("book/nav.xhtml", nav), ("book/pics/c.png", PNG),
                             ("book/Chapter One.xhtml", a), ("book/sub/b.xhtml", b)],
         opf_path="book/package.opf", stored=("book/Chapter One.xhtml",))


def epub_drm():
    opf = b"""<?xml version="1.0"?><package xmlns="http://www.idpf.org/2007/opf" version="2.0"><metadata xmlns:dc="http://purl.org/dc/elements/1.1/"><dc:title>Locked</dc:title></metadata>
<manifest><item id="c" href="c.xhtml" media-type="application/xhtml+xml"/></manifest><spine><itemref idref="c"/></spine></package>"""
    enc = b"""<?xml version="1.0"?><encryption xmlns="urn:oasis:names:tc:opendocument:xmlns:container" xmlns:enc="http://www.w3.org/2001/04/xmlenc#">
<enc:EncryptedData><enc:EncryptionMethod Algorithm="http://www.w3.org/2001/04/xmlenc#aes128-cbc"/>
<enc:CipherData><enc:CipherReference URI="OEBPS/c.xhtml"/></enc:CipherData></enc:EncryptedData></encryption>"""
    epub("epub-drm.epub", [("META-INF/encryption.xml", enc), ("OEBPS/content.opf", opf), ("OEBPS/c.xhtml", b"\x93\x11garbage")])
    # Font obfuscation only: reads.
    fonts = b"""<?xml version="1.0"?><encryption xmlns="urn:oasis:names:tc:opendocument:xmlns:container" xmlns:enc="http://www.w3.org/2001/04/xmlenc#">
<enc:EncryptedData><enc:EncryptionMethod Algorithm="http://www.idpf.org/2008/embedding"/>
<enc:CipherData><enc:CipherReference URI="OEBPS/f.otf"/></enc:CipherData></enc:EncryptedData></encryption>"""
    epub("epub-fonts.epub", [("META-INF/encryption.xml", fonts), ("OEBPS/content.opf", opf), ("OEBPS/c.xhtml", xhtml("<p>Open.</p>"))])


def fb2():
    cover = base64.encodebytes(JPEG).decode()
    text = """<?xml version="1.0" encoding="windows-1251"?>
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0" xmlns:l="http://www.w3.org/1999/xlink">
<description><title-info><genre>prose</genre>
<author><first-name>Анна</first-name><middle-name>И.</middle-name><last-name>Чернилова</last-name></author>
<author><first-name>Second</first-name></author>
<book-title>Тихая река</book-title><lang>ru</lang>
<annotation><p>Книга о реке
и чернилах.</p><empty-line/><p><emphasis>Вторая</emphasis> часть.</p></annotation>
<coverpage><image l:href="#cover.jpg"/></coverpage></title-info>
<document-info><author><nickname>maker</nickname></author></document-info></description>
<body><title><p>Тихая река</p></title>
<epigraph><p>Слова &amp; вода.</p><text-author>Кто-то</text-author></epigraph>
<section><title><p>Глава 1</p><p>Зима</p></title>
<p>Было <emphasis>тихо</emphasis> и <strong>темно</strong>.<a l:href="#n1" type="note">1</a></p>
<empty-line/><subtitle>* * *</subtitle>
<poem><title><p>Песня</p></title><stanza><v>Строка один,</v><v>строка два.</v></stanza></poem>
<section><title><p>Часть</p></title><p>Внутри.</p></section>
</section>
<section><title><p>Глава 2</p></title><cite><p>Цитата.</p></cite><p>Конец.</p></section>
</body>
<body name="notes"><section id="n1"><title><p>1</p></title><p>Примечание.</p></section></body>
<binary id="big.png" content-type="image/png">%s</binary>
<binary id="cover.jpg" content-type="image/jpeg">%s</binary>
</FictionBook>
""" % (base64.encodebytes(PNG * 40).decode(), cover)
    write("fb2-river.fb2", text.encode("cp1251"))


def texts():
    # Hard-wrapped, Project Gutenberg's way, in windows-1252.
    wrapped = """The Project Gutenberg eBook of The Quiet River\r
\r
Title: The Quiet River\r
\r
Author: Ana Ink\r
\r
Language: English\r
\r
*** START OF THE PROJECT GUTENBERG EBOOK THE QUIET RIVER ***\r
\r
Contents\r
\r
 CHAPTER I.    Winter\r
 CHAPTER II.   The Boat\r
\r
\r
CHAPTER I.\r
Winter\r
\r
It was winter on the river, and the ink in the lantern room had gone\r
thick and slow. Nobody read at night any more; the caf\xe9 was shut and\r
the paper boats stayed _very_ still along the quay, waiting for spring.\r
\r
Chapter one of the old book was the best part, she said, and closed it.\r
\r
    How quiet the river,\r
        How dark the ink,\r
    How slow the winter\r
        Before you think.\r
\r
*       *       *       *       *\r
\r
CHAPTER II.\r
\r
The Boat\r
\r
A paper boat is a small thing, but it carries a word from one bank to\r
the other if the wind is kind and the reader patient enough to wait.\r
\r
*** END OF THE PROJECT GUTENBERG EBOOK THE QUIET RIVER ***\r
"""
    write("text-wrapped-1252.txt", wrapped.encode("cp1252"))
    # One paragraph a line, UTF-8.
    long1 = "It was winter on the river, and the ink in the lantern room had gone thick and slow, so nobody read at night any more, not even the old ferryman."
    long2 = "The paper boats stayed still along the quay, waiting for spring, and the café on the corner kept its shutters down from November until the thaw."
    perline = "PART ONE\n\nChapter 1: The Ink\n" + long1 + "\n" + long2 + "\n\nII\n\n" + long2 + "\n"
    write("text-perline.txt", perline.encode())
    md = """# The Quiet River

A *quiet* river and **dark** ink, with `code` and a [link](http://example.com) \\*not italic\\*.
Second line of the same paragraph.

Underlined
----------

- one
- two *it*
1. first
2. second

> A quote
> on two lines.

---

```
  code line
second
```

## Two &amp; three
Last paragraph with ![a boat](boat.png) and snake_case_word.
"""
    write("text-notes.md", md.encode())
    ru = "Глава 1\n\nБыло тихо на реке, и чернила в комнате с фонарём\nстали густыми и медленными.\n\nГлава 2\n\nКонец истории, и все ушли домой.\n"
    write("text-cp1251.txt", ru.encode("cp1251"))
    html = """<!DOCTYPE html><html><head><title>A  River &amp; Ink</title><meta name="Description" content="About a river &amp; ink."><style>h1{}</style></head>
<body><h1>One</h1><p>caf&eacute; &ldquo;quiet&rdquo; &#x2014; done</p><h2>Two</h2><p>end</p></body></html>"""
    write("html-utf16.html", b"\xFF\xFE" + html.encode("utf-16-le"))
    write("html-latin1.htm", """<html><head><meta charset="iso-8859-1"><title>Caf\xe9</title></head><body><p>na\xefve</p></body></html>""".encode("latin-1"))


def zips():
    path = os.path.join(OUT, "zip-mixed.zip")
    with zipfile.ZipFile(path, "w") as z:
        z.writestr(zipfile.ZipInfo("stored.txt"), gen(11, 5000), zipfile.ZIP_STORED)
        z.writestr("Dir/Deflated.TXT", gen(12, 70000), zipfile.ZIP_DEFLATED)
        z.writestr("empty", b"", zipfile.ZIP_DEFLATED)
        z.writestr("noise.bin", noise(13, 3000), zipfile.ZIP_DEFLATED)
        z.comment = b"a comment that ends with PK\x05\x06 to confuse a reader" + b"x" * 300
    print("%-28s %6d bytes" % ("zip-mixed.zip", os.path.getsize(path)))


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    inflate_cases()
    zips()
    epub2()
    epub3()
    epub_drm()
    fb2()
    texts()
