#include "ebook.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "bookformats.h"

static bool endsWith(const char *name, const char *ext) {
  size_t n = strlen(name), e = strlen(ext);
  if (n < e) return false;
  for (size_t i = 0; i < e; i++)
    if (tolower((unsigned char)name[n - e + i]) != ext[i]) return false;
  return true;
}

BookFormat bookFormatOfName(const char *name) {
  static const struct { const char *ext; BookFormat f; } EXT[] = {
      {".epub", BF_EPUB}, {".mobi", BF_MOBI}, {".azw", BF_MOBI},     {".azw3", BF_MOBI}, {".prc", BF_MOBI},
      {".fb2", BF_FB2},   {".pdf", BF_PDF},   {".txt", BF_TXT},      {".md", BF_TXT},    {".markdown", BF_TXT},
      {".html", BF_HTML}, {".htm", BF_HTML},  {".xhtml", BF_HTML},
  };
  if (!name || name[0] == '.') return BF_UNKNOWN;  // hidden files: macOS's "._Alice.epub"
  for (auto &e : EXT)
    if (endsWith(name, e.ext)) return e.f;
  return BF_UNKNOWN;
}

const char *bookFormatLabel(BookFormat f) {
  static const char *const LABEL[] = {"", "EPUB", "MOBI", "FB2", "PDF", "Text", "HTML"};
  return f <= BF_HTML ? LABEL[f] : "";
}

// What the file is: by its first bytes when they say (a book with the wrong
// ending still opens), else by its name.
static BookFormat formatOf(const char *path) {
  const char *slash = strrchr(path, '/');
  BookFormat byName = bookFormatOfName(slash ? slash + 1 : path);
  FILE *f = fopen(path, "rb");
  if (!f) return byName;
  unsigned char head[68] = {0};
  size_t n = fread(head, 1, sizeof head, f);
  fclose(f);
  if (n >= 5 && !memcmp(head, "%PDF-", 5)) return BF_PDF;
  if (n >= 68 && (!memcmp(head + 60, "BOOKMOBI", 8) || !memcmp(head + 60, "TEXtREAd", 8))) return BF_MOBI;
  if (n >= 4 && !memcmp(head, "PK\3\4", 4) && byName != BF_UNKNOWN) return BF_EPUB;
  return byName;
}

bool bookProbe(const char *path, BookMeta &meta, BookErr &err) {
  meta = BookMeta();
  err = BE_OK;
  meta.format = formatOf(path);
  switch (meta.format) {
    case BF_EPUB: return epubProbe(path, meta, err);
    case BF_MOBI: return mobiProbe(path, meta, err);
    case BF_FB2: return fb2Probe(path, meta, err);
    case BF_PDF: return pdfProbe(path, meta, err);
    case BF_TXT:
    case BF_HTML: return textProbe(path, meta.format, meta, err);
    default: err = BE_FORMAT; return false;
  }
}

BookConverter::BookConverter() {}
BookConverter::~BookConverter() { cleanup(true); }

void BookConverter::cleanup(bool removeFiles) {
  delete job_;
  job_ = nullptr;
  if (writer_) {
    writer_->close();
    delete writer_;
    writer_ = nullptr;
    if (removeFiles) {
      remove(flowPath_.c_str());
      remove((flowPath_ + ".toc").c_str());
      if (coverPath_.size()) remove(coverPath_.c_str());
    }
  }
}

bool BookConverter::begin(const char *bookPath, const char *flowPath, const char *coverPath) {
  cleanup(true);
  meta_ = BookMeta();
  err_ = BE_OK;
  flowPath_ = flowPath;
  coverPath_ = coverPath ? coverPath : "";
  meta_.format = formatOf(bookPath);
  if (meta_.format == BF_UNKNOWN) return err_ = BE_FORMAT, false;
  writer_ = new FlowWriter();
  if (!writer_->open(flowPath)) {
    delete writer_;
    writer_ = nullptr;
    return err_ = BE_WRITE, false;
  }
  switch (meta_.format) {
    case BF_EPUB: job_ = epubJob(bookPath, *writer_, coverPath, meta_, err_); break;
    case BF_MOBI: job_ = mobiJob(bookPath, *writer_, coverPath, meta_, err_); break;
    case BF_FB2: job_ = fb2Job(bookPath, *writer_, coverPath, meta_, err_); break;
    case BF_PDF: job_ = pdfJob(bookPath, *writer_, coverPath, meta_, err_); break;
    default: job_ = textJob(bookPath, *writer_, meta_.format, meta_, err_); break;
  }
  if (!job_) {
    if (err_ == BE_OK) err_ = BE_DAMAGED;
    cleanup(true);
    return false;
  }
  return true;
}

int BookConverter::step() {
  if (!job_) return err_ == BE_OK ? BOOK_DONE : -(int)err_;
  int p = job_->step();
  if (p >= 0 && p < BOOK_DONE) {
    if (!writer_->failed()) return p;
    p = -(int)BE_WRITE;  // the card filled up or went
  }
  if (p == BOOK_DONE) {
    delete job_;
    job_ = nullptr;
    bool ok = writer_->close(), any = writer_->textBytes() > 0;
    if (!ok) p = -(int)BE_WRITE;
    else if (!any) p = -(int)BE_NO_TEXT;
    else {
      delete writer_;
      writer_ = nullptr;
      return BOOK_DONE;
    }
  }
  err_ = (BookErr)(-p > 0 && -p < BE_COUNT ? -p : BE_DAMAGED);
  cleanup(true);
  return -(int)err_;
}

void BookConverter::abort() { cleanup(true); }
