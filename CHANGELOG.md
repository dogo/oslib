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
