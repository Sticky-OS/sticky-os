// Encodings and named characters: see bookcharset.h. The tables below were
// made from the Unicode mappings (Python's codecs and html.entities).
#include "bookcharset.h"

#include <ctype.h>
#include <string.h>

#include "psalloc.h"

namespace {

// The named characters of HTML 4 (and XML's apos), in strcmp order.
const struct { char name[9]; uint16_t cp; } ENTITIES[] = {
    {"AElig", 0xC6}, {"Aacute", 0xC1}, {"Acirc", 0xC2}, {"Agrave", 0xC0}, {"Alpha", 0x391}, {"Aring", 0xC5}, {"Atilde", 0xC3}, {"Auml", 0xC4},
    {"Beta", 0x392}, {"Ccedil", 0xC7}, {"Chi", 0x3A7}, {"Dagger", 0x2021}, {"Delta", 0x394}, {"ETH", 0xD0}, {"Eacute", 0xC9}, {"Ecirc", 0xCA},
    {"Egrave", 0xC8}, {"Epsilon", 0x395}, {"Eta", 0x397}, {"Euml", 0xCB}, {"Gamma", 0x393}, {"Iacute", 0xCD}, {"Icirc", 0xCE}, {"Igrave", 0xCC},
    {"Iota", 0x399}, {"Iuml", 0xCF}, {"Kappa", 0x39A}, {"Lambda", 0x39B}, {"Mu", 0x39C}, {"Ntilde", 0xD1}, {"Nu", 0x39D}, {"OElig", 0x152},
    {"Oacute", 0xD3}, {"Ocirc", 0xD4}, {"Ograve", 0xD2}, {"Omega", 0x3A9}, {"Omicron", 0x39F}, {"Oslash", 0xD8}, {"Otilde", 0xD5}, {"Ouml", 0xD6},
    {"Phi", 0x3A6}, {"Pi", 0x3A0}, {"Prime", 0x2033}, {"Psi", 0x3A8}, {"Rho", 0x3A1}, {"Scaron", 0x160}, {"Sigma", 0x3A3}, {"THORN", 0xDE},
    {"Tau", 0x3A4}, {"Theta", 0x398}, {"Uacute", 0xDA}, {"Ucirc", 0xDB}, {"Ugrave", 0xD9}, {"Upsilon", 0x3A5}, {"Uuml", 0xDC}, {"Xi", 0x39E},
    {"Yacute", 0xDD}, {"Yuml", 0x178}, {"Zeta", 0x396}, {"aacute", 0xE1}, {"acirc", 0xE2}, {"acute", 0xB4}, {"aelig", 0xE6}, {"agrave", 0xE0},
    {"alefsym", 0x2135}, {"alpha", 0x3B1}, {"amp", 0x26}, {"and", 0x2227}, {"ang", 0x2220}, {"apos", 0x27}, {"aring", 0xE5}, {"asymp", 0x2248},
    {"atilde", 0xE3}, {"auml", 0xE4}, {"bdquo", 0x201E}, {"beta", 0x3B2}, {"brvbar", 0xA6}, {"bull", 0x2022}, {"cap", 0x2229}, {"ccedil", 0xE7},
    {"cedil", 0xB8}, {"cent", 0xA2}, {"chi", 0x3C7}, {"circ", 0x2C6}, {"clubs", 0x2663}, {"cong", 0x2245}, {"copy", 0xA9}, {"crarr", 0x21B5},
    {"cup", 0x222A}, {"curren", 0xA4}, {"dArr", 0x21D3}, {"dagger", 0x2020}, {"darr", 0x2193}, {"deg", 0xB0}, {"delta", 0x3B4}, {"diams", 0x2666},
    {"divide", 0xF7}, {"eacute", 0xE9}, {"ecirc", 0xEA}, {"egrave", 0xE8}, {"empty", 0x2205}, {"emsp", 0x2003}, {"ensp", 0x2002}, {"epsilon", 0x3B5},
    {"equiv", 0x2261}, {"eta", 0x3B7}, {"eth", 0xF0}, {"euml", 0xEB}, {"euro", 0x20AC}, {"exist", 0x2203}, {"fnof", 0x192}, {"forall", 0x2200},
    {"frac12", 0xBD}, {"frac14", 0xBC}, {"frac34", 0xBE}, {"frasl", 0x2044}, {"gamma", 0x3B3}, {"ge", 0x2265}, {"gt", 0x3E}, {"hArr", 0x21D4},
    {"harr", 0x2194}, {"hearts", 0x2665}, {"hellip", 0x2026}, {"iacute", 0xED}, {"icirc", 0xEE}, {"iexcl", 0xA1}, {"igrave", 0xEC},
    {"image", 0x2111}, {"infin", 0x221E}, {"int", 0x222B}, {"iota", 0x3B9}, {"iquest", 0xBF}, {"isin", 0x2208}, {"iuml", 0xEF}, {"kappa", 0x3BA},
    {"lArr", 0x21D0}, {"lambda", 0x3BB}, {"lang", 0x2329}, {"laquo", 0xAB}, {"larr", 0x2190}, {"lceil", 0x2308}, {"ldquo", 0x201C}, {"le", 0x2264},
    {"lfloor", 0x230A}, {"lowast", 0x2217}, {"loz", 0x25CA}, {"lrm", 0x200E}, {"lsaquo", 0x2039}, {"lsquo", 0x2018}, {"lt", 0x3C}, {"macr", 0xAF},
    {"mdash", 0x2014}, {"micro", 0xB5}, {"middot", 0xB7}, {"minus", 0x2212}, {"mu", 0x3BC}, {"nabla", 0x2207}, {"nbsp", 0xA0}, {"ndash", 0x2013},
    {"ne", 0x2260}, {"ni", 0x220B}, {"not", 0xAC}, {"notin", 0x2209}, {"nsub", 0x2284}, {"ntilde", 0xF1}, {"nu", 0x3BD}, {"oacute", 0xF3},
    {"ocirc", 0xF4}, {"oelig", 0x153}, {"ograve", 0xF2}, {"oline", 0x203E}, {"omega", 0x3C9}, {"omicron", 0x3BF}, {"oplus", 0x2295}, {"or", 0x2228},
    {"ordf", 0xAA}, {"ordm", 0xBA}, {"oslash", 0xF8}, {"otilde", 0xF5}, {"otimes", 0x2297}, {"ouml", 0xF6}, {"para", 0xB6}, {"part", 0x2202},
    {"permil", 0x2030}, {"perp", 0x22A5}, {"phi", 0x3C6}, {"pi", 0x3C0}, {"piv", 0x3D6}, {"plusmn", 0xB1}, {"pound", 0xA3}, {"prime", 0x2032},
    {"prod", 0x220F}, {"prop", 0x221D}, {"psi", 0x3C8}, {"quot", 0x22}, {"rArr", 0x21D2}, {"radic", 0x221A}, {"rang", 0x232A}, {"raquo", 0xBB},
    {"rarr", 0x2192}, {"rceil", 0x2309}, {"rdquo", 0x201D}, {"real", 0x211C}, {"reg", 0xAE}, {"rfloor", 0x230B}, {"rho", 0x3C1}, {"rlm", 0x200F},
    {"rsaquo", 0x203A}, {"rsquo", 0x2019}, {"sbquo", 0x201A}, {"scaron", 0x161}, {"sdot", 0x22C5}, {"sect", 0xA7}, {"shy", 0xAD}, {"sigma", 0x3C3},
    {"sigmaf", 0x3C2}, {"sim", 0x223C}, {"spades", 0x2660}, {"sub", 0x2282}, {"sube", 0x2286}, {"sum", 0x2211}, {"sup", 0x2283}, {"sup1", 0xB9},
    {"sup2", 0xB2}, {"sup3", 0xB3}, {"supe", 0x2287}, {"szlig", 0xDF}, {"tau", 0x3C4}, {"there4", 0x2234}, {"theta", 0x3B8}, {"thetasym", 0x3D1},
    {"thinsp", 0x2009}, {"thorn", 0xFE}, {"tilde", 0x2DC}, {"times", 0xD7}, {"trade", 0x2122}, {"uArr", 0x21D1}, {"uacute", 0xFA}, {"uarr", 0x2191},
    {"ucirc", 0xFB}, {"ugrave", 0xF9}, {"uml", 0xA8}, {"upsih", 0x3D2}, {"upsilon", 0x3C5}, {"uuml", 0xFC}, {"weierp", 0x2118}, {"xi", 0x3BE},
    {"yacute", 0xFD}, {"yen", 0xA5}, {"yuml", 0xFF}, {"zeta", 0x3B6}, {"zwj", 0x200D}, {"zwnj", 0x200C},
};
const uint16_t CP1252_80[32] = {  // windows-1252's 0x80..0x9F; the rest is Latin-1
    0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD, 0xFFFD, 0x2018,
    0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178,
};
const uint16_t CP1250_80[128] = {  // windows-1250 (Central European)
    0x20AC, 0xFFFD, 0x201A, 0xFFFD, 0x201E, 0x2026, 0x2020, 0x2021, 0xFFFD, 0x2030, 0x0160, 0x2039, 0x015A, 0x0164, 0x017D, 0x0179, 0xFFFD, 0x2018,
    0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0xFFFD, 0x2122, 0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A, 0x00A0, 0x02C7, 0x02D8, 0x0141,
    0x00A4, 0x0104, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B, 0x00B0, 0x00B1, 0x02DB, 0x0142, 0x00B4, 0x00B5,
    0x00B6, 0x00B7, 0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C, 0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
    0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E, 0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7, 0x0158, 0x016E,
    0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF, 0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7, 0x010D, 0x00E9, 0x0119, 0x00EB,
    0x011B, 0x00ED, 0x00EE, 0x010F, 0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7, 0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD,
    0x0163, 0x02D9,
};
const uint16_t CP1251_80[128] = {  // windows-1251 (Cyrillic)
    0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F, 0x0452, 0x2018,
    0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0xFFFD, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F, 0x00A0, 0x040E, 0x045E, 0x0408,
    0x00A4, 0x0490, 0x00A6, 0x00A7, 0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407, 0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5,
    0x00B6, 0x00B7, 0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457, 0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429,
    0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F, 0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043A, 0x043B,
    0x043C, 0x043D, 0x043E, 0x043F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D,
    0x044E, 0x044F,
};
const uint16_t ISO8859_2_A0[96] = {  // iso-8859-2's 0xA0..0xFF
    0x00A0, 0x0104, 0x02D8, 0x0141, 0x00A4, 0x013D, 0x015A, 0x00A7, 0x00A8, 0x0160, 0x015E, 0x0164, 0x0179, 0x00AD, 0x017D, 0x017B, 0x00B0, 0x0105,
    0x02DB, 0x0142, 0x00B4, 0x013E, 0x015B, 0x02C7, 0x00B8, 0x0161, 0x015F, 0x0165, 0x017A, 0x02DD, 0x017E, 0x017C, 0x0154, 0x00C1, 0x00C2, 0x0102,
    0x00C4, 0x0139, 0x0106, 0x00C7, 0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E, 0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150,
    0x00D6, 0x00D7, 0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF, 0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
    0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F, 0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7, 0x0159, 0x016F,
    0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9,
};
const uint16_t KOI8R_80[128] = {  // koi8-r
    0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524, 0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590, 0x2591, 0x2592,
    0x2593, 0x2320, 0x25A0, 0x2219, 0x221A, 0x2248, 0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0, 0x00B2, 0x00B7, 0x00F7, 0x2550, 0x2551, 0x2552, 0x0451,
    0x2553, 0x2554, 0x2555, 0x2556, 0x2557, 0x2558, 0x2559, 0x255A, 0x255B, 0x255C, 0x255D, 0x255E, 0x255F, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563,
    0x2564, 0x2565, 0x2566, 0x2567, 0x2568, 0x2569, 0x256A, 0x256B, 0x256C, 0x00A9, 0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
    0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044C, 0x044B,
    0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A, 0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041A,
    0x041B, 0x041C, 0x041D, 0x041E, 0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429,
    0x0427, 0x042A,
};

bool nameIs(const char *name, size_t n, const char *want) {
  // Letters and digits only, any case: "ISO_8859-1" is "iso88591".
  size_t j = 0;
  for (size_t i = 0; i < n; i++) {
    unsigned char c = name[i];
    if (!isalnum(c)) continue;
    if (!want[j] || tolower(c) != want[j]) return false;
    j++;
  }
  return !want[j];
}

}  // namespace

bool charsetByName(const char *name, size_t n, Charset &cs) {
  static const struct { const char *name; Charset cs; } NAMES[] = {
      {"utf8", CS_UTF8},        {"usascii", CS_UTF8},     {"ascii", CS_UTF8},
      {"windows1252", CS_1252}, {"cp1252", CS_1252},      {"iso88591", CS_1252},   {"latin1", CS_1252}, {"iso885915", CS_1252},
      {"windows1250", CS_1250}, {"cp1250", CS_1250},      {"iso88592", CS_8859_2}, {"latin2", CS_8859_2},
      {"windows1251", CS_1251}, {"cp1251", CS_1251},      {"koi8r", CS_KOI8R},     {"koi8u", CS_KOI8R},
      {"utf16le", CS_UTF16LE},  {"utf16", CS_UTF16LE},    {"utf16be", CS_UTF16BE}, {"ucs2", CS_UTF16LE},
  };
  for (auto &e : NAMES)
    if (nameIs(name, n, e.name)) return cs = e.cs, true;
  return false;
}

uint32_t charsetHigh(Charset cs, uint8_t b) {
  if (b < 0x80) return b;
  switch (cs) {
    case CS_1250: return CP1250_80[b - 0x80];
    case CS_1251: return CP1251_80[b - 0x80];
    case CS_KOI8R: return KOI8R_80[b - 0x80];
    case CS_8859_2: return b < 0xA0 ? 0xFFFD : ISO8859_2_A0[b - 0xA0];
    default: return b < 0xA0 ? CP1252_80[b - 0x80] : b;
  }
}

int utf8Encode(uint32_t cp, char out[4]) {
  if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = 0xFFFD;
  if (cp < 0x80) return out[0] = (char)cp, 1;
  if (cp < 0x800) return out[0] = (char)(0xC0 | cp >> 6), out[1] = (char)(0x80 | (cp & 0x3F)), 2;
  if (cp < 0x10000) return out[0] = (char)(0xE0 | cp >> 12), out[1] = (char)(0x80 | (cp >> 6 & 0x3F)), out[2] = (char)(0x80 | (cp & 0x3F)), 3;
  return out[0] = (char)(0xF0 | cp >> 18), out[1] = (char)(0x80 | (cp >> 12 & 0x3F)), out[2] = (char)(0x80 | (cp >> 6 & 0x3F)), out[3] = (char)(0x80 | (cp & 0x3F)), 4;
}

void utf8Append(std::string &s, uint32_t cp) {
  char b[4];
  s.append(b, utf8Encode(cp, b));
}

bool utf8Valid(const uint8_t *p, size_t n, size_t *highs) {
  size_t count = 0;
  for (size_t i = 0; i < n;) {
    uint8_t c = p[i];
    if (c < 0x80) {
      i++;
      continue;
    }
    int need = c >= 0xC2 && c <= 0xDF ? 1 : c >= 0xE0 && c <= 0xEF ? 2 : c >= 0xF0 && c <= 0xF4 ? 3 : 0;
    if (!need) return false;
    for (int k = 1; k <= need; k++) {
      if (i + k >= n) {  // cut off by the end
        if (highs) *highs = count;
        return true;
      }
      if ((p[i + k] & 0xC0) != 0x80) return false;
    }
    if ((c == 0xE0 && p[i + 1] < 0xA0) || (c == 0xED && p[i + 1] > 0x9F) || (c == 0xF0 && p[i + 1] < 0x90) || (c == 0xF4 && p[i + 1] > 0x8F)) return false;
    i += need + 1;
    count++;
  }
  if (highs) *highs = count;
  return true;
}

void CharsetDecoder::feed(const char *p, size_t n, std::string &out) {
  if (cs_ == CS_UTF8) {
    out.append(p, n);
    return;
  }
  if (cs_ != CS_UTF16LE && cs_ != CS_UTF16BE) {
    for (size_t i = 0; i < n; i++) {
      uint8_t b = p[i];
      if (b < 0x80) out += (char)b;
      else utf8Append(out, charsetHigh(cs_, b));
    }
    return;
  }
  for (size_t i = 0; i < n; i++) {
    uint8_t b = p[i];
    if (!have_) {
      first_ = b, have_ = true;
      continue;
    }
    have_ = false;
    uint32_t u = cs_ == CS_UTF16LE ? (uint32_t)(first_ | b << 8) : (uint32_t)(first_ << 8 | b);
    if (hi_) {
      if (u >= 0xDC00 && u <= 0xDFFF) {
        utf8Append(out, 0x10000 + ((hi_ - 0xD800) << 10) + (u - 0xDC00));
        hi_ = 0;
        continue;
      }
      utf8Append(out, 0xFFFD);  // a high half with no low one
      hi_ = 0;
    }
    if (u >= 0xD800 && u <= 0xDBFF) hi_ = u;
    else utf8Append(out, u);
  }
}

uint32_t htmlEntity(const char *name, size_t n) {
  if (!n || n > 8) return 0;
  int lo = 0, hi = (int)(sizeof ENTITIES / sizeof ENTITIES[0]) - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    int c = strncmp(name, ENTITIES[mid].name, n);
    if (!c && ENTITIES[mid].name[n]) c = -1;  // name is only its start
    if (!c) return ENTITIES[mid].cp;
    if (c < 0) hi = mid - 1;
    else lo = mid + 1;
  }
  return 0;
}

namespace {

// decodeEntities, for any kind of string (the description's PSRAM ones too).
template <class S>
void decodeIn(S &s) {
  size_t amp = s.find('&');
  if (amp == S::npos) return;
  S out(s, 0, amp);
  for (size_t i = amp; i < s.size();) {
    if (s[i] != '&') {
      out += s[i++];
      continue;
    }
    size_t end = i + 1;
    while (end < s.size() && end - i <= 10 && s[end] != ';' && (isalnum((unsigned char)s[end]) || s[end] == '#')) end++;
    uint32_t cp = 0;
    if (end < s.size() && s[end] == ';' && end > i + 1) {
      if (s[i + 1] == '#') {
        bool hex = end > i + 2 && (s[i + 2] == 'x' || s[i + 2] == 'X'), ok = end > i + (hex ? 3 : 2);
        for (size_t k = i + (hex ? 3 : 2); k < end && ok; k++) {
          unsigned char c = s[k];
          if (hex ? !isxdigit(c) : !isdigit(c)) ok = false;
          else cp = hex ? cp * 16 + (isdigit(c) ? c - '0' : (tolower(c) - 'a' + 10)) : cp * 10 + (c - '0');
          if (cp > 0x10FFFF) cp = 0xFFFD;
        }
        if (!ok) cp = 0;
        else if (!cp) cp = 0xFFFD;
      } else {
        cp = htmlEntity(s.data() + i + 1, end - i - 1);
      }
    }
    char b[4];
    if (cp) out.append(b, (size_t)utf8Encode(cp, b)), i = end + 1;
    else out += s[i++];
  }
  s.swap(out);
}

}  // namespace

void decodeEntities(std::string &s) { decodeIn(s); }

namespace {

// Does this tag (its inside, without '<' and '>') part paragraphs? raw:
// it is <style> or <script>, whose contents are no text.
bool tagParts(const char *t, size_t n, bool &raw) {
  static const char *const PARTS[] = {"p", "div", "br", "li", "ul", "ol", "h1", "h2", "h3", "h4", "h5", "h6", "blockquote", "tr", "dd", "dt", "section", "hr"};
  size_t i = t[0] == '/' ? 1 : 0, k = i;
  char name[12];
  size_t len = 0;
  for (; k < n && (isalnum((unsigned char)t[k]) || t[k] == ':'); k++) {
    if (t[k] == ':') len = 0;
    else if (len < sizeof name - 1) name[len++] = (char)tolower((unsigned char)t[k]);
  }
  name[len] = 0;
  raw = !i && (!strcmp(name, "style") || !strcmp(name, "script"));
  for (const char *p : PARTS)
    if (!strcmp(name, p)) return true;
  return false;
}

}  // namespace

void bookCleanDescription(const char *in, size_t n, std::string &out, size_t max) {
  typedef std::basic_string<char, std::char_traits<char>, PsAlloc<char>> PsString;  // in PSRAM: it may be long
  out.clear();
  if (n > 64 * max) n = 64 * max;  // far more than can be kept, whatever the markup
  PsString s(in, n);
  bool markup = memchr(in, '<', n) != nullptr;
  if (!markup && (s.find("&lt;") != PsString::npos || s.find("&#60;") != PsString::npos)) {
    decodeIn(s);  // escaped HTML: the markup is under one layer of entities
    markup = true;
  }
  bool space = false, para = false;
  auto put = [&](const char *p, size_t len) {
    if (out.size() > max) return;  // full: the rest is cut anyway
    if (para && out.size()) out += '\n';
    else if (space && out.size()) out += ' ';
    para = space = false;
    out.append(p, len);
  };
  PsString run;  // text between tags, entities still in it
  auto flushRun = [&]() {
    decodeIn(run);
    for (size_t i = 0; i < run.size();) {
      unsigned char c = run[i];
      if (c == '\n' || c == '\r') {
        if (markup) space = true;
        else para = true;
        i++;
        continue;
      }
      if (c < 0x20 || c == ' ' || c == 0x7F) {
        space = true;
        i++;
        continue;
      }
      int need = c < 0x80 ? 0 : c >= 0xC2 && c <= 0xDF ? 1 : c >= 0xE0 && c <= 0xEF ? 2 : c >= 0xF0 && c <= 0xF4 ? 3 : -1;
      bool ok = need >= 0 && i + need < run.size() && utf8Valid((const uint8_t *)run.data() + i, (size_t)need + 1);
      if (!ok) {
        i++;  // a broken byte: dropped
        continue;
      }
      if (c == 0xC2 && (uint8_t)run[i + 1] == 0xA0) space = true;  // a no-break space: a space here
      else put(run.data() + i, (size_t)need + 1);
      i += (size_t)need + 1;
    }
    run.clear();
  };
  for (size_t i = 0; i < s.size(); i++) {
    bool tag = s[i] == '<' && markup && i + 1 < s.size() && (isalpha((unsigned char)s[i + 1]) || s[i + 1] == '/' || s[i + 1] == '!' || s[i + 1] == '?');
    if (!tag) {
      run += s[i];
      continue;
    }
    size_t e = s.find('>', i);
    if (e == PsString::npos) break;  // a tag cut off: the rest is not text
    flushRun();
    bool raw;
    if (s.compare(i, 4, "<!--") == 0) {
      size_t c = s.find("-->", i + 4);
      i = c == PsString::npos ? s.size() : c + 2;
      continue;
    }
    if (tagParts(s.data() + i + 1, e - i - 1, raw)) para = true;
    if (raw) {
      // <style> and <script>: their contents aren't text.
      size_t close = s.find("</", e);
      e = close == PsString::npos ? s.size() - 1 : s.find('>', close);
      if (e == PsString::npos) e = s.size() - 1;
    }
    i = e;
  }
  flushRun();
  if (out.size() > max) {
    size_t cut = max;
    while (cut > 0 && ((uint8_t)out[cut] & 0xC0) == 0x80) cut--;
    out.resize(cut);
    while (out.size() && (out.back() == ' ' || out.back() == '\n')) out.pop_back();
  }
}
