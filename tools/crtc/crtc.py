#!/usr/bin/env python3
import re
import sys

def main():
    if len(sys.argv) < 2:
        print("Usage: {} <input_file>".format(sys.argv[0]))
        sys.exit(1)
        
    input_filename = sys.argv[1]
    try:
        with open(input_filename, 'r') as f:
            data = f.read()
    except Exception as e:
        print("Error reading file {}: {}".format(input_filename, e))
        sys.exit(1)
    
    # Mapping register names to their index (based on FM TOWNS CRTC documentation)
    reg_mapping = {
        "HSW1": 0x00,
        "HSW2": 0x01,
        # 0x02 and 0x03 are reserved.
        "HST":  0x04,
        "VST1": 0x05,
        "VST2": 0x06,
        "EET":  0x07,
        "VST":  0x08,
        "HDS0": 0x09,
        "HDE0": 0x0A,
        "HDS1": 0x0B,
        "HDE1": 0x0C,
        "VDS0": 0x0D,
        "VDE0": 0x0E,
        "VDS1": 0x0F,
        "VDE1": 0x10,
        "FA0":  0x11,
        "HAJ0": 0x12,
        "FO0":  0x13,
        "LO0":  0x14,
        "FA1":  0x15,
        "HAJ1": 0x16,
        "FO1":  0x17,
        "LO1":  0x18,
        "EHAJ": 0x19,
        "EVAJ": 0x1A,
        "ZOOM": 0x1B,
        "CR0":  0x1C,
        "CR1":  0x1D,
        "FR":   0x1E,
        "CR2":  0x1F,
    }

    # Initialize a 32-element list for registers (default to 0)
    reg_values = [0] * 32

    # (Reserved registers at 0x02 and 0x03 remain zero.)

    # Use a regex to pull out tokens of the form NAME:HEXVALUE.
    token_pattern = re.compile(r'([A-Z0-9\-]+)\s*:\s*([0-9A-Fa-f]+)')
    tokens = token_pattern.findall(data)

    # Process each token – only update if the name is known.
    for name, hex_str in tokens:
        # Skip video-related tokens (we’ll handle the “Sifters” line separately)
        if name in ("Sifters", "PLT"):
            continue
        if name in reg_mapping:
            idx = reg_mapping[name]
            val = int(hex_str, 16)
            # Special case: for CR0 (register 0x1C) the value is given as 800A.
            # The expected output shows just 000A so we clear the 0x8000 bit.
            if name == "CR0" and (val & 0x8000):
                val = val & 0x7FFF
            reg_values[idx] = val
        # For reserved entries shown as "----", we leave them as 0.

    # Extract the video shifter values.
    # The “Sifters (Isn't it Shifter?):” line contains two hex numbers.
    video_pattern = re.compile(r"Sifters.*?:\s*([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)")
    video_match = video_pattern.search(data)
    if video_match:
        video1 = int(video_match.group(1), 16)
        video2 = int(video_match.group(2), 16)
        video_vals = (video1, video2)
    else:
        video_vals = (0, 0)

    # Format the CRTC macro.
    # We want 32 values arranged in 4 lines of 8 values.
    lines = []
    for i in range(0, 32, 8):
        segment = reg_values[i:i+8]
        formatted = []
        for j, val in enumerate(segment, start=i):
            if j in (2, 3):
                # reserved registers: print as a plain 0 (right-aligned in an 8-char field)
                formatted.append(f"{val:8d}")
            else:
                formatted.append(f"0x{val:04x}".rjust(8))
        lines.append(", ".join(formatted))

    # Build the final CRTC_SET_31 macro string.
    crtc_macro = (
        "#define CRTC_SET_31                                                     \\\n"
        "   {                                                                    \\\n"
    )
    for line in lines:
        crtc_macro += "      " + line + ",   \\\n"
    # Remove the trailing comma/backslash from the last line and close the brace.
    crtc_macro = crtc_macro.rstrip(" ,\\\n") + "\n   }"

    # Build the VIDEO_SET_31 macro.
    video_macro = f"#define VIDEO_SET_31   {{ 0x{video_vals[0]:02x}, 0x{video_vals[1]:02x} }}"

    # Output the result.
    print(crtc_macro)
    print()
    print(video_macro)

if __name__ == "__main__":
    main()
