#include <pspkernel.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("Internet Browser Test", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4*1024);

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
    int skip = 0;
    char message[100] = "";
    int browser = 0;
    SetupCallbacks();

    initOSLib();
    oslIntraFontInit(INTRAFONT_CACHE_MED);
	oslNetInit();

    //Loads image:
    OSL_IMAGE *bkg = oslLoadImageFilePNG("bkg.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);

    //Load font:
    OSL_FONT *font = oslLoadFontFile("flash0:/font/ltn0.pgf");
    oslSetFont(font);

    while(runningFlag && !osl_quit){
		browser = oslBrowserIsActive();
		if (!skip){
            oslStartDrawing();
            oslDrawImageXY(bkg, 0, 0);
            oslDrawString(30, 50, "Press X to open the internet browser");
            oslDrawString(30, 150, "Press /\\ to quit.");

            oslDrawString(30, 200, message);

            if (browser){
                oslDrawBrowser();
                if (oslGetBrowserStatus() == PSP_UTILITY_DIALOG_NONE){
					sprintf(message, "Browser closed");
                    oslEndBrowser();
                }
            }
            oslEndDrawing();
        }
        oslEndFrame();
        skip = oslSyncFrame();

        if (!browser){
            oslReadKeys();
            if (osl_keys->pressed.triangle){
                runningFlag = 0;
            }else if (osl_keys->pressed.cross){
                int res = oslBrowserInit("http://www.ps2dev.org/", "/PSP/PHOTO", 5*1024*1024,
                                         PSP_UTILITY_HTMLVIEWER_DISPLAYMODE_SMART_FIT,
                                         PSP_UTILITY_HTMLVIEWER_DISABLE_STARTUP_LIMITS,
                                         PSP_UTILITY_HTMLVIEWER_INTERFACEMODE_FULL,
                                         PSP_UTILITY_HTMLVIEWER_CONNECTMODE_MANUAL_ALL);
                memset(message, 0, sizeof(message));
				if (res)
					sprintf(message, "Error %i initializing browser!", res);
				else
					sprintf(message, "Browser initialized.");
			}
        }
    }
    //Quit OSL:
	oslNetTerm();
	oslEndGfx();
    oslQuit();

    sceKernelExitGame();
    return 0;

}
