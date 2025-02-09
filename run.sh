mkisofs -iso-level 1 -G IPL.BIN -o output.iso -pad -N -V "FROG_FEAST" -D -input-charset ASCII -sysid "Win32" -sort sort.file CD
./Tsugaru_CUI.elf "$PWD/MARTY_ROM/" -TOWNSTYPE MARTY -CD output.iso  -NORMALFD -DONTUSEFPU -HIGHFIDELITYCPU -AUTOSCALE
