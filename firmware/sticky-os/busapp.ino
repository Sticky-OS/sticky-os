// The Bus app (issue #22): the next buses at your stop, counting down.
//
// The stops are the bus.stops setting (one or two ATCO codes, or their
// bustimes.org links), set from the setup app. A London stop (490...) asks
// TfL, free and with no key; any other asks NextBuses through TransportAPI
// with the owner's key, which is free for 30 requests a day (bus.h, net.cpp).
//
// It fetches only while it's on screen: TfL every BUS_EVERY_MIN, NextBuses
// every BUS_NEXTBUSES_EVERY_MIN and, to keep some of the day's free requests
// for OK, by itself only BUS_AUTO_PER_DAY times a day. The minutes count down
// from the times it got, redrawn each minute with the clock.
//
// Swiping up pages on from the live times to the timetable's: the rest of
// today, then tomorrow. TfL's comes as each route's whole timetable from the
// stop, read once a day when the second page is first wanted; NextBuses' a
// page at a time (each one a request of the day's 30).
//
//   swipe up / down                later and earlier pages
//   OK, or a tap with one stop     back to now, and fetch now
//   a tap with two stops           the other stop
//   UP / DOWN, sideways swipes     the next app, as everywhere

#define BUS_EVERY_MIN 1
#define BUS_NEXTBUSES_EVERY_MIN 10
static const int BUS_AUTO_PER_DAY = 24;
static const int BUS_DAY_REQUESTS = 30;  // NextBuses' free plan

// busPage and busLastPage are with the other apps' screens in sticky-os.ino.
// The later departures of the stop on screen (from the timetable), under
// dataLock, and the network task's copy it reads the next into. In PSRAM: about 38 KB each.
static BusLater *busLater, *busLaterNext;
static String busLaterError;             // why the timetable couldn't be read; under dataLock
static volatile bool busLaterAsk, busLaterBusy;

RTC_DATA_ATTR static int busShown;          // which stop is on screen, kept across deep sleep
// Each stop's last good reply, under dataLock, and (the last one) where the
// network task reads the next. In PSRAM: about 650 bytes each (busBegin).
static BusBoard *busBoard;
static String busError[BUS_MAX_STOPS];      // why the last try failed, "" when it worked; under dataLock
// NextBuses requests today: kept across deep sleep, lost on a restart.
RTC_DATA_ATTR static long busCountDay;
RTC_DATA_ATTR static int busCount;
static bool busDemoOn;
static const char *BUS_DEMO_CODE = "490DEMO";

static void busBegin() {
  busBoard = (BusBoard *)heap_caps_calloc(BUS_MAX_STOPS + 1, sizeof(BusBoard), MALLOC_CAP_SPIRAM);
  if (!busBoard) busBoard = (BusBoard *)calloc(BUS_MAX_STOPS + 1, sizeof(BusBoard));
  busLater = (BusLater *)heap_caps_calloc(1, sizeof(BusLater), MALLOC_CAP_SPIRAM);
  busLaterNext = (BusLater *)heap_caps_calloc(1, sizeof(BusLater), MALLOC_CAP_SPIRAM);
}

void busRequestCounted() { busCounted(); }

// The stops in the settings (demo mode: one made-up stop).
static int busStops(char codes[][BUS_CODE_MAX]) {
  if (busDemoOn) return strlcpy(codes[0], BUS_DEMO_CODE, BUS_CODE_MAX), 1;
  return busStopCodes(cfg::str("bus.stops").c_str(), codes, BUS_MAX_STOPS);
}

static int busEveryMin(const char *code) { return busSource(code) == BUS_TFL ? BUS_EVERY_MIN : BUS_NEXTBUSES_EVERY_MIN; }

static int busToday() { return busCountDay == localDate(time(nullptr)) ? busCount : 0; }

static void busCounted() {
  long today = localDate(time(nullptr));
  if (busCountDay != today) busCountDay = today, busCount = 0;
  busCount++;
}

// The stop on screen has times recent enough not to ask again on opening.
static bool busFresh() {
  char codes[BUS_MAX_STOPS][BUS_CODE_MAX];
  int n = busStops(codes);
  if (!n) return true;  // nothing to fetch
  int i = constrain(busShown, 0, n - 1);
  xSemaphoreTake(dataLock, portMAX_DELAY);
  const BusBoard &b = busBoard[i];
  bool fresh = b.ok && !strcmp(b.code, codes[i]) && time(nullptr) - b.at < busEveryMin(codes[i]) * 60L;
  xSemaphoreGive(dataLock);
  return fresh;
}

// The later pages' departures are for this stop and today.
static bool busLaterFor(const char *code) {
  return busLater && !strcmp(busLater->code, code) && busLater->date == localDate(time(nullptr));
}

// On the network task: the timetable's next part for the stop on screen.
static void busLaterFetch(int i, const char *code) {
  busLaterAsk = false;
  if (!busLater || !busLaterNext) return;
  busLaterBusy = true;
  fetchDone[APP_BUS] = true;  // the page says it's getting them
  BusLater &next = *busLaterNext;
  xSemaphoreTake(dataLock, portMAX_DELAY);
  bool more = busLaterFor(code);  // NextBuses: go on from where the last page ended
  if (more) memcpy(&next, busLater, sizeof next);
  xSemaphoreGive(dataLock);
  if (!more) {
    memset(&next, 0, sizeof next);
    strlcpy(next.code, code, sizeof next.code);
    next.date = localDate(time(nullptr));
    next.from = time(nullptr);
    next.until = next.from;
  }
  String error;
  bool ok = false;
  if (busSource(code) == BUS_NEXTBUSES && busToday() >= BUS_DAY_REQUESTS) error = T(S_BUS_NO_REQUESTS);
  else ok = fetchBusLater(next, busBoard[i], error);  // busBoard: only this task writes it
  xSemaphoreTake(dataLock, portMAX_DELAY);
  if (ok) std::swap(busLater, busLaterNext);
  busLaterError = error;
  xSemaphoreGive(dataLock);
  busLaterBusy = false;
  fetchDone[APP_BUS] = true;
}

// On the network task: the stop on screen, when it's due or asked for.
static void busNetTick() {
  static uint32_t nextAt;
  static uint8_t fails;
  static char lastCode[BUS_CODE_MAX];
  char codes[BUS_MAX_STOPS][BUS_CODE_MAX];
  int n = busStops(codes);
  if (!n || busDemoOn) {
    fetchAsk[APP_BUS] = false;
    busLaterAsk = false;
    return;
  }
  int i = constrain(busShown, 0, n - 1);
  const char *code = codes[i];
  if (busLaterAsk) return busLaterFetch(i, code);
  bool asked = fetchAsk[APP_BUS], nextBuses = busSource(code) == BUS_NEXTBUSES;
  if (!asked) {
    if (!(screen == SCREEN_APP && app == APP_BUS)) return;
    if (!strcmp(code, lastCode) && nextAt && (int32_t)(millis() - nextAt) < 0) return;
    if (nextBuses && busToday() >= BUS_AUTO_PER_DAY) return;  // the rest are for OK
    if (aiOnNetwork()) return;  // one TLS session at a time; asked for, it goes ahead
  }
  fetchAsk[APP_BUS] = false;
  BusBoard &fresh = busBoard[BUS_MAX_STOPS];  // only this task touches it
  fresh = {};
  strlcpy(fresh.code, code, sizeof fresh.code);
  String error;
  bool counts = nextBuses && !busNeedsKey(code);
  bool ok = fetchBus(fresh, error);
  if (counts) busCounted();
  xSemaphoreTake(dataLock, portMAX_DELAY);
  if (ok) busBoard[i] = fresh;
  else if (strcmp(busBoard[i].code, code)) busBoard[i] = {}, strlcpy(busBoard[i].code, code, BUS_CODE_MAX);  // another stop's: gone
  busError[i] = error;
  xSemaphoreGive(dataLock);
  fails = ok ? 0 : min<uint8_t>(fails + 1, 6);
  nextAt = millis() + (ok ? busEveryMin(code) * 60000UL : backoffMs(fails));
  if (!nextAt) nextAt = 1;
  strlcpy(lastCode, code, sizeof lastCode);
  if (counts) Serial.printf("bus: %d NextBuses requests today\n", busToday());
  fetchDone[APP_BUS] = true;
}

// ---------------------------------------------------------------- drawing

static String busMinutesText(long minutes) {
  return minutes <= 0 ? String(T(S_BUS_DUE)) : TF(S_BUS_MIN, minutes);
}

// A route number in a rounded box, 2 px lines; returns its width.
static int busBadge(int x, int y, int h, const GFXfont *f, const char *line, uint16_t colour, int minW) {
  int w = max(minW, textW(f, line) + 20);
  canvas->drawRoundRect(x, y, w, h, 8, colour);
  canvas->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 7, colour);
  canvas->setTextColor(colour);
  textCentre(x + w / 2, y + h / 2 + (f == &FreeSansBold18pt7b ? 12 : 8), f, line);
  canvas->setTextColor(INK);
  return w;
}

// The card: the next bus, large, in white on black, and the two after it.
static void busCard(int x, int y, int w, int h, const BusBoard &b, const int *idx, int m, time_t now) {
  const BusDep &d = b.dep[idx[0]];
  char line[48];
  canvas->fillRoundRect(x, y, w, h, 14, INK);
  canvas->setTextColor(PAPER);
  const int pad = 18;
  text(x + pad, y + 30, &FreeSansBold9pt7b, T(S_BUS_NEXT_CAPS));
  time_t at = d.at;
  tStrftime(line, sizeof line, T(S_BUS_AT), localtime(&at));
  textRight(x + w - pad, y + 32, &FreeSansBold12pt7b, line);
  int bw = busBadge(x + pad, y + 46, 50, &FreeSansBold18pt7b, d.line, PAPER, 70);
  canvas->setTextColor(PAPER);
  String mins = busMinutesText(busMinutesTo(d.at, now));
  text(x + pad + bw + 16, y + 88, d.live ? &FreeSansBold24pt7b : &FreeSans18pt7b, mins.c_str());
  text(x + pad, y + 132, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, d.dest, w - 2 * pad).c_str());
  if (m > 1) {
    String then = String(busMinutesTo(b.dep[idx[1]].at, now));
    if (m > 2) then += ", " + String(busMinutesTo(b.dep[idx[2]].at, now));
    text(x + pad, y + 166, &FreeSans12pt7b, fitted(&FreeSans12pt7b, TF(S_BUS_THEN, then.c_str()), w - 2 * pad).c_str());
  }
  canvas->setTextColor(INK);
}

// The stop's name (two lines at most) and which way it goes; returns the y below it.
static int busStopBlock(int x, int y, int w, const BusBoard &b, const char *code) {
  auto lines = wrapped(&FreeSansBold12pt7b, b.name[0] ? b.name : code, w);
  for (size_t k = 0; k < lines.size() && k < 2; k++, y += 28) text(x, y, &FreeSansBold12pt7b, lines[k].c_str());
  String sub = b.indicator;
  if (b.towards[0]) sub += String(sub.length() ? ", " : "") + TF(S_BUS_TOWARDS, b.towards);
  if (sub.length()) {
    auto s = wrapped(&FreeSans12pt7b, sub, w);
    for (size_t k = 0; k < s.size() && k < 2; k++, y += 26) text(x, y, &FreeSans12pt7b, s[k].c_str());
  }
  return y;
}

// One departure in the list: the route, where it goes, the minutes (bold:
// live; plain: the timetable's) and the clock time.
static void busRow(int x, int y, int w, int h, const BusDep &d, time_t now) {
  char hm[8];
  time_t at = d.at;
  strftime(hm, sizeof hm, "%H:%M", localtime(&at));
  int cy = y + h / 2;
  int bw = busBadge(x, cy - 17, 34, &FreeSansBold12pt7b, d.line, INK, 62);
  int timeW = textW(&FreeSans12pt7b, "00:00"), minsRight = x + w - timeW - 18;
  String mins = busMinutesText(busMinutesTo(d.at, now));
  const GFXfont *mf = d.live ? &FreeSansBold12pt7b : &FreeSans12pt7b;
  int minsW = max(textW(mf, mins.c_str()), textW(&FreeSansBold12pt7b, "00 min"));
  textRight(minsRight, cy + 8, mf, mins.c_str());
  textRight(x + w, cy + 8, &FreeSans12pt7b, hm);
  int destX = x + bw + 14;
  text(destX, cy + 8, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, d.dest, minsRight - minsW - 12 - destX).c_str());
}

static void drawBus() {
  char codes[BUS_MAX_STOPS][BUS_CODE_MAX];
  int n = busStops(codes);
  if (!n) return drawSetupHint(T(S_BUS_SETUP_TITLE), T(S_BUS_SETUP));
  int i = busShown = constrain(busShown, 0, n - 1);
  const char *code = codes[i];
  if (!busDemoOn && busNeedsKey(code)) return drawSetupHint(T(S_BUS_SETUP_TITLE), T(S_BUS_NEEDS_KEY));
  if (busDemoOn && (time(nullptr) - busBoard[0].at > 300 || !busLaterFor(BUS_DEMO_CODE))) busDemoFill(true);  // render holds dataLock
  const BusBoard &b = busBoard[i];
  bool have = b.ok && !strcmp(b.code, code), nextBuses = busSource(code) == BUS_NEXTBUSES;
  time_t now = time(nullptr);
  int idx[BUS_MAX_DEPS], m = 0;  // the departures still to come
  for (int k = 0; have && k < b.n; k++)
    if (busMinutesTo(b.dep[k].at, now) >= 0) idx[m++] = k;

  if (busPage > 0) return drawBusLater(code, i, n, b, m ? b.dep[idx[m - 1]].at : 0, nextBuses);

  // The foot: that there's more, which stop of two, and where the times come from.
  char foot[64];
  const char *later = T(S_BUS_LATER_HINT), *source = T(nextBuses ? S_BUS_LIVE_NEXTBUSES : S_BUS_LIVE_TFL);
  int laterW = textW(&FreeSans9pt7b, later);
  text(16, H - 12, &FreeSans9pt7b, later);
  textRight(W - 16, H - 12, &FreeSans9pt7b, fitted(&FreeSans9pt7b, source, W - 56 - laterW).c_str());
  if (n > 1) {
    snprintf(foot, sizeof foot, T(S_BUS_STOPS), i + 1, n);
    text(16, H - 34, &FreeSans9pt7b, foot);
  }

  if (!have) {
    BusBoard none = {};
    busStopBlock(TALL ? 16 : 24, BODY_Y + 36, W - 48, none, code);
    const String &why = busError[i];
    textCentre(W / 2, H / 2, &FreeSansBold18pt7b, T(why.length() ? S_BUS_CANT : S_BUS_GETTING));
    if (why.length()) {
      failureLine(H / 2 + 44, why);
      textCentre(W / 2, H / 2 + 80, &FreeSans12pt7b, T(S_PRESS_OK_RETRY));
    }
    return;
  }

  bool stale;
  String upd = updatedText(b.at, busEveryMin(code), &stale);
  // NextBuses: how many of today's free requests are gone, or that it stopped asking by itself.
  String budget;
  bool paused = nextBuses && !busDemoOn && busToday() >= BUS_AUTO_PER_DAY;
  if (nextBuses && !busDemoOn) budget = paused ? String(T(S_BUS_PAUSED)) : TF(S_BUS_TODAY, busToday());
  const GFXfont *budgetFont = paused ? &FreeSansBold9pt7b : &FreeSans9pt7b;

  if (!TALL) {
    // Sideways: the card and the stop on the left, the rest of the list on the right.
    const int leftW = 288, colX = 12 + leftW + 18, colW = W - colX - 16;
    int y = BODY_Y + 12;
    if (m) busCard(12, y, leftW, 186, b, idx, m, now), y += 186 + 32;
    else y += 24;
    y = busStopBlock(20, y, leftW - 8, b, code);
    if (budget.length()) {
      auto lines = wrapped(budgetFont, budget, leftW - 8);
      for (size_t k = 0; k < lines.size() && k < 3 && y < H - 40; k++, y += 20) text(20, y + 6, budgetFont, lines[k].c_str());
    }
    canvas->fillRect(12 + leftW + 8, BODY_Y + 14, 2, H - BODY_Y - 50, INK);
    int hy = BODY_Y + 34;
    text(colX, hy, &FreeSansBold12pt7b, T(S_BUS_DEPARTURES));
    textRight(colX + colW, hy, stale ? &FreeSansBold9pt7b : &FreeSans9pt7b, upd.c_str());
    canvas->fillRect(colX, hy + 10, colW, 2, INK);
    if (!m) {
      textCentre(colX + colW / 2, (hy + H) / 2, &FreeSansBold18pt7b, T(S_BUS_NONE));
      return;
    }
    const int rowH = 52;
    int top = hy + 14, rows = min(m - 1, (H - 40 - top) / rowH);
    for (int r = 0; r < rows; r++) {
      busRow(colX, top + r * rowH, colW, rowH, b.dep[idx[r + 1]], now);
      if (r < rows - 1) canvas->drawFastHLine(colX, top + (r + 1) * rowH, colW, INK);
    }
    return;
  }

  // Upright: the stop, the card, then the list.
  int y = busStopBlock(16, BODY_Y + 32, W - 32, b, code) - 8;
  if (!m) {
    textRight(W - 16, y + 18, stale ? &FreeSansBold9pt7b : &FreeSans9pt7b, upd.c_str());
    textCentre(W / 2, H / 2 + 40, &FreeSansBold18pt7b, T(S_BUS_NONE));
    return;
  }
  busCard(12, y, W - 24, 186, b, idx, m, now);
  y += 186 + 36;
  text(16, y, &FreeSansBold12pt7b, T(S_BUS_DEPARTURES));
  textRight(W - 16, y, stale ? &FreeSansBold9pt7b : &FreeSans9pt7b, upd.c_str());
  canvas->fillRect(16, y + 10, W - 32, 2, INK);
  int footTop = H - (n > 1 ? 60 : 36) - (budget.length() ? 44 : 0);
  if (budget.length()) {
    auto lines = wrapped(budgetFont, budget, W - 32);
    for (size_t k = 0; k < lines.size() && k < 2; k++) text(16, footTop + 22 + k * 20, budgetFont, lines[k].c_str());
  }
  const int rowH = 54;
  int top = y + 14, rows = min(m - 1, (footTop - top) / rowH);
  for (int r = 0; r < rows; r++) {
    busRow(16, top + r * rowH, W - 32, rowH, b.dep[idx[r + 1]], now);
    if (r < rows - 1) canvas->drawFastHLine(16, top + (r + 1) * rowH, W - 32, INK);
  }
}

// The later pages: the timetable's departures after the live ones (after
// `after`, or now), a row each, with the day as a heading where it changes.
static void drawBusLater(const char *code, int i, int n, const BusBoard &b, time_t after, bool nextBuses) {
  time_t now = time(nullptr);
  char line[64];
  const int top = BODY_Y + 46, rowH = 44, bottom = H - 30, slots = max(2, (bottom - top) / rowH);
  text(16, BODY_Y + 30, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, b.name[0] ? b.name : code, W - 140).c_str());
  canvas->fillRect(16, BODY_Y + 40, W - 32, 2, INK);
  const char *source = T(nextBuses ? S_BUS_TIMETABLE_NEXTBUSES : S_BUS_TIMETABLE_TFL);
  String right = nextBuses && !busDemoOn ? TF(S_BUS_TODAY, busToday()) : String(source);
  text(16, H - 12, &FreeSans9pt7b, T(S_BUS_PAGE_HINT));
  textRight(W - 16, H - 12, &FreeSans9pt7b, fitted(&FreeSans9pt7b, right, W - 40 - textW(&FreeSans9pt7b, T(S_BUS_PAGE_HINT))).c_str());

  if (!busLaterFor(code)) {  // not read yet today: ask, and say so (or why it failed)
    busLastPage = -1;
    bool failed = busLaterError.length() && !busLaterBusy && !busLaterAsk;
    if (!failed && !busLaterBusy) busLaterAsk = true;
    textCentre(W / 2, H / 2, &FreeSansBold18pt7b, T(failed ? S_BUS_CANT_LATER : S_BUS_GETTING_LATER));
    if (failed) {
      failureLine(H / 2 + 44, busLaterError);
      textCentre(W / 2, H / 2 + 80, &FreeSans12pt7b, T(S_BUS_SWIPE_RETRY));
    }
    return;
  }
  const BusLater &l = *busLater;
  int first = 0;
  time_t cut = max(after, now);
  while (first < l.n && l.dep[first].at <= cut) first++;

  // Lay the pages out: a heading at the top of each and where the day changes.
  int page = 1, pageStart = first, used = 0, shownStart = -1, shownEnd = -1;
  long day = 0;
  for (int k = first; k <= l.n; k++) {
    bool end = k == l.n;
    long d = end ? 0 : localDate(l.dep[k].at);
    int need = end ? 0 : (used == 0 || d != day ? 2 : 1);  // a heading and the row, or the row
    if (!end && used + need > slots) {  // the next page
      if (page == busPage) shownStart = pageStart, shownEnd = k;
      page++, pageStart = k, used = 0, need = 2;
    }
    if (end) {
      if (page == busPage || (busPage > page && l.complete)) shownStart = pageStart, shownEnd = k, busPage = page;
      break;
    }
    used += need;
    day = d;
  }
  busLastPage = l.complete ? page : -1;
  snprintf(line, sizeof line, l.complete ? "%d / %d" : "%d", busPage + 1, page + 1);
  textRight(W - 16, BODY_Y + 30, &FreeSans9pt7b, line);
  if (shownStart < 0) {  // past what's read: NextBuses' next page
    if (!busLaterBusy && !busLaterAsk && !busLaterError.length()) busLaterAsk = true;
    bool failed = busLaterError.length() && !busLaterBusy;
    textCentre(W / 2, H / 2, &FreeSansBold18pt7b, T(failed ? S_BUS_CANT_LATER : S_BUS_GETTING_LATER));
    if (failed) failureLine(H / 2 + 44, busLaterError), textCentre(W / 2, H / 2 + 80, &FreeSans12pt7b, T(S_BUS_SWIPE_RETRY));
    return;
  }
  int y = top;
  day = 0;
  for (int k = shownStart; k < shownEnd; k++) {
    const BusDep &d = l.dep[k];
    long dd = localDate(d.at);
    if (k == shownStart || dd != day) {  // the day's heading
      time_t at = d.at;
      if (dd == localDate(now)) strlcpy(line, T(S_BUS_LATER_TODAY), sizeof line);
      else tStrftime(line, sizeof line, T(S_BUS_TOMORROW), localtime(&at));
      text(16, y + 30, &FreeSansBold9pt7b, upperText(line).c_str());
      y += rowH;
      day = dd;
    }
    char hm[8];
    time_t at = d.at;
    strftime(hm, sizeof hm, "%H:%M", localtime(&at));
    int cy = y + rowH / 2;
    int bw = busBadge(16, cy - 16, 32, &FreeSansBold12pt7b, d.line, INK, 62);
    int timeW = textW(&FreeSansBold12pt7b, "00:00");
    textRight(W - 16, cy + 8, &FreeSansBold12pt7b, hm);
    text(16 + bw + 14, cy + 8, &FreeSansBold12pt7b, fitted(&FreeSansBold12pt7b, d.dest, W - 16 - timeW - 20 - (16 + bw + 14)).c_str());
    canvas->drawFastHLine(16, y + rowH, W - 32, INK);
    y += rowH;
  }
  if (shownEnd < l.n || y + rowH > bottom) return;
  if (l.complete) {  // the last page: nothing after it
    textCentre(W / 2, y + 30, &FreeSans12pt7b, T(S_BUS_END));
  } else {           // NextBuses: the page isn't full, so read on
    if (!busLaterBusy && !busLaterAsk && !busLaterError.length()) busLaterAsk = true;
    textCentre(W / 2, y + 30, &FreeSans12pt7b, busLaterError.length() && !busLaterBusy ? busLaterError.c_str() : T(S_BUS_GETTING_LATER));
  }
}

// Up and down: the later and earlier pages. True when it was one.
static bool busSwipe(int dx, int dy) {
  if (abs(dx) >= abs(dy)) return false;
  if (dy < 0) {
    if (busLastPage >= 0 && busPage >= busLastPage) return true;  // the last page
    busPage++;
    xSemaphoreTake(dataLock, portMAX_DELAY);
    if (!busLaterBusy) busLaterError = "";  // a swipe after a failure tries again
    xSemaphoreGive(dataLock);
  } else {
    if (!busPage) return true;
    busPage--;
  }
  Serial.printf("bus: page %d\n", busPage + 1);
  render(false);
  return true;
}

// The app's icon: a bus from the front, its windscreen, lights and wheels.
static void busIcon(int cx, int cy, int s, int t) {
  int w = s * 62 / 100, h = s * 72 / 100, x = cx - w / 2, y = cy - h / 2 - s / 20;
  for (int k = 0; k < t; k++) canvas->drawRoundRect(x + k, y + k, w - 2 * k, h - 2 * k, max(2, s / 9 - k), INK);
  int gx = x + t + 4, gw = w - 2 * (t + 4);
  canvas->fillRoundRect(gx, y + t + 5, gw, h * 45 / 100, 3, INK);  // the windscreen
  int ly = y + h - t - s / 8;
  canvas->fillCircle(x + t + s / 10, ly, max(2, s / 18), INK);   // the lights
  canvas->fillCircle(x + w - t - s / 10 - 1, ly, max(2, s / 18), INK);
  int ww = max(4, s / 9), wh = max(4, s / 10);
  canvas->fillRect(x + s / 14, y + h, ww, wh, INK);  // the wheels
  canvas->fillRect(x + w - s / 14 - ww, y + h, ww, wh, INK);
}

// A tap: the other stop when there are two, else fetch now.
static void busTap(int x, int y) {
  char codes[BUS_MAX_STOPS][BUS_CODE_MAX];
  int n = busStops(codes);
  if (n < 2) return action();
  busShown = (busShown + 1) % n;
  busPage = 0, busLastPage = -1;
  Serial.printf("bus: stop %d of %d, %s\n", busShown + 1, n, codes[busShown]);
  fetchFor(APP_BUS, false);
  render(false);
}

// Demo mode (screenshots): a made-up stop with buses in the next half hour,
// made again when it's gone stale (demo mode turned on before the clock was
// set, or left on). Under dataLock.
static void busDemoFill(bool on) {
  busBoard[0] = {};
  busError[0] = "";
  if (on) {
    BusBoard &b = busBoard[0];
    strlcpy(b.code, BUS_DEMO_CODE, sizeof b.code);
    strlcpy(b.name, "High Street", sizeof b.name);
    strlcpy(b.indicator, "B", sizeof b.indicator);
    strlcpy(b.towards, "Town Centre", sizeof b.towards);
    static const struct { const char *line, *dest; int minutes; bool live; } DEMO[] = {
        {"7", "Town Centre", 2, true},  {"12", "Hospital", 6, true},     {"7", "Town Centre", 11, true},
        {"12A", "Riverside", 15, true}, {"7", "Town Centre", 19, false}, {"12", "Hospital", 24, false}};
    time_t t = time(nullptr) + 20;
    for (auto &d : DEMO) {
      BusDep &x = b.dep[b.n++];
      strlcpy(x.line, d.line, sizeof x.line);
      strlcpy(x.dest, d.dest, sizeof x.dest);
      x.at = t + d.minutes * 60;
      x.live = d.live;
    }
    b.at = time(nullptr);
    b.ok = true;
    if (busLater) {  // and a made-up timetable: every 12 minutes, to the end of tomorrow
      BusLater &l = *busLater;
      memset(&l, 0, sizeof l);
      strlcpy(l.code, BUS_DEMO_CODE, sizeof l.code);
      l.date = localDate(t);
      struct tm end = *localtime(&t);
      end.tm_mday += 2, end.tm_hour = end.tm_min = end.tm_sec = 0, end.tm_isdst = -1;
      time_t stop = mktime(&end);
      static const char *const LINES[] = {"7", "12", "7", "12A"}, *const DESTS[] = {"Town Centre", "Hospital", "Town Centre", "Riverside"};
      int k = 0;
      for (time_t at = t + 27 * 60; at < stop && l.n < BUS_LATER_MAX; at += 12 * 60, k++) {
        struct tm h = *localtime(&at);
        if (h.tm_hour < 6) continue;  // no buses at night
        BusDep &x = l.dep[l.n++];
        strlcpy(x.line, LINES[k % 4], sizeof x.line);
        strlcpy(x.dest, DESTS[k % 4], sizeof x.dest);
        x.at = at;
      }
      l.complete = true;
    }
  } else if (busLater) {
    memset(busLater, 0, sizeof *busLater);
  }
}

static void busDemo(bool on) {
  busDemoOn = on;
  xSemaphoreTake(dataLock, portMAX_DELAY);
  busDemoFill(on);
  xSemaphoreGive(dataLock);
}

// The serial "bus" command: the stops and what each last got, or
// "bus next": the other stop, as a tap does.
static void busCommand(String args) {
  args.trim();
  char codes[BUS_MAX_STOPS][BUS_CODE_MAX];
  int n = busStops(codes);
  if (args == "up" || args == "down") busSwipe(0, args == "up" ? -100 : 100);
  if (args == "next") {
    if (n > 1) busShown = (busShown + 1) % n, busPage = 0, busLastPage = -1;
    if (screen == SCREEN_APP && app == APP_BUS) fetchFor(APP_BUS, false), render(false);
  }
  Serial.printf("bus: %d stops, showing %d; %d NextBuses requests today\n", n, n ? constrain(busShown, 0, n - 1) + 1 : 0, busToday());
  xSemaphoreTake(dataLock, portMAX_DELAY);
  for (int i = 0; i < n; i++) {
    const BusBoard &b = busBoard[i];
    bool have = b.ok && !strcmp(b.code, codes[i]);
    Serial.printf("bus: %d %s %s%s, %s", i + 1, codes[i], busSource(codes[i]) == BUS_TFL ? "TfL" : "NextBuses",
                  busNeedsKey(codes[i]) && !busDemoOn ? " (no TransportAPI key)" : "", have ? b.name : "nothing yet");
    if (have) Serial.printf(", %d departures, %ld s old", b.n, (long)(time(nullptr) - b.at));
    if (busError[i].length()) Serial.printf(", error: %s", busError[i].c_str());
    Serial.println();
    for (int k = 0; have && k < b.n; k++) {
      char hm[8];
      time_t at = b.dep[k].at;
      strftime(hm, sizeof hm, "%H:%M", localtime(&at));
      Serial.printf("bus:   %s %s %s %s\n", b.dep[k].line, hm, b.dep[k].live ? "live" : "timetable", b.dep[k].dest);
    }
  }
  xSemaphoreGive(dataLock);
}
