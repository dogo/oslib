#!/bin/sh
export TARGET="./Distrib"

echo
echo "-------------------"
echo "OSLib release maker"
echo "-------------------"
rm -rf $TARGET

echo "Creating directories...."
mkdir -p $TARGET
mkdir -p $TARGET/Install
mkdir -p $TARGET/Install/oslib
mkdir -p $TARGET/Install/oslib/intraFont

echo "Copying lib...."
cp -f  libosl.a $TARGET/Install

echo "Copying header files...."
cp -f intraFont/intraFont.h $TARGET/Install/oslib/intraFont/
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
cp -f vfpu.h $TARGET/Install/oslib/
cp -f vfpu_ops.h $TARGET/Install/oslib/
cp -f VirtualFile.h $TARGET/Install/oslib/
cp -f vram_mgr.h $TARGET/Install/oslib/

echo "Copying other files...."
cp -f README.TXT $TARGET
cp -f CHANGELOG.TXT $TARGET

echo "Copying DOC...."
cp -rf OSLib_MOD_Documentation $TARGET/Doc

echo "Copying Samples...."
cp -rf samples $TARGET/

echo "Done!"
echo