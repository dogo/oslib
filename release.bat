@echo off

set TARGET=..\Distrib
set SOURCE_DIR=.\src

echo
echo "-------------------"
echo "OSLib release maker"
echo "-------------------"
echo "Deleting target directory...."
rmdir /s /q %TARGET%

echo "Creating target directories...."
mkdir %TARGET%
mkdir %TARGET%\Install
mkdir %TARGET%\Install\oslib
mkdir %TARGET%\Install\oslib\intraFont
mkdir %TARGET%\Install\oslib\libpspmath
mkdir %TARGET%\Install\oslib\adhoc
mkdir %TARGET%\Install\tools
mkdir %TARGET%\Install\samples

echo "xcopying lib...."
xcopy libosl.a %TARGET%\Install

echo "xcopying header files...."
xcopy %SOURCE_DIR%\intraFont\intraFont.h %TARGET%\Install\oslib\intraFont\
xcopy %SOURCE_DIR%\intraFont\libccc.h %TARGET%\Install\oslib\intraFont\
xcopy %SOURCE_DIR%\libpspmath\pspmath.h %TARGET%\Install\oslib\libpspmath\
xcopy %SOURCE_DIR%\adhoc\pspadhoc.h %TARGET%\Install\oslib\adhoc\
xcopy %SOURCE_DIR%\oslmath.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\net.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\browser.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\audio.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\bgm.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\dialog.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\drawing.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\keys.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\map.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\messagebox.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\osk.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\saveload.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\oslib.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\ext.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\usb.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\vfpu_ops.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\VirtualFile.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\vram_mgr.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\ccc.h %TARGET%\Install\oslib\
xcopy %SOURCE_DIR%\sfont.h %TARGET%\Install\oslib\

echo "xcopying other files...."
xcopy README.md %TARGET%
xcopy CHANGELOG.TXT %TARGET%
xcopy install.bat %TARGET%
xcopy install.sh %TARGET%

echo "xcopying DOC...."
xcopy OSLib_MODv2_Documentation %TARGET%\Doc

echo "xcopying Samples...."
xcopy samples /e %TARGET%\Install\samples

echo "xcopying Tools...."
xcopy tools /e %TARGET%\Install\tools

echo "Done!"
pause