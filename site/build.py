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

The setup app is src/setup/ (served at setup/; setup/?demo runs it against a
simulated board). Its tests: node test/run.mjs, from this folder.
"""
import os
import re
import pathlib
import shutil
import subprocess
import sys

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
    ("GitHub", "https://github.com/ovidiul/sticky-os"),
]
LOGO = ('<svg width="18" height="20" viewBox="0 0 18 20" aria-hidden="true"><rect x="1.5" y="1.5" '
        'width="15" height="17" rx="3" fill="none" stroke="currentColor" stroke-width="2.4"/>'
        '<path d="M5 7h8M5 11h5" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/></svg>')

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
            href, attrs = path, ' class="gh" target="_blank" rel="noopener noreferrer"'
        elif path.startswith("#"):
            href = path if home else up + path   # on the home page: scroll-spy links
        else:
            href = up + path
            if rel.startswith(path):
                attrs = ' aria-current="page"'
                if rel == path + "index.html":
                    href = "./"
        items.append(f'        <li><a href="{href}"{attrs}>{label}</a></li>')
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
        page.write_text(text.replace("{{NAME}}", NAME))
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
