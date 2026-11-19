// Games: Mines (games_mines.ino). Ids S_MINES_*; see strings.h for the format.
S(S_MINES_DIG, "Dig")  // the switch for what a tap does: open the square (bold 12pt, ~70 px with its icon beside it)
S(S_MINES_FLAG, "Flag")  // ... or mark it as a mine
S(S_MINES_SMALL, "Small")  // board sizes: buttons on the New game card, ~350 px
S(S_MINES_MEDIUM, "Medium")
S(S_MINES_LARGE, "Large")
S(S_MINES_SIZE_LINE, "%d x %d, %d mines")  // columns, rows, mines: under a size's button; sideways under the size's name (9pt, ~195 px)
S(S_MINES_BEST, "best %s")  // after the size line: the quickest win, as "1:04"
S(S_MINES_FIRST, "Tap any square: the first one is safe.")  // a new board, before the first tap
S(S_MINES_HOW_DIG, "Tap to dig. Tap a number with its flags to open around it.")
S(S_MINES_HOW_FLAG, "Tap a square to flag a mine, again to take it off.")
S(S_MINES_WON, "Cleared in %s!")  // the board won; %s the time, "1:23". Bold 12pt, ~220 px upright
S(S_MINES_WON_BEST, "Cleared in %s, a best!")  // ... the quickest at this size yet
S(S_MINES_LOST, "Boom! That was a mine.")
S(S_MINES_L_LEFT, "%d mines to find")  // the launcher, under the name (9pt, ~150 px): a board under way
S(S_MINES_L_WON, "Cleared %d")  // the launcher: boards won, all sizes
S(S_MINES_L_HINT, "Find the mines")  // the launcher, before any win
