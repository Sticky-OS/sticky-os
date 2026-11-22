// Games: the Daily screen's Nonogram (games_nonogram.ino; issue #18). Ids S_NONO_*; see strings.h
// for the format. What it says as Sudoku does (Solved!, Solved in, Under way, Best, A new best
// time!, Come back tomorrow, Solved today, the no-date notice) is Sudoku's: strings_sudoku.h.
S(S_NONO_NAME, "Nonogram")  // the puzzle's name (a picture logic puzzle, also called picross or griddler): a row on the Daily screen, the status bar
S(S_NONO_L_HINT, "8 x 8 or 10 x 10")  // the Daily screen's line under Nonogram before one is solved (12pt, ~300 px)
S(S_NONO_EASY, "Easy: 8 x 8")  // the sizes: buttons on the sizes card (~380 px, bold 12pt)
S(S_NONO_NORMAL, "Normal: 10 x 10")
S(S_NONO_CARD, "Today's nonogram")  // the sizes card's title, bold 18pt, ~300 px
S(S_NONO_SIZE, "Size")  // a button top right, ~120 px (bold 9pt): back to the sizes card
S(S_NONO_FILL, "Fill")  // the switch for what a tap does: fill a square (bold 12pt, ~200 px upright, ~160 sideways)
S(S_NONO_CROSS, "Cross")  // ... or cross it out, as known to be empty
// Under the Fill / Cross switch (9pt, ~430 px, up to 3 lines) or beside it (9pt, ~300 px, up to 6).
S(S_NONO_HOW_FILL, "The numbers are the runs of filled squares in each row and column. Tap to fill.")
S(S_NONO_HOW_CROSS, "Tap a square you know is empty to cross it out.")
