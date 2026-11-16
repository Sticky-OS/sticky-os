// Books: the library. Which books are on the card, what the board already
// knows about each (so the list is drawn after reading one file), the
// converted copies kept on the card, and where the reader was in each book.
//
// Plain C++ over stdio, dirent and stat (no Arduino): it runs on the board,
// where the card is mounted in the file system at "/sd", and in the tests on
// a computer, on a folder (firmware/test/ebook/lib_test.cpp).
//
// Where the books are: <root>/books (any case; "ebooks" too) and its
// sub-folders two levels down (books/Author/Title/book.epub, as Calibre lays
// them out); with no such folder, the files at the card's top. Names starting
// with a dot, "System Volume Information" and "__MACOSX" are left out. At
// most LIB_MAX_BOOKS are listed (truncated() says there were more).
//
// What the library keeps, all of it in <root>/.sticky/books/ ("key" is 16 hex
// digits made of the book's path, size and date: a replaced file is a new book):
//
//   library.idx     the list: one line a book (title, author, how far, ...)
//   <key>.flow      the converted book (bookflow.h), <key>.flow.toc its contents
//   <key>.cover     the cover picture as it was in the book (JPEG or PNG)
//   <key>.meta      written once the conversion is whole: title, author, sizes
//   <key>.pos       the place in the book, the bookmarks, finished or not, time spent reading it
//   stats.txt       the reading statistics of the card: totals and a day by day log
//
// The small files end with a checksum line and are written beside themselves
// first (name.tmp), so a card pulled or a battery gone flat in the middle of a
// write leaves either the old file or the new one; a torn file reads as "not there".
//
// The list:
//
//   BookLibrary lib("/sd");
//   lib.scanBegin();                      // lists the folder: fast
//   draw the list (count(), book(i, b));  // names stand in for titles not yet read
//   while (lib.scanStep()) ...            // a book a call; redraw when generation() changed
//
// A book (i is a place in the list; key is what to remember across a restart):
//
//   if (!lib.openBegin(i, now)) say(lib.openError());
//   int p;
//   while ((p = lib.openStep()) >= 0 && p < BOOK_DONE) showProgress(p);   // converting, the first time
//   if (p < 0) say(lib.openError());
//   ... lib.flowPath(), lib.position() ... lib.setPosition(offset, now); lib.saveState(); ...
//   lib.closeBook();
//
// "now" is the time in seconds since 1970, or 0 when the board doesn't know
// it: "last read" then counts up from the newest one the card has seen, so
// the order of the list stays right without a clock.
//
// Reading statistics: while a book is open the app tells the library what
// happened since it last did (lib.addReading(seconds, pages, bytes, now, day)
// every few pages and on leaving the book). The book's own figures are kept
// in its .pos, the card's in stats.txt: libraryStats() for the statistics
// screen, bookInfo() for a book's page. "day" is the local day number,
// bookLibLocalDay(now, the time zone's offset), -1 when the clock isn't set.
#pragma once
#include <dirent.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <string>

#include "ebook.h"
#include "psalloc.h"

static const int LIB_MAX_BOOKS = 500;
static const int LIB_MAX_MARKS = 32;
static const int LIB_LABEL_MAX = 80;  // a bookmark's label, bytes of UTF-8
static const int LIB_STATS_DAYS = 400;  // the day by day log keeps this many days
static const uint32_t LIB_SESSION_GAP = 30 * 60;  // a pause longer than this: a new session

// What scanBegin() found. Anything but LS_OK: there is no list, and this is why.
enum LibState : uint8_t {
  LS_IDLE,       // not looked yet
  LS_OK,         // there are books
  LS_NO_CARD,    // the card isn't there or can't be read (also: it went away since)
  LS_NO_FOLDER,  // a card with no "books" folder and no books at its top
  LS_EMPTY,      // a "books" folder with no books in it
};

// The list's three parts, in the order they're shown.
enum LibStatus : uint8_t {
  LB_READING,   // opened and not finished: the last read first
  LB_NEW,       // never opened: by title
  LB_FINISHED,  // read to the end: the last finished first
};

// A line of the list. The strings are the library's: use them before the next
// call that changes it (scanStep, openStep, deleteBook, ...).
struct LibBook {
  const char *title;   // from the book, or made of the file's name until it's been read (never empty)
  const char *author;  // "" when the book doesn't say
  const char *path;    // from the card's top: "books/Carroll/alice.epub"
  char key[17];        // 16 hex digits and a 0: the book's name across restarts (find())
  BookFormat format;
  LibStatus status;
  uint16_t permille;   // how far, 0..1000
  uint32_t lastRead;   // 0: never
  bool probed;         // the title is the book's own (false: the scan hasn't got to it)
  bool error;          // the book couldn't be read; opening it says why
  uint32_t secsRead;   // time spent reading it, seconds
};

// What is known of the reading of one book.
struct LibBookStats {
  uint32_t secs;        // time spent reading it
  uint32_t pages;       // pages turned forward
  uint32_t sessions;    // times it was taken up (a pause of over LIB_SESSION_GAP: a new one)
  uint32_t started;     // when it was first read, seconds since 1970; 0 unknown
  uint32_t finishedAt;  // when the last page was reached; 0 not finished or unknown
  uint32_t bytes;       // flow bytes read forward
  uint32_t lastAt;      // when it was last read; 0 unknown
  // The reader's pace in flow bytes a minute, 0 until there are a few minutes
  // and pages to go by: time left = (flowSize - position) / pace.
  uint32_t paceBytesPerMin() const;
};

// The statistics screen.
struct LibStats {
  uint32_t booksFinished, booksReading;
  uint32_t totalSecs, totalPages;   // since the card has been read on (books removed since included)
  uint32_t secsToday, secsThisWeek;  // this week: the last 7 days, today included
  uint32_t currentStreakDays;        // days in a row with a minute or more read, up to today (or yesterday)
  uint32_t longestStreakDays, daysRead;
  uint32_t last7[7];                 // seconds read: [6] today, [5] yesterday ... [0] six days ago
};

// A book's page, without opening it.
struct LibInfo {
  bool converted = false;                 // opens at once: the files below are there
  std::string flowPath, coverPath;        // when converted ("" otherwise, and coverPath when no cover)
  char coverType = 0;                     // 'j' JPEG, 'p' PNG, 0 none
  std::string title, author, description, path;  // description: paragraphs split by '\n', "" when the book has none
  char key[17] = {0};
  BookFormat format = BF_UNKNOWN;
  LibStatus status = LB_NEW;
  bool error = false;
  uint32_t fileSize = 0;                  // the book's file, bytes
  uint32_t position = 0, flowSize = 0;    // flowSize: 0 until converted once
  uint16_t permille = 0;
  bool finished = false;
  int marks = 0;
  LibBookStats stats = {};
};

struct LibMark {
  uint32_t offset;  // in the flow
  char label[LIB_LABEL_MAX + 1];
};

// The seam the tests use to convert with something else than BookConverter.
class LibConverter {
 public:
  virtual ~LibConverter() {}
  virtual bool begin(const char *bookPath, const char *flowPath, const char *coverPath) = 0;
  virtual int step() = 0;  // as BookConverter::step()
  virtual void abort() = 0;
  virtual BookErr error() const = 0;
  virtual const BookMeta &meta() const = 0;
};
typedef LibConverter *(*LibConverterFactory)(void *arg);
typedef bool (*LibProbe)(const char *path, BookMeta &meta, BookErr &err);

// The library's own records (not for the app).
struct LibEntry {
  uint64_t key;
  uint32_t path, title, author;  // places in the string pool
  uint32_t size, mtime, stamp, secs;
  uint16_t permille;
  uint8_t format, flags;
};
struct LibReadState {
  uint32_t offset, flowSize, stamp;
  uint16_t permille;
  bool finished;
  bool counted;  // its finishing is in the card's total
  LibBookStats stats;
  int nMarks;
  LibMark marks[LIB_MAX_MARKS];
};
struct LibDay {
  int32_t day;
  uint32_t secs, pages;
};
struct LibMeta;

class BookLibrary {
 public:
  // root: the card's top folder, "/sd" on the board. Nothing is read yet.
  explicit BookLibrary(const char *root);
  ~BookLibrary();
  BookLibrary(const BookLibrary &) = delete;
  BookLibrary &operator=(const BookLibrary &) = delete;

  // For the tests: another converter (default: BookConverter), another probe (default: bookProbe).
  void setConverterFactory(LibConverterFactory make, void *arg);
  void setProbe(LibProbe probe);

  // ---- the list
  // Lists the folders (names only: no book is opened) and takes what the
  // index knows of each. Closes the open book. False: no list, see state().
  bool scanBegin();
  // A slice of the rest: checks one book against the index and reads its
  // title if it's new or changed; when all are done, removes a few files of
  // books that are gone from the cache. True while there is more to do.
  bool scanStep();
  bool scanning() const { return phase_ != 0; }
  int scanDone() const;   // books looked at so far ...
  int scanTotal() const;  // ... of this many
  LibState state() const { return state_; }
  bool truncated() const { return truncated_; }  // more than LIB_MAX_BOOKS on the card
  // Goes up whenever the list's lines or their order changed: redraw, and
  // look your book up again with find() (places in the list move).
  uint32_t generation() const { return gen_; }

  int count() const { return (int)entries_.size(); }
  int count(LibStatus status) const;  // the size of a part: reading, new, finished come in this order
  bool book(int i, LibBook &out);     // i: 0..count()-1, in the order shown
  // The place in the list of the book with this key (LibBook::key), -1 if it's gone.
  int find(const char *key16);
  // The book last read and not finished (for "Continue"), -1 if none.
  int lastRead();
  // Removes the book's file from the card, and what the library kept of it.
  bool deleteBook(int i);

  // ---- opening a book
  // Starts opening book i: ready at once if it was converted before, else the
  // conversion starts. False: openError() says why. Closes the book open before.
  bool openBegin(int i, uint32_t now);
  // 0..999 while converting, BOOK_DONE when the book is open, negative when
  // it failed (openError()). When the card is full, the converted copies of
  // finished and long-unread books are removed (their places are kept) and
  // the conversion starts once more.
  int openStep();
  void openAbort();  // stops a conversion; nothing of it is left
  BookErr openError() const { return openErr_; }
  bool isOpen() const;

  // The open book (after openStep() returned BOOK_DONE).
  const char *flowPath() const { return flowPath_.c_str(); }    // its contents: flowPath + ".toc"
  const char *coverPath() const { return coverPath_.c_str(); }  // "" when there is no cover
  char coverType() const { return coverType_; }                 // 'j' JPEG, 'p' PNG, 0 none
  const char *openTitle() const { return openTitle_.c_str(); }
  const char *openAuthor() const { return openAuthor_.c_str(); }
  const char *openKey() const { return openKey_; }
  uint32_t flowSize() const { return st_.flowSize; }

  // ---- where the reader is, in the open book
  uint32_t position() const { return st_.offset; }
  uint16_t permille() const { return st_.permille; }
  bool finished() const { return st_.finished; }
  // The page shown starts at this flow offset. A finished book is being read
  // again once this is below 90% of it.
  void setPosition(uint32_t offset, uint32_t now);
  void setFinished(uint32_t now);  // the last page was reached

  int markCount() const { return st_.nMarks; }
  const LibMark &mark(int i) const { return st_.marks[i]; }  // by offset, 0..markCount()-1
  // The first bookmark in [start, end) (a page), -1 if none.
  int markIn(uint32_t start, uint32_t end) const;
  // Adds a bookmark (the label is cut to LIB_LABEL_MAX bytes, on a character).
  // One already at this offset gets the new label. False: there are LIB_MAX_MARKS.
  bool addMark(uint32_t offset, const char *label);
  void removeMark(int i);
  // The page's corner was tapped: removes the bookmarks in [start, end) if
  // there are any (returns 0), else adds one at start (1; -1: no room).
  int toggleMark(uint32_t start, uint32_t end, const char *label);

  // ---- statistics
  // What happened in the open book since the last call: seconds spent on it
  // (the app leaves out the time it sat untouched), pages turned forward and
  // the flow bytes they held. now: seconds since 1970 or 0; localDay:
  // bookLibLocalDay(), -1 unknown (the day log is left out then). Saved with
  // the place (saveState); the card's statistics at least every 5 minutes of
  // reading and on sync().
  void addReading(uint32_t seconds, uint32_t pagesForward, uint32_t bytesForward, uint32_t now, int32_t localDay = -1);
  const LibBookStats &bookStats() const { return st_.stats; }  // the open book's
  // today: bookLibLocalDay() (-1 unknown: the figures of today, this week and
  // the current streak are 0).
  LibStats libraryStats(int32_t today);
  // Book i's page: what the card has of it, without opening it (when it
  // hasn't been converted, its description is read from the book: quick).
  // False: no such book, or it's gone from the card.
  bool bookInfo(int i, LibInfo &out);

  // Writes the place and the bookmarks if they changed since the last call:
  // one small file. Call it every few pages. False: the card didn't take it.
  bool saveState();
  // saveState() and the list's file too: before sleeping, on leaving the app.
  bool sync();
  void closeBook();  // sync() and forget the book

 private:
  const char *str(uint32_t at) const { return &pool_[at]; }
  uint32_t intern(const std::string &s);
  std::string cachePath(uint64_t key, const char *suffix) const;
  bool ensureCacheDir();
  bool checkCard();
  void clearList();
  void eraseEntry(size_t idx);
  void listDir(const std::string &rel, int depth, int maxDepth);
  void adoptIndex();
  bool writeIndex();
  bool verify(size_t idx, bool &changed);
  void probe(LibEntry &e, bool fresh);
  bool sweepList();
  void sweepDelete();
  void ensureSorted();
  LibStatus statusOf(const LibEntry &e) const;
  void removeCache(uint64_t key, bool keepPos);
  bool startConvert();
  bool evict(uint64_t need);
  int openFail(BookErr err);
  void finishOpen(const LibMeta &m, uint32_t flowSize);
  void touch(uint32_t now);
  void stateToEntry();
  void dropOpen();
  bool metaValid(const LibEntry &e, LibMeta &m, uint32_t &flowSize);
  void loadStats();
  bool writeStats();

  std::string root_, cacheDir_;
  PsVec<LibEntry> entries_;
  PsVec<char> pool_;       // every string of the list, one after the other
  PsVec<uint16_t> order_;  // the list as shown
  bool sortDirty_ = true;
  LibState state_ = LS_IDLE;
  bool truncated_ = false, listErr_ = false, indexDirty_ = false;
  uint32_t gen_ = 0, top_ = 0;  // top_: the newest "last read" given so far

  uint8_t phase_ = 0;
  size_t cursor_ = 0;
  int probes_ = 0;
  DIR *sweepDir_ = nullptr;
  PsVec<char> sweepNames_;
  size_t sweepPos_ = 0;
  PsVec<uint64_t> live_;

  LibConverterFactory make_;
  void *makeArg_ = nullptr;
  LibProbe probe_;
  LibConverter *conv_ = nullptr;
  uint8_t openPhase_ = 0;
  int openIdx_ = -1;
  BookErr openErr_ = BE_OK;
  bool retried_ = false, stDirty_ = false;
  uint32_t now_ = 0;
  std::string flowPath_, coverPath_, openTitle_, openAuthor_;
  char coverType_ = 0;
  char openKey_[17] = {0};
  std::string openDesc_;
  bool firstReading_ = false;  // nothing read yet since it was opened
  LibReadState st_;

  PsVec<LibDay> days_;  // by day, the last LIB_STATS_DAYS
  uint32_t totFinished_ = 0, totSecs_ = 0, totPages_ = 0, statsUnsaved_ = 0;
  bool statsLoaded_ = false, statsDirty_ = false;
};

// "pride-and-prejudice.epub" -> "pride and prejudice": what stands in for a
// title until the book has been read (and for books that have none).
std::string bookLibTitleFromName(const char *fileName);
// A file's date as the card holds it (FAT keeps local time, with no zone):
// the same number whatever time zone the board is set to. The board's keys
// are made of this; a computer's of the plain modification time.
uint32_t bookLibLocalStamp(time_t t);
// The local day number (days since 1970 where the reader is): now in seconds
// since 1970 and the time zone's offset from UTC in seconds (summer time
// included). -1 when now is 0 (the clock isn't set).
int32_t bookLibLocalDay(uint32_t now, int32_t utcOffsetSecs);
