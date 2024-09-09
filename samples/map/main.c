#include <pspkernel.h>
#include <oslib/oslib.h>
#include "maps.h"

PSP_MODULE_INFO("Map Drawing Example", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12 * 1024);

OSL_IMAGE *tilesets[8];   // Array for tile images
OSL_MAP *maps[8];         // Array for maps
OSL_SOUND *music, *jump, *key;

int layers = 8;

void InitMaps();
void HandleKeys();
void RenderText();
OSL_IMAGE *LoadTileset(const char *filename);
void CleanupResources();
void CheckFilesLoaded();

int main(int argc, char* argv[])
{
	// Initialization
	oslInit(0);						// OSLib core
	oslInitGfx(OSL_PF_8888, 1);		// Graphics
	oslInitConsole();				// Text
	oslInitAudio();					// Sound

	// Load tilesets
	tilesets[0] = LoadTileset("nuages.png");
	tilesets[1] = LoadTileset("grassland.png");
	tilesets[2] = LoadTileset("nuage1.png");
	tilesets[3] = LoadTileset("nuage2.png");
	tilesets[4] = LoadTileset("montagne.png");
	tilesets[5] = LoadTileset("niveau.png");
	tilesets[6] = LoadTileset("decors.png");
	tilesets[7] = LoadTileset("sol.png");

	// Load sounds
	music = oslLoadSoundFile("Resources/music.bgm", OSL_FMT_STREAM);
	jump = oslLoadSoundFile("Resources/jump.wav", OSL_FMT_NONE);
	key = oslLoadSoundFile("Resources/key.wav", OSL_FMT_NONE);

	// Check if all files were loaded successfully
	CheckFilesLoaded();

	// Loop the music
	oslSetSoundLoop(music, 1);

	// Configure joypad auto-repeat
	oslSetKeyAutorepeatInit(40);
	oslSetKeyAutorepeatInterval(10);

	// Initialize maps
	InitMaps();

	while (!osl_quit)
	{
		oslStartDrawing();

		// Draw gradient background
		oslDrawGradientRect(0, 0, 480, 272, RGB(0, 0, 128), RGB(0, 0, 128), RGB(0, 255, 255), RGB(0, 255, 255));

		// Handle input keys
		HandleKeys();

		// Draw maps
		for (int i = 0; i < layers && i < 8; ++i) {
			oslDrawMapSimple(maps[i]);
		}

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

	// Adjust layer visibility
	if (osl_keys->pressed.down && layers > 0) layers--;
	if (osl_keys->pressed.up && layers < 8) layers++;
	if (osl_keys->pressed.left) layers = 0;
	if (osl_keys->pressed.right) layers = 8;

	// Scroll maps using joystick
	int scrollSpeed[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	for (int i = 0; i < 8; ++i) {
		if (osl_keys->analogX > 32) maps[i]->scrollX += scrollSpeed[i];
		if (osl_keys->analogX < -32) maps[i]->scrollX -= scrollSpeed[i];
	}

	// Handle sound controls
	if (osl_keys->pressed.square) oslPauseSound(music, -1);
	if (osl_keys->pressed.circle) oslPlaySound(key, 1);
	if (osl_keys->pressed.cross) oslPlaySound(jump, 2);
	if (osl_keys->pressed.triangle) oslStopSound(music);
	if (osl_keys->pressed.R) oslPlaySound(music, 0);

	// Handle other actions
	if (osl_keys->pressed.start) oslQuit();   // Exit the application
	if (osl_keys->pressed.select)             // Take a screenshot
		oslWriteImageFile(OSL_SECONDARY_BUFFER, "screenshot.png", 0);
}

void RenderText()
{
	// Set text color
	oslSetTextColor(RGBA(0, 0, 0, 255));
	oslSetBkColor(RGBA(0, 0, 0, 0));

	// Display instructions
	oslPrintf_xy(1, 5, "OSLib Maps & Sound Sample");
	oslPrintf_xy(1, 25, "Left/Right: display maps");
	oslPrintf_xy(1, 35, "Up/Down: more/less maps");
	oslPrintf_xy(1, 45, "Joystick: scroll maps");
	oslPrintf_xy(1, 55, "R: play music");
	oslPrintf_xy(1, 65, "[]: pause music");
	oslPrintf_xy(1, 75, "^: stop music");
	oslPrintf_xy(1, 85, "X & (): play sounds");
	oslPrintf_xy(1, 95, "Select: take screenshot");
	oslPrintf_xy(1, 105, "Start: quit");
}

void InitMaps()
{
	const int tileSize = 8;
	const int mapWidth = 64;
	const int mapHeight = 34;
	const OSL_MAP_FORMAT format = OSL_MF_U16;

	maps[0] = oslCreateMap(tilesets[0], nuages_map, tileSize, tileSize, mapWidth, mapHeight, format);
	maps[1] = oslCreateMap(tilesets[1], grassland_map, tileSize, tileSize, mapWidth, mapHeight, format);
	maps[2] = oslCreateMap(tilesets[2], nuage1_map, tileSize, tileSize, mapWidth, mapHeight, format);
	maps[3] = oslCreateMap(tilesets[3], nuage2_map, tileSize, tileSize, mapWidth, mapHeight, format);
	maps[4] = oslCreateMap(tilesets[4], montagne_map, tileSize, tileSize, mapWidth, mapHeight, format);
	maps[5] = oslCreateMap(tilesets[5], niveau_map, tileSize, tileSize, mapWidth, mapHeight, format);
	maps[6] = oslCreateMap(tilesets[6], decors_map, tileSize, tileSize, mapWidth, mapHeight, format);
	maps[7] = oslCreateMap(tilesets[7], sol_map, tileSize, tileSize, mapWidth, mapHeight, format);
}

OSL_IMAGE *LoadTileset(const char *filename)
{
	char fullPath[256];
	snprintf(fullPath, sizeof(fullPath), "Resources/%s", filename);
	return oslLoadImageFile(fullPath, OSL_IN_RAM, OSL_PF_5551);
}

void CheckFilesLoaded()
{
	for (int i = 0; i < 8; ++i) {
		if (!tilesets[i]) {
			oslFatalError("Tileset %d failed to load. Ensure all resources are in the correct directory.", i);
		}
	}
	if (!music || !jump || !key) {
		oslFatalError("Sound file(s) missing. Ensure all audio files are in the correct directory.");
	}
}

void CleanupResources()
{
	for (int i = 0; i < 8; ++i) {
		if (tilesets[i]) oslDeleteImage(tilesets[i]);
		if (maps[i]) oslDeleteMap(maps[i]);
	}

	if (music) oslDeleteSound(music);
	if (jump) oslDeleteSound(jump);
	if (key) oslDeleteSound(key);
}
