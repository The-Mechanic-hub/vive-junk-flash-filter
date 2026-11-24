#!/usr/bin/env python3
"""Read-only check: counts short (junk) vs normal light hits from an original HTC Vive headset for 5 s.
Close SteamVR first (or at least leave the headset still, in view of both base stations).
If the headset has been idle since boot, start and close SteamVR once so it wakes up."""
import collections, glob, os, select, time
CUT = int(os.environ.get("VIVE_MIN_TICKS", "300"))
for h in sorted(glob.glob("/sys/class/hidraw/*")):
    if "000028DE:00002000" not in open(h + "/device/uevent").read().upper():
        continue
    fd = os.open("/dev/" + os.path.basename(h), os.O_RDONLY | os.O_NONBLOCK)
    t0, c, w = time.time(), collections.Counter(), collections.Counter()
    while time.time() - t0 < 5:
        if select.select([fd], [], [], 0.1)[0]:
            b = os.read(fd, 64)
            if len(b) == 64 and b[0] == 0x25:
                for k in range(9):
                    e = b[1 + 7 * k:8 + 7 * k]
                    if e[0] < 32:
                        n = e[1] | e[2] << 8
                        c["short" if n < CUT else "normal"] += 1
                        w[min(n // 100, 20)] += 1
    if c:
        print(f"{os.path.basename(h)}: {c['normal']} normal, {c['short']} short (<{CUT} ticks) in 5 s")
        print("widths (x100 ticks, 48 ticks = 1 us):", dict(sorted(w.items())))
