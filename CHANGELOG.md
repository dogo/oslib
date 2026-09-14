## Version 1.5.1

Added: MP3 support in the audio sample<br>
Added: CMake and Makefile presets for sample projects<br>
Added: Build instructions and LSP support details in README<br>
Fixed: GIF line buffer overflow and crash on load failure<br>
Fixed: PNG palette buffer overflow<br>
Fixed: VRAM allocation bugs and alignment<br>
Fixed: Memory VFS EOF check and virtual file list removal<br>
Fixed: ATRAC header validation before reading its fields<br>
Fixed: Signed overflow in RGB and RGBA macros<br>
Fixed: Handling of larger BGM buffers<br>
Fixed: Savedata key copy warning<br>
Fixed: CMake runtime output directory and target name consistency<br>

## Version 1.5.0

Added: Audio BGM sample and jump sound effect in the audio sample<br>
Added: Auto-generated osl_config.h; OSL_IMAGE_* flags no longer needed in samples<br>
Added: Mode field to VIRTUAL_FILE struct for open mode tracking<br>
Added: CMake sample builds in CI<br>
Fixed: WAV streaming length calculation for mono and stereo<br>
Fixed: BGM streaming playback<br>
Fixed: Prefer PSP SDK for image/zlib libraries, falling back to bundled loaders when missing<br>
Fixed: PSP toolchain now loaded before the project() declaration<br>
Fixed: memset usage and connection message formatting in adhoc<br>
Fixed: Volatile qualifiers to prevent longjmp clobber warnings in PNG<br>
Fixed: Sign-compare and missing prototype warnings<br>
Fixed: Doxyfile warnings<br>
Updated: giflib, libintraFont and libpspmath are now submodules<br>
Updated: macOS CI image to 15 (was 13) and more systems in the build matrix<br>
Updated: Third-party library warnings suppressed in the build system<br>
Updated: Uncrustify configuration and code formatting<br>

## Version 1.4.0

Added: Automated releases and CI/CD pipeline improvements<br>
Added: Static analysis tools (cppcheck and uncrustify)<br>
Added: CMake modern configuration with improved installation paths<br>
Added: DrawMap sample<br>
Fixed: Memory safety improvements and buffer overflow prevention<br>
Fixed: Enhanced audio system with better codec handling and ADPCM decoding<br>
Fixed: Refactored Save/Load dialog, OSK, VirtualFile, and input handling<br>
Fixed: Improved shape drawing, message box, and UI rendering<br>
Fixed: Better documentation and code structure across multiple modules<br>
Fixed: PNG loading and SFont handling with improved memory management<br>
Updated: Uncrustify code formatting<br>
Updated: Enhanced shapes sample with C++ support and fixed texturing<br>

## Version 1.3.0

Added: Comprehensive documentation for all modules (Audio, Browser, Dialog, Keys, Map, Net, OSK, Text, SFont, SaveLoad, VFPU, USB, VirtualFile, vram_mgr, and more)<br>
Added: Doxygen documentation generation in CI/CD pipeline<br>
Updated: Doxygen to 1.12.0 (from 1.8.7)<br>
Updated: CODE_OF_CONDUCT.md and issue templates<br>
Fixed: Refactored and improved image manipulation functions (oslDrawImage, oslDrawImageBig, oslMoveImageTo, oslScaleImageCreate, oslSetImagePixel, oslResetImageProperties)<br>
Fixed: Enhanced swizzle/unswizzle functions for better performance<br>
Fixed: Improved oslGetImagePixel and oslSetDrawBuffer functions<br>
Fixed: Better image conversion and palette search functions<br>
Fixed: Refactored oslLoadImageFile and oslWriteImageFile for better safety<br>
Fixed: Improved oslLoadImageFileJPG and GIF loading code<br>
Fixed: Enhanced drawing functions with better documentation and code formatting<br>
Fixed: Removed old references to modv2 and dead files<br>

## Version 1.2.0

Updated: intraFont to latest version with bug fixes, performance improvements, and new features<br>
Fixed: Improved oslLoadImageFilePNG code formatting, removed unused code, simplified error handling, and optimized palette handling<br>
Fixed: Enhanced text functions with better font type support, improved rendering, optimized resource management, detailed comments and improved code readability<br>

## Version 1.1.3

Updated: libpng to 1.6<br>
Fixed: Make oslib build with newer PSP toolchain<br>
Fixed: Multiple definitions of...'<br>

## Version 1.1.2

Updated: libpng to 1.5<br>
Updated: Doxyfile to 1.8.6<br>
Fixed: Doxygen warnings<br>

## Version 1.1.1a

Added: Some Strangelove fixes<br>
Updated: zlib 1.2.5<br>
Fixed: oslLoadImageFilePNG.c<br>

## Version 1.1.1

Added: Net functions (from PGE LUA)<br>
Added: oslDrawStringLimited (limit a string blit to a fixed width)<br>
Fixed: Loading an intraFont using Virtual fileSystem<br>
Fixed: Bugs by STAS (many thanks)<br>

## Version 1.1.0

Added: VFPU Math Library libpspmath version 4 by MrMr[iCE]<br>
Added: Remote control functions<br>
Added: Sony Browser (see the sample)<br>
Added: oslDrawStringf [oslDrawStringf(0, 0, "Test string %i", 1);]<br>
Added: Adhoc functions (based on pspZorba's adhoc sample)<br>
Added: Switched to libjpeg to load jpg<br>
Added: Switched to intraFont 0.31<br>
Added: Functions to delete a savedata or save without multi list (thanks to valantin)<br>
Fixed: Debug console<br>
Fixed: Minor bugs<br>

## Version 1.0.1

Added: Support for UTF-8 with intraFont (many thanks to nextos!!!)<br>
Fixed: Corrupted graphic using intraFont<br>
Fixed: MP3/ATRAC3+ playback under kernel 3.xx (sorry, I didn't test it before)<br>
Fixed: Crash in MP3 playback after some seconds (many thanks to Drakon for reporting and testing)<br>
Fixed: Message boxes work now<br>

## Version 1.0.0

Added: oslSetReadKeysFunction<br>
Added: All USB.c functions now works<br>
Added: OSL_VERSION<br>
Added: oslSetHoldForAnalog<br>
Added: Support for intraFont.<br>
Added: Dialogs (message, error and net conf)<br>
Added: On Screen Keyboard<br>
Added: Save and load<br>
Fixed: Tagged MP3 and ATRAC3+ now works<br>
Fixed: osl_keys->analogToDPadSensivity now works correctly<br>
