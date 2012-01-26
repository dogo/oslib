@echo off

rem This defines the installation directory, please change it:
set PSPSDKDIR=C:\PSPSDKDIR\psp\sdk

echo Files will be copied to the following directory:
echo %PSPSDKDIR%
echo Please verify that it's correct, or edit this batch file.
pause

echo
echo "---------------"
echo "OSLib installer"
echo "---------------"
echo "Creating directories...."
mkdir %PSPSDKDIR%\include\oslib
mkdir %PSPSDKDIR%\include\oslib\intraFont
mkdir %PSPSDKDIR%\include\oslib\libpspmath
mkdir %PSPSDKDIR%\include\oslib\adhoc

echo "Copying lib...."
xcopy libosl.a %PSPSDKDIR%\lib\
IF ERRORLEVEL 1 GOTO ERROR

echo "Copying header files...."
xcopy intraFont\intraFont.h %PSPSDKDIR%\include\oslib\intraFont\
xcopy intraFont\libccc.h %PSPSDKDIR%\include\oslib\intraFont\
xcopy libpspmath\pspmath.h %PSPSDKDIR%\include\oslib\libpspmath\
xcopy adhoc\pspadhoc.h %PSPSDKDIR%\include\oslib\adhoc\
xcopy oslmath.h %PSPSDKDIR%\include\oslib\
xcopy net.h %PSPSDKDIR%\include\oslib\
xcopy browser.h %PSPSDKDIR%\include\oslib\
xcopy audio.h %PSPSDKDIR%\include\oslib\
xcopy bgm.h %PSPSDKDIR%\include\oslib\
xcopy dialog.h %PSPSDKDIR%\include\oslib\
xcopy drawing.h %PSPSDKDIR%\include\oslib\
xcopy keys.h %PSPSDKDIR%\include\oslib\
xcopy map.h %PSPSDKDIR%\include\oslib\
xcopy messagebox.h %PSPSDKDIR%\include\oslib\
xcopy osk.h %PSPSDKDIR%\include\oslib\
xcopy saveload.h %PSPSDKDIR%\include\oslib\
xcopy oslib.h %PSPSDKDIR%\include\oslib\
xcopy text.h %PSPSDKDIR%\include\oslib\
xcopy usb.h %PSPSDKDIR%\include\oslib\
xcopy vfpu_ops.h %PSPSDKDIR%\include\oslib\
xcopy VirtualFile.h %PSPSDKDIR%\include\oslib\
xcopy vram_mgr.h %PSPSDKDIR%\include\oslib\
xcopy ccc.h %PSPSDKDIR%\include\oslib\
xcopy sfont.h %PSPSDKDIR%\include\oslib\

IF ERRORLEVEL 1 GOTO ERROR
echo Installation completed successfully.
pause
exit

:ERROR
color c
echo Installation failed. Please verify the installation path!
pause