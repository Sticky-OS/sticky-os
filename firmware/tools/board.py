#!/usr/bin/env python3
"""Drive a board running Sticky OS from this computer, over its USB serial port.

    board.py cmd "book list"              send a dev command, print what comes back
    board.py shot out.png                 the screen as drawn, as a PNG (needs Pillow)
    board.py shot4 out.png                the four-grey frame last sent (a photo, a book's cover)
    board.py put book.epub books/x.epub   copy a file onto the microSD card (sdput)
    board.py ls [books]                   list a folder of the card
    board.py run "book open 0" wait:2 shot:a.png "book next" shot:b.png
                                          several steps in one session: a dev
                                          command, wait:<seconds>, shot:<file>,
                                          put:<file>:<card path>

PORT=/dev/cu.usbmodem... picks the port (default: the first one there is).
Opening the port resets the board on some computers; this opens it without
touching DTR/RTS and keeps it open for the whole run. The dev commands are
in CONTRIBUTING.md.
"""
import base64
import glob
import os
import sys
import time
import urllib.parse

import serial  # pyserial


def port():
    p = os.environ.get("PORT") or next(iter(sorted(glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*"))), None)
    if not p:
        sys.exit("no board found; set PORT=/dev/...")
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = p, 115200, 0.2
    s.dtr = s.rts = False
    s.open()
    # On boards whose USB bridge resets the chip when the port opens, wait
    # for it to be up (the setup app's banner), so the first command isn't lost.
    try:
        for line in lines(s, lambda l: l.startswith('@cfg {"ready"'), 1.5):
            if line.startswith("sticky: starting"):
                for _ in lines(s, lambda l: l.startswith('@cfg {"ready"'), 30):
                    pass
                time.sleep(0.5)
                break
    except TimeoutError:
        pass
    return s


def lines(s, until, timeout):
    """Lines from the board until one for which until(line) is true (that one included)."""
    buf, end = b"", time.time() + timeout
    while time.time() < end:
        buf += s.read(4096)
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            text = line.decode("utf-8", "replace").rstrip("\r")
            yield text
            if until(text):
                return
    raise TimeoutError("the board didn't answer in time")


def cmd(s, command, until=None, timeout=20.0, echo=True):
    s.reset_input_buffer()
    s.write((command + "\n").encode())
    out = []
    try:
        for line in lines(s, until or (lambda l: False), timeout):
            out.append(line)
            if echo:
                print(line)
    except TimeoutError:
        if until:
            raise
    return out


def shot(s, path, tries=4):
    from PIL import Image

    for _ in range(tries):  # a log line from another task can land inside a row: ask again
        s.reset_input_buffer()
        s.write(b"shot\n")
        rows, head = [], None
        for line in lines(s, lambda l: l == "END" and head is not None, 60):
            if line.startswith("SHOT "):
                head = [int(v) for v in line.split()[1:4]]
            elif head and len(line) == head[0] // 4:
                try:
                    rows.append(bytes.fromhex(line))
                except ValueError:
                    pass
        if head and len(rows) == head[1]:
            break
        time.sleep(1)
    else:
        sys.exit(f"{path}: the picture came back damaged {tries} times")
    w, h, rot = head
    img = Image.frombytes("1", (w, h), b"".join(rows))
    # The buffer is the panel's 800 x 480, whichever way the picture is turned
    # (Adafruit GFX's setRotation): turn it back to the picture as seen.
    turn = {1: Image.Transpose.ROTATE_90, 2: Image.Transpose.ROTATE_180, 3: Image.Transpose.ROTATE_270}.get(rot)
    if turn is not None:
        img = img.transpose(turn)
    img.save(path)
    print(f"{path}: {img.size[0]} x {img.size[1]}")


def shot4(s, path, tries=4):
    from PIL import Image

    for _ in range(tries):
        s.reset_input_buffer()
        s.write(b"shot4\n")
        rows, head = [], None
        for line in lines(s, lambda l: l == "END" and head is not None, 90):
            if line.startswith("SHOT4 "):
                head = [int(v) for v in line.split()[1:4]]
            elif head and len(line) == head[0] // 2:
                try:
                    rows.append(bytes.fromhex(line))
                except ValueError:
                    pass
        if head and len(rows) == head[1]:
            break
        time.sleep(1)
    else:
        sys.exit(f"{path}: the picture came back damaged {tries} times")
    w, h, rot = head
    px = bytearray()
    for row in rows:  # 2 bits a pixel, the leftmost in the top bits, 0 black
        for b in row:
            for sh in (6, 4, 2, 0):
                px.append((b >> sh & 3) * 85)
    img = Image.frombytes("L", (w, h), bytes(px))
    turn = {1: Image.Transpose.ROTATE_90, 2: Image.Transpose.ROTATE_180, 3: Image.Transpose.ROTATE_270}.get(rot)
    if turn is not None:
        img = img.transpose(turn)
    img.save(path)
    print(f"{path}: {img.size[0]} x {img.size[1]}, four greys")


def put(s, src, dst, chunk=4096):
    data = open(src, "rb").read()
    t = time.time()
    for off in range(0, max(1, len(data)), chunk):
        piece = base64.b64encode(data[off:off + chunk]).decode()
        s.reset_input_buffer()
        s.write(f"sdput {urllib.parse.quote(dst)} {off} {piece}\n".encode())
        ok = False
        for line in lines(s, lambda l: l.startswith("SDPUT "), 20):
            if line.startswith("SDPUT "):
                ok = not line.endswith("failed")
        if not ok:
            sys.exit(f"{dst}: failed at byte {off}")
        print(f"\r{dst}: {min(len(data), off + chunk) * 100 // max(1, len(data))}%", end="", flush=True)
    print(f"\r{dst}: {len(data)} bytes in {time.time() - t:.0f} s")


def step(s, a):
    if a.startswith("wait:"):
        time.sleep(float(a[5:]))
    elif a.startswith("shot:"):
        shot(s, a[5:])
    elif a.startswith("shot4:"):
        shot4(s, a[6:])
    elif a.startswith("put:"):
        src, dst = a[4:].split(":", 1)
        put(s, src, dst)
    else:
        ends = {"book list": "END", "book toc": "END", "book peek": "END"}
        if a.startswith(("sdls ", "sdrm ", "sdmkdir ")):
            verb, path = a.split(" ", 1)
            a = verb + " " + urllib.parse.quote(path)
        if a in ends or a.startswith("sdls"):
            cmd(s, a, lambda l: l in ("END", "SDLS failed", "SD no card"), 120)
        elif a.startswith("book open"):
            cmd(s, a, lambda l: l.startswith("book: open"), 600)
        elif a.startswith("book bench"):
            cmd(s, a, lambda l: l.startswith("book: bench"), 600)
        else:
            cmd(s, a, None, float(os.environ.get("WAIT", "2.5")))


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    s = port()
    what = sys.argv[1]
    if what == "cmd":
        step(s, sys.argv[2])
    elif what == "run":
        for a in sys.argv[2:]:
            step(s, a)
    elif what == "shot":
        shot(s, sys.argv[2])
    elif what == "shot4":
        shot4(s, sys.argv[2])
    elif what == "put":
        put(s, sys.argv[2], sys.argv[3])
    elif what == "ls":
        cmd(s, "sdls " + (sys.argv[2] if len(sys.argv) > 2 else ""), lambda l: l in ("END", "SDLS failed", "SD no card"))
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
