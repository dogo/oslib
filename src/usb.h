#ifndef USB_H
#define USB_H

#include <pspusb.h>
#include <pspusbstor.h>
#include <pspsdk.h>

/** @file usb.h
 *  @brief USB Module Header File
 *
 *  This file contains functions and definitions related to USB operations,
 *  including initializing, starting, stopping, and deinitializing USB storage.
 */

/** @brief Gets the current USB state.
 *
 *  This macro retrieves the current state of the USB connection.
 *  It maps directly to `sceUsbGetState`.
 */
#define oslGetUsbState sceUsbGetState

/** @defgroup Usb USB
 *  @brief USB functions to start/stop USB storage.
 *  @{
 */

/** @brief USB State Enum.
 *
 *  Enum containing the possible states of the USB connection.
 */
enum {
	OSL_USB_ACTIVATED = PSP_USB_ACTIVATED, /**< USB is activated */
	OSL_USB_CABLE_CONNECTED = PSP_USB_CABLE_CONNECTED, /**< USB cable is connected */
	OSL_USB_CONNECTION_ESTABLISHED = PSP_USB_CONNECTION_ESTABLISHED /**< USB connection is established */
};

/** @brief Initializes the USB storage modules.
 *
 *  This function initializes the necessary USB modules. It must be called
 *  before any other USB storage operations.
 *
 *  @return Returns 0 on success, a negative value on failure.
 */
extern int oslInitUsbStorage();

/** @brief Starts USB storage.
 *
 *  This function starts the USB storage, making the PSP appear as a USB storage
 *  device when connected to a host.
 *
 *  @return Returns 0 on success, a negative value on failure.
 */
extern int oslStartUsbStorage();

/** @brief Stops USB storage.
 *
 *  This function stops the USB storage, safely disconnecting the PSP from the
 *  host system.
 *
 *  @return Returns 0 on success, a negative value on failure.
 */
extern int oslStopUsbStorage();

/** @brief Deinitializes the USB storage modules.
 *
 *  This function deinitializes the USB modules, releasing any resources that
 *  were allocated during initialization.
 */
extern int oslDeinitUsbStorage();

/** @} */ // end of Usb

#endif
