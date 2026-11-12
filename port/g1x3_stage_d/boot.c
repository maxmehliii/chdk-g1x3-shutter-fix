/*
 * boot.c - g1x3 (G1 X Mark III) GM1.00A - STAGE c (test build with read-only logger, not the final port)
 *
 * = boot.c from g1x3_stage_ab (tested on the camera) plus:
 *   - the CreateTask hook also replaces task_InitFileModules (card ready -> logger may write)
 *   - task_Startup creates the task "KeyLog" after PhySw (logger.c, read only)
 *
 * Contents: only the restart of the Canon firmware from RAM.
 *   - copy of boot() @0xE0020011 with CreateTask hook (sole purpose: TricInit fix)
 *   - move the heap start behind this code (sub_e0020108)
 *   - start chain sub_e0020320 -> sub_e0020530 -> power-on check sub_e0040842
 *   - task_Startup WITHOUT the diskboot call (sub_e058131c), with SD power cycle (sub_e0543b3e)
 * NOT included: spytask, CHDK core, key hook (PhySw stays Canon), other task hooks.
 *
 * Generated from capdis output of the g1x3 dump (format -f=chdk), changes marked with "+"/"-".
 * Every called or patched address: docs/NOTES_de.md, "Prompt 5 - Adressliste".
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
// CreateTask hook: only TricInitTask is replaced, everything else goes to Canon unchanged.
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
            // execute the 8 overwritten bytes @0xdffc5ade, then continue in the firmware
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
            "    beq     loc_boot\n"                    // + core 0 boots
            "    adr     r0, loc_boot\n"                // +
            "    orr     r0, #1\n"                      // +
            "    bl      sub_e05c01f8\n"                // + park core1 (waits twice for SGI 10), then continue at r0
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
            // extra block g1x3 (core 0 only), 1:1 from the firmware
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

            // + write the CreateTask hook into the RAM copy of the kernel (8 bytes @0xdffc5ade)
            // halfword by halfword, because the target is not word aligned
            "    adr     r0, patch_CreateTask\n"        // source
            "    ldr     r1, =0xdffc5ade\n"             // target: hook_CreateTask
            "    add     r2, r0, #8\n"
            "patch_hook_loop:\n"
            "    ldrh    r3, [r0], #2\n"
            "    strh    r3, [r1], #2\n"
            "    cmp     r0, r2\n"
            "    blo     patch_hook_loop\n"

            // original: cache operations over the whole kernel copy (now incl. hook)
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

            // patch data, same alignment as the target (0xdffc5ade = 2 mod 4)
            "    .align  2\n"
            "    .short  0\n"
            "patch_CreateTask:\n"
            "    ldr.w   pc, _createtask_my\n"
            "_createtask_my:\n"
            "    .long   CreateTask_my + 1\n"           // Thumb address
            "    .align  1\n"
            "    .ltorg\n"
    );
}

/*************************************************************/
//** sub_e0020108_my @ 0xE0020109 (heap start, both cores)
void __attribute__((naked,noinline)) sub_e0020108_my() {
    asm volatile (
            // + start mode as in the g7x2 port: the call sub_e003aafe in the power-on check
            //   is dropped (see sub_e0040842_my), so the mode is set here.
            //   GPIO 0x168 (power) bit 0x10000 set = not pressed -> playback 0x400000, otherwise record 0x200000.
            //   target: variable 0x9e1c+8 (from sub_e003aafe).
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
            "    bne     skip_unblock\n"                // + core 0 only
            "    movs    r0, #1\n"                      // +
            "    bl      sub_e05812b0\n"                // + unblock core1, needed twice (park routine waits twice)

            "    movs    r0, #1\n"
            "    bl      sub_e05812b0\n"                // unblock core1
            "skip_unblock:\n"                           // +

            "    bl      sub_e002f0b8\n"                // IsNormalCameraMode_FW
            "    bl      sub_e0040842_my\n"             // Patched (was sub_e0040842) power-on check
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
            "    bl      sub_dffc5ade\n"                // CreateTask (goes through the hook)
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
//** sub_e0040842_my @ 0xE0040843 (power-on check)
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
//          "    beq     loc_e00408d4\n"                // - on restart the key may no longer be pressed
            "loc_e00408be:\n"
            "    mov     r3, r8\n"
            "    strd    r4, r6, [sp]\n"
            "    mov     r2, r5\n"
            "    mov     r1, sb\n"
            "    mov     r0, r7\n"
//          "    bl      sub_e003aafe\n"                // - start mode, replaced in sub_e0020108_my
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
            "    bl      sub_e0543b3e\n"                // + SD power cycle (UHS fix as in g7x2, there sub_e04d9986)
//          "    bl      sub_e058131c\n"                // - StartDiskboot (otherwise boot loop)
            "    bl      sub_e0040266\n"
            "    bl      sub_e04569c4\n"
            "    bl      sub_e0020878\n"
            "    bl      sub_e0020812\n"                // init_ex_drivers
            "    bl      sub_e0558dc2\n"
            "    bl      sub_e003d870\n"                // init_error_handlers
            "    bl      sub_e04569ca\n"
            "    bl      sub_e004074a_my\n"             // create PhySw, task -> mykbd_task (stage c2/d)
            "    bl      logger_start\n"               // + task KeyLog (read only)
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
//** task_TricInitTask_my @ 0xE03B13FB (dual-core fix as in g7x2)
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

            "    ldr     r0, =0xd2050074\n"             // + Smac register
            "    ldr     r0, [r0]\n"                    // + != 0: core/Smac already running (from the first start)
            "    cmp     r0, #0\n"                      // +
            "    beq     tric1\n"                       // +
            "    ldr     r0, [r4]\n"                    // +
            "    mov     r1, #0x80\n"                   // + SmacIdleCmp flag (FUN_e03bf7a4)
            "    bl      sub_dffcbc80\n"                // + SetEventFlag
            "tric1:\n"                                  // +

            "    bl      sub_e03b18ee\n"
            "    ldr     pc, =0xe03b1453\n"             // Continue in firmware (b loc_e03b1496)
            "    .ltorg\n"
    );
}

/*************************************************************/
//** init_file_modules_task_my @ 0xE0456A0D (as in g7x2, without MBR patch)
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
            "    bl      logger_fs_ready\n"            // + card mounted
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

/*************************************************************/
//** sub_e004074a_my @ 0xE004074B (stage c2/d): create PhySw, 1:1 copy, only the task entry is replaced
void __attribute__((naked,noinline)) sub_e004074a_my() {
    asm volatile (
            "    push    {r2, r3, r4, lr}\n"
            "    bl      sub_e003cf80\n"
            "    bl      sub_e002f038\n"                // IsFactoryMode_FW
            "    cbnz    r0, loc_e004075a\n"
            "    bl      sub_e003cf24\n"                // OpLog.Start_FW
            "loc_e004075a:\n"
            "    ldr     r4, =0x000082b4\n"
            "    ldr     r0, [r4, #4]\n"
            "    cbnz    r0, loc_e0040776\n"
            "    movs    r1, #1\n"
            "    ldr     r3, =mykbd_task\n"             // Patched (was 0xe0040725 task_PhySw)
            "    movs    r0, #0\n"
            "    lsls    r2, r1, #0xb\n"                // stack 0x800 like Canon
            "    strd    r0, r1, [sp]\n"
            "    movs    r1, #0x13\n"
            "    ldr     r0, =0xe0040af8\n"             //  *"PhySw"
            "    bl      sub_dffcc3d6\n"                // CreateTaskStrictly_alt
            "    str     r0, [r4, #4]\n"
            "loc_e0040776:\n"
            "    pop.w   {r2, r3, r4, lr}\n"
            "    b.w     sub_e0039f7c_my\n"             // create PhySw_ReleaseSw, task -> copy with virtual S2 (stage d)
            "    .ltorg\n"
    );
}

/*************************************************************/
//** mykbd_task (stage c2/d): 1:1 like task_PhySw @ 0xE0040725, only kbd_p1_f -> wrap_kbd_p1_f
void __attribute__((naked,noinline)) mykbd_task() {
    asm volatile (
            "    push    {r4, lr}\n"
            "    ldr     r4, =0x000082b4\n"             // PhySw control block: [0] running, [8] wait time
            "    b       mykbd_chk\n"
            "mykbd_loop:\n"
            "    ldr     r0, [r4, #8]\n"
            "    bl      _SleepTask\n"                  // 0xdffc598f
            "    bl      wrap_kbd_p1_f\n"               // instead of kbd_p1_f 0xe0040a7d
            "    cmp     r0, #1\n"
            "    bne     mykbd_chk\n"
            "    bl      sub_e0040478\n"                // kbd_p2_f
            "mykbd_chk:\n"
            "    ldr     r0, [r4, #0]\n"
            "    cmp     r0, #0\n"
            "    bne     mykbd_loop\n"
            "    pop.w   {r4, lr}\n"
            "    b.w     sub_dffc5cd6\n"                // ExitTask
            "    .ltorg\n"
    );
}

/*************************************************************/
//** wrap_kbd_p1_f (stage c2/d): start of kbd_p1_f @ 0xE0040A7D, reading via my_kbd_read_keys
void __attribute__((naked,noinline)) wrap_kbd_p1_f() {
    asm volatile (
            "    push    {r1, r2, r3, r4, r5, r6, r7, lr}\n"
            "    movs    r4, #0\n"
            "    bl      my_kbd_read_keys\n"            // kbd_read_keys + clear SD write-protect bit (kbd_c2.c)
            "    ldr.w   pc, =0xe0040a85\n"             // kbd_p1_f_cont (firmware, ends with pop {r1-r7,pc})
            "    .ltorg\n"
    );
}

/*************************************************************/
//** sub_e0039f7c_my @ 0xE0039F7D (stage d): create PhySw_ReleaseSw, 1:1, only the task entry is replaced
void __attribute__((naked,noinline)) sub_e0039f7c_my() {
    asm volatile (
            "    push    {r2, r3, r4, lr}\n"
            "    ldr     r4, =0x00009e3c\n"
            "    ldr     r0, [r4, #4]\n"
            "    cmp     r0, #0\n"
            "    bne     loc_e0039fbc\n" //  return
            "    movs    r1, #1\n"
            "    ldr     r3, =task_PhySw_ReleaseSw_my\n" // Patched (was 0xe0039eef task_PhySw_ReleaseSw)
            "    lsls    r2, r1, #0xa\n"
            "    strd    r0, r1, [sp]\n"
            "    movs    r1, #0x13\n"
            "    ldr     r0, =0xe003a064\n" //  *"PhySw_ReleaseSw"
            "    bl      sub_dffcc3d6\n" // CreateTaskStrictly_alt
            "    str     r0, [r4, #4]\n"
            "    movs    r1, #3\n"
            "    ldr     r0, [r4, #8]\n"
            "    movw    r2, #0x2710\n"
            "    bl      sub_dffcc090\n" // PostMessageQueue
            "    lsls    r0, r0, #0x1f\n"
            "    beq     loc_e0039fbc\n" //  return
            "    add     sp, #8\n"
            "    movs    r0, #0\n"
            "    mov.w   r2, #0x162\n"
            "    pop.w   {r4, lr}\n"
            "    ldr     r1, =0xe003a044\n" //  *"PhySwIsr.c"
            "    b.w     sub_dffc5e24\n" // DebugAssert
            "loc_e0039fbc:\n"
            "    pop     {r2, r3, r4, pc}\n"
            "    .ltorg\n"
    );
}

/*************************************************************/
//** task_PhySw_ReleaseSw_my @ 0xE0039EEF (stage d, option A): 1:1 copy, plus virtual S2
// Additions (marked "+"): message 4 = press S2; in the 10 ms poll (message 3) the S2 level counts as pressed
// while vs2_active is set. Canon's own poll does the release as soon as vs2_active = 0.
void __attribute__((naked,noinline)) task_PhySw_ReleaseSw_my() {
    asm volatile (
            "    ldr     r6, =0x00009e3c\n"
            "    movs    r5, #0\n"
            "    ldr     r4, =0x0006f754\n"
            "    push    {r3, lr}\n"
            "loc_e0039ef6:\n"
            "    movs    r2, #0\n"
            "    ldr     r0, [r6, #8]\n"
            "    mov     r1, sp\n"
            "    bl      sub_dffcbfb4\n" // ReceiveMessageQueue
            "    cbz     r0, loc_e0039f0e\n"
            "    movs    r0, #0\n"
            "    mov.w   r2, #0x1bc\n"
            "    ldr     r1, =0xe003a044\n" //  *"PhySwIsr.c"
            "    bl      sub_dffc5e24\n" // DebugAssert
            "loc_e0039f0e:\n"
            "    ldr     r0, [sp]\n"
            "    cmp     r0, #1\n"
            "    beq     loc_e0039f34\n"
            "    cmp     r0, #2\n"
            "    beq     loc_e0039f38\n"
            "    cmp     r0, #4\n"                 // + message 4 (from my_kbd_read_keys): press virtual S2
            "    beq     rsw_vs2_press\n"          // +
            "    cmp     r0, #3\n"
            "    bne     loc_e0039ef6\n"
            "    ldr     r0, =0xd5013000\n"
            "    ldr     r1, [r0, #0x3c]\n"
            "    ldr     r0, [r0, #0x38]\n"
            "    ldr     r2, =vs2_active\n"        // + virtual S2 active?
            "    ldr     r2, [r2]\n"               // +
            "    cbz     r2, rsw_hw_s2\n"          // +
            "    bic     r1, r1, #1\n"             // + then S2 level = 0 (pressed), like the real button
            "rsw_hw_s2:\n"                         // +
            "    lsls    r1, r1, #0x1f\n"
            "    beq     loc_e0039f40\n"
            "    ldrb    r1, [r6, #0xc]\n"
            "    lsls    r1, r1, #0x1e\n"
            "    bpl     loc_e0039f40\n"
            "    ldr     r1, [r4, #0xc]\n"
            "    adds    r1, r1, #1\n"
            "    str     r1, [r4, #0xc]\n"
            "    b       loc_e0039f42\n"
            "loc_e0039f34:\n"
            "    movs    r0, #0\n"
            "    b       loc_e0039f3a\n"
            "loc_e0039f38:\n"
            "    movs    r0, #1\n"
            "loc_e0039f3a:\n"
            "    bl      sub_e0039e98\n"
            "    b       loc_e0039ef6\n"
            "loc_e0039f40:\n"
            "    str     r5, [r4, #0xc]\n"
            "loc_e0039f42:\n"
            "    ldrb    r1, [r6, #0xc]\n"
            "    ands    r0, r1\n"
            "    lsls    r0, r0, #0x1f\n"
            "    beq     loc_e0039f52\n"
            "    ldr     r0, [r4, #4]\n"
            "    adds    r0, r0, #1\n"
            "    str     r0, [r4, #4]\n"
            "    b       loc_e0039f54\n"
            "loc_e0039f52:\n"
            "    str     r5, [r4, #4]\n"
            "loc_e0039f54:\n"
            "    ldr     r0, [r4, #0xc]\n"
            "    cbz     r0, loc_e0039f60\n"
            "    movs    r0, #1\n"
            "    bl      sub_e0039e0c\n"
            "    str     r5, [r4, #0xc]\n"
            "loc_e0039f60:\n"
            "    ldr     r0, [r4, #4]\n"
            "    cbz     r0, loc_e0039f6c\n"
            "    movs    r0, #0\n"
            "    bl      sub_e0039e0c\n"
            "    str     r5, [r4, #4]\n"
            "loc_e0039f6c:\n"
            "    movs    r3, #0\n"
            "    ldr     r2, =0xe0039e03\n"
            "    movw    r0, #0x2710\n"
            "    mov     r1, r2\n"
            "    bl      sub_e04f285e\n"
            "    b       loc_e0039ef6\n"
            "rsw_vs2_press:\n"                     // + message 4
            "    ldr     r2, =vs2_active\n"        // + only if virtual S2 is still active
            "    ldr     r2, [r2]\n"               // +
            "    cmp     r2, #0\n"                 // +
            "    beq     loc_e0039ef6\n"           // +
            "    ldrb    r1, [r6, #0xc]\n"         // + and only if Canon has S1 pressed (state bit0)
            "    lsls    r1, r1, #0x1f\n"          // +
            "    beq     loc_e0039ef6\n"           // +
            "    ldr     r0, =0x80000001\n"        // + press S2, exactly like FUN_e0039e98 at level 0
            "    bl      sub_e0039e0c\n"           // + state machine
            "    b       loc_e0039ef6\n"           // +
            "    .ltorg\n"
    );
}
