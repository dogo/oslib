#!/bin/sh
export TARGET="./Distrib"

echo
echo "-------------------"
echo "OSLib release maker"
echo "-------------------"
echo "Deleting target directory...."
rm -rf $TARGET

echo "Creating target directories...."
mkdir -p $TARGET
mkdir -p $TARGET/Install
mkdir -p $TARGET/Install/oslib
mkdir -p $TARGET/Install/oslib/intraFont
mkdir -p $TARGET/Install/oslib/libpspmath
mkdir -p $TARGET/Install/oslib/adhoc

echo "Copying lib...."
cp -f  libosl.a $TARGET/Install

echo "Copying header files...."
cp -f intraFont/intraFont.h $TARGET/Install/oslib/intraFont/
cp -f intraFont/libccc.h $TARGET/Install/oslib/intraFont/
cp -f libpspmath/pspmath.h $TARGET/Install/oslib/libpspmath/
cp -f adhoc/pspadhoc.h $TARGET/Install/oslib/adhoc/
cp -f oslmath.h $TARGET/Install/oslib/
cp -f net.h $TARGET/Install/oslib/
cp -f browser.h $TARGET/Install/oslib/
cp -f audio.h $TARGET/Install/oslib/
cp -f bgm.h $TARGET/Install/oslib/
cp -f dialog.h $TARGET/Install/oslib/
cp -f drawing.h $TARGET/Install/oslib/
cp -f keys.h $TARGET/Install/oslib/
cp -f map.h $TARGET/Install/oslib/
cp -f messagebox.h $TARGET/Install/oslib/
cp -f osk.h $TARGET/Install/oslib/
cp -f saveload.h $TARGET/Install/oslib/
cp -f oslib.h $TARGET/Install/oslib/
cp -f text.h $TARGET/Install/oslib/
cp -f usb.h $TARGET/Install/oslib/
cp -f vfpu_ops.h $TARGET/Install/oslib/
cp -f VirtualFile.h $TARGET/Install/oslib/
cp -f vram_mgr.h $TARGET/Install/oslib/
cp -f ccc.h $TARGET/Install/oslib/
cp -f sfont.h $TARGET/Install/oslib/

echo "Copying other files...."
cp -f README.TXT $TARGET
cp -f CHANGELOG.TXT $TARGET

echo "Copying DOC...."
cp -rf OSLib_MOD_Documentation $TARGET/Doc

echo "Copying Samples...."
cp -rf samples $TARGET/

echo "Done!"
echo