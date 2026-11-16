// FB2, FictionBook 2 (bookformats.h): one XML file, with the book's
// description (title, authors, language, the cover's name), its text in
// <body> as nested <section>s, and its pictures in <binary> as base64.
//
// Read 16 KB a step and taken apart by a small XML tokenizer that keeps its
// place between any two bytes (the tag being read, an entity: a few hundred
// bytes). The XML declaration's encoding (UTF-8, windows-1251, koi8-r,
// windows-1252...) is looked at once, before the first byte is decoded.
// Pictures are skipped at memchr's speed, except the cover, which is
// decoded from base64 straight into the cover file.
//
// What becomes what: a top-level section starts a page; a section's title
// is a heading (the depth: H1, H2, H3) and a contents entry; <p> a paragraph;
// epigraph and cite quote; poem stanzas keep their lines; subtitle and
// text-author are centred; emphasis italic, strong bold; a note's link
// "[1]"; <empty-line/> ends the paragraph. The notes' body (and any other
// after the first) comes after the text, under its own title or "Notes".
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "bookcharset.h"
#include "bookformats.h"
#include "psalloc.h"

namespace {

const size_t READ = 16384;
const size_t TAG_MAX = 1024, TITLE_MAX = 600, META_MAX = 300, ANNOTATION_MAX = BOOK_DESCRIPTION_MAX + 512;
const int DEPTH_MAX = 64;

bool isSp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

// An attribute of a tag's inside by its local name ("href" for "l:href").
bool attrOf(const std::string &tag, const char *name, std::string &out) {
  out.clear();
  size_t want = strlen(name), i = 0, n = tag.size();
  while (i < n && !isSp(tag[i])) i++;  // the tag's name
  while (i < n) {
    while (i < n && (isSp(tag[i]) || tag[i] == '/')) i++;
    size_t a = i;
    while (i < n && !isSp(tag[i]) && tag[i] != '=') i++;
    size_t e = i, local = a;
    for (size_t k = a; k < e; k++)
      if (tag[k] == ':') local = k + 1;
    while (i < n && isSp(tag[i])) i++;
    if (i >= n || tag[i] != '=') continue;
    i++;
    while (i < n && isSp(tag[i])) i++;
    size_t v = i, ve;
    if (i < n && (tag[i] == '"' || tag[i] == '\'')) {
      char q = tag[i++];
      v = i;
      while (i < n && tag[i] != q) i++;
      ve = i;
      if (i < n) i++;
    } else {
      while (i < n && !isSp(tag[i])) i++;
      ve = i;
    }
    if (e - local == want && !tag.compare(local, want, name)) {
      out = tag.substr(v, ve - v);
      decodeEntities(out);
      return true;
    }
  }
  return false;
}

enum Tag : uint8_t {
  F_OTHER, F_A, F_ANNOTATION, F_AUTHOR, F_BINARY, F_BODY, F_BOOK_TITLE, F_CITE, F_COVERPAGE, F_DESCRIPTION, F_EMPTY_LINE, F_EMPHASIS,
  F_EPIGRAPH, F_FIRST_NAME, F_IMAGE, F_LANG, F_LAST_NAME, F_MIDDLE_NAME, F_P, F_POEM, F_SECTION, F_STANZA, F_STRONG,
  F_SUBTITLE, F_TD, F_TEXT_AUTHOR, F_TITLE, F_TITLE_INFO, F_TR, F_V
};

// In strcmp order.
const struct { char name[14]; uint8_t tag; } TAGS[] = {
    {"a", F_A}, {"annotation", F_ANNOTATION}, {"author", F_AUTHOR}, {"binary", F_BINARY}, {"body", F_BODY}, {"book-title", F_BOOK_TITLE}, {"cite", F_CITE},
    {"coverpage", F_COVERPAGE}, {"description", F_DESCRIPTION}, {"emphasis", F_EMPHASIS}, {"empty-line", F_EMPTY_LINE},
    {"epigraph", F_EPIGRAPH}, {"first-name", F_FIRST_NAME}, {"image", F_IMAGE}, {"lang", F_LANG}, {"last-name", F_LAST_NAME},
    {"middle-name", F_MIDDLE_NAME}, {"p", F_P}, {"poem", F_POEM}, {"section", F_SECTION}, {"stanza", F_STANZA}, {"strong", F_STRONG},
    {"subtitle", F_SUBTITLE}, {"td", F_TD}, {"text-author", F_TEXT_AUTHOR}, {"th", F_TD}, {"title", F_TITLE}, {"title-info", F_TITLE_INFO},
    {"tr", F_TR}, {"v", F_V},
};

Tag tagOf(const char *name) {
  int lo = 0, hi = (int)(sizeof TAGS / sizeof TAGS[0]) - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2, c = strcmp(name, TAGS[mid].name);
    if (!c) return (Tag)TAGS[mid].tag;
    if (c < 0) hi = mid - 1;
    else lo = mid + 1;
  }
  return F_OTHER;
}

int b64(unsigned char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+' || c == '-') return 62;
  if (c == '/' || c == '_') return 63;
  return -1;
}

enum St : uint8_t { ST_TEXT, ST_LT, ST_TAG, ST_BANG, ST_COMMENT, ST_CDATA, ST_DECL, ST_PI, ST_ENTITY };

class Fb2 {
 public:
  Fb2(FlowWriter *out, BookMeta &meta) : out_(out), meta_(meta) {}
  ~Fb2() {
    if (cover_) fclose(cover_), remove(coverPath_.c_str());
  }
  std::string coverPath_;  // where the cover goes ("": nowhere)
  bool descriptionDone() const { return descDone_; }

  // Bytes as they are in the file. The first call should hold the XML declaration.
  void feed(const char *p, size_t n) {
    if (!sniffed_) {
      sniffed_ = true;
      Charset cs = CS_UTF8;
      size_t skip = 0;
      if (n >= 3 && !memcmp(p, "\xEF\xBB\xBF", 3)) skip = 3;
      else if (n >= 2 && (uint8_t)p[0] == 0xFF && (uint8_t)p[1] == 0xFE) cs = CS_UTF16LE, skip = 2;
      else if (n >= 2 && (uint8_t)p[0] == 0xFE && (uint8_t)p[1] == 0xFF) cs = CS_UTF16BE, skip = 2;
      else {
        // <?xml version="1.0" encoding="windows-1251"?>
        const char *end = p + (n < 200 ? n : 200), *e = p;
        for (; e + 9 < end; e++)
          if (!memcmp(e, "encoding=", 9)) break;
        if (e + 10 < end) {
          char q = e[9];
          const char *v = e + 10, *ve = v;
          while (ve < end && *ve != q) ve++;
          if (ve < end) charsetByName(v, (size_t)(ve - v), cs);
        }
      }
      dec_.reset(cs);
      p += skip, n -= skip;
    }
    if (dec_.charset() == CS_UTF8) return tokenize(p, n);
    for (size_t i = 0; i < n; i += 256) {
      tmp_.clear();
      dec_.feed(p + i, n - i < 256 ? n - i : 256, tmp_);
      tokenize(tmp_.data(), tmp_.size());
    }
  }

  void finish() {
    if (out_) {
      flushTitle();
      out_->endBlock();
    }
    if (cover_) fclose(cover_), cover_ = nullptr, remove(coverPath_.c_str());
  }

 private:
  // ---- the tokenizer ----

  void tokenize(const char *p, size_t n) {
    size_t i = 0;
    while (i < n) {
      unsigned char c = p[i];
      switch (st_) {
        case ST_TEXT: {
          const char *lt = (const char *)memchr(p + i, '<', n - i);
          size_t j = lt ? (size_t)(lt - p) : n;
          if (!inBinary_) {
            const char *amp = (const char *)memchr(p + i, '&', j - i);
            if (amp) j = (size_t)(amp - p);
          }
          if (j > i) text(p + i, j - i);
          i = j;
          if (i < n) {
            st_ = p[i] == '<' ? ST_LT : ST_ENTITY;
            tag_.clear();
            i++;
          }
          break;
        }
        case ST_LT:
          if (c == '!') st_ = ST_BANG, i++;
          else if (c == '?') st_ = ST_PI, i++;
          else st_ = ST_TAG, attrQuote_ = 0;
          break;
        case ST_TAG:
          i++;
          if (attrQuote_) {
            if (c == attrQuote_) attrQuote_ = 0;
          } else if (c == '>') {
            st_ = ST_TEXT;
            endOfTag();
            break;
          } else if (c == '"' || c == '\'') {
            attrQuote_ = (char)c;
          }
          if (tag_.size() < TAG_MAX) tag_ += (char)c;
          break;
        case ST_BANG:
          tag_ += (char)c;
          i++;
          if (tag_ == "--") st_ = ST_COMMENT, match_ = 0;
          else if (tag_ == "[CDATA[") st_ = ST_CDATA, match_ = 0;
          else if (strncmp("--", tag_.c_str(), tag_.size()) && strncmp("[CDATA[", tag_.c_str(), tag_.size())) st_ = ST_DECL, match_ = 0, i--;
          break;
        case ST_COMMENT:
          i++;
          if (c == '-') match_++;
          else if (c == '>' && match_ >= 2) st_ = ST_TEXT;
          else match_ = 0;
          break;
        case ST_CDATA:
          i++;
          if (c == ']') {
            if (++match_ > 2) text("]", 1), match_ = 2;
          } else if (c == '>' && match_ >= 2) {
            st_ = ST_TEXT;
          } else {
            for (; match_ > 0; match_--) text("]", 1);
            text(p + i - 1, 1);
          }
          break;
        case ST_DECL:
          i++;
          if (c == '[') match_++;
          else if (c == ']') match_--;
          else if (c == '>' && match_ <= 0) st_ = ST_TEXT;
          break;
        case ST_PI:
          i++;
          if (c == '>') st_ = ST_TEXT;
          break;
        case ST_ENTITY:
          if (c == ';' || !(isalnum(c) || c == '#') || tag_.size() > 12) {
            if (c == ';') i++;
            st_ = ST_TEXT;
            std::string ent = "&" + tag_ + (c == ';' ? ";" : "");
            decodeEntities(ent);
            text(ent.data(), ent.size());
          } else {
            tag_ += (char)c;
            i++;
          }
          break;
      }
    }
  }

  // ---- the document ----

  void text(const char *p, size_t n) {
    if (inBinary_) return coverBytes(p, n);
    if (inAnnotation_) {
      // The book's description: its paragraphs' text, line ends inside them made spaces.
      for (size_t i = 0; i < n && annotation_.size() < ANNOTATION_MAX; i++) annotation_ += p[i] == '\n' || p[i] == '\r' ? ' ' : p[i];
      return;
    }
    if (field_) {
      if (field_->size() < META_MAX) field_->append(p, n < META_MAX ? n : META_MAX);
      return;
    }
    if (!out_ || !inBody_) return;
    if (inTitle_) {
      for (size_t i = 0; i < n && title_.size() < TITLE_MAX; i++) title_ += (unsigned char)p[i] < 0x20 ? ' ' : p[i];
      return;
    }
    if (!blockOpen_) {
      // The white space between tags is not a paragraph.
      size_t k = 0;
      while (k < n && isSp(p[k])) k++;
      if (k == n) return;
      openBlock();
    }
    out_->text(p, n);
  }

  void openBlock() {
    BlockKind kind = center_ ? BK_CENTER : quote_ ? BK_QUOTE : BK_PARA;
    out_->beginBlock(kind);
    blockOpen_ = true;
    out_->italic(italic_ > 0);
    out_->bold(bold_ > 0);
  }

  void endBlock() {
    if (out_ && blockOpen_) out_->endBlock();
    blockOpen_ = false;
  }

  void flushTitle() {
    if (!inTitle_) return;
    inTitle_ = false;
    std::string toc;
    for (char c : title_) toc += c == '\n' ? ' ' : c;
    int level = titleDepth_ < 1 ? 1 : titleDepth_ > 3 ? 3 : titleDepth_;
    if (notes_ && titleDepth_ > 0) {
      level = 3;  // a note's number: a small heading, not a chapter
    } else {
      if (titleDepth_ <= 1) out_->pageBreak();
      out_->tocEntry(toc.c_str(), level);
    }
    out_->beginBlock(level == 1 ? BK_H1 : level == 2 ? BK_H2 : BK_H3);
    size_t from = 0;
    for (size_t i = 0; i <= title_.size(); i++) {
      if (i < title_.size() && title_[i] != '\n') continue;
      if (i > from) {
        if (from) out_->lineBreak();
        out_->text(title_.data() + from, i - from);
      }
      from = i + 1;
    }
    out_->endBlock();
    title_.clear();
  }

  void endOfTag() {
    bool closing = tag_.size() && tag_[0] == '/';
    size_t a = closing ? 1 : 0, e = a;
    char name[16];
    size_t len = 0;
    for (; e < tag_.size() && !isSp(tag_[e]) && tag_[e] != '/'; e++) {
      if (tag_[e] == ':') len = 0;
      else if (len < sizeof name - 1) name[len++] = (char)tolower((unsigned char)tag_[e]);
    }
    name[len] = 0;
    bool selfClose = !closing && tag_.size() && tag_.back() == '/';
    Tag t = tagOf(name);
    if (inBinary_) {
      if (closing && t == F_BINARY) endBinary();
      return;
    }
    if (closing) return close(t);
    if (selfClose) {
      if (t == F_EMPTY_LINE) endBlock(), annotation_ += inAnnotation_ ? "\n" : "";
      else if (t == F_IMAGE && inCoverpage_ && coverId_.empty()) coverRef();
      return;
    }
    open(t);
  }

  void coverRef() {
    std::string href;
    if (attrOf(tag_, "href", href) && href.size() > 1 && href[0] == '#') coverId_ = href.substr(1);
  }

  void open(Tag t) {
    switch (t) {
      case F_DESCRIPTION: inDesc_ = true; break;
      case F_TITLE_INFO: if (inDesc_) inTitleInfo_ = true; break;
      case F_BOOK_TITLE: if (inTitleInfo_ && meta_.title.empty()) field_ = &meta_.title; break;
      case F_LANG: if (inTitleInfo_ && meta_.language.empty()) field_ = &meta_.language; break;
      case F_AUTHOR: if (inTitleInfo_ && meta_.author.empty() && !authorDone_) inAuthor_ = true, first_.clear(), middle_.clear(), last_.clear(); break;
      case F_FIRST_NAME: if (inAuthor_) field_ = &first_; break;
      case F_MIDDLE_NAME: if (inAuthor_) field_ = &middle_; break;
      case F_LAST_NAME: if (inAuthor_) field_ = &last_; break;
      case F_COVERPAGE: if (inTitleInfo_) inCoverpage_ = true; break;
      case F_ANNOTATION: if (inTitleInfo_ && meta_.description.empty()) inAnnotation_ = true, annotation_.clear(); break;
      case F_P: case F_SUBTITLE: if (inAnnotation_) annotation_ += '\n'; break;
      case F_IMAGE: if (inCoverpage_ && coverId_.empty()) coverRef(); break;
      case F_BINARY: startBinary(); break;
      case F_BODY: {
        inBody_ = true;
        depth_ = 0;
        notes_ = bodies_++ > 0;
        notesTitled_ = false;
        break;
      }
      default: break;
    }
    if (!out_ || !inBody_) return;
    switch (t) {
      case F_SECTION:
        endBlock();
        if (depth_ < DEPTH_MAX) depth_++;
        if (depth_ == 1 && !notes_) out_->pageBreak();
        break;
      case F_TITLE:
        endBlock();
        if (quote_ || poem_) {  // a poem's or an epigraph's title: centred, not in the contents
          center_++;
          break;
        }
        inTitle_ = true;
        title_.clear();
        titleDepth_ = depth_;
        if (notes_ && !depth_) notesTitled_ = true;
        break;
      case F_P:
        if (inTitle_) {
          if (title_.size()) title_ += '\n';
          break;
        }
        endBlock();
        break;
      case F_SUBTITLE: case F_TEXT_AUTHOR: endBlock(); center_++; break;
      case F_EPIGRAPH: case F_CITE: endBlock(); quote_++; break;
      case F_POEM: endBlock(); poem_++; break;
      case F_STANZA: endBlock(); break;
      case F_V:
        if (blockOpen_) out_->lineBreak();
        break;
      case F_TR: endBlock(); break;
      case F_TD: if (blockOpen_) out_->text(" ", 1); break;
      case F_EMPHASIS: italic_++; if (blockOpen_) out_->italic(true); break;
      case F_STRONG: bold_++; if (blockOpen_) out_->bold(true); break;
      case F_A: {
        std::string type;
        note_ = attrOf(tag_, "type", type) && type == "note";
        if (note_) text("[", 1);
        break;
      }
      default: break;
    }
    if (notes_ && !notesTitled_ && t == F_SECTION && depth_ == 1 && !notesHeaded_) {
      // The notes' body without a title of its own: "Notes".
      notesHeaded_ = true;
      out_->pageBreak();
      out_->tocEntry("Notes", 1);
      out_->beginBlock(BK_H1);
      out_->text("Notes");
      out_->endBlock();
    }
  }

  void close(Tag t) {
    if (field_ && (t == F_BOOK_TITLE || t == F_LANG || t == F_FIRST_NAME || t == F_MIDDLE_NAME || t == F_LAST_NAME)) field_ = nullptr;
    switch (t) {
      case F_DESCRIPTION: inDesc_ = inTitleInfo_ = inCoverpage_ = false; descDone_ = true; return;
      case F_TITLE_INFO: inTitleInfo_ = false; return;
      case F_COVERPAGE: inCoverpage_ = false; return;
      case F_ANNOTATION:
        if (inAnnotation_) bookCleanDescription(annotation_.data(), annotation_.size(), meta_.description, BOOK_DESCRIPTION_MAX);
        inAnnotation_ = false;
        std::string().swap(annotation_);
        return;
      case F_AUTHOR:
        if (inAuthor_) {
          std::string name;
          for (std::string *s : {&first_, &middle_, &last_}) {
            std::string w;
            for (char c : *s)
              if (!isSp(c) || (w.size() && w.back() != ' ')) w += isSp(c) ? ' ' : c;
            while (w.size() && w.back() == ' ') w.pop_back();
            if (w.size()) name += (name.size() ? " " : "") + w;
          }
          if (name.size()) meta_.author = name, authorDone_ = true;
          inAuthor_ = false;
        }
        return;
      case F_BODY: endBlock(); inBody_ = false; return;
      default: break;
    }
    if (!out_ || !inBody_) return;
    switch (t) {
      case F_SECTION: endBlock(); if (depth_ > 0) depth_--; break;
      case F_TITLE:
        if (inTitle_) flushTitle();
        else endBlock(), center_ = center_ > 0 ? center_ - 1 : 0;
        break;
      case F_P: if (!inTitle_) endBlock(); break;
      case F_SUBTITLE: case F_TEXT_AUTHOR: endBlock(); if (center_) center_--; break;
      case F_EPIGRAPH: case F_CITE: endBlock(); if (quote_) quote_--; break;
      case F_POEM: endBlock(); if (poem_) poem_--; break;
      case F_STANZA: case F_TR: endBlock(); break;
      case F_EMPHASIS: if (italic_) italic_--; if (blockOpen_) out_->italic(italic_ > 0); break;
      case F_STRONG: if (bold_) bold_--; if (blockOpen_) out_->bold(bold_ > 0); break;
      case F_A: if (note_) text("]", 1), note_ = false; break;
      default: break;
    }
  }

  // ---- the cover ----

  void startBinary() {
    inBinary_ = true;
    if (coverPath_.empty() || cover_ || coverDone_ || coverId_.empty()) return;
    std::string id;
    if (!attrOf(tag_, "id", id) || id != coverId_) return;
    cover_ = fopen(coverPath_.c_str(), "wb");
    acc_ = 0, nacc_ = 0, head_ = 0;
  }

  void coverBytes(const char *p, size_t n) {
    if (!cover_) return;
    uint8_t out[192];
    size_t m = 0;
    for (size_t i = 0; i < n; i++) {
      int v = b64((unsigned char)p[i]);
      if (v < 0) continue;  // white space, '=' padding
      acc_ = acc_ << 6 | (uint32_t)v;
      if (++nacc_ == 4) {
        out[m++] = (uint8_t)(acc_ >> 16), out[m++] = (uint8_t)(acc_ >> 8), out[m++] = (uint8_t)acc_;
        nacc_ = 0, acc_ = 0;
      }
      if (m + 3 > sizeof out) writeCover(out, m), m = 0;
    }
    if (m) writeCover(out, m);
  }

  void writeCover(const uint8_t *b, size_t n) {
    if (!cover_) return;
    // The first bytes say what it is: a JPEG or a PNG, or no cover.
    for (size_t i = 0; i < n && head_ < 8; i++) magic_[head_++] = b[i];
    if (head_ >= 3 && !kind_) {
      if (magic_[0] == 0xFF && magic_[1] == 0xD8 && magic_[2] == 0xFF) kind_ = 'j';
      else if (head_ >= 8 && !memcmp(magic_, "\x89PNG\r\n\x1A\n", 8)) kind_ = 'p';
      else if (head_ >= 8 || magic_[0] != 0x89) return dropCover();
    }
    if (fwrite(b, 1, n, cover_) != n) dropCover();
  }

  void dropCover() {
    if (cover_) fclose(cover_), remove(coverPath_.c_str());
    cover_ = nullptr;
    kind_ = 0;
    coverDone_ = true;
  }

  void endBinary() {
    inBinary_ = false;
    if (!cover_) return;
    // The last 2 or 3 sextets: one or two bytes more.
    uint8_t out[2];
    size_t m = 0;
    if (nacc_ == 2) out[m++] = (uint8_t)(acc_ >> 4);
    if (nacc_ == 3) out[m++] = (uint8_t)(acc_ >> 10), out[m++] = (uint8_t)(acc_ >> 2);
    if (m) writeCover(out, m);
    if (cover_ && kind_ && fclose(cover_) == 0) meta_.cover = kind_;
    else if (cover_) fclose(cover_), remove(coverPath_.c_str());
    cover_ = nullptr;
    coverDone_ = true;
  }

  FlowWriter *out_;
  BookMeta &meta_;
  CharsetDecoder dec_;
  bool sniffed_ = false;
  std::string tmp_, tag_;
  uint8_t st_ = ST_TEXT;
  char attrQuote_ = 0;
  int match_ = 0;
  // The description.
  bool inDesc_ = false, inTitleInfo_ = false, inAuthor_ = false, inCoverpage_ = false, authorDone_ = false, descDone_ = false;
  std::string *field_ = nullptr;
  std::string first_, middle_, last_, coverId_, annotation_;
  bool inAnnotation_ = false;
  // The text.
  bool inBody_ = false, notes_ = false, notesTitled_ = false, notesHeaded_ = false, inTitle_ = false, blockOpen_ = false, note_ = false;
  int bodies_ = 0, depth_ = 0, titleDepth_ = 0, center_ = 0, quote_ = 0, poem_ = 0, italic_ = 0, bold_ = 0;
  std::string title_;
  // The cover.
  bool inBinary_ = false, coverDone_ = false;
  FILE *cover_ = nullptr;
  uint32_t acc_ = 0;
  int nacc_ = 0, head_ = 0;
  uint8_t magic_[8];
  char kind_ = 0;
};

class Fb2Job : public BookJob {
 public:
  Fb2Job(FlowWriter &out, BookMeta &meta) : fb2_(&out, meta) {}
  ~Fb2Job() override {
    if (f_) fclose(f_);
    free(buf_);
  }
  bool start(const char *path, const char *coverPath, BookErr &err) {
    f_ = fopen(path, "rb");
    if (!f_) return err = BE_OPEN, false;
    if (fseek(f_, 0, SEEK_END) == 0) size_ = ftell(f_);
    if (size_ <= 0 || fseek(f_, 0, SEEK_SET) != 0) return err = BE_OPEN, false;
    buf_ = (char *)psMalloc(READ);
    if (!buf_) return err = BE_MEMORY, false;
    if (coverPath) fb2_.coverPath_ = coverPath;
    return true;
  }
  int step() override {
    size_t n = fread(buf_, 1, READ, f_);
    if (!n) {
      if (ferror(f_)) return -(int)BE_OPEN;
      fb2_.finish();
      return BOOK_DONE;
    }
    fb2_.feed(buf_, n);
    done_ += (long)n;
    int p = (int)((double)done_ * 999 / (double)size_);
    return p > 999 ? 999 : p;
  }

 private:
  Fb2 fb2_;
  FILE *f_ = nullptr;
  char *buf_ = nullptr;
  long size_ = 0, done_ = 0;
};

}  // namespace

BookJob *fb2Job(const char *path, FlowWriter &out, const char *coverPath, BookMeta &meta, BookErr &err) {
  Fb2Job *job = new Fb2Job(out, meta);
  if (!job->start(path, coverPath, err)) {
    delete job;
    return nullptr;
  }
  return job;
}

bool fb2Probe(const char *path, BookMeta &meta, BookErr &err) {
  FILE *f = fopen(path, "rb");
  if (!f) return err = BE_OPEN, false;
  char *buf = (char *)psMalloc(READ);
  if (!buf) return fclose(f), err = BE_MEMORY, false;
  Fb2 fb2(nullptr, meta);
  size_t n;
  // The description is at the start: 64 KB of the file at most.
  for (int k = 0; k < 4 && !fb2.descriptionDone() && (n = fread(buf, 1, READ, f)) > 0; k++) fb2.feed(buf, n);
  fclose(f);
  free(buf);
  return true;
}
