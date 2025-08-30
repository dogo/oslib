#include <pspkernel.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("OSK Test", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12*1024);

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
    SetupCallbacks();

    initOSLib();
    oslIntraFontInit(INTRAFONT_CACHE_MED);

    //Loads image:
    OSL_IMAGE *bkg = oslLoadImageFilePNG("bkg.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);

    //Load font:
    OSL_FONT *font = oslLoadFontFile("flash0:/font/ltn0.pgf");
    oslSetFont(font);

    while(runningFlag && !osl_quit){
        if (!skip){
			oslStartDrawing();
			oslDrawImageXY(bkg, 0, 0);
			oslDrawString(30, 50, "Press X to use the On Screen Keyboard.");
			oslDrawString(30, 150, "Press /\\ to quit.");

			oslDrawString(30, 200, message);

			if (oslOskIsActive()){
				oslDrawOsk();
				if (oslGetOskStatus() == PSP_UTILITY_DIALOG_NONE){
					if (oslOskGetResult() == OSL_OSK_CANCEL)
						sprintf(message, "Cancel");
					else{
						char userText[100] = "";
						oslOskGetText(userText);
						sprintf(message, "You entered: %s", userText);
					}
					oslEndOsk();
				}
			}
			oslEndDrawing();
		}

		if (!oslOskIsActive()){
			oslReadKeys();
			if (osl_keys->pressed.triangle){
				runningFlag = 0;
			}else if (osl_keys->pressed.cross){
				oslInitOsk("Please insert some text", "Initial text", 128, 1, -1);
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
