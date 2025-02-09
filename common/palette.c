#include "palette.h"
#include "common.h"

void set_palette(uint8_t c, uint8_t r, uint8_t g, uint8_t b) {
   _outb(PALETTE_CODE,  c);
   _outb(PALETTE_BLUE,  b);
   _outb(PALETTE_RED,   r);
   _outb(PALETTE_GREEN, g);
}
