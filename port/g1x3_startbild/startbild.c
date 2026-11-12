/*
 * startbild.c - eigenes Startbild statt Canon-Logo beim Einschalten (nur RAM).
 *
 * Aufgerufen aus task_StartupImage_my direkt nach FUN_e044c430. Dort hat AvData (FUN_e01d5b90) fuer ID 0
 * einen RAM-Puffer (0x2f93 Byte) angelegt und das Canon-Logo aus dem ROM hineinkopiert (Satz 0).
 * Wir ueberschreiben nur diesen RAM-Puffer, und nur wenn er wirklich das Canon-Logo enthaelt.
 * ROM, Flash und Einstellungen werden nicht beruehrt. Beim naechsten Start kopiert Canon wieder das Original.
 *
 * Groesse: Canon meldet dem Dekoder die Groesse des Satzes der Startbild-Einstellung (FUN_e01d5c52):
 *   Einstellung "1" = Satz 0: 0x143e Byte, Einstellung "2" = Satz 1: 0x2f93 Byte (= ganzer Puffer).
 * Vor der Anzeige kopiert FUN_e01d5cc8 den Satz neu aus dem ROM, ausser das Satz-Wort 0xe8844 nennt ihn schon.
 * Deshalb tragen wir nach dem Schreiben Satz 1 ein:
 *   - Einstellung "2": keine Neukopie, Dekoder bekommt unser JPEG mit 12179 Byte gemeldet -> eigenes Bild.
 *   - Einstellung "1": Canon kopiert sein Logo neu und zeigt es (harmloser Rueckfall).
 * Unser JPEG ist kuerzer als 0x2f93, der Rest des Puffers wird 0 (wie bisher, Kameratest 2026-11-12).
 */
extern const unsigned int  startbild_len;
extern const unsigned char startbild_jpg[];
extern void _dcache_clean_range(void *addr, int size);

#define AV_BUF_ID0   (*(unsigned char * volatile *)0x000e87f0)   // = 0xe8844 - 0x54, Zeiger-Array [ID]
#define AV_SET_ID0   (*(volatile int *)0x000e8844)               // Satz, der im Puffer von ID 0 liegt
#define ROM_LOGO     ((const unsigned char *)0xe0a741a0)          // Canon-Logo (Satz 0, ID 0)
#define BUF_SIZE     0x2f93                                       // Puffer ID 0 (0xe0ab6d8c) = Groesse Satz 1 (0xe0ab6ed4)
#define SET_GROSS    1                                            // Satz 1 = Startbild-Einstellung "2"

void startbild_patch(void)
{
    unsigned char *b = AV_BUF_ID0;
    unsigned int i;

    if (!b || AV_SET_ID0 != 0 || startbild_len > BUF_SIZE)
        return;
    for (i = 0; i < 64; i++)              // Puffer enthaelt wirklich das Canon-Logo?
        if (b[i] != ROM_LOGO[i])
            return;
    for (i = 0; i < BUF_SIZE; i++)
        b[i] = (i < startbild_len) ? startbild_jpg[i] : 0;
    _dcache_clean_range(b, BUF_SIZE);
    AV_SET_ID0 = SET_GROSS;
}
