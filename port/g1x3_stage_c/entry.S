// Entry of the stage a/b core. The loader jumps to MEMISOSTART+1 (Thumb).
    .section .entry
    .code 16
    .syntax unified
    B       startup
