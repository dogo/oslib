#include "oslib.h"
#include "test.h"

#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>

extern volatile int osl_audioActive[];

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

int sceAudioChReserve(int channel, int samplecount, int format) {
	(void)samplecount; (void)format;
	return channel;
}

int sceAudioChRelease(int channel) {
	(void)channel;
	return 0;
}

int sceAudioOutputPannedBlocking(int channel, int leftvol, int rightvol, void *buf) {
	(void)channel; (void)leftvol; (void)rightvol; (void)buf;
	return usleep(2000); // one buffer playing
}

int sceAudioChangeChannelConfig(int channel, int format) {
	(void)channel; (void)format;
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

// WAVs call the end callback from their own decoder, which then went on using the WAV
static void wavDeletedByItsEndCallback(int stream) {
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
	return TEST_RESULT();
}
