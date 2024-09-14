#include <pspkernel.h>
#include "oslib.h"
#include "usb.h"

// Global variable to store module IDs
SceUID modules[8];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function to stop and unload a module
static void oslStopUnloadModule(SceUID modID) {
    if (modID >= 0) {
        int status = 0;
        sceKernelStopModule(modID, 0, NULL, &status, NULL);
        sceKernelUnloadModule(modID);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int oslInitUsbStorage() {
    u32 retVal = 0;

    // Load and start necessary kernel modules
    const char* modulePaths[] = {
        "flash0:/kd/chkreg.prx",
        "flash0:/kd/npdrm.prx",
        "flash0:/kd/semawm.prx",
        "flash0:/kd/usbstor.prx",
        "flash0:/kd/usbstormgr.prx",
        "flash0:/kd/usbstorms.prx",
        "flash0:/kd/usbstorboot.prx",
        "flash0:/kd/usbdevice.prx"
    };

    for (int i = 0; i < 8; i++) {
        modules[i] = pspSdkLoadStartModule(modulePaths[i], PSP_MEMORY_PARTITION_KERNEL);
        if (modules[i] < 0) {
            return -1; // Error loading module
        }
    }

    // Start USB bus driver
    retVal = sceUsbStart(PSP_USBBUS_DRIVERNAME, 0, 0);
    if (retVal != 0) {
        return -6; // Error starting USB bus driver
    }

    // Start USB storage driver
    retVal = sceUsbStart(PSP_USBSTOR_DRIVERNAME, 0, 0);
    if (retVal != 0) {
        return -7; // Error starting USB storage driver
    }

    // Set the USB storage capacity (e.g., 8 MB)
    retVal = sceUsbstorBootSetCapacity(0x800000);
    if (retVal != 0) {
        return -8; // Error setting USB storage capacity
    }

    return 0;
}

int oslStartUsbStorage() {
    return sceUsbActivate(0x1c8); // 0x1c8 is the USB PID for storage
}

int oslStopUsbStorage() {
    int retVal = sceUsbDeactivate(0x1c8); // Deactivate USB storage

    // Send IOCTL to prevent file corruption on the memory stick
    sceIoDevctl("fatms0:", 0x0240D81E, NULL, 0, NULL, 0);

    return retVal;
}

int oslDeinitUsbStorage() {
    // Stop USB storage if it is still active
    unsigned long state = oslGetUsbState();
    if (state & PSP_USB_ACTIVATED) {
        oslStopUsbStorage();
    }

    // Stop USB drivers
    sceUsbStop(PSP_USBSTOR_DRIVERNAME, 0, 0);
    sceUsbStop(PSP_USBBUS_DRIVERNAME, 0, 0);

    // Stop and unload all loaded modules in reverse order
    for (int i = 7; i >= 0; i--) {
        oslStopUnloadModule(modules[i]);
    }

    return 0;
}
