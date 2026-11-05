# Tests

## CMake sample builds

The build CI job compiles every sample standalone, then builds the audio sample in
`samples/audio/build/debug` and all samples together in `samples/cbuild`. It checks
that every sample has its own EBOOT, that the audio sample's `Resources/music.bgm`
is copied byte for byte, and that standalone and nested build trees are not copied
into the sample outputs as assets. It also checks that `rectAndFonts` produces its
own executable name and that the shapes sample's C++ source is not copied as an asset.

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

`test_audio` also checks custom-driver cancellation and deletion of another channel's sound
from an end callback, using pthread conditions to hold the decoder until the test releases it.
It also checks end callbacks that play another sound: on their own voice (with another buffer
size), on each other's voice at the same time, and while the main thread deletes the new sound,
replaces it or stops the old one; pausing or entering standby while a stop is pending (the stop
wins), a stream paused across standby (stays paused, its file is reopened), a stop while
oslAudioVSync reopens a file, and a stream played after a standby (its file reopened, the closed
one freed, played from its start) or replayed while a callback the standby interrupted still
reads it (the file only changes between two callbacks); streams remember their file as an
absolute path (a virtual file's name, or a path whose absolute form doesn't fit, as given); and a thread that can't allocate its buffer (calloc failures are
injected), which can't be checked on the PSP: there a malloc after one failed on a full heap
can crash or hang in the libc. `test_bgm` checks that truncated BGM headers and formats other
than IMA-ADPCM are rejected, and that deleting a BGM frees all it allocated.

## PPSSPP tests (`tests/ppsspp`)

Behaviour that needs something a real PSP here can't provide, such as networking without a
Wi-Fi profile. Each test has a `run.sh` that builds it against the installed `libosl.a`, runs
it in the PPSSPP flatpak (restoring PPSSPP's ini afterwards) and compares `result.txt` with
`expected.txt`.

| Test | Covers |
|---|---|
| `httpget` | `oslNetGetFile` against a local server (`server.py`): a download lands complete; a 404, a connection cut short or a missing directory returns an error and leaves an existing file (and no `.oslpart`); a failed `oslNetPostForm` doesn't shut HTTP down for the next call |

The `httpget` runner refuses an existing `PSP/GAME/httpget` directory, stops only the
Flatpak instance it starts, and restores the ini on normal exit or interruption. If
restoration fails, it reports and keeps `PSP/GAME/httpget/ini.bak` for recovery.
`python3 tests/ppsspp/httpget/test_runner.py` checks cleanup using fake tools and private
temporary paths, without running PPSSPP or changing the user's configuration.
This cleanup check also runs in the host-tests CI job.

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
`tests/device/run.py --update <test>`. A test that times out is reported with the
last `usbhostfs_pc`/`pspsh` lines and, if it crashed, PSPLink's exception
record; PSPLink is then reset. Only one `usbhostfs_pc` may run: a second one
can grab the PSP when PSPLink reconnects after a test. `--port` changes the `usbhostfs_pc`/`pspsh`
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
| `formats` | An image converted to 5650/5551/4444/8-bit palette and drawn 1:1 shows its colours as the GE widens them (bit replication), linear or swizzled |
| `blend` | Each `oslSetAlpha` effect (none, RGBA, alpha, add, sub, tint, with full and half coefficients) blends a solid image over the background as its formula says, within the GE's rounding |
| `rotate` | `oslDrawImage`'s rotated path (any angle, or a hotspot) draws a mirrored 100 px wide image in the same place as the plain one with its halves swapped, at 0/90/37 degrees, and finishes (it looped forever on widths not a multiple of 64) |
| `imgload` | PNG (RGBA/RGB/16-bit/interlaced/palette 8 and 4 bit/tRNS/grey variants), JPEG (baseline, progressive, greyscale) and GIF load into `OSL_PF_8888` with the pixels ImageMagick decodes from the same file (`gen.sh`), and grey PNGs key out the colour key like RGB ones |
| `imgpal` | PNGs loaded into `OSL_PF_8BIT`/`OSL_PF_4BIT` (palette with and without tRNS, grey 4/8/16-bit and grey+tRNS, and a 256-entry palette into 4 bit, with tRNS or a colour key too, which go through `oslConvertImageTo`) resolve through their palette to the pixels ImageMagick decodes. Injected image/palette allocation failures return NULL without leaking images; failed conversions preserve their source. `imgpal.prx N` runs only pixel case N |
| `textbounds` | An SFont with more than 256 letters is rejected (it wrote past `OSL_SFONT`), one with 10 loads all 10; `oslDrawStringf` and `oslPrintf` with 1500 characters return (built with `-fstack-protector-all`, an overflowed buffer ends the test) |
| `imgmem` | A failed `oslMoveImageTo` keeps the image where it was; repeated GIF (two frames, missing file, no VRAM after the 32-bit decode) and PNG (no VRAM for the image or for its conversion) loads keep no heap; `oslConvertImageTo` without memory for the palette returns NULL; a GIF with a 256-entry colour table loads into `OSL_PF_4BIT` with the right colours |
| `drawfix` | Tile maps (normal and GBA) whose tileset has 3 tiles per line draw each tile's colour; `oslScaleImageCreate` keeps a uniform image's alpha and colour, also when only the width changes; an `OSL_PF_8BIT` image converts to 8888 through its palette; all 65536 RGB565 colours expand to 8888 with the correct bit replication |
| `dialogmem` | Autosaves free their copy of the data, including a completed save replaced without calling end; allocation and SDK initialization failures roll back all three save modes. A keyboard set up but never activated can be freed; reinitializing its data frees the old output only after allocation succeeds; `oslInitOsk`'s failures free only what they made, leaving another active keyboard alone. Heap measured with `mallinfo`; linker wrappers inject allocation and SDK failures. Writes, then removes, `ms0:/PSP/SAVEDATA/OSLTEST00MEMTEST`. The added reinitialization, failure, and replacement lines in `expected.txt` are expected results, not yet recorded on hardware |
| `wavload` | WAVs decode to the file's own samples, whole or streamed, from the memory stick or from memory (`ram:/`); 22050 Hz doubles each sample; 48 and 32 kHz files play at 404 and 303 Hz instead of 440 (no divider for those rates) |
| `bigdraw` | A 600 px wide image: `oslDrawImage` draws texels 512+ wrapped (column x shows x-512), `oslDrawImageBig` draws them right; `oslDrawImageBig` draws a 1024 px wide image right and a 1100 px one wrong (its documented width limit) |
| `imgmisc` | `oslImageIsMirroredH`/`V` report what `oslMirrorImageH`/`V` did (on a whole image and on a tile), `oslDeleteImage(NULL)` returns, and a new image's buffer (padding included) starts zeroed in RAM and VRAM |
| `keybits` | Each `OSL_KEYLIST` and `OSL_REMOTEKEYLIST` bit field reads the SDK's button bit (`PSP_CTRL_*`, `PSP_HPRM_*`) and no other field does; no buttons need pressing |
| `palvram` | With `osl_useVramManager = 0` (VRAM as a stack), a VRAM image gets allocated, and deleting a VRAM palette gives back exactly its bytes so the next image doesn't overlap a live one |
| `soundload` | Before `oslInitAudioME`, the MP3 and AT3 loaders return NULL (no crash, user mode); after it, `oslLoadSoundFile` hands `.wav`, `.bgm` (any case), `.mp3` and `.at3` to their loaders, streamed, and returns NULL for other extensions. Uses the sounds of the `wavload`, `mp3` and `at3` tests |
| `soundlife` | Missing WAV and BGM files load as NULL (no crash); `oslDeleteSound` returns only after the sound's audio thread is done with it; a channel whose thread can't be created (no stack memory) ends, and plays the next sound; stopping a sound while its callback runs ends it without its endCallback (a loop used to replay it, a self-deleting one to free it under the stop), two endCallbacks stopping each other's sound don't deadlock; an endCallback playing the next sound on its own voice (mono, 1152 samples) plays it, two playing on each other's voice at once don't deadlock; loading and deleting a BGM 20 times keeps no heap; pausing, resuming or toggling a sound whose stop an endCallback requested lets it end; a standby (its power callback) suspends a playing stream and leaves a paused one paused, and oslAudioVSync reopens both files and frees the closed ones; a stream played after the standby gets its file reopened without keeping the closed one; an endCallback replaying a stream after a standby reopens it from its audio thread, which has no current directory (the stream keeps an absolute path) |
| `soundprio` | With the main thread above the audio threads, `oslPlaySound` on a channel whose thread is ending lets that thread finish (it used to spin forever). The test times its wake-up into the few-us end window |
| `mp3` | MP3 files decode frame by frame on the Media Engine with the right frame count, level, pitch and PCM: MPEG-1 44.1 kHz (with an ID3v2 tag) and 48 kHz, MPEG-2 22.05 kHz mono. Copy the `.mp3` files along with the EBOOT when running from the memory stick |
| `at3` | ATRAC3 at 132/105/66 kbps (block align 384/304/192, the last joint stereo) and ATRAC3plus, with and without Sony's fmt extension, decode frame by frame on the Media Engine with exactly data size / block align frames, the right level and pitch. `gen.sh` regenerates the files from a 440 Hz sine with [atracdenc](https://github.com/dcherednik/atracdenc). Copy the `.at3` files along with the EBOOT when running from the memory stick |
