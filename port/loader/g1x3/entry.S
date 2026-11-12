// g1x3 Loader-Einstieg. Die Canon-Firmware springt mit "bx 0x8001" (Thumb) hierher,
// nachdem sie DISKBOOT.BIN nach 0x40008000 (= 0x8000 ungecacht) kopiert hat
// (sub_e05812c4 / loc_e05c0214 im g1x3-Dump, Prompt 5).
.section .entry
.code 16
.align 2
.syntax unified
    LDR     SP, =0x2000
    B       my_restart
