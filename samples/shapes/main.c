#include <pspkernel.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("OSLib Shape Example", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12*1024);

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

    initOSLib();

    // Variables to store the selected drawing mode
    int drawMode = 0;  // 0: No drawing, 1: Line, 2: Rect, 3: Filled Rect, 4: Gradient Rect

    while(!osl_quit){
        if (!skip){
            oslStartDrawing();

            oslDrawFillRect(0, 0, 480, 272, RGBA(0, 0, 0, 255));  // Clear screen with black

            oslDrawString(50, 10, "Press X, Triangle, Square, or Circle to draw:");
            oslDrawString(50, 30, "Press Start to quit");

            oslReadKeys();

            // Check which button is pressed and set the corresponding drawing mode
            if (osl_keys->pressed.cross) {
                drawMode = 1;
            }
            else if (osl_keys->pressed.triangle) {
                drawMode = 2;
            }
            else if (osl_keys->pressed.square) {
                drawMode = 3;
            }
            else if (osl_keys->pressed.circle) {
                drawMode = 4;
            }
            else if (osl_keys->pressed.start) {
                oslQuit();
            }

            switch (drawMode) {
                case 1:  // Draw a line
                    oslDrawString(50, 250, "Drawing a line (X Button pressed)");
                    OSL_COLOR lineColor = RGBA(255, 0, 0, 255);  // Red
                    oslDrawLine(100, 100, 300, 200, lineColor);
                    break;
                case 2:  // Draw a rectangle outline
                    oslDrawString(50, 250, "Drawing a rectangle (Triangle Button pressed)");
                    OSL_COLOR rectColor = RGBA(0, 255, 0, 255);  // Green
                    oslDrawRect(50, 100, 250, 200, rectColor);
                    break;
                case 3:  // Draw a filled rectangle
                    oslDrawString(50, 250, "Drawing a filled rectangle (Square Button pressed)");
                    OSL_COLOR fillColor = RGBA(0, 0, 255, 255);  // Blue
                    oslDrawFillRect(50, 100, 250, 200, fillColor);
                    break;
                case 4:  // Draw a gradient rectangle
                    oslDrawString(50, 250, "Drawing a gradient rectangle (Circle Button pressed)");
                    OSL_COLOR topLeft = RGBA(255, 0, 0, 255);      // Red (top-left corner)
                    OSL_COLOR topRight = RGBA(0, 255, 0, 255);     // Green (top-right corner)
                    OSL_COLOR bottomLeft = RGBA(0, 0, 255, 255);   // Blue (bottom-left corner)
                    OSL_COLOR bottomRight = RGBA(255, 255, 0, 255);// Yellow (bottom-right corner)
                    oslDrawGradientRect(50, 100, 250, 200, topLeft, topRight, bottomLeft, bottomRight);
                    break;
            }

            oslEndDrawing();
        }

        oslEndFrame();
        skip = oslSyncFrame();
    }

    // End OSLib
    oslEndGfx();
    sceKernelExitGame();

    return 0;
}
