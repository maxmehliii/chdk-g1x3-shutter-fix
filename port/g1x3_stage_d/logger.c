/*
 * logger.c - g1x3 stage d: key logger (read only). In addition V = virtual S2 active, H = Q/SET hidden.
 * p0/p2 are raw hardware words (Canon sees p0 without 0x20 and p2 with 0x2000 = 1 while H = 1).
 * New in c2: p0 is the raw hardware word (physw0_raw from kbd_c2.c). Canon sees p0 without bit 0x20
 * (SD write protection). No LED signal any more (was not visible during operation).
 *
 * Reads every 10 ms (read only, nothing is changed):
 *   - physw_status[0..2]          (Canon key words, 0x6c510)
 *   - RSW = state of PhySw_ReleaseSw (word 0x9e48: bit0 = S1, bit1 = S2 as Canon sees it)
 *   - S1/S2 = GPIO input 0xd20821d8 / 0xd20821d4, bit 0x10000 (1 = released, 0 = pressed)
 * Every change adds one line to a RAM buffer (max. 64 KB).
 * The file A/KEYLOG.TXT is rewritten completely when nothing happened for 1 s or
 * 5 s have passed since the last write. When the buffer is full, the log ends with "LOG FULL".
 * Size limit therefore: file <= 64 KB.
 */

extern int  _CreateTask(const char *name, int prio, int stack_size, void *entry, int arg);
extern void _SleepTask(int ms);
extern int  _Open(const char *name, int flags, int mode);
extern int  _Write(int fd, const void *buf, int len);
extern int  _Close(int fd);
extern int  _GetSystemTime(void);
extern volatile unsigned int physw_status[3];
extern volatile unsigned int physw0_raw, physw2_raw;   // kbd_d.c
extern volatile int vs2_active, qset_hidden;

#define O_WRONLY    0x001
#define O_CREAT     0x100
#define O_TRUNC     0x200      // "w" in fopen_low = 0x301, mode 0x1b6

#define LOG_NAME    "A/KEYLOG.TXT"
#define LOG_MAX     (64*1024)
#define LOG_RESERVE 64         // room for the final line
#define POLL_MS     10

#define RSW_STATE   (*(volatile unsigned int *)0x00009e48)
#define GPIO_S1     (*(volatile unsigned int *)0xd20821d8)
#define GPIO_S2     (*(volatile unsigned int *)0xd20821d4)

#define NVAL 8

static volatile int fs_ready;
static char logbuf[LOG_MAX];
static int  loglen;
static int  logfull;

// called from init_file_modules_task_my once the card is mounted
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
    v[0] = physw0_raw;          // raw value (Canon sees it without bit 0x20)
    v[1] = physw_status[1];
    v[2] = physw2_raw;
    v[3] = RSW_STATE;
    v[4] = (GPIO_S1 >> 16) & 1;
    v[5] = (GPIO_S2 >> 16) & 1;
    v[6] = vs2_active;
    v[7] = qset_hidden;
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
    put_hex(v[5], 1); put_str(" V=");
    put_hex(v[6], 1); put_str(" H=");
    put_hex(v[7], 1); put_str("\r\n");
}

static int write_file(void)
{
    int fd = _Open(LOG_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0x1b6);
    int ok;
    if (fd < 0)
        return 0;             // try again later
    ok = (_Write(fd, logbuf, loglen) == loglen);
    _Close(fd);
    return ok;
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

    put_str("KEYLOG g1x3 GM1.00A stage d - S1+Q/SET = S2 (p0/p2 = raw values, V = virtual S2, H = Q/SET hidden)\r\n");
    put_str("time(ms)  no    p0..p2 = physw_status, RSW = Canon state S1/S2 (bit0 S1, bit1 S2), S1/S2 = GPIO (1 released, 0 pressed)\r\n");
    _SleepTask(2000);                       // keep away from the blinking of Canon's start phase
    read_vals(last);
    put_line((unsigned int)_GetSystemTime(), nr++, last);
    have = 1;
    dirty = !write_file();
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
                    put_str("LOG FULL\r\n");
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

// called from task_Startup_my, after PhySw has been created
void logger_start(void)
{
    _CreateTask("KeyLog", 0x19, 0x1000, logger_task, 0);
}
