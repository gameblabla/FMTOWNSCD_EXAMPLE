#include <stdlib.h>
#include <stdio.h>

#include "defs.h"
#include "common.h"
#include "settings.h"
#include "palette.h"

extern const uint16_t picture[76800*2] ;

// 17: 16bpp (512x480 / 512x512)

static const uint16_t crtc_set_31[32] = {
    0x0060, // HSW1 = 96
    0x02c0, // HSW2 = 704
    0x0000, // RSV0 = 0
    0x0000, // RSV1 = 0
    0x031f, // HST  = 799
    0x0000, // VST1 = 0
    0x0004, // VST2 = 4
    0x0000, // EET  = 0
    0x0419, // VST  = 1049
    0x00ca, // HDS0 = 202
    0x02ca, // HDE0 = 714
    0x00ca, // HDS1 = 202
    0x02ca, // HDE1 = 714
    0x0046, // VDS0 = 70
    0x0406, // VDE0 = 1030
    0x0046, // VDS1 = 70
    0x0406, // VDE1 = 1030
    0x0000, // FA0  = 0
    0x00ca, // HAJ0 = 202
    0x0000, // FO0  = 0
    0x0080, // LO0  = 128
    0x0000, // FA1  = 0
    0x00ca, // HAJ1 = 202
    0x0000, // FO1  = 0
    0x0080, // LO1  = 128
    0x0058, // EHAJ = 88
    0x0001, // EVAJ = 1
    0x0000, // ZOOM = 0
    0x000a, // CR0  = 10
    0x0002, // CR1  = 2
    0x0000, // FR   = 0
    0x0192  // CR2  = 402
};

// 4bpp color mode
static const uint8_t video_set_31[2] = { 0x0f, 0x08 };

static const uint16_t crtc_frog[32] = {
    0x0074, // HSW1 = 116
    0x0530, // HSW2 = 1328
    0x0000, // RSV0 = 0
    0x0000, // RSV1 = 0
    0x0617, // HST  = 1559
    0x0006, // VST1 = 6
    0x000C, // VST2 = 12
    0x0012, // EET  = 18
    0x020B, // VST  = 523
    0x00E7, // HDS0 = 231
    0x05E7, // HDE0 = 1511
    0x00E7, // HDS1 = 231
    0x05E7, // HDE1 = 1511
    0x002A, // VDS0 = 42
    0x020A, // VDE0 = 522
    0x002A, // VDS1 = 42
    0x020A, // VDE1 = 522
    0x0000, // FA0  = 0
    0x00E7, // HAJ0 = 231
    0x0000, // FO0  = 0
    0x0100, // LO0  = 256
    0x0000, // FA1  = 0
    0x00E7, // HAJ1 = 231
    0x0000, // FO1  = 0
    0x0100, // LO1  = 256
    0x0056, // EHAJ = 86
    0x0001, // EVAJ = 1
    0x0303, // ZOOM = 771
    0x8005, // CR0  = 32773
    0x0001, // CR1  = 1
    0x0002, // FR   = 0
    0x0188  // CR2  = 392
};

// 16-bits color
static const uint8_t video_frog[2] = { 0x1f, 0x28 };


int main(int argc, char *argv[]) {
	volatile uint16_t *vram = (uint16_t *)0x80000000;
	volatile uint16_t *vram2 = (uint16_t *)0x80040000;
	
	_outb(IO_FMR_GVRAMDISPMODE, 0x27);
	_outb(IO_FMR_VRAM_OR_MAINRAM, 0);
	_outb(IO_FMR_GVRAMMASK, 0x0F);	  
	 
	stop_display();
	
	for(int i=0;i<32;i++)
	{
		crtc_out16(i, crtc_frog[i]);
	}
	
	for(int i=0;i<2;i++)
	{
		video_out(i, video_set_31[i]);
	}
	
	for (int c = 0; c < 256; ++c) 
	{
		set_palette(c, c, c, c);
	}
	
	start_display();
	

	const uint32_t *src_line = picture;
	for (int i = 0; i < 320*240; i++) 
	{
		vram[i] = 0x2222;
	}
	
	while(1)
	{
	   
	}
	return 0;
}
