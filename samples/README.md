# OSLib Samples

Install OSLib before building the samples. Run the following commands from an
individual sample directory:

```bash
cmake -S . -B cbuild -DCMAKE_TOOLCHAIN_FILE=../../cmake/PSP.cmake
cmake --build cbuild -j
```

Or use the presets:

```bash
cmake --preset psp
cmake --build --preset psp -j
```

Outputs: `cbuild/EBOOT.PBP` and copied assets.

To build all samples, run the preset commands from `samples/`. Each sample's
executable, `PARAM.SFO`, `EBOOT.PBP` and assets go in `cbuild/<sample>/`.
CMake target and executable names match the sample directory, including case:
for example, `helloWorld`, `intraFont`, `OSK` and `rectAndFonts`. To build only one
sample from the combined build:

```bash
cmake --build cbuild --target rectAndFonts -j
```

Each sample uses the shared `cmake/OSLibSample.cmake` helper for linking,
packaging and asset copying. A new sample's `CMakeLists.txt` follows this template:

```cmake
cmake_minimum_required(VERSION 3.10)
project(mySample C)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/OSLibSample.cmake")
osl_add_sample(${PROJECT_NAME} "My Sample" main.c)
```

Pass sample-specific libraries after the source file, for example:

```cmake
osl_add_sample(${PROJECT_NAME} "Network Sample" main.c pspnet pspnet_apctl)
```

For a C++ sample, enable `CXX` in `project()` and pass `main.cpp` instead.
The title argument becomes the PSP menu title. Source files and CMake build trees
are excluded from copied assets; asset directories such as `Resources/` are kept.
