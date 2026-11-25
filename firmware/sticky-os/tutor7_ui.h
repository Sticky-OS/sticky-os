// Three small types of tutor7.ino. They are here, not in the tab, because Arduino
// writes its function prototypes above the first function of the sketch, and a
// type a prototype uses has to be declared before that (see sticky-os.ino).
#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>

// A piece of a line of text: plain, what is under a square root, or a pi (the fonts have neither).
struct T7Piece {
  String s;
  bool root;
  bool pi = false;
};

// How a block of paragraphs is set: the font, the line height, the gap between paragraphs.
struct T7Block {
  const GFXfont *f;
  int line, gap;
};

// A vertex of a Unit 4 figure, in a unit box, y down (t7Poly takes them).
struct T7Pt {
  float x, y;
};

// The circle figures' room: the centre and the largest radius that leaves room for the names.
struct T7Box {
  int cx, cy, R;
};

// A triangle's corners A (top), B (bottom left), C (bottom right), for the Thales figures.
struct T7Tri {
  int X[3], Y[3];
};
