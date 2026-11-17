// ---------------------------------------------------------------- Tutor
//
// A school tutor, class by class and subject by subject. It starts with 7th grade maths (geometry,
// units 4 and 7 of the Romanian textbook); other subjects (chemistry, geography...) and classes are
// meant to be added the same way: a class menu, a subject menu, then lessons. Each lesson has a summary and worked
// examples, then practice that is made from numbers and checked before it is
// shown (tutor7_core.cpp), with a clue button and an explanation of every
// wrong choice. The lessons' plan is in tutor7_lessons.cpp, their words and the exercises' words in
// src/tutor/ (tutor7_text.h), in Romanian and English: the content shows in the board's language
// when Tutor has all of it in that language, else in English (t7::contentLang); the buttons and
// headers are the board's own strings (S_G_T7_*). This tab draws the screens and takes
// the taps; it follows the other games (gHeader, gButton, gTarget).
//
// The fonts have no square root sign, so t7Text() draws it: a stroke down and
// up, and a bar over what is under the root. Everything is laid out from W and
// H on every draw, so a rotation just works.

// ---- text with square roots

static int t7Cap(const GFXfont *f) { return f->yAdvance * 62 / 100; }  // about the height of a capital

// How far the pen moves for s, trailing spaces included (textW stops at the last ink).
static int t7Adv(const GFXfont *f, const String &s) {
  if (!s.length()) return 0;
  return textW(f, (s + "|").c_str()) - textW(f, "|");
}

static bool t7IsRoot(const String &str, int i) {
  int n = str.length();
  return i + 2 < n && (uint8_t)str[i] == 0xE2 && (uint8_t)str[i + 1] == 0x88 && (uint8_t)str[i + 2] == 0x9A;
}

// "2√34 cm" -> "2", root "34", " cm". A root takes the digits after it, or a parenthesised expression.
static std::vector<T7Piece> t7Pieces(const String &str) {
  std::vector<T7Piece> out;
  String cur;
  int n = str.length();
  for (int i = 0; i < n;) {
    if (!t7IsRoot(str, i)) {
      cur += str[i++];
      continue;
    }
    if (cur.length()) out.push_back({cur, false}), cur = "";
    i += 3;
    String rad;
    if (i < n && str[i] == '(') {
      int depth = 0, j = i;
      for (; j < n; j++) {
        if (str[j] == '(') depth++;
        else if (str[j] == ')' && --depth == 0) break;
      }
      rad = str.substring(i + 1, j < n ? j : n);
      i = j < n ? j + 1 : n;
    } else {
      int j = i;
      while (j < n && isdigit((uint8_t)str[j])) j++;
      rad = str.substring(i, j);
      i = j;
    }
    out.push_back({rad, true});
  }
  if (cur.length()) out.push_back({cur, false});
  return out;
}

static int t7RootW(const GFXfont *f, const String &rad) { return t7Cap(f) * 55 / 100 + 3 + t7Adv(f, rad) + 3; }

static int t7TextW(const GFXfont *f, const String &str) {
  int w = 0;
  for (const T7Piece &p : t7Pieces(str)) w += p.root ? t7RootW(f, p.s) : t7Adv(f, p.s);
  return w;
}

// y is the baseline, as for text().
static void t7Text(int x, int y, const GFXfont *f, const String &str) {
  for (const T7Piece &p : t7Pieces(str)) {
    if (!p.root) {
      text(x, y, f, p.s.c_str());
      x += t7Adv(f, p.s);
      continue;
    }
    int h = t7Cap(f), rw = h * 55 / 100, aw = t7Adv(f, p.s), top = y - h - 3;
    thickLine(x, y - h * 42 / 100, x + rw * 30 / 100, y - h * 30 / 100, 2);
    thickLine(x + rw * 30 / 100, y - h * 30 / 100, x + rw * 55 / 100, y + 1, 2);
    thickLine(x + rw * 55 / 100, y + 1, x + rw, top, 2);
    thickLine(x + rw, top, x + rw + 3 + aw + 2, top, 2);
    text(x + rw + 3, y, f, p.s.c_str());
    x += t7RootW(f, p.s);
  }
}

static void t7TextCentre(int cx, int y, const GFXfont *f, const String &s) { t7Text(cx - t7TextW(f, s) / 2, y, f, s); }

// Words, but a space inside a root's parentheses does not end one.
static std::vector<String> t7Words(const String &s) {
  std::vector<String> out;
  String cur;
  int depth = 0;
  for (int i = 0; i < (int)s.length(); i++) {
    char c = s[i];
    if (c == '(' && i >= 3 && t7IsRoot(s, i - 3)) depth = 1;
    else if (depth && c == '(') depth++;
    else if (depth && c == ')') depth--;
    if (c == ' ' && !depth) {
      if (cur.length()) out.push_back(cur), cur = "";
    } else {
      cur += c;
    }
  }
  if (cur.length()) out.push_back(cur);
  // a unit stays with its number: "50 cm" never breaks between the two
  std::vector<String> merged;
  for (const String &w : out) {
    bool unit = w == "cm" || w.startsWith("cm.") || w.startsWith("cm,") || w.startsWith("cm?") || w.startsWith("cm)");
    if (unit && !merged.empty()) merged.back() += " " + w;
    else merged.push_back(w);
  }
  return merged;
}

static std::vector<String> t7Wrap(const GFXfont *f, const String &s, int width) {
  std::vector<String> lines;
  String line;
  for (const String &w : t7Words(s)) {
    String cand = line.length() ? line + " " + w : w;
    if (!line.length() || t7TextW(f, cand) <= width) line = cand;
    else lines.push_back(line), line = w;
  }
  if (line.length()) lines.push_back(line);
  return lines;
}

// Paragraphs (split at \n) wrapped to a width: the height they take, and drawing them.
static int t7BlockH(const T7Block &b, const String &body, int width) {
  int h = 0, paras = 0;
  int from = 0;
  while (from <= (int)body.length()) {
    int nl = body.indexOf('\n', from);
    String para = body.substring(from, nl < 0 ? body.length() : nl);
    h += (int)t7Wrap(b.f, para, width).size() * b.line;
    paras++;
    if (nl < 0) break;
    from = nl + 1;
  }
  return h + (paras - 1) * b.gap;
}
// y is the baseline of the first line; returns the y after the last.
static int t7BlockDraw(const T7Block &b, int x, int y, const String &body, int width) {
  int from = 0;
  while (from <= (int)body.length()) {
    int nl = body.indexOf('\n', from);
    String para = body.substring(from, nl < 0 ? body.length() : nl);
    for (const String &l : t7Wrap(b.f, para, width)) {
      t7Text(x, y, b.f, l);
      y += b.line;
    }
    if (nl < 0) break;
    from = nl + 1;
    y += b.gap;
  }
  return y;
}
// The biggest of 12 pt and 9 pt that fits `height`; draws it. Returns the y after.
static int t7Fitted(int x, int y, const String &body, int width, int height) {
  T7Block big = {&FreeSans12pt7b, 30, 10}, small = {&FreeSans9pt7b, 24, 8};
  const T7Block &b = t7BlockH(big, body, width) <= height ? big : small;
  if (t7BlockH(b, body, width) > height) Serial.printf("games: tutor7 text does not fit: %d > %d px\n", t7BlockH(b, body, width), height);
  return t7BlockDraw(b, x, y, body, width);
}

// ---- figures

static void t7Dashed(int x0, int y0, int x1, int y1) {
  float len = sqrtf((float)((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)));
  int n = max(1, (int)(len / 14));
  for (int i = 0; i < n; i++) {
    float a = (i + 0.0f) / n, b = (i + 0.55f) / n;
    thickLine(x0 + (int)((x1 - x0) * a), y0 + (int)((y1 - y0) * a), x0 + (int)((x1 - x0) * b), y0 + (int)((y1 - y0) * b), 2);
  }
}

// A label on a white patch, so a line behind it does not run through the letters.
static void t7Tag(int cx, int baseline, const String &s, bool bold = false) {
  const GFXfont *f = bold ? &FreeSansBold9pt7b : &FreeSans9pt7b;
  int w = t7TextW(f, s);
  canvas->fillRect(cx - w / 2 - 3, baseline - 16, w + 6, 22, PAPER);
  t7TextCentre(cx, baseline, f, s);
}

static String t7Len(int v) { return v > 0 ? String(v) + " cm" : String("?"); }

static void t7FigRight(const t7::Figure &fg, bool named, int bx, int by, int bw, int bh) {
  float ab = 4, ac = 3, bc = 5;
  if (!named) {
    ab = fg.v[0], ac = fg.v[1], bc = fg.v[2];
    if (ab <= 0) ab = sqrtf(max(1.0f, bc * bc - ac * ac));
    if (ac <= 0) ac = sqrtf(max(1.0f, bc * bc - ab * ab));
  }
  float dx = ab, dy = ac;  // drawn proportions: a very long leg is shortened so both stay visible
  if (dx > 2.4f * dy) dx = 2.4f * dy;
  if (dy > 2.4f * dx) dy = 2.4f * dx;
  int left = 70, right = 44, top = 28, bottom = 34;
  float sc = fminf((bw - left - right) / dx, (bh - top - bottom) / dy);
  int w = (int)(dx * sc), h = (int)(dy * sc);
  int ax = bx + left + (bw - left - right - w) / 2, ay = by + bh - bottom, cy = ay - h, bxp = ax + w;
  thickLine(ax, ay, bxp, ay, 3);
  thickLine(ax, ay, ax, cy, 3);
  thickLine(bxp, ay, ax, cy, 3);
  canvas->drawRect(ax + 3, ay - 15, 12, 12, INK);  // the right angle at A
  t7Tag(ax - 14, ay + 22, "A", true);
  t7Tag(bxp + 16, ay + 22, "B", true);
  t7Tag(ax, cy - 10, "C", true);
  String sAB = named ? "AB" : t7Len(fg.v[0]), sAC = named ? "AC" : t7Len(fg.v[1]), sBC = named ? "BC" : t7Len(fg.v[2]);
  t7Tag(ax + w / 2, ay + 24, sAB);
  int acw = t7TextW(&FreeSans9pt7b, sAC);
  t7Tag(ax - 12 - acw / 2, ay - h / 2 + 5, sAC);
  int bcw = t7TextW(&FreeSans9pt7b, sBC);
  t7Tag(ax + w / 2 + 14 + bcw / 2, cy + h / 2 - 4, sBC);
  if (named) return;
  if (fg.v[3] == 2) {  // the height from A to BC is asked: dashed, to its foot on BC
    float t = (float)w * w / max(1, w * w + h * h);
    int fx = bxp - (int)(t * w), fy = ay - (int)(t * h);
    t7Dashed(ax, ay, fx, fy);
    t7Tag((ax + fx) / 2, (ay + fy) / 2 + 6, "h = ?", true);
  } else if (fg.v[3] == 4) {  // the area is asked
    t7Tag(ax + w / 3, ay - h / 3 + 6, t7::tx(gs->t7Ex.lang, t7::TX_FIG_AREA), true);
  }
}

static void t7FigTriangle(const t7::Figure &fg, int bx, int by, int bw, int bh) {
  float s[3] = {(float)fg.v[0], (float)fg.v[1], (float)fg.v[2]};
  for (int i = 0; i < 3; i++)
    for (int j = i + 1; j < 3; j++)
      if (s[j] < s[i]) {
        float t = s[i];
        s[i] = s[j];
        s[j] = t;
      }
  float a = s[0], b = s[1], c = s[2];  // c, the longest, is the base
  float x2 = (b * b + c * c - a * a) / (2 * c), y2 = sqrtf(max(0.01f, b * b - x2 * x2));
  int mx = 56, mtop = 30, mbot = 42;
  float sc = fminf((bw - 2 * mx) / c, (bh - mtop - mbot) / y2);
  int px0 = bx + mx + (int)((bw - 2 * mx - c * sc) / 2), py = by + bh - mbot;
  int x0 = px0, x1 = px0 + (int)(c * sc), x2p = px0 + (int)(x2 * sc), y2p = py - (int)(y2 * sc);
  thickLine(x0, py, x1, py, 3);
  thickLine(x0, py, x2p, y2p, 3);
  thickLine(x1, py, x2p, y2p, 3);
  t7Tag((x0 + x1) / 2, py + 26, String((int)c) + " cm");
  t7Tag((x0 + x2p) / 2 - 34, (py + y2p) / 2 + 4, String((int)b) + " cm");
  t7Tag((x1 + x2p) / 2 + 34, (py + y2p) / 2 + 4, String((int)a) + " cm");
}

static void t7FigSquare(const t7::Figure &fg, int bx, int by, int bw, int bh) {
  int side = min(bw - 110, bh - 62);
  int x0 = bx + (bw - side) / 2, y0 = by + 12, x1 = x0 + side, y1 = y0 + side;
  thickLine(x0, y0, x1, y0, 3);
  thickLine(x1, y0, x1, y1, 3);
  thickLine(x1, y1, x0, y1, 3);
  thickLine(x0, y1, x0, y0, 3);
  t7Dashed(x0, y1, x1, y0);
  if (fg.v[1] == 1) {  // the diagonal is given, the side is asked
    t7Tag((x0 + x1) / 2, y1 + 28, t7::tx(gs->t7Ex.lang, t7::TX_FIG_SIDE), true);
    t7Tag(x0 + side * 32 / 100, y0 + side * 28 / 100, String("d = ") + t7Len(fg.v[0]), true);
    return;
  }
  t7Tag((x0 + x1) / 2, y1 + 28, t7Len(fg.v[0]));
  t7Tag(x0 + side * 32 / 100, y0 + side * 28 / 100, "d = ?", true);  // above the diagonal, clear of it
}

static void t7FigEquilateral(const t7::Figure &fg, int bx, int by, int bw, int bh) {
  int L = min(bw - 120, (int)((bh - 66) / 0.866f));
  int x0 = bx + (bw - L) / 2, x1 = x0 + L, y = by + bh - 36, hh = (int)(L * 0.866f), ax = x0 + L / 2, ay = y - hh;
  thickLine(x0, y, x1, y, 3);
  thickLine(x0, y, ax, ay, 3);
  thickLine(x1, y, ax, ay, 3);
  t7Dashed(ax, ay, ax, y);
  canvas->drawRect(ax + 3, y - 14, 11, 11, INK);  // the right angle at D
  t7Tag(ax, ay - 10, "A", true);
  t7Tag(x0 - 14, y + 22, "B", true);
  t7Tag(x1 + 14, y + 22, "C", true);
  t7Tag(ax, y + 24, "D", true);
  t7Tag(ax + 12 + t7TextW(&FreeSansBold9pt7b, "AD = ?") / 2, y - hh / 7, "AD = ?", true);  // low, where the triangle is wide
  String s = t7Len(fg.v[0]);
  t7Tag((x0 + ax) / 2 - 30 - t7TextW(&FreeSans9pt7b, s) / 2, ay + hh / 2 + 6, s);
}


// ---- Unit 4 figures: the vertices are named, the numbers are in the question

static void t7Poly(const T7Pt *p, int n, int bx, int by, int bw, int bh, const char *names, bool diagonals, int inset = 34) {
  int ix = bx + inset, iy = by + 22, iw = bw - 2 * inset, ih = bh - 52;
  int X[8], Y[8];
  float cx = 0, cy = 0;
  for (int i = 0; i < n; i++) {
    X[i] = ix + (int)(p[i].x * iw), Y[i] = iy + (int)(p[i].y * ih);
    cx += X[i], cy += Y[i];
  }
  cx /= n, cy /= n;
  for (int i = 0; i < n; i++) thickLine(X[i], Y[i], X[(i + 1) % n], Y[(i + 1) % n], 3);
  if (diagonals && n == 4) {
    t7Dashed(X[0], Y[0], X[2], Y[2]);
    t7Dashed(X[1], Y[1], X[3], Y[3]);
    // the diagonals' meeting point
    float d1x = X[2] - X[0], d1y = Y[2] - Y[0], d2x = X[3] - X[1], d2y = Y[3] - Y[1];
    float den = d1x * d2y - d1y * d2x;
    if (fabsf(den) > 0.001f) {
      float t = ((X[1] - X[0]) * d2y - (Y[1] - Y[0]) * d2x) / den;
      t7Tag((int)(X[0] + t * d1x) + 12, (int)(Y[0] + t * d1y) - 6, "O", true);
    }
  }
  for (int i = 0; i < n; i++) {
    float dx = X[i] - cx, dy = Y[i] - cy, len = sqrtf(dx * dx + dy * dy);
    if (len < 1) len = 1;
    char one[2] = {names[i], 0};
    t7Tag(X[i] + (int)(dx / len * 17), Y[i] + (int)(dy / len * 17) + 7, one, true);
  }
}

static void t7FigQuad(int bx, int by, int bw, int bh) {
  static const T7Pt q[4] = {{0.04f, 0.9f}, {0.9f, 0.84f}, {0.78f, 0.06f}, {0.2f, 0.16f}};
  t7Poly(q, 4, bx, by, bw, bh, "ABCD", false);
  int ix = bx + 34, iy = by + 22, iw = bw - 68, ih = bh - 52;  // the diagonal BD, dashed
  t7Dashed(ix + (int)(q[1].x * iw), iy + (int)(q[1].y * ih), ix + (int)(q[3].x * iw), iy + (int)(q[3].y * ih));
}

static void t7FigPar(int bx, int by, int bw, int bh) {
  static const T7Pt q[4] = {{0.02f, 0.9f}, {0.68f, 0.9f}, {0.98f, 0.1f}, {0.32f, 0.1f}};
  t7Poly(q, 4, bx, by, bw, bh, "ABCD", true);
}

static void t7FigRect(int bx, int by, int bw, int bh) {
  static const T7Pt q[4] = {{0.05f, 0.9f}, {0.95f, 0.9f}, {0.95f, 0.1f}, {0.05f, 0.1f}};
  t7Poly(q, 4, bx, by, bw, bh, "ABCD", true);
  int ix = bx + 34, iy = by + 22, iw = bw - 68, ih = bh - 52;
  canvas->drawRect(ix + (int)(q[0].x * iw) + 3, iy + (int)(q[0].y * ih) - 15, 12, 12, INK);  // the right angle at A
}

static void t7FigRhombus(int bx, int by, int bw, int bh) {
  static const T7Pt q[4] = {{0.03f, 0.5f}, {0.5f, 0.92f}, {0.97f, 0.5f}, {0.5f, 0.08f}};
  t7Poly(q, 4, bx, by, bw, bh, "ABCD", true);
}

static void t7FigTrapezoid(int bx, int by, int bw, int bh, bool isosceles) {
  static const T7Pt t[4] = {{0.03f, 0.9f}, {0.97f, 0.9f}, {0.72f, 0.1f}, {0.3f, 0.1f}};
  static const T7Pt u[4] = {{0.03f, 0.9f}, {0.97f, 0.9f}, {0.78f, 0.1f}, {0.22f, 0.1f}};
  const T7Pt *q = isosceles ? u : t;
  t7Poly(q, 4, bx, by, bw, bh, "ABCD", false);
  int ix = bx + 34, iy = by + 22, iw = bw - 68, ih = bh - 52;
  int dx = ix + (int)(q[3].x * iw), dy = iy + (int)(q[3].y * ih);
  if (isosceles) {  // the height from D
    t7Dashed(dx, dy, dx, iy + (int)(q[0].y * ih));
  }
}

static void t7FigMidline(int bx, int by, int bw, int bh) {
  static const T7Pt q[3] = {{0.5f, 0.06f}, {0.03f, 0.92f}, {0.97f, 0.92f}};
  t7Poly(q, 3, bx, by, bw, bh, "ABC", false);
  int ix = bx + 34, iy = by + 22, iw = bw - 68, ih = bh - 52;
  int mx = ix + (int)((q[0].x + q[1].x) / 2 * iw), my = iy + (int)((q[0].y + q[1].y) / 2 * ih);
  int nx = ix + (int)((q[0].x + q[2].x) / 2 * iw), ny = iy + (int)((q[0].y + q[2].y) / 2 * ih);
  t7Dashed(mx, my, nx, ny);
  t7Tag(mx - 16, my + 6, "M", true);
  t7Tag(nx + 16, ny + 6, "N", true);
}

static void t7FigMedians(int bx, int by, int bw, int bh) {
  static const T7Pt q[3] = {{0.5f, 0.06f}, {0.03f, 0.92f}, {0.97f, 0.92f}};
  t7Poly(q, 3, bx, by, bw, bh, "ABC", false);
  int ix = bx + 34, iy = by + 22, iw = bw - 68, ih = bh - 52;
  int X[3], Y[3];
  for (int i = 0; i < 3; i++) X[i] = ix + (int)(q[i].x * iw), Y[i] = iy + (int)(q[i].y * ih);
  for (int i = 0; i < 3; i++) {  // each median: from a vertex to the middle of the opposite side
    int a = (i + 1) % 3, b = (i + 2) % 3;
    t7Dashed(X[i], Y[i], (X[a] + X[b]) / 2, (Y[a] + Y[b]) / 2);
  }
  t7Tag((X[1] + X[2]) / 2, Y[1] + 26, "M", true);
  t7Tag((X[0] + X[1] + X[2]) / 3 + 16, (Y[0] + Y[1] + Y[2]) / 3 + 6, "G", true);
}

static void t7Figure(const t7::Figure &fg, int bx, int by, int bw, int bh) {
  switch (fg.kind) {
    case t7::FIG_RIGHT_TRIANGLE: return t7FigRight(fg, false, bx, by, bw, bh);
    case t7::FIG_RIGHT_NAMED: return t7FigRight(fg, true, bx, by, bw, bh);
    case t7::FIG_TRIANGLE: return t7FigTriangle(fg, bx, by, bw, bh);
    case t7::FIG_SQUARE_DIAGONAL: return t7FigSquare(fg, bx, by, bw, bh);
    case t7::FIG_EQUILATERAL_HEIGHT: return t7FigEquilateral(fg, bx, by, bw, bh);
    case t7::FIG_QUAD: return t7FigQuad(bx, by, bw, bh);
    case t7::FIG_PARALLELOGRAM: return t7FigPar(bx, by, bw, bh);
    case t7::FIG_RECTANGLE: return t7FigRect(bx, by, bw, bh);
    case t7::FIG_RHOMBUS: return t7FigRhombus(bx, by, bw, bh);
    case t7::FIG_TRAPEZOID: return t7FigTrapezoid(bx, by, bw, bh, false);
    case t7::FIG_TRAPEZOID_ISOSCELES: return t7FigTrapezoid(bx, by, bw, bh, true);
    case t7::FIG_MIDLINE: return t7FigMidline(bx, by, bw, bh);
    case t7::FIG_MEDIANS: return t7FigMedians(bx, by, bw, bh);
    case t7::FIG_NONE: return;
    default: return;
  }
}

// ---- the flow

enum : int {
  T7_OPT0 = 0,       // 0..3: the answers
  T7_CLUE_BTN = 10,  // Indicii
  T7_SHOW_BTN = 11,  // Arată rezolvarea
  T7_AGAIN_BTN = 12, // back to the exercise
  T7_MORE_CLUE = 13, // another clue
  T7_LIST_BTN = 14,  // the result's Lecții: the lesson list
  T7_CLASS0 = 25,    // the class buttons
  T7_PART0 = 20,     // 20 summary, 21 examples, 22 practice
  T7_SUBJECT0 = 30,
  T7_LESSON0 = 40,
  T7_PREV = 50,
  T7_RESET = 60,   // clear the lesson's progress, start from the summary (a second tap confirms)
};

static const t7::Lesson &t7L() { return t7::lesson(gs->t7Lesson); }
static const t7::LessonText &t7Words(const t7::Lesson &l) { return t7::lessonText(t7Lang(), l.slot); }
static const char *t7Name(const t7::Lesson &l) { return t7Words(l).name; }

// The language Tutor's lessons and exercises are shown in now.
static int t7Lang() { return t7::contentLang(i18nLang()); }

// What an exercise asks, whatever its language: its kind and its givens.
static uint32_t t7Hash(const t7::Exercise &ex) {
  uint32_t h = 2166136261u;
  auto mix = [&h](uint32_t v) {
    for (int i = 0; i < 4; i++, v >>= 8) h = (h ^ (v & 0xFF)) * 16777619u;
  };
  mix(ex.kind);
  mix(ex.fig.kind);
  for (int32_t v : ex.fig.v) mix((uint32_t)v);
  return h;
}

// Makes exercise gs->t7Index of the practice: a fresh seed until the exercise exists, verifies and
// has not been asked in this practice. False only if nothing works (it is then skipped).
// It is made in place, in gs (PSRAM): an Exercise is 3.9 KB and the loop task's stack 8 KB. The exercise
// it overwrites is the one just finished, and nothing shows it while this runs.
static bool t7Make() {
  const t7::Step &st = t7L().steps[gs->t7Index];
  t7::Exercise &ex = gs->t7Ex;
  for (int tries = 0; tries < 400; tries++) {
    uint32_t seed = (uint32_t)gameRnd(1, 2000000000);
    t7::Rng rng(seed);
    char err[120];
    if (!t7::generate(st.kind, st.level, rng, ex, t7Lang())) continue;
    if (!t7::verify(ex, err, sizeof err)) {
      Serial.printf("games: tutor7 exercise rejected (seed %lu): %s\n", (unsigned long)seed, err);
      continue;
    }
    uint32_t h = t7Hash(ex);
    bool seen = false;
    for (int i = 0; i < gs->t7Index; i++) seen |= gs->t7Seen[i] == h;
    if (seen && tries < 300) continue;
    gs->t7Seen[gs->t7Index] = h;
    gs->t7Seed = seed;
    Serial.printf("games: tutor7 exercise %d of %d: kind %d level %d seed %lu %s, right option %d (%s)\n", gs->t7Index + 1, t7L().nSteps, (int)ex.kind,
                  (int)ex.level, (unsigned long)seed, LANGS[ex.lang].code, ex.correct, ex.opt[ex.correct]);
    return true;
  }
  Serial.printf("games: tutor7 could not make exercise %d\n", gs->t7Index + 1);
  return false;
}

// The board's language changed while an exercise is open: the same exercise (its seed) again, in the new
// content language. The numbers, the options' order and the right one are the same in every language,
// so what was crossed out, picked or answered still holds.
static void t7Relang() {
  if (gs->t7Screen < T7_QUESTION || gs->t7Screen > T7_SOLUTION || gs->t7Ex.lang == t7Lang()) return;
  const t7::Step &st = t7L().steps[gs->t7Index];
  t7::Exercise &ex = gs->t7Ex;
  int was = ex.lang, correct = ex.correct;
  t7::Rng rng(gs->t7Seed);
  char err[120] = "";
  bool ok = t7::generate(st.kind, st.level, rng, ex, t7Lang()) && t7::verify(ex, err, sizeof err) && ex.correct == correct;
  Serial.printf("games: tutor7 exercise %d now in %s (seed %lu)%s%s\n", gs->t7Index + 1, LANGS[ex.lang].code, (unsigned long)gs->t7Seed, ok ? "" : ": failed ", err);
  if (ok) return;
  // never happens (firmware/test/tutor7 checks every seed makes the same exercise in every language); back to the old one
  t7::Rng again(gs->t7Seed);
  t7::generate(st.kind, st.level, again, ex, was);
}

// Shows exercise gs->t7Index, or the result after the last.
static void t7Exercise() {
  while (gs->t7Index < t7L().nSteps && !t7Make()) gs->t7Index++;
  gs->t7Tries = gs->t7Clues = 0;
  gs->t7Picked = -1;
  gs->t7GaveUp = false;
  for (bool &c : gs->t7Crossed) c = false;
  gs->t7Screen = T7_QUESTION;
  if (gs->t7Index >= t7L().nSteps) {
    uint8_t &best = gameProgress().tutor7Best[t7L().slot];
    gs->t7NewBest = gs->t7Right > best;
    if (gs->t7NewBest) best = gs->t7Right, gamesChanged();
    gs->t7Screen = T7_RESULT;
    Serial.printf("games: tutor7 lesson done: %d of %d right first time%s\n", gs->t7Right, t7L().nSteps, gs->t7NewBest ? " (a best)" : "");
  }
  gs->gFocusId = -1;
}

static void t7StartPractice() {
  gs->t7Index = 0;
  gs->t7Right = 0;
  for (uint32_t &h : gs->t7Seen) h = 0;
  Serial.printf("games: tutor7 practice: %s\n", t7Name(t7L()));
  t7Exercise();
}

static void tutor7Open() {
  if (gs->t7Screen == T7_RESULT) gs->t7Screen = T7_LESSONS;
  gs->t7ResetArmed = false;
  gs->gFocusId = -1;
}

static void tutor7Peek(String &s) {
  t7Relang();  // as the screen will show it
  static const char *NAMES[] = {"classes", "subjects", "lessons", "lesson", "page", "question", "clue", "wrong", "solution", "result"};
  s += String("tutor7 ") + NAMES[constrain(gs->t7Screen, 0, 9)] + " lang " + LANGS[t7Lang()].code;
  if (gs->t7Screen == T7_PAGE) s += String(" ") + (gs->t7Part ? "example " : "summary ") + (gs->t7Page + 1);
  if (gs->t7Screen >= T7_QUESTION && gs->t7Screen <= T7_SOLUTION) {
    const t7::Exercise &ex = gs->t7Ex;
    s += String(" ") + (gs->t7Index + 1) + "/" + t7L().nSteps + ": " + ex.text + " options";
    for (int i = 0; i < ex.nopt; i++) s += String(" [") + i + (i == ex.correct ? "*" : "") + "] " + ex.opt[i];
    s += String(", tries ") + gs->t7Tries + ", clues " + gs->t7Clues + ", right " + gs->t7Right;
  }
  if (gs->t7Screen == T7_RESULT) s += String(": ") + gs->t7Right + " of " + t7L().nSteps;
}

static void t7Answer(int i) {
  t7::Exercise &ex = gs->t7Ex;
  if (i < 0 || i >= ex.nopt || gs->t7Crossed[i]) return;
  if (i == ex.correct) {
    if (!gs->t7Tries) gs->t7Right++;
    Serial.printf("games: tutor7 answer %d right%s\n", i, gs->t7Tries ? " (after a wrong one)" : " first time");
    beep(2200, 40);
    gs->t7Screen = T7_SOLUTION;
  } else {
    gs->t7Tries++;
    gs->t7Crossed[i] = true;
    gs->t7Picked = i;
    Serial.printf("games: tutor7 answer %d wrong (%s)\n", i, ex.opt[i]);
    beep(900, 80);
    gs->t7Screen = T7_WRONG;
  }
  gs->gFocusId = -1;
}

static void actTutor7(int id) {
  int &sc = gs->t7Screen;
  gs->gFocusId = -1;
  bool resetArmed = gs->t7ResetArmed;
  gs->t7ResetArmed = false;  // any tap but a second one on Reset disarms it
  if (id == ID_BACK) {
    switch (sc) {
      case T7_CLASSES: gs->gGame = -1; break;
      case T7_SUBJECTS: sc = T7_CLASSES; break;
      case T7_LESSONS: sc = T7_SUBJECTS; break;
      case T7_LESSON: sc = T7_LESSONS; break;
      default: sc = T7_LESSON; break;  // a page, the practice, the result: the lesson
    }
    return render(false);
  }
  switch (sc) {
    case T7_CLASSES:
      if (id == T7_CLASS0) sc = T7_SUBJECTS;
      break;
    case T7_SUBJECTS:
      if (id == T7_SUBJECT0) sc = T7_LESSONS;
      break;
    case T7_LESSONS:
      if (id >= T7_LESSON0 && id < T7_LESSON0 + t7::LESSON_COUNT) gs->t7Lesson = id - T7_LESSON0, sc = T7_LESSON;
      break;
    case T7_LESSON:
      if (id == T7_PART0 || id == T7_PART0 + 1) {
        gs->t7Part = id - T7_PART0, gs->t7Page = 0, sc = T7_PAGE;
        Serial.printf("games: tutor7 %s: %s\n", gs->t7Part ? "examples" : "summary", t7Name(t7L()));
      } else if (id == T7_PART0 + 2) {
        return t7StartPractice(), render(true);
      } else if (id == T7_RESET) {
        if (!resetArmed) {
          gs->t7ResetArmed = true;
          return render(false);
        }
        gameProgress().tutor7Best[t7L().slot] = 0;
        gamesChanged();
        gs->t7Part = 0, gs->t7Page = 0, gs->t7Index = 0, gs->t7Right = 0, sc = T7_PAGE;
        Serial.printf("games: tutor7 reset: %s, progress cleared\n", t7Name(t7L()));
        return render(true);
      }
      break;
    case T7_PAGE: {
      int n = gs->t7Part ? t7L().nExample : t7L().nSummary;
      if (id == T7_PREV) gs->t7Page = max(0, gs->t7Page - 1);
      else if (id == ID_NEXT) {
        if (gs->t7Page + 1 < n) gs->t7Page++;
        else sc = T7_LESSON;
      }
      break;
    }
    case T7_QUESTION:
      if (id >= 0 && id < 4) return t7Answer(id), render(false);
      if (id == T7_CLUE_BTN) {
        gs->t7Clues = max(1, gs->t7Clues), sc = T7_CLUE;
        Serial.printf("games: tutor7 clue %d\n", gs->t7Clues);
      } else if (id == T7_SHOW_BTN) {
        gs->t7GaveUp = true, sc = T7_SOLUTION;
        Serial.println("games: tutor7 solution shown");
      }
      break;
    case T7_CLUE:
      if (id == T7_AGAIN_BTN) sc = T7_QUESTION;
      else if (id == T7_MORE_CLUE && gs->t7Clues < 2) gs->t7Clues = 2, Serial.println("games: tutor7 clue 2");
      break;
    case T7_WRONG:
      if (id == T7_AGAIN_BTN) sc = T7_QUESTION;
      else if (id == T7_CLUE_BTN) gs->t7Clues = max(1, gs->t7Clues), sc = T7_CLUE, Serial.printf("games: tutor7 clue %d\n", gs->t7Clues);
      else if (id == T7_SHOW_BTN) gs->t7GaveUp = true, sc = T7_SOLUTION, Serial.println("games: tutor7 solution shown");
      break;
    case T7_SOLUTION:
      if (id == ID_NEXT) {
        gs->t7Index++;
        t7Exercise();
        return render(sc == T7_RESULT);
      }
      break;
    case T7_RESULT:
      if (id == ID_AGAIN) return t7StartPractice(), render(true);
      if (id == T7_LIST_BTN) sc = T7_LESSONS;
      break;
  }
  render(false);
}

// ---- drawing

// A tall button: the label wrapped to its width, centred.
static int t7ButtonH(const String &label, int w, int minH) {
  int lines = (int)t7Wrap(&FreeSans12pt7b, label, w - 28).size();
  return max(minH, lines * 28 + 22);
}
static void t7Button(int x, int y, int w, int h, const String &label, int id, bool filled = false, bool strike = false) {
  canvas->fillRoundRect(x, y, w, h, 18, filled ? INK : PAPER);
  for (int k = 0; k < 2; k++) canvas->drawRoundRect(x + k, y + k, w - 2 * k, h - 2 * k, 18 - k, INK);
  const GFXfont *f = &FreeSans12pt7b;
  auto lines = t7Wrap(f, label, w - 28);
  int total = (int)lines.size() * 28, ly = y + (h - total) / 2 + 21;
  canvas->setTextColor(filled ? PAPER : INK);
  for (const String &l : lines) {
    int lw = t7TextW(f, l);
    t7Text(x + (w - lw) / 2, ly, f, l);
    ly += 28;
  }
  canvas->setTextColor(INK);
  if (strike) thickLine(x + 20, y + h / 2, x + w - 20, y + h / 2, 4);
  if (!strike) gTarget(x, y, w, h, id);
}

static void drawT7Menu(const String &title, const std::vector<String> &labels, const std::vector<int> &ids, const String &note) {
  gHeader(title);
  bool many = labels.size() > 5;  // the lesson list: compact, and two columns in landscape
  int cols = many && !TALL ? 2 : 1;
  int top = gTop() + (many ? 8 : 14), gap = many ? 9 : 14, minH = many ? 56 : 64;
  int w = cols == 2 ? (W - 32 - 14) / 2 : (TALL ? W - 32 : min(W - 32, 520)), x0 = cols == 2 ? 16 : (W - w) / 2, y = top;
  if (note.length() && !many) y = t7BlockDraw({&FreeSans12pt7b, 30, 8}, 16, y + 20, note, W - 32) - 4;
  int rowY[2] = {y, y};
  for (size_t i = 0; i < labels.size(); i++) {
    int col = cols == 2 ? (int)(i % 2) : 0;
    int h = t7ButtonH(labels[i], w, minH);
    if (cols == 2 && col == 0 && i + 1 < labels.size()) h = max(h, t7ButtonH(labels[i + 1], w, minH));  // a row is as tall as its taller button
    if (cols == 2 && col == 1) h = max(t7ButtonH(labels[i], w, minH), t7ButtonH(labels[i - 1], w, minH));
    t7Button(x0 + col * (w + 14), rowY[col], w, h, labels[i], ids[i]);
    rowY[col] += h + gap;
  }
}

static void drawT7Page() {
  const t7::Lesson &l = t7L();
  bool ex = gs->t7Part == 1;
  const t7::PageText &p = ex ? t7Words(l).example[gs->t7Page] : t7Words(l).summary[gs->t7Page];
  int figure = ex ? l.exampleFigure[gs->t7Page] : l.summaryFigure[gs->t7Page];
  int n = ex ? l.nExample : l.nSummary;
  String count = String(gs->t7Page + 1) + " / " + n;
  int btnH = 52, btnY = H - 16 - btnH, top = gTop() + 4;
  // The title goes in the header when it fits there (landscape, a short title); else the header says
  // what part this is, and the title is wrapped at the top of the page.
  int headRoom = W - (16 + (TALL ? 104 : 116) + 18) - textW(&FreeSansBold12pt7b, count.c_str()) - 40;
  if (t7TextW(&FreeSansBold18pt7b, p.title) <= headRoom) {
    gHeader(p.title, count);
  } else {
    gHeader(T(ex ? S_G_T7_EXAMPLE : S_G_T7_SUMMARY), count);
    auto lines = t7Wrap(&FreeSansBold12pt7b, p.title, W - 32);
    int ty = top + 24;
    for (const String &l : lines) t7Text(16, ty, &FreeSansBold12pt7b, l), ty += 30;
    top = ty - 14;
  }
  bool fig = figure >= 0;
  int figW = TALL ? W - 32 : 230, figH = TALL ? 190 : btnY - top - 8;
  int textX = 16, textW = W - 32, textTop = top + 22, textBottom = btnY - 8;
  if (fig && !TALL) textW = W - 32 - figW - 16;
  if (fig && TALL) textTop = top + figH + 22;
  if (fig) t7Figure(t7::Figure{(t7::FigureKind)figure, {0, 0, 0}}, TALL ? 16 : W - 16 - figW, top, figW, figH);
  t7Fitted(textX, textTop, p.body, textW, textBottom - textTop);
  int bw = (W - 32 - 14) / 2;
  if (gs->t7Page > 0) t7Button(16, btnY, bw, btnH, T(S_G_T7_PREV), T7_PREV);
  t7Button(W - 16 - bw, btnY, bw, btnH, T(gs->t7Page + 1 < n ? S_G_T7_NEXT : S_G_T7_DONE), ID_NEXT, true);
}

static void drawT7Question() {
  const t7::Exercise &ex = gs->t7Ex;
  gHeader(TF(S_G_T7_EXERCISE, gs->t7Index + 1, t7L().nSteps));
  int top = gTop() + 4, btnH = 52, gap = 10;
  const GFXfont *qf = &FreeSans12pt7b;
  bool land = !TALL;
  int leftW = land ? (W - 48) / 2 : W - 32;
  int optX = land ? 16 + leftW + 16 : 16, optW = land ? W - 16 - optX : W - 32;
  // the answers' heights
  int oh[4], optsH = 0;
  for (int i = 0; i < ex.nopt; i++) oh[i] = t7ButtonH(ex.opt[i], optW, 58), optsH += oh[i] + (i ? gap : 0);
  // the question
  int qy = top + 22, qEnd = t7BlockDraw({qf, 30, 8}, 16, qy, ex.text, leftW);
  // the figure: under the question; in portrait it gets what the answers and buttons leave
  int figY = qEnd - 4, figH = land ? H - 16 - figY : H - 16 - figY - 8 - optsH - 10 - btnH;
  if (ex.fig.kind != t7::FIG_NONE) {
    if (figH > 60) t7Figure(ex.fig, 16, figY, leftW, min(figH, 230));
    else Serial.printf("games: tutor7 no room for the figure (%d px)\n", figH);
  }
  // the answers: from the top on the right (landscape), or just above the buttons (portrait)
  int oy = land ? top + 6 : H - 16 - btnH - 10 - optsH;
  for (int i = 0; i < ex.nopt; i++) {
    t7Button(optX, oy, optW, oh[i], ex.opt[i], i, false, gs->t7Crossed[i]);
    oy += oh[i] + gap;
  }
  // the clue and solution buttons
  int by = land ? max(oy + 4, H - 16 - btnH) : H - 16 - btnH;
  int bw = optW;
  bool show = gs->t7Tries >= 1;
  int cw = show ? (bw - 10) / 2 : bw;
  t7Button(optX, by, cw, btnH, T(S_G_T7_HINTS), T7_CLUE_BTN);
  if (show) t7Button(optX + cw + 10, by, bw - cw - 10, btnH, T(S_G_T7_SOLUTION), T7_SHOW_BTN);
}

static void drawT7Clue() {
  const t7::Exercise &ex = gs->t7Ex;
  gHeader(TF(S_G_T7_HINT_N, gs->t7Clues));
  int top = gTop() + 14, btnH = 56, btnY = H - 16 - btnH;
  String body = String(ex.clue[0]);
  if (gs->t7Clues >= 2) body += String("\n") + ex.clue[1];
  t7Fitted(16, top + 24, body, W - 32, btnY - top - 24);
  int bw = gs->t7Clues < 2 ? (W - 32 - 14) / 2 : W - 32;
  t7Button(16, btnY, bw, btnH, T(S_G_T7_BACK_TO_EXERCISE), T7_AGAIN_BTN, true);
  if (gs->t7Clues < 2) t7Button(W - 16 - bw, btnY, bw, btnH, T(S_G_T7_MORE_HINT), T7_MORE_CLUE);
}

static void drawT7Wrong() {
  const t7::Exercise &ex = gs->t7Ex;
  gHeader(T(S_G_T7_NOT_RIGHT));
  int top = gTop() + 14, btnH = 56, btnY = H - 16 - btnH;
  char chose[120];
  snprintf(chose, sizeof chose, t7::tx(ex.lang, t7::TX_YOU_CHOSE), ex.opt[gs->t7Picked]);
  String body = String(chose) + "\n" + ex.why[gs->t7Picked];
  t7Fitted(16, top + 24, body, W - 32, btnY - top - 24);
  bool show = gs->t7Tries >= 2;
  int n = show ? 3 : 2, bw = (W - 32 - 10 * (n - 1)) / n, x = 16;
  t7Button(x, btnY, bw, btnH, T(S_G_T7_TRY_AGAIN), T7_AGAIN_BTN, true), x += bw + 10;
  t7Button(x, btnY, bw, btnH, T(S_G_T7_HINTS), T7_CLUE_BTN), x += bw + 10;
  if (show) t7Button(x, btnY, bw, btnH, T(S_G_T7_SOLUTION), T7_SHOW_BTN);
}

static void drawT7Solution() {
  const t7::Exercise &ex = gs->t7Ex;
  gHeader(T(gs->t7GaveUp ? S_G_T7_SOLUTION : S_G_T7_CORRECT));
  int top = gTop() + 14, btnH = 56, btnY = H - 16 - btnH;
  String body = String(ex.solution);
  if (gs->t7GaveUp) {
    char right[120];
    snprintf(right, sizeof right, t7::tx(ex.lang, t7::TX_RIGHT_ANSWER), ex.opt[ex.correct]);
    body += String("\n") + right;
  }
  t7Fitted(16, top + 24, body, W - 32, btnY - top - 24);
  bool last = gs->t7Index + 1 >= t7L().nSteps;
  t7Button(16, btnY, W - 32, btnH, T(last ? S_G_T7_SEE_RESULT : S_G_T7_NEXT), ID_NEXT, true);
}

static void drawT7Result() {
  const t7::Lesson &l = t7L();
  gHeader(T(S_G_T7_RESULT));
  int top = gTop() + 20, cx = W / 2;
  textCentre(cx, top + 50, &FreeSansBold24pt7b, TF(S_G_T7_SCORE, gs->t7Right, l.nSteps).c_str());
  textCentre(cx, top + 92, &FreeSans12pt7b, T(S_G_T7_FIRST_TIME));
  String msg = T(gs->t7Right == l.nSteps ? S_G_T7_MSG_ALL : gs->t7Right * 2 >= l.nSteps ? S_G_T7_MSG_HALF : S_G_T7_MSG_LOW);
  if (gs->t7NewBest) msg += String("\n") + T(S_G_T7_NEW_BEST);
  int y = top + 140;
  for (int from = 0; from <= (int)msg.length();) {  // paragraphs, each wrapped and centred
    int nl = msg.indexOf('\n', from);
    for (const String &line : t7Wrap(&FreeSans12pt7b, msg.substring(from, nl < 0 ? msg.length() : nl), W - 40)) {
      textCentre(cx, y, &FreeSans12pt7b, line.c_str());
      y += 30;
    }
    if (nl < 0) break;
    from = nl + 1;
    y += 8;
  }
  int btnH = 56, btnY = H - 16 - btnH, bw = (W - 32 - 14) / 2;
  t7Button(16, btnY, bw, btnH, T(S_G_T7_LESSONS), T7_LIST_BTN);
  t7Button(W - 16 - bw, btnY, bw, btnH, T(S_G_T7_ONCE_MORE), ID_AGAIN, true);
}

static void drawTutor7() {
  t7Relang();
  switch (gs->t7Screen) {
    case T7_LESSONS: {
      std::vector<String> labels;
      std::vector<int> ids;
      for (int i = 0; i < t7::LESSON_COUNT; i++) {
        int best = gameProgress().tutor7Best[t7::lesson(i).slot];
        labels.push_back(String(t7::lessonNumber(t7::lesson(i).slot)) + " " + t7Name(t7::lesson(i)) + (best ? String(" (") + best + "/" + t7::lesson(i).nSteps + ")" : String("")));
        ids.push_back(T7_LESSON0 + i);
      }
      return drawT7Menu(t7Words(t7::lesson(0)).subject, labels, ids, T(S_G_T7_PICK_LESSON));
    }
    case T7_LESSON: {
      gHeader(t7Name(t7L()));
      int best = gameProgress().tutor7Best[t7L().slot];
      std::vector<String> labels = {T(S_G_T7_PART_SUMMARY), T(S_G_T7_PART_EXAMPLES), best ? TF(S_G_T7_PART_PRACTICE_BEST, best, t7L().nSteps) : String(T(S_G_T7_PART_PRACTICE))};
      int top = gTop() + 18, w = TALL ? W - 32 : min(W - 32, 520), x = (W - w) / 2, y = top;
      for (int i = 0; i < 3; i++) {
        int h = t7ButtonH(labels[i], w, 72);
        t7Button(x, y, w, h, labels[i], T7_PART0 + i, i == 2);
        y += h + 16;
      }
      if (best) {  // Recipes' Delete works the same way: the first tap asks, the second clears
        const char *label = T(gs->t7ResetArmed ? S_G_T7_RESET_SURE : S_G_T7_RESET);
        t7Button(x, y, w, t7ButtonH(label, w, 60), label, T7_RESET, gs->t7ResetArmed);
      }
      return;
    }
    case T7_PAGE: return drawT7Page();
    case T7_QUESTION: return drawT7Question();
    case T7_CLUE: return drawT7Clue();
    case T7_WRONG: return drawT7Wrong();
    case T7_SOLUTION: return drawT7Solution();
    case T7_RESULT: return drawT7Result();
    case T7_SUBJECTS:
      return drawT7Menu(T(S_G_T7_GRADE7), {T(S_G_T7_MATHS_GEOMETRY)}, {T7_SUBJECT0}, T(S_G_T7_PICK_SUBJECT));
    default:
      return drawT7Menu(T(S_G_TUTOR7), {T(S_G_T7_GRADE7)}, {T7_CLASS0}, T(S_G_T7_INTRO));
  }
}
