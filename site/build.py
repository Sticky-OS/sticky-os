#!/usr/bin/env python3
"""Builds the product site into dist/.

    python3 build.py            # writes dist/
    python3 build.py --publish  # and publishes it with the Spacefast CLI (sf)

Publishing needs the Space and the sf profile to publish with, from the
environment (STICKY_SITE_SPACE, STICKY_SITE_PROFILE) or from a git-ignored
site/publish.local with the same KEY=value lines. Building needs neither.
dist/.spacefast (the link to the Space) is kept between builds.

The product name lives in NAME below and nowhere else: the pages say {{NAME}}.
The top menu lives in MENU below and nowhere else: the pages say {{HEADER}}.
The home page's "Latest from the blog" ({{LATEST}}) is the blog list's first cards.

Screenshots: drop a PNG into src/shots/ with one of these names and rebuild.
Until a file is there, its frame shows a "screenshot coming" placeholder.
  home-portrait.png  home-landscape.png  quick.png
  calendar-day.png   calendar-week.png   notes.png   keyboard.png
(home-landscape.png also replaces the Timer picture in the hero.)
Captures must hold demo data only: no names, WiFi names, addresses or places.
The board's black-and-white captures (480x800 or 800x480, or those doubled)
are drawn smooth at build time: shots/<name>.<width>.webp at 1x, 2x and 3x,
offered to the browser by srcset, so the screens read like a print at any size
instead of showing their pixels unevenly when scaled (smooth_shots(); needs Pillow).

Blog posts live in src/blog/<permalink>/index.html, listed in src/blog/index.html.
A permalink is the title's keywords, lower case and hyphenated, without small
words ("Sticky OS 1.4: a books app for the fridge door" ->
sticky-os-1-4-books-app-fridge-door). Once published it stays; a post moved
anyway gets a 301 in src/_redirects from its old address.

The brand page is src/brand/; its files are in src/brand/assets/, drawn by
tools/brand_assets.py (with favicon.svg, favicon.ico, apple-touch-icon.png and
shots/social-brand.jpg). The build zips them into brand/sticky-os-brand-kit.zip.

The setup app is src/setup/ (served at setup/; setup/?demo runs it against a
simulated board). Its tests: node test/run.mjs, from this folder.
"""
import os
import re
import pathlib
import shutil
import subprocess
import sys
import zipfile

try:  # the screenshots' smooth versions (smooth_shots); --publish refuses without it
    from PIL import Image, ImageFilter
except ImportError:
    Image = None

NAME = "Sticky OS"   # working name: change it here only

# The site's one menu: every page says {{HEADER}} and gets this, the same links
# in the same order everywhere. (label, path from the site's root; "#x" is a
# section of the home page; http… opens in a new tab.) Change it here only.
MENU = [
    ("Tour", "#tour"),
    ("Apps", "#apps"),
    ("Install", "#install"),
    ("Privacy", "#privacy"),
    ("Setup app", "setup/"),
    ("Blog", "blog/"),
    ("GitHub", "https://github.com/Sticky-OS/sticky-os"),
    ("YouTube", "https://www.youtube.com/@stickyosapp"),   # drawn as a small icon (ICONS)
]
# Menu entries drawn as an icon on wide screens; the label stays for screen readers
# and shows beside the icon in the narrow-screen Menu panel.
ICONS = {
    "GitHub": ('<svg width="18" height="18" viewBox="0 0 16 16" aria-hidden="true"><path fill="currentColor" '
               'd="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49'
               '-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82'
               '.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15'
               '-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82'
               ' 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48'
               ' 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0016 8c0-4.42-3.58-8-8-8z"/></svg>'),
    "YouTube": ('<svg width="22" height="16" viewBox="0 0 22 16" aria-hidden="true">'
                '<rect x="1" y="1" width="20" height="14" rx="4" fill="none" stroke="currentColor" stroke-width="2"/>'
                '<path d="M9 5.2v5.6l4.8-2.8z" fill="currentColor"/></svg>'),
}
# The Reading Lines mark, cut for this size: an 18x20 grid with every line on whole
# pixels (site/tools/brand_assets.py draws the same cut). Ink is the text colour;
# the tail (the highlighted word) takes the palette's yellow from site.css (.brand .tail).
LOGO = ('<svg width="18" height="20" viewBox="0 0 18 20" aria-hidden="true">'
        '<path class="tail" d="M7.5 17.5H2.5" stroke-width="3" stroke-linecap="round" fill="none"/>'
        '<path d="M15.5 3.5H6A3.5 3.5 0 0 0 6 10.5H12A3.5 3.5 0 0 1 12 17.5H7.5" stroke="currentColor" '
        'stroke-width="3" fill="none"/><circle cx="15.5" cy="3.5" r="1.5" fill="currentColor"/></svg>')

# The brand kit download (brand/): every file in src/brand/assets/ (drawn by
# tools/brand_assets.py), zipped at build time with fixed dates so the zip is
# the same bytes for the same files.
KIT = "brand/sticky-os-brand-kit.zip"

HERE = pathlib.Path(__file__).resolve().parent
SRC, DIST = HERE / "src", HERE / "dist"


def header(rel):
    """The top bar for the page at rel (e.g. "blog/x/index.html"): links relative to it,
    the page's own section marked aria-current. The setup app keeps its connection status."""
    depth = rel.count("/")
    up = "../" * depth
    home = depth == 0
    items = []
    for label, path in MENU:
        attrs = ""
        if path.startswith("http"):
            cls = "menu-icon" if label in ICONS else "gh"
            href, attrs = path, f' class="{cls}" target="_blank" rel="noopener noreferrer"'
        elif path.startswith("#"):
            href = path if home else up + path   # on the home page: scroll-spy links
        else:
            href = up + path
            if rel.startswith(path):
                attrs = ' aria-current="page"'
                if rel == path + "index.html":
                    href = "./"
        text = (f'{ICONS[label]}<span class="icon-label">{label}</span>' if label in ICONS else label)
        if label in ICONS:
            attrs += f' title="{{{{NAME}}}} on {label}"'
        items.append(f'        <li><a href="{href}"{attrs}>{text}</a></li>')
    status = ('\n    <div class="conn" id="status" aria-live="polite"></div>'
              if rel.startswith("setup/") else "")
    brand = "#main" if home else up
    return ('<header class="top">\n  <div class="wrap">\n'
            f'    <a class="brand" href="{brand}">{LOGO}{{{{NAME}}}}</a>\n'
            '    <nav aria-label="Site">\n      <ul>\n' + "\n".join(items) +
            f'\n      </ul>\n    </nav>{status}\n  </div>\n</header>')


LATEST = 3   # how many posts the home page's "Latest from the blog" shows


def latest_posts():
    """The newest posts for the home page ({{LATEST}}): the first cards of the blog's
    own list (src/blog/index.html, newest first), so a post added there shows on the
    home page too. Links and pictures are made relative to the site's root, and the
    card titles become h3 under the section's h2."""
    blog = (SRC / "blog" / "index.html").read_text()
    lst = re.search(r'<ol class="post-list"[^>]*>(.*?)</ol>', blog, re.S)
    items = re.findall(r'\n( *<li>.*?</li>)', lst.group(1), re.S)[:LATEST]
    out = []
    for item in items:
        item = re.sub(r'href="(?!https?:|/|\.\./)([^"]+)"', r'href="blog/\1"', item)
        item = item.replace('src="../', 'src="')
        item = re.sub(r'<h2 class="post-card-title">(.*?)</h2>', r'<h3 class="post-card-title">\1</h3>', item, flags=re.S)
        out.append(item)
    return '    <ol class="post-list">\n' + "\n".join(out) + '\n    </ol>'


def touch_icon(text):
    """Every page links favicon.svg; give each the .ico and Apple touch icon beside it.
    They are linked from brand/assets/icons/: the host answers the root
    /favicon.ico and /apple-touch-icon.png with its own placeholders."""
    return re.sub(r'(<link rel="icon" href="((?:\.\./)*)favicon\.svg" type="image/svg\+xml">)',
                  r'\1\n<link rel="icon" href="\2brand/assets/icons/favicon.ico" sizes="16x16 32x32 48x48">'
                  r'\n<link rel="apple-touch-icon" href="\2brand/assets/icons/apple-touch-icon.png">', text, count=1)


def brand_kit():
    assets = DIST / "brand" / "assets"
    with zipfile.ZipFile(DIST / KIT, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(p for p in assets.rglob("*") if p.is_file()):
            info = zipfile.ZipInfo("sticky-os-brand-kit/" + f.relative_to(assets).as_posix(), (2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            z.writestr(info, f.read_bytes())


# ---- screenshots, smooth

PANEL = {(480, 800), (800, 480)}


def smooth_versions(png):
    """The smooth 1x/2x/3x versions of a board capture: [(width, path)], or [] if it isn't one.

    A capture is the board's 1-bit frame (sometimes saved doubled). Scaled by a
    browser, its pixels come out uneven (some rows two pixels, some three) or
    blurred. Drawn here instead: enlarged 6x, softened by about half a board
    pixel, and the soft edge pulled back into a clean anti-aliased contour, then
    reduced to each size: the letters keep their shapes and read like print."""
    im = Image.open(png)
    if im.mode not in ("1", "L", "P") or len(im.convert("L").getcolors(256) or range(3)) > 2:
        return []  # photos, covers, anything with greys: as they are
    im = im.convert("L")
    if (im.width // 2, im.height // 2) in PANEL and im.size not in PANEL:
        im = im.resize((im.width // 2, im.height // 2), Image.NEAREST)  # saved doubled: the board's own pixels
    if im.size not in PANEL:
        return []
    k = 6
    big = im.resize((im.width * k, im.height * k), Image.NEAREST).filter(ImageFilter.GaussianBlur(0.55 * k))
    lo, hi = 96, 160
    big = big.point([0 if v <= lo else 255 if v >= hi else (v - lo) * 255 // (hi - lo) for v in range(256)])
    out = []
    for scale in (1, 2, 3):
        w, h = im.width * scale, im.height * scale
        path = png.with_name(f"{png.stem}.{w}.webp")
        big.resize((w, h), Image.LANCZOS).save(path, "WEBP", quality=88, method=6)
        out.append((w, path))
    return out


def smooth_shots():
    """Makes the smooth versions and points the pages at them (srcset; lightbox links at 3x)."""
    if Image is None:
        print("build: Pillow missing, screenshots left as captured (pip install pillow)")
        return 0
    made = {}
    for png in sorted((DIST / "shots").glob("*.png")):
        versions = smooth_versions(png)
        if versions:
            made[png.name] = versions
    shot = re.compile(r'((?:\.\./)*shots/)([a-z0-9-]+\.png)')
    for page in DIST.rglob("*.html"):
        text = page.read_text()

        def img(m):
            tag = m.group(0)
            found = shot.search(tag)
            if not found or found.group(2) not in made or "srcset=" in tag:
                return tag
            base, versions = found.group(1), made[found.group(2)]
            srcset = ", ".join(f"{base}{p.name} {i + 1}x" for i, (w, p) in enumerate(versions))
            return tag[:-1].rstrip("/").rstrip() + f' srcset="{srcset}">'

        def link(m):
            found = shot.search(m.group(0))
            if not found or found.group(2) not in made:
                return m.group(0)
            return m.group(0).replace(found.group(2), made[found.group(2)][-1][1].name)

        text = re.sub(r"<img\b[^>]*>", img, text)
        text = re.sub(r'<a\b[^>]*href="[^"]*shots/[a-z0-9-]+\.png"', link, text)
        page.write_text(text)
    return len(made)


def main():
    keep = {".spacefast", ".gitignore"}   # sf's link to the Space survives a rebuild
    DIST.mkdir(exist_ok=True)
    for old in DIST.iterdir():
        if old.name in keep:
            continue
        shutil.rmtree(old) if old.is_dir() else old.unlink()
    shutil.copytree(SRC, DIST, dirs_exist_ok=True)
    for page in DIST.rglob("*.html"):
        text = page.read_text()
        if "{{HEADER}}" in text:
            text = text.replace("{{HEADER}}", header(page.relative_to(DIST).as_posix()))
        if "{{LATEST}}" in text:
            text = text.replace("{{LATEST}}", latest_posts())
        page.write_text(touch_icon(text).replace("{{NAME}}", NAME))
    brand_kit()
    smooth = smooth_shots()
    shots = sorted(p.name for p in (DIST / "shots").glob("*.png"))
    print(f"built {DIST} as {NAME!r}; screenshots ({smooth} drawn smooth): {', '.join(shots)}")


def publish_settings():
    """STICKY_SITE_SPACE and STICKY_SITE_PROFILE: the environment, else publish.local."""
    found = {}
    local = HERE / "publish.local"
    if local.exists():
        for line in local.read_text().splitlines():
            key, sep, value = line.strip().partition("=")
            if sep and not key.startswith("#"):
                found[key.strip()] = value.strip()
    found.update({k: v for k, v in os.environ.items() if k.startswith("STICKY_SITE_")})
    missing = [k for k in ("STICKY_SITE_SPACE", "STICKY_SITE_PROFILE") if not found.get(k)]
    if missing:
        sys.exit("can't publish: set " + " and ".join(missing) +
                 " in the environment or in site/publish.local (KEY=value lines; git-ignored)")
    return found["STICKY_SITE_SPACE"], found["STICKY_SITE_PROFILE"]


if __name__ == "__main__":
    if "--publish" in sys.argv[1:]:
        space, profile = publish_settings()   # checked before building: a clear error first
        if Image is None:
            sys.exit("build: --publish needs Pillow (pip install pillow): without it the screenshots go out unsmoothed")
        main()
        subprocess.run(["sf", "publish", str(DIST), "--profile", profile, "--space", space,
                        "--mode", "website", "-y", "--wait"], check=True)
    else:
        main()
