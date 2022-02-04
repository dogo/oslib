## Version 1.1.2
Updated: libpng to 1.5.  
Updated: Doxyfile to 1.8.6  
Fixed: Doxygen warnings.  

## Version 1.1.1a
Added: some Strangelove fixes.  
Updated: zlib 1.2.5   
Fixed: oslLoadImageFilePNG.c  

## Version 1.1.1
Added: net functions (from PGE LUA)  
Added: oslDrawStringLimited (limit a string blit to a fixed width)  
Fixed: loading an intraFont using Virtual fileSystem  
Fixed: bugs by STAS (many thanks)  

## Version 1.1.0
Added: VFPU Math Library libpspmath version 4 by MrMr[iCE]  
Added: Remote control functions  
Added: Sony Browser (see the sample)  
Added: oslDrawStringf [oslDrawStringf(0, 0, "Test string %i", 1);]  
Added: adhoc functions (based on pspZorba's adhoc sample)  
Added: switched to libjpeg to load jpg  
Added: Switched to intraFont 0.31  
Added: Functions to delete a savedata or save without multi list (thanks to valantin)  
Fixed: debug console  
Fixed: minor bugs  

## Version 1.0.1
Added: Support for UTF-8 with intraFont (many thanks to nextos!!!)
Fixed: corrupted graphic using intraFont  
Fixed: MP3/ATRAC3+ playback under kernel 3.xx (sorry, I didn't test it before)  
Fixed: crash in MP3 playback after some seconds (many thanks to Drakon for reporting and testing)  
Fixed: message boxes work now.  

## Version 1.0.0
Added: oslSetReadKeysFunction  
Added: All USB.c functions now works  
Added: OSL_VERSION  
Added: oslSetHoldForAnalog  
Added: support for intraFont.  
Added: Dialogs (message, error and net conf)  
Added: On Screen Keyboard  
Added: Save and load  
Fixed: Tagged MP3 and ATRAC3+ now works  
Fixed: osl_keys->analogToDPadSensivity now works correctly  
