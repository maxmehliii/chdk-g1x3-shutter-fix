// Stage c: named firmware addresses (all sub_xxx come from stubs_auto.S)
// Evidence: docs/NOTES_de.md, "Prompt 6 - Adressliste"
#include "stubs_asm.h"
    .align 2
DEF(task_TricInitTask,      0xe03b13fb)   // Prompt 4
DEF(physw_status,           0x0006c510)   // kbd_p1_f_cont = g7x2 (0x4e440)
NHSTUB(CreateTask,          0xdffc5adf)   // Prompt 4
NHSTUB(SleepTask,           0xdffc598f)   // sigfinder + code (ms -> ticks, wait), called as Sleep(20) in SdPower
NHSTUB(GetSystemTime,       0xe057bb2b)   // wrapper around _GetSystemTime(&t), returns t
// Stage c2 (Prompt 6, write-protect fix), evidence: docs/NOTES_de.md "Prompt 6 - Stufe c2"
NHSTUB(kbd_read_keys,       0xe0040a47)   // sigfinder + code: clear physw_status, GetKbdState, kbd_read_keys_r2
// Stage d (Prompt 7)
NHSTUB(TryPostMessageQueue, 0xdffcc113)   // funcs_by_address.csv; the S1/S2 ISRs use it to send message 1/2 to *(0x9e3c+8)
// Start-up image (after Prompt 7), evidence: docs/NOTES_de.md "Startbild"
DEF(task_StartupImage,      0xe03a5e09)   // CreateTask_FW(0xdffc5ade) in FUN_e03a5e18, name "StartupImage"
NHSTUB(dcache_clean_range,  0xe03a9971)   // funcs_by_address.csv, already used in the loader (Prompt 5)
