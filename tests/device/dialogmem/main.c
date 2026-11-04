// Memory owned by the firmware dialogs: each save frees its copy of the data, a keyboard
// can be freed without being activated, and oslInitOsk's failures free only what they
// made. Heap measured with mallinfo. Writes and then removes
// ms0:/PSP/SAVEDATA/OSLTEST00MEMTEST (autosave shows no dialog).
#include <pspkernel.h>
#include <psputility.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("dialogmem", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define SAVEDIR "ms0:/PSP/SAVEDATA/OSLTEST00MEMTEST"

extern OSL_KEYBOARD *osl_osk;
extern SceUtilitySavedataParam savedata;

static FILE *f;
static char data[64 * 1024];
static size_t failAllocationSize;
static int allocationFailures, savedataInitCalls, failSavedataInit;

void *__real_malloc(size_t size);
int __real_sceUtilitySavedataInitStart(SceUtilitySavedataParam *params);

// Fail only the requested allocation; firmware allocations and cleanup stay real.
void *__wrap_malloc(size_t size) {
	if (failAllocationSize && size == failAllocationSize) {
		failAllocationSize = 0;
		allocationFailures++;
		return NULL;
	}
	return __real_malloc(size);
}

int __wrap_sceUtilitySavedataInitStart(SceUtilitySavedataParam *params) {
	savedataInitCalls++;
	if (failSavedataInit) return -1234;
	return __real_sceUtilitySavedataInitStart(params);
}

static int heapUsed(void) {
	return mallinfo().uordblks;
}

static void saveData(struct oslSaveLoad *d, int mode) {
	memset(d, 0, sizeof(*d));
	strcpy(d->gameTitle, "OSLib test");
	strcpy(d->gameID, "OSLTEST00");
	strcpy(d->saveName, "MEMTEST");
	strcpy(d->savedataTitle, "dialogmem");
	strcpy(d->detail, "removed by the test");
	d->data = data;
	d->dataSize = sizeof(data);
	d->dialogType = mode;
}

// Runs one autosave to its end; returns its result, or -1 if it never ended.
static int autosave(int end) {
	struct oslSaveLoad d;
	saveData(&d, 2);
	oslInitSaveDialog(&d);
	for (int frame = 0; frame < 1200 && oslGetSaveLoadType() != OSL_DIALOG_NONE; frame++) {
		oslStartDrawing();
		oslDrawSaveLoad();
		oslEndDrawing();
		oslEndFrame();
		oslSyncFrame();
		if (oslGetLoadSaveStatus() == PSP_UTILITY_DIALOG_NONE) {
			int result = oslSaveLoadGetResult();
			if (end) oslEndSaveLoadDialog();
			return result;
		}
	}
	return -1;
}

static void saves(void) {
	fprintf(f, "5 autosaves of 64 KB       ");
	int result = autosave(1); // warm-up
	int before = heapUsed();
	for (int i = 0; i < 5 && result == 0; i++)
		result = autosave(1);
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

static void keyboardDataReinitialized(void) {
	fprintf(f, "20 keyboard data reinitializations ");
	int before = heapUsed();
	OSL_KEYBOARD *kbd = oslInitOskEx(1, -1);
	int ok = kbd != NULL;
	for (int i = 0; i < 20; i++)
		ok &= oslInitOskDataEx(kbd, 0, NULL, NULL, 64, 1);
	unsigned short *old = oslOskOutTextEx(kbd, 0);
	int failedBefore = allocationFailures;
	failAllocationSize = 65 * sizeof(unsigned short);
	ok &= !oslInitOskDataEx(kbd, 0, NULL, NULL, 64, 1);
	failAllocationSize = 0;
	int preserved = old && oslOskOutTextEx(kbd, 0) == old && allocationFailures == failedBefore + 1;
	oslEndOskEx(kbd);
	int kept = heapUsed() - before;
	fprintf(f, "-> old buffer preserved %d, %d bytes kept %s\n", preserved, kept, ok && preserved && !kept ? "PASS" : "FAIL");
}

static int saveRolledBack(int result) {
	return oslGetSaveLoadType() == OSL_DIALOG_NONE && oslSaveLoadGetResult() == result &&
	       !savedata.dataBuf && !savedata.dataBufSize && !savedata.dataSize;
}

static void saveFailures(void) {
	int allocationPassed = 0, initPassed = 0;
	for (int mode = 0; mode < 3; mode++) {
		struct oslSaveLoad d;
		saveData(&d, mode);
		int before = heapUsed();
		int callsBefore = savedataInitCalls;
		int failedBefore = allocationFailures;
		failAllocationSize = sizeof(data);
		oslInitSaveDialog(&d);
		failAllocationSize = 0;
		allocationPassed += saveRolledBack(-ENOMEM) && heapUsed() == before &&
		                    savedataInitCalls == callsBefore && allocationFailures == failedBefore + 1;
		oslEndSaveLoadDialog();

		before = heapUsed();
		callsBefore = savedataInitCalls;
		failSavedataInit = 1;
		oslInitSaveDialog(&d);
		failSavedataInit = 0;
		initPassed += saveRolledBack(-1234) && heapUsed() == before && savedataInitCalls == callsBefore + 1;
		oslEndSaveLoadDialog();
	}
	fprintf(f, "save malloc failure, 3 modes -> %d/3 rolled back %s\n", allocationPassed, allocationPassed == 3 ? "PASS" : "FAIL");
	fprintf(f, "save SDK init failure, 3 modes -> %d/3 rolled back %s\n", initPassed, initPassed == 3 ? "PASS" : "FAIL");
}

static void saveReplacements(void) {
	fprintf(f, "autosave replacement, no end ");
	int before = heapUsed();
	int result = autosave(0);
	if (!result) result = autosave(1);
	int kept = heapUsed() - before;
	fprintf(f, "-> result %d, %d bytes kept %s\n", result, kept, !result && !kept ? "PASS" : "FAIL");

	fprintf(f, "autosave then malloc failure ");
	before = heapUsed();
	result = autosave(0);
	int rolledBack = 0;
	if (!result) {
		struct oslSaveLoad d;
		saveData(&d, 2);
		int callsBefore = savedataInitCalls;
		int failedBefore = allocationFailures;
		failAllocationSize = sizeof(data);
		oslInitSaveDialog(&d);
		failAllocationSize = 0;
		rolledBack = saveRolledBack(-ENOMEM) && savedataInitCalls == callsBefore && allocationFailures == failedBefore + 1;
		oslEndSaveLoadDialog();
	}
	kept = heapUsed() - before;
	fprintf(f, "-> replacement rolled back %d, %d bytes kept %s\n", rolledBack, kept, rolledBack && !kept ? "PASS" : "FAIL");

	sceIoRemove(SAVEDIR "/DATA.BIN");
	sceIoRemove(SAVEDIR "/PARAM.SFO");
	sceIoRmdir(SAVEDIR);
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	f = fopen("result.txt", "w");
	if (!f) {
		perror("result.txt");
		oslEndGfx();
		sceKernelExitGame();
		return 1;
	}
	setvbuf(f, NULL, _IONBF, 0); // keep what was written if a case crashes
	for (unsigned i = 0; i < sizeof(data); i++) data[i] = i;

	saves();
	keyboardNeverActivated();
	initOskFails();
	initOskWhileAnotherActive();
	keyboardDataReinitialized();
	saveFailures();
	saveReplacements();

	fprintf(f, "DONE\n");
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
