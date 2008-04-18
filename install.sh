#!/bin/sh
echo
echo "---------------"
echo "OSLib installer"
echo "---------------"
echo "Creating directories...."
mkdir -p $PSPSDK/include/oslib
mkdir -p $PSPSDK/include/oslib/intraFont

echo "Copying lib...."
cp -f  libosl.a $PSPSDK/lib/

echo "Copying header files...."
cp -f intraFont/intraFont.h $PSPSDK/include/oslib/intraFont/
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
cp -f vfpu.h $PSPSDK/include/oslib/
cp -f vfpu_ops.h $PSPSDK/include/oslib/
cp -f VirtualFile.h $PSPSDK/include/oslib/
cp -f vram_mgr.h $PSPSDK/include/oslib/

echo "Done!"
echo