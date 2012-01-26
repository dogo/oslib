@echo off

set TARGET=..\Distrib

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
xcopy intraFont\intraFont.h %TARGET%\Install\oslib\intraFont\
xcopy intraFont\libccc.h %TARGET%\Install\oslib\intraFont\
xcopy libpspmath\pspmath.h %TARGET%\Install\oslib\libpspmath\
xcopy adhoc\pspadhoc.h %TARGET%\Install\oslib\adhoc\
xcopy oslmath.h %TARGET%\Install\oslib\
xcopy net.h %TARGET%\Install\oslib\
xcopy browser.h %TARGET%\Install\oslib\
xcopy audio.h %TARGET%\Install\oslib\
xcopy bgm.h %TARGET%\Install\oslib\
xcopy dialog.h %TARGET%\Install\oslib\
xcopy drawing.h %TARGET%\Install\oslib\
xcopy keys.h %TARGET%\Install\oslib\
xcopy map.h %TARGET%\Install\oslib\
xcopy messagebox.h %TARGET%\Install\oslib\
xcopy osk.h %TARGET%\Install\oslib\
xcopy saveload.h %TARGET%\Install\oslib\
xcopy oslib.h %TARGET%\Install\oslib\
xcopy text.h %TARGET%\Install\oslib\
xcopy usb.h %TARGET%\Install\oslib\
xcopy vfpu_ops.h %TARGET%\Install\oslib\
xcopy VirtualFile.h %TARGET%\Install\oslib\
xcopy vram_mgr.h %TARGET%\Install\oslib\
xcopy ccc.h %TARGET%\Install\oslib\
xcopy sfont.h %TARGET%\Install\oslib\

echo "xcopying other files...."
xcopy README.TXT %TARGET%
xcopy CHANGELOG.TXT %TARGET%
xcopy install.bat %TARGET%
xcopy install.sh %TARGET%

echo "xcopying DOC...."
xcopy OSLib_MOD_Documentation %TARGET%\Doc

echo "xcopying Samples...."
xcopy samples /e %TARGET%\Install\samples

echo "xcopying Tools...."
xcopy tools /e %TARGET%\Install\tools

echo "Done!"
pause