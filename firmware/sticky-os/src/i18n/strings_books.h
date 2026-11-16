// ---- Books (books.ino): the library, opening a book, the reader and its menu

// The library
S(S_BOOKS_READING, "Reading")  // a section of the list: books started and not finished
S(S_BOOKS_NEW, "New")  // a section: books never opened
S(S_BOOKS_FINISHED, "Finished")  // a section: books read to the end
S(S_BOOKS_CONTINUE, "Continue reading")  // the card at the top of the list: the book last read
S(S_BOOKS_PERCENT, "%d%%")  // how far into a book: "34%"
S(S_BOOKS_COUNT, "%ld book|%ld books")  // under the list
S(S_BOOKS_PAGE_OF, "%d / %d")  // the list's page
S(S_BOOKS_LOOKING, "Reading the card: %d of %d")  // while the titles are read from the books
S(S_BOOKS_TOO_MANY, "Only the first 500 books are listed.")
S(S_BOOKS_NO_CARD, "No microSD card")  // a title, big
S(S_BOOKS_NO_CARD_1, "Put a microSD card in the slot on the side, with your books on it in a folder named books.")
S(S_BOOKS_NONE, "No books yet")  // a title, big
S(S_BOOKS_NONE_1, "Copy books to the microSD card, into a folder named books (sub-folders are fine).")
S(S_BOOKS_FORMATS, "EPUB, MOBI and AZW3 without DRM, FB2, PDF, plain text and HTML.")
S(S_BOOKS_FREE, "Free books: gutenberg.org and standardebooks.org")

// Opening a book
S(S_BOOKS_PREPARING, "Getting the book ready")  // the first time a book is opened, over a progress bar
S(S_BOOKS_PREPARING_NOTE, "Only the first time: next time it opens at once.")
S(S_BOOKS_CANT_OPEN, "Can't open this book")  // a title, over the reason
S(S_BOOKS_BACK_TO_LIST, "Back to the books")  // a button, ~260 px
S(S_BOOKS_CARD_OUT, "The microSD card was taken out.")

// Why a book can't be opened (BookErr)
S(S_BOOKERR_OPEN, "The file can't be read from the card.")
S(S_BOOKERR_FORMAT, "This isn't a kind of book the board can read.")
S(S_BOOKERR_DAMAGED, "The file is damaged or cut short. Copy it again.")
S(S_BOOKERR_DRM, "This book is copy-protected (DRM). Only books without DRM can be read here.")
S(S_BOOKERR_ENCRYPTED, "This PDF is locked with a password.")
S(S_BOOKERR_NO_TEXT, "There's no text in this book to show: it may be scanned pages or pictures.")
S(S_BOOKERR_WRITE, "The card is full or locked: there's no room to get the book ready.")
S(S_BOOKERR_MEMORY, "Not enough memory to open this book.")

// The reader and its menu
S(S_BOOKS_LIBRARY, "Library")  // the menu's buttons, ~90 px each, under an icon, small
S(S_BOOKS_CONTENTS, "Contents")
S(S_BOOKS_BOOKMARKS, "Bookmarks")
S(S_BOOKS_TEXT, "Text")  // text size and layout
S(S_BOOKS_PAGE_ABOUT, "Page %d of about %d")  // the menu, over the progress bar
S(S_BOOKS_LEFT_CHAPTER, "%ld page left in the chapter|%ld pages left in the chapter")  // the reader's foot
S(S_BOOKS_LAST_IN_CHAPTER, "Last page of the chapter")
S(S_BOOKS_THE_END, "The end")  // the last page of a book
S(S_BOOKS_SIZE, "Text size")  // the Text panel
S(S_BOOKS_JUSTIFY, "Even right edge")  // a switch: justified text
S(S_BOOKS_SPACING, "Line spacing")  // the Text panel: a label left of three small buttons (~200 px)
S(S_BOOKS_MARGINS, "Margins")
S(S_BOOKS_NO_CONTENTS, "This book has no table of contents.")
S(S_BOOKS_NO_MARKS, "No bookmarks yet. Tap the top right corner of a page to add one.")
S(S_BOOKS_MARK_ADDED, "Bookmark added")
S(S_BOOKS_MARKS_FULL, "There are already 32 bookmarks in this book.")
S(S_BOOKS_DONE, "Done")  // closes a panel
S(S_BOOKS_HINT, "Tap the right side for the next page, the left for the one before, the middle for the menu.")  // shown once, on the first page read

// The book page (a tap on a book in the library)
S(S_BOOKS_START, "Start reading")  // the book page's big button, ~400 px
S(S_BOOKS_CONTINUE_BTN, "Continue reading")
S(S_BOOKS_READ_AGAIN, "Read again")
S(S_BOOKS_ABOUT, "About this book")  // a heading
S(S_BOOKS_CHAPTERS, "Chapters")  // a heading
S(S_BOOKS_CHAPTERS_LATER, "The chapters show here once the book has been opened.")
S(S_BOOKS_CHAPTER_OF, "Chapter %d of %d")  // under the progress bar
S(S_BOOKS_FINISHED_ON, "Finished %s")  // "Finished 3 November"
S(S_BOOKS_STARTED_ON, "Started %s")
S(S_BOOKS_TIME_READ, "Time read")  // a label over a number, ~200 px, small
S(S_BOOKS_PAGES_READ, "Pages read")
S(S_BOOKS_SESSIONS, "Sessions")
S(S_BOOKS_LEFT, "Left at your pace")
S(S_BOOKS_LEFT_UNKNOWN, "Read a little more")  // where the time left would be, before the pace is known
S(S_BOOKS_HOURS_MIN, "%d h %d min")  // a duration
S(S_BOOKS_MIN, "%d min")
S(S_BOOKS_LESS_MIN, "under a minute")
S(S_BOOKS_SIZE_KB, "%s  ·  %d KB")  // the format and the file's size: "EPUB  ·  352 KB"
S(S_BOOKS_SIZE_MB, "%s  ·  %d.%d MB")
S(S_BOOKS_MIN_LEFT_CHAPTER, "%s left in the chapter")  // the reader's foot: "12 min left in the chapter"
S(S_BOOKS_MORE_BELOW, "Swipe up for more")

// Statistics (from the library's top line)
S(S_BOOKS_STATS, "Reading statistics")  // a title
S(S_BOOKS_STATS_READ, "%s read")  // the library's top line: "12 h 30 min read", then S_BOOKS_DAYS_ROW
S(S_BOOKS_DAYS_ROW, "%ld day in a row|%ld days in a row")
S(S_BOOKS_STATS_NONE, "Your reading statistics show here")  // the library's top line before any reading
S(S_BOOKS_TODAY, "Today")  // labels over big numbers, ~200 px
S(S_BOOKS_THIS_WEEK, "This week")
S(S_BOOKS_STREAK, "Days in a row")
S(S_BOOKS_LONGEST, "Longest: %d")  // under the streak
S(S_BOOKS_BOOKS_FINISHED, "Books finished")
S(S_BOOKS_TOTAL_TIME, "Time reading")
S(S_BOOKS_TOTAL_PAGES, "Pages turned")
S(S_BOOKS_LAST_7, "The last 7 days")  // over a bar chart
