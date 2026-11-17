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
    """Every page links favicon.svg; give each the Apple touch icon beside it."""
    return re.sub(r'(<link rel="icon" href="((?:\.\./)*)favicon\.svg" type="image/svg\+xml">)',
                  r'\1\n<link rel="apple-touch-icon" href="\2apple-touch-icon.png">', text, count=1)


def brand_kit():
    assets = DIST / "brand" / "assets"
    with zipfile.ZipFile(DIST / KIT, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(p for p in assets.rglob("*") if p.is_file()):
            info = zipfile.ZipInfo("sticky-os-brand-kit/" + f.relative_to(assets).as_posix(), (2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            z.writestr(info, f.read_bytes())


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
    shots = sorted(p.name for p in (DIST / "shots").glob("*.png"))
    print(f"built {DIST} as {NAME!r}; screenshots: {', '.join(shots)}")


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
        main()
        subprocess.run(["sf", "publish", str(DIST), "--profile", profile, "--space", space,
                        "--mode", "website", "-y", "--wait"], check=True)
    else:
        main()
