// bookCleanDescription (bookcharset.h): a book's description from HTML,
// escaped HTML or plain text.
#include <stdio.h>
#include <string.h>
#include <string>

#include "bookcharset.h"

static int fails;

static void expect(const char *in, const char *want, size_t max = 2000) {
  std::string out;
  bookCleanDescription(in, strlen(in), out, max);
  if (out != want) printf("FAIL %s\n got: [%s]\nwant: [%s]\n", in, out.c_str(), want), fails++;
}

int main() {
  expect("", "");
  expect("  A plain   description. ", "A plain description.");
  expect("Line one\r\nline two\n\n\nthree", "Line one\nline two\nthree");
  expect("<p>First <i>para</i>.</p>\n<p>Second&nbsp;one &amp; more&hellip;</p>", "First para.\nSecond one & more\xE2\x80\xA6");
  expect("&lt;p&gt;Escaped &amp;amp; &lt;b&gt;bold&lt;/b&gt;&lt;/p&gt;&lt;p&gt;Two&lt;/p&gt;", "Escaped & bold\nTwo");
  expect("<div><style>p{x}</style><script>a<b</script>Text<br/>break<!-- <p>no</p> --></div>", "Text\nbreak");
  expect("Caf\xC3\xA9 \xFF\xC3 ok", "Caf\xC3\xA9 ok");  // broken UTF-8 dropped
  expect("a < b, but &lt; is fine", "a < b, but < is fine");
  expect("<ul><li>one</li><li>two</li></ul>", "one\ntwo");
  expect("<p>cut <b", "cut");
  expect("\xC3\xA9\xC3\xA9\xC3\xA9", "\xC3\xA9", 3);  // cut on a character
  expect("one two three", "one two", 8);
  printf("desc: %s\n", fails ? "FAILED" : "ok");
  return fails ? 1 : 0;
}
