#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <psputility.h>

#include "oslib.h"
#include "osk.h"

SceUtilityOskData OskData;
SceUtilityOskParams oskParams;
unsigned short *intext;
unsigned short *outtext;
unsigned short desc[128]  = { 0 };

int OskActive = 0;

void oslInitOsk(char *descStr, char *initialStr, int textLimit, int linesNumber, int language){
    int i = 0;

    intext = (unsigned short *) malloc((textLimit + 1)*sizeof(unsigned short));
    if (!intext)
        return;
    outtext = (unsigned short *) malloc((textLimit + 1)*sizeof(unsigned short));
    if (!outtext)
        return;

    for (i=0; i<=strlen(initialStr); i++)
        intext[i] = (unsigned short)initialStr[i];

    for (i=0; i<=strlen(descStr); i++){
        desc[i] = (unsigned short)descStr[i];
        if (i >= 128)
            break;
    }

	memset(&OskData, 0, sizeof(OskData));
	/*if (language >= 0)
		OskData.language = language;
	else
	    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &OskData.language);*/
	OskData.language = PSP_UTILITY_OSK_LANGUAGE_DEFAULT;

	OskData.lines = linesNumber;
	OskData.unk_24 = 1;			    // set to 1
	OskData.desc = desc;
	OskData.intext = intext;
	OskData.outtextlength = textLimit;
	OskData.outtextlimit = textLimit;
	OskData.outtext = outtext;

	memset(&oskParams, 0, sizeof(oskParams));
	oskParams.base.size = sizeof(oskParams);
	if (language >= 0)
		oskParams.base.language = language;
	else
	    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &oskParams.base.language);
	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &oskParams.base.buttonSwap);
	oskParams.base.graphicsThread = 17;
	oskParams.base.accessThread = 19;
	oskParams.base.fontThread = 18;
	oskParams.base.soundThread = 16;
	oskParams.datacount = 1;
	oskParams.data = &OskData;

	sceUtilityOskInitStart(&oskParams);
    OskActive = 1;
}

void oslDrawOsk()
{
    switch(sceUtilityOskGetStatus()){
		case PSP_UTILITY_DIALOG_INIT:
        case PSP_UTILITY_DIALOG_VISIBLE :
            sceDisplayWaitVblankStart();
            sceDisplayWaitVblankStart();
			sceGuFinish();
			sceGuSync(0,0);
            sceUtilityOskUpdate(2);
			sceGuStart(GU_DIRECT, osl_list);
			oslSetAlpha(OSL_FX_RGBA, 0xff);
            break;
        case PSP_UTILITY_DIALOG_QUIT:
            sceUtilityOskShutdownStart();
            break;
        case PSP_UTILITY_DIALOG_FINISHED:
        case PSP_UTILITY_DIALOG_NONE:
			break;
    }
}

int oslOskIsActive(){
    return OskActive;
}

int oslGetOskStatus(){
    return sceUtilityOskGetStatus();
}


int oslOskGetResult(){
    return oskParams.base.result;
}

void oslOskGetText(char *text){
    int i, j;
    j = 0;
    for(i = 0; OskData.outtext[i]; i++)
        if (OskData.outtext[i]!='\0' && OskData.outtext[i]!='\n' && OskData.outtext[i]!='\r'){
            text[j] = OskData.outtext[i];
            j++;
        }
    text[j] = 0;
}

void oslEndOsk(){
    free(intext);
    free(outtext);
    memset(&OskData, 0, sizeof(OskData));
    memset(&oskParams, 0, sizeof(oskParams));
    OskActive = 0;
}
