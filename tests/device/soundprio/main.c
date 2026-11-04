// With the main thread above the audio threads (0x10), oslPlaySound on a channel whose
// thread is ending (-2) must let that thread finish instead of spinning: the thread
// can't run while main spins, so it hung. The end window is a few us wide, so main
// sweeps its wake-up time across the end of the sound until it lands in it.
#include <pspkernel.h>
#include <oslib/oslib.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("soundprio", 0, 1, 0);
PSP_MAIN_THREAD_PARAMS(0x08, 256, THREAD_ATTR_USER | THREAD_ATTR_VFPU);

extern volatile int osl_audioActive[];

#define VOICE 3

// Ends at the first buffer: the channel is deleted (-1), then ends (-2, 0) once it played
static int endAtOnce(unsigned int voice, void *buf, unsigned int length) {
	(void)voice;
	memset(buf, 0, length * 4);
	return 0;
}

static void nop(OSL_SOUND *s) {
	(void)s;
}

static void waitEnd(void) {
	while (osl_audioActive[VOICE] != 0)
		sceKernelDelayThread(100);
}

int main() {
	oslInit(0);
	oslInitAudio();
	FILE *f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0); // keep what was written if the replay hangs
	OSL_SOUND *s = calloc(1, sizeof(OSL_SOUND));
	s->audioCallback = endAtOnce;
	s->playSound = s->stopSound = s->deleteSound = nop;

	// Where the sound ends: the first wake-up that finds the channel ended
	int end = -1;
	for (int d = 0; d < 30000 && end < 0; d += 100) {
		oslPlaySound(s, VOICE);
		sceKernelDelayThread(d);
		if (osl_audioActive[VOICE] == 0) end = d;
		waitEnd();
	}
	fprintf(f, "sound end found            %s\n", end >= 0 ? "PASS" : "FAIL");

	fprintf(f, "replay of an ending channel ");
	int hits = 0;
	for (int pass = 0; pass < 5 && hits < 3; pass++) {
		for (int d = end > 300 ? end - 300 : 0; d < end + 100 && hits < 3; d++) {
			oslPlaySound(s, VOICE);
			sceKernelDelayThread(d);
			if (osl_audioActive[VOICE] == -2) {
				hits++;
				oslPlaySound(s, VOICE); // spun forever here
			}
			waitEnd();
		}
	}
	fprintf(f, "%s\n", hits ? "PASS" : "FAIL (never caught the channel ending)");
	oslDeleteSound(s);
	fprintf(f, "DONE\n");
	fclose(f);
	sceKernelExitGame();
	return 0;
}
