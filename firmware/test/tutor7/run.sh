#!/bin/bash
# Tutor7's exercise generators, on this computer: ./run.sh [samples]
set -euo pipefail
cd "$(dirname "$0")"
BUILD=${BUILD:-build}  # another folder lets two builds run at once
mkdir -p "$BUILD"
CXX=${CXX:-c++}
FLAGS=(-std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-undefined-internal -I../../sticky-os)
# Some macOS command line tools don't find libc++'s headers by themselves.
if ! echo '#include <new>' | "$CXX" -std=c++17 -x c++ -fsyntax-only - 2>/dev/null; then
  SDK=$(xcrun --show-sdk-path 2>/dev/null || true)
  [ -n "$SDK" ] && FLAGS+=(-isysroot "$SDK" -I"$SDK/usr/include/c++/v1")
fi
"$CXX" "${FLAGS[@]}" -o "$BUILD/tutor7_test" tutor7_test.cpp ../../sticky-os/tutor7_core.cpp ../../sticky-os/tutor7_u4.cpp ../../sticky-os/tutor7_u5.cpp ../../sticky-os/tutor7_u6.cpp ../../sticky-os/tutor7_lessons.cpp ../../sticky-os/tutor7_text.cpp ../../sticky-os/i18n_tables.cpp
"./$BUILD/tutor7_test" "$@"
