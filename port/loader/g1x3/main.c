/*
 * Loader g1x3 (PowerShot G1 X Mark III), firmware GM1.00A only.
 *
 * Same flow as loader/g7x2/main.c, but every address from the g1x3 dump
 * (evidence: docs/NOTES_de.md, section "Prompt 5 - Adressliste").
 *
 * Instead of loader/generic/check_compat.c a small check of its own:
 * Only two verified ROM locations are READ. If anything does not match,
 * the LED blinks forever and Canon is NOT restarted.
 * (Removing the battery ends this; afterwards the camera starts normally from a normal card.)
 */

extern long *blob_chdk_core;
extern long blob_chdk_core_size;

#define FW_PID_ADR   0xE1F20270  // halfword P-ID (verified)
#define FW_VER_ADR   0xE05546BD  // "GM1.00A" (verified, sigfinder line "Firmware Ver")
#define LED_GPIO     0xD20801DC  // LED0, offset 0x1dc from Canon's LED init (verified)
#define LED_ON       0x024D0002
#define LED_OFF      0x024C0003

static const char fw_ver[] = "GM1.00A";

int __attribute__((naked)) get_core_id() {
    asm volatile (
"    mrc     p15, #0, r0, c0, c0, #5\n"
"    ands    r0, #0xf\n"
"    bx      lr\n"
    );
}

static void __attribute__((noreturn)) blink_forever(void) {
    volatile int *p = (int*)LED_GPIO;
    volatile int i;
    while (1) {
        *p = LED_ON;
        for (i = 0; i < 3000000; i++) ;
        *p = LED_OFF;
        for (i = 0; i < 3000000; i++) ;
    }
}

static void check_compat_g1x3(void) {
    int i;
    if (*(volatile unsigned short*)FW_PID_ADR != PLATFORMID)
        blink_forever();
    const char *v = (const char*)FW_VER_ADR;
    for (i = 0; i < (int)sizeof(fw_ver); i++) {   // incl. terminating 0
        if (v[i] != fw_ver[i])
            blink_forever();
    }
}

static void core_copy(const long *src, long *dst, long length) {
    if (src < dst && dst < src + length) {
        src += length;
        dst += length;
        while (length--)  *--dst = *--src;
    }
    else
        while (length--)  *dst++ = *src++;
}

void __attribute__((noreturn)) my_restart()
{
    int coreid = get_core_id();

    if (!coreid) {
        check_compat_g1x3();

        long *dst = (long*)MEMISOSTART;
        const long *src = blob_chdk_core;
        long length = (blob_chdk_core_size + 3) >> 2;

        core_copy(src, dst, length);

        // sign of life: LED0 on. The Canon firmware switches it again itself during start.
        *(volatile int*)LED_GPIO = LED_ON;
    }

    asm volatile (
            "ldr    r2, =0xb5102101\n"  // first 2 instructions of sub_e05812b0 (unblock core1)
            "ldr    r1, =0xe05812b0\n"  // GM1.00A
            "ldr    r0, [r1]\n"
            "cmp    r0, r2\n"
            "bne    fail\n"             // wrong pattern: call nothing, stop here

            "movs   r0, %2\n"           // only core 0 wakes core 1
            "bne    cont1\n"
            "orr    r1, #1\n"
            "movs   r0, #1\n"
            "blx    r1\n"               // unblock core1 (SGI 10 to core 1)

        "cont1:\n"
            "mov    r1, %1\n"
            "mov    r0, %0\n"
            "ldr    r2, =0xe03a9971\n"  // dcache_clean_range
            "blx    r2\n"
            "mov    r1, %1\n"
            "mov    r0, %0\n"
            "ldr    r2, =0xe03a9a45\n"  // icache_flush_range
            "blx    r2\n"
            "mov    r0, %0\n"
            "add    r0, r0, #1\n"       // Thumb entry
            "bx     r0\n"

        "fail:\n"
            "b      fail\n"
            : : "r"(MEMISOSTART), "r"(((blob_chdk_core_size+3)>>2)<<2), "r"(coreid) : "memory","r0","r1","r2","r3","ip","lr"
    );
    while(1);
}
