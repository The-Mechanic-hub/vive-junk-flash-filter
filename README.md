# Vive junk-flash filter (Linux / SteamVR)

Fixes one specific cause of tracking failure on the **original HTC Vive** (v1 base stations) under SteamVR on Linux: gray/white screen, the view drifting off "backwards and up", and tracking that resets every few seconds, when the headset itself is healthy.

## The symptom
- SteamVR `vrserver.txt` repeats lines like
  `Resetting tracking: IMU misalignment unreasonably large`, `position out of bounds`, `Dropped ... back-facing hits, ... non-clustered hits`
- Two different headsets failed the same way on one PC, while the wands tracked fine.
- An unrelated open-source tracker (libsurvive) failed the same way on the same data.

## The cause (in the case this was built for)
The headset was reporting extra, very short light hits, about 45-120 times a second, on 6-30 of its 32 sensors at once, locked to about **120.1 Hz**. Base-station sweeps run at 120.00 Hz, so the junk drifts through them and sensors that cannot even see a base station get counted as hits.
- Normal sweep hits: about 600-1050 ticks (12-22 us, 48 ticks = 1 us)
- Junk hits: mostly about 50-200 ticks (1-4 us)
- Replaying recordings with only the junk removed: worst false jump **18-22 m -> about 1 mm**. Dropping the same kind of hit at random did not help.
- In SteamVR with the filter: 0 resets in a 19-minute session, and the headset's "non-clustered hits" went from 10,000+ per short session to 46.

**What actually emits the junk was not found.** It may be electrical. Removing the source is better than filtering it.

## What it does
An `LD_PRELOAD` shim for SteamVR that removes hits shorter than 300 ticks from the headset's optical report (USB 28de:2000, report 0x25) before SteamVR reads them. It writes nothing to the headset and passes everything else through untouched.

## Check whether you have it
Put the headset still, in view of both base stations, and run `python3 check_hits.py`. A clear pile of hits under 300 ticks means the filter may help.

## Install
```
make
```
Then set SteamVR's launch options in Steam (SteamVR -> Properties -> Launch Options), using your own path:
```
LD_PRELOAD="$LD_PRELOAD:/full/path/to/vivefilter.so" %command%
```
Undo: clear the launch options. Environment: `VIVE_MIN_TICKS=300` changes the cutoff, `VIVE_FILTER=0` turns it off. `VIVE_FILTER_LOG=/path/file` writes hit counts (did not work inside the snap version of Steam here).

## Limits
- Tested on one system: original Vive, v1 base stations about 1.2-2.1 m from the headset, Ubuntu, Steam snap. At longer range real hits get shorter, so 300 may be too high: check the width list from `check_hits.py` first.
- Only the headset is filtered, not wands or trackers.
- Not for Vive Pro / Index / v2 base stations (different reports).
- Made with help from Claude (Anthropic). No warranty.
