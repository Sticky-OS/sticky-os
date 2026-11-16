// Books: reading a book file and turning it into the Books app's flow
// (bookflow.h). EPUB 2 and 3, MOBI / AZW / AZW3 without DRM, FB2, PDF (its
// text, reflowed), plain text, Markdown and HTML.
//
// Plain C++ over stdio (no Arduino): it runs on the board, where the card
// is mounted in the file system, and in the tests on a computer
// (firmware/test/ebook).
//
// A book is converted in steps, so the screen can show how far it is and the
// buttons keep working:
//
//   BookConverter c;
//   if (!c.begin("/sd/books/alice.epub", "/sd/books/.sticky/1a2b.flow", "/sd/books/.sticky/1a2b.cover")) ...
//   int p;
//   while ((p = c.step()) >= 0 && p < BOOK_DONE) showProgress(p);
//   if (p < 0) say(c.error());
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string>

#include "bookflow.h"

enum BookFormat : uint8_t { BF_UNKNOWN, BF_EPUB, BF_MOBI, BF_FB2, BF_PDF, BF_TXT, BF_HTML };

// Why a book can't be opened. The screens say it in the board's language.
enum BookErr : uint8_t {
  BE_OK,
  BE_OPEN,         // the file can't be opened or read (a removed card)
  BE_FORMAT,       // not a kind of book the board reads
  BE_DAMAGED,      // the file is cut short or broken
  BE_DRM,          // copy-protected (Kindle or Adobe DRM)
  BE_ENCRYPTED,    // a PDF with a password
  BE_NO_TEXT,      // nothing to read in it: a scanned PDF, a picture book
  BE_WRITE,        // the converted book can't be saved (a full or locked card)
  BE_MEMORY,
  BE_COUNT
};

// How long a book's description may be, in bytes of UTF-8 (cut on a character).
static const size_t BOOK_DESCRIPTION_MAX = 2000;

struct BookMeta {
  std::string title, author, language;  // UTF-8; empty when the file doesn't say
  // The book's own introduction, as its publisher wrote it (EPUB dc:description,
  // MOBI EXTH 103, FB2 annotation, PDF /Subject): plain UTF-8 text, markup and
  // entities resolved, white space collapsed, paragraphs separated by one '\n',
  // no other control characters, at most BOOK_DESCRIPTION_MAX bytes. Empty
  // when the book has none. bookProbe() fills it too, when it's cheap to.
  std::string description;
  BookFormat format = BF_UNKNOWN;
  // What the cover file written by the converter holds: 'j' a JPEG, 'p' a PNG, 0 none.
  char cover = 0;
};

// By the file's name ("Alice.EPUB"): is it a book the board reads, and which kind.
// .epub; .mobi .azw .azw3 .prc; .fb2; .pdf; .txt .md .markdown; .html .htm .xhtml
BookFormat bookFormatOfName(const char *name);
const char *bookFormatLabel(BookFormat f);  // "EPUB", "MOBI", "FB2", "PDF", "Text", "HTML"

// The title and author, read quickly without converting the book (the
// library's list). False: the file can't be read or isn't that kind of book
// (err says); a book with no title in it is still true, with title empty.
bool bookProbe(const char *path, BookMeta &meta, BookErr &err);

static const int BOOK_DONE = 1000;

class BookJob;

class BookConverter {
 public:
  BookConverter();
  ~BookConverter();
  BookConverter(const BookConverter &) = delete;
  BookConverter &operator=(const BookConverter &) = delete;

  // Starts turning bookPath into flowPath (and flowPath + ".toc"); the cover
  // picture, if the book has one, goes to coverPath as it is in the book
  // (nullptr: no cover wanted). False: see error().
  bool begin(const char *bookPath, const char *flowPath, const char *coverPath);
  // A slice of the work, some tens of milliseconds on the board (never more
  // than about 300). Returns how far it is, 0..999; BOOK_DONE when the flow
  // is written and closed; negative when it failed (error() says why, and
  // the half-written files are removed).
  int step();
  // Stops and removes what was written.
  void abort();
  bool running() const { return job_ != nullptr; }
  BookErr error() const { return err_; }
  // Filled in as the book is read; whole once step() returned BOOK_DONE.
  const BookMeta &meta() const { return meta_; }

 private:
  void cleanup(bool removeFiles);
  BookJob *job_ = nullptr;
  FlowWriter *writer_ = nullptr;
  BookMeta meta_;
  BookErr err_ = BE_OK;
  std::string flowPath_, coverPath_;
};
