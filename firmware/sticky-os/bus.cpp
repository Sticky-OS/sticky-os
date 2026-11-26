// The Bus app's parsers (bus.h). They read just the fields they need from
// the services' JSON, without building a tree: a reply is walked once, an
// object at a time, and nothing is allocated.
#include "bus.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------- stops

int busStopCodes(const char *setting, char codes[][BUS_CODE_MAX], int max) {
  int n = 0;
  if (!setting) return 0;
  const char *p = setting;
  while (*p && n < max) {
    while (*p && (isspace((unsigned char)*p) || *p == ',' || *p == ';')) p++;
    const char *start = p;
    while (*p && !isspace((unsigned char)*p) && *p != ',' && *p != ';') p++;
    const char *end = p;
    if (start == end) break;
    // A bustimes.org address: the part after /stops/, up to the next / or ?.
    for (const char *s = start; s + 7 <= end; s++)
      if (!strncmp(s, "/stops/", 7)) {
        start = s + 7;
        const char *e = start;
        while (e < end && *e != '/' && *e != '?' && *e != '#') e++;
        end = e;
        break;
      }
    size_t len = end - start;
    bool good = len >= 3 && len <= 20;
    for (const char *s = start; good && s < end; s++) good = isalnum((unsigned char)*s);
    if (!good) continue;
    bool seen = false;
    for (int i = 0; i < n && !seen; i++) seen = strlen(codes[i]) == len && !strncmp(codes[i], start, len);
    if (seen) continue;
    memcpy(codes[n], start, len);
    codes[n][len] = 0;
    n++;
  }
  return n;
}

BusSource busSource(const char *code) { return code && !strncmp(code, "490", 3) ? BUS_TFL : BUS_NEXTBUSES; }

long busMinutesTo(time_t at, time_t now) {
  long s = (long)(at - now);
  return s >= 0 ? s / 60 : -((-s + 59) / 60);
}

// ---------------------------------------------------------------- times

static long long daysFromCivil(int y, int m, int d) {  // days since 1970-01-01 (Howard Hinnant's)
  y -= m <= 2;
  long long era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long long)doe - 719468;
}

static bool digits(const char *s, int n, int &out) {
  out = 0;
  for (int i = 0; i < n; i++) {
    if (!isdigit((unsigned char)s[i])) return false;
    out = out * 10 + (s[i] - '0');
  }
  return true;
}

time_t busUtcTime(const char *s) {
  int y, mo, d, h, mi, se;
  if (!s || strlen(s) < 19 || s[4] != '-' || s[7] != '-' || (s[10] != 'T' && s[10] != ' ') || s[13] != ':' || s[16] != ':') return 0;
  if (!digits(s, 4, y) || !digits(s + 5, 2, mo) || !digits(s + 8, 2, d) || !digits(s + 11, 2, h) || !digits(s + 14, 2, mi) ||
      !digits(s + 17, 2, se))
    return 0;
  if (mo < 1 || mo > 12 || d < 1 || d > 31 || h > 23 || mi > 59 || se > 60) return 0;
  return (time_t)(daysFromCivil(y, mo, d) * 86400LL + h * 3600 + mi * 60 + se);
}

static long long lastSundayAt1Utc(int y, int m) {  // the last Sunday of month m, 01:00 UTC
  int last = m == 3 || m == 10 ? 31 : 30;
  long long day = daysFromCivil(y, m, last);
  long long weekday = (day % 7 + 7 + 4) % 7;  // 1970-01-01 was a Thursday (4); Sunday is 0
  return (day - weekday) * 86400LL + 3600;
}

time_t busUkTime(int y, int mo, int d, int h, int mi) {
  long long asUtc = daysFromCivil(y, mo, d) * 86400LL + h * 3600 + mi * 60;
  long long probe = asUtc - 3600;  // in BST the clock is an hour ahead of UTC
  bool bst = probe >= lastSundayAt1Utc(y, 3) && probe < lastSundayAt1Utc(y, 10);
  return (time_t)(asUtc - (bst ? 3600 : 0));
}

// ---------------------------------------------------------------- JSON, read in place

// The end of the value starting at s[i] (a string, an object, an array or a
// plain word or number): the index just after it; -1 when it runs off the end.
static long skipValue(const char *s, long i) {
  if (s[i] == '"') {
    for (i++; s[i]; i++) {
      if (s[i] == '\\' && s[i + 1]) i++;
      else if (s[i] == '"') return i + 1;
    }
    return -1;
  }
  if (s[i] == '{' || s[i] == '[') {
    int depth = 0;
    for (; s[i]; i++) {
      if (s[i] == '"') {
        long e = skipValue(s, i);
        if (e < 0) return -1;
        i = e - 1;
      } else if (s[i] == '{' || s[i] == '[') depth++;
      else if ((s[i] == '}' || s[i] == ']') && --depth == 0) return i + 1;
    }
    return -1;
  }
  while (s[i] && s[i] != ',' && s[i] != '}' && s[i] != ']' && !isspace((unsigned char)s[i])) i++;
  return i;
}

static long skipSpace(const char *s, long i) {
  while (s[i] && isspace((unsigned char)s[i])) i++;
  return i;
}

static void putUtf8(char *out, size_t size, size_t &n, uint32_t cp) {
  char b[4];
  int len;
  if (cp < 0x80) b[0] = (char)cp, len = 1;
  else if (cp < 0x800) b[0] = (char)(0xC0 | cp >> 6), b[1] = (char)(0x80 | (cp & 0x3F)), len = 2;
  else if (cp < 0x10000) b[0] = (char)(0xE0 | cp >> 12), b[1] = (char)(0x80 | (cp >> 6 & 0x3F)), b[2] = (char)(0x80 | (cp & 0x3F)), len = 3;
  else b[0] = (char)(0xF0 | cp >> 18), b[1] = (char)(0x80 | (cp >> 12 & 0x3F)), b[2] = (char)(0x80 | (cp >> 6 & 0x3F)), b[3] = (char)(0x80 | (cp & 0x3F)), len = 4;
  if (n + len >= size) return;  // a letter that doesn't fit whole is left out, never cut
  memcpy(out + n, b, len);
  n += len;
}

// The string starting at s[i] (its opening quote), unescaped into `out`,
// cut at a whole letter to fit.
static void readString(const char *s, long i, char *out, size_t size) {
  size_t n = 0;
  for (i++; s[i] && s[i] != '"'; i++) {
    uint32_t cp = (unsigned char)s[i];
    if (s[i] == '\\' && s[i + 1]) {
      char e = s[++i];
      if (e == 'n' || e == 't' || e == 'r') cp = ' ';
      else if (e == 'b' || e == 'f') continue;
      else if (e == 'u') {
        char hex[5] = {0};
        for (int k = 0; k < 4 && s[i + 1]; k++) hex[k] = s[++i];
        cp = (uint32_t)strtoul(hex, nullptr, 16);
        if (cp >= 0xD800 && cp < 0xDC00 && s[i + 1] == '\\' && s[i + 2] == 'u') {  // a surrogate pair
          char lo[5] = {s[i + 3], s[i + 4], s[i + 5], s[i + 6], 0};
          uint32_t low = (uint32_t)strtoul(lo, nullptr, 16);
          if (low >= 0xDC00 && low < 0xE000) cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00), i += 6;
        }
        if (cp >= 0xD800 && cp < 0xE000) cp = '?';
      } else cp = (unsigned char)e;
      putUtf8(out, size, n, cp);
      continue;
    }
    if (n + 1 < size) out[n++] = s[i];
  }
  // A multi-byte letter cut by the copy above: drop its start.
  size_t k = n;
  while (k > 0 && ((unsigned char)out[k - 1] & 0xC0) == 0x80) k--;
  if (k > 0 && ((unsigned char)out[k - 1] & 0x80)) {
    unsigned char lead = (unsigned char)out[k - 1];
    size_t need = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
    if (n - (k - 1) < need) n = k - 1;
  }
  if (size) out[n < size ? n : size - 1] = 0;
}

// The members of the object starting at s[i]: calls f(key start, value
// index) for each, at this level only. False when it isn't a whole object.
template <typename F>
static bool eachMember(const char *s, long i, F f) {
  if (s[i] != '{') return false;
  i = skipSpace(s, i + 1);
  if (s[i] == '}') return true;
  for (;;) {
    if (s[i] != '"') return false;
    long key = i;
    long e = skipValue(s, i);
    if (e < 0) return false;
    i = skipSpace(s, e);
    if (s[i] != ':') return false;
    i = skipSpace(s, i + 1);
    long v = i;
    e = skipValue(s, i);
    if (e < 0) return false;
    f(s + key + 1, v);
    i = skipSpace(s, e);
    if (s[i] == '}') return true;
    if (s[i] != ',') return false;
    i = skipSpace(s, i + 1);
  }
}

static bool keyIs(const char *key, const char *name) {
  size_t n = strlen(name);
  return !strncmp(key, name, n) && key[n] == '"';
}

// The string member `name` of the object at s[i]; false when it's missing,
// null or not a string.
static bool stringMember(const char *s, long i, const char *name, char *out, size_t size) {
  bool found = false;
  eachMember(s, i, [&](const char *key, long v) {
    if (!found && keyIs(key, name) && s[v] == '"') readString(s, v, out, size), found = true;
  });
  return found;
}

// The value of member `name` of the object at s[i]: its index, -1 if none.
static long member(const char *s, long i, const char *name) {
  long at = -1;
  eachMember(s, i, [&](const char *key, long v) {
    if (at < 0 && keyIs(key, name)) at = v;
  });
  return at;
}

// Each element of the array at s[i]: f(its index). False when it isn't one.
template <typename F>
static bool eachElement(const char *s, long i, F f) {
  if (s[i] != '[') return false;
  i = skipSpace(s, i + 1);
  if (s[i] == ']') return true;
  for (;;) {
    long e = skipValue(s, i);
    if (e < 0) return false;
    f(i);
    i = skipSpace(s, e);
    if (s[i] == ']') return true;
    if (s[i] != ',') return false;
    i = skipSpace(s, i + 1);
  }
}

// ---------------------------------------------------------------- departures

// Keeps the soonest `cap`, sorted: a new one goes in its place, the latest
// falls off. `dedupe`: the same route at the same time is there once.
static bool insertDep(BusDep *dep, int &n, int cap, const BusDep &d, bool dedupe) {
  int at = n;
  while (at > 0 && dep[at - 1].at > d.at) at--;
  if (dedupe)
    for (int k = at - 1; k >= 0 && dep[k].at == d.at; k--)
      if (!strcmp(dep[k].line, d.line)) return false;
  if (at >= cap) return false;
  int last = n < cap ? n : cap - 1;
  for (int k = last; k > at; k--) dep[k] = dep[k - 1];
  dep[at] = d;
  if (n < cap) n++;
  return true;
}

static void addDeparture(BusBoard &b, const BusDep &d) { insertDep(b.dep, b.n, BUS_MAX_DEPS, d, false); }

static bool addLater(BusLater &out, const BusDep &d) {
  if (d.at < out.from) return false;
  return insertDep(out.dep, out.n, BUS_LATER_MAX, d, true);
}

static void startBoard(BusBoard &b) {
  char code[BUS_CODE_MAX];
  memcpy(code, b.code, sizeof code);
  memset(&b, 0, sizeof b);
  memcpy(b.code, code, sizeof code);
}

bool busParseTfl(const char *json, BusBoard &b) {
  startBoard(b);
  if (!json) return false;
  long i = skipSpace(json, 0);
  bool named = false;
  bool whole = eachElement(json, i, [&](long o) {
    if (json[o] != '{') return;
    BusDep d = {};
    char when[40] = "";
    if (!stringMember(json, o, "lineName", d.line, sizeof d.line)) return;
    stringMember(json, o, "destinationName", d.dest, sizeof d.dest);
    stringMember(json, o, "expectedArrival", when, sizeof when);
    d.at = busUtcTime(when);
    if (!d.at) return;
    d.live = true;  // TfL's arrivals are all predictions
    if (!named) {
      named = stringMember(json, o, "stationName", b.name, sizeof b.name);
      stringMember(json, o, "platformName", b.indicator, sizeof b.indicator);
      stringMember(json, o, "towards", b.towards, sizeof b.towards);
    }
    addDeparture(b, d);
  });
  return b.ok = whole;
}

static bool hhmm(const char *s, int &h, int &m) {
  return strlen(s) >= 5 && s[2] == ':' && digits(s, 2, h) && digits(s + 3, 2, m) && h < 24 && m < 60;
}

static bool ymd(const char *s, int &y, int &mo, int &d) {
  return strlen(s) >= 10 && s[4] == '-' && s[7] == '-' && digits(s, 4, y) && digits(s + 5, 2, mo) && digits(s + 8, 2, d);
}

// Whether a stop_timetables departure is cancelled: its
// "status":{"cancellation":{"value":true}}.
static bool cancelled(const char *json, long o) {
  long st = member(json, o, "status");
  if (st < 0 || json[st] != '{') return false;
  long c = member(json, st, "cancellation");
  if (c < 0 || json[c] != '{') return false;
  long v = member(json, c, "value");
  return v >= 0 && !strncmp(json + v, "true", 4);
}

// One departure of a NextBuses reply: false when it has no route or no
// usable time, or is cancelled.
static bool nextBusesDep(const char *json, long o, BusLocalTime local, BusDep &d) {
  d = {};
  if (json[o] != '{' || cancelled(json, o)) return false;
  char date[16] = "", time[8] = "", liveDate[16] = "", liveTime[8] = "";
  if (!stringMember(json, o, "line_name", d.line, sizeof d.line)) stringMember(json, o, "line", d.line, sizeof d.line);
  if (!d.line[0]) return false;
  stringMember(json, o, "direction", d.dest, sizeof d.dest);
  stringMember(json, o, "date", date, sizeof date);
  stringMember(json, o, "aimed_departure_time", time, sizeof time);
  stringMember(json, o, "expected_departure_date", liveDate, sizeof liveDate);
  stringMember(json, o, "expected_departure_time", liveTime, sizeof liveTime);
  int y, mo, dd, h, mi;
  if (liveTime[0] && hhmm(liveTime, h, mi) && (ymd(liveDate, y, mo, dd) || ymd(date, y, mo, dd))) {
    d.live = true;
  } else {
    if (!time[0]) stringMember(json, o, "best_departure_estimate", time, sizeof time);  // TransportAPI's best guess
    if (!hhmm(time, h, mi) || !ymd(date, y, mo, dd)) return false;
  }
  d.at = local(y, mo, dd, h, mi);
  return d.at != 0;
}

bool busParseNextBuses(const char *json, BusLocalTime local, BusBoard &b) {
  startBoard(b);
  if (!json || !local) return false;
  long root = skipSpace(json, 0);
  if (json[root] != '{') return false;
  if (!stringMember(json, root, "stop_name", b.name, sizeof b.name)) stringMember(json, root, "name", b.name, sizeof b.name);
  stringMember(json, root, "indicator", b.indicator, sizeof b.indicator);
  long deps = member(json, root, "departures");
  if (deps < 0 || json[deps] != '{') return false;
  long all = member(json, deps, "all");
  if (all < 0) return false;
  bool whole = eachElement(json, all, [&](long o) {
    BusDep d;
    if (nextBusesDep(json, o, local, d)) addDeparture(b, d);
  });
  return b.ok = whole;
}

// departures.all of a NextBuses reply: its index, -1 when it has none.
static long nextBusesAll(const char *json) {
  long root = skipSpace(json, 0);
  if (json[root] != '{') return -1;
  long deps = member(json, root, "departures");
  if (deps < 0 || json[deps] != '{') return -1;
  long all = member(json, deps, "all");
  return all >= 0 && json[all] == '[' ? all : -1;
}

bool busAddNextBuses(const char *json, BusLocalTime local, BusLater &out, int *added) {
  if (added) *added = 0;
  if (!json || !local) return false;
  long all = nextBusesAll(json);
  if (all < 0) return false;
  return eachElement(json, all, [&](long o) {
    BusDep d;
    if (nextBusesDep(json, o, local, d) && addLater(out, d) && added) (*added)++;
  });
}

// ---------------------------------------------------------------- TfL's timetables

int busTflLines(const char *json, char ids[][12], int max) {
  if (!json) return 0;
  long root = skipSpace(json, 0);
  long lines = json[root] == '{' ? member(json, root, "lines") : -1;
  if (lines < 0) return 0;
  int n = 0;
  eachElement(json, lines, [&](long o) {
    char id[12] = "";
    if (n >= max || json[o] != '{' || !stringMember(json, o, "id", id, sizeof id) || !id[0]) return;
    for (int k = 0; k < n; k++)
      if (!strcmp(ids[k], id)) return;
    memcpy(ids[n++], id, sizeof id);
  });
  return n;
}

static const char *const DAY_NAMES[] = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};

bool busScheduleCovers(const char *name, int weekday) {
  // The days the name says, in order, and whether "to" joins two of them.
  // A night's name counts its first day only: "Friday Night/Saturday Morning" is Friday.
  int days[8], nd = 0;
  bool range = false;
  char word[16];
  for (const char *p = name; p && *p;) {
    while (*p && !isalpha((unsigned char)*p)) p++;
    size_t len = 0;
    while (isalpha((unsigned char)*p)) {
      if (len + 1 < sizeof word) word[len++] = (char)tolower((unsigned char)*p);
      p++;
    }
    word[len] = 0;
    if (!len) break;
    if (!strcmp(word, "to") && nd) range = true;
    if (!strcmp(word, "morning") && nd && nd < 8) nd--;  // "... Night/Saturday Morning": not Saturday's
    for (int k = 0; k < 7; k++)
      if (!strcmp(word, DAY_NAMES[k]) && nd < 8) days[nd++] = k;
  }
  if (!nd) return false;
  if (range && nd >= 2) {  // Monday to Thursday; Sunday to Saturday wraps
    for (int k = days[0];; k = (k + 1) % 7) {
      if (k == weekday) return true;
      if (k == days[nd - 1]) break;
    }
    return false;
  }
  for (int k = 0; k < nd; k++)
    if (days[k] == weekday) return true;
  return false;
}

bool busAddTflTimetable(const char *json, int y, int m, int d, int weekday, const char *dest, BusLocalTime local, BusLater &out) {
  if (!json || !local) return false;
  long root = skipSpace(json, 0);
  if (json[root] != '{') return false;
  char line[8] = "";
  if (!stringMember(json, root, "lineName", line, sizeof line) || !line[0]) return false;
  long tt = member(json, root, "timetable");
  long routes = tt >= 0 && json[tt] == '{' ? member(json, tt, "routes") : -1;
  if (routes < 0 || json[routes] != '[') return false;
  char where[44] = "";
  if (dest && dest[0]) {
    snprintf(where, sizeof where, "%s", dest);
  } else {  // the last stop of the first route, by name
    char last[24] = "";
    eachElement(json, routes, [&](long r) {
      if (last[0]) return;
      long si = member(json, r, "stationIntervals");
      if (si < 0) return;
      eachElement(json, si, [&](long s) {
        if (last[0]) return;
        long iv = member(json, s, "intervals");
        if (iv >= 0) eachElement(json, iv, [&](long i) { stringMember(json, i, "stopId", last, sizeof last); });
      });
    });
    long stops = member(json, root, "stops");
    if (last[0] && stops >= 0)
      eachElement(json, stops, [&](long o) {
        char id[24] = "";
        if (!where[0] && stringMember(json, o, "id", id, sizeof id) && !strcmp(id, last)) stringMember(json, o, "name", where, sizeof where);
      });
  }
  bool whole = eachElement(json, routes, [&](long r) {
    long schedules = member(json, r, "schedules");
    if (schedules < 0) return;
    eachElement(json, schedules, [&](long s) {
      char name[64] = "";
      stringMember(json, s, "name", name, sizeof name);
      long journeys = member(json, s, "knownJourneys");
      if (journeys < 0) return;
      for (int off = -1; off <= 1; off++) {  // the schedule as yesterday's, today's and tomorrow's
        if (!busScheduleCovers(name, (weekday + off + 7) % 7)) continue;
        eachElement(json, journeys, [&](long j) {
          char hs[8] = "", ms[8] = "";
          if (!stringMember(json, j, "hour", hs, sizeof hs) || !stringMember(json, j, "minute", ms, sizeof ms)) return;
          int h = atoi(hs), mi = atoi(ms);
          int day = off + h / 24;  // past midnight: the next day
          if (day < 0 || day > 1 || h < 0 || h > 47 || mi < 0 || mi > 59) return;
          BusDep dep = {};
          memcpy(dep.line, line, sizeof dep.line);
          memcpy(dep.dest, where, sizeof dep.dest);
          dep.at = local(y, m, d + off, h, mi);  // d + off and h past 23 run on (busUkTime, mktime)
          if (dep.at) addLater(out, dep);
        });
      }
    });
  });
  return whole;
}
