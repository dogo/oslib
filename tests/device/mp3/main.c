// Decodes MP3 files frame by frame on the Media Engine through the sound's own
// audio callback (no playback, so no timing) and checks the PCM it returns.
#include <pspkernel.h>
#include <oslib/oslib.h>
#include <stdio.h>
#include <stdlib.h>

PSP_MODULE_INFO("mp3", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

extern OSL_AUDIO_VOICE osl_audioVoices[];

#define VOICE 7 // never played, so the audio thread leaves it alone
#define MAX_FRAMES 400

static FILE *f;

// A 2 s 440 Hz sine: count frames, peak, frequency and a checksum of the PCM.
// Crossings use +-500 hysteresis and span first..last crossing, so the encoder's
// near-silent lead-in and padding (+-1 noise) don't count.
static void decode(const char *file, int rate, int spf, int wantFrames) {
	OSL_SOUND *s = oslLoadSoundFileMP3(file, OSL_FMT_STREAM);
	if (!s) {
		fprintf(f, "%-10s load failed FAIL\n", file);
		return;
	}
	osl_audioVoices[VOICE].data = s->data;

	static short pcm[1152 * 2] __attribute__((aligned(64)));
	int frames = 0, peak = 0, crossings = 0, state = 0, n = 0, first = -1, last = -1;
	u32 sum = 2166136261u; // FNV-1a
	while (frames < MAX_FRAMES && s->audioCallback(VOICE, pcm, spf)) {
		int i;
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

	int hz = last > first ? (int)((long long)(crossings - 1) * rate / (2LL * (last - first))) : 0;
	int ok = frames >= wantFrames && frames <= wantFrames + 3 && peak > 1000 && hz >= 430 && hz <= 450;
	fprintf(f, "%-10s frames %3d (want %d+) peak %5d %3d Hz sum %08lx %s\n",
	        file, frames, wantFrames, peak, hz, sum, ok ? "PASS" : "FAIL");
}

int main() {
	oslInit(0);
	oslInitAudio();
	oslInitAudioME(OSL_FMT_MP3);
	f = fopen("result.txt", "w");
	// 2 s of audio = rate*2/spf frames, plus the encoder's padding
	decode("tone44.mp3", 44100, 1152, 44100 * 2 / 1152); // MPEG-1, ID3v2 tag in front
	decode("tone48.mp3", 48000, 1152, 48000 * 2 / 1152); // MPEG-1
	decode("tone22.mp3", 22050, 576, 22050 * 2 / 576);   // MPEG-2, mono
	fclose(f);
	sceKernelExitGame();
	return 0;
}
