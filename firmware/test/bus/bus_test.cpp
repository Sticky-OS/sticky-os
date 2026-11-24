// The Bus app's parsers (bus.h): stop codes, TfL's arrivals, NextBuses'
// departures. fixtures/tfl-arrivals.json is a real TfL reply (24 Nov 2026,
// Lady Somerset Road, stop GY). fixtures/nextbuses-live.json is written by
// hand in the shape of TransportAPI's bus/stop/<code>/live.json, with the
// Accrington stop's timetable that day: no TransportAPI key was at hand.
#include "bus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

static int failed, checks;
#define CHECK(c)                                                       \
  do {                                                                 \
    checks++;                                                          \
    if (!(c)) failed++, printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
  } while (0)

static std::string readFile(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    printf("FAIL can't open %s\n", path);
    exit(1);
  }
  std::string s;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) s.append(buf, n);
  fclose(f);
  return s;
}

static time_t londonTime(int y, int mo, int d, int h, int mi) {
  struct tm tm = {};
  tm.tm_year = y - 1900, tm.tm_mon = mo - 1, tm.tm_mday = d, tm.tm_hour = h, tm.tm_min = mi, tm.tm_isdst = -1;
  return mktime(&tm);
}

static void stopCodes() {
  char c[4][BUS_CODE_MAX];
  CHECK(busStopCodes("490008660N", c, 2) == 1 && !strcmp(c[0], "490008660N"));
  CHECK(busStopCodes(" 2500717 , 490008660N\n", c, 2) == 2 && !strcmp(c[0], "2500717") && !strcmp(c[1], "490008660N"));
  CHECK(busStopCodes("https://bustimes.org/stops/2500717", c, 2) == 1 && !strcmp(c[0], "2500717"));
  CHECK(busStopCodes("bustimes.org/stops/2500IMG1735/departures?x=1", c, 2) == 1 && !strcmp(c[0], "2500IMG1735"));
  CHECK(busStopCodes("a b 12 x-y 2500717", c, 2) == 1 && !strcmp(c[0], "2500717"));  // too short, or not letters and digits
  CHECK(busStopCodes("2500717 2500717 490008660N", c, 2) == 2);                       // the same twice counts once
  CHECK(busStopCodes("1111 2222 3333", c, 2) == 2);                                   // at most max
  CHECK(busStopCodes("", c, 2) == 0 && busStopCodes(nullptr, c, 2) == 0);
  CHECK(busSource("490008660N") == BUS_TFL && busSource("2500717") == BUS_NEXTBUSES);
}

static void times() {
  CHECK(busUtcTime("1970-01-01T00:00:00Z") == 0);
  CHECK(busUtcTime("2026-11-24T20:22:03Z") == 1795551723);
  CHECK(busUtcTime("2026-11-24T20:22:03.123Z") == 1795551723);
  CHECK(busUtcTime("2026-11-24") == 0 && busUtcTime("not a time at all!!") == 0 && busUtcTime(nullptr) == 0);
  // The UK's clocks, against the computer's own Europe/London.
  CHECK(busUkTime(2026, 11, 24, 20, 41) == londonTime(2026, 11, 24, 20, 41));  // GMT
  CHECK(busUkTime(2026, 7, 1, 8, 0) == londonTime(2026, 7, 1, 8, 0));          // BST
  CHECK(busUkTime(2026, 3, 29, 0, 59) == londonTime(2026, 3, 29, 0, 59));      // the last minute of GMT in spring
  CHECK(busUkTime(2026, 3, 29, 2, 30) == londonTime(2026, 3, 29, 2, 30));      // BST (01:00-02:00 doesn't exist)
  CHECK(busUkTime(2026, 10, 25, 0, 30) == londonTime(2026, 10, 25, 0, 30));    // still BST
  CHECK(busUkTime(2026, 10, 25, 2, 0) == londonTime(2026, 10, 25, 2, 0));      // GMT again
  for (int y = 2025; y <= 2032; y++)
    for (int mo = 1; mo <= 12; mo++)
      for (int d = 1; d <= 28; d += 3) CHECK(busUkTime(y, mo, d, 12, 34) == londonTime(y, mo, d, 12, 34));
  CHECK(busMinutesTo(1000, 1000) == 0);
  CHECK(busMinutesTo(1059, 1000) == 0);   // 59 s: due
  CHECK(busMinutesTo(1060, 1000) == 1);
  CHECK(busMinutesTo(1000 + 11 * 60 + 30, 1000) == 11);
  CHECK(busMinutesTo(990, 1000) == -1);   // gone
}

static void tfl() {
  std::string json = readFile("fixtures/tfl-arrivals.json");
  BusBoard b = {};
  strcpy(b.code, "490008660N");
  CHECK(busParseTfl(json.c_str(), b));
  CHECK(b.ok && !strcmp(b.code, "490008660N"));
  CHECK(!strcmp(b.name, "Lady Somerset Road"));
  CHECK(!strcmp(b.indicator, "GY"));
  CHECK(!strcmp(b.towards, "Highgate Village Or Parliament Hill Fields"));
  CHECK(b.n == 7);
  for (int i = 1; i < b.n; i++) CHECK(b.dep[i - 1].at <= b.dep[i].at);
  CHECK(!strcmp(b.dep[0].line, "88") && !strcmp(b.dep[0].dest, "Parliament Hill Fields"));
  CHECK(b.dep[0].at == busUtcTime("2026-11-24T20:22:03Z") && b.dep[0].live);
  CHECK(!strcmp(b.dep[1].line, "214") && !strcmp(b.dep[1].dest, "Highgate Village"));
  CHECK(!strcmp(b.dep[6].line, "214") && b.dep[6].at == busUtcTime("2026-11-24T20:45:41Z"));

  BusBoard e = {};
  CHECK(busParseTfl("[]", e) && e.ok && e.n == 0);          // no bus due: a good reply
  CHECK(busParseTfl(" [ ] ", e) && e.n == 0);
  CHECK(!busParseTfl("{\"message\":\"nope\"}", e) && !e.ok);  // TfL's error object
  CHECK(!busParseTfl("[{\"lineName\":\"1\"", e));            // cut short
  CHECK(!busParseTfl("", e) && !busParseTfl(nullptr, e));

  // More than BUS_MAX_DEPS, in no order: the soonest are kept, sorted.
  std::string many = "[";
  for (int k = 0; k < 12; k++) {
    int minute = (k * 7) % 12;  // 0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5
    char one[160];
    snprintf(one, sizeof one, "%s{\"lineName\":\"L%d\",\"destinationName\":\"D\",\"expectedArrival\":\"2026-11-24T20:%02d:00Z\"}",
             k ? "," : "", minute, minute);
    many += one;
  }
  many += "]";
  BusBoard m = {};
  CHECK(busParseTfl(many.c_str(), m) && m.n == BUS_MAX_DEPS);
  for (int i = 0; i < m.n; i++) {
    char want[8];
    snprintf(want, sizeof want, "L%d", i);
    CHECK(!strcmp(m.dep[i].line, want));
  }

  // Escapes and a long name: unescaped, cut at a whole letter.
  BusBoard u = {};
  CHECK(busParseTfl("[{\"lineName\":\"N5\",\"destinationName\":\"Caf\\u00e9 \\\"Royal\\\" \\u2014 a very long destination name indeed, longer than it fits\","
                    "\"expectedArrival\":\"2026-11-24T20:00:00Z\",\"timing\":{\"lineName\":\"not this\"}}]", u));
  CHECK(u.n == 1 && !strcmp(u.dep[0].line, "N5"));
  CHECK(!strncmp(u.dep[0].dest, "Caf\xc3\xa9 \"Royal\" \xe2\x80\x94 a very", 20));
  CHECK(strlen(u.dep[0].dest) < sizeof u.dep[0].dest);
}

static void nextbuses() {
  std::string json = readFile("fixtures/nextbuses-live.json");
  BusBoard b = {};
  strcpy(b.code, "2500717");
  CHECK(busParseNextBuses(json.c_str(), londonTime, b));
  CHECK(b.ok && !strcmp(b.name, "Adelaide Street") && !strcmp(b.indicator, "by") && !b.towards[0]);
  CHECK(b.n == 4);
  CHECK(!strcmp(b.dep[0].line, "464") && !strcmp(b.dep[0].dest, "Rochdale") && b.dep[0].live);
  CHECK(b.dep[0].at == londonTime(2026, 11, 24, 20, 41));                     // the live time, not the timetable's 20:38
  CHECK(!strcmp(b.dep[1].line, "6") && !b.dep[1].live);                       // 20:50, sorted before 21:08
  CHECK(!strcmp(b.dep[1].dest, "Clayton-le-Moors & Caf\xc3\xa9"));
  CHECK(!strcmp(b.dep[2].dest, "Bacup") && b.dep[2].at == londonTime(2026, 11, 24, 21, 8));
  CHECK(b.dep[3].at == londonTime(2026, 11, 25, 0, 8));                       // after midnight: the next day's date

  BusBoard e = {};
  CHECK(busParseNextBuses("{\"name\":\"X\",\"departures\":{\"all\":[]}}", londonTime, e) && e.ok && e.n == 0 && !strcmp(e.name, "X"));
  CHECK(!busParseNextBuses("{\"error\":\"Invalid app_id or app_key\"}", londonTime, e) && !e.ok);
  CHECK(!busParseNextBuses("[]", londonTime, e));
  CHECK(!busParseNextBuses(json.substr(0, json.size() / 2).c_str(), londonTime, e));  // cut short
  // A departure without a usable time is skipped, not shown at midnight.
  CHECK(busParseNextBuses("{\"departures\":{\"all\":[{\"line_name\":\"1\",\"date\":\"2026-11-24\",\"aimed_departure_time\":null},"
                          "{\"line_name\":\"2\",\"date\":\"2026-11-24\",\"aimed_departure_time\":\"25:99\"}]}}",
                          londonTime, e) &&
        e.n == 0);
}

int main() {
  setenv("TZ", "Europe/London", 1);
  tzset();
  stopCodes();
  times();
  tfl();
  nextbuses();
  printf("bus: %d checks, %d failed\n", checks, failed);
  return failed ? 1 : 0;
}
