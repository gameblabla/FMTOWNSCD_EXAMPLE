Example FM TOWNS MARTY CD program
===============================

See https://github.com/pinterior/elf2exp/
to set up toolchain.

Requires 
```
CD/IO.SYS
```
from an FM TOWNS game otherwise it won't boot. (i am not sure about the license of IO.SYS therefore it's not provided)

Use RUN.SH to build iso and run it with Tsugaru_CUI.elf.

Do not remove IPL.BIN and sort.file as they are needed to make sure it works.
IO.SYS must be set at specific logical sector.

This example makes the screen red by drawing to VRAM.