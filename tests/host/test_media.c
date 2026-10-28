#include "oslib.h"
#include "test.h"

#include <pspaudiocodec.h>

// media.c's non-static audio callbacks
int oslAudioCallback_AudioCallback_MP3(unsigned int i, void *buf, unsigned int length);
int oslAudioCallback_AudioCallback_AT3(unsigned int i, void *buf, unsigned int length);

OSL_AUDIO_VOICE osl_audioVoices[OSL_NUM_AUDIO_CHANNELS];
int osl_suspendNumber;

void oslHandleLoadNoFailError(const char *filename) {
	(void)filename;
}

int sceKernelDevkitVersion(void) {
	return 0;
}

SceUID sceKernelLoadModule(const char *path, int flags, SceKernelLMOption *option) {
	(void)path; (void)flags; (void)option;
	return -1;
}

int sceKernelStartModule(SceUID modid, SceSize argsize, void *argp, int *status, void *option) {
	(void)modid; (void)argsize; (void)argp; (void)status; (void)option;
	return -1;
}

int sceUtilityLoadAvModule(int module) {
	(void)module;
	return 0;
}

// Fake Media Engine: records the start and size of every frame handed to the decoder
#define MAX_DECODES 32
static int decodes, edramReleases;
static unsigned char decodedHead[MAX_DECODES][2];
static unsigned long decodedSize[MAX_DECODES];

int sceAudiocodecCheckNeedMem(unsigned long *buf, int type) {
	(void)buf; (void)type;
	return 0;
}

int sceAudiocodecGetEDRAM(unsigned long *buf, int type) {
	(void)buf; (void)type;
	return 0;
}

int sceAudiocodecInit(unsigned long *buf, int type) {
	(void)buf; (void)type;
	return 0;
}

int sceAudiocodecDecode(unsigned long *buf, int type) {
	(void)type;
	if (decodes < MAX_DECODES) {
		memcpy(decodedHead[decodes], (void *)buf[6], 2);
		decodedSize[decodes] = buf[7];
	}
	decodes++;
	return 0;
}

int sceAudiocodecReleaseEDRAM(unsigned long *buf) {
	// The ME reads the codec context on release: it must not be freed yet
	volatile unsigned long touch = buf[0];
	(void)touch;
	edramReleases++;
	return 0;
}

static void writeFile(const char *path, const void *data, size_t size) {
	FILE *f = fopen(path, "wb");
	fwrite(data, 1, size, f);
	fclose(f);
}

static void put16(unsigned char *p, unsigned v) {
	p[0] = v & 0xff;
	p[1] = v >> 8;
}

static void put32(unsigned char *p, unsigned long v) {
	put16(p, v & 0xffff);
	put16(p + 2, v >> 16);
}

#define MP3_FRAME_SIZE 417 // MPEG1 layer III, 128 kbps, 44.1 kHz, no padding
#define MP3_FRAMES 4

static void mp3DecodesEvery44kLayer3Frame(void) {
	static unsigned char mp3[MP3_FRAMES * MP3_FRAME_SIZE + 16];
	size_t len = 0;
	for (int i = 0; i < MP3_FRAMES; i++) {
		if (i == 2) {
			// A layer II header between frames: a sync the decoder must not be fed
			static const unsigned char junk[16] = { 0xff, 0xfd, 0x90, 0x00 };
			memcpy(mp3 + len, junk, sizeof(junk));
			len += sizeof(junk);
		}
		unsigned char *frame = mp3 + len;
		memset(frame, 0, MP3_FRAME_SIZE);
		frame[0] = 0xff;
		frame[1] = 0xfb; // MPEG1, layer III, no CRC
		frame[2] = 0x90; // 128 kbps, 44.1 kHz (samplerate index 0)
		len += MP3_FRAME_SIZE;
	}
	const char *path = "test_media.mp3";
	writeFile(path, mp3, len);

	decodes = edramReleases = 0;
	OSL_SOUND *s = oslLoadSoundFileMP3(path, OSL_FMT_STREAM);
	CHECK(s != NULL);
	if (!s) return;
	CHECK_EQ(s->numSamples, 1152);

	osl_audioVoices[0].data = s->data;
	s->playSound(s);
	static short out[1152 * 2];
	for (int guard = 0; guard < MAX_DECODES && oslAudioCallback_AudioCallback_MP3(0, out, 1152); guard++)
		;

	CHECK_EQ(decodes, MP3_FRAMES);
	for (int i = 0; i < decodes && i < MAX_DECODES; i++) {
		CHECK_EQ(decodedHead[i][0], 0xff);
		CHECK_EQ(decodedHead[i][1], 0xfb);
		CHECK_EQ(decodedSize[i], MP3_FRAME_SIZE);
	}

	s->deleteSound(s);
	CHECK_EQ(edramReleases, 1);
	free(s);
	remove(path);
}

#define AT3_ALIGN 0xC0
#define AT3_BLOCKS 3
#define AT3_DATA_START 48 // RIFF(12) + "fmt "(8 + 20) + "data"(8)

static void at3LifecycleStaysInsideItsInfo(void) {
	static unsigned char at3[AT3_DATA_START + AT3_BLOCKS * AT3_ALIGN];
	memcpy(at3, "RIFF", 4);
	put32(at3 + 4, sizeof(at3) - 8);
	memcpy(at3 + 8, "WAVEfmt ", 8);
	put32(at3 + 16, 20);
	put16(at3 + 20, 0x270); // ATRAC3
	put16(at3 + 22, 2);
	put32(at3 + 24, 44100);
	put16(at3 + 32, AT3_ALIGN);
	memcpy(at3 + 40, "data", 4);
	put32(at3 + 44, AT3_BLOCKS * AT3_ALIGN);
	const char *path = "test_media.at3";
	writeFile(path, at3, sizeof(at3));

	decodes = edramReleases = 0;
	OSL_SOUND *s = oslLoadSoundFileAT3(path, OSL_FMT_STREAM);
	CHECK(s != NULL);
	if (!s) return;
	CHECK_EQ(s->numSamples, 1024);

	osl_audioVoices[0].data = s->data;
	s->playSound(s);
	static short out[1024 * 2];
	CHECK(oslAudioCallback_AudioCallback_AT3(0, out, 1024));
	CHECK_EQ(decodes, 1);

	s->stopSound(s);
	CHECK_EQ(VirtualFileTell(s->standBySound(s)), AT3_DATA_START);

	s->deleteSound(s);
	CHECK_EQ(edramReleases, 1);
	free(s);
	remove(path);
}

int main(void) {
	VirtualFileInit();
	mp3DecodesEvery44kLayer3Frame();
	at3LifecycleStaysInsideItsInfo();
	return TEST_RESULT();
}
