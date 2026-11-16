// EPUB 2 and 3 (bookformats.h): a zip (bookzip.h) of XHTML chapters, with a
// package file (the OPF) that lists them in reading order and names the
// cover and the contents (EPUB 3's nav document, or EPUB 2's NCX).
//
// The factory reads the small XML files whole (each with a size cap) with the
// minimal scanner below and keeps what it needs in a few PSRAM vectors: the
// chapters in order, the contents entries, their strings in one pool. Then
// step() copies the cover (32 KB a step) and feeds the chapters to
// HtmlToFlow (32 KB of unpacked markup a step; the inflater is carried from
// step to step).
//
// The contents point at "chapter.xhtml#id". An entry is given to the writer
// at the moment its place is written: when its chapter starts, or when the
// parser reports its id. So entries come out in the flow's order, which
// can't go backwards, and the ids nobody points at cost nothing.
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

#include "bookcharset.h"
#include "bookformats.h"
#include "bookzip.h"
#include "psalloc.h"

namespace {

const size_t CONTAINER_MAX = 64 * 1024, OPF_MAX = 512 * 1024, TOC_MAX = 2 * 1024 * 1024, COVER_PAGE_MAX = 64 * 1024;
const size_t SLICE = 32 * 1024;  // unpacked bytes a step
const size_t READ_BUF = 8192;
const size_t TOC_ENTRIES_MAX = 3000, SPINE_MAX = 3000, MANIFEST_MAX = 4000;

// ---- a minimal XML scanner, for the package's own files ----

struct XmlTag {
  bool close, selfClose;
  const char *start;  // its '<'
  const char *name;   // the local name (after any "prefix:")
  size_t nameLen;
  const char *attrs, *attrsEnd;
};

bool isSp(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

// The next tag from p, skipping comments, declarations, processing
// instructions and CDATA. The text before it is [*text, t.start). False at the end.
bool xmlNext(const char *&p, const char *end, XmlTag &t, const char **text = nullptr) {
  if (text) *text = p;
  for (;;) {
    const char *lt = (const char *)memchr(p, '<', (size_t)(end - p));
    if (!lt) return p = end, false;
    const char *q = lt + 1;
    const char *skipTo = nullptr;
    if (end - q >= 3 && !memcmp(q, "!--", 3)) skipTo = "-->";
    else if (end - q >= 8 && !memcmp(q, "![CDATA[", 8)) skipTo = "]]>";
    if (skipTo) {
      const char *e = q + 3;
      while (e + 2 < end && memcmp(e, skipTo, 3)) e++;
      if (e + 2 >= end) return p = end, false;
      p = e + 3;
      continue;
    }
    // The tag's end: the first '>' outside quotes.
    const char *e = q;
    char quote = 0;
    for (; e < end; e++) {
      if (quote) {
        if (*e == quote) quote = 0;
      } else if (*e == '"' || *e == '\'') {
        quote = *e;
      } else if (*e == '>') {
        break;
      }
    }
    if (e >= end) return p = end, false;
    p = e + 1;
    if (*q == '!' || *q == '?') continue;
    t.start = lt;
    t.close = *q == '/';
    if (t.close) q++;
    const char *n = q;
    while (q < e && !isSp(*q) && *q != '/') q++;
    const char *local = n;
    for (const char *k = n; k < q; k++)
      if (*k == ':') local = k + 1;
    t.name = local, t.nameLen = (size_t)(q - local);
    t.attrs = q;
    t.attrsEnd = e;
    t.selfClose = e > q && e[-1] == '/';
    if (t.selfClose) t.attrsEnd = e - 1;
    return true;
  }
}

bool is(const XmlTag &t, const char *name) { return strlen(name) == t.nameLen && !strncmp(t.name, name, t.nameLen); }

// An attribute by its local name ("type" for "epub:type"), entities decoded.
bool attr(const XmlTag &t, const char *name, std::string &out) {
  out.clear();
  const char *p = t.attrs, *end = t.attrsEnd;
  size_t want = strlen(name);
  while (p < end) {
    while (p < end && isSp(*p)) p++;
    const char *n = p;
    while (p < end && !isSp(*p) && *p != '=') p++;
    const char *ne = p, *local = n;
    for (const char *k = n; k < ne; k++)
      if (*k == ':') local = k + 1;
    while (p < end && isSp(*p)) p++;
    if (p >= end || *p != '=') continue;
    p++;
    while (p < end && isSp(*p)) p++;
    const char *v = p, *ve;
    if (p < end && (*p == '"' || *p == '\'')) {
      char q = *p++;
      v = p;
      while (p < end && *p != q) p++;
      ve = p;
      if (p < end) p++;
    } else {
      while (p < end && !isSp(*p)) p++;
      ve = p;
    }
    if ((size_t)(ne - local) == want && !strncmp(local, name, want)) {
      out.assign(v, ve);
      decodeEntities(out);
      return true;
    }
  }
  return false;
}

void trim(std::string &s) {
  while (s.size() && s.back() == ' ') s.pop_back();
  size_t i = 0;
  while (i < s.size() && s[i] == ' ') i++;
  s.erase(0, i);
}

// The text of the element whose start tag was just read, up to its end tag
// (tags inside it left out): entities decoded, spaces single, trimmed.
std::string elementText(const char *&p, const char *end, size_t cap = 400) {
  std::string raw;
  XmlTag t;
  const char *text;
  int depth = 0;
  for (;;) {
    bool more = xmlNext(p, end, t, &text);
    const char *upto = more ? t.start : end;
    if (raw.size() < cap * 2) raw.append(text, (size_t)(upto - text) < cap * 2 ? (size_t)(upto - text) : cap * 2);
    if (!more) break;
    if (t.close) {
      if (!depth--) break;
    } else if (!t.selfClose) {
      depth++;
    }
    raw += ' ';  // "<br/>" or a span's edge: words don't run together
  }
  decodeEntities(raw);
  std::string s;
  for (char c : raw) {
    if (s.size() >= cap) break;
    if (isSp(c)) {
      if (s.size() && s.back() != ' ') s += ' ';
    } else {
      s += c;
    }
  }
  while (s.size() && ((uint8_t)s.back() & 0xC0) == 0x80 && s.size() >= cap) s.pop_back();  // not cut inside a character
  trim(s);
  return s;
}

// The element's contents as they are, markup and all (dc:description holds
// HTML, escaped or not), made into a description (bookCleanDescription).
std::string elementMarkup(const char *&p, const char *end) {
  const char *from = p;
  XmlTag t;
  int depth = 0;
  const char *upto = end;
  while (xmlNext(p, end, t)) {
    if (t.close) {
      if (!depth--) {
        upto = t.start;
        break;
      }
    } else if (!t.selfClose) {
      depth++;
    }
  }
  std::string raw(from, upto), out;
  for (const char *mark : {"<![CDATA[", "]]>"})
    for (size_t at; (at = raw.find(mark)) != std::string::npos;) raw.erase(at, strlen(mark));
  bookCleanDescription(raw.data(), raw.size(), out, BOOK_DESCRIPTION_MAX);
  return out;
}

// ---- paths ----

int hexVal(char c) { return isdigit((unsigned char)c) ? c - '0' : isxdigit((unsigned char)c) ? tolower((unsigned char)c) - 'a' + 10 : -1; }

std::string percentDecode(const std::string &s) {
  std::string o;
  for (size_t i = 0; i < s.size(); i++) {
    int a, b;
    if (s[i] == '%' && i + 2 < s.size() && (a = hexVal(s[i + 1])) >= 0 && (b = hexVal(s[i + 2])) >= 0) o += (char)(a * 16 + b), i += 2;
    else o += s[i];
  }
  return o;
}

// An href (no fragment) against the folder of the file it is in ("OEBPS/"):
// the file's path in the zip, %20 and ../ resolved.
std::string resolve(const std::string &base, const std::string &href) {
  std::string h = percentDecode(href);
  std::string path = h.size() && h[0] == '/' ? std::string() : base;
  size_t i = 0;
  while (i <= h.size()) {
    size_t slash = h.find('/', i);
    if (slash == std::string::npos) slash = h.size();
    std::string part = h.substr(i, slash - i);
    if (part == "..") {
      if (path.size()) path.pop_back();  // its '/'
      size_t up = path.rfind('/');
      path.erase(up == std::string::npos ? 0 : up + 1);
    } else if (part.size() && part != ".") {
      path += part;
      if (slash < h.size()) path += '/';
    }
    i = slash + 1;
  }
  return path;
}

std::string folderOf(const std::string &path) {
  size_t s = path.rfind('/');
  return s == std::string::npos ? std::string() : path.substr(0, s + 1);
}

bool hasWord(const std::string &list, const char *word) {
  size_t n = strlen(word);
  for (size_t i = 0; i + n <= list.size(); i++)
    if (!list.compare(i, n, word) && (i == 0 || isSp(list[i - 1])) && (i + n == list.size() || isSp(list[i + n]))) return true;
  return false;
}

bool containsNoCase(const std::string &s, const char *what) {
  size_t n = strlen(what);
  for (size_t i = 0; i + n <= s.size(); i++) {
    size_t k = 0;
    while (k < n && tolower((unsigned char)s[i + k]) == what[k]) k++;
    if (k == n) return true;
  }
  return false;
}

bool endsNoCase(const std::string &s, const char *what) {
  size_t n = strlen(what);
  return s.size() >= n && containsNoCase(s.substr(s.size() - n), what);
}

// ---- the package ----

// The job's strings in one PSRAM pool, by offset.
struct Pool {
  PsVec<char> bytes;
  uint32_t add(const std::string &s) {
    uint32_t at = (uint32_t)bytes.size();
    bytes.insert(bytes.end(), s.begin(), s.end());
    bytes.push_back(0);
    return at;
  }
  const char *at(uint32_t i) const { return bytes.data() + i; }
};

struct Item { uint32_t id, path, type, props; };  // of the manifest
struct Doc { int zip; uint32_t size; };          // a chapter, in reading order
struct Entry {                                   // of the contents
  int doc;               // the chapter it points into
  uint32_t frag, title;  // the fragment ("" for the chapter's start), in the pool
  uint8_t level;
  bool done;
};

struct Package {
  ZipFile zip;
  Pool pool;
  PsVec<Item> items;
  PsVec<Doc> docs;
  PsVec<Entry> toc;
  std::string title, author, language, description, coverPath, navPath, ncxPath;
};

bool readXml(ZipFile &zip, const std::string &path, PsVec<char> &buf, size_t cap) {
  int i = zip.find(path.c_str());
  return i >= 0 && zipReadAll(zip, i, buf, cap);
}

// META-INF/encryption.xml: anything but font obfuscation on anything but a font is DRM.
bool encrypted(ZipFile &zip) {
  if (zip.find("META-INF/rights.xml") >= 0) return true;
  int i = zip.find("META-INF/encryption.xml");
  if (i < 0) return false;
  PsVec<char> buf;
  if (!zipReadAll(zip, i, buf, OPF_MAX)) return true;  // there, and unreadable: assume the worst
  const char *p = buf.data(), *end = p + buf.size() - 1;
  XmlTag t;
  std::string alg, uri;
  while (xmlNext(p, end, t)) {
    if (t.close && !is(t, "EncryptedData")) continue;
    if (is(t, "EncryptedData") && !t.close) alg.clear(), uri.clear();
    else if (is(t, "EncryptionMethod")) attr(t, "Algorithm", alg);
    else if (is(t, "CipherReference")) attr(t, "URI", uri);
    else if (is(t, "EncryptedData")) {
      bool obfuscation = alg == "http://www.idpf.org/2008/embedding" || alg == "http://ns.adobe.com/pdf/enc#RC";
      bool font = endsNoCase(uri, ".ttf") || endsNoCase(uri, ".otf") || endsNoCase(uri, ".woff") || endsNoCase(uri, ".woff2");
      if (!obfuscation && !font) return true;
    }
  }
  return false;
}

// container.xml and the OPF: metadata; unless metaOnly, the chapters and the cover's and contents' files.
bool readPackage(Package &pk, BookErr &err, bool metaOnly) {
  PsVec<char> buf;
  std::string opfPath;
  if (!readXml(pk.zip, "META-INF/container.xml", buf, CONTAINER_MAX)) return err = BE_DAMAGED, false;
  {
    const char *p = buf.data(), *end = p + buf.size() - 1;
    XmlTag t;
    std::string type, path;
    while (xmlNext(p, end, t)) {
      if (!is(t, "rootfile") || t.close) continue;
      attr(t, "media-type", type);
      if (attr(t, "full-path", path) && (type.empty() || type == "application/oebps-package+xml")) {
        opfPath = resolve("", path);
        break;
      }
    }
  }
  if (opfPath.empty() || !readXml(pk.zip, opfPath, buf, OPF_MAX)) return err = BE_DAMAGED, false;
  std::string opfDir = folderOf(opfPath);
  const char *p = buf.data(), *end = p + buf.size() - 1;
  XmlTag t;
  std::string a, b, c, d, tocId, coverId, guideCover;
  bool inMeta = false, inManifest = false, inSpine = false;
  PsVec<uint32_t> spine, later;  // idrefs; linear="no" ones apart
  while (xmlNext(p, end, t)) {
    if (is(t, "metadata")) {
      inMeta = !t.close && !t.selfClose;
    } else if (is(t, "manifest")) {
      inManifest = !t.close && !t.selfClose;
    } else if (is(t, "spine")) {
      inSpine = !t.close && !t.selfClose;
      if (!t.close) attr(t, "toc", tocId);
    } else if (t.close) {
      continue;
    } else if (inMeta) {
      if (t.selfClose) {
        if (is(t, "meta") && attr(t, "name", a) && a == "cover" && attr(t, "content", b)) coverId = b;
      } else if (is(t, "title") && pk.title.empty()) {
        pk.title = elementText(p, end);
      } else if (is(t, "creator") && pk.author.empty()) {
        pk.author = elementText(p, end);
      } else if (is(t, "language") && pk.language.empty()) {
        pk.language = elementText(p, end, 40);
      } else if (is(t, "description") && pk.description.empty()) {
        pk.description = elementMarkup(p, end);
      } else if (is(t, "meta") && attr(t, "name", a) && a == "cover" && attr(t, "content", b)) {
        coverId = b;
      }
    } else if (metaOnly) {
      continue;
    } else if (inManifest && is(t, "item") && pk.items.size() < MANIFEST_MAX) {
      if (!attr(t, "id", a) || !attr(t, "href", b)) continue;
      attr(t, "media-type", c);
      attr(t, "properties", d);
      Item it;
      it.id = pk.pool.add(a), it.path = pk.pool.add(resolve(opfDir, b.substr(0, b.find('#')))), it.type = pk.pool.add(c), it.props = pk.pool.add(d);
      pk.items.push_back(it);
    } else if (inSpine && is(t, "itemref") && spine.size() + later.size() < SPINE_MAX) {
      if (!attr(t, "idref", a)) continue;
      bool linear = !attr(t, "linear", b) || b != "no";
      (linear ? spine : later).push_back(pk.pool.add(a));
    } else if (is(t, "reference") && guideCover.empty() && attr(t, "type", a) && containsNoCase(a, "cover") && attr(t, "href", b)) {
      guideCover = resolve(opfDir, b.substr(0, b.find('#')));
    }
  }
  if (metaOnly) return true;

  auto byId = [&pk](const char *id) -> const Item * {
    for (auto &it : pk.items)
      if (!strcmp(pk.pool.at(it.id), id)) return &it;
    return nullptr;
  };
  // The reading order: the linear chapters, or all if none is.
  if (spine.empty()) spine.swap(later);
  for (uint32_t ref : spine) {
    const Item *it = byId(pk.pool.at(ref));
    if (!it) continue;
    std::string type = pk.pool.at(it->type), path = pk.pool.at(it->path);
    bool html = type.empty() ? containsNoCase(path, ".htm") || containsNoCase(path, ".xhtml") || endsNoCase(path, ".xml") : containsNoCase(type, "html") || containsNoCase(type, "xml");
    int z = pk.zip.find(path.c_str());
    if (html && z >= 0) pk.docs.push_back({z, pk.zip.size(z)});
  }
  // The contents' file, the cover's.
  const Item *cover = nullptr;
  for (auto &it : pk.items) {
    std::string props = pk.pool.at(it.props), type = pk.pool.at(it.type);
    if (hasWord(props, "nav") && pk.navPath.empty()) pk.navPath = pk.pool.at(it.path);
    if (hasWord(props, "cover-image") && !cover) cover = &it;
    if (type == "application/x-dtbncx+xml" && pk.ncxPath.empty()) pk.ncxPath = pk.pool.at(it.path);
  }
  if (tocId.size()) {
    const Item *it = byId(tocId.c_str());
    if (it) pk.ncxPath = pk.pool.at(it->path);
  }
  if (!cover && coverId.size()) cover = byId(coverId.c_str());
  if (cover) {
    pk.coverPath = pk.pool.at(cover->path);
  } else if (guideCover.size()) {
    pk.coverPath = guideCover;  // a picture, or a page with one in it
  } else {
    for (auto &it : pk.items) {
      std::string id = pk.pool.at(it.id), path = pk.pool.at(it.path), type = pk.pool.at(it.type);
      if (containsNoCase(type, "image") && (containsNoCase(id, "cover") || containsNoCase(path, "cover"))) {
        pk.coverPath = path;
        break;
      }
    }
  }
  return true;
}

// The contents, from the nav document (EPUB 3) or the NCX (EPUB 2), in the order they list them.
void readToc(Package &pk) {
  PsVec<int16_t> docOf(pk.zip.count(), -1);  // a zip entry's place in the reading order
  for (size_t i = 0; i < pk.docs.size(); i++)
    if (docOf[pk.docs[i].zip] < 0) docOf[pk.docs[i].zip] = (int16_t)i;
  auto add = [&](const std::string &file, const std::string &href, std::string title, int level) {
    trim(title);
    if (title.empty() || href.empty() || pk.toc.size() >= TOC_ENTRIES_MAX) return;
    size_t hash = href.find('#');
    std::string path = hash == 0 ? file : resolve(folderOf(file), href.substr(0, hash));
    int z = pk.zip.find(path.c_str());
    if (z < 0 || docOf[z] < 0) return;
    Entry e;
    e.doc = docOf[z];
    e.frag = pk.pool.add(hash == std::string::npos ? std::string() : percentDecode(href.substr(hash + 1)));
    e.title = pk.pool.add(title);
    e.level = (uint8_t)(level < 1 ? 1 : level > 3 ? 3 : level);
    e.done = false;
    pk.toc.push_back(e);
  };
  PsVec<char> buf;
  XmlTag t;
  std::string a;
  if (pk.navPath.size() && readXml(pk.zip, pk.navPath, buf, TOC_MAX)) {
    const char *p = buf.data(), *end = p + buf.size() - 1;
    int navDepth = 0, ol = 0;
    bool inToc = false;
    while (xmlNext(p, end, t)) {
      if (is(t, "nav") && !t.selfClose) {
        if (t.close) {
          if (inToc && --navDepth == 0) break;
        } else if (inToc) {
          navDepth++;
        } else if (attr(t, "type", a) && hasWord(a, "toc")) {
          inToc = true, navDepth = 1;
        }
      } else if (inToc && is(t, "ol") && !t.selfClose) {
        ol += t.close ? -1 : 1;
      } else if (inToc && is(t, "a") && !t.close && !t.selfClose) {
        std::string href;
        attr(t, "href", href);
        std::string title = elementText(p, end);
        add(pk.navPath, href, title, ol);
      }
    }
  }
  if (pk.toc.empty() && pk.ncxPath.size() && readXml(pk.zip, pk.ncxPath, buf, TOC_MAX)) {
    const char *p = buf.data(), *end = p + buf.size() - 1;
    std::string label;
    int depth = 0;
    bool inMap = false, inLabel = false;
    while (xmlNext(p, end, t)) {
      if (is(t, "navMap")) inMap = !t.close && !t.selfClose;
      else if (!inMap) continue;
      else if (is(t, "navPoint") && !t.selfClose) depth += t.close ? -1 : 1, label.clear();
      else if (is(t, "navLabel")) inLabel = !t.close && !t.selfClose;
      else if (is(t, "text") && inLabel && !t.close && !t.selfClose) label = elementText(p, end);
      else if (is(t, "content") && !t.close && attr(t, "src", a)) add(pk.ncxPath, a, label, depth), label.clear();
    }
  }
}

bool coverKind(const uint8_t *b, size_t n, char &kind) {
  if (n >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF) return kind = 'j', true;
  if (n >= 8 && !memcmp(b, "\x89PNG\r\n\x1A\n", 8)) return kind = 'p', true;
  return false;
}

class EpubJob : public BookJob {
 public:
  EpubJob(FlowWriter &out, BookMeta &meta) : out_(out), meta_(meta), html_(out) {}
  ~EpubJob() override {
    free(buf_);
    if (coverFile_) fclose(coverFile_), remove(coverOut_.c_str());
  }

  bool start(const char *path, const char *coverPath, BookErr &err) {
    bool notZip = false;
    if (!pk_.zip.open(path, &notZip)) return err = notZip ? BE_FORMAT : BE_DAMAGED, false;
    if (encrypted(pk_.zip)) return err = BE_DRM, false;
    if (!readPackage(pk_, err, false)) return false;
    meta_.title = pk_.title, meta_.author = pk_.author, meta_.language = pk_.language, meta_.description = pk_.description;
    if (pk_.docs.empty()) return err = BE_NO_TEXT, false;
    readToc(pk_);
    buf_ = (uint8_t *)psMalloc(READ_BUF);
    if (!buf_) return err = BE_MEMORY, false;
    for (auto &d : pk_.docs) total_ += d.size;
    // The entries in the order of their chapter, then fragment: for the anchors' lookups.
    order_.resize(pk_.toc.size());
    for (size_t i = 0; i < order_.size(); i++) order_[i] = (uint16_t)i;
    const Pool *pool = &pk_.pool;
    const PsVec<Entry> *toc = &pk_.toc;
    std::stable_sort(order_.begin(), order_.end(), [pool, toc](uint16_t a, uint16_t b) {
      const Entry &x = (*toc)[a], &y = (*toc)[b];
      if (x.doc != y.doc) return x.doc < y.doc;
      return strcmp(pool->at(x.frag), pool->at(y.frag)) < 0;
    });
    html_.tocFromHeadings(pk_.toc.empty());  // no contents of its own: the headings
    html_.onAnchor(&EpubJob::anchorThunk, this);
    if (coverPath && pk_.coverPath.size()) startCover(coverPath);
    return true;
  }

  int step() override {
    if (coverFile_) return copyCover(), 0;
    if (doc_ >= pk_.docs.size()) return BOOK_DONE;
    if (!reading_) {
      if (!reader_.open(pk_.zip, pk_.docs[doc_].zip)) return -(int)BE_DAMAGED;
      reading_ = true;
      out_.pageBreak();
      // This chapter's entries: [from, to) of order_, the ones for its start first.
      from_ = firstOf(doc_), to_ = from_;
      while (to_ < order_.size() && pk_.toc[order_[to_]].doc == (int)doc_) to_++;
      for (size_t i = from_; i < to_ && !*pk_.pool.at(pk_.toc[order_[i]].frag); i++) emit(order_[i]);
    }
    for (size_t made = 0; made < SLICE;) {
      long n = reader_.read(buf_, READ_BUF);
      if (n < 0) return -(int)BE_DAMAGED;
      if (n == 0) {
        html_.finish();
        reading_ = false;
        done_ += pk_.docs[doc_].size;
        doc_++;
        break;
      }
      html_.feed((const char *)buf_, (size_t)n);
      made += (size_t)n;
    }
    uint64_t at = done_ + (reading_ ? html_.bytesFed() : 0);
    int p = total_ ? (int)(at * 999 / total_) : 0;
    return p > 999 ? 999 : p;
  }

 private:
  size_t firstOf(size_t doc) const {
    size_t lo = 0, hi = order_.size();
    while (lo < hi) {
      size_t mid = (lo + hi) / 2;
      if (pk_.toc[order_[mid]].doc < (int)doc) lo = mid + 1;
      else hi = mid;
    }
    return lo;
  }

  // An entry to the writer, now: it points at the block written next. Two at
  // the same place in one document are both kept (a part and its first
  // chapter); one left waiting by a document that had no text (a cover page)
  // is dropped for the next document's.
  void emit(uint16_t i) {
    Entry &e = pk_.toc[i];
    if (e.done) return;
    e.done = true;
    if (out_.offset() == lastTocAt_ && lastTocDoc_ != (int)doc_) out_.tocDropPending();
    out_.tocEntry(pk_.pool.at(e.title), e.level);
    lastTocAt_ = out_.offset(), lastTocDoc_ = (int)doc_;
  }

  static void anchorThunk(void *arg, const char *id, uint32_t) { ((EpubJob *)arg)->anchor(id); }
  void anchor(const char *id) {
    size_t lo = from_, hi = to_;
    while (lo < hi) {
      size_t mid = (lo + hi) / 2;
      if (strcmp(pk_.pool.at(pk_.toc[order_[mid]].frag), id) < 0) lo = mid + 1;
      else hi = mid;
    }
    for (; lo < to_ && !strcmp(pk_.pool.at(pk_.toc[order_[lo]].frag), id); lo++) emit(order_[lo]);
  }

  // The cover: a picture as it is, or the first picture of a cover page.
  void startCover(const char *coverPath) {
    std::string path = pk_.coverPath;
    int z = pk_.zip.find(path.c_str());
    if (z < 0) return;
    if (containsNoCase(path, ".htm") || containsNoCase(path, ".xhtml")) {
      PsVec<char> page;
      if (!zipReadAll(pk_.zip, z, page, COVER_PAGE_MAX)) return;
      const char *p = page.data(), *end = p + page.size() - 1;
      XmlTag t;
      std::string src;
      while (xmlNext(p, end, t))
        if (!t.close && ((is(t, "img") && attr(t, "src", src)) || (is(t, "image") && attr(t, "href", src)))) break;
      if (src.empty()) return;
      z = pk_.zip.find(resolve(folderOf(path), src.substr(0, src.find('#'))).c_str());
      if (z < 0) return;
    }
    if (!reader_.open(pk_.zip, z)) return;
    long n = reader_.read(buf_, READ_BUF);
    char kind;
    if (n <= 0 || !coverKind(buf_, (size_t)n, kind)) return;
    coverOut_ = coverPath;
    coverFile_ = fopen(coverPath, "wb");
    if (!coverFile_) return;
    kind_ = kind;
    if (fwrite(buf_, 1, (size_t)n, coverFile_) != (size_t)n) dropCover();
  }

  void copyCover() {
    for (size_t made = 0; made < SLICE;) {
      long n = reader_.read(buf_, READ_BUF);
      if (n == 0) {
        bool ok = fclose(coverFile_) == 0;
        coverFile_ = nullptr;
        if (ok) meta_.cover = kind_;
        else remove(coverOut_.c_str());
        return;
      }
      if (n < 0 || fwrite(buf_, 1, (size_t)n, coverFile_) != (size_t)n) return dropCover();  // a broken picture: no cover, the book still reads
      made += (size_t)n;
    }
  }

  void dropCover() {
    fclose(coverFile_);
    coverFile_ = nullptr;
    remove(coverOut_.c_str());
  }

  FlowWriter &out_;
  BookMeta &meta_;
  HtmlToFlow html_;
  Package pk_;
  ZipReader reader_;
  PsVec<uint16_t> order_;
  size_t from_ = 0, to_ = 0;
  uint8_t *buf_ = nullptr;
  size_t doc_ = 0;
  bool reading_ = false;
  uint64_t total_ = 0, done_ = 0;
  uint32_t lastTocAt_ = 0xFFFFFFFF;
  int lastTocDoc_ = -1;
  FILE *coverFile_ = nullptr;
  std::string coverOut_;
  char kind_ = 0;
};

}  // namespace

BookJob *epubJob(const char *path, FlowWriter &out, const char *coverPath, BookMeta &meta, BookErr &err) {
  EpubJob *job = new EpubJob(out, meta);
  if (!job->start(path, coverPath, err)) {
    delete job;
    if (err == BE_OK) err = BE_DAMAGED;
    return nullptr;
  }
  return job;
}

bool epubProbe(const char *path, BookMeta &meta, BookErr &err) {
  Package pk;
  bool notZip = false;
  if (!pk.zip.open(path, &notZip)) return err = notZip ? BE_FORMAT : BE_DAMAGED, false;
  if (encrypted(pk.zip)) return err = BE_DRM, false;
  if (!readPackage(pk, err, true)) return false;
  meta.title = pk.title, meta.author = pk.author, meta.language = pk.language, meta.description = pk.description;
  return true;
}
