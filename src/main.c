/*
 * FM TOWNS GRB555 512×480 Double Buffered Image Display Example
 *
 * This example runs under MSDOS using the FREE386 DOS extender (Phar Lap compatible).
 * It loads a raw image ("image.raw") into a pre-allocated buffer and then displays the
 * image using double buffering. The video mode is 512×480 in GRB555 (16 bits per pixel).
 *
 * Key features:
 *   - Double buffering using two VRAM pages.
 *   - Waiting for vertical sync (vsync) for smooth screen updates.
 *   - Setting the GS register to point to VRAM in 32-bit protected mode.
 *   - Enabling/disabling the CPU cache (only on 486 and above).
 *   - Setting the CRTC and video modes specific to FM TOWNS.
 *
 * Note: DOS extender file I/O functions are implemented via inline assembly.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include "palette.h"   
#include "test.h"  
#include "defs.h"
#include "io.h"  
#include "common.h" 
#include "settings.h"  // (defines video and CRTC settings)

int offset, currentpage;

#define SCREEN_WIDTH 512
#define SCREEN_HEIGHT 480

#define SCREEN_WIDTH_STRIDE 512

//----------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------

/*
 * VRAM pointers:
 * We use __seg_gs to make the GS segment pointer point to VRAM.
 * Once GS is set to the VRAM segment (selector 0x104 for GRB555 mode),
 * the pointer "vram" can be used to access VRAM in 16-bit mode.
 */
__seg_gs uint16_t *vram = 0;

/*
 * Picture buffer:
 * The raw image is 512×480 pixels, with each pixel being 16 bits.
 * However, we will be transferring data in 16-bit chunks.
 */
uint16_t picture[SCREEN_WIDTH * SCREEN_HEIGHT];

//----------------------------------------------------------------
// CPU Cache Control Functions
//----------------------------------------------------------------

/*
 * cpu_has_cache
 * --------------
 * Checks if the CPU has a cache (i.e. is a 486 or later) by trying
 * to toggle the AC flag (bit 18) in the EFLAGS register.
 *
 * Returns non-zero if the CPU supports caching.
 */
static inline int cpu_has_cache(void) 
{
    uint32_t original, modified, after;

    // Save current EFLAGS.
    __asm__ volatile (
        "pushfl\n\t"
        "popl %0"
        : "=r" (original)
    );

    // Toggle the AC flag (bit 18).
    modified = original ^ 0x40000;
    __asm__ volatile (
        "pushl %0\n\t"
        "popfl"
        : : "r" (modified)
    );

    // Read back the EFLAGS.
    __asm__ volatile (
        "pushfl\n\t"
        "popl %0"
        : "=r" (after)
    );

    // If the AC flag toggled, then caching is supported.
    return ((after ^ original) & 0x40000) != 0;
}

/*
 * set_cache
 * ---------
 * Enables or disables the CPU cache (only if the CPU supports caching).
 *  - val == 0: Turn cache off.
 *  - val == 1: Turn cache on.
 */
void set_cache(int val)
{
    if (!cpu_has_cache())
        return;
   
    switch (val) {
        case 0:
            cache_off();
            break;
        case 1:
            cache_on();
            break;
    }
}

//----------------------------------------------------------------
// Simple Delay Function
//----------------------------------------------------------------

/*
 * sleep
 * -----
 * Creates a simple delay loop. FM TOWNS uses port 0x006C,
 * which waits ~1 microsecond per iteration.
 * The parameter n represents the delay in milliseconds.
 */
void sleep(int n)
{
    int i;
    for (i = 0; i < n * 1000; i++)
        outb(0, 0x006C);  // Wait ~1µs per iteration.
}

//----------------------------------------------------------------
// DOS Extender File I/O Functions
//----------------------------------------------------------------

/*
 * dosext_open_handle
 * ------------------
 * Opens a file using DOS interrupt 0x21.
 *   mode: open mode (0 for read-only).
 *   filename: pointer to the file name.
 *   fd: pointer where the file handle is stored on success.
 *
 * Returns 0 on success; otherwise, returns an error code.
 */
static inline uint16_t dosext_open_handle(uint8_t mode, const char *filename, uint16_t *fd) {
    uint16_t result;
    bool cf;

    asm volatile (
        "movb    $0x3d, %%ah\n\t"  // DOS function 3Dh: Open file.
        "int     $0x21"
        : "=@ccc" (cf), "=a" (result)
        : "a" (mode), "d" (filename), "m" (*(const char (*)[])filename)
    );

    if (!cf) {
        *fd = result;
        return 0;
    } else {
        return result;
    }
}

/*
 * dosext_close_handle
 * -------------------
 * Closes an open file handle.
 *
 * Returns true on success, false otherwise.
 */
static inline bool dosext_close_handle(uint16_t handle, uint16_t *r) {
    uint16_t result;
    bool cf;

    asm volatile (
        "movb    $0x3e, %%ah\n\t"  // DOS function 3Eh: Close file.
        "int     $0x21"
        : "=@ccc" (cf), "=a" (result)
        : "b" (handle)
    );

    *r = result;
    return !cf;
}

/*
 * dosext_read_handle
 * ------------------
 * Reads data from an open file handle.
 *   handle: file handle.
 *   count: number of bytes to read.
 *   buf: destination buffer.
 *   r: number of bytes read (on success).
 *
 * Returns true on success, false on failure.
 */
static inline bool dosext_read_handle(uint16_t handle, uint32_t count, void *buf, uint32_t *r) {
    uint16_t result;
    bool cf;

    asm volatile (
        "movb    $0x3f, %%ah\n\t"  // DOS function 3Fh: Read file.
        "int     $0x21"
        : "=@ccc" (cf), "=a" (result), "=m" (*(char (*)[])buf)
        : "b" (handle), "c" (count), "d" (buf)
    );

    if (cf) {
        *r = (uint16_t)result;
    } else {
        *r = result;
    }
    return !cf;
}

//----------------------------------------------------------------
// Image Loading
//----------------------------------------------------------------

/*
 * load_raw_image
 * --------------
 * Loads a raw image file ("image.raw") into the 'picture' buffer.
 * The image is expected to be 512×480 with 16 bits per pixel.
 */
void load_raw_image(const char *filename) {
    const size_t width = SCREEN_WIDTH;
    const size_t height = SCREEN_HEIGHT;
    const size_t expected_size = width * height * sizeof(uint16_t); // Expected file size in bytes.

    // Open the file in read-only mode.
    uint16_t file_handle;
    uint16_t open_err = dosext_open_handle(0, filename, &file_handle);
    if (open_err != 0) {
        // Failed to open the file.
        return;
    }

    // Read the image data into the pre-allocated 'picture' buffer.
    uint32_t bytes_read;
    if (!dosext_read_handle(file_handle, expected_size, picture, &bytes_read) ||
        bytes_read != expected_size) {
        // Read failed or did not return the expected number of bytes.
        uint16_t close_dummy;
        dosext_close_handle(file_handle, &close_dummy);
        return;
    }

    // Close the file.
    uint16_t close_err;
    dosext_close_handle(file_handle, &close_err);
}

void load_palette(const char *filename) {
    const size_t palette_size = 768;  // 256 colors * 3 bytes per color (RGB888)
    uint8_t palette[palette_size];

    // Open the palette file in read-only mode.
    uint16_t file_handle;
    uint16_t open_err = dosext_open_handle(0, filename, &file_handle);
    if (open_err != 0) {
        // Failed to open the file.
        return;
    }

    // Read the entire palette data.
    uint32_t bytes_read;
    if (!dosext_read_handle(file_handle, palette_size, palette, &bytes_read) ||
        bytes_read != palette_size) {
        // Read failed or did not return the expected number of bytes.
        uint16_t close_dummy;
        dosext_close_handle(file_handle, &close_dummy);
        return;
    }

    // Close the palette file.
    uint16_t close_err;
    dosext_close_handle(file_handle, &close_err);

    // Apply each palette entry using the set_palette function.
    for (uint8_t i = 0; i < 256; i++) {
        uint8_t r = palette[i * 3 + 0];
        uint8_t g = palette[i * 3 + 1];
        uint8_t b = palette[i * 3 + 2];
        set_palette(i, r, g, b);
    }
}

//----------------------------------------------------------------
// Display Functions
//----------------------------------------------------------------

/*
 * WaitforVsync
 * ------------
 * Waits for the vertical synchronization (vsync) signal.
 * This ensures that screen updates occur during the vertical blanking interval.
 */
static void WaitforVsync()
{
    __outb(30, 0x0440);  // Trigger vsync read.
    while (!(__inb(0x443) & 4)) {
        // Busy-wait until vsync is active.
    }	
}

/*
 * WriteCRTC
 * ---------
 * Writes a value to a CRTC (Cathode Ray Tube Controller) register.
 *   address: register address.
 *   data: data to be written.
 */
static inline void WriteCRTC(int address, int data)
{
    __outb(address, 0x0440); // Write the register address.
    __outw(data, 0x0442);    // Write the data.
}

/* 512x480 15bpp mode */
static crtc_set_t crtc = CRTC_SET_31;
static video_set_t video = VIDEO_SET_31;


#define SetPixel_16bpp(x, y, color) vram[x + (y * SCREEN_WIDTH_STRIDE)] = color;

//----------------------------------------------------------------
// Main Application Entry Point
//----------------------------------------------------------------

void Put_Image(uint16_t* src_line, uint32_t num_pixels)
{
        vram = 0;          // Base pointer for first VRAM page.
        vram += offset;
        // Copy image data from 'picture' to both VRAM ports (odd/even)
        for (int i = 0; i < num_pixels; i++) {
            *vram++  = *src_line++;
        }
        
        // Set pixel at defined location
		vram = 0;
        vram += offset;	
}


/*
 * main
 * ----
 * The entry point of the application. It sets up video mode, loads the image,
 * and continuously displays the image using double buffering.
 */
int main(int argc, char* argv[])
{


    // Enable CPU cache if available (for 486 and above).
    set_cache(1);

    // Stop display output before changing video settings.
    stop_display();
    // Set CRTC and video mode as defined in settings.h.
    set_crtc(crtc);
    set_video(video);
    // Restart display.
    start_display();

    // Set GS to point to VRAM.
    // For GRB555 mode on FM TOWNS, the VRAM segment selector is 0x104.
    uint16_t vram_selector = 0x10c;
    asm volatile("movw %w0, %%gs\n\t" : : "r"(vram_selector));

    // Load the raw image file into the 'picture' buffer.
    load_raw_image("image.raw");

    /*
     * Calculate the number of 32-bit words to transfer.
     * The raw image is 16 bits per pixel, but we are transferring 32 bits
     * at a time (each 32-bit word holds two pixels).
     */
    uint32_t num_pixels = (SCREEN_WIDTH * SCREEN_HEIGHT);

    currentpage = 0;

    // Main loop: display the image with double buffering.
    while (1)
    {
		offset = currentpage * (SCREEN_WIDTH * SCREEN_HEIGHT);

        Put_Image(picture, num_pixels);
        
		SetPixel_16bpp(2,4, 0xFFFF);

        // Update the CRTC registers to point to the current buffer.
		WriteCRTC(17, offset);
		WriteCRTC(21, offset);

        // Wait for vertical synchronization before swapping buffers.
        WaitforVsync();

        // Toggle the current page (double buffering).
        currentpage ^= 1;
    }


    return 0;
}
