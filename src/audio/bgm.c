#include "oslib.h"

// ADPCM Globals Structure
typedef struct ADGlobals {
	const unsigned char *data;
	int last_sample;
	int last_index;
} OSL_ADGlobals;

// Initializes ADPCM playback
void oslStartAD(OSL_ADGlobals *ad, const unsigned char *data) {
	ad->data = data;
	ad->last_sample = 0;
	ad->last_index = 0;
}

// Resets ADPCM playback with new data
void oslRepriseAD(OSL_ADGlobals *ad, const unsigned char *data) {
	ad->data = data;
}

// Step indices for ADPCM decoding
static const signed char ima9_step_indices[16] = {
	-1, -1, -1, -1, 2, 4, 7, 12,
	-1, -1, -1, -1, 2, 4, 7, 12
};

// Step table for ADPCM decoding
const unsigned short ima_step_table[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

// Rescales ADPCM code based on step
static inline int ima9_rescale(int step, unsigned int code) {
	int diff = step >> 3;
	if (code & 1) {
		diff += step >> 2;
	}
	if (code & 2) {
		diff += step >> 1;
	}
	if (code & 4) {
		diff += step;
	}
	if ((code & 7) == 7) {
		diff += step >> 1;
	}
	if (code & 8) {
		diff = -diff;
	}
	return diff;
}

// Decodes a mono ADPCM audio stream
short *oslDecodeADMono(OSL_ADGlobals *ad, short *dst, const unsigned char *src, unsigned int len, unsigned int samples, unsigned int streaming) {
	int last_sample = ad->last_sample;
	int index = ad->last_index;
	unsigned int i, byte = 0;
	unsigned char *streambuffer = NULL;

	// Allocate stream buffer if streaming
	if (streaming) {
		streambuffer = (unsigned char *)alloca(len >> 1);
		VirtualFileRead(streambuffer, len >> 1, 1, (VIRTUAL_FILE *)src);
	}

	while (len > 0) {
		int step = ima_step_table[index];
		int diff;
		unsigned int code;

		// Bounds check for index
		index = index < 0 ? 0 : (index > 88 ? 88 : index);

		// Handle odd or even bytes
		if (len & 1) {
			code = byte >> 4;
		} else {
			byte = streaming ? *streambuffer++ : *src++;
			code = byte & 0x0f;
		}

		diff = ima9_rescale(step, code);
		index += ima9_step_indices[code & 0x07];
		last_sample += diff;

		// Clamp sample value to valid range
		last_sample = last_sample < -32768 ? -32768 : (last_sample > 32767 ? 32767 : last_sample);

		// Output the sample to the destination buffer
		for (i = 0; i < samples; i++) {
			*dst++ += last_sample;
		}

		len--;
	}

	// Update ADPCM state
	ad->last_index = index;
	ad->last_sample = last_sample;
	ad->data = src;
	return dst;
}

// Standard Callbacks for Audio

// Stops BGM playback and resets the file pointer
void oslAudioCallback_StopSound_BGM(OSL_SOUND *s) {
	if (s->isStreamed)
		VirtualFileSeek((VIRTUAL_FILE *)s->data, s->baseoffset, SEEK_SET);
}

// Starts BGM playback and resets ADPCM state
void oslAudioCallback_PlaySound_BGM(OSL_SOUND *s) {
	oslAudioCallback_StopSound_BGM(s);
	oslStartAD((OSL_ADGlobals *)s->dataplus, (unsigned char *)s->data);
}

// Main audio callback function
int oslAudioCallback_AudioCallback_BGM(unsigned int i, void *buf, unsigned int length) {
	void *buf2;
	unsigned int l = 0;
	OSL_ADGlobals *ad = (OSL_ADGlobals *)osl_audioVoices[i].dataplus;

	// Clear the buffer
	memset(buf, 0, length << 2);

	// Check if size is valid
	if (osl_audioVoices[i].size <= 0)
		return 1;

	// Calculate the length based on the divider
	l = length >> (osl_audioVoices[i].divider + 1);
	if (l > osl_audioVoices[i].size)
		l = osl_audioVoices[i].size;

	// Decode the audio
	buf2 = oslDecodeADMono(ad, (short *)buf, ad->data, l << 1, 1 << (osl_audioVoices[i].divider), osl_audioVoices[i].isStreamed);
	osl_audioVoices[i].size -= l;

	// Check if playback has finished
	if (osl_audioVoices[i].size <= 0 && l) {
		memset(buf2, 0, (u32)buf + (length << 1) - (u32)buf2);
		return 0;
	}
	return 1;
}

// Reactivates BGM playback
VIRTUAL_FILE **oslAudioCallback_ReactiveSound_BGM(OSL_SOUND *s, VIRTUAL_FILE *f) {
	VIRTUAL_FILE **w = (VIRTUAL_FILE **)&s->data;
	oslRepriseAD((OSL_ADGlobals *)s->dataplus, (unsigned char *)f);
	return w;
}

// Returns the file pointer for BGM
VIRTUAL_FILE *oslAudioCallback_StandBy_BGM(OSL_SOUND *s) {
	return (VIRTUAL_FILE *)s->data;
}

// Deletes BGM sound data
void oslAudioCallback_DeleteSound_BGM(OSL_SOUND *s) {
	if (s->isStreamed) {
		VirtualFileClose((VIRTUAL_FILE *)s->data);
	} else {
		free(s->data);
	}
}

// Loads a BGM sound file and initializes it
OSL_SOUND *oslLoadSoundFileBGM(const char *filename, int stream) {
	VIRTUAL_FILE *f;
	int start_offset, end_offset;
	OSL_SOUND *s;
	OSL_ADGlobals *ad = NULL;
	BGM_FORMAT_HEADER bfh;

	// Allocate memory for the sound object
	s = (OSL_SOUND *)malloc(sizeof(OSL_SOUND));
	if (!s) goto error;
	memset(s, 0, sizeof(OSL_SOUND));  // Ensure all fields are zeroed

	f = VirtualFileOpen((void *)filename, 0, VF_AUTO, VF_O_READ);
	if (!f) goto cleanup_and_error;

	// Read the format header
	VirtualFileRead(&bfh, sizeof(bfh), 1, f);
	if (strcmp(bfh.strVersion, "OSLBGM v01")) goto cleanup_and_error;

	// If the format is ADPCM, initialize ADPCM data structure
	if (bfh.format == 1) {
		ad = (OSL_ADGlobals *)malloc(sizeof(OSL_ADGlobals));
		if (!ad) goto cleanup_and_error;
		s->dataplus = ad;
	}

	// Get the file size and load data
	start_offset = VirtualFileTell(f);
	s->baseoffset = start_offset;
	VirtualFileSeek(f, 0, SEEK_END);
	end_offset = VirtualFileTell(f);
	if (end_offset - start_offset <= 0) goto cleanup_and_error;

	// Allocate memory for the sound data if not streamed
	VirtualFileSeek(f, 0, SEEK_SET);
	s->isStreamed = stream;
	if (s->isStreamed) {
		if (strlen(filename) < sizeof(s->filename))
			strcpy(s->filename, filename);
		s->data = (void *)f;
	} else {
		s->data = malloc(end_offset - start_offset);
		if (!s->data) goto cleanup_and_error;
		VirtualFileRead(s->data, end_offset - start_offset, 1, f);
		VirtualFileClose(f);
	}

	// Set additional properties
	s->endCallback = NULL;
	s->numSamples = 0;
	s->format = bfh.format;
	s->divider = (bfh.sampleRate == 44100) ? OSL_FMT_44K :
				 (bfh.sampleRate == 22050) ? OSL_FMT_22K : OSL_FMT_11K;
	s->size = end_offset - start_offset;
	s->mono = 0x10;  // Mono audio format
	s->volumeLeft = s->volumeRight = OSL_VOLUME_MAX;

	// Set the callback functions
	s->audioCallback = oslAudioCallback_AudioCallback_BGM;
	s->playSound = oslAudioCallback_PlaySound_BGM;
	s->stopSound = oslAudioCallback_StopSound_BGM;
	s->standBySound = oslAudioCallback_StandBy_BGM;
	s->reactiveSound = oslAudioCallback_ReactiveSound_BGM;
	s->deleteSound = oslAudioCallback_DeleteSound_BGM;

	return s;

cleanup_and_error:
	free(s);
	if (ad) free(ad);
	VirtualFileClose(f);

error:
	oslHandleLoadNoFailError(filename);
	return NULL;
}
