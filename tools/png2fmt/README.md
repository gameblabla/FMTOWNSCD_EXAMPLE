# png2fmt

## Overview

`png2fmt` is a command-line tool for converting PNG images into raw VRAM-compatible formats for the FM TOWNS. It supports two primary export modes:

- **GRB555 (16-bit little-endian)**
- **8bpp (indexed color with RGB888 palette)**

This tool is useful for homebrew development, allowing developers to prepare graphics assets for direct use in FM TOWNS software.

## Features

- Converts PNG images into FM TOWNS-compatible VRAM formats.
- Supports **GRB555** format (16-bit per pixel, little-endian).
- Supports **8bpp indexed color** format with an accompanying RGB888 palette.
- Handles palette conversion for indexed images.
- Automatically processes PNGs of various formats, converting them to the expected RGB data.

## Usage

```
Usage:
  png2fmt -g input.png output.raw
    (Export as RAW GRB555 little-endian; each pixel is 16-bit)
  png2fmt -8 input.png output.raw palette.bin
    (Export as RAW 8bpp with palette; palette file is RGB888)
```

### GRB555 Mode

To convert a PNG image to **GRB555**, run:

```
png2fmt -g input.png output.raw
```

- Each pixel is stored as **16-bit little-endian**, packed as:
  - **[Unused:1] [Green:5] [Red:5] [Blue:5]**
  - This matches the FM TOWNS VRAM format.

### 8bpp Mode

To convert a PNG image to **8bpp indexed color**, run:

```
png2fmt -8 input.png output.raw palette.bin
```

- The output raw file contains **1 byte per pixel** (palette index).
- The palette is stored in a separate **palette.bin** file, using **RGB888** format (3 bytes per color: R, G, B).

## Compiling

### Requirements

Ensure you have the following installed:

- **GCC (or another C compiler)**
- **libpng** (for handling PNG images)

### Build Instructions

Compile the tool using the provided `Makefile`:

```
make
```

This will generate the `png2fmt` executable.

---

Developed for FM TOWNS homebrew development.

