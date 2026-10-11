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

Each test builds an `EBOOT.PBP` against the installed OSLib, runs once without
UI, writes `ms0:/PSP/GAME/<name>/result.txt` with a `PASS`/`FAIL` line per case
and exits.

```sh
make install                      # the test links the installed libosl.a
make -C tests/device/texcache
cp tests/device/texcache/EBOOT.PBP /path/to/memstick/PSP/GAME/texcache/
# run it on the PSP, then read PSP/GAME/texcache/result.txt
```

CI only checks that they build.

| Test | Covers |
|---|---|
| `texcache` | Re-drawing an image after render-to-texture or a CPU write samples the new contents, not stale texture-cache lines |
