// Sound lifecycle on the audio threads: missing files fail cleanly, deleting a playing
// sound waits for its thread, and a channel whose thread can't start or allocate
// stays usable. A case that crashes or hangs leaves its line unfinished in result.txt.
#include <pspkernel.h>
#include <pspaudio.h>
#include <oslib/oslib.h>
#include <malloc.h>
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

static volatile int selfDeleted;

static int deleteSelf(OSL_SOUND *s, int voice) {
	(void)voice;
	oslDeleteSound(s);
	selfDeleted = 1;
	return 0;
}

static OSL_SOUND *peer[2];

static int stopPeer(OSL_SOUND *s, int voice) {
	oslStopSound(peer[s == peer[0]]);
	(void)voice;
	return 0;
}

static OSL_SOUND *slowSound(void) {
	OSL_SOUND *s = calloc(1, sizeof(OSL_SOUND));
	s->audioCallback = slowCallback;
	s->playSound = s->stopSound = s->deleteSound = nop;
	return s;
}

// Sounds identified by userdata[0] that play three buffers and record the length asked for
static volatile int buffers[4], lengthAsked[4];

static int threeBuffers(unsigned int voice, void *buf, unsigned int length) {
	int id = osl_audioVoices[voice].sound->userdata[0];
	memset(buf, 0, length * (osl_audioVoices[voice].mono ? 2 : 4));
	lengthAsked[id] = length;
	return ++buffers[id] < 3;
}

static OSL_SOUND *countingSound(int id, int numSamples, int mono) {
	OSL_SOUND *s = calloc(1, sizeof(OSL_SOUND));
	s->userdata[0] = id;
	s->numSamples = numSamples;
	s->mono = mono;
	s->audioCallback = threeBuffers;
	s->playSound = s->stopSound = s->deleteSound = nop;
	s->volumeLeft = s->volumeRight = OSL_VOLUME_MAX;
	return s;
}

// End callbacks that play next[voice & 1] on target[voice & 1]; with meet, both are running
// before either plays
static OSL_SOUND *next[2];
static int target[2], meet;
static volatile int cbEntered;

static int playNext(OSL_SOUND *s, int voice) {
	(void)s;
	cbEntered++;
	for (int ms = 0; meet && cbEntered < 2 && ms < 1000; ms++)
		sceKernelDelayThread(1000);
	oslPlaySound(next[voice & 1], target[voice & 1]);
	return 0;
}

static int waitIdle(int a, int b) {
	for (int ms = 0; ms < 2000 && (osl_audioActive[a] || osl_audioActive[b]); ms++)
		sceKernelDelayThread(1000);
	return !osl_audioActive[a] && !osl_audioActive[b];
}

static OSL_SOUND *stopTarget;
static volatile int stopRequested;

static int endNow(unsigned int voice, void *buf, unsigned int length) {
	(void)voice;
	memset(buf, 0, length * 4);
	return 0;
}

// On another channel's thread, oslStopSound only requests the stop
static int stopTargetSound(OSL_SOUND *s, int voice) {
	(void)s; (void)voice;
	oslStopSound(stopTarget);
	stopRequested = 1;
	return 0;
}

// A standby closed the stream's file: its channel's thread reopens it before its next callback
static int waitReopened(OSL_SOUND *s) {
	for (int ms = 0; ms < 1000 && s->suspendNumber != osl_suspendNumber; ms++)
		sceKernelDelayThread(1000);
	return s->suspendNumber == osl_suspendNumber;
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

	// A thread that can't allocate its buffer is checked on the host (test_audio "bufferfail"):
	// here, once the heap is full and a malloc fails, the next one can crash or hang in the libc
	// (with or without OSLib).

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
	int voice = oslGetSoundChannel(s);
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

	// The end callback would delete the sound, but the stop was requested first: it must win
	fprintf(f, "stop self-deleting sound   ");
	s = slowSound();
	s->audioCallback = slowEnd;
	s->endCallback = deleteSelf;
	selfDeleted = 0;
	oslPlaySound(s, 5);
	entered = waitFor(&inCallback, 1000);
	oslStopSound(s);
	active = osl_audioActive[5];
	fprintf(f, "-> callback entered %d, deleted by it %d, channel %d %s\n", entered, selfDeleted, active,
	        entered && !selfDeleted && !active ? "PASS" : "FAIL");
	if (!selfDeleted) oslDeleteSound(s);

	// Two end callbacks stopping each other's sound: the audio threads must not wait for each other
	fprintf(f, "end callbacks stop each other ");
	for (int i = 0; i < 2; i++) {
		peer[i] = slowSound();
		peer[i]->audioCallback = slowEnd;
		peer[i]->endCallback = stopPeer;
		oslPlaySound(peer[i], 6 + i);
	}
	for (int ms = 0; ms < 2000 && (osl_audioActive[6] || osl_audioActive[7]); ms++)
		sceKernelDelayThread(1000);
	fprintf(f, "-> channels %d %d %s\n", osl_audioActive[6], osl_audioActive[7],
	        !osl_audioActive[6] && !osl_audioActive[7] ? "PASS" : "FAIL");
	oslDeleteSound(peer[0]);
	oslDeleteSound(peer[1]);

	// A playlist: the end callback plays the next sound on its own voice, in mono with MP3's
	// frame size, which the channel must be reconfigured for
	fprintf(f, "end callback plays next on its voice ");
	{
		OSL_SOUND *a = slowSound(), *b = countingSound(0, 1152, PSP_AUDIO_FORMAT_MONO);
		a->audioCallback = slowEnd;
		a->endCallback = playNext;
		meet = 0;
		next[0] = b;
		target[0] = 4;
		oslPlaySound(a, 4);
		int idle = waitIdle(4, 4);
		fprintf(f, "-> played %d, length %d, channel %d %s\n", buffers[0], lengthAsked[0], osl_audioActive[4],
		        idle && buffers[0] == 3 && lengthAsked[0] == 1152 ? "PASS" : "FAIL");
		oslDeleteSound(a);
		oslDeleteSound(b);
	}

	// Two end callbacks running at once each play a sound on the other's voice
	fprintf(f, "end callbacks play on each other's voice ");
	{
		OSL_SOUND *a[2], *c[2];
		meet = 1;
		cbEntered = 0;
		for (int i = 0; i < 2; i++) {
			c[i] = countingSound(2 + i, 0, 0);
			next[i] = c[i];
			target[i] = 4 + !i;
			a[i] = slowSound();
			a[i]->audioCallback = slowEnd;
			a[i]->endCallback = playNext;
		}
		oslPlaySound(a[0], 4);
		oslPlaySound(a[1], 5);
		int idle = waitIdle(4, 5);
		fprintf(f, "-> played %d %d, channels %d %d %s\n", buffers[2], buffers[3], osl_audioActive[4], osl_audioActive[5],
		        idle && buffers[2] == 3 && buffers[3] == 3 ? "PASS" : "FAIL");
		for (int i = 0; i < 2; i++) {
			oslDeleteSound(a[i]);
			oslDeleteSound(c[i]);
		}
	}

	// Deleting a BGM frees its decoder state too
	fprintf(f, "20 BGM loads and deletes ");
	{
		OSL_SOUND *s = oslLoadSoundFileBGM("../soundload/tone.bgm", OSL_FMT_NONE);
		if (s) oslDeleteSound(s);
		int before = mallinfo().uordblks, loaded = 0;
		for (int i = 0; i < 20; i++) {
			s = oslLoadSoundFileBGM("../soundload/tone.bgm", OSL_FMT_NONE);
			if (s) {
				loaded++;
				oslDeleteSound(s);
			}
		}
		int kept = mallinfo().uordblks - before;
		fprintf(f, "-> loaded %d, %d bytes kept %s\n", loaded, kept, loaded == 20 && kept == 0 ? "PASS" : "FAIL");
	}

	// Pausing, resuming or toggling a sound whose stop an end callback requested must let it end
	static const char *const pauseNames[] = { "toggle", "resume", "pause" };
	for (int pause = -1; pause <= 1; pause++) {
		fprintf(f, "%s while stop pending ", pauseNames[pause + 1]);
		OSL_SOUND *a = slowSound(), *b = slowSound();
		b->audioCallback = endNow;
		b->endCallback = stopTargetSound;
		stopTarget = a;
		stopRequested = inCallback = 0;
		oslPlaySound(a, 0);
		waitFor(&inCallback, 1000);
		oslPlaySound(b, 1);
		waitFor(&stopRequested, 1000);
		int pending = osl_audioActive[0];
		oslPauseSound(a, pause);
		int ended = waitIdle(0, 1);
		fprintf(f, "-> stop %s, channel %d %s\n", pending == -1 ? "pending" : "NOT PENDING", osl_audioActive[0],
		        pending == -1 && ended ? "PASS" : "FAIL");
		oslDeleteSound(a);
		oslDeleteSound(b);
	}

	// A standby (its power callback, without suspending): the playing stream waits for its file
	// to be reopened, the paused one stays paused; oslAudioVSync has both reopened and the closed
	// ones freed
	fprintf(f, "standby, playing + paused streams ");
	{
		OSL_SOUND *s[2];
		for (int i = 0; i < 2; i++) {
			s[i] = oslLoadSoundFileWAV("../wavload/tone22m.wav", OSL_FMT_STREAM);
			if (s[i]) oslSetSoundLoop(s[i], 1);
		}
		if (s[0] && s[1]) {
			oslPlaySound(s[0], 2);
			oslPlaySound(s[1], 3);
			oslPauseSound(s[1], 1);
			int before = mallinfo().uordblks;
			osl_powerCallback(0, PSP_POWER_CB_POWER_SWITCH, NULL);
			int during[2] = { osl_audioActive[2], osl_audioActive[3] };
			osl_powerCallback(0, PSP_POWER_CB_RESUME_COMPLETE, NULL);
			oslAudioVSync();
			int reopened = waitReopened(s[0]) && waitReopened(s[1]);
			int after[2] = { osl_audioActive[2], osl_audioActive[3] };
			int kept = mallinfo().uordblks - before;
			fprintf(f, "-> standby %d %d, after %d %d, reopened %d, %d bytes kept %s\n", during[0], during[1], after[0],
			        after[1], reopened, kept,
			        during[0] == 3 && during[1] == 2 && after[0] == 1 && after[1] == 2 && reopened && !kept ? "PASS" : "FAIL");
		} else {
			fprintf(f, "-> load failed FAIL\n");
		}
		for (int i = 0; i < 2; i++)
			if (s[i]) oslDeleteSound(s[i]);
	}

	// A stream that wasn't playing during the standby: oslPlaySound reopens its file and must
	// free the one the standby closed
	fprintf(f, "standby, stream played after ");
	{
		int before = mallinfo().uordblks;
		OSL_SOUND *s = oslLoadSoundFileWAV("../wavload/tone22m.wav", OSL_FMT_STREAM);
		if (s) {
			osl_powerCallback(0, PSP_POWER_CB_POWER_SWITCH, NULL);
			osl_powerCallback(0, PSP_POWER_CB_RESUME_COMPLETE, NULL);
			oslPlaySound(s, 2);
			int reopened = waitReopened(s);
			oslDeleteSound(s);
			int kept = mallinfo().uordblks - before;
			fprintf(f, "-> reopened %d, %d bytes kept %s\n", reopened, kept, reopened && !kept ? "PASS" : "FAIL");
		} else {
			fprintf(f, "-> load failed FAIL\n");
		}
	}

	// An end callback replays a stream after a standby: the file is reopened on that callback's
	// audio thread, which has no current directory, so the stream must remember an absolute path
	fprintf(f, "end callback replays stream after standby ");
	{
		OSL_SOUND *s = oslLoadSoundFileWAV("../wavload/tone22m.wav", OSL_FMT_STREAM);
		if (s) {
			osl_powerCallback(0, PSP_POWER_CB_POWER_SWITCH, NULL);
			osl_powerCallback(0, PSP_POWER_CB_RESUME_COMPLETE, NULL);
			OSL_SOUND *a = slowSound();
			a->audioCallback = endNow;
			a->endCallback = playNext;
			meet = 0;
			next[0] = s;
			target[0] = 2;
			oslPlaySound(a, 0);
			int reopened = waitReopened(s);
			fprintf(f, "-> path %s, reopened %d %s\n", strchr(s->filename, ':') ? "absolute" : "RELATIVE", reopened,
			        reopened ? "PASS" : "FAIL");
			oslDeleteSound(s);
			waitIdle(0, 2);
			oslDeleteSound(a);
		} else {
			fprintf(f, "-> load failed FAIL\n");
		}
	}

	fprintf(f, "DONE\n");
	fclose(f);
	sceKernelExitGame();
	return 0;
}
