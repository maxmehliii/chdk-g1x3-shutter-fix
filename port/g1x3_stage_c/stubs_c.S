// Stage c: named firmware addresses (all sub_xxx come from stubs_auto.S)
// Evidence: docs/NOTES_de.md, "Prompt 6 - Adressliste"
#include "stubs_asm.h"
    .align 2
DEF(task_TricInitTask,      0xe03b13fb)   // Prompt 4
DEF(task_InitFileModules,   0xe0456a0d)   // Prompt 4, created via CreateTask in FUN_e0456a5c
DEF(physw_status,           0x0006c510)   // kbd_p1_f_cont = g7x2 (0x4e440)
NHSTUB(CreateTask,          0xdffc5adf)   // Prompt 4
NHSTUB(SleepTask,           0xdffc598f)   // sigfinder + code (ms -> ticks, wait), called as Sleep(20) in SdPower
NHSTUB(Open,                0xe039dc4d)   // FsIO.c wrapper, flags from fopen_low ("w" = 0x301, 0x1b6)
NHSTUB(Write,               0xe039e11d)   // eventproc Write_FW, same layout as g7x2 Write
NHSTUB(Close,               0xe039dc8b)   // FsIO.c wrapper
NHSTUB(GetSystemTime,       0xe057bb2b)   // wrapper around _GetSystemTime(&t), returns t
