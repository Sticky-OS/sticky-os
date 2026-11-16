#!/bin/bash
# Builds tools/book2flow.cpp with run.sh's flags and objects, then runs it:
#   tools/book2flow.sh [-d] [-n bytes] book outdir
set -euo pipefail
cd "$(dirname "$0")/.."
here=$PWD
cd - >/dev/null
CXX=${CXX:-c++}
FLAGS=(-std=c++17 -O1 -g -fno-exceptions -fno-rtti -fsanitize=undefined -fno-sanitize-recover=undefined -I"$here/../../sticky-os" -I"$here" -I"$here/shim")
if ! echo '#include <new>' | "$CXX" -std=c++17 -x c++ -fsyntax-only - 2>/dev/null; then
  SDK=$(xcrun --show-sdk-path 2>/dev/null || true)
  [ -n "$SDK" ] && FLAGS+=(-isysroot "$SDK" -I"$SDK/usr/include/c++/v1")
fi
# The objects are run.sh's: build them first (./run.sh flow is quick).
(cd "$here" && ./run.sh flow >/dev/null)
bin=$here/build/book2flow
if [ ! -e "$bin" ] || [ -n "$(find "$here/build" -name '*.o' -newer "$bin" | head -1)" ] || [ "$here/tools/book2flow.cpp" -nt "$bin" ] || [ "$here/flowcheck.h" -nt "$bin" ]; then
  "$CXX" "${FLAGS[@]}" -o "$bin" "$here/tools/book2flow.cpp" "$here"/build/book*.o "$here/build/ebook.o" "$here/build/glyphs.o" 2>&1 | grep -v '^ld: warning' || true
fi
exec "$bin" "$@"
