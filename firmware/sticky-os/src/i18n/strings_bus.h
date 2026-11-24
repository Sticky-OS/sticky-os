// The Bus app (busapp.ino; issue #22): the next buses at a stop. Ids S_BUS_*; see strings.h for the
// format. "TfL", "TransportAPI" and "NextBuses" are names: they stay as they are in every language.
S(S_APP_BUS, "Bus")  // the app's name: under its icon (12pt, ~180 px), the status bar
S(S_BUS_SETUP_TITLE, "Bus stop")  // the setup card's title, bold 18pt
S(S_BUS_SETUP, "Bus shows the next buses at your stop. It needs the stop's code, from the sign at the stop or its bustimes.org page.")
S(S_BUS_NEEDS_KEY, "Stops outside London get their times from NextBuses, through TransportAPI. It needs a TransportAPI key: free for 30 requests a day.")
S(S_BUS_UNKNOWN_STOP, "%s doesn't know the stop %s.")  // the service ("TfL"), then the code it was asked for
S(S_BUS_NEXT_CAPS, "NEXT BUS")  // the card's label, small capitals (bold 9pt, ~130 px)
S(S_BUS_AT, "at %H:%M")  // a time format, on the card beside NEXT BUS: when it comes
S(S_BUS_MIN, "%ld min")  // minutes until a bus, short: "4 min" (bold 24pt on the card, bold 12pt in the list, ~90 px)
S(S_BUS_DUE, "Due")  // a bus that's here or under a minute away, in place of "0 min"
S(S_BUS_THEN, "then %s min")  // on the card, the two after it: "then 11, 19 min"
S(S_BUS_DEPARTURES, "Departures")  // the list's heading, bold 12pt
S(S_BUS_TOWARDS, "towards %s")  // under the stop's name: which way its buses go (TfL's stops)
S(S_BUS_STOPS, "Stop %d of %d: tap for the other")  // at the foot, 9pt, ~300 px
S(S_BUS_NONE, "No buses due soon")  // where the departures go, bold 18pt
S(S_BUS_GETTING, "Getting the times...")  // before the first reply, bold 18pt
S(S_BUS_CANT, "Can't get the times")  // the first try failed, bold 18pt
S(S_BUS_LIVE_TFL, "Live times from TfL")  // at the foot, 9pt
S(S_BUS_LIVE_NEXTBUSES, "Bold: live. Plain: timetable. From NextBuses")  // at the foot, 9pt: which times are live
S(S_BUS_TODAY, "%d of 30 free requests today")  // at the foot, 9pt (NextBuses: what the free plan allows)
S(S_BUS_PAUSED, "Refreshes only when you press OK, to save today's free requests")  // at the foot, bold 9pt
