#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <psputility.h>

#include "oslib.h"
#include "osk.h"

unsigned short *intext = NULL;
unsigned short *desc = NULL;
OSL_KEYBOARD *osl_osk = NULL;  // Pointer to the currently active OSL_KEYBOARD

OSL_KEYBOARD* oslInitOskEx(int nData, int language)
{
    OSL_KEYBOARD* kbd = (OSL_KEYBOARD*) malloc(sizeof(OSL_KEYBOARD));
    if (!kbd) return NULL;

    memset(kbd, 0, sizeof(OSL_KEYBOARD));
    kbd->oskParams.base.size = sizeof(SceUtilityOskParams);

    if (language >= 0) {
        kbd->oskParams.base.language = language;
    } else {
        sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &kbd->oskParams.base.language);
    }

    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &kbd->oskParams.base.buttonSwap);

    kbd->oskParams.base.graphicsThread = 17;
    kbd->oskParams.base.accessThread = 19;
    kbd->oskParams.base.fontThread = 18;
    kbd->oskParams.base.soundThread = 16;
    kbd->oskParams.datacount = oslMax(1, nData);
    kbd->oskParams.data = (SceUtilityOskData*) malloc(kbd->oskParams.datacount * sizeof(SceUtilityOskData));

    if (!kbd->oskParams.data) {
        free(kbd);
        return NULL;
    }

    memset(kbd->oskParams.data, 0, kbd->oskParams.datacount * sizeof(SceUtilityOskData));
    return kbd;
}

int oslInitOskDataEx(OSL_KEYBOARD *kbd, int idx, unsigned short *desc, unsigned short *intext, int textLimit, int linesNumber)
{
    if (!kbd || idx < 0 || idx >= kbd->oskParams.datacount) return 0;

    unsigned short *outtext = (unsigned short*) malloc((textLimit + 1) * sizeof(unsigned short));
    if (!outtext) return 0;

    SceUtilityOskData *oskData = &kbd->oskParams.data[idx];
    memset(oskData, 0, sizeof(SceUtilityOskData));

    oskData->language = PSP_UTILITY_OSK_LANGUAGE_DEFAULT;
    oskData->lines = linesNumber;
    oskData->unk_24 = 1;
    oskData->desc = desc;
    oskData->intext = intext;
    oskData->outtextlength = textLimit;
    oskData->outtextlimit = textLimit;
    oskData->outtext = outtext;

    return 1;
}

int oslActivateOskEx(OSL_KEYBOARD *kbd, int waitcycle)
{
    if (!kbd) return -1;

    if (!osl_osk) {  // No active OSK, activate the given one
        osl_osk = kbd;
        int ret = sceUtilityOskInitStart(&kbd->oskParams);
        if (ret < 0) osl_osk = NULL;
        return ret;
    }

    if (osl_osk == kbd) return 0;  // The given OSK is already active
    if (!waitcycle) return -2;  // Another OSK is currently active

    // Wait for the current OSK to finish, then retry activation
    while (osl_osk) sceDisplayWaitVblankStart();
    return oslActivateOskEx(kbd, waitcycle);
}

int oslOskIsActiveEx(OSL_KEYBOARD *kbd)
{
    return (kbd && (kbd == osl_osk));
}

void oslDeActivateOskEx(OSL_KEYBOARD *kbd)
{
    if (oslOskIsActiveEx(kbd)) {
        osl_osk = NULL;
    }
}

int oslOskGetResultEx(OSL_KEYBOARD *kbd, int idx)
{
    if (!kbd) return 0;
    if (idx < 0 || idx >= kbd->oskParams.datacount) {
        return kbd->oskParams.base.result;
    }
    return kbd->oskParams.data[idx].result;
}

unsigned short* oslOskOutTextEx(OSL_KEYBOARD *kbd, int idx)
{
    if (!kbd || idx < 0 || idx >= kbd->oskParams.datacount) {
        return NULL;
    }
    return kbd->oskParams.data[idx].outtext;
}

void oslEndOskEx(OSL_KEYBOARD *kbd)
{
    if (!kbd || !oslOskIsActiveEx(kbd)) return;

    oslDeActivateOskEx(kbd);
    for (int i = 0; i < kbd->oskParams.datacount; i++) {
        free(kbd->oskParams.data[i].outtext);
    }
    free(kbd->oskParams.data);
    free(kbd);
}

void oslInitOsk(char *descStr, char *initialStr, int textLimit, int linesNumber, int language)
{
    if (intext || desc) return;  // OSK already initialized

    intext = (unsigned short*) malloc((strlen(initialStr) + 1) * sizeof(unsigned short));
    desc = (unsigned short*) malloc((strlen(descStr) + 1) * sizeof(unsigned short));

    if (!intext || !desc) {
        oslEndOsk();
        return;
    }

    for (size_t i = 0; i <= strlen(initialStr); i++) {
        intext[i] = (unsigned short)initialStr[i];
    }

    for (size_t i = 0; i <= strlen(descStr); i++) {
        desc[i] = (unsigned short)descStr[i];
    }

    OSL_KEYBOARD *kbd = oslInitOskEx(1, language);
    if (!kbd || !oslInitOskDataEx(kbd, 0, desc, intext, textLimit, linesNumber) || (oslActivateOskEx(kbd, 0) < 0)) {
        oslEndOsk();
    }
}

void oslDrawOsk()
{
    switch (sceUtilityOskGetStatus()) {
        case PSP_UTILITY_DIALOG_VISIBLE:
            sceDisplayWaitVblankStart();
            sceDisplayWaitVblankStart();
            sceGuFinish();
            sceGuSync(0, 0);
            sceUtilityOskUpdate(2);
            sceGuStart(GU_DIRECT, osl_list);
            oslSetAlpha(OSL_FX_RGBA, 0xff);
            break;
        case PSP_UTILITY_DIALOG_QUIT:
            sceUtilityOskShutdownStart();
            break;
        default:
            break;
    }
}

int oslOskIsActive()
{
    return oslOskIsActiveEx(osl_osk);
}

int oslGetOskStatus()
{
    return sceUtilityOskGetStatus();
}

int oslOskGetResult()
{
    return oslOskGetResultEx(osl_osk, -1);
}

void oslOskGetText(char *text)
{
    int i, j = 0;
    if (osl_osk) {
        for (i = 0; osl_osk->oskParams.data[0].outtext[i]; i++) {
            if (osl_osk->oskParams.data[0].outtext[i] != '\n' && osl_osk->oskParams.data[0].outtext[i] != '\r') {
                text[j] = osl_osk->oskParams.data[0].outtext[i];
                j++;
            }
        }
    }
    text[j] = '\0';
}

void oslOskGetTextUCS2(unsigned short *text)
{
    int i, j = 0;
    if (osl_osk) {
        for (i = 0; osl_osk->oskParams.data[0].outtext[i]; i++) {
            if (osl_osk->oskParams.data[0].outtext[i] != '\n' && osl_osk->oskParams.data[0].outtext[i] != '\r') {
                text[j] = osl_osk->oskParams.data[0].outtext[i];
                j++;
            }
        }
    }
    text[j] = 0;
}

void oslEndOsk()
{
    free(intext);
    free(desc);
    intext = NULL;
    desc = NULL;
    oslEndOskEx(osl_osk);
}
