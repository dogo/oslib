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
mkdir -p $PSPSDK/include/oslib
mkdir -p $PSPSDK/include/oslib/intraFont
mkdir -p $PSPSDK/include/oslib/libpspmath
mkdir -p $PSPSDK/include/oslib/adhoc

echo "Copying lib...."
cp -f  libosl.a $PSPSDK/lib/

echo "Copying header files...."
cp -f intraFont/intraFont.h $PSPSDK/include/oslib/intraFont/
cp -f intraFont/libccc.h $PSPSDK/include/oslib/intraFont/
cp -f libpspmath/pspmath.h $PSPSDK/include/oslib/libpspmath/
cp -f adhoc/pspadhoc.h $PSPSDK/include/oslib/adhoc/
cp -f oslmath.h $PSPSDK/include/oslib/
cp -f net.h $PSPSDK/include/oslib/
cp -f browser.h $PSPSDK/include/oslib/
cp -f audio.h $PSPSDK/include/oslib/
cp -f bgm.h $PSPSDK/include/oslib/
cp -f dialog.h $PSPSDK/include/oslib/
cp -f drawing.h $PSPSDK/include/oslib/
cp -f keys.h $PSPSDK/include/oslib/
cp -f map.h $PSPSDK/include/oslib/
cp -f messagebox.h $PSPSDK/include/oslib/
cp -f osk.h $PSPSDK/include/oslib/
cp -f saveload.h $PSPSDK/include/oslib/
cp -f oslib.h $PSPSDK/include/oslib/
cp -f text.h $PSPSDK/include/oslib/
cp -f usb.h $PSPSDK/include/oslib/
cp -f vfpu_ops.h $PSPSDK/include/oslib/
cp -f VirtualFile.h $PSPSDK/include/oslib/
cp -f vram_mgr.h $PSPSDK/include/oslib/
cp -f ccc.h $PSPSDK/include/oslib/
cp -f sfont.h $PSPSDK/include/oslib/

echo "Done!"
echo
