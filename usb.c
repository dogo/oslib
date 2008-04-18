#include <pspkernel.h>
#include <kubridge.h>
#include "oslib.h"
#include "usb.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SceUID modules[7];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int oslLoadStartModule(char *path)
{
    u32 loadResult;
    u32 startResult;
    int status;

    loadResult = kuKernelLoadModule(path, 0, NULL);
    if (loadResult & 0x80000000)
       return -1;

    startResult = sceKernelStartModule(loadResult, 0, NULL, &status, NULL);
    if (loadResult != startResult)
       return -2;
    return loadResult;
}

int oslStopUnloadModule(SceUID modID){
    int status;
    sceKernelStopModule(modID, 0, NULL, &status, NULL);
    sceKernelUnloadModule(modID);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int oslInitUsbStorage()		{
	u32 retVal;

    //start necessary drivers
    modules[0] = oslLoadStartModule("flash0:/kd/chkreg.prx");
    if (modules[0] < 0)
        return -1;
    modules[1] = oslLoadStartModule("flash0:/kd/npdrm.prx");
    if (modules[1] < 0)
        return -1;
    modules[2] = oslLoadStartModule("flash0:/kd/semawm.prx");
    if (modules[2] < 0)
        return -1;
    modules[3] = oslLoadStartModule("flash0:/kd/usbstor.prx");
    if (modules[3] < 0)
        return -1;
    modules[4] = oslLoadStartModule("flash0:/kd/usbstormgr.prx");
    if (modules[4] < 0)
        return -1;
    modules[5] = oslLoadStartModule("flash0:/kd/usbstorms.prx");
    if (modules[5] < 0)
        return -1;
    modules[6] = oslLoadStartModule("flash0:/kd/usbstorboot.prx");
    if (modules[6] < 0)
        return -1;

    //setup USB drivers
    retVal = sceUsbStart(PSP_USBBUS_DRIVERNAME, 0, 0);
    if (retVal != 0)
		return -6;

    retVal = sceUsbStart(PSP_USBSTOR_DRIVERNAME, 0, 0);
    if (retVal != 0)
		return -7;

    retVal = sceUsbstorBootSetCapacity(0x800000);
    if (retVal != 0)
		return -8;
    return 0;
}

void oslStartUsbStorage()		{
    sceUsbActivate(0x1c8);
}

void oslStopUsbStorage()		{
    sceUsbDeactivate(0x1c8);
    sceIoDevctl("fatms0:", 0x0240D81E, NULL, 0, NULL, 0 ); //Avoid corrupted files
}

int oslDeinitUsbStorage()			{
    int i;
	unsigned long state = oslGetUsbState();
    if (state & PSP_USB_ACTIVATED)
        oslStopUsbStorage();
    sceUsbStop(PSP_USBSTOR_DRIVERNAME, 0, 0);
    sceUsbStop(PSP_USBBUS_DRIVERNAME, 0, 0);
    for (i=6; i>=0; i--)
        oslStopUnloadModule(modules[i]);
    return 0;
}
