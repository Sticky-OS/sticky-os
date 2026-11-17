#!/usr/bin/env python3
"""The Sticky OS brand in the firmware: the Reading Lines mark and the
"Sticky OS" wordmark as 1-bit bitmaps, made from the brand's own SVGs.

    python3 firmware/tools/gen_brand.py              # writes firmware/sticky-os/src/brand/brand.h
    python3 firmware/tools/gen_brand.py --png DIR    # also each bitmap as a PNG, to look at

The source of truth is site/tools/brand_assets.py, which writes the SVGs in
site/src/brand/assets/svg/. This reads the 1-bit mark and lockup from there,
rasterises them at the sizes the screens use (rsvg-convert, four times
larger, then scaled down) and cuts them to black and white at half grey, as brand_assets.py makes the
1-bit boot screen. Each bitmap is cropped to its ink.

The sizes follow the boot screen proposal (site/src/brand/assets/panel/
boot-1bit-800x480.png): the S 170 px tall, the wordmark set at 76 px. The
About app's header uses the lockup with the S 48 px tall. In one colour the
tail of the bottom line is ink too: the S, never "S.", so there is no grey
variant here; no screen that draws a logo uses the four-grey refresh.

Needs Python 3 with Pillow, and rsvg-convert (librsvg) on the PATH. The
header is committed: building the firmware needs none of this.
"""
import pathlib
import re
import subprocess
import sys
import tempfile

from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parents[2]
SVG = ROOT / "site" / "src" / "brand" / "assets" / "svg"
OUT = ROOT / "firmware" / "sticky-os" / "src" / "brand" / "brand.h"

S_UNITS = 46  # the S's height on the mark's 64-unit grid (its box is y 9..55)


def inner(svg_text):
    """What is inside the <svg> element, and its viewBox."""
    vb = re.search(r'viewBox="([^"]+)"', svg_text).group(1)
    body = svg_text.split(">", 1)[1].rsplit("</svg>", 1)[0]
    return body, [float(v) for v in vb.split()]


def rasterise(body, vb, scale, tmp):
    """body drawn at `scale` pixels a unit, cut to 1 bit and cropped to its ink."""
    x, y, w, h = vb
    pad = 2  # units of white round it, so nothing touches the edge
    pw, ph = round((w + 2 * pad) * scale), round((h + 2 * pad) * scale)
    big = 4  # drawn four times larger and scaled down: where the tail meets the S, the
    #          two shapes' soft edges would otherwise leave a nick in the line
    src = tmp / "in.svg"
    src.write_text(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{x - pad} {y - pad} {w + 2 * pad} {h + 2 * pad}" '
                   f'width="{pw * big}" height="{ph * big}"><rect x="{x - pad}" y="{y - pad}" width="{w + 2 * pad}" '
                   f'height="{h + 2 * pad}" fill="#ffffff"/>{body}</svg>')
    png = tmp / "out.png"
    subprocess.run(["rsvg-convert", str(src), "-o", str(png)], check=True)
    im = Image.open(png).convert("L").resize((pw, ph), Image.BOX).point(lambda v: 0 if v < 128 else 255)
    box = Image.eval(im, lambda v: 255 - v).getbbox()
    return im.crop(box)


def packed(im):
    """Adafruit GFX drawBitmap's layout: rows of bytes, the leftmost pixel in the high bit, 1 = ink."""
    w, h = im.size
    px = im.load()
    out = []
    for y in range(h):
        for x0 in range(0, w, 8):
            b = 0
            for i in range(8):
                if x0 + i < w and px[x0 + i, y] == 0:
                    b |= 0x80 >> i
            out.append(b)
    return out


def main():
    png_dir = None
    if "--png" in sys.argv:
        png_dir = pathlib.Path(sys.argv[sys.argv.index("--png") + 1])
        png_dir.mkdir(parents=True, exist_ok=True)
    mark, mark_vb = inner((SVG / "sticky-os-mark-1bit.svg").read_text())
    lockup, lockup_vb = inner((SVG / "sticky-os-lockup-1bit.svg").read_text())
    word = re.sub(r"<g\b.*?</g>", "", lockup, flags=re.S)  # the lockup without its mark: the type alone
    # name, what, body, viewBox, pixels a unit (the lockup's type is set at 40 units)
    parts = [
        ("BRAND_MARK_BOOT", "the S, 170 px tall: the boot screen", mark, mark_vb, 170 / S_UNITS),
        ("BRAND_WORD_BOOT", "\"Sticky OS\" set at 76 px: the boot screen", word, lockup_vb, 76 / 40),
        ("BRAND_LOCKUP_ABOUT", "the S 48 px tall and \"Sticky OS\" beside it: About's header", lockup, lockup_vb, 48 / S_UNITS),
    ]
    lines = [
        "// The Sticky OS brand as 1-bit bitmaps for drawBitmap(): the Reading Lines",
        "// mark and the wordmark, from site/src/brand/assets/svg/. Made by",
        "// firmware/tools/gen_brand.py: don't edit, run it again.",
        "// Rows of bytes, the leftmost pixel in the high bit, 1 = ink; in flash.",
        "#pragma once",
        "#include <stdint.h>",
        "#ifndef PROGMEM",
        "#define PROGMEM",
        "#endif",
        "",
        "struct BrandBitmap {",
        "  const uint8_t *bits;",
        "  int16_t w, h;",
        "};",
    ]
    total = 0
    with tempfile.TemporaryDirectory() as t:
        for name, what, body, vb, scale in parts:
            im = rasterise(body, vb, scale, pathlib.Path(t))
            data = packed(im)
            total += len(data)
            if png_dir:
                im.save(png_dir / f"{name.lower()}.png")
            w, h = im.size
            lines += ["", f"// {what}: {w} x {h}, {len(data)} bytes",
                      f"static const uint8_t {name}_BITS[] PROGMEM = {{"]
            for i in range(0, len(data), 20):
                lines.append("  " + ", ".join(f"0x{b:02x}" for b in data[i:i + 20]) + ",")
            lines += ["};", f"static const BrandBitmap {name} = {{{name}_BITS, {w}, {h}}};"]
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(lines) + "\n")
    print(f"wrote {OUT.relative_to(ROOT)}: {len(parts)} bitmaps, {total} bytes of flash")


if __name__ == "__main__":
    main()
