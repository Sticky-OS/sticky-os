// The microSD card: on the screen's SPI bus, mounted at CARD_ROOT when
// something first asks for it (Photos, Books). A card can be taken out and
// put back while the board runs: cardMount() notices and mounts it again.
#pragma once

#define CARD_ROOT "/sd"

bool cardIn();       // a card is in the slot (the slot's switch)
// True when the card is mounted, mounting it if needed. False: no card, or
// one the board can't read (not FAT). After a card was taken out, what was
// open on it is gone: files read as errors, and the next call mounts afresh.
bool cardMount();
// How many times a card has been mounted since the start: it changes when a
// card was swapped, so lists made from the old one are known to be stale.
unsigned cardGeneration();
