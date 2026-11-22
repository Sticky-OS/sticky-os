#!/usr/bin/env python3
"""Brings the games' demo captures into the site, for the help page (src/games/).

    python3 tools/import_demos.py                 # every game whose captures are there
    python3 tools/import_demos.py chess 2048      # just these
    python3 tools/import_demos.py --from DIR      # captures somewhere else

The captures come from the board, one folder per game with a manifest.json
(DIR/<slug>/manifest.json; DIR is --from, else $STICKY_HOWTO_CAPS, else
~/Desktop/openwrt/devices/inksim/shorts/howto/cap, where the Shorts take them from):

    {"game": "Chess", "orientation": "portrait",
     "steps": [{"file": "01-new.png", "caption": "Tap a piece.",
                "tap": {"x": 240, "y": 410} or null,     # panel pixels
                "shows": "What the screen shows (the alt text)", "clock": "15:40"}]}

Each game's captures are checked (the board's 1-bit frames, 480 x 800 upright or
800 x 480 sideways, or those doubled; taps on the screen; a caption each) and
copied, with a manifest holding only what the page uses, to
src/shots/demo/<slug>/ (its old captures removed first). Then rebuild:
build.py draws the steps in the game's <!-- demo:<slug> --> block and smooths
the captures like every other screenshot. A game with no captures yet keeps
the single screenshot written in its block.

Captures go on a public site: demo data only, no names, WiFi names or places.
Needs Pillow (pip install pillow).
"""
import json
import os
import pathlib
import shutil
import sys

from PIL import Image

SLUGS = ["2048", "memory", "maths-quest", "daily-maths", "quiz", "adventure",
         "xo", "tutor", "solitaire", "mines", "four", "chess"]   # the page's anchors too
HERE = pathlib.Path(__file__).resolve().parent.parent
DEST = HERE / "src" / "shots" / "demo"
DEFAULT = pathlib.Path.home() / "Desktop/openwrt/devices/inksim/shorts/howto/cap"
SIZE = {"portrait": (480, 800), "landscape": (800, 480)}


def check(slug, folder):
    """The manifest, cleaned, or SystemExit with what is wrong."""
    def bad(why):
        sys.exit(f"{slug}: {why}")
    m = json.loads((folder / "manifest.json").read_text())
    orient = m.get("orientation", "portrait")
    if orient not in SIZE:
        bad(f"orientation {orient!r}: portrait or landscape")
    w, h = SIZE[orient]
    steps = m.get("steps") or bad("no steps")
    out = []
    for i, s in enumerate(steps, 1):
        f = s.get("file", "")
        if not f.endswith(".png") or "/" in f or not (folder / f).is_file():
            bad(f"step {i}: no capture {f!r}")
        im = Image.open(folder / f)
        if im.size not in ((w, h), (w * 2, h * 2)):
            bad(f"step {i}: {f} is {im.size[0]} x {im.size[1]}, not the {orient} screen ({w} x {h})")
        if len(im.convert("L").getcolors(256) or range(3)) > 2:
            print(f"{slug}: step {i}: {f} has greys; it is shown as it is, not drawn smooth")
        caption = (s.get("caption") or "").strip() or bad(f"step {i}: no caption")
        tap = s.get("tap")
        if tap is not None:
            x, y = tap.get("x"), tap.get("y")
            if not (isinstance(x, (int, float)) and isinstance(y, (int, float)) and 0 <= x < w and 0 <= y < h):
                bad(f"step {i}: tap {tap} is off the {w} x {h} screen")
            tap = {"x": round(x), "y": round(y)}
        shows = (s.get("shows") or "").strip()
        if not shows:
            print(f"{slug}: step {i}: no 'shows', the caption is its alt text")
        out.append({"file": f, "caption": caption, "tap": tap, "shows": shows or caption})
    return {"game": m.get("game") or slug, "orientation": orient, "steps": out}


def main(args):
    src = pathlib.Path(os.environ.get("STICKY_HOWTO_CAPS", DEFAULT))
    if "--from" in args:
        i = args.index("--from")
        src = pathlib.Path(args[i + 1]).expanduser()
        del args[i:i + 2]
    unknown = [a for a in args if a not in SLUGS]
    if unknown:
        sys.exit(f"unknown game {', '.join(unknown)}; the games are {', '.join(SLUGS)}")
    done, missing = [], []
    for slug in args or SLUGS:
        folder = src / slug
        if not (folder / "manifest.json").is_file():
            missing.append(slug)
            continue
        m = check(slug, folder)
        dest = DEST / slug
        if dest.exists():
            shutil.rmtree(dest)
        dest.mkdir(parents=True)
        for s in m["steps"]:
            shutil.copy2(folder / s["file"], dest / s["file"])
        (dest / "manifest.json").write_text(json.dumps(m, indent=1, ensure_ascii=False) + "\n")
        done.append(f"{slug} ({len(m['steps'])} steps)")
    print("imported: " + (", ".join(done) or "nothing"))
    have = sorted(p.parent.name for p in DEST.glob("*/manifest.json"))
    print("on the page as demos: " + (", ".join(have) or "none"))
    print("single screenshot still: " + (", ".join(s for s in SLUGS if s not in have) or "none"))
    if missing:
        print(f"no captures in {src} for: {', '.join(missing)}")
    print("now: python3 build.py")


if __name__ == "__main__":
    main(sys.argv[1:])
