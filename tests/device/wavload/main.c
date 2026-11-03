// Device test: WAV sounds decode to the file's own samples, loaded whole or
// streamed, from the memory stick or from memory (ram:/, which the docs call
// untested). 22050 Hz doubles each sample (OSL_FMT_22K); rates the loader has
// no divider for play at another pitch, measured here: 48 kHz is treated as
// 44100 Hz, 32 kHz as 22050 Hz.
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("wavload", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

extern OSL_AUDIO_VOICE osl_audioVoices[];

#define VOICE 7 // never played, so the audio thread leaves it alone
#define CHUNK 512 // output frames per callback, even for the 2x divider

static FILE *f;
static short src[12000 * 2], out[11025 * 2];

// The file's PCM (after the "data" chunk header), as shorts
static int readSource(const char *file) {
	static unsigned char buf[48 * 1024];
	FILE *in = fopen(file, "rb");
	int n = in ? fread(buf, 1, sizeof(buf), in) : 0;
	if (in) fclose(in);
	for (int i = 12; i + 8 <= n; i++)
		if (!memcmp(buf + i, "data", 4)) {
			int size = buf[i + 4] | buf[i + 5] << 8 | buf[i + 6] << 16 | buf[i + 7] << 24;
			memcpy(src, buf + i + 8, size);
			return size / 2;
		}
	return 0;
}

// Decodes `frames` 44.1 kHz output frames; returns the number of shorts written
static int decode(const char *path, int stream, int frames) {
	OSL_SOUND *s = oslLoadSoundFileWAV(path, stream);
	if (!s) return -1;
	osl_audioVoices[VOICE].dataplus = s->dataplus;
	osl_audioVoices[VOICE].divider = s->divider;
	osl_audioVoices[VOICE].mono = s->mono;
	int perFrame = s->mono ? 1 : 2, n = 0;
	for (int done = 0; done < frames; done += CHUNK) {
		int len = frames - done < CHUNK ? frames - done : CHUNK;
		s->audioCallback(VOICE, out + n, len);
		n += len * perFrame;
	}
	osl_audioVoices[VOICE].dataplus = NULL;
	oslDeleteSound(s);
	return n;
}

static void exact(const char *file, int frames, int dup) {
	int have = readSource(file);
	static const char *how[] = { "file", "file stream", "ram", "ram stream" };
	static unsigned char mem[48 * 1024];
	char ram[32];
	snprintf(ram, sizeof(ram), "ram:/%s", file);
	FILE *in = fopen(file, "rb");
	int size = in ? fread(mem, 1, sizeof(mem), in) : 0;
	if (in) fclose(in);
	OSL_VIRTUALFILENAME vf = { ram, mem, size, &VF_MEMORY };
	oslAddVirtualFileList(&vf, 1);
	for (int i = 0; i < 4; i++) {
		int n = decode(i < 2 ? file : ram, i & 1, frames), bad = 0;
		for (int k = 0; k < n; k++)
			bad += out[k] != src[dup ? k / 2 : k];
		fprintf(f, "%-11s %-11s %5d samples, %5d differ from the file (%d in it) %s\n", file, how[i], n, bad, have,
		        n > 0 && !bad ? "PASS" : "FAIL");
	}
	oslRemoveVirtualFileList(&vf, 1);
}

static void pitch(const char *file, int rate, int divided) {
	int n = decode(file, 0, 11024), crossings = 0, state = 0, first = -1, last = -1;
	for (int i = 0; i < n / 2; i++) { // left channel
		int l = out[i * 2];
		if ((state >= 0 && l < -500) || (state <= 0 && l > 500)) {
			if (state != 0) {
				crossings++;
				if (first < 0) first = i;
				last = i;
			}
			state = l > 500 ? 1 : -1;
		}
	}
	int hz = last > first ? (int)((long long)(crossings - 1) * 44100 / (2LL * (last - first))) : 0;
	// played as 44100 Hz (no divider) or 22050 Hz (each sample twice)
	int want = (int)(440.f * (divided ? 22050 : 44100) / rate + 0.5f);
	fprintf(f, "%-11s %5d Hz plays a 440 Hz tone at %3d Hz (want %3d) %s\n", file, rate, hz, want,
	        abs(hz - want) <= 3 ? "PASS" : "FAIL");
}

int main() {
	oslInit(0);
	oslInitAudio();
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0);
	// One frame short of the end: reaching it calls the voice's sound->endCallback, and no sound plays here
	exact("tone44s.wav", 11024, 0);
	exact("tone22m.wav", 11024, 1); // mono: one sample per output frame, each source sample twice
	pitch("tone48s.wav", 48000, 0);
	pitch("tone32s.wav", 32000, 1);
	fclose(f);
	sceKernelExitGame();
	return 0;
}
