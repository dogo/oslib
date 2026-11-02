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
| `pixelcache` | Pixels set with `oslSetImagePixel` (written uncached) survive `oslUncacheImage` when the image reuses a block its previous owner left as dirty cache lines, and when the next heap block is written through the cache; RAM image data starts on a cache line |
| `rotate` | `oslDrawImage`'s rotated path (any angle, or a hotspot) draws a mirrored 100 px wide image in the same place as the plain one with its halves swapped, at 0/90/37 degrees, and finishes (it looped forever on widths not a multiple of 64) |
| `imgload` | PNG (RGBA/RGB/16-bit/interlaced/palette 8 and 4 bit/tRNS/grey variants), JPEG (baseline, progressive) and GIF load into `OSL_PF_8888` with the pixels ImageMagick decodes from the same file (`gen.sh`), and grey PNGs key out the colour key like RGB ones |
| `mp3` | MP3 files decode frame by frame on the Media Engine with the right frame count, level, pitch and PCM: MPEG-1 44.1 kHz (with an ID3v2 tag) and 48 kHz, MPEG-2 22.05 kHz mono. Copy the `.mp3` files along with the EBOOT when running from the memory stick |
| `at3` | ATRAC3 at 132/105/66 kbps (block align 384/304/192, the last joint stereo) and ATRAC3plus decode frame by frame on the Media Engine with exactly data size / block align frames, the right level and pitch. `gen.sh` regenerates the files from a 440 Hz sine with [atracdenc](https://github.com/dcherednik/atracdenc). Copy the `.at3` files along with the EBOOT when running from the memory stick |
