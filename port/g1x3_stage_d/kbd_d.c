/*
 * kbd_d.c - g1x3 stage d: key logic "S1 held + Q/SET = S2" (option A).
 *
 * Runs in mykbd_task (PhySw copy), right after the firmware has read all keys
 * and before kbd_p1_f_cont turns them into events. This is the only place that reads and changes physw_status.
 *
 * 1. Clear the SD write-protect bit p0 0x20 (as in stage c2, otherwise Canon saves nothing on the LOCKed card).
 * 2. Key logic (table in README.md, "Controls"):
 *    - S1 = Canon state 0x9e48 bit0 (from PhySw_ReleaseSw, already debounced), additionally stable for >= 50 ms.
 *    - Q/SET = physw_status[2] bit 0x2000, 0 = pressed (log of stage c2).
 *    - Q/SET edge (released -> pressed) while S1 is held stable:
 *        Q/SET is hidden (bit set to 1 = released) and the virtual S2 is switched on:
 *        vs2_active = 1 and message 4 to the queue of PhySw_ReleaseSw (task_PhySw_ReleaseSw_my in boot.c).
 *    - Q/SET held and S1 held: S2 stays (burst/video like the real shutter).
 *    - S1 released: vs2_active = 0 (S2 released). Q/SET stays hidden until it is released.
 *    - Q/SET released: vs2_active = 0, Q/SET back to normal.
 *    - Q/SET first, then S1: no edge while S1 is held -> nothing happens, Q/SET goes to Canon as usual.
 *    Releasing S2 is done by Canon's own 10 ms poll in PhySw_ReleaseSw as soon as vs2_active = 0.
 *    All other bits stay unchanged.
 */
extern void _kbd_read_keys(void);
extern int  _GetSystemTime(void);
extern int  _TryPostMessageQueue(int queue, int msg);
extern volatile unsigned int physw_status[3];

#define SD_READONLY_FLAG 0x00000020
#define SD_READONLY_IDX  0
#define QSET_FLAG        0x00002000
#define QSET_IDX         2

#define RSW_BASE         0x00009e3c                                  // PhySw_ReleaseSw block (verified)
#define RSW_QUEUE        (*(volatile int *)(RSW_BASE + 0x08))        // message queue
#define RSW_STATE        (*(volatile unsigned int *)(RSW_BASE + 0x0c)) // = 0x9e48, bit0 S1, bit1 S2
#define RSW_MSG_VS2      4                                           // new message: press virtual S2

#define S1_STABLE_MS     50

volatile int vs2_active;              // read by task_PhySw_ReleaseSw_my
volatile int qset_hidden;             // Q/SET is hidden from Canon
volatile unsigned int physw0_raw;     // raw words for the log
volatile unsigned int physw2_raw;

static int s1_on, s1_t, qset_prev;

static void vs2_set(int on)
{
    if (on) {
        vs2_active = 1;
        _TryPostMessageQueue(RSW_QUEUE, RSW_MSG_VS2);
    } else {
        vs2_active = 0;                // Canon's poll reports UnpressSW2 within 10 ms
    }
}

// called from wrap_kbd_p1_f (boot.c) instead of the firmware call kbd_read_keys in kbd_p1_f
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

    if (!qset) {                       // Q/SET released: reset everything
        if (vs2_active) vs2_set(0);
        qset_hidden = 0;
    } else if (qset_hidden) {          // Q/SET held, combination active
        if (!s1 && vs2_active) vs2_set(0);   // S1 released first: S2 released, Q/SET stays hidden
    } else if (!qset_prev && s1_stable) {    // new Q/SET edge while S1 is held
        qset_hidden = 1;
        vs2_set(1);
    }
    qset_prev = qset;

    if (qset_hidden)
        physw_status[QSET_IDX] |= QSET_FLAG; // Canon sees Q/SET "released"
}
