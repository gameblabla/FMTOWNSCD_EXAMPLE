#include "common.h"
#include <stdlib.h>

uint16_t crtc_in16(crtc_reg_t reg) {
   _outb(CRTC_ADDR, reg);
   return _inw(CRTC_DATA);
}

void crtc_out8_lo(crtc_reg_t reg, uint16_t val) {
   _outb(CRTC_ADDR, reg);
   _outb(CRTC_DATA_LO, val & 0xff);
}

void crtc_out8_hi(crtc_reg_t reg, uint16_t val) {
   _outb(CRTC_ADDR, reg);
   _outb(CRTC_DATA_HI, val >> 8);
}

void crtc_out16(crtc_reg_t reg, uint16_t val) {
   _outb(CRTC_ADDR, reg);
   _outw(CRTC_DATA, val);
}

uint8_t video_in(video_reg_t reg) {
   _outb(VIDEO_ADDR, reg);
   return _inb(VIDEO_DATA);
}

void video_out(video_reg_t reg, uint8_t val) {
   _outb(VIDEO_ADDR, reg);
   _outb(VIDEO_DATA, val);
}

void start_display(void) {
   uint16_t v = crtc_in16(CR0);
   v |= 0x8000;
   crtc_out16(CR0, v);
}

void stop_display(void) {
   uint16_t v = crtc_in16(CR0);
   v &= 0x7fff;
   crtc_out16(CR0, v);
}

void set_crtc(const crtc_set_t set) {
   for (int i = 0; i < sizeof(crtc_set_t) / sizeof(set[0]); ++i) {
      if (i == RSV0 || i == RSV1) {
         continue;
      }
      crtc_out16(i, set[i]);
   }
}

void set_video(const video_set_t set) {
   for (int i = 0; i < sizeof(video_set_t) / sizeof(set[0]); ++i) {
      video_out(i, set[i]);
   }
}
