// Einstieg des Stufe-a/b-Kerns. Der Loader springt nach MEMISOSTART+1 (Thumb).
    .section .entry
    .code 16
    .syntax unified
    B       startup
