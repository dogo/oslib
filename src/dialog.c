#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <psputility.h>

#include "oslib.h"
#include "dialog.h"

pspUtilityMsgDialogParams dialog;
pspUtilityNetconfData netConf;
int dialogType = OSL_DIALOG_NONE;

void ConfigureDialog(pspUtilityMsgDialogParams *inDialog, size_t dialog_size) {
    memset(inDialog, 0, dialog_size);

    inDialog->base.size = dialog_size;
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &inDialog->base.language); // Prompt language
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &inDialog->base.buttonSwap); // X/O button swap

    inDialog->base.graphicsThread = 0x11;
    inDialog->base.accessThread = 0x13;
    inDialog->base.fontThread = 0x12;
    inDialog->base.soundThread = 0x10;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Public API
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int oslInitMessageDialog(const char *message, int enableYesNo) {
    ConfigureDialog(&dialog, sizeof(dialog));
    dialog.mode = PSP_UTILITY_MSGDIALOG_MODE_TEXT;
    dialog.options = PSP_UTILITY_MSGDIALOG_OPTION_TEXT;

    if (enableYesNo) {
        dialog.options |= PSP_UTILITY_MSGDIALOG_OPTION_YESNO_BUTTONS | PSP_UTILITY_MSGDIALOG_OPTION_DEFAULT_NO;
    }

    strncpy(dialog.message, message, sizeof(dialog.message) - 1);
    int res = sceUtilityMsgDialogInitStart(&dialog);

    if (res == 0) {
        dialogType = OSL_DIALOG_MESSAGE;
    }
    return res;
}

int oslInitErrorDialog(const unsigned int error) {
    ConfigureDialog(&dialog, sizeof(dialog));
    dialog.mode = PSP_UTILITY_MSGDIALOG_MODE_ERROR;
    dialog.options = PSP_UTILITY_MSGDIALOG_OPTION_ERROR;
    dialog.errorValue = error;

    int res = sceUtilityMsgDialogInitStart(&dialog);

    if (res == 0) {
        dialogType = OSL_DIALOG_ERROR;
    }
    return res;
}

int oslInitNetDialog() {
    memset(&netConf, 0, sizeof(netConf));
    netConf.base.size = sizeof(netConf);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &netConf.base.language);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &netConf.base.buttonSwap);

    netConf.base.graphicsThread = 17;
    netConf.base.accessThread = 19;
    netConf.base.fontThread = 18;
    netConf.base.soundThread = 16;
    netConf.action = PSP_NETCONF_ACTION_CONNECTAP;

    struct pspUtilityNetconfAdhoc adhocparam;
    memset(&adhocparam, 0, sizeof(adhocparam));
    netConf.adhocparam = &adhocparam;

    int res = sceUtilityNetconfInitStart(&netConf);

    if (res == 0) {
        dialogType = OSL_DIALOG_NETCONF;
    }
    return res;
}

int oslDialogGetResult() {
    switch (dialogType) {
        case OSL_DIALOG_MESSAGE:
        case OSL_DIALOG_ERROR:
            return dialog.base.result;
        case OSL_DIALOG_NETCONF:
            return netConf.base.result;
        default:
            return OSL_DIALOG_CANCEL;
    }
}

void oslDrawDialog() {
    int status;

    if (dialogType == OSL_DIALOG_MESSAGE || dialogType == OSL_DIALOG_ERROR) {
        status = sceUtilityMsgDialogGetStatus();
    } else if (dialogType == OSL_DIALOG_NETCONF) {
        status = sceUtilityNetconfGetStatus();
    }

    switch (status) {
        case PSP_UTILITY_DIALOG_INIT:
        case PSP_UTILITY_DIALOG_VISIBLE:
            sceGuFinish();
            sceGuSync(0, 0);
            if (dialogType == OSL_DIALOG_MESSAGE || dialogType == OSL_DIALOG_ERROR) {
                sceUtilityMsgDialogUpdate(1);
            } else if (dialogType == OSL_DIALOG_NETCONF) {
                sceUtilityNetconfUpdate(1);
            }
            sceGuStart(GU_DIRECT, osl_list);
            oslSetAlpha(OSL_FX_RGBA, 0xff);
            break;
        case PSP_UTILITY_DIALOG_QUIT:
            if (dialogType == OSL_DIALOG_MESSAGE || dialogType == OSL_DIALOG_ERROR) {
                sceUtilityMsgDialogShutdownStart();
            } else if (dialogType == OSL_DIALOG_NETCONF) {
                sceUtilityNetconfShutdownStart();
            }
            break;
        case PSP_UTILITY_DIALOG_NONE:
        case PSP_UTILITY_DIALOG_FINISHED:
            break;
    }
}

int oslGetDialogType() {
    return dialogType;
}

int oslGetDialogStatus() {
    if (dialogType == OSL_DIALOG_MESSAGE || dialogType == OSL_DIALOG_ERROR) {
        return sceUtilityMsgDialogGetStatus();
    } else if (dialogType == OSL_DIALOG_NETCONF) {
        return sceUtilityNetconfGetStatus();
    }
    return PSP_UTILITY_DIALOG_NONE;
}

int oslGetDialogButtonPressed() {
    return dialog.buttonPressed;
}

void oslEndDialog() {
    dialogType = OSL_DIALOG_NONE;
}

int oslDialogDrawAndWait(int dialogType) {
    int status = OSL_DIALOG_STATUS_INIT;
    OSL_IMAGE *img = oslCreateImage(480, 272, OSL_IN_RAM, OSL_PF_8888);

    oslSyncDrawing();
    oslCopyImageTo(img, OSL_DEFAULT_BUFFER);  // Save the current image

    while (status >= 0 && status != OSL_DIALOG_STATUS_NONE && !osl_quit) {
        if (!oslSyncFrameEx(0, 0, 1)) {
            oslStartDrawing();
            oslCopyImageTo(OSL_DEFAULT_BUFFER, img);  // Restore the image

            switch (dialogType) {
                case OSL_DIALOG_MESSAGE:
                case OSL_DIALOG_ERROR:
                case OSL_DIALOG_NETCONF:
                    oslDrawDialog();
                    status = oslGetDialogStatus();
                    break;
                case OSL_DIALOG_OSK:
                    oslDrawOsk();
                    status = oslGetOskStatus();
                    break;
                case OSL_DIALOG_SAVELOAD:
                    oslDrawSaveLoad();
                    status = oslGetLoadSaveStatus();
                    break;
                case OSL_DIALOG_BROWSER:
                    oslDrawBrowser();
                    status = oslGetBrowserStatus();
                    break;
            }

            oslEndDrawing();
        }
        oslEndFrame();
    }

    oslDeleteImage(img);
    return (status < 0) ? status : 0;
}

int oslDialogIsActive() {
    return (dialogType != OSL_DIALOG_NONE);
}
