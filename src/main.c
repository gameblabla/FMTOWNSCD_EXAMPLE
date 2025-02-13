#include <stdint.h>
#include <inttypes.h>
#include "test.h"
#include "defs.h"
#include "io.h"
#include "common.h"
#include "settings.h"

#undef TEST_TIMES
#define DEFTESTS 9

struct cpu_ident cpu_id;

void set_cache(int val)
{
    extern struct cpu_ident cpu_id;
    /* 386's don't have a cache */
    if ((cpu_id.cpuid < 1) && (cpu_id.type == 3))
        return;
    switch(val) {
        case 0:
            cache_off();
            break;
        case 1:
            cache_on();
            break;
    }
}

void sleep(int n, int sms)
{
    int i;
    /* FM TOWNS: a simple delay loop */
    for (i = 0; i < n * 1000; i++)
        outb(0, 0x006C);  // wait ~1us per iteration
}

static crtc_set_t crtc = CRTC_SET_31;
static video_set_t video = VIDEO_SET_31;

/*
 * Use a __seg_gs pointer to VRAM.
 * Once GS is set to the VRAM segment (selector 0x104), then vram[0]
 * corresponds to offset 0 in VRAM.
 */
__seg_gs uint16_t *vram = 0;
__seg_gs uint16_t *vram2 = 0;

void _start(void)
{
    set_cache(1);

    stop_display();
    set_crtc(crtc);
    set_video(video);
    start_display();

    /* Set GS to point to VRAM.
       The following snippet loads GS with the VRAM segment selector (0x104).
    */
    uint16_t s = 0x104;
    asm volatile("movw %w0, %%gs\n\t" : : "r"(s));

    /* For this example, assume the display is 512x480 pixels.
       We'll fill two VRAM banks (the first at offset 0, the second at offset 0x40000 bytes)
       with the 16-bit pattern 0xAAAA.
       Note that since each bank holds half the lines, the number of pixels per bank is:
       (512 * 480) / 2.
    */
    uint32_t num_pixels = (512 * 480) / 2;

    /* Setup vram2 pointer. Since each element is 2 bytes, we add 0x40000/2 elements */
    vram2 = vram + (0x40000 / sizeof(uint16_t));

    for (uint32_t i = 0; i < num_pixels; i++) {
        vram[i] = 0xAAAA;
        vram2[i] = 0xAAAA;
    }

    while(1)
    {
        /* Infinite loop */
    }
}
