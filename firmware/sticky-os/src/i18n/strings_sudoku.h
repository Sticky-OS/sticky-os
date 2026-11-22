// Games: the Daily screen and its Sudoku (games_sudoku.ino; issue #18). Ids S_DAILY_*, S_SUDOKU_*;
// see strings.h for the format. Sudoku's levels are by clues and by the techniques they need.
// ---- the Daily screen: a row a puzzle (its name bold 18pt, ~300 px; a line under it, 12pt, ~300 px)
S(S_DAILY_FOOT, "New puzzles every day, the same on every board.")  // 9pt, under the rows, ~440 px
S(S_DAILY_L_DONE, "Done today: %d of %d")  // the launcher, under Daily (9pt, ~150 px, two lines at most)
S(S_DAILY_L_HINT, "Maths, Sudoku, Nonogram")  // the launcher, before anything is done today
// ---- Sudoku
S(S_SUDOKU_NAME, "Sudoku")  // the puzzle's name: a row on the Daily screen, the status bar, titles
S(S_SUDOKU_EASY, "Easy")  // the levels: buttons on the levels card (~380 px, bold 12pt); in the header (bold 12pt, ~150 px)
S(S_SUDOKU_MEDIUM, "Medium")
S(S_SUDOKU_HARD, "Hard")
S(S_SUDOKU_EASY_HOW, "36 to 40 clues, singles solve it")  // under the level's button, 9pt, ~380 px
S(S_SUDOKU_MEDIUM_HOW, "30 to 34 clues, at times a pair")
S(S_SUDOKU_HARD_HOW, "26 to 29 clues, more to think")
S(S_SUDOKU_TODAY, "Today's sudoku")  // the levels card's title, bold 18pt, ~300 px
S(S_SUDOKU_SOLVED_IN, "Solved in %s")  // %s the time, "6:42": on the levels card; bold 18pt when solved
S(S_SUDOKU_UNDER_WAY, "Under way: %s")  // a level started today, the time so far
S(S_SUDOKU_BEST, "Best %s")  // the quickest solve at a level
S(S_SUDOKU_LEVELS, "Levels")  // a button top right, ~120 px (bold 9pt): back to the levels card
S(S_SUDOKU_ERASE, "Erase")  // pad buttons, ~140 px upright, ~95 px sideways (bold 12pt, else 9pt)
S(S_SUDOKU_PENCIL, "Pencil")  // a switch: numbers go in small, as notes
S(S_SUDOKU_UNDO, "Undo")
// Under the board (9pt, ~460 px, two lines) or beside it (9pt, ~300 px, four lines).
S(S_SUDOKU_HOW, "Tap a square, then a number.")
S(S_SUDOKU_HOW_PENCIL, "Pencil is on: numbers go in small, as notes.")
S(S_SUDOKU_GIVEN, "That number is given: it can't change.")
S(S_SUDOKU_CLASH, "Struck through: the same number twice in a row, column or box.")
S(S_SUDOKU_LEFT, "1 square to go|%ld squares to go")
S(S_SUDOKU_SOLVED, "Solved!")  // bold 24pt
S(S_SUDOKU_NEW_BEST, "A new best time!")
S(S_SUDOKU_TOMORROW, "Come back tomorrow for a new one.")
S(S_SUDOKU_L_DONE, "Solved today: %s")  // the Daily screen's line: %s the levels, "Easy, Hard"
S(S_SUDOKU_L_HINT, "Easy, Medium or Hard")
S(S_SUDOKU_NO_DATE, "Today's puzzle needs today's date. Connect to WiFi, then try again.")
