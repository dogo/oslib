#include "oslib.h"
#include <pspaudiocodec.h>
#include <pspsdk.h>
#include <pspmpeg.h>

//
// Definitions
//

// ATRAC3 and ATRAC3plus Definitions
#define TYPE_ATRAC3 0x270
#define TYPE_ATRAC3PLUS 0xFFFE

// Audio Sample Frame Sizes
#define SAMPLE_PER_FRAME_ATRAC3 1024
#define SAMPLE_PER_FRAME_ATRAC3PLUS 2048
#define SAMPLE_PER_FRAME_MP3 1152

// ATRAC3 and ATRAC3plus Buffer and Header Definitions
#define AUDIO_ALIGN_ATRAC3 0xC0
#define AUDIO_ALIGN_ATRAC3PLUS 0x130
#define ATRAC3_BUFFER_SIZE 0x180
#define ATRAC3PLUS_HEADER_SIZE 8
#define ATRAC3PLUS_HEADER_0 0x0F
#define ATRAC3PLUS_HEADER_1 0xD0

// ATRAC3 and ATRAC3plus Decode Types
#define ATRAC3_DECODE_TYPE 0x1001
#define ATRAC3PLUS_DECODE_TYPE 0x1000

// MP3 Specific Definitions
#define MP3_HEADER_SIZE 4
#define MP3_BUFFER_SIZE 2889
#define MPEG1_VERSION 3
#define DECODE_TYPE_MP3 0x1002

// General Audio and File Definitions
#define INVALID_FRAME -1
#define RIFF_HEADER_SIZE 8
#define WAVEFMT_HEADER_SIZE 12
#define CODEC_BUFFER_SIZE 65

//
// Data Structures
//

typedef struct {
    VIRTUAL_FILE *handle;
    unsigned long *codecBuffer;
    u8 *dataBuffer;
    u32 sample_per_frame;
    u32 data_start_init;

    // Codec-specific
    u16 at3_type;
    u16 at3_data_align;
    u16 at3_channel_mode;
    u8 at3_at3plus_flagdata[2];
    u32 at3_data_size;
    u8 at3_getEDRAM;
    u32 at3_channels;
    u32 at3_samplerate;
} AT3_INFO;

typedef struct {
    VIRTUAL_FILE *handle;
    unsigned long *codecBuffer;
    unsigned char dataBuffer[MP3_BUFFER_SIZE] __attribute__((aligned(64)));
    u32 sample_per_frame;
    u32 data_start_init;

    // Codec-specific
    u32 channels;
    u32 samplerate;
    u32 data_start;
    u8 getEDRAM;
} MP3_INFO;

//
// Static Variables and Constants
//

static int samplerates[4][3] = {
    {11025, 12000, 8000},   // mpeg 2.5
    {0, 0, 0},              // reserved
    {22050, 24000, 16000},  // mpeg 2
    {44100, 48000, 32000}   // mpeg 1
};

static int bitrates[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};
static int bitrates_v2[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160};

static int osl_at3Inited = 0, osl_mp3Inited = 0;

//
// Audio Module Loading
//

SceUID LoadStartAudioModule(char *modname, int partition) {
    SceKernelLMOption option;
    SceUID modid;

    memset(&option, 0, sizeof(option));
    option.size = sizeof(option);
    option.mpidtext = partition;
    option.mpiddata = partition;
    option.position = 0;
    option.access = 1;

    modid = sceKernelLoadModule(modname, 0, &option);
    if (modid < 0) {
        return modid;
    }

    return sceKernelStartModule(modid, 0, NULL, NULL, NULL);
}

void initME() {
    if (sceKernelDevkitVersion() == 0x01050001) {
        LoadStartAudioModule("flash0:/kd/me_for_vsh.prx", PSP_MEMORY_PARTITION_KERNEL);
        LoadStartAudioModule("flash0:/kd/audiocodec.prx", PSP_MEMORY_PARTITION_KERNEL);
    } else {
        sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC);
    }
}

//
// ID3 Tag and MP3 Frame Handling
//

int GetID3TagSize(char *fname) {
    SceUID fd;
    char header[10];
    int size = 0;

    fd = sceIoOpen(fname, PSP_O_RDONLY, 0777);
    if (fd < 0) {
        return 0;
    }

    sceIoRead(fd, header, sizeof(header));
    sceIoClose(fd);

    if (!strncmp(header, "ea3", 3) || !strncmp(header, "EA3", 3) || !strncmp(header, "ID3", 3)) {
        size = header[6];
        size = (size << 7) | header[7];
        size = (size << 7) | header[8];
        size = (size << 7) | header[9];
        size += 10;
        if (header[5] & 0x10) {
            size += 10;
        }
    }
    return size;
}

int SeekNextFrameMP3(VIRTUAL_FILE *fd) {
    int offset = 0;
    unsigned char buf[1024];
    unsigned char *pBuffer;
    int size = 0;
    int i;

    VirtualFileSeek(fd, 0, PSP_SEEK_CUR);
    offset = VirtualFileTell(fd);
    VirtualFileRead(buf, sizeof(buf), 1, fd);

    if (!strncmp((char *)buf, "ID3", 3) || !strncmp((char *)buf, "ea3", 3)) {
        size = buf[6];
        size = (size << 7) | buf[7];
        size = (size << 7) | buf[8];
        size = (size << 7) | buf[9];
        size += 10;
        if (buf[5] & 0x10) {
            size += 10;
        }
    }

    VirtualFileSeek(fd, offset, PSP_SEEK_SET); // now seek for a sync

    while (1) {
        VirtualFileSeek(fd, 0, PSP_SEEK_CUR);
        offset = VirtualFileTell(fd);
        size = VirtualFileRead(buf, sizeof(buf), 1, fd);
        if (size <= 2) return INVALID_FRAME;

        if (!strncmp((char *)buf, "EA3", 3)) { // oma mp3 files have non-safe ints in the EA3 header
            VirtualFileSeek(fd, (buf[4] << 8) + buf[5], PSP_SEEK_CUR);
            continue;
        }

        pBuffer = buf;
        for (i = 0; i < size; i++) {
            // if this is a valid frame sync (0xe0 is for mpeg version 2.5,2+1)
            if ((pBuffer[i] == 0xff) && ((pBuffer[i + 1] & 0xE0) == 0xE0)) {
                offset += i;
                VirtualFileSeek(fd, offset, PSP_SEEK_SET);
                return offset;
            }
        }
        // go back two bytes to catch any syncs that on the boundary
        VirtualFileSeek(fd, -2, PSP_SEEK_CUR);
    }
}

//
// ATRAC3/MP3 Codec Initialization
//

static int osl_at3Init() {
    if (!osl_at3Inited) {
        initME();
        osl_at3Inited = 1;
    }
    return 0;
}

static int osl_mp3Init() {
    if (!osl_mp3Inited) {
        initME();
        osl_mp3Inited = 1;
    }
    return 0;
}

//
// MP3 and AT3 Info Handling
//

static void osl_mp3DestroyInfo(MP3_INFO *info) {
    if (info) {
        if (info->codecBuffer) {
			free(info->codecBuffer);
		}
        if (info->handle) {
			VirtualFileClose(info->handle);
		}
        if (info->getEDRAM) {
			sceAudiocodecReleaseEDRAM(info->codecBuffer);
		}
        free(info);
    }
}

// Create and allocate memory for MP3_INFO structure
static MP3_INFO *osl_mp3CreateInfo() {
    MP3_INFO *info = (MP3_INFO *)malloc(sizeof(MP3_INFO));
    if (!info) return NULL;

    memset(info, 0, sizeof(MP3_INFO));
    info->codecBuffer = memalign(64, CODEC_BUFFER_SIZE * sizeof(unsigned long));
    if (!info->codecBuffer) {
        osl_mp3DestroyInfo(info);
        return NULL;
    }
    memset(info->codecBuffer, 0, CODEC_BUFFER_SIZE * sizeof(unsigned long));
    return info;
}

// Destroy AT3_INFO structure and release resources
static void osl_at3DestroyInfo(AT3_INFO *info) {
    if (info) {
        osl_mp3DestroyInfo((MP3_INFO *)info);
    }
}

static AT3_INFO *osl_at3CreateInfo() {
    AT3_INFO *info = (AT3_INFO *)malloc(sizeof(AT3_INFO));
    if (!info) return NULL;

    memset(info, 0, sizeof(AT3_INFO));
    info->codecBuffer = memalign(64, CODEC_BUFFER_SIZE * sizeof(unsigned long));
    if (!info->codecBuffer) {
        osl_at3DestroyInfo(info);
        return NULL;
    }
    memset(info->codecBuffer, 0, CODEC_BUFFER_SIZE * sizeof(unsigned long));
    return info;
}

//
// MP3 and ATRAC3 Loading
//

static int osl_mp3Load(const char *fileName, MP3_INFO *info) {
    info->handle = VirtualFileOpen((void *)fileName, 0, VF_AUTO, VF_O_READ);
    if (!info->handle) return 0;

    info->channels = 2;
    info->samplerate = 44100;
    info->sample_per_frame = SAMPLE_PER_FRAME_MP3;

    info->data_start_init = GetID3TagSize((char*)fileName);
    VirtualFileSeek(info->handle, info->data_start_init, PSP_SEEK_SET);
    info->data_start_init = VirtualFileTell(info->handle);

    if (sceAudiocodecCheckNeedMem(info->codecBuffer, 0x1002) >= 0) {
        if (sceAudiocodecGetEDRAM(info->codecBuffer, 0x1002) >= 0) {
            info->getEDRAM = 1;
            if (sceAudiocodecInit(info->codecBuffer, 0x1002) >= 0)
                return 1;
        }
    }
    return 0;
}

static int osl_at3Load(const char *fileName, AT3_INFO *info) {
    info->handle = VirtualFileOpen((void *)fileName, 0, VF_AUTO, VF_O_READ);
    if (!info->handle) return 0;

    info->data_start_init = GetID3TagSize((char*)fileName);
    VirtualFileSeek(info->handle, info->data_start_init, PSP_SEEK_SET);

    u32 riff_header[2];
    if (VirtualFileRead(riff_header, RIFF_HEADER_SIZE, 1, info->handle) != RIFF_HEADER_SIZE)
        return 0;

    if (riff_header[0] != 0x46464952)  // "RIFF"
        return 0;

    u32 wavefmt_header[3];
    if (VirtualFileRead(wavefmt_header, WAVEFMT_HEADER_SIZE, 1, info->handle) != WAVEFMT_HEADER_SIZE)
        return 0;

    if (wavefmt_header[0] != 0x45564157 || wavefmt_header[1] != 0x20746D66)  // "WAVEfmt "
        return 0;

    u8 *wavefmt_data = (u8 *)malloc(wavefmt_header[2]);
    if (!wavefmt_data) return 0;

    if (VirtualFileRead(wavefmt_data, wavefmt_header[2], 1, info->handle) != wavefmt_header[2]) {
        free(wavefmt_data);
        return 0;
    }

    info->at3_type = *((u16 *)wavefmt_data);
    info->at3_channels = *((u16 *)(wavefmt_data + 2));
    info->at3_samplerate = *((u32 *)(wavefmt_data + 4));
    info->at3_data_align = *((u16 *)(wavefmt_data + 12));

    if (info->at3_type == TYPE_ATRAC3PLUS) {
        info->at3_at3plus_flagdata[0] = wavefmt_data[42];
        info->at3_at3plus_flagdata[1] = wavefmt_data[43];
    }

    free(wavefmt_data);

    u32 data_header[2];
    if (VirtualFileRead(data_header, RIFF_HEADER_SIZE, 1, info->handle) != RIFF_HEADER_SIZE)
        return 0;

    while (data_header[0] != 0x61746164) {  // "data"
        VirtualFileSeek(info->handle, data_header[1], PSP_SEEK_CUR);
        if (VirtualFileRead(data_header, RIFF_HEADER_SIZE, 1, info->handle) != RIFF_HEADER_SIZE)
            return 0;
    }

    info->data_start_init = VirtualFileTell(info->handle);
    info->at3_data_size = data_header[1];

    if (info->at3_data_size % info->at3_data_align != 0)
        return 0;

    if (info->at3_type == TYPE_ATRAC3) {
        info->at3_channel_mode = 0x0;
        if (info->at3_data_align == AUDIO_ALIGN_ATRAC3)
            info->at3_channel_mode = 0x1;

        info->sample_per_frame = SAMPLE_PER_FRAME_ATRAC3;
        info->dataBuffer = (u8 *)memalign(64, ATRAC3_BUFFER_SIZE);
        if (!info->dataBuffer) return 0;

        info->codecBuffer[26] = 0x20;
        if (sceAudiocodecCheckNeedMem(info->codecBuffer, 0x1001) < 0) return 0;
        if (sceAudiocodecGetEDRAM(info->codecBuffer, 0x1001) < 0) return 0;

        info->at3_getEDRAM = 1;
        info->codecBuffer[10] = 4;
        info->codecBuffer[44] = 2;
        if (info->at3_data_align == AUDIO_ALIGN_ATRAC3PLUS)
            info->codecBuffer[10] = 6;
        if (sceAudiocodecInit(info->codecBuffer, 0x1001) < 0) return 0;
        return 1;
    } else if (info->at3_type == TYPE_ATRAC3PLUS) {
        info->sample_per_frame = SAMPLE_PER_FRAME_ATRAC3PLUS;
        int temp_size = info->at3_data_align + ATRAC3PLUS_HEADER_SIZE;
        int mod_64 = temp_size & 0x3f;
        if (mod_64 != 0) temp_size += 64 - mod_64;

        info->dataBuffer = (u8 *)memalign(64, temp_size);
        if (!info->dataBuffer) return 0;

        info->codecBuffer[5] = 0x1;
        info->codecBuffer[10] = info->at3_at3plus_flagdata[1];
        info->codecBuffer[10] = (info->codecBuffer[10] << 8) | info->at3_at3plus_flagdata[0];
        info->codecBuffer[12] = 0x1;
        info->codecBuffer[14] = 0x1;
        if (sceAudiocodecCheckNeedMem(info->codecBuffer, 0x1000) < 0) return 0;
        if (sceAudiocodecGetEDRAM(info->codecBuffer, 0x1000) < 0) return 0;

        info->at3_getEDRAM = 1;
        if (sceAudiocodecInit(info->codecBuffer, 0x1000) < 0) return 0;
        return 1;
    }

    return 0;
}

//
// Sound Handling Callbacks
//

void oslAudioCallback_StopSound_ME(OSL_SOUND *s) {
    // Safety check for null pointer
    if (!s || !s->data) {
        return;
    }

    MP3_INFO *info = (MP3_INFO *)s->data;

    // Ensure the handle is valid before performing file operations
    if (info->handle) {
        info->data_start = info->data_start_init;
        VirtualFileSeek(info->handle, info->data_start, PSP_SEEK_SET);
    }
}

void oslAudioCallback_PlaySound_ME(OSL_SOUND *s) {
    oslAudioCallback_StopSound_ME(s);
}

int oslAudioCallback_AudioCallback_MP3(unsigned int i, void* buf, unsigned int length) {
    int eof = 0;
    MP3_INFO *info = (MP3_INFO*)osl_audioVoices[i].data;

    // Ensure valid info and handle
    if (!info || !info->handle) {
        return 0;
    }

    unsigned long decode_type = DECODE_TYPE_MP3;
    unsigned char mp3_header_buf[MP3_HEADER_SIZE];

    // Seek to the next valid MP3 frame
    info->data_start = SeekNextFrameMP3(info->handle);
    if (info->data_start == INVALID_FRAME) {
        eof = 1;
        goto end;
    }

start:
    // Read MP3 header
    if (VirtualFileRead(mp3_header_buf, MP3_HEADER_SIZE, 1, info->handle) != MP3_HEADER_SIZE) {
        eof = 1;
        goto end;
    }

    // Parse MP3 header
    int mp3_header = (mp3_header_buf[0] << 24) | (mp3_header_buf[1] << 16) |
                     (mp3_header_buf[2] << 8)  | mp3_header_buf[3];

    int bitrate = (mp3_header & 0xf000) >> 12;
    int padding = (mp3_header & 0x200) >> 9;
    int version = (mp3_header & 0x180000) >> 19;
    int samplerate_index = (mp3_header & 0xC00) >> 10;

    // Validate frame (bitrate, version, and samplerate)
    if ((bitrate > 14) || (version == 1) || (samplerate_index == 0) || (bitrate == 0)) {
        info->data_start = SeekNextFrameMP3(info->handle);
        if (info->data_start == INVALID_FRAME) {
            eof = 1;
            goto end;
        }
        goto start;
    }

    int samplerate = samplerates[version][samplerate_index];
    int frame_size;

    // Determine frame size and samples per frame based on version
    if (version == MPEG1_VERSION) {
        info->sample_per_frame = SAMPLE_PER_FRAME_MP3;
        frame_size = 144000 * bitrates[bitrate] / samplerate + padding;
    } else {
        info->sample_per_frame = 576;
        frame_size = 72000 * bitrates_v2[bitrate] / samplerate + padding;
    }

    // Seek back to the start of the frame
    VirtualFileSeek(info->handle, info->data_start, PSP_SEEK_SET);

    // Read the frame into the data buffer
    if (VirtualFileRead(info->dataBuffer, frame_size, 1, info->handle) != frame_size) {
        eof = 1;
        goto end;
    }

    // Update data_start for the next frame
    info->data_start += frame_size;

    // Setup codec buffer
    info->codecBuffer[7] = info->codecBuffer[10] = frame_size;
    info->codecBuffer[9] = info->sample_per_frame * 4;

    // Assign data buffer and output buffer to the codec buffer
    info->codecBuffer[6] = (unsigned long)info->dataBuffer;
    info->codecBuffer[8] = (unsigned long)buf;

    // Decode the frame
    int res = sceAudiocodecDecode(info->codecBuffer, decode_type);
    if (res < 0) {
        info->data_start = SeekNextFrameMP3(info->handle);
        if (info->data_start == INVALID_FRAME) {
            eof = 1;
            goto end;
        }
        goto start;
    }

end:
    // Return 0 if end-of-file (EOF), else 1 for successful decode
    return eof ? 0 : 1;
}

VIRTUAL_FILE **oslAudioCallback_ReactiveSound_ME(OSL_SOUND *s, VIRTUAL_FILE *f) {
    // Safety check to ensure the sound data is valid
    if (!s || !s->data) {
        return NULL;
    }

    MP3_INFO *info = (MP3_INFO *)s->data;
    return &info->handle;
}

VIRTUAL_FILE *oslAudioCallback_StandBy_ME(OSL_SOUND *s) {
    // Safety check to ensure the sound data is valid
    if (!s || !s->data) {
        return NULL;
    }

    MP3_INFO *info = (MP3_INFO *)s->data;
    return info->handle;
}

void oslAudioCallback_DeleteSound_ME(OSL_SOUND *s) {
    // Ensure sound data is valid before attempting to free it
    if (s && s->data) {
        osl_mp3DestroyInfo((MP3_INFO *)s->data);
        s->data = NULL;  // Ensure the sound data is reset after deletion
    }
}

int oslAudioCallback_AudioCallback_AT3(unsigned int i, void* buf, unsigned int length) {
    AT3_INFO *info = (AT3_INFO*)osl_audioVoices[i].data;
    int eof = 0;

    // Ensure valid info and handle
    if (!info || !info->handle || !info->dataBuffer) {
        return 0;
    }

    unsigned long decode_type;

    // ATRAC3 decoding
    if (info->at3_type == TYPE_ATRAC3) {
        // Clear the buffer
        memset(info->dataBuffer, 0, ATRAC3_BUFFER_SIZE);

        // Read data from the file
        if (VirtualFileRead(info->dataBuffer, info->at3_data_align, 1, info->handle) != info->at3_data_align) {
            eof = 1;
            goto end;
        }

        // Check and apply channel mode if needed
        if (info->at3_channel_mode) {
            memcpy(info->dataBuffer + info->at3_data_align, info->dataBuffer, info->at3_data_align);
        }

        decode_type = ATRAC3_DECODE_TYPE;
    }
    // ATRAC3plus decoding
    else {
        // Clear the buffer and set up ATRAC3plus header
        memset(info->dataBuffer, 0, info->at3_data_align + ATRAC3PLUS_HEADER_SIZE);
        info->dataBuffer[0] = ATRAC3PLUS_HEADER_0;
        info->dataBuffer[1] = ATRAC3PLUS_HEADER_1;
        info->dataBuffer[2] = info->at3_at3plus_flagdata[0];
        info->dataBuffer[3] = info->at3_at3plus_flagdata[1];

        // Read data from the file
        if (VirtualFileRead(info->dataBuffer + ATRAC3PLUS_HEADER_SIZE, info->at3_data_align, 1, info->handle) != info->at3_data_align) {
            eof = 1;
            goto end;
        }

        decode_type = ATRAC3PLUS_DECODE_TYPE;
    }

    // Set up codec buffer
    info->codecBuffer[6] = (unsigned long)info->dataBuffer;
    info->codecBuffer[8] = (unsigned long)buf;

    // Decode the frame
    int res = sceAudiocodecDecode(info->codecBuffer, decode_type);
    if (res < 0) {
        eof = 1;
    }

end:
    // Return 0 if end-of-file (EOF), else 1 for successful decode
    return eof ? 0 : 1;
}

//
// Sound File Loading
//

static void soundInit(const char *filename, OSL_SOUND *s, MP3_INFO *info) {
    // Ensure that the sound and info pointers are valid
    if (!s || !info || !filename) {
        return;
    }

    // Set up the sound data with the provided MP3 information
    s->data = (void *)info;

    // Initialize sound attributes
    s->endCallback = NULL;
    s->volumeLeft = s->volumeRight = OSL_VOLUME_MAX;
    s->format = 0;   // No special format, default stereo output
    s->mono = 0;     // Always stereo output
    s->divider = OSL_FMT_44K;  // Set sample rate divider to 44kHz
    s->isStreamed = 1;  // MP3 files are streamed by default
    s->numSamples = info->sample_per_frame;

    // If the sound is streamed, store the filename and suspend information
    if (s->isStreamed) {
        // Safely copy the filename, ensuring no buffer overflow
        strncpy(s->filename, filename, sizeof(s->filename) - 1);
        s->filename[sizeof(s->filename) - 1] = '\0';  // Ensure null termination

        // Set the suspend number to track suspension state
        s->suspendNumber = osl_suspendNumber;
    }

    // Set up the sound control callbacks
    s->playSound = oslAudioCallback_PlaySound_ME;
    s->stopSound = oslAudioCallback_StopSound_ME;
    s->standBySound = oslAudioCallback_StandBy_ME;
    s->reactiveSound = oslAudioCallback_ReactiveSound_ME;
    s->deleteSound = oslAudioCallback_DeleteSound_ME;
}

OSL_SOUND *oslLoadSoundFileMP3(const char *filename, int stream) {
    OSL_SOUND *s = NULL;
    MP3_INFO *info = NULL;
    int success = 0;

    // MP3 files must be streamed, otherwise exit early
    if (stream & OSL_FMT_STREAM) {
        // Allocate memory for the OSL_SOUND structure
        s = (OSL_SOUND *)malloc(sizeof(OSL_SOUND));
        if (s) {
            // Initialize the allocated memory to zero
            memset(s, 0, sizeof(OSL_SOUND));

            // Allocate memory for the MP3_INFO structure
            info = osl_mp3CreateInfo();
            if (info) {
                // Load the MP3 file and initialize the sound structure
                if (osl_mp3Load(filename, info)) {
                    soundInit(filename, s, info);  // Initialize sound properties
                    s->audioCallback = oslAudioCallback_AudioCallback_MP3;  // Set audio callback
                    success = 1;
                } else {
                    // Cleanup in case of failure to load the MP3 file
                    osl_mp3DestroyInfo(info);
                    info = NULL;
                }
            }
        }

        // If loading the sound was unsuccessful, free allocated memory
        if (!success) {
            if (s) {
                free(s);
            }
            s = NULL;
        }
    }

    // If loading failed, handle the error
    if (!s) {
        oslHandleLoadNoFailError(filename);
    }

    return s;
}

OSL_SOUND *oslLoadSoundFileAT3(const char *filename, int stream) {
    OSL_SOUND *s = NULL;
    AT3_INFO *info = NULL;
    int success = 0;

    // AT3 files must be streamed, otherwise exit early
    if (stream & OSL_FMT_STREAM) {
        // Allocate memory for the OSL_SOUND structure
        s = (OSL_SOUND *)malloc(sizeof(OSL_SOUND));
        if (s) {
            // Initialize the allocated memory to zero
            memset(s, 0, sizeof(OSL_SOUND));

            // Allocate memory for the AT3_INFO structure
            info = osl_at3CreateInfo();
            if (info) {
                // Load the AT3 file and initialize the sound structure
                if (osl_at3Load(filename, info)) {
                    soundInit(filename, s, (MP3_INFO *)info);  // Initialize sound properties
                    s->audioCallback = oslAudioCallback_AudioCallback_AT3;  // Set audio callback
                    success = 1;
                } else {
                    // Cleanup in case of failure to load the AT3 file
                    osl_at3DestroyInfo(info);
                    info = NULL;
                }
            }
        }

        // If loading the sound was unsuccessful, free allocated memory
        if (!success) {
            if (s) {
                free(s);
            }
            s = NULL;
        }
    }

    // If loading failed, handle the error
    if (!s) {
        oslHandleLoadNoFailError(filename);
    }

    return s;
}

void oslInitAudioME(int formats) {
    // Initialize AT3 format if requested
    if (formats & OSL_FMT_AT3) {
        osl_at3Init();
    }

    // Initialize MP3 format if requested
    if (formats & OSL_FMT_MP3) {
        osl_mp3Init();
    }
}