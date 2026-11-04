// Sound lifecycle on the audio threads: missing files fail cleanly, deleting a playing
// sound waits for its thread, and a channel whose thread can't start or allocate
// stays usable. A case that crashes or hangs leaves its line unfinished in result.txt.
#include <pspkernel.h>
#include <oslib/oslib.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("soundlife", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
// A fixed heap: exhausting it leaves the partition memory thread stacks come from
PSP_HEAP_SIZE_KB(4096);

extern volatile int osl_audioActive[];

static FILE *f;
static volatile int inCallback, calls;

// A silent sound whose callback takes 100 ms, so the main thread can act while it runs
static int slowCallback(unsigned int voice, void *buf, unsigned int length) {
	(void)voice;
	inCallback = 1;
	calls++;
	memset(buf, 0, length * 4);
	sceKernelDelayThread(100 * 1000);
	inCallback = 0;
	return 1;
}

// Like slowCallback, but every buffer ends the sound: with a loop end callback it replays
static int slowEnd(unsigned int voice, void *buf, unsigned int length) {
	slowCallback(voice, buf, length);
	return 0;
}

static void nop(OSL_SOUND *s) {
	(void)s;
}

static OSL_SOUND *slowSound(void) {
	OSL_SOUND *s = calloc(1, sizeof(OSL_SOUND));
	s->audioCallback = slowCallback;
	s->playSound = s->stopSound = s->deleteSound = nop;
	return s;
}

static int waitFor(volatile int *flag, int ms) {
	for (; ms > 0 && !*flag; ms--)
		sceKernelDelayThread(1000);
	return *flag;
}

static void missing(const char *what, OSL_SOUND *(*load)(const char *, int), const char *file, int stream) {
	fprintf(f, "%-26s ", what);
	OSL_SOUND *s = load(file, stream);
	fprintf(f, "-> %s %s\n", s ? "sound" : "NULL", s ? "FAIL" : "PASS");
	if (s) oslDeleteSound(s);
}

int main() {
	oslInit(0);
	oslInitAudio();
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0); // keep what was written if a case below crashes or hangs

	missing("missing WAV, RAM", oslLoadSoundFileWAV, "missing.wav", 0);
	missing("missing WAV, stream", oslLoadSoundFileWAV, "missing.wav", OSL_FMT_STREAM);
	missing("missing BGM, RAM", oslLoadSoundFileBGM, "missing.bgm", 0);
	missing("missing BGM, stream", oslLoadSoundFileBGM, "missing.bgm", OSL_FMT_STREAM);

	// oslDeleteSound frees the sound: its thread must be done with it by then
	fprintf(f, "delete during callback     ");
	OSL_SOUND *s = slowSound();
	oslPlaySound(s, 0);
	int entered = waitFor(&inCallback, 1000);
	oslDeleteSound(s);
	int still = inCallback, active = osl_audioActive[0];
	fprintf(f, "-> callback entered %d, still running %d, channel %d %s\n", entered, still, active,
	        entered && !still && !active ? "PASS" : "FAIL");

	// The thread can't allocate its buffer: it must still end the channel
	fprintf(f, "thread buffer alloc fails  ");
	void *hog = NULL, *p;
	while ((p = malloc(1024))) { // chain the blocks through their first word
		*(void **)p = hog;
		hog = p;
	}
	s = slowSound();
	oslPlaySound(s, 2);
	sceKernelDelayThread(200 * 1000);
	active = osl_audioActive[2];
	int voice = oslGetSoundChannel(s);
	while (hog) {
		p = *(void **)hog;
		free(hog);
		hog = p;
	}
	fprintf(f, "-> channel %d, voice %d %s\n", active, voice, !active && voice < 0 ? "PASS" : "FAIL");
	oslDeleteSound(s);

	// No memory for the thread's stack: the channel must not stay active without a thread
	fprintf(f, "thread creation fails      ");
	SceUID blocks[64];
	int nblocks = 0;
	// The largest free block is taken until none can hold the 64 KB stack
	while (nblocks < 64 && sceKernelMaxFreeMemSize() >= 64 * 1024) {
		blocks[nblocks] = sceKernelAllocPartitionMemory(2, "hog", PSP_SMEM_Low, sceKernelMaxFreeMemSize(), NULL);
		if (blocks[nblocks] < 0) break;
		nblocks++;
	}
	int starved = sceKernelMaxFreeMemSize() < 64 * 1024;
	s = slowSound();
	oslPlaySound(s, 1);
	active = osl_audioActive[1];
	voice = oslGetSoundChannel(s);
	while (nblocks > 0)
		sceKernelFreePartitionMemory(blocks[--nblocks]);
	fprintf(f, "-> stack memory %s, channel %d, voice %d %s\n", starved ? "exhausted" : "LEFT", active, voice,
	        starved && !active && voice < 0 ? "PASS" : "FAIL");

	// The same channel then plays another sound (it hung waiting for the missing thread)
	fprintf(f, "channel plays again        ");
	OSL_SOUND *s2 = slowSound();
	calls = 0;
	oslPlaySound(s2, 1);
	int played = waitFor(&calls, 1000);
	fprintf(f, "-> callback called %s\n", played ? "PASS" : "FAIL");
	oslDeleteSound(s2);
	oslDeleteSound(s);

	// The loop's end callback replays the sound while oslStopSound waits for the channel
	fprintf(f, "stop looping sound         ");
	s = slowSound();
	s->audioCallback = slowEnd;
	oslSetSoundLoop(s, 1);
	oslPlaySound(s, 4);
	entered = waitFor(&inCallback, 1000);
	oslStopSound(s);
	active = osl_audioActive[4];
	fprintf(f, "-> callback entered %d, channel %d %s\n", entered, active, entered && !active ? "PASS" : "FAIL");
	oslDeleteSound(s);

	fprintf(f, "DONE\n");
	fclose(f);
	sceKernelExitGame();
	return 0;
}
