// Memory owned by the firmware dialogs: each save frees its copy of the data, a keyboard
// can be freed without being activated, and oslInitOsk's failures free only what they
// made. Heap measured with mallinfo. Writes and then removes
// ms0:/PSP/SAVEDATA/OSLTEST00MEMTEST (autosave shows no dialog).
#include <pspkernel.h>
#include <psputility.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("dialogmem", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define SAVEDIR "ms0:/PSP/SAVEDATA/OSLTEST00MEMTEST"

extern OSL_KEYBOARD *osl_osk;

static FILE *f;
static char data[64 * 1024];

static int heapUsed(void) {
	return mallinfo().uordblks;
}

// Runs one autosave to its end; returns its result, or -1 if it never ended
static int autosave(void) {
	struct oslSaveLoad d;
	memset(&d, 0, sizeof(d));
	strcpy(d.gameTitle, "OSLib test");
	strcpy(d.gameID, "OSLTEST00");
	strcpy(d.saveName, "MEMTEST");
	strcpy(d.savedataTitle, "dialogmem");
	strcpy(d.detail, "removed by the test");
	d.data = data;
	d.dataSize = sizeof(data);
	d.dialogType = 2; // auto
	oslInitSaveDialog(&d);
	for (int frame = 0; frame < 1200 && oslGetSaveLoadType() != OSL_DIALOG_NONE; frame++) {
		oslStartDrawing();
		oslDrawSaveLoad();
		oslEndDrawing();
		oslEndFrame();
		oslSyncFrame();
		if (oslGetLoadSaveStatus() == PSP_UTILITY_DIALOG_NONE) {
			int result = oslSaveLoadGetResult();
			oslEndSaveLoadDialog();
			return result;
		}
	}
	return -1;
}

static void saves(void) {
	fprintf(f, "5 autosaves of 64 KB       ");
	int result = autosave(); // warm-up
	int before = heapUsed();
	for (int i = 0; i < 5 && result == 0; i++)
		result = autosave();
	int kept = heapUsed() - before;
	fprintf(f, "-> result %d, %d bytes kept %s\n", result, kept, !result && !kept ? "PASS" : "FAIL");
	sceIoRemove(SAVEDIR "/DATA.BIN");
	sceIoRemove(SAVEDIR "/PARAM.SFO");
	sceIoRmdir(SAVEDIR);
}

static void keyboardNeverActivated(void) {
	fprintf(f, "20 keyboards, not activated ");
	int before = heapUsed();
	for (int i = 0; i < 20; i++) {
		OSL_KEYBOARD *kbd = oslInitOskEx(1, -1);
		oslInitOskDataEx(kbd, 0, NULL, NULL, 64, 1);
		oslEndOskEx(kbd);
	}
	int kept = heapUsed() - before;
	fprintf(f, "-> %d bytes kept %s\n", kept, kept ? "FAIL" : "PASS");
}

static void initOskFails(void) {
	fprintf(f, "20 oslInitOsk, text alloc fails ");
	int before = heapUsed();
	for (int i = 0; i < 20; i++)
		oslInitOsk("desc", "text", 0x40000000, 1, -1); // (limit + 1) * 2 bytes can't be had
	int kept = heapUsed() - before;
	fprintf(f, "-> %d bytes kept, active %d %s\n", kept, osl_osk != NULL, !kept && !osl_osk ? "PASS" : "FAIL");
}

// Another keyboard is active: oslInitOsk must fail without touching it
static void initOskWhileAnotherActive(void) {
	fprintf(f, "oslInitOsk, another active ");
	OSL_KEYBOARD *other = oslInitOskEx(1, -1);
	osl_osk = other; // as if activated, without starting the firmware dialog
	oslInitOsk("desc", "text", 64, 1, -1);
	int kept = osl_osk == other;
	fprintf(f, "-> other still active %d %s\n", kept, kept ? "PASS" : "FAIL");
	if (kept) {
		osl_osk = NULL;
		oslEndOskEx(other);
	}
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0); // keep what was written if a case crashes
	for (unsigned i = 0; i < sizeof(data); i++) data[i] = i;

	saves();
	keyboardNeverActivated();
	initOskFails();
	initOskWhileAnotherActive();

	fprintf(f, "DONE\n");
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
