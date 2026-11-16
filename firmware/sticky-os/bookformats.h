// Between the book converter (ebook.cpp) and the formats: what each format's
// file implements, and what they share. Not for the app (that's ebook.h).
//
//   bookepub.cpp   EPUB (and the zip reading it needs, bookzip.h)
//   bookmobi.cpp   MOBI, AZW, AZW3, PRC
//   bookfb2.cpp    FB2
//   bookpdf.cpp    PDF
//   booktext.cpp   plain text, Markdown, HTML files
//   bookhtml.cpp   HtmlToFlow, which the others feed their (X)HTML to
//   bookinflate.cpp  inflate (bookinflate.h)
#pragma once
#include "ebook.h"

// One book being converted. step() does a slice of the work (aim for 20 to
// 100 ms on the board: a chapter's worth at most, a few tens of KB read)
// and returns 0..999, or BOOK_DONE when everything has been given to the
// writer (the converter closes it), or -(BookErr). The job opens the book in
// its factory or its first step and closes it in its destructor.
class BookJob {
 public:
  virtual ~BookJob() {}
  virtual int step() = 0;
};

// The factories: nullptr with err set when the job can't start. The writer
// is open and outlives the job. meta.format is already set; the job fills
// title, author, language and, if it wrote coverPath (may be nullptr: no
// cover wanted), meta.cover.
BookJob *epubJob(const char *path, FlowWriter &out, const char *coverPath, BookMeta &meta, BookErr &err);
BookJob *mobiJob(const char *path, FlowWriter &out, const char *coverPath, BookMeta &meta, BookErr &err);
BookJob *fb2Job(const char *path, FlowWriter &out, const char *coverPath, BookMeta &meta, BookErr &err);
BookJob *pdfJob(const char *path, FlowWriter &out, const char *coverPath, BookMeta &meta, BookErr &err);
BookJob *textJob(const char *path, FlowWriter &out, BookFormat format, BookMeta &meta, BookErr &err);  // BF_TXT (.md too) or BF_HTML

// The quick look for the library's list (bookProbe): title, author, language.
bool epubProbe(const char *path, BookMeta &meta, BookErr &err);
bool mobiProbe(const char *path, BookMeta &meta, BookErr &err);
bool fb2Probe(const char *path, BookMeta &meta, BookErr &err);
bool pdfProbe(const char *path, BookMeta &meta, BookErr &err);
bool textProbe(const char *path, BookFormat format, BookMeta &meta, BookErr &err);

// (X)HTML to the flow: fed the markup in pieces of any size (a tag, an
// entity or a character may be split between two), it writes paragraphs,
// headings, quotes, list items, rules, line breaks, italic and bold, and
// leaves out what isn't the text (head, style, script, the markup itself).
// A picture becomes its alt text in square brackets, or nothing.
// Used for an EPUB's chapters, a MOBI's text and .html files; FB2 has its own
// tags (bookfb2.cpp).
class HtmlToFlow {
 public:
  explicit HtmlToFlow(FlowWriter &out);
  ~HtmlToFlow();
  HtmlToFlow(const HtmlToFlow &) = delete;
  HtmlToFlow &operator=(const HtmlToFlow &) = delete;

  // The bytes' encoding when the document doesn't say (an XML declaration,
  // a meta charset or a byte order mark win): "utf-8" (the default),
  // "windows-1252", "iso-8859-1", "utf-16le", "utf-16be"; others are read as windows-1252.
  void setCharset(const char *name);
  // Headings (h1, h2, h3) become entries of the table of contents: for books
  // that have none of their own.
  void tocFromHeadings(bool on);
  // Called for every id="..." and <a name="..."> with the flow offset of the
  // block that holds it (the next block's, between blocks): how an EPUB's or
  // MOBI's contents find their place. arg is passed back.
  void onAnchor(void (*fn)(void *arg, const char *id, uint32_t flowOffset), void *arg);
  // Called for a MOBI's <mbp:pagebreak/> and for <a filepos=...>: byte
  // positions in the markup fed so far are the format's own business, so the
  // parser only reports how many bytes it has taken.
  uint32_t bytesFed() const;

  void feed(const char *data, size_t n);
  // The document's end: the open block is ended, the state reset for the next
  // document (an EPUB's next chapter). The charset default stays.
  void finish();
  // The document's <title>, if it had one (an .html file's name in the library).
  const std::string &title() const;

 private:
  struct Impl;
  Impl *d;
};
