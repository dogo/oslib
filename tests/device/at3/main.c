// Decodes ATRAC3 and ATRAC3plus files frame by frame on the Media Engine
// through the sound's own audio callback (no playback, so no timing) and
// checks the PCM it returns. The files come from gen.sh: a 2 s 440 Hz sine.
#include <pspkernel.h>
#include <oslib/oslib.h>
#include <stdio.h>
#include <stdlib.h>

PSP_MODULE_INFO("at3", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

extern OSL_AUDIO_VOICE osl_audioVoices[];

#define VOICE 7 // never played, so the audio thread leaves it alone
#define MAX_FRAMES 400

static FILE *f;

// Frames must be exactly data size / block align. Frequency uses +-500
// hysteresis between the first and last crossing, like tests/device/mp3.
static void decode(const char *file, int spf, int wantFrames) {
	OSL_SOUND *s = oslLoadSoundFileAT3(file, OSL_FMT_STREAM);
	if (!s) {
		fprintf(f, "%-9s load failed FAIL\n", file);
		return;
	}
	osl_audioVoices[VOICE].data = s->data;

	static short pcm[2048 * 2] __attribute__((aligned(64)));
	int frames = 0, peak = 0, crossings = 0, state = 0, n = 0, first = -1, last = -1, i;
	u32 sum = 2166136261u; // FNV-1a
	while (frames < MAX_FRAMES && s->audioCallback(VOICE, pcm, spf)) {
		for (i = 0; i < spf; i++) {
			int l = pcm[i * 2];
			if (abs(l) > peak) peak = abs(l);
			if ((state >= 0 && l < -500) || (state <= 0 && l > 500)) {
				if (state != 0) {
					crossings++;
					if (first < 0) first = n;
					last = n;
				}
				state = l > 500 ? 1 : -1;
			}
			n++;
		}
		const u8 *b = (const u8 *)pcm;
		for (i = 0; i < spf * 4; i++) sum = (sum ^ b[i]) * 16777619u;
		frames++;
	}
	osl_audioVoices[VOICE].data = NULL;
	oslDeleteSound(s);

	int hz = last > first ? (int)((long long)(crossings - 1) * 44100 / (2LL * (last - first))) : 0;
	int ok = frames == wantFrames && peak > 1000 && hz >= 430 && hz <= 450;
	fprintf(f, "%-9s frames %2d (want %d) peak %5d %3d Hz sum %08lx %s\n",
	        file, frames, wantFrames, peak, hz, sum, ok ? "PASS" : "FAIL");
}

int main() {
	oslInit(0);
	oslInitAudio();
	oslInitAudioME(OSL_FMT_AT3);
	f = fopen("result.txt", "w");
	decode("lp2.at3", 1024, 87);  // ATRAC3 132 kbps, align 384
	decode("lp3.at3", 1024, 87);  // ATRAC3 105 kbps, align 304
	decode("lp4.at3", 1024, 87);  // ATRAC3 66 kbps, align 192, joint stereo
	decode("plus.at3", 2048, 44); // ATRAC3plus 352 kbps, align 2048
	fclose(f);
	sceKernelExitGame();
	return 0;
}
