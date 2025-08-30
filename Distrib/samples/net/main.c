#include <pspkernel.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("Net Test", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(6*1024);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ADDRESS "www.google.it"

static int runningFlag = 1;
static OSL_IMAGE *bkg = NULL;
static char message[100] = "";
static char buffer[100] = "";

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
// Connect to Access Point:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int connectAPCallback(int state){
    oslStartDrawing();
    oslDrawImageXY(bkg, 0, 0);
    oslDrawString(30, 200, "Connecting to AP...");
    sprintf(buffer, "State: %i", state);
    oslDrawString(30, 230, buffer);
    oslEndDrawing();
    oslEndFrame();
    oslSyncFrame();

    return 0;
}


int connectToAP(int config){
    oslStartDrawing();
    oslDrawImageXY(bkg, 0, 0);
    oslDrawString(30, 200, "Connecting to AP...");
    oslEndDrawing();
    oslEndFrame();
    oslSyncFrame();

    int result = oslConnectToAP(config, 30, connectAPCallback);
    if (!result){
        char ip[30] = "";
        char resolvedIP[30] = "";

        oslStartDrawing();
        oslDrawImageXY(bkg, 0, 0);
        oslGetIPaddress(ip);
        sprintf(buffer, "IP address: %s", ip);
        oslDrawString(30, 170, buffer);

        sprintf(buffer, "Resolving %s", ADDRESS);
        oslDrawString(30, 200, buffer);
        oslEndDrawing();
        oslEndFrame();
        oslSyncFrame();

        result = oslResolveAddress(ADDRESS, resolvedIP);

        oslStartDrawing();
        oslDrawImageXY(bkg, 0, 0);
        oslGetIPaddress(ip);
        if (!result)
            sprintf(buffer, "Resolved IP address: %s", ip);
        else
            sprintf(buffer, "Error resolving address!");
        oslDrawString(30, 230, buffer);
        oslEndDrawing();
        oslEndFrame();
        oslSyncFrame();
		sceKernelDelayThread(3*1000000);
    }else{
        oslStartDrawing();
        oslDrawImageXY(bkg, 0, 0);
        sprintf(buffer, "Error connecting to AP!");
        oslDrawString(30, 200, buffer);
        oslEndDrawing();
        oslEndFrame();
        oslSyncFrame();
		sceKernelDelayThread(3*1000000);
    }
    oslDisconnectFromAP();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(){
	int skip = 0;
    int enabled = 1;
    int selectedConfig = 0;
    SetupCallbacks();

    initOSLib();
    oslIntraFontInit(INTRAFONT_CACHE_MED);
    oslNetInit();

    //Loads image:
    bkg = oslLoadImageFilePNG("bkg.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);

    //Load font:
    OSL_FONT *font = oslLoadFontFile("flash0:/font/ltn0.pgf");
    oslSetFont(font);

    if (!oslIsWlanPowerOn())
        sprintf(message, "Please turn on the WLAN.");

    //Get connections list:
    struct oslNetConfig configs[OSL_MAX_NET_CONFIGS];
    int numconfigs = oslGetNetConfigs(configs);
    if (!numconfigs){
        sprintf(message, "No configuration found!");
        enabled = 0;
    }

    while(runningFlag && !osl_quit){
        if (!skip){
			oslStartDrawing();
			oslDrawImageXY(bkg, 0, 0);
            if (enabled){
                sprintf(buffer, "Press X to connect to %s.", configs[selectedConfig].name);
    			oslDrawString(30, 50, buffer);
    			oslDrawString(30, 80, "Press UP and DOWN to change settings.");
            }
            oslDrawString(30, 150, "Press /\\ to quit.");
			oslDrawString(30, 200, message);

			oslEndDrawing();
		}
        oslEndFrame();
        skip = oslSyncFrame();

        oslReadKeys();
        if (osl_keys->released.triangle)
            runningFlag = 0;

        if (osl_keys->released.cross){
            connectToAP(selectedConfig + 1);
        }else if (osl_keys->released.up){
            if (++selectedConfig >= numconfigs)
                selectedConfig = numconfigs - 1;
        }else if (osl_keys->released.down){
            if (--selectedConfig < 0)
                selectedConfig = 0;
        }
    }
    //Quit OSL:
    oslNetTerm();
    oslEndGfx();
    sceKernelExitGame();
    return 0;

}
