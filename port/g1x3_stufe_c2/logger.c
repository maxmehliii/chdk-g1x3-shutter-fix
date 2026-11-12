/*
 * logger.c - g1x3 Stufe c2: reiner LESE-Logger fuer Tasten (wie Stufe c).
 * Neu in c2: p0 ist das Rohwort der Hardware (physw0_raw aus kbd_c2.c). Canon sieht p0 ohne Bit 0x20
 * (SD-Schreibschutz). Nach dem ersten Schreibversuch ein LED-Zeichen: 2 s an = Datei geschrieben,
 * 2 s schnelles Blinken = Schreiben fehlgeschlagen.
 *
 * Liest alle 10 ms (nur lesen, nichts wird veraendert):
 *   - physw_status[0..2]          (Canon-Tastenwoerter, 0x6c510)
 *   - RSW = Zustand von PhySw_ReleaseSw (Wort 0x9e48: Bit0 = S1, Bit1 = S2 aus Canon-Sicht)
 *   - S1/S2 = GPIO-Eingang 0xd20821d8 / 0xd20821d4, Bit 0x10000 (1 = los, 0 = gedrueckt)
 * Bei jeder Aenderung eine Zeile in einen RAM-Puffer (max. 64 KB).
 * Die Datei A/KEYLOG.TXT wird komplett neu geschrieben, wenn 1 s nichts passiert ist oder
 * 5 s seit dem letzten Schreiben vergangen sind. Ist der Puffer voll, endet das Log mit "LOG VOLL".
 * Groessenbegrenzung damit: Datei <= 64 KB.
 */

extern int  _CreateTask(const char *name, int prio, int stack_size, void *entry, int arg);
extern void _SleepTask(int ms);
extern int  _Open(const char *name, int flags, int mode);
extern int  _Write(int fd, const void *buf, int len);
extern int  _Close(int fd);
extern int  _GetSystemTime(void);
extern volatile unsigned int physw_status[3];
extern volatile unsigned int physw0_raw;   // kbd_c2.c

#define O_WRONLY    0x001
#define O_CREAT     0x100
#define O_TRUNC     0x200      // "w" in fopen_low = 0x301, Modus 0x1b6

#define LOG_NAME    "A/KEYLOG.TXT"
#define LOG_MAX     (64*1024)
#define LOG_RESERVE 64         // Platz fuer die Schlusszeile
#define POLL_MS     10

#define RSW_STATE   (*(volatile unsigned int *)0x00009e48)
#define GPIO_S1     (*(volatile unsigned int *)0xd20821d8)
#define GPIO_S2     (*(volatile unsigned int *)0xd20821d4)
#define LED_GPIO    (*(volatile unsigned int *)0xD20801DC)   // LED0 (verifiziert, Prompt 5)
#define LED_ON      0x024D0002
#define LED_OFF     0x024C0003

#define NVAL 6

static volatile int fs_ready;
static char logbuf[LOG_MAX];
static int  loglen;
static int  logfull;

// aus init_file_modules_task_my, wenn die Karte eingehaengt ist
void logger_fs_ready(void)
{
    fs_ready = 1;
}

static void put_ch(char c)
{
    if (loglen < LOG_MAX)
        logbuf[loglen++] = c;
}

static void put_str(const char *s)
{
    while (*s)
        put_ch(*s++);
}

static void put_hex(unsigned int v, int digits)
{
    static const char hx[] = "0123456789ABCDEF";
    int i;
    for (i = digits - 1; i >= 0; i--)
        put_ch(hx[(v >> (i * 4)) & 0xf]);
}

static void put_dec(unsigned int v, int digits)
{
    char tmp[10];
    int i;
    for (i = 0; i < digits; i++) {
        tmp[i] = '0' + (v % 10);
        v /= 10;
    }
    while (i--)
        put_ch(tmp[i]);
}

static void read_vals(unsigned int *v)
{
    v[0] = physw0_raw;          // Rohwert (Canon sieht ihn ohne Bit 0x20)
    v[1] = physw_status[1];
    v[2] = physw_status[2];
    v[3] = RSW_STATE;
    v[4] = (GPIO_S1 >> 16) & 1;
    v[5] = (GPIO_S2 >> 16) & 1;
}

static void put_line(unsigned int t, unsigned int nr, const unsigned int *v)
{
    put_dec(t, 9);    put_ch(' ');
    put_dec(nr, 5);   put_str(" p0=");
    put_hex(v[0], 8); put_str(" p1=");
    put_hex(v[1], 8); put_str(" p2=");
    put_hex(v[2], 8); put_str(" RSW=");
    put_hex(v[3], 1); put_str(" S1=");
    put_hex(v[4], 1); put_str(" S2=");
    put_hex(v[5], 1); put_str("\r\n");
}

static int write_file(void)
{
    int fd = _Open(LOG_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0x1b6);
    int ok;
    if (fd < 0)
        return 0;             // spaeter erneut versuchen
    ok = (_Write(fd, logbuf, loglen) == loglen);
    _Close(fd);
    return ok;
}

// einmalig nach dem ersten Schreibversuch: ok -> 2 s an, sonst 2 s schnell blinken
static void led_signal(int ok)
{
    int i;
    if (ok) {
        LED_GPIO = LED_ON;  _SleepTask(2000);
    } else {
        for (i = 0; i < 10; i++) {
            LED_GPIO = LED_ON;  _SleepTask(100);
            LED_GPIO = LED_OFF; _SleepTask(100);
        }
    }
    LED_GPIO = LED_OFF;
}

static void logger_task(void)
{
    unsigned int last[NVAL], cur[NVAL];
    unsigned int nr = 0;
    int have = 0, dirty = 0, i, diff;
    int now, t_change = 0, t_flush = 0;

    while (!fs_ready)
        _SleepTask(100);
    _SleepTask(1000);

    put_str("KEYLOG g1x3 GM1.00A Stufe c2 - nur lesen (p0 = Rohwert, Canon sieht p0 ohne Bit 0x20 = SD-Schreibschutz)\r\n");
    put_str("Zeit(ms)  Nr    p0..p2 = physw_status, RSW = Canon-Zustand S1/S2 (Bit0 S1, Bit1 S2), S1/S2 = GPIO (1 los, 0 gedrueckt)\r\n");
    _SleepTask(2000);                       // Abstand zum Blinken der Canon-Startphase
    read_vals(last);
    put_line((unsigned int)_GetSystemTime(), nr++, last);
    have = 1;
    i = write_file();
    led_signal(i);
    dirty = !i;
    t_flush = _GetSystemTime();

    while (1) {
        now = _GetSystemTime();
        if (!logfull) {
            read_vals(cur);
            diff = !have;
            for (i = 0; i < NVAL; i++)
                if (cur[i] != last[i])
                    diff = 1;
            if (diff) {
                if (loglen > LOG_MAX - LOG_RESERVE - 80) {
                    put_str("LOG VOLL\r\n");
                    logfull = 1;
                } else {
                    put_line((unsigned int)now, nr++, cur);
                    for (i = 0; i < NVAL; i++)
                        last[i] = cur[i];
                    have = 1;
                }
                dirty = 1;
                t_change = now;
            }
        }
        if (dirty && ((now - t_change) >= 1000 || (now - t_flush) >= 5000)) {
            if (write_file())
                dirty = 0;
            t_flush = now;
        }
        _SleepTask(logfull ? 1000 : POLL_MS);
    }
}

// aus task_Startup_my, nach dem Anlegen von PhySw
void logger_start(void)
{
    _CreateTask("KeyLog", 0x19, 0x1000, logger_task, 0);
}
