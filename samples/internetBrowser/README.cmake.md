Build this sample with CMake (standalone):

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/PSP.cmake
cmake --build build -j
```

Outputs: `build/EBOOT.PBP` and copied assets.
