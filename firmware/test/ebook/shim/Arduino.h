// A stand-in for <Arduino.h> on a computer: just what glyphs.h/.cpp and the
// font headers use, so the firmware's own text code (extIndex, foldCodePoint,
// utf8Next, textFold...) is compiled as it is into the host tests and the
// desktop renderer. Add -I firmware/test/ebook/shim; it does not define
// ARDUINO, which is how code tells the board from the computer.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <string>

#define PROGMEM

// Arduino's String, as far as glyphs.cpp goes.
class String {
 public:
  String() {}
  String(const char *s) : s_(s ? s : "") {}
  String(const std::string &s) : s_(s) {}
  void reserve(size_t n) { s_.reserve(n); }
  unsigned length() const { return (unsigned)s_.size(); }
  const char *c_str() const { return s_.c_str(); }
  String &operator+=(const char *s) { return s_ += s ? s : "", *this; }
  String &operator+=(char c) { return s_ += c, *this; }
  String &operator+=(const String &o) { return s_ += o.s_, *this; }
  bool operator==(const char *s) const { return s_ == s; }

 private:
  std::string s_;
};
