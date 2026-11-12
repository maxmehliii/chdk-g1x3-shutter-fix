/*
 * kbd_d.c - g1x3 Stufe d: Tastenlogik "S1 gehalten + Q/SET = S2" (Option A).
 *
 * Laeuft in mykbd_task (PhySw-Kopie), direkt nach dem vollstaendigen Einlesen der Tasten durch die Firmware
 * und bevor kbd_p1_f_cont daraus Events macht. Nur hier wird physw_status gelesen und veraendert.
 *
 * 1. SD-Schreibschutzbit p0 0x20 loeschen (wie Stufe c2, sonst speichert Canon auf der LOCK-Karte nichts).
 * 2. Tastenlogik (Tabelle in CLAUDE.md):
 *    - S1 = Canon-Zustand 0x9e48 Bit0 (von PhySw_ReleaseSw, bereits entprellt), zusaetzlich >= 50 ms stabil.
 *    - Q/SET = physw_status[2] Bit 0x2000, 0 = gedrueckt (Log Stufe c2).
 *    - Q/SET-Flanke (los -> gedrueckt), waehrend S1 stabil gehalten:
 *        Q/SET wird versteckt (Bit auf 1 = los) und das virtuelle S2 eingeschaltet:
 *        vs2_active = 1 und Nachricht 4 an die Queue von PhySw_ReleaseSw (task_PhySw_ReleaseSw_my in boot.c).
 *    - Q/SET gehalten und S1 gehalten: S2 bleibt (Serie/Video wie echter Ausloeser).
 *    - S1 los: vs2_active = 0 (S2 los). Q/SET bleibt versteckt, bis es losgelassen ist.
 *    - Q/SET los: vs2_active = 0, Q/SET wieder normal.
 *    - Erst Q/SET, dann S1: keine Flanke waehrend S1 -> nichts passiert, Q/SET geht normal an Canon.
 *    Das Loslassen von S2 erledigt Canons eigener 10-ms-Poll in PhySw_ReleaseSw, sobald vs2_active = 0.
 *    Alle anderen Bits bleiben unveraendert.
 */
extern void _kbd_read_keys(void);
extern int  _GetSystemTime(void);
extern int  _TryPostMessageQueue(int queue, int msg);
extern volatile unsigned int physw_status[3];

#define SD_READONLY_FLAG 0x00000020
#define SD_READONLY_IDX  0
#define QSET_FLAG        0x00002000
#define QSET_IDX         2

#define RSW_BASE         0x00009e3c                                  // PhySw_ReleaseSw-Block (verifiziert)
#define RSW_QUEUE        (*(volatile int *)(RSW_BASE + 0x08))        // Message-Queue
#define RSW_STATE        (*(volatile unsigned int *)(RSW_BASE + 0x0c)) // = 0x9e48, Bit0 S1, Bit1 S2
#define RSW_MSG_VS2      4                                           // neue Nachricht: virtuelles S2 druecken

#define S1_STABLE_MS     50

volatile int vs2_active;              // gelesen von task_PhySw_ReleaseSw_my
volatile int qset_hidden;             // Q/SET wird vor Canon versteckt
volatile unsigned int physw0_raw;     // Rohwoerter fuer das Log
volatile unsigned int physw2_raw;

static int s1_on, s1_t, qset_prev;

static void vs2_set(int on)
{
    if (on) {
        vs2_active = 1;
        _TryPostMessageQueue(RSW_QUEUE, RSW_MSG_VS2);
    } else {
        vs2_active = 0;                // Canons Poll meldet UnpressSW2 innerhalb von 10 ms
    }
}

// aus wrap_kbd_p1_f (boot.c), anstelle des Firmware-Aufrufs kbd_read_keys in kbd_p1_f
void my_kbd_read_keys(void)
{
    int now, s1, s1_stable, qset;

    _kbd_read_keys();
    physw0_raw = physw_status[SD_READONLY_IDX];
    physw2_raw = physw_status[QSET_IDX];
    physw_status[SD_READONLY_IDX] &= ~SD_READONLY_FLAG;

    now  = _GetSystemTime();
    s1   = RSW_STATE & 1;
    qset = !(physw2_raw & QSET_FLAG);

    if (s1) {
        if (!s1_on) { s1_on = 1; s1_t = now; }
    } else {
        s1_on = 0;
    }
    s1_stable = s1_on && (now - s1_t) >= S1_STABLE_MS;

    if (!qset) {                       // Q/SET los: alles zurueck
        if (vs2_active) vs2_set(0);
        qset_hidden = 0;
    } else if (qset_hidden) {          // Q/SET gehalten, Kombination laeuft
        if (!s1 && vs2_active) vs2_set(0);   // S1 zuerst losgelassen: S2 los, Q/SET bleibt versteckt
    } else if (!qset_prev && s1_stable) {    // neue Q/SET-Flanke bei gehaltenem S1
        qset_hidden = 1;
        vs2_set(1);
    }
    qset_prev = qset;

    if (qset_hidden)
        physw_status[QSET_IDX] |= QSET_FLAG; // Canon sieht Q/SET "los"
}
