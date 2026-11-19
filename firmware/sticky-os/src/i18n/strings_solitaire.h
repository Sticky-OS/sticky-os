// Games: Solitaire (games_solitaire.ino). Ids S_SOL_*; see strings.h for the format.
// Card ranks (A, 2..10, J, Q, K) are drawn as they are in every language.
S(S_SOL_UNDO, "Undo")  // a button, ~96 px upright: take back the last move
S(S_SOL_MOVES, "1 move|%ld moves")  // moves made in this deal: the header (~106 px upright, bold 12pt or 9pt)
S(S_SOL_DRAW1, "Draw 1")  // the stock turns one card at a time; a button on the New game card, and a line under the moves
S(S_SOL_DRAW3, "Draw 3")  // ... three at a time
S(S_SOL_DRAW1_LINE, "One card at a time")  // under the Draw 1 button (9pt, ~170 px)
S(S_SOL_DRAW3_LINE, "Three at a time: harder")
S(S_SOL_WON, "You won!")  // the card over the board at the end (bold 18pt)
S(S_SOL_WON_MOVES, "In 1 move|In %ld moves")
S(S_SOL_BEST, "Best: 1 move|Best: %ld moves")  // the fewest moves in a win so far
S(S_SOL_NEW_BEST, "Your best yet!")  // this win took the fewest moves so far
S(S_SOL_STUCK, "No moves left")  // the deal can't get anywhere: upright in place of the moves (bold 9pt, ~106 px, wraps to 2 lines); sideways 116 px, 2 lines
S(S_SOL_STUCK_HINT, "Undo, or start a new game.")  // sideways, under it (9pt, 116 px wide, wraps)
S(S_SOL_HOW, "Tap a card to move it. Tap the stock to turn a card.")  // sideways, beside the cards (9pt, 116 px wide, wraps)
S(S_SOL_WHERE, "Now tap where it goes.")  // a card was picked up (drawn black)
S(S_SOL_L_ON, "Game on: 1 move|Game on: %ld moves")  // the launcher, under the name (9pt, ~150 px): a deal under way
S(S_SOL_L_WON, "Won %d of %d")  // the launcher: deals won, of deals played
S(S_SOL_L_HINT, "Klondike, draw 1 or 3")  // the launcher, before any win
