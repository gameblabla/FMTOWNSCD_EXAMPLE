#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

typedef enum { MODE_GRB555, MODE_8BPP } ExportMode;

void usage(const char *progname) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s -g input.png output.raw\n", progname);
    fprintf(stderr, "    (Export as RAW GRB555 little-endian; each pixel is 16-bit)\n");
    fprintf(stderr, "  %s -8 input.png output.raw palette.bin\n", progname);
    fprintf(stderr, "    (Export as RAW 8bpp with palette; palette file is RGB888)\n");
}

/* Prototypes */
int export_grb555(const char *input_filename, const char *output_filename);
int export_8bpp(const char *input_filename, const char *output_filename, const char *palette_filename);

int main(int argc, char *argv[]) {
    if (argc < 4) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    ExportMode mode;
    const char *input_filename = NULL;
    const char *output_filename = NULL;
    const char *palette_filename = NULL;
    
    if (strcmp(argv[1], "-g") == 0) {
        if (argc != 4) {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
        mode = MODE_GRB555;
        input_filename = argv[2];
        output_filename = argv[3];
    } else if (strcmp(argv[1], "-8") == 0) {
        if (argc != 5) {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
        mode = MODE_8BPP;
        input_filename = argv[2];
        output_filename = argv[3];
        palette_filename = argv[4];
    } else {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    if (mode == MODE_GRB555) {
        if (export_grb555(input_filename, output_filename) != 0) {
            fprintf(stderr, "Error exporting GRB555\n");
            return EXIT_FAILURE;
        }
    } else {
        if (export_8bpp(input_filename, output_filename, palette_filename) != 0) {
            fprintf(stderr, "Error exporting 8bpp\n");
            return EXIT_FAILURE;
        }
    }
    
    return EXIT_SUCCESS;
}

/* Export a PNG file to RAW GRB555 little-endian format.
   Each pixel is converted from 8-bit per channel RGB to 5 bits per channel.
   The 16 bits are packed as: [unused:1][green:5][red:5][blue:5] and written
   as little-endian. */
int export_grb555(const char *input_filename, const char *output_filename) {
    FILE *fp = fopen(input_filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open input file %s\n", input_filename);
        return -1;
    }
    
    png_byte header[8];
    if (fread(header, 1, 8, fp) != 8) {
        fprintf(stderr, "Failed to read PNG header\n");
        fclose(fp);
        return -1;
    }
    if (png_sig_cmp(header, 0, 8)) {
        fprintf(stderr, "File %s is not recognized as a PNG file\n", input_filename);
        fclose(fp);
        return -1;
    }
    
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                 NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "png_create_read_struct failed\n");
        fclose(fp);
        return -1;
    }
    
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fprintf(stderr, "png_create_info_struct failed\n");
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return -1;
    }
    
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error during PNG init_io\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }
    
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);
    
    png_uint_32 width, height;
    int bit_depth, color_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height,
                 &bit_depth, &color_type, NULL, NULL, NULL);
    
    /* Ensure we have RGB data.
       If the PNG is paletted or grayscale, convert it to RGB.
       Also remove any alpha channel and reduce 16-bit data to 8-bit. */
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);
    if (color_type & PNG_COLOR_MASK_ALPHA)
        png_set_strip_alpha(png_ptr);
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    
    png_read_update_info(png_ptr, info_ptr);
    
    /* Allocate memory for image rows */
    png_bytep *row_pointers = malloc(sizeof(png_bytep) * height);
    if (!row_pointers) {
        fprintf(stderr, "Failed to allocate memory for PNG rows\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }
    for (png_uint_32 y = 0; y < height; y++) {
        row_pointers[y] = malloc(png_get_rowbytes(png_ptr, info_ptr));
        if (!row_pointers[y]) {
            fprintf(stderr, "Failed to allocate memory for row %u\n", y);
            for (png_uint_32 k = 0; k < y; k++)
                free(row_pointers[k]);
            free(row_pointers);
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(fp);
            return -1;
        }
    }
    
    png_read_image(png_ptr, row_pointers);
    fclose(fp);
    
    FILE *outfp = fopen(output_filename, "wb");
    if (!outfp) {
        fprintf(stderr, "Failed to open output file %s\n", output_filename);
        for (png_uint_32 y = 0; y < height; y++) free(row_pointers[y]);
        free(row_pointers);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return -1;
    }
    
    /* Process each pixel.
       Assume that after the transformations each pixel is 3 bytes (RGB).
       Convert each 8-bit channel to 5 bits (by shifting right 3)
       and pack as: [unused:1][green:5][red:5][blue:5]. */
    for (png_uint_32 y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        for (png_uint_32 x = 0; x < width; x++) {
            png_bytep px = row + x * 3;
            int r5 = px[0] >> 3;
            int g5 = px[1] >> 3;
            int b5 = px[2] >> 3;
            unsigned short pixel_val = (g5 << 10) | (r5 << 5) | (b5);
            unsigned char out_bytes[2];
            out_bytes[0] = pixel_val & 0xFF;           /* little-endian low byte */
            out_bytes[1] = (pixel_val >> 8) & 0xFF;      /* high byte */
            fwrite(out_bytes, 1, 2, outfp);
        }
    }
    
    fclose(outfp);
    
    for (png_uint_32 y = 0; y < height; y++)
        free(row_pointers[y]);
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    
    return 0;
}

/* Export a PNG file to a RAW 8bpp (indexed) file along with a palette file.
   The input PNG must be an 8-bit paletted image.
   The output RAW file contains one byte per pixel (the palette index)
   and the palette file is written in RGB888 format (three bytes per entry). */
int export_8bpp(const char *input_filename, const char *output_filename, const char *palette_filename) {
    FILE *fp = fopen(input_filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open input file %s\n", input_filename);
        return -1;
    }
    
    png_byte header[8];
    if (fread(header, 1, 8, fp) != 8) {
        fprintf(stderr, "Failed to read PNG header\n");
        fclose(fp);
        return -1;
    }
    if (png_sig_cmp(header, 0, 8)) {
        fprintf(stderr, "File %s is not recognized as a PNG file\n", input_filename);
        fclose(fp);
        return -1;
    }
    
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                 NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "png_create_read_struct failed\n");
        fclose(fp);
        return -1;
    }
    
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fprintf(stderr, "png_create_info_struct failed\n");
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return -1;
    }
    
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error during PNG init_io\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }
    
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);
    
    png_uint_32 width, height;
    int bit_depth, color_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height,
                 &bit_depth, &color_type, NULL, NULL, NULL);
    
    /* For 8bpp export the PNG must be paletted and 8-bit */
    if (color_type != PNG_COLOR_TYPE_PALETTE || bit_depth != 8) {
        fprintf(stderr, "PNG must be 8-bit paletted for 8bpp export\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }
    
    png_colorp palette = NULL;
    int num_palette = 0;
    if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette) != PNG_INFO_PLTE) {
        fprintf(stderr, "Failed to get PNG palette\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }
    
    /* Allocate memory for image rows */
    png_bytep *row_pointers = malloc(sizeof(png_bytep) * height);
    if (!row_pointers) {
        fprintf(stderr, "Failed to allocate memory for PNG rows\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }
    for (png_uint_32 y = 0; y < height; y++) {
        row_pointers[y] = malloc(png_get_rowbytes(png_ptr, info_ptr));
        if (!row_pointers[y]) {
            fprintf(stderr, "Failed to allocate memory for row %u\n", y);
            for (png_uint_32 k = 0; k < y; k++)
                free(row_pointers[k]);
            free(row_pointers);
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            fclose(fp);
            return -1;
        }
    }
    
    png_read_image(png_ptr, row_pointers);
    fclose(fp);
    
    /* Write out the raw pixel indices (one byte per pixel) */
    FILE *outfp = fopen(output_filename, "wb");
    if (!outfp) {
        fprintf(stderr, "Failed to open output file %s\n", output_filename);
        for (png_uint_32 y = 0; y < height; y++) free(row_pointers[y]);
        free(row_pointers);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return -1;
    }
    for (png_uint_32 y = 0; y < height; y++) {
        fwrite(row_pointers[y], 1, png_get_rowbytes(png_ptr, info_ptr), outfp);
    }
    fclose(outfp);
    
    /* Write out the palette file.
       Each palette entry is 3 bytes (RGB888), in order R, G, B. */
    FILE *palfp = fopen(palette_filename, "wb");
    if (!palfp) {
        fprintf(stderr, "Failed to open palette file %s\n", palette_filename);
        for (png_uint_32 y = 0; y < height; y++) free(row_pointers[y]);
        free(row_pointers);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return -1;
    }
    for (int i = 0; i < num_palette; i++) {
        unsigned char rgb[3];
        rgb[0] = palette[i].red;
        rgb[1] = palette[i].green;
        rgb[2] = palette[i].blue;
        fwrite(rgb, 1, 3, palfp);
    }
    fclose(palfp);
    
    for (png_uint_32 y = 0; y < height; y++)
        free(row_pointers[y]);
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    
    return 0;
}
