#include <pspkernel.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("Rect and Fonts Test", 0, 1, 0);
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
    SetupCallbacks();

    initOSLib();
    oslIntraFontInit(INTRAFONT_CACHE_MED);

    //Loads image:
    OSL_IMAGE *bkg = oslLoadImageFilePNG("bkg.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    OSL_IMAGE *rect_01 = oslLoadImageFilePNG("rect_01.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);

    //Load fonts:
    OSL_FONT *pgfFont = oslLoadFontFile("flash0:/font/ltn0.pgf");
    oslIntraFontSetStyle(pgfFont, 0.5, RGBA(255,255,255,255), RGBA(0,0,0,0), INTRAFONT_ALIGN_LEFT);

    OSL_FONT *oftFont = oslLoadFontFile("font.oft");
    oslSetTextColor(RGBA(255,255,255,255));
    oslSetBkColor(RGBA(0,0,0,0));

    while(runningFlag && !osl_quit){
        if (!skip){
            oslStartDrawing();
            oslDrawImageXY(bkg, 0, 0);

            oslDrawImageXY(rect_01, 50, 50);
            oslSetFont(pgfFont);
            oslDrawString(55, 55, "pgf on alpha image");
            oslSetFont(oftFont);
            oslDrawString(55, 70, "oft on alpha image");

            oslDrawFillRect(30, 150, 200, 250, RGB(150, 150, 150));
            oslSetFont(pgfFont);
            oslDrawString(35, 155, "pgf on rect");
            oslSetFont(oftFont);
            oslDrawString(35, 170, "oft on rect");

            oslDrawFillRect(300, 150, 400, 250, RGB(100, 100, 100));
            oslSetFont(pgfFont);
            oslDrawString(305, 155, "pgf on rect");
            oslSetFont(oftFont);
            oslDrawString(305, 170, "oft on rect");

            oslDrawGradientRect(300, 20, 430, 120, RGBA(100, 100, 100, 100),RGBA(100, 100, 100, 100), RGBA(200, 200, 200, 100), RGBA(200, 200, 200, 100));
            oslSetFont(pgfFont);
            oslDrawString(305, 25, "pgf on gradient rect");
            oslSetFont(oftFont);
            oslDrawString(305, 40, "oft on gradient rect");

            oslEndDrawing();
        }
        oslEndFrame();
        skip = oslSyncFrame();
    }
    //Quit OSL:
    oslEndGfx();
    oslQuit();

    sceKernelExitGame();
    return 0;

}
