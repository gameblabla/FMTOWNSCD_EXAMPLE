#include <stdlib.h>

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


static const uint8_t video_set_31[2] = { 0x0f, 0x08 };

static crtc_set_t crtc = CRTC_SET_1;
static video_set_t video = VIDEO_SET_1;

int main(int argc, char *argv[]) {
	volatile uint32_t *vram = (uint32_t *)0x80000000;
	volatile uint32_t *vram2 = (uint32_t *)0x80040000;
	   
	stop_display();
	
	for(int i=0;i<32;i++)
	{
		crtc_out16(i, crtc_set_31[i]);
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
	for (int i = 0; i < (512*480)/4; i++) 
	{
		*vram++ = *src_line++;
		*vram2++ = *src_line++;
	}
	
	while(1)
	{
	   
	}
	return 0;
}
