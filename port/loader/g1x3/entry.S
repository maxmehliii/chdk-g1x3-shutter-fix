// g1x3 loader entry. The Canon firmware jumps here with "bx 0x8001" (Thumb),
// after it has copied DISKBOOT.BIN to 0x40008000 (= 0x8000 uncached)
// (sub_e05812c4 / loc_e05c0214 in the g1x3 dump, Prompt 5).
.section .entry
.code 16
.align 2
.syntax unified
    LDR     SP, =0x2000
    B       my_restart
