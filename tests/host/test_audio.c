#include "oslib.h"
#include "test.h"

#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>

extern volatile int osl_audioActive[];

// calloc is osl_test_calloc in every source of this test (CMakeLists.txt): it fails the
// allocations of a channel's buffer (4 KB and up) while failBuffers is set
#undef calloc
void *calloc(size_t n, size_t size); // its <stdlib.h> declaration was renamed too
static volatile int failBuffers;

void *osl_test_calloc(size_t n, size_t size) {
	if (failBuffers && n * size >= 4096) return NULL;
	return calloc(n, size);
}

int (*osl_powerCallback)(int, int, void *);

void oslHandleLoadNoFailError(const char *filename) {
	(void)filename;
}

void oslFatalError(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	abort();
}

OSL_SOUND *oslLoadSoundFileBGM(const char *filename, int stream) {
	(void)filename; (void)stream;
	return NULL;
}

OSL_SOUND *oslLoadSoundFileMP3(const char *filename, int stream) {
	(void)filename; (void)stream;
	return NULL;
}

OSL_SOUND *oslLoadSoundFileAT3(const char *filename, int stream) {
	(void)filename; (void)stream;
	return NULL;
}

// PSP threads on pthreads: ids are slot + 1, the main thread is 0
#define MAX_THREADS 64
static struct {
	SceKernelThreadEntry entry;
	char args[16];
	SceSize arglen;
} threads[MAX_THREADS];
static int nthreads;
static __thread int currentThread;

SceUID sceKernelCreateThread(const char *name, SceKernelThreadEntry entry, int initPriority, int stackSize,
                             unsigned int attr, void *option) {
	(void)name; (void)initPriority; (void)stackSize; (void)attr; (void)option;
	if (nthreads == MAX_THREADS) return -1;
	threads[nthreads].entry = entry;
	return ++nthreads;
}

static void *runThread(void *id) {
	currentThread = (int)(intptr_t)id;
	threads[currentThread - 1].entry(threads[currentThread - 1].arglen, threads[currentThread - 1].args);
	return NULL;
}

int sceKernelStartThread(SceUID thid, SceSize arglen, void *argp) {
	// The PSP copies the arguments to the new thread's stack
	memcpy(threads[thid - 1].args, argp, arglen);
	threads[thid - 1].arglen = arglen;
	pthread_t t;
	if (pthread_create(&t, NULL, runThread, (void *)(intptr_t)thid)) return -1;
	pthread_detach(t);
	return 0;
}

int sceKernelExitDeleteThread(int status) {
	(void)status;
	pthread_exit(NULL);
}

int sceKernelWaitThreadEnd(SceUID thid, unsigned int *timeout) {
	(void)thid; (void)timeout;
	return 0;
}

int sceKernelDeleteThread(SceUID thid) {
	(void)thid;
	return 0;
}

int sceKernelGetThreadId(void) {
	return currentThread;
}

int sceKernelDelayThread(unsigned int delay) {
	return usleep(delay);
}

static pthread_mutex_t interrupts = PTHREAD_MUTEX_INITIALIZER;

unsigned int sceKernelCpuSuspendIntr(void) {
	pthread_mutex_lock(&interrupts);
	return 0;
}

void sceKernelCpuResumeIntr(unsigned int flags) {
	(void)flags;
	pthread_mutex_unlock(&interrupts);
}

int sceAudioChReserve(int channel, int samplecount, int format) {
	(void)samplecount; (void)format;
	return channel;
}

int sceAudioChRelease(int channel) {
	(void)channel;
	return 0;
}

static volatile int loudOutput[8]; // a channel output a non-silent sample

int sceAudioOutputPannedBlocking(int channel, int leftvol, int rightvol, void *buf) {
	(void)leftvol; (void)rightvol;
	int bytes = osl_audioVoices[channel].numSamples * (osl_audioVoices[channel].mono ? 2 : 4);
	for (int i = 0; i < bytes; i++)
		if (((unsigned char *)buf)[i]) loudOutput[channel] = 1;
	return usleep(2000); // one buffer playing
}

// Like the PSP, refuse a reconfiguration while the last buffer plays: busyCalls times
static volatile int dataLen[8], channelFormat[8], busyCalls;

static int busy(void) {
	return busyCalls > 0 && __atomic_sub_fetch(&busyCalls, 1, __ATOMIC_SEQ_CST) >= 0;
}

int sceAudioChangeChannelConfig(int channel, int format) {
	if (busy()) return (int)0x80260002;
	channelFormat[channel] = format;
	return 0;
}

int sceAudioSetChannelDataLen(int channel, int samplecount) {
	if (busy()) return (int)0x80260002;
	dataLen[channel] = samplecount;
	return 0;
}

static volatile int deleted;

// An end callback that deletes its own sound, on the channel's thread
static int deleteSelf(OSL_SOUND *s, int voice) {
	(void)voice;
	oslDeleteSound(s);
	deleted = 1;
	return 0;
}

// Plays s on voice 0 and waits until its channel has ended
static void playToEnd(OSL_SOUND *s) {
	deleted = 0;
	oslPlaySound(s, 0);
	for (int ms = 0; ms < 2000 && osl_audioActive[0]; ms++)
		usleep(1000);
	CHECK(deleted);
	CHECK_EQ(osl_audioActive[0], 0);
}

static int endAtOnce(unsigned int voice, void *buf, unsigned int length) {
	(void)voice;
	memset(buf, 0, length * 4);
	return 0;
}

static void nop(OSL_SOUND *s) {
	(void)s;
}

static void customSoundDeletedByItsEndCallback(void) {
	OSL_SOUND *s = calloc(1, sizeof(OSL_SOUND));
	s->audioCallback = endAtOnce;
	s->playSound = s->stopSound = s->deleteSound = nop;
	s->endCallback = deleteSelf;
	playToEnd(s);
}

static void put16(unsigned char *p, unsigned v) {
	p[0] = v & 0xff;
	p[1] = v >> 8;
}

static void put32(unsigned char *p, unsigned long v) {
	put16(p, v & 0xffff);
	put16(p + 2, v >> 16);
}

// Writes a 44.1 kHz stereo WAV of 100 silent samples and returns its path
static const char *writeTestWav(void) {
	enum { SAMPLES = 100, DATA = SAMPLES * 4 };
	static unsigned char wav[44 + DATA];
	memcpy(wav, "RIFF", 4);
	put32(wav + 4, sizeof(wav) - 8);
	memcpy(wav + 8, "WAVEfmt ", 8);
	put32(wav + 16, 16);
	put16(wav + 20, 1); // PCM
	put16(wav + 22, 2);
	put32(wav + 24, 44100);
	put32(wav + 28, 44100 * 4);
	put16(wav + 32, 4);
	put16(wav + 34, 16);
	memcpy(wav + 36, "data", 4);
	put32(wav + 40, DATA);
	const char *path = "test_audio.wav";
	FILE *f = fopen(path, "wb");
	fwrite(wav, 1, sizeof(wav), f);
	fclose(f);
	return path;
}

// WAVs call the end callback from their own decoder, which then went on using the WAV
static void wavDeletedByItsEndCallback(int stream) {
	const char *path = writeTestWav();
	OSL_SOUND *s = oslLoadSoundFileWAV(path, stream);
	CHECK(s != NULL);
	if (s) {
		s->endCallback = deleteSelf;
		playToEnd(s);
	}
	remove(path);
}

static volatile int inCallback;

// Every buffer ends the sound, slowly, so a stop can land while the callback runs
static int slowEnd(unsigned int voice, void *buf, unsigned int length) {
	(void)voice;
	inCallback = 1;
	memset(buf, 0, length * 4);
	usleep(50 * 1000);
	inCallback = 0;
	return 0;
}

// The loop end callback replays the sound: a stop requested meanwhile must still win
static void loopingSoundStopsDuringItsCallback(void) {
	OSL_SOUND *s = calloc(1, sizeof(OSL_SOUND));
	s->audioCallback = slowEnd;
	s->playSound = s->stopSound = s->deleteSound = nop;
	oslSetSoundLoop(s, 1);
	oslPlaySound(s, 0);
	for (int ms = 0; ms < 1000 && !inCallback; ms++)
		usleep(1000);
	CHECK(inCallback);
	oslStopSound(s);
	CHECK_EQ(osl_audioActive[0], 0);
	oslDeleteSound(s);
}

// The end callback deletes the sound; a stop requested while the decoder runs must win,
// so the stopping thread is the only one to touch the sound afterwards
static void stopWhileSelfDeletingSoundDecodes(void) {
	OSL_SOUND *s = calloc(1, sizeof(OSL_SOUND));
	s->audioCallback = slowEnd;
	s->playSound = s->stopSound = s->deleteSound = nop;
	s->endCallback = deleteSelf;
	deleted = 0;
	oslPlaySound(s, 0);
	for (int ms = 0; ms < 1000 && !inCallback; ms++)
		usleep(1000);
	CHECK(inCallback);
	oslStopSound(s);
	CHECK_EQ(deleted, 0);
	CHECK_EQ(osl_audioActive[0], 0);
	oslDeleteSound(s);
}

// Two sounds whose end callbacks stop each other: the audio threads must not wait for each other
static OSL_SOUND *peer[2];

static int stopPeer(OSL_SOUND *s, int voice) {
	(void)s;
	oslStopSound(peer[!voice]);
	return 0;
}

static void endCallbacksStopEachOther(void) {
	for (int i = 0; i < 2; i++) {
		peer[i] = calloc(1, sizeof(OSL_SOUND));
		peer[i]->audioCallback = slowEnd;
		peer[i]->playSound = peer[i]->stopSound = peer[i]->deleteSound = nop;
		peer[i]->endCallback = stopPeer;
		oslPlaySound(peer[i], i);
	}
	for (int ms = 0; ms < 2000 && (osl_audioActive[0] || osl_audioActive[1]); ms++)
		usleep(1000);
	CHECK_EQ(osl_audioActive[0], 0);
	CHECK_EQ(osl_audioActive[1], 0);
	oslDeleteSound(peer[0]);
	oslDeleteSound(peer[1]);
}

static pthread_mutex_t inputMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t inputReady = PTHREAD_COND_INITIALIZER;
static int inputEntered, inputCancelled, inputFinished, inputDeleted;
static OSL_SOUND *inputSound;

static int waitForInput(unsigned int voice, void *buf, unsigned int length) {
	memset(buf, 0, length * 4);
	pthread_mutex_lock(&inputMutex);
	inputEntered = 1;
	pthread_cond_broadcast(&inputReady);
	while (!inputCancelled)
		pthread_cond_wait(&inputReady, &inputMutex);
	((short *)buf)[0] = *(int *)osl_audioVoices[voice].data;
	inputFinished = 1;
	pthread_mutex_unlock(&inputMutex);
	return 0;
}

static void cancelInput(OSL_SOUND *s) {
	(void)s;
	pthread_mutex_lock(&inputMutex);
	inputCancelled = 1;
	pthread_cond_broadcast(&inputReady);
	pthread_mutex_unlock(&inputMutex);
}

static void deleteInput(OSL_SOUND *s) {
	pthread_mutex_lock(&inputMutex);
	CHECK(inputFinished);
	inputDeleted = 1;
	free(s->data);
	pthread_mutex_unlock(&inputMutex);
}

static void startInputSound(void) {
	inputEntered = inputCancelled = inputFinished = inputDeleted = 0;
	inputSound = calloc(1, sizeof(OSL_SOUND));
	inputSound->data = calloc(1, sizeof(int));
	inputSound->audioCallback = waitForInput;
	inputSound->playSound = nop;
	inputSound->stopSound = cancelInput;
	inputSound->deleteSound = deleteInput;
	oslPlaySound(inputSound, 1);
	pthread_mutex_lock(&inputMutex);
	while (!inputEntered)
		pthread_cond_wait(&inputReady, &inputMutex);
	pthread_mutex_unlock(&inputMutex);
}

// A custom driver's stop hook must cancel its read before waiting for that read to finish.
static void stopCancelsBlockedDriver(void) {
	startInputSound();
	oslStopSound(inputSound);
	CHECK(inputFinished);
	CHECK(!inputDeleted);
	CHECK_EQ(osl_audioActive[1], 0);
	oslDeleteSound(inputSound);
	CHECK(inputDeleted);
}

static void cancelInputAndWait(OSL_SOUND *s) {
	cancelInput(s);
	while (osl_audioActive[1])
		usleep(1000);
}

static int deleteOtherSound(OSL_SOUND *s, int voice) {
	(void)s; (void)voice;
	oslDeleteSound(inputSound);
	pthread_mutex_lock(&inputMutex);
	if (inputFinished) {
		CHECK(inputDeleted); // the channel ended during the stop hook
	} else {
		CHECK(!inputDeleted); // the other channel still owns the blocked read
		inputCancelled = 1;
		pthread_cond_broadcast(&inputReady);
	}
	pthread_mutex_unlock(&inputMutex);
	return 0;
}

// Deletion from another audio thread returns without waiting, but must not free a live driver.
static void endCallbackDeletesOtherSound(int finishDuringStop) {
	startInputSound();
	inputSound->stopSound = finishDuringStop ? cancelInputAndWait : nop;
	OSL_SOUND *s = calloc(1, sizeof(OSL_SOUND));
	s->audioCallback = endAtOnce;
	s->playSound = s->stopSound = s->deleteSound = nop;
	s->endCallback = deleteOtherSound;
	oslPlaySound(s, 0);
	for (int ms = 0; ms < 2000 && (osl_audioActive[0] || osl_audioActive[1]); ms++)
		usleep(1000);
	CHECK_EQ(osl_audioActive[0], 0);
	CHECK_EQ(osl_audioActive[1], 0);
	CHECK(inputFinished);
	CHECK(inputDeleted);
	oslDeleteSound(s);
}

// Sounds identified by userdata[0] that play three buffers and record the length asked for
static volatile int played[4], lengthSeen[4];

static int threeBuffers(unsigned int voice, void *buf, unsigned int length) {
	int id = osl_audioVoices[voice].sound->userdata[0];
	memset(buf, 0, length * (osl_audioVoices[voice].mono ? 2 : 4));
	lengthSeen[id] = length;
	return ++played[id] < 3;
}

static OSL_SOUND *countingSound(int id, int numSamples) {
	OSL_SOUND *s = calloc(1, sizeof(OSL_SOUND));
	s->userdata[0] = id;
	s->numSamples = numSamples;
	s->audioCallback = threeBuffers;
	s->playSound = s->stopSound = s->deleteSound = nop;
	played[id] = lengthSeen[id] = 0;
	return s;
}

static void waitIdle(void) {
	for (int ms = 0; ms < 2000 && (osl_audioActive[0] || osl_audioActive[1]); ms++)
		usleep(1000);
	CHECK_EQ(osl_audioActive[0], 0);
	CHECK_EQ(osl_audioActive[1], 0);
}

// End callbacks that play another sound: next[voice] on voice target[voice], then hold the
// channel thread for holdMs so the main thread can act while that play is pending
static OSL_SOUND *next[2];
static int target[2], holdMs, meet;
static volatile int chained, entered;

static int playNext(OSL_SOUND *s, int voice) {
	(void)s;
	// With meet, both end callbacks are running before either plays
	__atomic_add_fetch(&entered, 1, __ATOMIC_SEQ_CST);
	while (meet && entered < 2)
		usleep(1000);
	oslPlaySound(next[voice], target[voice]);
	chained = 1;
	usleep(holdMs * 1000);
	return 0;
}

static OSL_SOUND *chainingSound(int voice, OSL_SOUND *then, int on) {
	OSL_SOUND *s = calloc(1, sizeof(OSL_SOUND));
	s->audioCallback = endAtOnce;
	s->playSound = s->stopSound = s->deleteSound = nop;
	s->endCallback = playNext;
	next[voice] = then;
	target[voice] = on;
	return s;
}

static void waitChained(void) {
	for (int ms = 0; ms < 1000 && !chained; ms++)
		usleep(1000);
	CHECK(chained);
}

// A playlist: the end callback plays the next sound, in mono with another buffer size, on its
// own voice, while the hardware is still busy with the last buffer
static void endCallbackPlaysNextOnItsVoice(void) {
	holdMs = 0;
	chained = 0;
	OSL_SOUND *b = countingSound(1, 1152);
	b->mono = 0x10;
	OSL_SOUND *a = chainingSound(0, b, 0);
	busyCalls = 3;
	oslPlaySound(a, 0);
	waitIdle();
	CHECK_EQ(played[1], 3);
	CHECK_EQ(lengthSeen[1], 1152);
	CHECK_EQ(dataLen[0], 1152);
	CHECK_EQ(channelFormat[0], 0x10);
	oslDeleteSound(a);
	oslDeleteSound(b);
}

// Two end callbacks running at once each play a sound on the other's voice: neither may wait for
// the other
static void endCallbacksPlayOnEachOthersVoice(void) {
	holdMs = 0;
	meet = 1;
	entered = 0;
	OSL_SOUND *c[2], *s[2];
	for (int i = 0; i < 2; i++) {
		c[i] = countingSound(2 + i, 0);
		s[i] = chainingSound(i, c[i], !i);
		s[i]->audioCallback = slowEnd;
	}
	oslPlaySound(s[0], 0);
	oslPlaySound(s[1], 1);
	waitIdle();
	meet = 0;
	CHECK_EQ(played[2], 3);
	CHECK_EQ(played[3], 3);
	for (int i = 0; i < 2; i++) {
		oslDeleteSound(s[i]);
		oslDeleteSound(c[i]);
	}
}

// While an end callback's play is pending, the main thread stops or deletes the new sound,
// plays a third one on that voice, or stops the old sound
enum { DELETE_NEW, REPLACE, STOP_OLD };

static void mainActsWhilePlayPending(int action) {
	holdMs = 50;
	chained = 0;
	OSL_SOUND *b = countingSound(1, 0);
	OSL_SOUND *a = chainingSound(0, b, 0);
	OSL_SOUND *c = countingSound(2, 0);
	oslPlaySound(a, 0);
	waitChained();
	if (action == DELETE_NEW) {
		oslDeleteSound(b); // a pending play kept would play freed memory
		b = NULL;
	} else if (action == REPLACE) {
		oslPlaySound(c, 0);
	} else {
		oslStopSound(a);
		CHECK(osl_audioVoices[0].sound != a);
	}
	waitIdle();
	CHECK_EQ(played[1], action == STOP_OLD ? 3 : 0);
	CHECK_EQ(played[2], action == REPLACE ? 3 : 0);
	oslDeleteSound(a);
	oslDeleteSound(b);
	oslDeleteSound(c);
}

// A sound that keeps playing, 100 ms per buffer: a stop requested meanwhile stays pending
static int slowPlay(unsigned int voice, void *buf, unsigned int length) {
	(void)voice;
	inCallback = 1;
	memset(buf, 0, length * 4);
	usleep(100 * 1000);
	inCallback = 0;
	return 1;
}

static int keepPlaying(unsigned int voice, void *buf, unsigned int length) {
	(void)voice;
	memset(buf, 0, length * 4);
	return 1;
}

static VIRTUAL_FILE *noFile(OSL_SOUND *s) {
	(void)s;
	return NULL;
}

static OSL_SOUND *stopTarget, *stopper;
static volatile int stopRequested;

// On another channel's thread, oslStopSound only requests the stop
static int stopTargetSound(OSL_SOUND *s, int voice) {
	(void)s; (void)voice;
	oslStopSound(stopTarget);
	stopRequested = 1;
	return 0;
}

static int waitEnd(int voice) {
	for (int ms = 0; ms < 1000 && osl_audioActive[voice]; ms++)
		usleep(1000);
	return osl_audioActive[voice] == 0;
}

// Returns a sound on voice 0 whose stop, requested by an end callback on voice 1, is pending
static OSL_SOUND *stopPending(void) {
	OSL_SOUND *a = calloc(1, sizeof(OSL_SOUND));
	a->audioCallback = slowPlay;
	a->playSound = a->stopSound = a->deleteSound = nop;
	a->standBySound = noFile;
	stopTarget = a;
	stopRequested = 0;
	oslPlaySound(a, 0);
	for (int ms = 0; ms < 1000 && !inCallback; ms++)
		usleep(1000);
	stopper = calloc(1, sizeof(OSL_SOUND));
	stopper->audioCallback = endAtOnce;
	stopper->playSound = stopper->stopSound = stopper->deleteSound = nop;
	stopper->endCallback = stopTargetSound;
	oslPlaySound(stopper, 1);
	for (int ms = 0; ms < 1000 && !stopRequested; ms++)
		usleep(1000);
	CHECK(stopRequested);
	CHECK_EQ(osl_audioActive[0], -1);
	return a;
}

static void endStopPending(OSL_SOUND *a) {
	waitIdle();
	oslDeleteSound(a);
	oslDeleteSound(stopper);
}

// Pausing, resuming or toggling a sound whose stop is pending must not cancel the stop
static void pauseWhileStopPending(int pause) {
	OSL_SOUND *a = stopPending();
	oslPauseSound(a, pause);
	CHECK(waitEnd(0));
	endStopPending(a);
}

// Entering standby must not cancel a pending stop either
static void standbyWhileStopPending(void) {
	OSL_SOUND *a = stopPending();
	a->isStreamed = 1;
	osl_powerCallback(0, PSP_POWER_CB_POWER_SWITCH, NULL);
	CHECK(waitEnd(0));
	osl_powerCallback(0, PSP_POWER_CB_RESUME_COMPLETE, NULL);
	endStopPending(a);
}

static OSL_SOUND *loopingStream(const char *path, int voice) {
	OSL_SOUND *s = oslLoadSoundFileWAV(path, OSL_FMT_STREAM);
	CHECK(s != NULL);
	oslSetSoundLoop(s, 1);
	oslPlaySound(s, voice);
	CHECK_EQ(osl_audioActive[voice], 1);
	return s;
}

// A standby closed the stream's file: its channel's thread reopens it before its next callback
static int waitReopened(OSL_SOUND *s) {
	for (int ms = 0; ms < 1000 && s->suspendNumber != osl_suspendNumber; ms++)
		usleep(1000);
	return s->suspendNumber == osl_suspendNumber;
}

static int waitState(int voice, int state) {
	for (int ms = 0; ms < 1000 && osl_audioActive[voice] != state; ms++)
		usleep(1000);
	return osl_audioActive[voice] == state;
}

// A paused stream stays paused across standby; resumed before VSync reopens its file, it waits
// for VSync instead of reading the file closed by the standby
static void pausedStreamAcrossStandby(int resumeBeforeVSync) {
	const char *path = writeTestWav();
	OSL_SOUND *s = loopingStream(path, 2);
	oslPauseSound(s, 1);
	CHECK_EQ(osl_audioActive[2], 2);
	osl_powerCallback(0, PSP_POWER_CB_POWER_SWITCH, NULL);
	CHECK_EQ(osl_audioActive[2], 2);
	osl_powerCallback(0, PSP_POWER_CB_RESUME_COMPLETE, NULL);
	if (resumeBeforeVSync) {
		oslPauseSound(s, 0);
		CHECK_EQ(osl_audioActive[2], 3);
		oslAudioVSync();
		CHECK(waitReopened(s));
		CHECK(waitState(2, 1));
	} else {
		oslAudioVSync();
		CHECK(waitReopened(s));
		CHECK_EQ(osl_audioActive[2], 2);
		oslPauseSound(s, 0);
		CHECK_EQ(osl_audioActive[2], 1);
	}
	oslDeleteSound(s);
	remove(path);
}

// Toggling a stream suspended by standby pauses it (3 - 3 used to mark the running channel idle)
static void toggleSuspendedStream(void) {
	const char *path = writeTestWav();
	OSL_SOUND *s = loopingStream(path, 2);
	osl_powerCallback(0, PSP_POWER_CB_POWER_SWITCH, NULL);
	CHECK_EQ(osl_audioActive[2], 3);
	osl_powerCallback(0, PSP_POWER_CB_RESUME_COMPLETE, NULL);
	oslPauseSound(s, -1);
	CHECK_EQ(osl_audioActive[2], 2);
	oslAudioVSync();
	CHECK(waitReopened(s));
	CHECK_EQ(osl_audioActive[2], 2);
	oslPauseSound(s, -1);
	CHECK_EQ(osl_audioActive[2], 1);
	oslDeleteSound(s);
	remove(path);
}

static VIRTUAL_FILE *reopened;

// What an audio thread's oslStopSound does while VSync reopens the stream's file
static VIRTUAL_FILE **stopWhileReopening(OSL_SOUND *s, VIRTUAL_FILE *f) {
	(void)s; (void)f;
	oslAudioDeleteChannel(3);
	return &reopened;
}

// A stop requested while VSync reopens a stream's file must not be undone by VSync
static void stopDuringVSyncReopen(void) {
	const char *path = writeTestWav();
	OSL_SOUND *a = calloc(1, sizeof(OSL_SOUND));
	a->audioCallback = keepPlaying;
	a->playSound = a->stopSound = a->deleteSound = nop;
	a->standBySound = noFile;
	a->reactiveSound = stopWhileReopening;
	oslPlaySound(a, 3);
	a->isStreamed = 1;
	strcpy(a->filename, path);
	osl_powerCallback(0, PSP_POWER_CB_POWER_SWITCH, NULL);
	CHECK_EQ(osl_audioActive[3], 3);
	osl_powerCallback(0, PSP_POWER_CB_RESUME_COMPLETE, NULL);
	oslAudioVSync();
	CHECK(waitEnd(3));
	if (reopened) VirtualFileClose(reopened);
	reopened = NULL;
	oslDeleteSound(a);
	remove(path);
}

// A stream played after a standby gets its file reopened, freeing the one the standby closed, and
// plays from its start: not from the position saved for whatever the channel played then (0
// here: the file's header would come out as sound)
static void streamReopenedByPlay(void) {
	extern long osl_filesave[];
	const char *path = writeTestWav();
	OSL_SOUND *s = oslLoadSoundFileWAV(path, OSL_FMT_STREAM);
	CHECK(s != NULL);
	osl_powerCallback(0, PSP_POWER_CB_POWER_SWITCH, NULL);
	osl_powerCallback(0, PSP_POWER_CB_RESUME_COMPLETE, NULL);
	osl_filesave[2] = 0;
	loudOutput[2] = 0;
	oslPlaySound(s, 2);
	CHECK(waitReopened(s));
	CHECK(waitEnd(2));
	CHECK(!loudOutput[2]);
	oslDeleteSound(s);
	remove(path);
}

static pthread_mutex_t readMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t readCond = PTHREAD_COND_INITIALIZER;
static int readEntered, readReleased;
static VIRTUAL_FILE *readSlot;

// A read the standby interrupts: it started on the stream's file and finishes on that same file
static int blockedRead(unsigned int voice, void *buf, unsigned int length) {
	(void)voice;
	memset(buf, 0, length * 4);
	VIRTUAL_FILE *mine = readSlot;
	pthread_mutex_lock(&readMutex);
	readEntered = 1;
	pthread_cond_broadcast(&readCond);
	while (!readReleased)
		pthread_cond_wait(&readCond, &readMutex);
	pthread_mutex_unlock(&readMutex);
	char c;
	VirtualFileRead(&c, 1, 1, mine);
	return 1;
}

static void releaseRead(OSL_SOUND *s) {
	(void)s;
	pthread_mutex_lock(&readMutex);
	readReleased = 1;
	pthread_cond_broadcast(&readCond);
	pthread_mutex_unlock(&readMutex);
}

static VIRTUAL_FILE **readSlotOf(OSL_SOUND *s, VIRTUAL_FILE *f) {
	(void)s; (void)f;
	return &readSlot;
}

static VIRTUAL_FILE *readFile(OSL_SOUND *s) {
	(void)s;
	return readSlot;
}

static void closeReadSlot(OSL_SOUND *s) {
	(void)s;
	VirtualFileClose(readSlot);
	readSlot = NULL;
}

// Replaying a stream whose callback a standby interrupted: its file must not change under that
// callback, and is reopened (the old one freed) once it returns
static void streamReopenedWhileReading(void) {
	const char *path = writeTestWav();
	readSlot = VirtualFileOpen((void *)path, 0, VF_AUTO, VF_O_READ);
	readEntered = readReleased = 0;
	OSL_SOUND *s = calloc(1, sizeof(OSL_SOUND));
	s->isStreamed = 1;
	s->suspendNumber = osl_suspendNumber;
	strcpy(s->filename, path);
	s->audioCallback = blockedRead;
	s->playSound = nop;
	s->stopSound = releaseRead;
	s->deleteSound = closeReadSlot;
	s->standBySound = readFile;
	s->reactiveSound = readSlotOf;
	oslPlaySound(s, 1);
	pthread_mutex_lock(&readMutex);
	while (!readEntered)
		pthread_cond_wait(&readCond, &readMutex);
	pthread_mutex_unlock(&readMutex);
	osl_powerCallback(0, PSP_POWER_CB_POWER_SWITCH, NULL);
	osl_powerCallback(0, PSP_POWER_CB_RESUME_COMPLETE, NULL);
	VIRTUAL_FILE *old = readSlot;
	oslPlaySound(s, 1);
	usleep(20 * 1000);
	CHECK(readSlot == old);
	releaseRead(s);
	CHECK(waitReopened(s));
	CHECK(readSlot != old);
	oslDeleteSound(s);
	CHECK(waitEnd(1));
	remove(path);
}

// A thread that can't allocate its buffer ends its channel, which then plays the next sound
static void threadBufferAllocFails(void) {
	OSL_SOUND *s = countingSound(0, 0);
	failBuffers = 1;
	oslPlaySound(s, 2);
	CHECK(waitEnd(2));
	failBuffers = 0;
	CHECK_EQ(oslGetSoundChannel(s), -1);
	CHECK_EQ(played[0], 0);
	oslPlaySound(s, 2);
	CHECK(waitEnd(2));
	CHECK_EQ(played[0], 3);
	oslDeleteSound(s);
}

// Streams remember their file as an absolute path, to reopen it from any thread (on the PSP,
// threads OSLib creates have no current directory) and after a chdir. A virtual file keeps its
// name, a path whose absolute form doesn't fit (256 bytes) stays as given.
static void streamsRememberAbsolutePaths(void) {
	char home[4096], tmp[] = "/tmp/osltestXXXXXX", expect[4096];
	CHECK(getcwd(home, sizeof(home)) != NULL);
	CHECK(mkdtemp(tmp) != NULL);
	CHECK_EQ(chdir(tmp), 0);
	const char *wav = writeTestWav();
	rename(wav, "a.wav");
	OSL_SOUND *s = oslLoadSoundFileWAV("a.wav", OSL_FMT_STREAM);
	snprintf(expect, sizeof(expect), "%s/a.wav", tmp);
	CHECK(s && !strcmp(s->filename, expect));
	oslDeleteSound(s);

	static unsigned char memWav[44 + 400];
	FILE *in = fopen("a.wav", "rb");
	CHECK_EQ(fread(memWav, 1, sizeof(memWav), in), sizeof(memWav));
	fclose(in);
	OSL_VIRTUALFILENAME v = { "a.wav", memWav, sizeof(memWav), &VF_MEMORY };
	oslAddVirtualFileList(&v, 1);
	s = oslLoadSoundFileWAV("a.wav", OSL_FMT_STREAM);
	CHECK(s && !strcmp(s->filename, "a.wav"));
	oslDeleteSound(s);
	oslRemoveVirtualFileList(&v, 1);

	// Longer than the 64 bytes filename used to have
	char dir[64], name[320];
	memset(dir, 'd', 50);
	dir[50] = 0;
	snprintf(name, sizeof(name), "%s/b.wav", dir);
	mkdir(dir, 0700);
	rename("a.wav", name);
	s = oslLoadSoundFileWAV(name, OSL_FMT_STREAM);
	snprintf(expect, sizeof(expect), "%s/%s", tmp, name);
	CHECK(s && !strcmp(s->filename, expect));

	// The file is reopened by its absolute path after a chdir elsewhere
	oslPlaySound(s, 2);
	osl_powerCallback(0, PSP_POWER_CB_POWER_SWITCH, NULL);
	osl_powerCallback(0, PSP_POWER_CB_RESUME_COMPLETE, NULL);
	CHECK_EQ(chdir(home), 0);
	oslAudioVSync();
	CHECK(waitReopened(s));
	oslDeleteSound(s);
	CHECK_EQ(chdir(tmp), 0);

	// 247 bytes relative, over 256 absolute: kept as given
	char deep[128], deeper[256], longName[320];
	memset(deep, 'e', 120);
	deep[120] = 0;
	snprintf(deeper, sizeof(deeper), "%s/%s", deep, deep);
	mkdir(deep, 0700);
	mkdir(deeper, 0700);
	snprintf(longName, sizeof(longName), "%s/b.wav", deeper);
	rename(name, longName);
	s = oslLoadSoundFileWAV(longName, OSL_FMT_STREAM);
	CHECK(s && !strcmp(s->filename, longName));
	oslDeleteSound(s);
	remove(longName);
	rmdir(deeper);
	rmdir(deep);
	rmdir(dir);
	CHECK_EQ(chdir(home), 0);
	rmdir(tmp);
}

static void hung(int sig) {
	(void)sig;
	static const char msg[] = "test_audio: hung (alarm)\n";
	write(2, msg, sizeof(msg) - 1);
	_exit(1);
}

int main(int argc, char **argv) {
	signal(SIGALRM, hung);
	alarm(10);
	// One case by name (each sanitizer report aborts), or all of them
	const char *only = argc > 1 ? argv[1] : "";
	VirtualFileInit();
	oslInitAudio();
	if (!*only || !strcmp(only, "custom")) customSoundDeletedByItsEndCallback();
	if (!*only || !strcmp(only, "wav")) wavDeletedByItsEndCallback(0);
	if (!*only || !strcmp(only, "wavstream")) wavDeletedByItsEndCallback(OSL_FMT_STREAM);
	if (!*only || !strcmp(only, "loopstop")) loopingSoundStopsDuringItsCallback();
	if (!*only || !strcmp(only, "stopdelete")) stopWhileSelfDeletingSoundDecodes();
	if (!*only || !strcmp(only, "crossstop")) endCallbacksStopEachOther();
	if (!*only || !strcmp(only, "cancel")) stopCancelsBlockedDriver();
	if (!*only || !strcmp(only, "crossdelete")) endCallbackDeletesOtherSound(0);
	if (!*only || !strcmp(only, "crossdeleteended")) endCallbackDeletesOtherSound(1);
	if (!*only || !strcmp(only, "playlist")) endCallbackPlaysNextOnItsVoice();
	if (!*only || !strcmp(only, "crossplay")) endCallbacksPlayOnEachOthersVoice();
	if (!*only || !strcmp(only, "pendingdelete")) mainActsWhilePlayPending(DELETE_NEW);
	if (!*only || !strcmp(only, "pendingreplace")) mainActsWhilePlayPending(REPLACE);
	if (!*only || !strcmp(only, "pendingstopold")) mainActsWhilePlayPending(STOP_OLD);
	if (!*only || !strcmp(only, "pausestopping")) pauseWhileStopPending(1);
	if (!*only || !strcmp(only, "resumestopping")) pauseWhileStopPending(0);
	if (!*only || !strcmp(only, "togglestopping")) pauseWhileStopPending(-1);
	if (!*only || !strcmp(only, "standbystopping")) standbyWhileStopPending();
	if (!*only || !strcmp(only, "pausedstandby")) pausedStreamAcrossStandby(0);
	if (!*only || !strcmp(only, "resumedstandby")) pausedStreamAcrossStandby(1);
	if (!*only || !strcmp(only, "togglesuspended")) toggleSuspendedStream();
	if (!*only || !strcmp(only, "vsyncstop")) stopDuringVSyncReopen();
	if (!*only || !strcmp(only, "bufferfail")) threadBufferAllocFails();
	if (!*only || !strcmp(only, "reopenonplay")) streamReopenedByPlay();
	if (!*only || !strcmp(only, "reopenwhilereading")) streamReopenedWhileReading();
	if (!*only || !strcmp(only, "absolutepath")) streamsRememberAbsolutePaths();
	return TEST_RESULT();
}
