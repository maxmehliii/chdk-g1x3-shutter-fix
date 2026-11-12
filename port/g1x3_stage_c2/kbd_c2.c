/*
 * kbd_c2.c - g1x3 stage c2: the only change to Canon's PhySw flow.
 *
 * The firmware decides "card write-protected" from physw_status[0] bit 0x20:
 *   GetSDProtect_FW (0xe0040a19) -> 0xe003b0fc reads table 0xe063ee34 (+8 = entry 0xe063ee3c,
 *   info 0x01000205 -> word 0, bit 5, event 0x30a) and returns (physw_status[0] >> 5) & 1.
 * Card boot requires the slider on LOCK -> bit = 1 -> Canon writes nothing to the card.
 * As in every CHDK port (core/kbd_common.c: physw_status[SD_READONLY_IDX] &= ~SD_READONLY_FLAG)
 * this one bit is cleared after every read. All key bits stay unchanged.
 */
extern void _kbd_read_keys(void);
extern volatile unsigned int physw_status[3];

#define SD_READONLY_FLAG 0x00000020
#define SD_READONLY_IDX  0

volatile unsigned int physw0_raw;     // word 0 as delivered by the hardware (for the log)

// called from wrap_kbd_p1_f (boot.c) instead of the firmware call kbd_read_keys in kbd_p1_f
void my_kbd_read_keys(void)
{
    _kbd_read_keys();
    physw0_raw = physw_status[SD_READONLY_IDX];
    physw_status[SD_READONLY_IDX] &= ~SD_READONLY_FLAG;
}
