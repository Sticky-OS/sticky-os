# Contributing

Thanks for wanting to help. Bug reports, ideas and pull requests are all welcome.
Everyone taking part follows the [code of conduct](CODE_OF_CONDUCT.md).

## Reporting a bug or an idea

Open an issue: <https://github.com/Sticky-OS/sticky-os/issues/new/choose>. Issues
go through the forms (there are no blank issues), and a bug report needs:

- **Steps to reproduce**, numbered, from a known start ("after a restart", "in
  Calendar's week view"), with the taps, swipes and settings that matter. If it
  needs particular data (a book, a calendar link), say what kind or attach a
  small file that shows it. We fix what we can make happen on our own board.
- **What happened and what you expected**, separately.
- **How often**: every time, sometimes, or once.
- **The firmware version** (Settings → About this device → Firmware), and a
  search of the open and closed issues first.
- If you can: the **serial log** around the problem (115200 baud) and a
  **screenshot** (`firmware/tools/board.py shot out.png`, after `demo on`).

Questions go to [Discussions](https://github.com/Sticky-OS/sticky-os/discussions),
and security problems to private reporting (see [SECURITY.md](SECURITY.md)),
not to issues.

Please never paste WiFi passwords, API keys or tokens. The board and the setup
app never print them, so you shouldn't need to.

## Building and trying a change

See [Build and flash](docs/development.md#build-and-flash) (the tools and versions). In short:

    ./upload.sh build    # build only
    ./upload.sh          # build and flash over USB

The serial port (115200 baud) takes test commands, handy for trying screens
without touching the board:

| Command | Does |
|---|---|
| `app <n>` | open app n (0 Clock, 1 Weather, 2 Calendar, 3 Notes, 4 Timer, 5 Photos, 6 Claude, 7 Settings, 8 About, 9 Quote, 10 Recipes, 11 Games, 12 Books). The numbers stay put (Settings keeps hidden apps as a bit each); `APP_ORDER` sets the home screen's order |
| `home`, `quick`, `ok`, `move <n>` | home screen, quick settings, the OK button, UP / DOWN |
| `tap <x> <y>`, `dtap <x> <y>`, `swipe <x0> <y0> <dx> <dy>` | touch, in the picture's own coordinates; `dtap` a double tap (Photos and Quote: full screen on or off). In Photos and Quote a single tap acts 0.35 s later, once no second one came |
| `full` | which apps are full screen: `full: photos on/off, quote on/off, on screen yes/no` |
| `turn <0-4>` | rotation: 0 automatic, 1 landscape, 2 landscape flipped, 3 portrait, 4 portrait flipped |
| `sleep`, `sleep preview` | deep sleep now, as Quick settings → Sleep now (the screen stays as it was, with a moon in the status bar); or draw that screen without sleeping, for `shot` |
| `demo on` / `demo off` | made-up events, notes, town and network, for screenshots |
| `shot` | the screen as drawn, as hex: `SHOT 800 480 <rotation>`, 480 rows, `END` |
| `shot4` | the four-grey frame last sent to the panel (a photo with its status bar): `SHOT4 800 480 <rotation> <on screen 0/1>`, 480 rows of 2 bits a pixel (0 black), `END` |
| `about` | everything About shows: the website, source and videos, then the device |
| `gauge` | the battery gauge's registers as read (`gauge: 0x0C Current -1234 (0xFB2E)` ...), then `gauge: design capacity <n>, scale 10`. The board's 100 mOhm sense resistor makes the gauge count current, capacity and power in tenths, so the board always divides them by 10 where it shows or sends them; DesignCapacity (7500 on Seeed's 750 mAh cell) is shown as a check: another value means the gauge lost its setup |
| `update check`, `update install` | Settings → Software update: Check now, and Install |
| `fsget <path>` | a LittleFS file as `FSGET <path> <bytes> <base64>` (up to 3 KB) |
| `fsput <path> <base64>` | writes that file whole (`FSPUT <path> <bytes>`); restart afterwards |
| `voice <seconds>` | a voice note of that length, without the button (the Listening card goes up first, then the seconds count); prints `voice: <the answer>` when it's back |
| `note open <n>` | Notes, with the item on row n open full screen (0 the first, in the list's order: open tasks, notes, finished tasks) |
| `note type <words>` | Notes: the words sent as a typed note, as the keyboard's Done does: the AI answers and the list changes (in demo mode nothing is saved). Handy for trying the AI in another language |
| `note ai <lines>` | applies `<lines>` to the Notes list as if the AI had answered them, `\|` between lines, e.g. `note ai TASK: Buy eggs\|TASK: Buy bread\|DONE: 1\|SAY: Added eggs and bread.` (numbers count from 1 in the order the items were added, as the AI sees the list, not the order shown). The new items get their *NEW* pill; in demo mode nothing is saved. Prints `note: <the SAY line>` |
| `fetch <n>` | fetch now for app n (1 Weather, 2 Calendar, 6 Claude), as OK in it does |
| `cal` | the Calendar's sources: InfluxDB and each link (its label and host, never the address), how many events, the last read (status, bytes, time, ETag) and why it failed |
| `wifi scan` | Settings → WiFi → Add network (the network task scans) |
| `hours <n>` | Weather: the hours of day n of the forecast (0 the first) |
| `quote new` | the Quote app's *New quote* |
| `recipe test` | the recipe parser against good and broken replies (a fenced reply, words around it, odd shapes, trailing commas, cut short, not JSON, a refusal, too many ingredients): `recipe test <name> PASS/FAIL` lines, then `recipe test: <n> failed` |
| `recipe sample`, `recipe sample save` | a sample recipe on screen (and saved), to check the pages without the AI |
| `recipe ask <words>`, `recipe voice <seconds>` | a recipe request, typed or spoken, as the app sends it |
| `game`, `game <n>` | Games: the launcher, or game n (0 2048, 1 Memory, 2 Maths Quest, 3 Daily (its screen: Daily maths and Sudoku), 4 Quiz, 5 Adventure, 6 X and O, 7 Tutor, 8 Solitaire, 9 Mines, 10 Four in a Row, 11 Chess), where it was left |
| `game peek` | the game on screen as one line of text: 2048's board, Memory's cards (`*` matched), the maths or quiz question with its answer, the story's scene, the X and O board, Tutor's screen and the language of its content (`tutor7 question lang en ...`) and its exercise with its options (`*` the right one); Solitaire's draw, moves, state, piles (`10H`, `KS`; `##` face down) and the move a hint would make (`hint <id> <id>`); Mines' size, Dig / Flag, state, counts, a safe square and a mine (`next`, `mine`) and the cells (`.` covered, `x` a covered mine, `F` / `f` a flag on a mine / a safe square); Four in a Row's rows (top first, `X` you, `O` the board), whose turn or the result, the level and the last column; Chess's position (FEN), its state, both sides' last moves (with the board's nodes and milliseconds), the level, the picked square and whether the panel is open; the Daily screen's state (`daily maths <done> streak <n>, sudoku easy <done> medium <done> hard <done> streak <n>`); Sudoku's `sudoku <level> <date> grid <81: digit or .> given <81: g or -> sel <cell> next <cell>=<digit> clashes <n> done <0\|1>` (then ` card` while the levels card is up). The top of each `games_*.ino` has the exact format. Then the ids of what can be tapped |
| `game right`, `game wrong` | answer the question on screen right or wrong (Maths, Quiz); Memory: turn over a pair, or two that don't match; Sudoku: the `next` square gets its number, or one that's wrong there |
| `daily date <YYYYMMDD>`, `daily date off` | test builds only (a `dev-*` version): today's date for the daily puzzles (Daily maths, Sudoku), to check that two boards agree and that a streak rolls over at midnight; `off` goes back to the clock |
| `game type <words>` | Quiz or Adventure on screen: the words as if typed for *Your own theme* / *Your own idea* |
| `game slide <0-3>`, `game pick <id>` | 2048: slide left, right, up, down; press what has that id (a card, an answer, a choice, a level, a square; 900 Back, 901 New game, 902 Again, 903 Games, 904 Next, 905 Cancel, 906 the built-in quiz or story, 907 Practice, 908 Play, 909 Try again; Tutor: 25 the class, 30 the subject, 40 to 48 the lessons, 20 / 21 / 22 a lesson's summary, examples and exercises, 0 to 3 an exercise's answers, 10 its clues; Solitaire: pile x 30 + a card's place in it from the bottom (piles 0 the stock, 1 the waste, 2 to 5 the foundations, 6 to 12 the columns), pile x 30 + 29 the pile itself (29 turns the stock), 400 Undo, 410 / 411 deal draw 1 / draw 3; Mines: 0 up a square (row x columns + column, held upright), 200 Dig, 201 Flag, 210 to 212 a new small, medium or large board; Four in a Row: 0 to 6 a column, 10 to 12 the level; Chess: 100 + a square (a1 = 0 to h8 = 63, so e2 is 112), 30 to 33 promote to a queen, rook, bishop or knight, 10 to 12 the level, 20 Undo, 21 Resign, 40 Menu, and 1000 + from x 64 + to plays that move at once (e2e4 is 1796); the Daily screen: 10 Daily maths, 11 Sudoku; Sudoku: 0 to 80 a square (row x 9 + column), 101 to 109 the numbers, 110 Erase, 111 Pencil, 112 Undo, 120 to 122 a level on the levels card, 130 Levels; `game peek` lists the ids on screen) |
| `lang <code>` | Settings → Language: `en`, `es`, `pt`, `fr`, `de`, `it`, `ru`, `tr`, `pl`, `ro` (kept, as the setting) |
| `settings language` | Settings, on its Language page |
| `keyboard`, `keyboard accents` | Notes' keyboard; `accents`: on its page of accented letters (in Russian: the Latin letters) |
| `listen show`, `listen think`, `listen off` | the voice card on its own over the current screen, for screenshots: *Listening...* (the microphone), *Thinking...* (the speech bubble), or gone. Recording shows the first by itself; the board reads no serial while it records, so a `shot` waits for the end |
| `airelay <host> <port>`, `airelay off` | until the next restart, the AI's requests (notes, recipes and the games) go to `https://host:port` without checking its certificate and without the OpenAI key: a fake server on a computer can answer (`/v1/audio/transcriptions` with `{"text":...}`, `/v1/responses` with an `output_text`; the request's `text.format.name` says what's wanted: `recipe`, `quiz` or `adventure`). Never saved |
| `book`, `book list`, `book open <n>` | Books: the app (the book left open, else the library); the books on the card, one `BOOK <n> <status> <percent> <format> "<title>" "<author>" <path>` line each, then `END`; open book n of that list (the first time, it waits while the book is got ready and says how long it took); `book start <n>` opens it as a tap does, the loop going on (the progress screen shows) |
| `book next [n]`, `book prev [n]`, `book goto <permille>`, `book toc [i]` | Books, with a book open: turn pages, jump to a thousandth of the book, list the contents (`TOC <i> <level> <offset> <title>`) or jump to entry i |
| `book peek`, `book bench <n>` | the page on screen as text (`PAGE <start>-<end> of <size>`, the chapter, then a `\| ` line for each line of type); n page turns laid out and not drawn, with the time a page takes |
| `book menu`, `book text`, `book contents`, `book marks`, `book page`, `book cover`, `book library` | Books' screens, for screenshots: the menu, the Text panel, the contents, the bookmarks, back to the page, the cover, the library |
| `book size <0-6>`, `book spacing <0-2>`, `book margins <0-2>`, `book justify on/off`, `book mark` | the text size (0 the smallest), line spacing and margins (0 tight / narrow, 2 loose / wide), justified or ragged, a bookmark on the page shown (or off it) |
| `book details <n>`, `book details more`, `book stats` | book n's page (cover, description, progress, statistics, chapters), its next screen; the reading statistics (also printed: `book: stats ...`) |
| `sdput <path> <offset> <base64>` | writes bytes into a file on the microSD card (offset 0 makes it afresh, else it must be the file's size so far): `SDPUT <path> <size>` or `SDPUT <path> failed`. Paths are from the card's top, with `%XX` for a space or any other byte. `firmware/tools/board.py put book.epub books/book.epub` sends a whole file this way |
| `sdls [folder]`, `sdrm <path>`, `sdmkdir <path>` | the card's files: `SDLS <name> <bytes>` lines (folders end in `/`) then `END`; remove a file (or an empty folder); make a folder |
| `stats` | one line: uptime, reset reason, the longest the loop was held up since the last `stats`, the free stack of the loop, network and notes tasks, internal heap (free, lowest, largest block), PSRAM, TLS sessions open at once and the smallest largest-block seen as one opened |

Please include a screenshot in pull requests that change a screen; `demo on`
keeps your own data out of it.

## Pull requests

Small, focused pull requests are reviewed fastest. For anything bigger than a
fix (a new app, a new setting, a change to how something works), open an issue
or a discussion first, so we can agree on the shape before you build it.

Changes reach `main` only through pull requests, and a pull request can be
merged only when every check passes: the firmware build and test suites, the
site, the secret scan and the description check. The description check makes
sure the template's **Tests** and **How I tested it** sections aren't empty.
What each section needs:

**Tests.** Run the suites and say which you ran:

| Suite | Covers | Run |
|---|---|---|
| Translations | every translation: ids, placeholders, plurals, letters the fonts can draw | `firmware/test/i18n/run.sh` |
| Calendar | the iCalendar parser and repeating events | `firmware/test/ics/run.sh` |
| Books | the readers (EPUB, MOBI, AZW3, PDF), pages, library, fonts | `firmware/test/ebook/run.sh`, `firmware/test/ebook/fonts.sh` |
| Games | the rules of Solitaire, Mines, Four in a Row and Chess (CrossPlay's own tests), and Sticky OS's code around them: Solitaire's taps, Mines' sizes, the board's play at each level; the daily Sudoku: a year of puzzles at each level (one solution, clues in range, the level's techniques), fixed dates' puzzles, timing | `firmware/test/games/run.sh` |
| Tutor | every exercise and lesson, in every language | `firmware/test/tutor7/run.sh` |
| Setup app and site | the setup app against a simulated board, the pages | `node test/run.mjs` in `site/` |

A change to logic that runs on the host (parsers, rules, maths, text) comes
with a test in the matching suite that fails without the change. Drawing and
touch can't be tested on the host: say so, and let the screenshots and steps
below carry it. A new test command for the serial port (see the table above)
counts as a way to test, too.

**How I tested it.** The steps you took on a real board, and what you saw:

- **Screenshots** of every screen the change touches, **before and after**, in
  **portrait and landscape** if it draws differently. Take them with
  `firmware/tools/board.py shot out.png` (or `run ... shot:a.png ...` for a
  sequence) after `demo on`, so no names, places or networks are in them.
  For the four-grey screens (photos, a book's cover) use `shot4`.
- For anything that talks to a service (weather, calendar, the AI, updates),
  the serial log of one run, with the keys and tokens left out.
- For the setup app or the site, a screenshot from the browser, at a phone
  width and a desktop width.

**Size and memory.** The board's internal RAM is scarce. Give the "Global
variables use N bytes" line and the sketch size from `./upload.sh build`,
before and after. Big buffers and task stacks go in PSRAM, images go in flash
(`const`), and the loop's stack is about 8 KB, so no big local arrays.

**Also:**

- New text goes through `S()` with all ten languages (see [Languages](#languages)).
- `src/sticky_drivers/` holds Seeed's screen and touch drivers. Keep them as
  close to Seeed's as you can, but changing them is fine when the change works
  on a board: say in the pull request how you tested it (the timings from the
  serial log, screenshots), and add a line to `src/sticky_drivers/README.md`
  saying what changed and where it came from.
- If something a user sees or does changes, update [docs/guide.md](docs/guide.md)
  and the README in the same pull request.
- Never commit private values: WiFi names and passwords, keys, tokens, your
  town or calendar links. CI scans every pull request for secrets.
- Commit messages say what changed and why, in a sentence a user could read.
- CI must be green: the firmware build, the test suites, the site and the
  secret scan.

## How the code is laid out

- `firmware/sticky-os/sticky-os.ino`: the apps, the home screen, input and drawing.
- `net.*` WiFi, weather (and a day's hours) and MQTT; `notes.*` notes and
  voice, and the network part of a recipe; `recipes.*` the recipe JSON (the
  schema the AI answers in, the defensive parser, the saved ones);
  `quotes.*` ZenQuotes, the built-in quotes and today's cache; `games.h` /
  `games.cpp` what the games know and keep (2048's rules, the maths
  questions, the built-in quiz and story, X and O's moves, the AI's
  quiz and scene JSON, the progress file) and `games.ino` how they look and
  take taps (a second tab of the sketch, so it shares the screen helpers);
  Solitaire, Mines, Four in a Row and Chess each have their own tab
  (`games_solitaire.ino`, `games_mines.ino`, `games_four.ino`,
  `games_chess.ino`; the top of each lists its ids and its `game peek`
  line), over CrossPlay's rules in `src/crossplay/` and Sticky OS's plain
  C++ around them, tested on a computer (`firmware/test/games/run.sh`):
  `solui.h` what a Solitaire tap does, `minesgrid.h` Mines' three sizes,
  `boardgames.h` how hard the board plays Four in a Row and Chess (the
  one place to tune the levels); the Daily tile's screen and its Sudoku
  are `games_sudoku.ino`, over `sudokugrid.h` (the puzzle of the day from
  the date, its solver, grader and clash check, plain C++ tested the same
  way; a change to the generator changes its salt's version digit);
  `json.*` JSON by key; `glyphs.*` UTF-8 and the letters beyond ASCII
  (`textFold()` for text from outside, `foldCodePoint()` for what the fonts
  can't draw); `i18n.*` and `src/i18n/` the languages (see
  [Languages](#languages)); `calendar.*`
  the Calendar's sources merged (the links read and streamed into the
  parser, ETags, errors); `ics.*` the iCalendar parser and the expansion of
  repeating events and time zones (plain C++, tested on a computer:
  `firmware/test/ics/run.sh`, optionally with real `.ics` files as
  arguments); `influx.*` the InfluxDB events; `psalloc.h` a PSRAM allocator
  for vectors; `photos.*` the microSD photos; `photostore.*` the photos
  sent from the setup app (their file format, and the `photo_*` ops);
  `config.*` and `prov*` the settings and the setup protocol; `card.*` the microSD
  card's mount, shared by Photos and Books.
- The Books app: `books.ino` draws it and takes its taps (a third tab of the
  sketch, like `games.ino`). Under it, in plain C++ tested on a computer
  (`firmware/test/ebook/run.sh`; with real books, `BOOKS_DIR=~/books
  firmware/test/ebook/run.sh`): `booklib.*` the library (the books on the card,
  and the converted copies and reading places it keeps in `.sticky/books/` on
  the card); `ebook.*` and `bookformats.h` turning a book into the app's own
  text format (`bookflow.h`) the first time it's opened, with a reader for each
  kind of book (`bookepub.cpp`, `bookmobi.cpp`, `bookfb2.cpp`, `bookpdf.cpp`,
  `booktext.cpp`, and `bookhtml.cpp`, `bookzip.*`, `bookinflate.*`,
  `bookcharset.*` that they share); `bookpage.*` the pages (line breaking,
  justification, and the page grid that keeps back and forth exact);
  `bookfonts.*` the reading typeface (FreeSerif, four styles, seven sizes, made
  by `firmware/tools/fontext/make.sh`). `firmware/test/ebook/tools/` has
  `book2flow` (convert a book, print its contents and text) and `pagedump`
  (its pages as text). `firmware/tools/board.py` drives a board from the
  computer: dev commands, screenshots (`shot`) and files onto its card.
- Tutor: `tutor7.ino` draws it and takes its taps (a tab, like `games.ino`). Under it, in plain C++
  tested on a computer (`firmware/test/tutor7/run.sh`; `run.sh samples` writes a few exercises of each
  kind per language to read): `tutor7_core.*` and `tutor7_u4.cpp` the exercise generators and their
  checks, `tutor7_lessons.*` each lesson's plan, `tutor7_text.*` and `src/tutor/` the words (see
  [Tutor's languages](#tutors-languages)).
- `src/sticky_drivers/`: Seeed's screen and touch drivers (MIT), kept close to Seeed's; what we changed is listed in their README.
- `src/brand/brand.h`: the Sticky OS logo as 1-bit bitmaps (the boot screen, About), made
  from the brand's SVGs in `site/src/brand/assets/svg/` by `firmware/tools/gen_brand.py`
  (Pillow and rsvg-convert); run it again when the brand changes.
- `src/crossplay/`: the rules of Solitaire, Mines, Four in a Row and Chess from
  [CrossPlay](https://github.com/ma-r-s/crossplay) by Mario Ruiz (MIT), and the
  "celtic" chess pieces by Maurizio Monge (MIT), unchanged; its README says
  from which commit and how to take a newer one.
- `site/`: the website and the browser setup app (`site/src/setup/`), with
  tests in `site/test/` (`node test/run.mjs`). The app's photo code (crop,
  scale, dither, pack, upload) is plain functions at the top of `app.js`, so
  the tests, and a script in Node, can run exactly what the page runs;
  `mock.js` is a second, independent board (photo store included).

A few habits that keep the board happy:

- Internal RAM is scarce: put big buffers in PSRAM, and mention the
  "Global variables use N bytes" change of your build in the pull request.
- Screens are laid out from `W` and `H`, which swap in portrait; check both.
- E-paper redraws take about 0.6 s: change the screen once per action, not
  per step.
- Nothing private in the source: settings live on the board and are set from
  the setup app. Don't attach a binary you built with `secrets.h` or
  `private_seed.h` present: it holds their values.
- The network may never be there: every network call needs a bounded wait
  (TLS: `netTrust()` sets the 8 s handshake), a plain-words failure for the
  screen (`netFailText()`), and no network work on the loop task (the notes
  task and the network task do it). Flash writes (NVS, LittleFS, OTA)
  happen on the loop or the network task, whose stacks are in internal RAM,
  never on a task whose stack is in PSRAM (the notes task): the loop saves
  what the notes task brings back (a note, a recipe, today's quote, a
  game's progress). The loop asks with a flag (`hourlyAsk`, `quoteAskToday`...), the
  network task answers under `dataLock` and raises `fetchDone`.
- Respect the services: ZenQuotes allows 5 requests in 30 s, so the board
  asks at most every 7 s and falls back to its own list when told to wait.

## Languages

Every word on the screens is in `firmware/sticky-os/src/i18n/`:

- `strings.h` (and `strings_games.h`, `strings_net.h`, `strings_books.h`, included at its end):
  one row a string, `S(S_SOME_ID, "English")`, with a comment saying where
  it shows and how much room it has. The code shows it with `T(S_SOME_ID)`,
  a format with `TF(S_SOME_ID, ...)`, a number and a noun with
  `TP(S_SOME_ID, n)` (plural forms, `"%ld photo|%ld photos"`), a date with
  `tStrftime()` / `tDate()` (never plain `strftime` with `%A %a %B %b`: those
  are the board's own day and month names).
- `es.h`, `pt.h`, ... one per language: `{S_SOME_ID, "translation"},` in
  any order. An id a language hasn't got shows in English.

**A new string:** add its row to `strings.h` (or the games / network / books file)
in English, use it with `T()`, and add the translations you can; leave the
others, they fall back to English. Keep the placeholders (`%s`, `%d`...) in
the same order in every language; a list (`S_DAYS`) keeps its number of
items; a plural string has the forms its language needs (`one|other`, or
`one|few|many` in Russian, Polish and Romanian), each with `%ld`.

**A new language:** add it to `Lang` in `i18n.h`, `LANGS` in `i18n_tables.cpp`
(its code, its own name, its English name for the AI, the decimal comma and
the plural rule), `LANGUAGE_CODES` in `config.h`, the keyboard's
`KB_ACCENTS` in `sticky-os.ino`, `LANGUAGES` in `site/src/setup/app.js` and
the mock's options, then copy `ro.h` to `<code>.h` and translate. Its letters
must be ones the fonts have (see below).

**Check:** `firmware/test/i18n/run.sh` checks every language file (unknown
or doubled ids, placeholders, lists, plural forms, characters the fonts
lack); `--widths` lists translations much wider than the English, to look
at on the board (`lang <code>` on the serial port, then the screen).

**Fonts:** the screens use Adafruit GFX's FreeSans, FreeSansBold and
FreeSerifItalic, which hold ASCII only. `src/fonts/*ptExt.h` add 340 more
characters to each size (Latin-1, Latin Extended-A, ș ț, Cyrillic, some
punctuation), looked up by `glyphs.cpp`; the canvas in `sticky-os.ino`
prints UTF-8 with both. They are generated from GNU FreeFont by
`firmware/tools/fontext/make.sh` (FreeType, the same rendering as
Adafruit's `fontconvert`); to add characters, add their code points there
and run it. Scripts that need shaping or thousands of glyphs (Chinese,
Japanese, Arabic, Hindi...) need a different text engine: see the pull
request that added the languages.

The Books app reads in GNU FreeSerif (regular, italic, bold, bold italic at
seven sizes, and larger bolds for chapter titles: `src/fonts/BookSerif*.h`).
Each comes whole from the same script, `make.sh`: ASCII, the extra letters and
the line's measures in one header; `bookfonts.cpp` lists them and is the only
file that includes them.

### Tutor's languages

Tutor's lessons and exercises are in `firmware/sticky-os/src/tutor/`, apart from the board's strings:
they are long, and a language either has all of them or none is shown.

- `t7_strings.h`: the exercises' words, `T7S(TX_ID, "English")`, with a comment saying where each
  shows and what its placeholders get, in order. Most are printf formats filled with the exercise's
  numbers; `ro.h` (one file a language) has `{TX_ID, "translation"},` pairs.
- `lessons_en.h`, `lessons_ro.h`: one `LessonText` a lesson, in slot order: its name, its subject, and
  its summary and example pages (title, body). The pages' number and figures are the lesson's plan
  (`tutor7_lessons.cpp`), the same in every language.
- Tutor shows its content in the board's language when that language has every string and every lesson
  (`t7::contentLang()`), else in English. The buttons and headers around it are ordinary board strings
  (`S_G_T7_*` in `strings_games.h`) and fall back string by string, like the rest of the board.
- An exercise is made from a seed, and nothing but its words depends on the language: the same seed
  makes the same numbers, options and right answer in every language. Changing the language with an
  exercise open makes it again from its seed in the new one.

**Adding a language to Tutor:** copy `ro.h` to `<code>.h` and `lessons_ro.h` to `lessons_<code>.h`
and translate both, all of it; add the two tables to `makeContent()` in `tutor7_text.cpp`; add the
language's column to the worked examples' checks in `firmware/test/tutor7/tutor7_test.cpp`
(`NEEDLES`); translate the `S_G_T7_*` strings in `src/i18n/<code>.h`. Then
`firmware/test/tutor7/run.sh` makes every exercise in every language (it fails if a translation doesn't
fit its buffer, or makes a different exercise than English), and `firmware/test/i18n/run.sh` checks
the placeholders and letters. Rules:

- Keep every placeholder, in the same order, and every number of the worked examples.
- Hints and the reasons of wrong answers add no numbers of their own (say "half", "twice"): the
  generator refuses an exercise whose hint shows the answer.
- Notation: the language's own (Romanian: `·` and `:`, a decimal comma; English: `×` and `÷`, a
  decimal point). Never `/` for a division (a fraction like `2/3` is fine): the checker reads `/` as part
  of a number. `√` followed by digits or a parenthesised expression; the fonts have no angle, parallel or
  perpendicular signs ("angle A", "AB and CD are parallel").
- Bodies of lesson pages stay under 620 bytes; look at them on the board in both orientations
  (`lang <code>`, `turn 3`, `game 7`, `game pick ...`) and watch the serial log for "does not fit".
- The unit numbers (4.1 to 4.8, 7.2) are the Romanian textbook's, in every language.

The English uses US school terms, the same ones throughout: leg, hypotenuse, the Pythagorean theorem
and its converse, convex quadrilateral, opposite and adjacent sides, consecutive angles, supplementary,
congruent, the diagonals bisect each other, midsegment (of a triangle and of a trapezoid), the medial
triangle, median, centroid, a trapezoid's bases (longer, shorter) and non-parallel sides (never "legs",
which belong to the right triangle), right and isosceles trapezoid, perpendicular bisector, angle
bisector, line of symmetry, height; Grade 7, Unit 4 review, Hints, Solution, right first time. A
square's side is s, a rectangle's length and width l and w.

## Licence

By contributing you agree that your work is released under the [MIT licence](LICENSE).
