# OldSchool Library (OSLib)
![Build Status](https://github.com/dogo/oslib/actions/workflows/Build.yml/badge.svg?event=push)

## Overview

OldSchool Library (OSLib) is a lightweight and versatile C++ library designed to provide functionality and tools commonly used in classic game development and retro programming projects. It bridges the gap between modern and old-school development environments, making it easier to create and manage applications with a nostalgic touch.

## Features

- **Graphics and Rendering:** Basic graphics functions and rendering capabilities for 2D game development.
- **Input Handling:** Manage keyboard, mouse, and gamepad inputs for interactive applications.
- **Sound Management:** Simple sound and music playback functionalities.
- **Utility Functions:** A collection of utility functions for handling files, strings, and more.
- **Cross-Platform Support:** Compatible with multiple platforms, including Windows, macOS, and Linux.

## Building

### Library

**Makefile:**
```bash
make
make install
```

**CMake:**
```bash
cmake --preset psp
cmake --build --preset psp -j
cmake --install build
```

### Samples

Each sample can be built standalone from its own directory using the same commands above.

To build all samples at once from the `samples/` directory:

**Makefile:**
```bash
cd samples && make
```

**CMake:**
```bash
cd samples
cmake --preset psp
cmake --build --preset psp -j
```

### Editor / LSP support (compile_commands.json)

The CMake build exports `compile_commands.json` automatically via `CMAKE_EXPORT_COMPILE_COMMANDS=ON`. This file is used by clangd (and other LSP clients) for accurate code completion, diagnostics, and navigation.

After running `cmake --preset psp` at the root, a symlink `compile_commands.json → build/compile_commands.json` is already in place. clangd will pick it up automatically.

> `compile_commands.json` is in `.gitignore` — run the cmake configure step once after cloning to generate it.

## Installation

To install OSLib:
1. Copy `libosl.a` to `$PSPSDK/lib/`.
2. Copy the `oslib` directory (containing header files) to `$PSPSDK/include/`.

## Documentation

You can find the documentation in the `doc` directory, or consult it online here:  
[http://dogo.github.io/oslib/](http://dogo.github.io/oslib/)

## Contributing

We welcome contributions to OSLib! If you’d like to contribute, please follow these steps:

1. Fork the repository.
2. Create a feature branch.
3. Make your changes and commit them.
4. Push your branch to your fork.
5. Create a pull request describing your changes.

Please follow the coding guidelines and write tests for any new features or bug fixes.

## License

OSLib is free software, licensed under the GPLv2. See `COPYING` for details.

## Thanks

- **Brunni** for the original OSLib
- **Sakya** for the first modification of OSLib
- **BenHur** for intraFont
