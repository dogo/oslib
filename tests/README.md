# Tests

## Host unit tests (`tests/host`)

Pure-logic parts of OSLib (VRAM allocator, memory virtual files, ...) compiled
for the host with AddressSanitizer/UBSan. `stubs/oslib.h` replaces the PSP-only
header; add a source to a test with `osl_host_test()` in `CMakeLists.txt`.

```sh
cmake -S tests/host -B build-host-tests
cmake --build build-host-tests
ctest --test-dir build-host-tests --output-on-failure
```

Runs in CI on every push.

## Device tests (`tests/device`)

Behaviour that only real hardware shows (GE, VFPU, ...). PPSSPP is not a
substitute: e.g. 1.20.4 does not model the GE texture cache, even with the
software renderer.

Each test runs once without UI, writes `result.txt` with a `PASS`/`FAIL` line
per case next to itself (visual tests also save the frame they drew as
`screen.png`) and exits. Tests link the installed `libosl.a`, so `make install`
first.

**Over USB (no screen needed):** start [PSPLink](https://github.com/pspdev/psplinkusb/releases)
on the PSP with the cable plugged in (not in USB mode), then

```sh
tests/device/run.py            # all tests; or e.g. tests/device/run.py seams
```

builds each test, runs it on the PSP and prints its results; they land in
`tests/device/<test>/` on the PC. A test also fails if its `result.txt` or
`screen.png` differs from the `expected.txt`/`expected.png` recorded on a real
PSP (the PSP gives byte-identical output run to run), which catches changes the
PASS thresholds let through. After an intended change, re-record them with
`tests/device/run.py --update <test>`. `--port` changes the `usbhostfs_pc`/`pspsh`
port (default 10000). The approach comes from
[pspautotests](https://github.com/hrydgard/pspautotests).

**From the memory stick:**

```sh
make -C tests/device/texcache
cp tests/device/texcache/EBOOT.PBP /path/to/memstick/PSP/GAME/texcache/
# run it on the PSP, then read PSP/GAME/texcache/result.txt
```

CI only checks that they build.

| Test | Covers |
|---|---|
| `texcache` | Re-drawing an image after render-to-texture or a CPU write (including a >512px image drawn by slice) samples the new contents, not stale texture-cache lines |
| `bilinear` | With the bilinear filter on, a 1:1 image matches its source pixel for pixel (mirrored too), while a stretched image is still filtered |
| `seams` | Images split into pieces (strip blit >64 px, `oslDrawImageBig` >512 px, map tiles) show no gaps, bleeding or per-strip sampling differences |
| `rotate` | `oslDrawImage`'s rotated path (any angle, or a hotspot) draws a mirrored 100 px wide image in the same place as the plain one with its halves swapped, at 0/90/37 degrees, and finishes (it looped forever on widths not a multiple of 64) |
