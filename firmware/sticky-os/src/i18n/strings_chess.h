// Games: Chess (games_chess.ino). Ids S_CHESS_*; see strings.h for the format.
// What's happening, bold 12pt: beside the board in landscape (~320 px, three lines), over it in portrait (~320 px, two lines).
S(S_CHESS_START, "You're White. Tap a piece to move it.")
S(S_CHESS_WHERE, "Now tap where it goes.")  // a piece is picked: its moves show as dots (captures: corners)
S(S_CHESS_YOUR_MOVE, "Your move.")
S(S_CHESS_CHECK, "Check!")  // your king is attacked
S(S_CHESS_BOARD_PLAYED, "The board played %s.")  // %s: its move in chess notation, "Nf6", "exd5", "O-O"
S(S_CHESS_THINKING, "Thinking...")  // the board is choosing its move (a second or so)
S(S_CHESS_PROMOTE, "Promote to:")  // a pawn reached the last row: four pieces to choose from follow
S(S_CHESS_MATE_WIN, "Checkmate. You win!")
S(S_CHESS_MATE_LOSE, "Checkmate. The board wins.")
S(S_CHESS_STALEMATE, "Stalemate: a draw.")  // the side to move has no move and isn't in check
S(S_CHESS_REPETITION, "A draw: the same position three times.")
S(S_CHESS_FIFTY, "A draw: fifty moves with no capture or pawn move.")
S(S_CHESS_MATERIAL, "A draw: neither side can checkmate.")  // too few pieces left
S(S_CHESS_RESIGNED, "You resigned. The board wins.")
S(S_CHESS_RESIGN_SURE, "Tap Resign again to give up this game.")
S(S_CHESS_EASY, "Easy")  // the levels: three pill buttons in a row, ~100 px each (bold 9pt)
S(S_CHESS_MEDIUM, "Medium")
S(S_CHESS_HARD, "Hard")
S(S_CHESS_TALLY, "Won %d   Drawn %d   Lost %d")  // one line, 12pt, ~320 px
S(S_CHESS_UNDO, "Undo")  // a button, ~125 px: takes back your last move and the board's reply
S(S_CHESS_RESIGN, "Resign")  // a button, ~190 px: give up the game (counts as lost)
S(S_CHESS_MENU, "Menu")  // beside three bars, ~140 px with them: opens the panel (levels, Undo, Resign, Back)
S(S_CHESS_CLOSE, "Close")  // the same button while the panel is open
S(S_CHESS_L_MOVE, "Move %d")  // the launcher's line under the name (9pt, ~120 px): a game under way, its move number
S(S_CHESS_L_TALLY, "Won %d, drawn %d")
S(S_CHESS_L_HINT, "You play White")
