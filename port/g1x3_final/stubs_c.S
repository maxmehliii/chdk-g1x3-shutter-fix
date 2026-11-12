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
