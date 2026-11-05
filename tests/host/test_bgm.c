#include "oslib.h"
#include "test.h"

OSL_AUDIO_VOICE osl_audioVoices[OSL_NUM_AUDIO_CHANNELS];

void oslHandleLoadNoFailError(const char *filename) {
	(void)filename;
}

static const char *path = "test_bgm.bgm";

static int format = 1;

// Writes the first len bytes of a 44.1 kHz BGM header followed by data bytes
static void writeBgm(size_t len, size_t data) {
	BGM_FORMAT_HEADER h;
	memset(&h, 0, sizeof(h));
	memcpy(h.strVersion, "OSLBGM v01", 11);
	h.format = format;
	h.sampleRate = 44100;
	h.nbChannels = 1;
	FILE *f = fopen(path, "wb");
	fwrite(&h, 1, len, f);
	for (size_t i = 0; i < data; i++)
		fputc(0x11, f);
	fclose(f);
}

static OSL_SOUND *load(size_t len, size_t data, int stream) {
	writeBgm(len, data);
	return oslLoadSoundFileBGM(path, stream);
}

int main(void) {
	VirtualFileInit();
	for (int stream = 0; stream <= OSL_FMT_STREAM; stream += OSL_FMT_STREAM) {
		// A whole header and data load, with the header's format and rate
		OSL_SOUND *s = load(sizeof(BGM_FORMAT_HEADER), 100, stream);
		CHECK(s != NULL);
		if (s) {
			CHECK_EQ(s->format, 1);
			CHECK_EQ(s->divider, OSL_FMT_44K);
			CHECK_EQ(s->size, 100);
			s->deleteSound(s);
			free(s);
		}
		// Files cut inside the header are rejected: the rest of it was never read
		CHECK(load(12, 0, stream) == NULL);     // just the version string
		CHECK(load(20, 0, stream) == NULL);     // up to the sample rate
		CHECK(load(5, 0, stream) == NULL);      // inside the version string
		CHECK(load(0, 0, stream) == NULL);      // empty
		// Only format 1 (IMA-ADPCM) exists: anything else has no decoder state to play with
		format = 0;
		CHECK(load(sizeof(BGM_FORMAT_HEADER), 100, stream) == NULL);
		format = 1;
	}
	remove(path);
	return TEST_RESULT();
}
