#!/bin/bash
# The games' rules from CrossPlay (Solitaire, Mines, Four in a Row, Chess),
# on this computer: ./run.sh
# The test_*.cpp files are CrossPlay's own host tests (host-tests/<game>/),
# with only their include paths changed; src/crossplay/README.md says from
# which commit.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build
CXX=${CXX:-c++}
SRC=../../sticky-os/src/crossplay
FLAGS=(-std=c++17 -O2 -Wall -Wextra -Werror -I"$SRC" -I"$SRC/minesweeper" -I"$SRC/connectfour")
# Some macOS command line tools don't find libc++'s headers by themselves.
if ! echo '#include <new>' | "$CXX" -std=c++17 -x c++ -fsyntax-only - 2>/dev/null; then
  SDK=$(xcrun --show-sdk-path 2>/dev/null || true)
  [ -n "$SDK" ] && FLAGS+=(-isysroot "$SDK" -I"$SDK/usr/include/c++/v1")
fi
run() {
  local name=$1; shift
  echo "== $name"
  "$CXX" "${FLAGS[@]}" -o "build/$name" "$@"
  "./build/$name"
}
run chess test_chess.cpp "$SRC/chess/ChessCore.cpp" "$SRC/chess/ChessEngine.cpp"
run connectfour test_connectfour.cpp
run minesweeper test_minesweeper.cpp
run solitaire test_solitaire.cpp "$SRC/solitaire/SolitaireCore.cpp"
# Sticky OS's own code around them: Solitaire's taps (solui.h).
run solui -I../../sticky-os test_solui.cpp "$SRC/solitaire/SolitaireCore.cpp"
echo "games: all passed"
