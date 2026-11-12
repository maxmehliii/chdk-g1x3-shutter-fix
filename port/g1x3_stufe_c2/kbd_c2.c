/*
 * kbd_c2.c - g1x3 Stufe c2: einzige Aenderung gegenueber Canon im PhySw-Ablauf.
 *
 * Die Firmware entscheidet "Karte schreibgeschuetzt" aus physw_status[0] Bit 0x20:
 *   GetSDProtect_FW (0xe0040a19) -> 0xe003b0fc liest Tabelle 0xe063ee34 (+8 = Eintrag 0xe063ee3c,
 *   info 0x01000205 -> Wort 0, Bit 5, Event 0x30a) und gibt (physw_status[0] >> 5) & 1 zurueck.
 * Der Kartenboot verlangt den Schieber auf LOCK -> Bit = 1 -> Canon schreibt nichts auf die Karte.
 * Wie in jedem CHDK-Port (core/kbd_common.c: physw_status[SD_READONLY_IDX] &= ~SD_READONLY_FLAG)
 * wird dieses eine Bit nach jedem Einlesen geloescht. Alle Tastenbits bleiben unveraendert.
 */
extern void _kbd_read_keys(void);
extern volatile unsigned int physw_status[3];

#define SD_READONLY_FLAG 0x00000020
#define SD_READONLY_IDX  0

volatile unsigned int physw0_raw;     // Wort 0 so, wie die Hardware es geliefert hat (fuer das Log)

// aus wrap_kbd_p1_f (boot.c), anstelle des Firmware-Aufrufs kbd_read_keys in kbd_p1_f
void my_kbd_read_keys(void)
{
    _kbd_read_keys();
    physw0_raw = physw_status[SD_READONLY_IDX];
    physw_status[SD_READONLY_IDX] &= ~SD_READONLY_FLAG;
}
