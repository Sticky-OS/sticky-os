// HtmlToFlow (bookformats.h): blocks, styles, entities, charsets, anchors,
// headings as contents, and markup cut into pieces anywhere.
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "bookformats.h"
#include "flowcheck.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

struct Anchor { std::string id; uint32_t at; };
static void onAnchor(void *arg, const char *id, uint32_t at) { ((std::vector<Anchor> *)arg)->push_back({id, at}); }

struct Out { std::string flow, toc, title; std::vector<Anchor> anchors; };

// Converts html fed in pieces of `step` bytes.
static Out convert(const std::string &html, size_t step = 1 << 20, bool toc = false, const char *cs = nullptr) {
  Out o;
  std::string path = "build/html_test.flow";
  FlowWriter w;
  CHECK(w.open(path.c_str()));
  {
    HtmlToFlow h(w);
    if (cs) h.setCharset(cs);
    h.tocFromHeadings(toc);
    h.onAnchor(onAnchor, &o.anchors);
    for (size_t i = 0; i < html.size(); i += step) h.feed(html.data() + i, html.size() - i < step ? html.size() - i : step);
    h.finish();
    CHECK(h.bytesFed() == 0 || h.bytesFed() == html.size());
    o.title = h.title();
  }
  CHECK(w.close());
  o.flow = flowSlurp(path), o.toc = flowSlurp(path + ".toc");
  std::string why;
  if (!flowValidate(o.flow, &why)) printf("invalid flow: %s\n%s\n", why.c_str(), flowDump(o.flow).c_str()), fails++;
  if (!tocValidate(o.toc, o.flow, &why)) printf("invalid toc: %s\n", why.c_str()), fails++;
  return o;
}

// The same result however the markup is cut.
static Out same(const std::string &html, bool toc = false, const char *cs = nullptr) {
  Out whole = convert(html, 1 << 20, toc, cs);
  for (size_t step : {1, 2, 3, 5, 7, 13}) {
    Out part = convert(html, step, toc, cs);
    if (part.flow != whole.flow || part.toc != whole.toc || part.title != whole.title || part.anchors.size() != whole.anchors.size()) {
      printf("FAIL pieces of %zu differ:\n%s\n--- vs ---\n%s\n", step, flowDump(part.flow).c_str(), flowDump(whole.flow).c_str());
      fails++;
      break;
    }
  }
  return whole;
}

static void expect(const char *line, const std::string &html, const std::string &want, bool toc = false) {
  Out o = same(html, toc);
  if (o.flow != want) {
    printf("FAIL %s:\n got: %s\nwant: %s\n", line, flowDump(o.flow).c_str(), flowDump(want).c_str());
    fails++;
  }
}
#define EXPECT(...) expect(#__VA_ARGS__, __VA_ARGS__)

int main() {
  // The head is left out, its title kept; style and script too.
  {
    Out o = same("<?xml version='1.0' encoding='utf-8'?><!DOCTYPE html [ <!ENTITY x 'y'> ]><html><head><title>The\n  Book</title>"
                 "<style>p { color: red } </p></style><script>if (a<b) document.write('<p>no</p>')</script></head>"
                 "<body><!-- a <p>comment</p> --><p>Hello  <i>wide</i>\n world.</p></body></html>");
    CHECK(o.title == "The Book");
    CHECK(o.flow == "Hello \x0Ewide\x0F world.\n");
  }
  EXPECT("<h1>One</h1><h2>Two</h2><h3>Three</h3><h5>Five</h5><p>x</p>", "\x01One\n\x02Two\n\x03Three\n\x03" "Five\nx\n");
  EXPECT("<blockquote><p>a</p><p>b</p></blockquote><p>c</p>", "\x04" "a\n\x04" "b\nc\n");
  EXPECT("<ol><li>a</li><li>b<ul><li>c</li></ul></li><li>d</li></ol><ul><li>e</li></ul>", "\x06" "1. a\n\x06" "2. b\n\x06" "c\n\x06" "3. d\n\x06" "e\n");
  EXPECT("<ol start=\"4\"><li>a</li></ol>", "\x06" "4. a\n");
  EXPECT("<p>a</p><hr/><p>b<br/>c<br></p><hr><hr>", "a\n\x07\nb\x0B" "c\n\x07\n");
  EXPECT("<pre/><p>a\nb</p><p/><i/>c", "a b\nc\n");
  EXPECT("<pre>line one\n  line two\n\nend</pre>", "line one\x0Bline two\x0B\x0B" "end\n");
  EXPECT("<table><tr><td>a</td><td>b</td></tr><tr><th>c</th></tr></table>", "a b\nc\n");
  EXPECT("<dl><dt>term</dt><dd>meaning</dd></dl>", "term\nmeaning\n");
  EXPECT("<figure><img src='x.png' alt='x.png'/><img alt=''/><img alt='A map'/><figcaption>The map</figcaption></figure><p><img alt='T'/>he <img alt='[Fig. 2]'/></p><h2><img alt='Preface'/></h2>", "[A map]\n\x05The map\nThe [Fig. 2]\n\x02Preface\n");
  EXPECT("<center>a</center><p align='center'>b</p><p style='text-align: center'>c</p><div class='txt-centre'>d</div><p>e</p>", "\x05" "a\n\x05" "b\n\x05" "c\n\x05" "d\ne\n");
  EXPECT("<p>a<mbp:pagebreak/>b</p>", "a\n\x0C" "b\n");
  EXPECT("<p><b>bold <i>both</i></b> <em>it <strong>x</strong></em> <cite>c</cite> <span style='font-style:italic'>s</span></p>",
         "\x10" "bold \x0E" "both\x11 it \x10x\x11 c s\x0F\n");  // the writer joins italic across the spaces
  EXPECT("<p>x<sup>2</sup> H<sub>2</sub>O <a href='#n'>link</a></p>", "x2 H2O link\n");
  EXPECT("<p>&amp; &lt;&gt; &nbsp;&mdash;&ndash;&hellip;&ldquo;&rdquo;&eacute;&Eacute;&alpha;&#233;&#xE9;&#x1F600;&#151; &bogus; &amp &</p>",
         "& <> \xC2\xA0\xE2\x80\x94\xE2\x80\x93\xE2\x80\xA6\xE2\x80\x9C\xE2\x80\x9D\xC3\xA9\xC3\x89\xCE\xB1\xC3\xA9\xC3\xA9\xF0\x9F\x98\x80\xE2\x80\x94 &bogus; & &\n");
  EXPECT("<p>a < b</p><p><![CDATA[x <y> ]] z]]></p>", "a < b\nx <y> ]] z\n");
  EXPECT("<svg><text>no</text><svg/></svg><math><mi>x</mi></math><div hidden>h</div><p style='display:none'>n</p><p>yes</p>", "yes\n");
  EXPECT("<p>a<br><br>b</p>", "a\nb\n");
  EXPECT("<p>&nbsp;</p><p> </p><p>q</p>", "q\n");
  EXPECT("<p>\xC3\xA9t\xC3\xA9 \xE2\x80\x94 \xF0\x9F\x98\x80</p>", "\xC3\xA9t\xC3\xA9 \xE2\x80\x94 \xF0\x9F\x98\x80\n");
  EXPECT("<P CLASS=x>Upper<BR>case</P>", "Upper\x0B" "case\n");
  EXPECT("<div><div><div>deep</div>x</div>y</div>z", "deep\nx\ny\nz\n");
  EXPECT("<p>unclosed <i>it<p>next", "unclosed \x0Eit\x0F\n\x0Enext\x0F\n");  // italic spans paragraphs, as in a browser
  EXPECT("<p>q: <q>yes</q></p>", "q: \xE2\x80\x9Cyes\xE2\x80\x9D\n");

  // Charsets: declared, by default, UTF-16 by its mark or its zeros.
  {
    std::string latin = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><p>caf\xE9 \x93q\x94</p>";
    CHECK(same(latin).flow == "caf\xC3\xA9 \xE2\x80\x9Cq\xE2\x80\x9D\n");
    std::string meta = "<html><head><meta http-equiv='Content-Type' content='text/html; charset=windows-1250'></head><p>\xB9\xE6</p>";
    CHECK(same(meta).flow == "\xC4\x85\xC4\x87\n");
    CHECK(same("<meta charset=\"windows-1251\"><p>\xCF\xF0\xE8</p>").flow == "\xD0\x9F\xD1\x80\xD0\xB8\n");
    CHECK(same("<p>\xE9</p>", false, "windows-1252").flow == "\xC3\xA9\n");
    CHECK(same("\xEF\xBB\xBF<meta charset='iso-8859-1'><p>\xC3\xA9</p>").flow == "\xC3\xA9\n");  // the mark wins
    std::string u8 = "<p>\xC3\xA9\xF0\x9F\x98\x80 &amp;</p>", le = "\xFF\xFE", be = "\xFE\xFF", le0, be0;
    // UTF-16 of u8, by hand: é is 00E9, the emoji D83D DE00.
    const uint16_t units[] = {'<', 'p', '>', 0xE9, 0xD83D, 0xDE00, ' ', '&', 'a', 'm', 'p', ';', '<', '/', 'p', '>'};
    for (uint16_t u : units) le0 += (char)(u & 0xFF), le0 += (char)(u >> 8), be0 += (char)(u >> 8), be0 += (char)(u & 0xFF);
    CHECK(same(le + le0).flow == "\xC3\xA9\xF0\x9F\x98\x80 &\n");
    CHECK(same(be + be0).flow == "\xC3\xA9\xF0\x9F\x98\x80 &\n");
    CHECK(same(le0).flow == "\xC3\xA9\xF0\x9F\x98\x80 &\n");
    CHECK(same(be0).flow == "\xC3\xA9\xF0\x9F\x98\x80 &\n");
  }

  // Anchors: between blocks, the next block's offset; inside one, its own.
  {
    Out o = same("<div id='top'><p>first</p></div><p id='p2'>second <span id='in'>x</span></p><a name='n3'></a><h2 id='h'>Head</h2><p>t<a id='end'/></p>");
    CHECK(o.anchors.size() == 6);
    size_t second = o.flow.find("second"), head = o.flow.find("\x02Head"), t = o.flow.find("\nt\n") + 1;
    if (o.anchors.size() == 6) {
      CHECK(o.anchors[0].id == "top" && o.anchors[0].at == 0);
      CHECK(o.anchors[1].id == "p2" && o.anchors[1].at == second);
      CHECK(o.anchors[2].id == "in" && o.anchors[2].at == second);
      CHECK(o.anchors[3].id == "n3" && o.anchors[3].at == head);
      CHECK(o.anchors[4].id == "h" && o.anchors[4].at == head);
      CHECK(o.anchors[5].id == "end" && o.anchors[5].at == t);
    }
  }
  // Headings as the contents.
  {
    Out o = same("<h1>Part <i>One</i></h1><p>a</p><h2>Chapter&nbsp;1<br/>The Start</h2><p>b</p><h4>small</h4><h3></h3><p>c</p>", true);
    CHECK(o.flow == "\x01Part \x0EOne\x0F\na\n\x02" "Chapter\xC2\xA0" "1\x0BThe Start\nb\n\x03small\nc\n");
    CHECK(o.toc == "0\t1\tPart One\n" + std::to_string(o.flow.find("\x02")) + "\t2\tChapter\xC2\xA0" "1 The Start\n");
  }
  // finish() resets: a second document starts plain.
  {
    std::string path = "build/html_test2.flow";
    FlowWriter w;
    CHECK(w.open(path.c_str()));
    HtmlToFlow h(w);
    h.feed("<blockquote><p><i>open", 22);
    h.finish();
    h.feed("<p>plain</p>", 12);
    h.finish();
    CHECK(w.close());
    CHECK(flowSlurp(path) == "\x04\x0Eopen\x0F\nplain\n");
  }
  // Hostile: deep nesting, huge tags, endless entities, random bytes.
  {
    std::string deep;
    for (int i = 0; i < 5000; i++) deep += "<blockquote><span><div>";
    deep += "x";
    for (int i = 0; i < 5000; i++) deep += "</div></span></blockquote>";
    deep += "<p>after</p>";
    Out o = convert(deep);
    CHECK(o.flow == "\x04x\nafter\n");
    std::string big = "<img alt='" + std::string(200000, 'a') + "'/><p title='" + std::string(100000, '>') + "'>ok</p>&" + std::string(1000, 'a');
    convert(big);
    uint32_t x = 7;
    const char *bits[] = {"<", ">", "&", ";", "</", "<p>", "<i>", "</i>", "<!--", "-->", "<![CDATA[", "]]>", "<?xml encoding='utf-16'?>", "<meta charset='cp1251'>",
                          "<pre>", "<h1>", "</h1>", "<li>", "<ol>", "<svg>", "</svg>", "'", "\"", "a", " ", "\n", "\xC3", "\xA9", "\xFF", "\x00", "&#x", "&#99999999;", "<script>", "</script>", "<style>", "<br>"};
    for (int k = 0; k < 400; k++) {
      std::string junk;
      int len = (int)((x = x * 1103515245u + 12345u) >> 16) % 200;
      for (int j = 0; j < len; j++) {
        x = x * 1103515245u + 12345u;
        const char *b = bits[(x >> 16) % (sizeof bits / sizeof bits[0])];
        junk.append(b, *b ? strlen(b) : 1);
      }
      Out a = convert(junk, 1 << 20, k & 1), b = convert(junk, 1 + k % 5, k & 1);
      if (a.flow != b.flow) printf("FAIL junk %d differs by pieces\n", k), fails++;
    }
  }
  printf("html: %s\n", fails ? "FAILED" : "ok");
  return fails ? 1 : 0;
}
