#include "palette.h"
#include "common.h"

void set_palette(uint8_t c, uint8_t r, uint8_t g, uint8_t b) {
   __outb( c, PALETTE_CODE);
   __outb( b, PALETTE_BLUE);
   __outb( r, PALETTE_RED);
   __outb( g, PALETTE_GREEN);
}
