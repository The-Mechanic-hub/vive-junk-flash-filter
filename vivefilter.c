/* Vive junk-flash filter. LD_PRELOAD into SteamVR.
   Drops light hits shorter than VIVE_MIN_TICKS (48 ticks = 1 us, default 300) from the HEADSET's
   optical report 0x25 (28de:2000) before SteamVR reads them. Everything else passes untouched.
   No device writes. Undo: remove the SteamVR launch option. VIVE_FILTER=0 turns it off. */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
extern long syscall(long, ...);
static int min_ticks = -1;
static unsigned long seen, cut;
static void logit(const char *s, unsigned long a, unsigned long b) {   /* small status file, every ~20k packets */
    char buf[160]; size_t n = 0; const char *p = s;
    while (*p) buf[n++] = *p++;
    unsigned long v[2] = {a, b};
    for (int k = 0; k < 2; k++) { char t[24]; int i = 0; unsigned long x = v[k]; do { t[i++] = '0' + x % 10; x /= 10; } while (x); buf[n++] = ' '; while (i) buf[n++] = t[--i]; }
    buf[n++] = '\n';
    const char *path = getenv("VIVE_FILTER_LOG"); if (!path) return;
    long fd = syscall(SYS_openat, -100, path, 02001 | 0100, 0644); /* O_WRONLY|O_APPEND|O_CREAT */
    if (fd >= 0) { syscall(SYS_write, fd, buf, n); syscall(SYS_close, fd); }
}
static int is_headset(int fd) {            /* HIDIOCGRAWINFO -> bustype, vendor, product */
    struct { uint32_t bus; int16_t vid, pid; } r;
    long rc = syscall(SYS_ioctl, fd, 0x80084803UL, &r);
    return rc == 0 && (uint16_t)r.vid == 0x28de && (uint16_t)r.pid == 0x2000;
}
ssize_t read(int fd, void *buf, size_t count) {
    long n = syscall(SYS_read, fd, buf, count);
    uint8_t *b = buf;
    if (n != 64 || b[0] != 0x25) return n;
    if (min_ticks < 0) {
        const char *e = getenv("VIVE_MIN_TICKS"), *off = getenv("VIVE_FILTER");
        min_ticks = e ? atoi(e) : 300; if (off && off[0] == '0') min_ticks = 0;
    }
    if (min_ticks <= 0 || !is_headset(fd)) return n;
    int out = 0;
    for (int k = 0; k < 9; k++) {
        uint8_t *e = b + 1 + 7 * k;
        if (e[0] >= 32) continue;                       /* empty slot */
        unsigned w = e[1] | (unsigned)e[2] << 8;
        seen++;
        if (w < (unsigned)min_ticks) { cut++; continue; } /* junk */
        if (out != k) memmove(b + 1 + 7 * out, e, 7);
        out++;
    }
    memset(b + 1 + 7 * out, 0xff, 7 * (9 - out));
    static unsigned long pk; if (++pk % 20000 == 1) logit("hits/removed", seen, cut);
    return n;
}
