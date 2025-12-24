#include <pspkernel.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("Audio BGM Sample", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12 * 1024);

OSL_SOUND *bgm;
OSL_IMAGE *bkg;

void HandleKeys();
void RenderText();
void CleanupResources();

int main(int argc, char* argv[])
{
	// Initialization
	oslInit(0);						// OSLib core
	oslInitGfx(OSL_PF_8888, 1);		// Graphics
	oslInitConsole();				// Text
	oslInitAudio();					// Sound
	oslSetQuitOnLoadFailure(1);		// Auto-quit on load failure

	// Configure joypad auto-repeat
	oslSetKeyAutorepeatInit(40);
	oslSetKeyAutorepeatInterval(10);

	// Load background image
	bkg = oslLoadImageFile("bkg.png", OSL_IN_RAM, OSL_PF_8888);

	// Load BGM sound file
	bgm = oslLoadSoundFile("Resources/music.bgm", OSL_FMT_STREAM);

	// If BGM loaded successfully, configure and play it
	if (bgm) {
		// Set BGM to loop
		oslSetSoundLoop(bgm, 1);

		// Start playing BGM automatically on channel 0
		oslPlaySound(bgm, 0);
	}

	while (!osl_quit)
	{
		oslStartDrawing();

		// Draw background
		if (bkg) {
			oslDrawImageXY(bkg, 0, 0);
		} else {
			// Fallback gradient if image fails
			oslDrawGradientRect(0, 0, 480, 272, RGB(0, 0, 64), RGB(0, 0, 64), RGB(0, 64, 128), RGB(0, 64, 128));
		}

		// Handle input keys
		HandleKeys();

		// Render on-screen text
		RenderText();

		oslEndDrawing();
		oslSyncFrame();
		oslAudioVSync();
	}

	// Cleanup resources and exit
	CleanupResources();
	oslEndGfx();
	oslQuit();

	return 0;
}

void HandleKeys()
{
	oslReadKeys();

	// BGM controls (only if BGM is loaded)
	if (bgm) {
		if (osl_keys->pressed.square) {
			oslPauseSound(bgm, -1);  // Toggle pause
		}
		if (osl_keys->pressed.triangle) {
			oslStopSound(bgm);  // Stop BGM
		}
		if (osl_keys->pressed.circle) {
			int channel = oslGetSoundChannel(bgm);
			if (channel < 0) {
				oslPlaySound(bgm, 0);
			} else if (osl_audioActive[channel] == 2) {
				oslPauseSound(bgm, 0);
			}
		}
	}

	// Exit
	if (osl_keys->pressed.start) {
		oslQuit();
	}
}

void RenderText()
{
	// Set text color
	oslSetTextColor(RGBA(255, 255, 255, 255));
	oslSetBkColor(RGBA(0, 0, 0, 128));

	// Display title
	oslPrintf_xy(10, 10, "OSLib Audio BGM Sample");

	// Display instructions
	oslPrintf_xy(10, 40, "Controls:");
	oslPrintf_xy(10, 60, "Circle: Play/Resume BGM");
	oslPrintf_xy(10, 80, "Square: Pause/Unpause BGM");
	oslPrintf_xy(10, 100, "Triangle: Stop BGM");
	oslPrintf_xy(10, 120, "Start: Quit");

	// Display status
	if (bgm) {
		int channel = oslGetSoundChannel(bgm);
		if (channel >= 0) {
			extern volatile int osl_audioActive[];
			if (osl_audioActive[channel] == 2) {
				oslPrintf_xy(10, 150, "Status: Paused (Channel %d)", channel);
			} else if (osl_audioActive[channel] == 1) {
				oslPrintf_xy(10, 150, "Status: Playing (Channel %d)", channel);
			} else {
				oslPrintf_xy(10, 150, "Status: Stopped");
			}
		} else {
			oslPrintf_xy(10, 150, "Status: Stopped");
		}
	} else {
		oslPrintf_xy(10, 150, "Status: BGM not loaded");
	}
}

void CleanupResources()
{
	if (bgm) {
		oslDeleteSound(bgm);
	}
	if (bkg) {
		oslDeleteImage(bkg);
	}
}
