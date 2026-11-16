#!/bin/bash
# The reading fonts on this computer (bookfonts.h, gfxdraw.h):
#   ./fonts.sh                     fonts_test.cpp
#   ./fonts.sh specimen <folder>   tools/fontspecimen.cpp: the specimen sheets, as PNGs in <folder>
# Unlike run.sh this builds with the stand-in <Arduino.h> (shim/) and the
# firmware's glyphs.cpp, which the text code needs.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
CXX=${CXX:-c++}
SRC=../../sticky-os
FLAGS=(-std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter -fno-exceptions -fno-rtti -fsanitize=undefined -fno-sanitize-recover=undefined -I"$SRC" -I. -Ishim)
# Some macOS command line tools don't find libc++'s headers by themselves.
if ! echo '#include <new>' | "$CXX" -std=c++17 -x c++ -fsyntax-only - 2>/dev/null; then
  SDK=$(xcrun --show-sdk-path 2>/dev/null || true)
  [ -n "$SDK" ] && FLAGS+=(-isysroot "$SDK" -I"$SDK/usr/include/c++/v1")
fi
if [ "${1:-}" = specimen ]; then
  "$CXX" "${FLAGS[@]}" -o build/fontspecimen tools/fontspecimen.cpp "$SRC/bookfonts.cpp" "$SRC/glyphs.cpp"
  mkdir -p "${2:?usage: fonts.sh specimen <folder>}"
  exec ./build/fontspecimen "$2"
fi
"$CXX" "${FLAGS[@]}" -o build/fonts_shim fonts_test.cpp "$SRC/bookfonts.cpp" "$SRC/glyphs.cpp"
echo "== fonts"
./build/fonts_shim "$PWD/fixtures"
