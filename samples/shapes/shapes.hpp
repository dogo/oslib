#pragma once
#include <pspkernel.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("OSLib Shape Example", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12*1024);

namespace sys
{
	int32_t   w = 320;
	int32_t   h = 240;
	float ratio = (float)w/(float)h;
	float     z = 1.0f;
	float     x = 0.0f;
	bool   skip = false;
	bool   quit = false;
	int    mode = 0;
	inline bool init()
	{
		oslInit(0);
		oslInitGfx(OSL_PF_8888, 1);
		oslInitAudio();
		oslSetQuitOnLoadFailure(1);
		oslSetKeyAutorepeatInit(40);
		oslSetKeyAutorepeatInterval(10);
		return true;
	}
	inline bool feed()
	{
		oslReadKeys();
		if (osl_keys->pressed.cross)
			mode = 1;
		else if (osl_keys->pressed.triangle)
			mode = 2;
		else if (osl_keys->pressed.square)
			mode = 3;
		else if (osl_keys->pressed.circle)
			mode = 4;
		else if (osl_keys->pressed.start)
			oslQuit();
		return !(quit = osl_quit);
	}
	inline bool swap()
	{
		oslEndFrame();
		return oslSyncFrame() != 0;
	}
	inline bool halt()
	{
		oslEndGfx();
		sceKernelExitGame();
		return true;
	}
	inline bool draw()
	{
		oslStartDrawing();
		oslDrawFillRect(0, 0, 480, 272, RGBA(0, 0, 0, 255));  // Clear screen with black
		oslDrawString(50, 10, "Press X, Triangle, Square, or Circle to draw:");
		oslDrawString(50, 30, "Press Start to quit");

		// Check which button is pressed and set the corresponding drawing mode
		switch (mode)
		{
			case 1:
				{
					oslDrawString(50, 250, "Drawing a line (X Button pressed)");
					OSL_COLOR lineColor = RGBA(255, 0, 0, 255);
					oslDrawLine(100, 100, 300, 200, lineColor);
				}
				break;
			case 2:
				{
					oslDrawString(50, 250, "Drawing a rectangle (Triangle Button pressed)");
					OSL_COLOR rectColor = RGBA(0, 255, 0, 255);  // Green
					oslDrawRect(50, 100, 250, 200, rectColor);
				}
				break;
			case 3:
				{
					oslDrawString(50, 250, "Drawing a filled rectangle (Square Button pressed)");
					OSL_COLOR fillColor = RGBA(0, 0, 255, 255);  // Blue
					oslDrawFillRect(50, 100, 250, 200, fillColor);
				}
				break;
			case 4:
				{
					oslDrawString(50, 250, "Drawing a gradient rectangle (Circle Button pressed)");
					OSL_COLOR topLeft = RGBA(255, 0, 0, 255);      // Red (top-left corner)
					OSL_COLOR topRight = RGBA(0, 255, 0, 255);     // Green (top-right corner)
					OSL_COLOR bottomLeft = RGBA(0, 0, 255, 255);   // Blue (bottom-left corner)
					OSL_COLOR bottomRight = RGBA(255, 255, 0, 255);// Yellow (bottom-right corner)
					oslDrawGradientRect(50, 100, 250, 200, topLeft, topRight, bottomLeft, bottomRight);
				}
				break;
			default:
				break;
		}

		oslEndDrawing();
		return true;
	}
};

