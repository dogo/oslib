@echo off

rem This defines the installation directory, please change it:
set PSPSDKDIR=C:\pspsdk\psp\sdk

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
xcopy Install\libosl.a %PSPSDKDIR%\lib\
IF ERRORLEVEL 1 GOTO ERROR

echo "Copying header files...."
xcopy Install\oslib\intraFont\intraFont.h %PSPSDKDIR%\include\oslib\intraFont\
xcopy Install\oslib\intraFont\libccc.h %PSPSDKDIR%\include\oslib\intraFont\
xcopy Install\oslib\libpspmath\pspmath.h %PSPSDKDIR%\include\oslib\libpspmath\
xcopy Install\oslib\adhoc\pspadhoc.h %PSPSDKDIR%\include\oslib\adhoc\
xcopy Install\oslib\oslmath.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\net.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\browser.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\audio.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\bgm.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\dialog.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\drawing.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\keys.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\map.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\messagebox.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\osk.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\saveload.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\oslib.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\text.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\usb.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\vfpu_ops.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\VirtualFile.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\vram_mgr.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\ccc.h %PSPSDKDIR%\include\oslib\
xcopy Install\oslib\sfont.h %PSPSDKDIR%\include\oslib\

IF ERRORLEVEL 1 GOTO ERROR
echo Installation completed successfully.
pause
exit

:ERROR
color c
echo Installation failed. Please verify the installation path!
pause