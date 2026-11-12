/*
 * splash.c - custom start-up image instead of the Canon logo at power-on (RAM only).
 *
 * Called from task_StartupImage_my right after FUN_e044c430. There AvData (FUN_e01d5b90) has allocated a
 * RAM buffer (0x2f93 bytes) for ID 0 and copied the Canon logo from ROM into it (set 0).
 * We overwrite only this RAM buffer, and only if it really contains the Canon logo.
 * ROM, flash and settings are not touched. On the next start Canon copies the original again.
 *
 * Size: Canon reports to the decoder the size of the set selected by the start-up image setting (FUN_e01d5c52):
 *   setting "1" = set 0: 0x143e bytes, setting "2" = set 1: 0x2f93 bytes (= whole buffer).
 * Before display, FUN_e01d5cc8 copies the set again from ROM, unless the set word 0xe8844 already names it.
 * So after writing we store set 1 there:
 *   - setting "2": no recopy, the decoder is told 12179 bytes -> our image.
 *   - setting "1": Canon copies its logo again and shows it (harmless fallback).
 * Our JPEG is shorter than 0x2f93, the rest of the buffer becomes 0 (camera test 2026-11-12).
 */
extern const unsigned int  splash_len;
extern const unsigned char splash_jpg[];
extern void _dcache_clean_range(void *addr, int size);

#define AV_BUF_ID0   (*(unsigned char * volatile *)0x000e87f0)   // = 0xe8844 - 0x54, pointer array [ID]
#define AV_SET_ID0   (*(volatile int *)0x000e8844)               // set currently held in the buffer of ID 0
#define ROM_LOGO     ((const unsigned char *)0xe0a741a0)          // Canon logo (set 0, ID 0)
#define BUF_SIZE     0x2f93                                       // buffer ID 0 (0xe0ab6d8c) = size of set 1 (0xe0ab6ed4)
#define SET_LARGE    1                                            // set 1 = start-up image setting "2"

void splash_patch(void)
{
    unsigned char *b = AV_BUF_ID0;
    unsigned int i;

    if (!b || AV_SET_ID0 != 0 || splash_len > BUF_SIZE)
        return;
    for (i = 0; i < 64; i++)              // does the buffer really hold the Canon logo?
        if (b[i] != ROM_LOGO[i])
            return;
    for (i = 0; i < BUF_SIZE; i++)
        b[i] = (i < splash_len) ? splash_jpg[i] : 0;
    _dcache_clean_range(b, BUF_SIZE);
    AV_SET_ID0 = SET_LARGE;
}
