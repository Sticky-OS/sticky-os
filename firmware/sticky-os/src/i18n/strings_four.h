// Games: Four in a Row (games_four.ino). Ids S_FOUR_*; see strings.h for the format.
// Beside the board: ~250 px wide in landscape (bold 12pt, up to three lines), ~430 in portrait (two lines).
S(S_FOUR_START, "You're the rings. Tap a column.")  // you start: your discs are drawn as rings, the board's solid
S(S_FOUR_BOARD_FIRST, "The board went first. Your turn.")
S(S_FOUR_YOUR_TURN, "Your turn.")
S(S_FOUR_WIN, "You win!")  // bold 18pt
S(S_FOUR_LOSE, "The board wins.")  // bold 18pt
S(S_FOUR_DRAW, "A draw.")  // bold 18pt: the board filled up and nobody made four
S(S_FOUR_EASY, "Easy")  // the levels: three pill buttons (bold 9pt), in a row when they fit, else stacked
S(S_FOUR_MEDIUM, "Medium")
S(S_FOUR_HARD, "Hard")
S(S_FOUR_TALLY, "Won %d   Drawn %d   Lost %d")  // one line, 12pt (9pt when it doesn't fit ~250 px)
S(S_FOUR_L_UNDER_WAY, "A game under way")  // the launcher's line under the name (9pt, ~120 px)
S(S_FOUR_L_TALLY, "Won %d, drawn %d")
S(S_FOUR_L_HINT, "Line up four")
