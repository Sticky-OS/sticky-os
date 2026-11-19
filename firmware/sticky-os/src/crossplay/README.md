# The games' rules, from CrossPlay

The rules of Solitaire (Klondike), Mines, Four in a Row and Chess come from
[CrossPlay](https://github.com/ma-r-s/crossplay), a fork of CrossPoint Reader
by Mario Ruiz, MIT licensed (`LICENSE` here, kept with the code). Sticky OS
draws them with its own screens (`games_*.ino`); only the rules are CrossPlay's.

Copied from commit `152f5d346e1445ebaf9636c436e07a6a70fb921b` (2026-11-18),
`src/apps_local/`, unchanged:

| Here | There |
|---|---|
| `cards/Cards.h` | `cards/Cards.h` |
| `solitaire/SolitaireCore.{h,cpp}` | `solitaire/` |
| `minesweeper/MinesweeperCore.h`, `MinesweeperFlow.h` | `minesweeper/` |
| `connectfour/ConnectFourCore.h`, `ConnectFourBrain.h`, `ConnectFourFlow.h` | `connectfour/` |
| `chess/ChessCore.{h,cpp}`, `ChessEngine.{h,cpp}`, `ChessPieces.h` | `chess/` |

`LICENSE` is CrossPlay's, word for word. Its last paragraph (GPLv3 for the
combined executable because of wolfSSL, and `licenses/README.md`) is about
CrossPlay's own firmware build, not about these files: the MIT licence above
it is what covers the code copied here.

`chess/ChessPieces.h` is generated from the "celtic" piece set by Maurizio
Monge (github.com/maurimo/chess-art), MIT: `chess/LICENSE-pieces`.

Their tests are CrossPlay's `host-tests/<game>/test_<game>.cpp`, in
`firmware/test/games/` with only the include paths changed; run them with
`firmware/test/games/run.sh` (CI does).

To take a newer CrossPlay: copy the same files from its `src/apps_local/` and
`host-tests/`, run the tests, and update the commit above. Don't edit these
files here; a change Sticky OS needs goes in its own code around them, so the
next copy stays a copy.
