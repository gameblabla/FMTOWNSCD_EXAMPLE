Example FM TOWNS MARTY CD program
===============================

See https://github.com/pinterior/elf2exp/
to set up toolchain.

Requires 
```
CD/TBIOS.SYS
CD/TBIOS.BIN
CD/IO.SYS
```
Otherwise it won't boot.

Use RUN.SH to build iso and run it with Tsugaru_CUI.elf.

Do not remove IPL.BIN and sort.file as they are needed to make sure it works.
IO.SYS must be set at specific logical sector.