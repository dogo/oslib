#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>

#include <oslib/oslib.h>

PSP_MODULE_INFO("intraFontTest", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12*1024);

// Colors
enum colors {
    RED =	0xFF0000FF,
    GREEN =	0xFF00FF00,
    BLUE =	0xFFFF0000,
    WHITE =	0xFFFFFFFF,
    LITEGRAY = 0xFFBFBFBF,
    GRAY =  0xFF7F7F7F,
    DARKGRAY = 0xFF3F3F3F,
    BLACK = 0xFF000000
};

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common) {
    osl_quit = 1;
    return 0;
}

int initOSLib(){
    oslInit(0);
    oslInitGfx(OSL_PF_8888, 1);
    oslInitAudio();
    oslSetQuitOnLoadFailure(1);
    oslSetKeyAutorepeatInit(40);
    oslSetKeyAutorepeatInterval(10);
    return 0;
}

int endOSLib(){
    oslEndGfx();
    osl_quit = 1;
    return 0;
}

int main(){
    int skip = 0;

    initOSLib();
    oslIntraFontInit(INTRAFONT_CACHE_ALL | INTRAFONT_STRING_UTF8); // All fonts loaded with oslLoadIntraFontFile will have UTF8 support

    // Load background
    OSL_IMAGE *bkg = oslLoadImageFilePNG("bkg.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);;

    // Load fonts
    OSL_FONT* ltn[16];                                         //latin fonts (large/small, with/without serif, regular/italic/bold/italic&bold)
    char file[40];
    int i;
    for (i = 0; i < 16; i++) {
        sprintf(file, "flash0:/font/ltn%d.pgf", i);
        ltn[i] = oslLoadFontFile(file);
        oslIntraFontSetStyle(ltn[i], 1.0f,WHITE,BLACK,INTRAFONT_ALIGN_LEFT);
    }

    OSL_FONT* jpn0 = oslLoadIntraFontFile("flash0:/font/jpn0.pgf", INTRAFONT_CACHE_ALL | INTRAFONT_STRING_SJIS); //japanese font with SJIS support
    oslIntraFontSetStyle(jpn0, 1.0f,WHITE,BLACK, INTRAFONT_ALIGN_LEFT);

	OSL_FONT* kr0 = oslLoadIntraFontFile("flash0:/font/kr0.pgf", INTRAFONT_STRING_UTF8);  //Korean font (not available on all systems) with UTF-8 encoding
	oslIntraFontSetStyle(kr0, 0.8f, WHITE, DARKGRAY, 0);                                  //scale to 80%

	OSL_FONT* chn = oslLoadIntraFontFile("flash0:/font/gb3s1518.bwfon", 0);               //chinese font
	oslIntraFontSetStyle(chn, 0.8f, WHITE, DARKGRAY, 0);                                  //scale to 80%

    while(!osl_quit){
        if (!skip){
            oslStartDrawing();
            oslDrawImageXY(bkg, 0, 0);

            // Draw various text
            float y = 15;
            oslIntraFontSetStyle(ltn[4], 1.0f,BLACK,WHITE,INTRAFONT_ALIGN_CENTER);
            oslSetFont(ltn[4]);
            oslDrawStringf(240, y, "OSLib %s with intraFont by Sakya", OSL_VERSION);
            oslIntraFontSetStyle(ltn[4], 1.0f,WHITE,BLACK,INTRAFONT_ALIGN_LEFT);

            y += 30;
            oslSetFont(ltn[8]);
            oslDrawString(10, y, "Latin Sans-Serif: ");
            oslSetFont(ltn[0]);
            oslDrawString(180, y, "regular, ");
            oslSetFont(ltn[2]);
            oslDrawString(270, y, "italic, ");
            oslSetFont(ltn[4]);
            oslDrawString(330, y, "bold, ");
            oslSetFont(ltn[6]);
            oslDrawString(390, y, "both");

            y += 20;
            oslSetFont(ltn[8]);
            oslDrawString(10, y, "Latin Sans-Serif small: ");
            oslDrawString(180, y, "regular, ");
            oslSetFont(ltn[10]);
            oslDrawString(270, y, "italic, ");
            oslSetFont(ltn[12]);
            oslDrawString(330, y, "bold, ");
            oslSetFont(ltn[14]);
            oslDrawString(390, y, "both");

            y += 20;
            oslSetFont(ltn[8]);
            oslDrawString(10, y, "Latin with Serif: ");
            oslSetFont(ltn[1]);
            oslDrawString(180, y, "regular, ");
            oslSetFont(ltn[3]);
            oslDrawString(270, y, "italic, ");
            oslSetFont(ltn[5]);
            oslDrawString(330, y, "bold, ");
            oslSetFont(ltn[7]);
            oslDrawString(390, y, "both");

            y += 20;
            oslSetFont(ltn[8]);
            oslDrawString(10, y, "Latin with Serif small: ");
            oslSetFont(ltn[9]);
            oslDrawString(180, y, "regular, ");
            oslSetFont(ltn[11]);
            oslDrawString(270, y, "italic, ");
            oslSetFont(ltn[13]);
            oslDrawString(330, y, "bold, ");
            oslSetFont(ltn[15]);
            oslDrawString(390, y, "both");

            y += 20;
            oslSetFont(ltn[8]);
            oslDrawString(10, y, "JPN (S-JIS): ");
            oslSetFont(jpn0);
            oslDrawString(180, y, "イントラフォント");

            y += 25;
            oslSetFont(ltn[8]);
            oslDrawString(10, y, "Colors: ");
            oslIntraFontSetStyle(ltn[8], 1.0f,RED,BLUE,0);
            oslDrawString(80, y, "colorful, ");
            oslIntraFontSetStyle(ltn[8], 1.0f,WHITE,0,0);
            oslDrawString(140, y, "no shadow, ");
            oslIntraFontSetStyle(ltn[8], 1.0f,0,BLACK,0);
            oslDrawString(220, y, "no text, ");
            oslIntraFontSetStyle(ltn[8], 1.0f,0x7FFFFFFF,BLACK,0);
            oslDrawString(275, y, "transparent, ");
            oslIntraFontSetStyle(ltn[8], 1.0f,GRAY,WHITE,0);
            oslDrawString(363, y, "glowing, ");
            float t = ((float)(clock() % CLOCKS_PER_SEC)) / ((float)CLOCKS_PER_SEC);
            int val = (t < 0.5f) ? t*511 : (1.0f-t)*511;
            oslIntraFontSetStyle(ltn[8], 1.0f,LITEGRAY,(0xFF<<24)+(val<<16)+(val<<8)+(val),0);
            oslDrawString(425, y, "flashing");
            oslIntraFontSetStyle(ltn[8], 1.0f,WHITE,BLACK,0);

            y += 20;
            oslDrawString(10, y, "Spacing: ");
            oslIntraFontSetStyle(ltn[8], 1.0f,WHITE,BLACK,INTRAFONT_WIDTH_FIX);
            oslDrawString(80, y, "fixed (default), ");
            oslIntraFontSetStyle(ltn[8], 1.0f,WHITE,BLACK,INTRAFONT_WIDTH_FIX | 12);
            oslDrawString(220, y, "fixed (12), ");
            oslIntraFontSetStyle(ltn[8], 1.0f,WHITE,BLACK,0);
            oslDrawString(360, y, "variable width");

            y += 30;
            oslDrawString(10, y, "Scaling: ");
            oslIntraFontSetStyle(ltn[0], 0.5f,WHITE,BLACK,0);
            oslSetFont(ltn[0]);
            oslDrawString(80, y, "tiny, ");
            oslIntraFontSetStyle(ltn[0], 0.75f,WHITE,BLACK,0);
            oslDrawString(110, y, "small, ");
            oslIntraFontSetStyle(ltn[0], 1.0f,WHITE,BLACK,0);
            oslDrawString(160, y, "regular, ");
            oslIntraFontSetStyle(ltn[0], 1.25f,WHITE,BLACK,0);
            oslDrawString(250, y, "large, ");
            oslIntraFontSetStyle(ltn[0], 1.5f,WHITE,BLACK,0);
            oslDrawString(330, y, "huge");
            oslIntraFontSetStyle(ltn[0], 1.0f,WHITE,BLACK,0);

            y += 20;
            oslSetFont(ltn[8]);
            oslDrawString(10, y, "Align: ");
            oslIntraFontSetStyle(ltn[8], 1.0f,WHITE,BLACK,INTRAFONT_ALIGN_LEFT);
            oslDrawString(80, y, "left");
            oslIntraFontSetStyle(ltn[8], 1.0f,WHITE,BLACK,INTRAFONT_ALIGN_CENTER);
            oslDrawString((80+470)/2, y, "center");
            oslIntraFontSetStyle(ltn[8], 1.0f,WHITE,BLACK,INTRAFONT_ALIGN_RIGHT);
            oslDrawString(470, y, "right");
            oslIntraFontSetStyle(ltn[8], 1.0f,WHITE,BLACK,0);

            y += 25;
            oslIntraFontSetStyle(ltn[4], 1.0f,BLACK,WHITE,INTRAFONT_ALIGN_CENTER);
            oslSetFont(ltn[4]);
            oslDrawString(240, y, "Press X to quit");

            oslEndDrawing();
        }

        oslReadKeys();
        if (osl_keys->pressed.cross)
            osl_quit = 1;

        oslEndFrame();
    	skip = oslSyncFrame();
    }

    for (i = 0; i < 16; i++)
        oslDeleteFont(ltn[i]);
    oslDeleteImage(bkg);
    oslIntraFontShutdown();
    endOSLib();
	sceKernelExitGame();
    return 0;
}
