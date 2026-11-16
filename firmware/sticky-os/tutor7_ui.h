// Two small types of tutor7.ino. They are here, not in the tab, because Arduino
// writes its function prototypes above the first function of the sketch, and a
// type a prototype uses has to be declared before that (see sticky-os.ino).
#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>

// A piece of a line of text: plain, or what is under a square root.
struct T7Piece {
  String s;
  bool root;
};

// How a block of paragraphs is set: the font, the line height, the gap between paragraphs.
struct T7Block {
  const GFXfont *f;
  int line, gap;
};
