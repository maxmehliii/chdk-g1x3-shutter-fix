/*
 * boot.c - g1x3 (G1 X Mark III) GM1.00A - STUFE c (Testbuild Lese-Logger, nicht der endgueltige Port)
 *
 * = boot.c aus g1x3_stufe_ab (auf der Kamera getestet) plus:
 *   - CreateTask-Hook ersetzt zusaetzlich task_InitFileModules (Karte bereit -> Logger darf schreiben)
 *   - task_Startup legt nach PhySw den Task "KeyLog" an (logger.c, liest nur)
 *
 * Inhalt: nur der Neustart der Canon-Firmware aus dem RAM.
 *   - Kopie von boot() @0xE0020011 mit CreateTask-Hook (einziger Zweck: TricInit-Fix)
 *   - Heap-Start hinter diesen Code verschieben (sub_e0020108)
 *   - Startkette sub_e0020320 -> sub_e0020530 -> Einschaltpruefung sub_e0040842
 *   - task_Startup OHNE Diskboot-Aufruf (sub_e058131c), mit SD-Stromzyklus (sub_e0543b3e)
 * NICHT enthalten: Spytask, CHDK-Core, Tasten-Hook (PhySw bleibt Canon), andere Task-Hooks.
 *
 * Erzeugt aus capdis-Ausgabe des g1x3-Dumps (Format -f=chdk), Aenderungen mit "+"/"-" markiert.
 * Jede aufgerufene oder gepatchte Adresse: NOTES.md, "Prompt 5 - Adressliste".
 */

extern long link_bss_start;
extern long link_bss_end;
extern char _end;

const char * const new_sa = &_end;

void boot(void);

void startup(void) {
    long *bss = &link_bss_start;
    while (bss < &link_bss_end)
        *bss++ = 0;
    boot();
}

/*************************************************************/
// CreateTask-Hook: nur TricInitTask wird ersetzt, alles andere geht unveraendert an Canon.
void __attribute__((naked,noinline)) CreateTask_my() {
    asm volatile (
            "    push   {r0}\n"
            //R3 = Pointer to task function to create
            "    ldr     r0, =task_TricInitTask\n"  // 0xe03b13fb
            "    cmp     r0, r3\n"
            "    it      eq\n"
            "    ldreq   r3, =task_TricInitTask_my\n"
            "    beq     exitHook\n"
            "    ldr     r0, =task_InitFileModules\n"  // 0xe0456a0d
            "    cmp     r0, r3\n"
            "    it      eq\n"
            "    ldreq   r3, =init_file_modules_task_my\n"
            "exitHook:\n"
            "    pop    {r0}\n"
            // die 8 ueberschriebenen Byte @0xdffc5ade ausfuehren, dann weiter in der Firmware
            "    push    {r1, r2, r3, r4, r5, r6, r7, lr}\n"
            "    mov     r4, r3\n"
            "    mov.w   r3, #0x1000\n"
            "    ldr.w   pc, =0xdffc5ae7\n"
            "    .ltorg\n"
    );
}

/*************************************************************/
//** boot @ 0xE0020011
void __attribute__((naked,noinline)) boot() {
    asm volatile (
            "    mrc     p15, #0, r0, c0, c0, #5\n"     // +
            "    ands    r0, #0xf\n"                    // +
            "    beq     loc_boot\n"                    // + Core 0 bootet
            "    adr     r0, loc_boot\n"                // +
            "    orr     r0, #1\n"                      // +
            "    bl      sub_e05c01f8\n"                // + park core1 (wartet 2x auf SGI 10), dann weiter bei r0
            "loc_boot:\n"

            "    ldr     r0, =0xe0020200\n"
            "    mcr     p15, #0, r0, c12, c0, #0\n"
            "    isb     sy\n"
            "    movw    r0, #0x2000\n"
            "    movt    r0, #0\n"
            "    mov     sp, r0\n"
            "    mrc     p15, #0, r5, c0, c0, #5\n"
            "    ands    r0, r5, #0xf\n"
            "    bne     loc_e0020044\n"
            // Zusatzblock g1x3 (nur Core 0), 1:1 aus der Firmware
            "    movw    r0, #0x1504\n"
            "    movt    r0, #0xd210\n"
            "    ldr     r1, [r0]\n"
            "    cmp     r1, #0\n"
            "    itt     eq\n"
            "    moveq   r1, #1\n"
            "    streq   r1, [r0]\n"
            "    b.w     loc_e002004c\n"
            "loc_e0020044:\n"
            "    b.w     sub_e0020108_my\n"             // Patched (was loc_e0020108)
            "loc_e002004c:\n"
            "    ldr     r0, =0xe10ab568\n"
            "    ldr     r1, =0x00008000\n"
            "    ldr     r3, =0x0006b1f8\n"
            "loc_e0020052:\n"
            "    cmp     r1, r3\n"
            "    itt     lo\n"
            "    ldrlo   r2, [r0], #4\n"
            "    strlo   r2, [r1], #4\n"
            "    blo     loc_e0020052\n"
            "    ldr     r1, =0x00339448\n"
            "    mov.w   r2, #0\n"
            "loc_e0020066:\n"
            "    cmp     r3, r1\n"
            "    it      lo\n"
            "    strlo   r2, [r3], #4\n"
            "    blo     loc_e0020066\n"
            "    ldr     r0, =0xe110e760\n"
            "    ldr     r1, =0x01900000\n"
            "    ldr     r3, =0x01901504\n"
            "loc_e0020076:\n"
            "    cmp     r1, r3\n"
            "    itt     lo\n"
            "    ldrlo   r2, [r0], #4\n"
            "    strlo   r2, [r1], #4\n"
            "    blo     loc_e0020076\n"
            "    ldr     r0, =0x01900000\n"
            "    ldr     r1, =0x00001504\n"
            "    bl      sub_e03a9970\n"                // dcache_clean_range
            "    ldr     r0, =0x01900000\n"
            "    ldr     r1, =0x00001504\n"
            "    bl      sub_e03a9a44\n"                // icache_flush_range
            "    ldr     r0, =0xe110fc64\n"
            "    ldr     r1, =0xdffc4900\n"
            "    ldr     r3, =0xdffd68b0\n"
            "loc_e002009a:\n"
            "    cmp     r1, r3\n"
            "    itt     lo\n"
            "    ldrlo   r2, [r0], #4\n"
            "    strlo   r2, [r1], #4\n"
            "    blo     loc_e002009a\n"
            "    ldr     r1, =0xdffd68b0\n"
            "    mov.w   r2, #0\n"
            "loc_e00200ae:\n"
            "    cmp     r3, r1\n"
            "    it      lo\n"
            "    strlo   r2, [r3], #4\n"
            "    blo     loc_e00200ae\n"

            // + CreateTask-Hook in die RAM-Kopie des Kernels schreiben (8 Byte @0xdffc5ade)
            // Halbwortweise, weil das Ziel nicht wortausgerichtet ist
            "    adr     r0, patch_CreateTask\n"        // Quelle
            "    ldr     r1, =0xdffc5ade\n"             // Ziel: hook_CreateTask
            "    add     r2, r0, #8\n"
            "patch_hook_loop:\n"
            "    ldrh    r3, [r0], #2\n"
            "    strh    r3, [r1], #2\n"
            "    cmp     r0, r2\n"
            "    blo     patch_hook_loop\n"

            // Original: Cache-Operationen ueber die ganze Kernel-Kopie (jetzt inkl. Hook)
            "    ldr     r0, =0xdffc4900\n"
            "    ldr     r1, =0x00011fb0\n"
            "    bl      sub_e03a9970\n"                // dcache_clean_range
            "    ldr     r0, =0xdffc4900\n"
            "    ldr     r1, =0x00011fb0\n"
            "    bl      sub_e03a9a44\n"                // icache_flush_range
//          "    ldr     r0, =0xe0020045\n"             // -
//          "    orr     r0, r0, #1\n"                  // -
//          "    bx      r0\n"                          // -
            "    b       loc_e0020044\n"                // +

            // Patch-Daten, gleiche Ausrichtung wie das Ziel (0xdffc5ade = 2 mod 4)
            "    .align  2\n"
            "    .short  0\n"
            "patch_CreateTask:\n"
            "    ldr.w   pc, _createtask_my\n"
            "_createtask_my:\n"
            "    .long   CreateTask_my + 1\n"           // Thumb-Adresse
            "    .align  1\n"
            "    .ltorg\n"
    );
}

/*************************************************************/
//** sub_e0020108_my @ 0xE0020109 (Heap-Start, beide Cores)
void __attribute__((naked,noinline)) sub_e0020108_my() {
    asm volatile (
            // + Start-Modus wie im g7x2-Port: Der Aufruf sub_e003aafe in der Einschaltpruefung
            //   entfaellt (siehe sub_e0040842_my), deshalb hier den Modus selbst setzen.
            //   GPIO 0x168 (Ein/Aus) Bit 0x10000 gesetzt = nicht gedrueckt -> Wiedergabe 0x400000, sonst Aufnahme 0x200000.
            //   Ziel: Variable 0x9e1c+8 (aus sub_e003aafe).
            "    ldr     r0, =0xd2082168\n"             // +
            "    ldr     r0, [r0]\n"                    // +
            "    tst     r0, #0x10000\n"                // +
            "    ite     ne\n"                          // +
            "    movne   r1, #0x400000\n"               // +
            "    moveq   r1, #0x200000\n"               // +
            "    ldr     r0, =0x00009e24\n"             // +
            "    str     r1, [r0]\n"                    // +

            "    push    {r4, r5, r6, lr}\n"
//          "    ldr     r0, =0x0033944f\n"             // -
            "    ldr     r0, =new_sa\n"                 // +
            "    ldr     r0, [r0]\n"                    // +
            "    add     r0, #7\n"                      // +
            "    sub     sp, #0x80\n"
            "    ldr     r1, =0x000ee64c\n"
            "    bic     r5, r0, #7\n"
            "    ldr     r0, =0x006cd400\n"
            "    subs    r0, r0, r5\n"
            "    cmp     r0, r1\n"
            "    bhs     loc_e002011e\n"
            "loc_e002011c:\n"
            "    b       loc_e002011c\n"
            "loc_e002011e:\n"
            "    mrc     p15, #0, r0, c0, c0, #5\n"
            "    and     r6, r0, #0xf\n"
            "    mov.w   r0, #0x400\n"
            "    add.w   r1, r5, #0x400\n"
            "    str     r0, [sp]\n"
            "    lsls    r2, r0, #1\n"
            "    mov     r3, r5\n"
            "    mov     r4, r1\n"
            "    mov     r0, r6\n"
            "    bl      sub_e00202dc\n"
            "    cbz     r6, loc_e0020146\n"
            "    bl      sub_dffc5780\n"
            "loc_e0020142:\n"
            "    add     sp, #0x80\n"
            "    pop     {r4, r5, r6, pc}\n"
            "loc_e0020146:\n"
            "    ldr     r0, =0x000080d4\n"
            "    mov.w   r1, #0x80000\n"
            "    str     r1, [r0]\n"
            "    ldr     r1, =0x000080d8\n"
            "    ldr     r0, =0x42aaa000\n"
            "    str     r0, [r1]\n"
            "    ldr     r1, =0x000080dc\n"
            "    ldr     r0, =0x42aac000\n"
            "    str     r0, [r1]\n"
            "    movs    r1, #0x78\n"
            "    add     r0, sp, #4\n"
            "    bl      sub_dffd1cec\n"                // bzero
            "    ldr     r0, =0x00100200\n"
            "    ldr     r1, =0x005cde00\n"
            "    ldr     r2, =0x005bebb4\n"
            "    strd    r1, r0, [sp, #4]\n"
            "    subs    r2, r2, r5\n"
            "    add.w   r0, r5, #0xc00\n"
            "    strd    r0, r2, [sp, #0xc]\n"
            "    ldr     r2, =0x005bf7b4\n"
            "    strd    r2, r1, [sp, #0x14]\n"
            "    movs    r1, #0x22\n"
            "    str     r1, [sp, #0x1c]\n"
            "    movs    r1, #0xca\n"
            "    str     r1, [sp, #0x20]\n"
            "    mov.w   r1, #0x2b0\n"
            "    str     r1, [sp, #0x24]\n"
            "    movs    r1, #0xfa\n"
            "    str     r1, [sp, #0x28]\n"
            "    mov.w   r1, #0x11a\n"
            "    str     r1, [sp, #0x2c]\n"
            "    movs    r1, #0x85\n"
            "    str     r1, [sp, #0x30]\n"
            "    movs    r1, #0x40\n"
            "    str     r1, [sp, #0x34]\n"
            "    movs    r1, #4\n"
            "    str     r1, [sp, #0x38]\n"
            "    movs    r1, #0\n"
            "    str     r1, [sp, #0x3c]\n"
            "    movs    r1, #0x10\n"
            "    str     r1, [sp, #0x60]\n"
            "    lsls    r1, r1, #8\n"
            "    str     r1, [sp, #0x64]\n"
            "    asrs    r1, r1, #4\n"
            "    str     r1, [sp, #0x68]\n"
            "    lsls    r1, r1, #5\n"
            "    str     r1, [sp, #0x6c]\n"
            "    mov.w   r1, #-0x11111112\n"
            "    b       loc_e00201bc\n"
            "loc_e00201ba:\n"
            "    stm     r4!, {r1}\n"
            "loc_e00201bc:\n"
            "    cmp     r0, r4\n"
            "    bhi     loc_e00201ba\n"
            "    movs    r2, #0\n"
            "    ldr     r1, =sub_e0020320_my\n"        // Patched (was 0xe0020321)
            "    add     r0, sp, #4\n"
            "    bl      sub_dffc4a54\n"
            "    b       loc_e0020142\n"
            "    .ltorg\n"
    );
}

/*************************************************************/
//** sub_e0020320_my @ 0xE0020321
void __attribute__((naked,noinline)) sub_e0020320_my() {
    asm volatile (
            "    push    {r4, lr}\n"
            "    ldr     r4, =0xe00203fc\n"             //  *"/_term"
            "    bl      sub_e0021246\n"
            "    ldr     r0, =0x00008464\n"
            "    ldr     r1, [r0]\n"
            "    ldr     r0, =0x000080d4\n"
            "    ldr     r0, [r0]\n"
            "    adds    r0, #0x10\n"
            "    cmp     r1, r0\n"
            "    bhs     loc_e002033c\n"
            "    ldr     r0, =0xe002040c\n"             //  *"USER_MEM size checking"
            "    bl      sub_e00203a0\n"                // init_task_error
            "loc_e002033c:\n"
            "    bl      sub_e04f2a7c\n"
            "    cmp     r0, #0\n"
            "    bge     loc_e002034a\n"
            "    ldr     r0, =0xe0020424\n"             //  *"dmSetup"
            "    bl      sub_e00203a0\n"                // init_task_error
            "loc_e002034a:\n"
            "    bl      sub_e00221c8\n"
            "    cmp     r0, #0\n"
            "    bge     loc_e0020358\n"
            "    ldr     r0, =0xe002042c\n"             //  *"termDriverInit"
            "    bl      sub_e00203a0\n"                // init_task_error
            "loc_e0020358:\n"
            "    mov     r0, r4\n"
            "    bl      sub_e002226c\n"
            "    cmp     r0, #0\n"
            "    bge     loc_e0020368\n"
            "    ldr     r0, =0xe002043c\n"             //  *"termDeviceCreate"
            "    bl      sub_e00203a0\n"                // init_task_error
            "loc_e0020368:\n"
            "    mov     r0, r4\n"
            "    bl      sub_e00205d0\n"
            "    cmp     r0, #0\n"
            "    bge     loc_e0020378\n"
            "    ldr     r0, =0xe0020450\n"             //  *"stdioSetup"
            "    bl      sub_e00203a0\n"                // init_task_error
            "loc_e0020378:\n"
            "    bl      sub_e0020714\n"
            "    cmp     r0, #0\n"
            "    bge     loc_e0020386\n"
            "    ldr     r0, =0xe002045c\n"             //  *"stdlibSetup"
            "    bl      sub_e00203a0\n"                // init_task_error
            "loc_e0020386:\n"
            "    bl      sub_e0028458\n"
            "    cmp     r0, #0\n"
            "    bge     loc_e0020394\n"
            "    ldr     r0, =0xe0020468\n"             //  *"extlib_setup"
            "    bl      sub_e00203a0\n"                // init_task_error
            "loc_e0020394:\n"
            "    bl      sub_e00203b6\n"
            "    pop.w   {r4, lr}\n"
            "    b.w     sub_e0020530_my\n"             // Patched (was loc_e0020530)
            "    .ltorg\n"
    );
}

/*************************************************************/
//** sub_e0020530_my @ 0xE0020531
void __attribute__((naked,noinline)) sub_e0020530_my() {
    asm volatile (
            "    push    {r3, lr}\n"
            "    movs    r0, #2\n"
            "    mov.w   r1, #0x1f0\n"
            "    bl      sub_e012e258\n"
            "    bl      sub_e00207e0\n"                //  return
            "    bl      sub_e002079c\n"

            "    mrc     p15, #0, r0, c0, c0, #5\n"     // +
            "    ands    r0, r0, #0xf\n"                // +
            "    bne     skip_unblock\n"                // + nur Core 0
            "    movs    r0, #1\n"                      // +
            "    bl      sub_e05812b0\n"                // + unblock core1, 2x noetig (Park-Routine wartet 2x)

            "    movs    r0, #1\n"
            "    bl      sub_e05812b0\n"                // unblock core1
            "skip_unblock:\n"                           // +

            "    bl      sub_e002f0b8\n"                // IsNormalCameraMode_FW
            "    bl      sub_e0040842_my\n"             // Patched (was sub_e0040842) Einschaltpruefung
            "    cbz     r0, loc_e0020574\n"
            "    bl      sub_dffc59d4\n"
            "    ldr     r1, =0x006ce000\n"
            "    movs    r0, #0\n"
            "    bl      sub_e03a95a8\n"                //  return 0
            "    ldr     r3, =task_Startup_my\n"        // Patched (was 0xe00204b5)
            "    movs    r0, #0\n"
            "    mov     r2, r0\n"
            "    str     r0, [sp]\n"
            "    movs    r1, #0x19\n"
            "    ldr     r0, =0xe0020594\n"             //  *"Startup"
            "    bl      sub_dffc5ade\n"                // CreateTask (laeuft ueber den Hook)
            "    movs    r0, #0\n"
            "    pop     {r3, pc}\n"
            "loc_e0020574:\n"
            "    bl      sub_e00207d0\n"
            "loc_e0020578:\n"
            "    b       loc_e0020578\n"
            "    .ltorg\n"
    );
}

/*************************************************************/
//** sub_e0040842_my @ 0xE0040843 (Einschaltpruefung)
void __attribute__((naked,noinline)) sub_e0040842_my() {
    asm volatile (
            "    push.w  {r2, r3, r4, r5, r6, r7, r8, sb, sl, lr}\n"
            "    movs    r5, #0\n"
            "    mov     sl, r0\n"
            "    mov     r4, r5\n"
            "    bl      sub_e003aafa\n"                //  return
            "    mov.w   r0, #0x168\n"
            "    bl      sub_e0039cc6\n"
            "    movs    r6, #1\n"
            "    bic.w   r7, r6, r0\n"
            "    mov.w   r0, #0x150\n"
            "    bl      sub_e0039cc6\n"
            "    bic.w   sb, r6, r0\n"
            "    movs    r0, #0\n"
            "    bl      sub_e003aaf6\n"                //  return 0x1
            "    cbz     r0, loc_e004087e\n"
            "    mov.w   r0, #0x16c\n"
            "    bl      sub_e0039cc6\n"
            "    bic.w   r5, r6, r0\n"
            "loc_e004087e:\n"
            "    mov.w   r0, #0x2c0\n"
            "    bl      sub_e0039cc6\n"
            "    bic.w   r8, r6, r0\n"
            "    movs    r0, #1\n"
            "    bl      sub_e003aaf6\n"                //  return 0x1
            "    cbz     r0, loc_e004089e\n"
            "    mov.w   r0, #0x194\n"
            "    bl      sub_e0039cc6\n"
            "    bic.w   r4, r6, r0\n"
            "loc_e004089e:\n"
            "    mov.w   r0, #0x1e4\n"
            "    bl      sub_e0039cc6\n"
            "    bics    r6, r0\n"
            "    cmp.w   sl, #0\n"
            "    beq     loc_e00408be\n"
            "    orr.w   r0, r7, sb\n"
            "    orr.w   r1, r5, r8\n"
            "    orrs    r0, r1\n"
            "    orrs    r0, r4\n"
            "    orrs    r0, r6\n"
//          "    beq     loc_e00408d4\n"                // - beim Neustart ist evtl. keine Taste mehr gedrueckt
            "loc_e00408be:\n"
            "    mov     r3, r8\n"
            "    strd    r4, r6, [sp]\n"
            "    mov     r2, r5\n"
            "    mov     r1, sb\n"
            "    mov     r0, r7\n"
//          "    bl      sub_e003aafe\n"                // - Start-Modus, ersetzt in sub_e0020108_my
//          "    bl      sub_e003aafc\n"                // -  return
            "    movs    r0, #1\n"
            "loc_e00408d4:\n"
            "    pop.w   {r2, r3, r4, r5, r6, r7, r8, sb, sl, pc}\n"
            "    .ltorg\n"
    );
}

/*************************************************************/
//** task_Startup_my @ 0xE00204B5
void __attribute__((naked,noinline)) task_Startup_my() {
    asm volatile (
            "    push    {r4, lr}\n"
            "    bl      sub_e02443ae\n"
            "    ldr     r0, =0x4194a000\n"
            "    mov.w   r1, #0x20000\n"
            "    bl      sub_e0469248\n"
            "    cbz     r0, loc_e00204d0\n"
            "    movs    r2, #0x99\n"
            "    movs    r0, #0\n"
            "    ldr     r1, =0xe0020580\n"             //  *"Startup.c"
            "    bl      sub_dffc5e24\n"                // DebugAssert
            "loc_e00204d0:\n"
            "    bl      sub_e00207b4\n"
            "    bl      sub_e05812c0\n"                //  return
            "    bl      sub_e0558d8c\n"
            "    bl      sub_e0543b3e\n"                // + SD-Stromzyklus (UHS-Fix wie g7x2, dort sub_e04d9986)
//          "    bl      sub_e058131c\n"                // - StartDiskboot (sonst Bootschleife)
            "    bl      sub_e0040266\n"
            "    bl      sub_e04569c4\n"
            "    bl      sub_e0020878\n"
            "    bl      sub_e0020812\n"                // init_ex_drivers
            "    bl      sub_e0558dc2\n"
            "    bl      sub_e003d870\n"                // init_error_handlers
            "    bl      sub_e04569ca\n"
            "    bl      sub_e004074a\n"                // PhySw anlegen: unveraendert Canon
            "    bl      logger_start\n"               // + Task KeyLog (nur lesen)
            "    bl      sub_e02b09ee\n"
            "    bl      sub_e04569e0\n"
            "    bl      sub_e0554054\n"
            "    bl      sub_e04d1dd4\n"
            "    bl      sub_e0041f40\n"
            "    bl      sub_e00d91bc\n"
            "    bl      sub_e04d1d90\n"
            "    bl      sub_e002087c\n"                //  return
            "    bl      sub_e03a6482\n"
            "    bl      sub_e04d1d62\n"
            "    pop.w   {r4, lr}\n"
            "    b.w     sub_e02443aa\n"                // jump to FW
            "    .ltorg\n"
    );
}

/*************************************************************/
//** task_TricInitTask_my @ 0xE03B13FB (Dual-Core-Fix wie g7x2)
void __attribute__((naked,noinline)) task_TricInitTask_my() {
    asm volatile (
            "    push.w  {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, ip, lr}\n"
            "    movs    r0, #8\n"
            "    ldr     r1, =0xe03b1688\n"             //  *"InitTskStart"
            "    bl      sub_e035a43e\n"
            "    ldr.w   fp, =0x0001fad8\n"
            "    mov.w   sl, #0x1000\n"
            "    ldr     r4, =0x0001fad4\n"
            "    movs    r2, #0\n"
            "    ldr     r1, =0x0703870f\n"
            "    ldr     r0, [r4]\n"
            "    bl      sub_dffcba84\n"                // WaitForAnyEventFlag
            "    lsls    r0, r0, #0x1f\n"
            "    beq     loc_e03b1430\n"
            "    movs    r0, #8\n"
            "    ldr     r1, =0xe03b16a0\n"             //  *"ER IniTskWait"
            "    bl      sub_e035a49e\n"
            "    ldr     r1, =0x0001fac0\n"
            "    movs    r0, #0\n"
            "    str     r0, [r1]\n"
            "    pop.w   {r0, r1, r2, r3, r4, r5, r6, r7, r8, sb, sl, fp, ip, pc}\n"
            "loc_e03b1430:\n"
            "    ldr     r4, =0x0001fad4\n"
            "    mov     r1, sp\n"
            "    ldr     r0, [r4]\n"
            "    bl      sub_dffcbcde\n"                // GetEventFlagValue
            "    ldr     r1, [sp]\n"
            "    ldr     r0, [r4]\n"
            "    bl      sub_dffcbcae\n"                // ClearEventFlag
            "    ldr     r0, =0x02000003\n"
            "    ldr     r7, [sp]\n"
            "    tst     r7, r0\n"
            "    beq     sub_e03b1526\n"                // jump to FW
            "    lsls    r0, r7, #0x1f\n"
            "    beq     sub_e03b1454\n"                // jump to FW

            "    ldr     r0, =0xd2050074\n"             // + Smac-Register
            "    ldr     r0, [r0]\n"                    // + != 0: Core/Smac laeuft schon (vom ersten Start)
            "    cmp     r0, #0\n"                      // +
            "    beq     tric1\n"                       // +
            "    ldr     r0, [r4]\n"                    // +
            "    mov     r1, #0x80\n"                   // + SmacIdleCmp-Flag (FUN_e03bf7a4)
            "    bl      sub_dffcbc80\n"                // + SetEventFlag
            "tric1:\n"                                  // +

            "    bl      sub_e03b18ee\n"
            "    ldr     pc, =0xe03b1453\n"             // Continue in firmware (b loc_e03b1496)
            "    .ltorg\n"
    );
}

/*************************************************************/
//** init_file_modules_task_my @ 0xE0456A0D (wie g7x2, ohne MBR-Patch)
void __attribute__((naked,noinline)) init_file_modules_task_my() {
    asm volatile (
            "    push    {r4, r5, r6, lr}\n"
            "    movs    r0, #6\n"
            "    bl      sub_e03a5a2a\n"                //  return
            "    bl      sub_e04e46d4\n"
            "    movs    r4, r0\n"
            "    movw    r5, #0x5006\n"
            "    beq     loc_e0456a28\n"
            "    movs    r1, #0\n"
            "    mov     r0, r5\n"
            "    bl      sub_e039d90c\n"                // PostLogicalEventToUI_FW
            "loc_e0456a28:\n"
            "    bl      sub_e04e46fc\n"
            "    bl      logger_fs_ready\n"            // + Karte eingehaengt
            "    cmp     r4, #0\n"
            "    bne     loc_e0456a3c\n"                //  return
            "    mov     r0, r5\n"
            "    pop.w   {r4, r5, r6, lr}\n"
            "    movs    r1, #1\n"
            "    b.w     sub_e039d90c\n"                // PostLogicalEventToUI_FW, jump to FW
            "loc_e0456a3c:\n"
            "    pop     {r4, r5, r6, pc}\n"
            "    .ltorg\n"
    );
}
