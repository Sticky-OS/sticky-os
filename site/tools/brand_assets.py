#!/usr/bin/env python3
"""Draws the Sticky OS brand kit: the Reading Lines mark, the lockup, favicons,
channel art, the GitHub social preview and the e-paper boot screen.

    python3 tools/brand_assets.py            # from site/: writes src/brand/assets/ and src/ icons
    python3 tools/brand_assets.py --extra DIR  # also copies the channel/Shorts files into DIR

Everything is drawn from the shapes below, so a change to the mark is made
here once. Text is outlined from the site's own fonts (src/fonts/*.woff2), so
no file depends on an installed font.

Needs: Python 3 with fontTools, brotli, uharfbuzz and Pillow; rsvg-convert
(librsvg) on the PATH. Building the site (build.py) needs none of this: the
outputs are committed, and build.py only zips src/brand/assets/.
"""
import io, os, pathlib, shutil, subprocess, sys, tempfile
import uharfbuzz as hb
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from PIL import Image

HERE = pathlib.Path(__file__).resolve().parent.parent
SRC = HERE / "src"
OUT = SRC / "brand" / "assets"
TMP = pathlib.Path(tempfile.mkdtemp(prefix="brand-"))

# ---------------------------------------------------------------- palette
INK, PAPER, MARK = "#1f2023", "#ecede8", "#f2cf3a"
INK_D, PAPER_D, MARK_D = "#e7e8e3", "#1d1e20", "#d6b941"
GREY_L, GREY_D = "#b9bab4", "#6e7074"           # the panel's two middle greys
VARIANTS = {                                      # name: (ink, tail)
    "colour": (INK, MARK),
    "dark": (INK_D, MARK_D),        # for dark backgrounds
    "1bit": ("#000000", "#000000"),
    "4grey": (INK, GREY_D),
    "white": ("#ffffff", "#ffffff"),  # 1-bit reversed, for black backgrounds
}

# ---------------------------------------------------------------- the mark
# Master: 64-unit grid, one 8-unit stroke. Three lines read back and forth;
# the last stretch of the bottom line is the highlighted word (the tail). The
# tail butts onto the ink, so in one colour it is simply the S, never "S.".
# Its bounding box is x 11..54, y 9..55.
BOX = (11, 9, 54, 55)


def mark64(ink, tail):
    return (f'<path d="M27 51H15" stroke="{tail}" stroke-width="8" stroke-linecap="round" fill="none"/>'
            f'<path d="M50 13H23.5A9.5 9.5 0 0 0 23.5 32H40.5A9.5 9.5 0 0 1 40.5 51H27" stroke="{ink}" '
            f'stroke-width="8" stroke-linejoin="round" fill="none"/><circle cx="50" cy="13" r="4" fill="{ink}"/>')


# Small cut for 16-20 px: drawn on an 18x20 pixel grid, every line on whole pixels
# (stroke 3, lines 7 apart). Used by the site header.
def mark_small(ink, tail, tail_attr=""):
    return (f'<path d="M7.5 17.5H2.5" stroke="{tail}"{tail_attr} stroke-width="3" stroke-linecap="round" fill="none"/>'
            f'<path d="M15.5 3.5H6A3.5 3.5 0 0 0 6 10.5H12A3.5 3.5 0 0 1 12 17.5H7.5" stroke="{ink}" '
            f'stroke-width="3" fill="none"/><circle cx="15.5" cy="3.5" r="1.5" fill="{ink}"/>')


# Favicon cut: a 16-pixel ink tile (it reads on light and dark tab strips alike)
# with the S in paper, lines 2 px thick on whole pixels, square ends.
def favicon16(tile=INK, ink=PAPER, tail=MARK):
    return (f'<rect width="16" height="16" rx="3.5" fill="{tile}"/>'
            f'<path d="M8 12H3" stroke="{tail}" stroke-width="2" fill="none"/>'
            f'<path d="M13 4H6A2 2 0 0 0 6 8H10A2 2 0 0 1 10 12H8" stroke="{ink}" stroke-width="2" fill="none"/>')


def svg(body, w, h, vb=None, extra=""):
    vb = vb or f"0 0 {w} {h}"
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{vb}" width="{w}" height="{h}"{extra}>'
            f'{body}</svg>\n')


def placed_mark(ink, tail, cx, cy, height):
    """The master mark scaled so the S is `height` tall, its box centred on (cx, cy)."""
    s = height / (BOX[3] - BOX[1])
    mx, my = (BOX[0] + BOX[2]) / 2, (BOX[1] + BOX[3]) / 2
    return f'<g transform="translate({cx - mx * s:.2f} {cy - my * s:.2f}) scale({s:.4f})">{mark64(ink, tail)}</g>'


# ---------------------------------------------------------------- text, outlined
def _instance(woff2, axes, name):
    f = TTFont(SRC / "fonts" / woff2)
    f.flavor = None
    inst = instancer.instantiateVariableFont(f, axes)
    path = TMP / name
    inst.save(path)
    return str(path)


BRIC = _instance("bricolage.woff2", {"wght": 800, "wdth": 80, "opsz": 48}, "bric.ttf")
ATK = _instance("atkinson.woff2", {"wght": 400}, "atk.ttf")
ATKB = _instance("atkinson.woff2", {"wght": 700}, "atkb.ttf")
_fonts = {}


def _font(path):
    if path not in _fonts:
        tt = TTFont(path)
        _fonts[path] = (hb.Font(hb.Face(hb.Blob.from_file_path(path))), tt, tt.getGlyphSet(),
                        tt["head"].unitsPerEm, tt["OS/2"].sCapHeight)
    return _fonts[path]


def text_path(text, size, x, y, path=BRIC, anchor="start"):
    """(svg path d, width) for text with its baseline at y."""
    hfont, tt, gs, upem, _ = _font(path)
    buf = hb.Buffer(); buf.add_str(text); buf.guess_segment_properties()
    hb.shape(hfont, buf, {"kern": True, "liga": True})
    order = tt.getGlyphOrder(); s = size / upem
    width = sum(p.x_advance for p in buf.glyph_positions) * s
    if anchor == "middle":
        x -= width / 2
    pen = SVGPathPen(gs); cx = 0
    for info, pos in zip(buf.glyph_infos, buf.glyph_positions):
        tp = TransformPen(pen, (s, 0, 0, -s, x + (cx + pos.x_offset) * s, y - pos.y_offset * s))
        gs[order[info.codepoint]].draw(tp)
        cx += pos.x_advance
    return pen.getCommands(), width


def cap(path=BRIC):
    _, _, _, upem, c = _font(path)
    return c / upem


def text(t, size, x, y, fill, path=BRIC, anchor="start"):
    d, _ = text_path(t, size, x, y, path, anchor)
    return f'<path d="{d}" fill="{fill}"/>'


def lockup(ink, tail):
    """Mark + 'Sticky OS': 64 units tall, the S as tall as the line of type's ascent."""
    size = 40
    d, w = text_path("Sticky OS", size, 57, 32 + cap() * size / 2)
    body = f'<g transform="translate(-11 0)">{mark64(ink, tail)}</g><path d="{d}" fill="{ink}"/>'
    W = round(57 + w + 2)
    return svg(body, W, 64), W


# ---------------------------------------------------------------- the Held illustration
def held(w=640, h=440, screen_mark=True):
    """The board held to a door by one round magnet. An illustration, not a logo."""
    bx, by, bw, bh = 40, 70, w - 80, h - 110
    body = (f'<rect x="{bx + 10}" y="{by + 18}" width="{bw}" height="{bh}" rx="34" fill="{INK}" opacity=".12"/>'
            f'<rect x="{bx}" y="{by}" width="{bw}" height="{bh}" rx="34" fill="#f6f5f1" stroke="{INK}" stroke-width="10"/>'
            f'<rect x="{bx + 40}" y="{by + 40}" width="{bw - 80}" height="{bh - 80}" rx="6" fill="{PAPER}" stroke="{INK}" stroke-width="5"/>')
    if screen_mark:
        body += placed_mark(INK, MARK, w / 2, by + bh / 2, (bh - 80) * .52)
    body += (f'<circle cx="{w / 2}" cy="{by}" r="40" fill="{MARK}" stroke="{INK}" stroke-width="10"/>'
             f'<circle cx="{w / 2 - 12}" cy="{by - 12}" r="9" fill="#ffffff" opacity=".55"/>')
    return svg(body, w, h)


# ---------------------------------------------------------------- render helpers
def write(path, data):
    path = pathlib.Path(path); path.parent.mkdir(parents=True, exist_ok=True)
    (path.write_bytes if isinstance(data, bytes) else path.write_text)(data)


def raster(svg_text, out, w=None, h=None):
    src = TMP / "r.svg"; src.write_text(svg_text)
    cmd = ["rsvg-convert", str(src), "-o", str(out)]
    if w: cmd += ["-w", str(w)]
    if h: cmd += ["-h", str(h)]
    pathlib.Path(out).parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(cmd, check=True)
    return out


def wrap(t, size, width, path=ATK):
    lines, cur = [], ""
    for word in t.split():
        trial = (cur + " " + word).strip()
        if cur and text_path(trial, size, 0, 0, path)[1] > width:
            lines.append(cur); cur = word
        else:
            cur = trial
    return lines + [cur]


# ---------------------------------------------------------------- pieces
def social(W, H):
    """Share card: lockup, one line, the independence line; Held on the right."""
    s = H / 640
    lk_size = 46 * s
    body = f'<rect width="{W}" height="{H}" fill="{PAPER}"/>'
    x0, base = 96 * s, 230 * s
    body += placed_mark(INK, MARK, x0 + 46 * s, base - lk_size * cap() / 2 - 2 * s, 96 * s)
    body += text("Sticky OS", 112 * s, x0 + 116 * s, base + 18 * s, INK)
    y = 340 * s
    for line in wrap("Calm apps for a magnetic e-paper board. Clock, weather, calendar, notes, "
                     "photos, books, games.", 38 * s, 640 * s):
        body += text(line, 38 * s, x0, y, INK, ATK); y += 52 * s
    body += text("Free, open-source firmware for the reTerminal Sticky. Not made by Seeed.",
                 24 * s, x0, H - 72 * s, GREY_D, ATK)
    hw = 400 * s
    body += f'<g transform="translate({W - hw - 60 * s} {(H - hw * 440 / 640) / 2 - 10 * s}) scale({hw / 640:.4f})">'
    body += held().split(">", 1)[1].rsplit("</svg>", 1)[0] + "</g>"
    return svg(body, W, H)


def boot(kind):
    """800x480 boot screen. 1-bit: black on white only; 4grey: the panel's tones."""
    ink, tail = VARIANTS["1bit" if kind == "1bit" else "4grey"]
    sub = "#000000" if kind == "1bit" else GREY_D
    body = '<rect width="800" height="480" fill="#ffffff"/>'
    body += placed_mark(ink, tail, 400, 160, 170)
    body += text("Sticky OS", 76, 400, 336, ink, BRIC, "middle")
    body += text("Starting…", 22, 400, 432, sub, ATK, "middle")
    return svg(body, 800, 480, extra=' shape-rendering="crispEdges"' if kind == "1bit" else "")


def to_1bit(png):
    im = Image.open(png).convert("L").point(lambda v: 255 if v >= 128 else 0).convert("1")
    im.save(png)


def flatten(png, bg, out=None, fmt=None, **kw):
    im = Image.open(png).convert("RGBA")
    base = Image.new("RGBA", im.size, bg); base.alpha_composite(im)
    base.convert("RGB").save(out or png, fmt, **kw)


# ---------------------------------------------------------------- build
def main():
    extra = None
    if "--extra" in sys.argv:
        extra = pathlib.Path(sys.argv[sys.argv.index("--extra") + 1])
    if OUT.exists():
        shutil.rmtree(OUT)
    (OUT / "svg").mkdir(parents=True); (OUT / "png").mkdir(); (OUT / "channel").mkdir(); (OUT / "panel").mkdir()

    # SVGs: mark (64 box, the clear space is built in) and lockup, in every variant
    for name, (ink, tail) in VARIANTS.items():
        write(OUT / "svg" / f"sticky-os-mark-{name}.svg", svg(mark64(ink, tail), 64, 64))
        lk, W = lockup(ink, tail)
        write(OUT / "svg" / f"sticky-os-lockup-{name}.svg", lk)
    write(OUT / "svg" / "sticky-os-mark-16px.svg", svg(favicon16(), 16, 16))
    write(OUT / "svg" / "sticky-os-mark-small.svg", svg(mark_small(INK, MARK), 18, 20))
    write(OUT / "svg" / "sticky-os-held.svg", held())

    # PNGs, transparent: the mark at six sizes, the lockup at three widths
    for name in ("colour", "dark", "1bit", "white"):
        ink, tail = VARIANTS[name]
        for px in (64, 128, 256, 512, 1024, 2048):
            raster(svg(mark64(ink, tail), 64, 64), OUT / "png" / f"sticky-os-mark-{name}-{px}.png", px, px)
        lk, W = lockup(ink, tail)
        for px in (512, 1024, 2048):
            raster(lk, OUT / "png" / f"sticky-os-lockup-{name}-{px}w.png", w=px)
    raster(held(), OUT / "png" / "sticky-os-held-1280w.png", w=1280)

    # favicons: the SVG the pages link, a PNG/ICO fallback, the Apple touch icon
    fav = svg(favicon16(), 16, 16)
    write(SRC / "favicon.svg", fav)
    raster(fav, TMP / "f16.png", 16, 16); raster(fav, TMP / "f32.png", 32, 32); raster(fav, TMP / "f48.png", 48, 48)
    Image.open(TMP / "f32.png").save(SRC / "favicon.ico", sizes=[(16, 16), (32, 32), (48, 48)],
                                     append_images=[Image.open(TMP / "f16.png"), Image.open(TMP / "f48.png")])
    for px in (16, 32):
        shutil.copy(TMP / f"f{px}.png", OUT / "png" / f"favicon-{px}.png")
    touch = svg(f'<rect width="180" height="180" fill="{INK}"/>' + placed_mark(PAPER, MARK, 90, 90, 96), 180, 180)
    raster(touch, SRC / "apple-touch-icon.png", 180, 180)
    shutil.copy(SRC / "apple-touch-icon.png", OUT / "png" / "apple-touch-icon-180.png")

    # channel: avatar (paper ground, the S large enough for a 48 px circle), banner, end-card logo
    av = svg(f'<rect width="800" height="800" fill="{PAPER}"/>' + placed_mark(INK, MARK, 402, 400, 380), 800, 800)
    raster(av, OUT / "channel" / "youtube-avatar-800.png", 800, 800)
    end = svg(placed_mark(INK, MARK, 144, 144, 226), 288, 288)
    raster(end, OUT / "channel" / "shorts-endcard-logo-288.png", 288, 288)
    banner(OUT / "channel" / "youtube-banner-2560x1440.png")
    raster(social(1280, 640), TMP / "gh.png", 1280, 640)
    flatten(TMP / "gh.png", PAPER, OUT / "channel" / "github-social-1280x640.png")
    raster(social(1200, 630), TMP / "og.png", 1200, 630)
    flatten(TMP / "og.png", PAPER, SRC / "shots" / "social-brand.jpg", "JPEG", quality=88)

    # the panel: boot screens at the panel's real size
    raster(boot("1bit"), OUT / "panel" / "boot-1bit-800x480.png", 800, 480)
    to_1bit(OUT / "panel" / "boot-1bit-800x480.png")
    raster(boot("4grey"), OUT / "panel" / "boot-4grey-800x480.png", 800, 480)

    if extra:
        extra.mkdir(parents=True, exist_ok=True)
        for f in OUT.rglob("*"):
            if f.is_file():
                shutil.copy(f, extra / f.name)
    print("wrote", sum(1 for f in OUT.rglob("*") if f.is_file()), "files in", OUT)


BANNER_SRC = os.environ.get("STICKY_BANNER_BASE")   # optional: the earlier banner, for its board pictures


def banner(out):
    """2560x1440; everything that matters inside the 1546x423 safe area (x 507-2053, y 508-931).
    The two boards at the edges come from the earlier banner (STICKY_BANNER_BASE) when given."""
    W, H = 2560, 1440
    body = f'<rect width="{W}" height="{H}" fill="{PAPER}"/>'
    cx = 1280
    body += placed_mark(INK, MARK, 700, 720, 230)
    body += text("Sticky OS", 196, 860, 735, INK)
    body += text("Calm apps for a magnetic e-paper board", 64, 866, 838, INK, ATK)
    body += text("Free, open-source firmware for the reTerminal Sticky. Not made by Seeed.", 34, 868, 908, GREY_D, ATK)
    raster(svg(body, W, H), TMP / "banner.png", W, H)
    im = Image.open(TMP / "banner.png").convert("RGB")
    if BANNER_SRC and os.path.exists(BANNER_SRC):
        old = Image.open(BANNER_SRC).convert("RGB")
        im.paste(old.crop((0, 0, 520, H)), (0, 0))
        im.paste(old.crop((2040, 0, W, H)), (2040, 0))
    im.save(out)


if __name__ == "__main__":
    try:
        main()
    finally:
        shutil.rmtree(TMP, ignore_errors=True)
