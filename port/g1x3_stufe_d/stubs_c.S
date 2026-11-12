// Stufe c: benannte Firmware-Adressen (alle sub_xxx kommen aus stubs_auto.S)
// Belege: NOTES.md, "Prompt 6 - Adressliste"
#include "stubs_asm.h"
    .align 2
DEF(task_TricInitTask,      0xe03b13fb)   // Prompt 4
DEF(task_InitFileModules,   0xe0456a0d)   // Prompt 4, angelegt ueber CreateTask in FUN_e0456a5c
DEF(physw_status,           0x0006c510)   // kbd_p1_f_cont = g7x2 (0x4e440)
NHSTUB(CreateTask,          0xdffc5adf)   // Prompt 4
NHSTUB(SleepTask,           0xdffc598f)   // Sigfinder + Code (ms -> Ticks, Warten), Aufruf Sleep(20) in SdPower
NHSTUB(Open,                0xe039dc4d)   // FsIO.c-Wrapper, Flags aus fopen_low ("w" = 0x301, 0x1b6)
NHSTUB(Write,               0xe039e11d)   // Eventproc Write_FW, Aufbau = g7x2 Write
NHSTUB(Close,               0xe039dc8b)   // FsIO.c-Wrapper
NHSTUB(GetSystemTime,       0xe057bb2b)   // Wrapper um _GetSystemTime(&t), gibt t zurueck
// Stufe c2 (Prompt 6, Schreibschutz-Fix), Belege: NOTES.md "Prompt 6 - Stufe c2"
NHSTUB(kbd_read_keys,       0xe0040a47)   // Sigfinder + Code: physw_status leeren, GetKbdState, kbd_read_keys_r2
// Stufe d (Prompt 7)
NHSTUB(TryPostMessageQueue, 0xdffcc113)   // funcs_by_address.csv; ISR S1/S2 schicken damit Nachricht 1/2 an *(0x9e3c+8)
