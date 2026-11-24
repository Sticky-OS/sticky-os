// The Bus app's data: the stops in the settings, and the departures read
// from TfL (London) or NextBuses through TransportAPI (the rest of Great
// Britain). Plain C++, no Arduino, so firmware/test/bus checks it on a
// computer; net.cpp fetches, busapp.ino draws.
#pragma once
#include <stddef.h>
#include <time.h>

static const int BUS_MAX_STOPS = 2;  // the stops the app shows, one at a time
static const int BUS_MAX_DEPS = 8;   // departures kept for a stop, soonest first
static const int BUS_CODE_MAX = 24;  // an ATCO code with its 0: 490008660N, 2500717, 2500IMG1735

// Where a stop's times come from. London's stops (ATCO codes starting 490)
// are TfL's, free and with no key; every other stop asks NextBuses through
// TransportAPI, with the owner's own key (bus.tapi_id, bus.tapi_key).
enum BusSource { BUS_TFL, BUS_NEXTBUSES };

struct BusDep {
  char line[8];      // "214", "464", "N20"
  char dest[44];     // "Highgate Village", UTF-8
  time_t at;         // when it's expected (or due, when there's no live time)
  bool live;         // `at` is a live prediction, not the timetable
};

struct BusBoard {
  bool ok;
  char code[BUS_CODE_MAX];  // the stop's ATCO code, as asked for
  char name[48];            // "Lady Somerset Road"; "" when the reply didn't say
  char indicator[16];       // what's on the stop's flag: "GY", "by", "Stop GY"
  char towards[48];         // TfL's "towards" line; "" otherwise
  int n;                    // departures below, soonest first
  BusDep dep[BUS_MAX_DEPS];
  time_t at;                // when it was fetched
};

// The stops in the bus.stops setting, at most `max`: ATCO codes, one a line
// or with spaces or commas between them. A bustimes.org stop address works
// too (https://bustimes.org/stops/2500717): the code is taken from it.
// Anything else that isn't 3 to 20 letters and digits is skipped.
int busStopCodes(const char *setting, char codes[][BUS_CODE_MAX], int max);

BusSource busSource(const char *code);  // 490...: TfL; anything else: NextBuses

// TfL's StopPoint/<code>/Arrivals reply (a JSON array of predictions) into
// `b`: the stop's name, flag and "towards" from the first, the departures
// sorted, the soonest BUS_MAX_DEPS. False when it isn't such a reply. An
// empty array is a good reply: no bus due in the next half hour or so.
bool busParseTfl(const char *json, BusBoard &b);

// TransportAPI's bus/stop/<code>/live.json reply (group=no): its "name" or
// "stop_name", "indicator", and departures.all with line_name, direction,
// date, aimed_departure_time and, when live, expected_departure_date and
// expected_departure_time. Its dates and times are the UK's local time;
// `local` turns one into a time_t (busUkTime on the board).
typedef time_t (*BusLocalTime)(int year, int month, int day, int hour, int minute);
bool busParseNextBuses(const char *json, BusLocalTime local, BusBoard &b);

// Whole minutes from `now` until `at`, rounded down; 0 or less is "Due".
long busMinutesTo(time_t at, time_t now);

// A time on the UK's clocks (GMT, or BST from 01:00 UTC on the last Sunday
// of March to 01:00 UTC on the last Sunday of October) as a time_t, whatever
// the board's own time zone. NextBuses' times are the UK's.
time_t busUkTime(int year, int month, int day, int hour, int minute);

// An ISO 8601 time in UTC, as TfL writes it: 2026-11-24T14:24:27Z (the
// fraction and the Z optional). 0 when it isn't one.
time_t busUtcTime(const char *s);
