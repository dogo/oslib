#include <pspkernel.h>
#include <oslib/oslib.h>

//Data in icon.h are generated with this command:
//  bin2c "icon0.png" "icon.h" "icon0"
#include "icon.h"

PSP_MODULE_INFO("Save and Load Test", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(6*1024);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Data for save and load:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char gameTitle[50] = "Title";
char gameID[10] = "ITA01";
char saveName[10] = "0001";

char nameList[][20] =	// End list with ""
{
 "0000",
 "0001",
 "0002",
 "0003",
 "0004",
 ""
};

char data[100] = "someData: 50";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int runningFlag = 1;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Exit callback */
int exit_callback(int arg1, int arg2, void *common) {
    runningFlag = 0;
    return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp) {
    int cbid;

    cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void) {
    int thid = 0;
    thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0);
    if(thid >= 0)
        sceKernelStartThread(thid, 0, 0);
    return thid;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Init OSLib:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int initOSLib(){
    oslInit(0);
    oslInitGfx(OSL_PF_8888, 1);
    oslInitAudio();
    oslSetQuitOnLoadFailure(1);
    oslSetKeyAutorepeatInit(40);
    oslSetKeyAutorepeatInterval(10);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(){
    struct oslSaveLoad saveLoadData;

    int type = OSL_DIALOG_NONE;
    int skip = 0;
    char message[100] = "";
    char loadedData[100] = "";

    SetupCallbacks();

    initOSLib();
    oslIntraFontInit(INTRAFONT_CACHE_MED);

    //Loads image:
    OSL_IMAGE *bkg = oslLoadImageFilePNG("bkg.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);

    //Load font:
    OSL_FONT *font = oslLoadFontFile("flash0:/font/ltn0.pgf");
    oslSetFont(font);

    while(runningFlag && !osl_quit){
		type = oslGetSaveLoadType();

		if (!skip){
            oslStartDrawing();
            oslDrawImageXY(bkg, 0, 0);

            oslDrawString(30, 50, "Press X to save data.");
            oslDrawString(30, 70, "Press O to load data.");
            oslDrawString(30, 150, "Press /\\ to quit.");
            oslDrawString(30, 200, message);

            if (type != OSL_DIALOG_NONE){
                oslDrawSaveLoad();
                if (oslGetLoadSaveStatus() == PSP_UTILITY_DIALOG_NONE){
                    if (oslSaveLoadGetResult() == OSL_SAVELOAD_CANCEL)
                        sprintf(message, "Cancel");
                    else if (type == OSL_DIALOG_LOAD)
                        sprintf(message, "Loaded data: %s", (char *)saveLoadData.data);
                    else if (type == OSL_DIALOG_SAVE)
                        sprintf(message, "Saved data: %s", (char *)saveLoadData.data);
                    oslEndSaveLoadDialog();
                }
            }
            oslEndDrawing();
        }

        if (type == OSL_DIALOG_NONE){
            oslReadKeys();
            if (osl_keys->pressed.triangle){
                break;
            }else if (osl_keys->pressed.cross){
                memset(&saveLoadData, 0, sizeof(saveLoadData));
                strcpy(saveLoadData.gameTitle, gameTitle);
                strcpy(saveLoadData.gameID, gameID);
                strcpy(saveLoadData.saveName, saveName);
                saveLoadData.nameList = nameList;
                saveLoadData.icon0 = icon0;
                saveLoadData.size_icon0 = size_icon0;
                saveLoadData.data = data;
                saveLoadData.dataSize = 100;
                oslInitSaveDialog(&saveLoadData);
                memset(message, 0, sizeof(message));
            }else if (osl_keys->pressed.circle){
                memset(&saveLoadData, 0, sizeof(saveLoadData));
                strcpy(saveLoadData.gameID, gameID);
                strcpy(saveLoadData.saveName, saveName);
                saveLoadData.nameList = nameList;
                saveLoadData.data = &loadedData;
                saveLoadData.dataSize = 100;
                oslInitLoadDialog(&saveLoadData);
                memset(message, 0, sizeof(message));
            }
        }

        oslEndFrame();
        skip = oslSyncFrame();
    }

    //Quit OSL:
    oslEndGfx();
    sceKernelExitGame();
    return 0;

}
