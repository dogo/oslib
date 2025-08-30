#!/bin/sh

PSPSDK=$(psp-config --pspsdk-path)
if [ -z "$PSPSDK" ]; then
    echo
    echo "ERROR: Env variable PSPSDK not set."
    echo
    exit 1
fi

echo
echo "---------------"
echo "OSLib installer"
echo "---------------"
echo "Creating directories...."
mkdir -p "$PSPSDK"/include/oslib
mkdir -p "$PSPSDK"/include/oslib/intraFont
mkdir -p "$PSPSDK"/include/oslib/libpspmath
mkdir -p "$PSPSDK"/include/oslib/adhoc

echo "Copying lib...."
cp -f Install/libosl.a "$PSPSDK"/lib/

echo "Copying header files...."
cp -f Install/oslib/intraFont/intraFont.h "$PSPSDK"/include/oslib/intraFont/
cp -f Install/oslib/intraFont/libccc.h "$PSPSDK"/include/oslib/intraFont/
cp -f Install/oslib/libpspmath/pspmath.h "$PSPSDK"/include/oslib/libpspmath/
cp -f Install/oslib/adhoc/pspadhoc.h "$PSPSDK"/include/oslib/adhoc/
cp -f Install/oslib/oslmath.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/net.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/browser.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/audio.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/bgm.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/dialog.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/drawing.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/keys.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/map.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/messagebox.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/osk.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/saveload.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/oslib.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/text.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/usb.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/vfpu_ops.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/VirtualFile.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/vram_mgr.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/ccc.h "$PSPSDK"/include/oslib/
cp -f Install/oslib/sfont.h "$PSPSDK"/include/oslib/

echo "Done!"
echo
