#include "oslib.h"
#include <mikmod.h>

static int osl_modPlaying = 0;
static int osl_modInitialized = 0;
static int osl_modFrequency = 44100;
static int osl_modStereo = 1;
static int osl_modShift = 0;

static BOOL PSP_IsThere(void) {
    return 1;
}

static BOOL PSP_Init(void) {
    if (VC_Init()) {
        return 1;
    }

    return 0;
}

static void PSP_Exit(void) {
    VC_Exit();
}

static BOOL PSP_Reset(void) {
    VC_Exit();
    return VC_Init();
}

static BOOL PSP_PlayStart(void) {
    VC_PlayStart();
    osl_modPlaying = 1;
    return 0;
}

static void PSP_PlayStop(void) {
    osl_modPlaying = 0;
    VC_PlayStop();
}

static void PSP_Update(void) {}

// Driver structure for MikMod to use PSP audio
MIKMODAPI MDRIVER drv_psp = {
    NULL,
    "PSP Audio",
    "PSP Output Driver v1.0 - by Jim Shaw",
    0, 255,
    "psp",
    NULL,
    NULL,
    PSP_IsThere,
    (void*)VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    PSP_Init,
    PSP_Exit,
    PSP_Reset,
    VC_SetNumVoices,
    PSP_PlayStart,
    PSP_PlayStop,
    PSP_Update,
    NULL,
    VC_VoiceSetVolume,
    VC_VoiceGetVolume,
    VC_VoiceSetFrequency,
    VC_VoiceGetFrequency,
    VC_VoiceSetPanning,
    VC_VoiceGetPanning,
    VC_VoicePlay,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceGetPosition,
    VC_VoiceRealVolume
};

/*
    Callbacks for MOD sound playback
*/
void oslAudioCallback_PlaySound_MOD(OSL_SOUND *s) {
    // Stereo only, mono doesn't work yet
    md_mode = DMODE_16BITS | DMODE_STEREO | DMODE_SOFT_MUSIC;
    s->mono = 0;

    Player_Stop();
    Player_Start((MODULE*)s->data);
    Player_SetPosition(0);
}

void oslAudioCallback_StopSound_MOD(OSL_SOUND *s) {
    Player_Stop();
}

int oslAudioCallback_AudioCallback_MOD(unsigned int i, void* buf, unsigned int length) {
    // Set up playback
    md_mixfreq = osl_modFrequency;

    if (osl_modPlaying) {
        VC_WriteBytes(buf, length << 2);
    } else {
        memset(buf, 0, length << 2);
    }

    // End of the song
    if (!Player_Active()) {
        return 0; // Stop playing
    }

    return 1; // Continue playing
}

VIRTUAL_FILE** oslAudioCallback_ReactiveSound_MOD(OSL_SOUND *s, VIRTUAL_FILE *f) {
    // Reactivate a MOD sound (not implemented)
    return NULL;
}

VIRTUAL_FILE* oslAudioCallback_StandBy_MOD(OSL_SOUND *s) {
    // Put a MOD sound on standby (not implemented)
    return NULL;
}

void oslAudioCallback_DeleteSound_MOD(OSL_SOUND *s) {
    MikMod_free((MODULE*)s->data);
}

/*
    Error handler for MikMod errors
*/
void oslHandleModError(void) {
    oslDebug("_mm_critical %i\nmm_errno %i\n%s", MikMod_critical, MikMod_errno, MikMod_strerror(MikMod_errno));
}

/*
    Set the sample rate and stereo configuration for MOD playback
    Example:
        oslSetModSampleRate(11025, 2); // Low CPU, bad sound
        oslSetModSampleRate(22050, 1); // Low CPU, medium sound
        oslSetModSampleRate(44100, 0); // Normal CPU, good sound
*/
void oslSetModSampleRate(int freq, int stereo, int shift) {
    osl_modFrequency = freq;
    osl_modStereo = stereo;
    osl_modShift = shift;
}

OSL_SOUND* oslLoadSoundFileMOD(const char* filename, int stream) {
    OSL_SOUND* s;
    MODULE* mf;

    if (!osl_modInitialized) {
        MikMod_RegisterAllLoaders();
        MikMod_RegisterDriver(&drv_psp);

        md_mode = DMODE_16BITS | DMODE_STEREO | DMODE_SOFT_MUSIC;
        MikMod_Init("");

        osl_modInitialized = 1;
    }

    s = (OSL_SOUND*)malloc(sizeof(OSL_SOUND));
    if (s) {
        memset(s, 0, sizeof(OSL_SOUND));
        mf = Player_Load((char*)filename, 128, 0);
        if (mf) {
			// Populate the OSL_SOUND structure with the loaded MOD file
			s->data = (void*)mf;
			s->endCallback = NULL;  // No end callback for MOD files
			s->volumeLeft = s->volumeRight = OSL_VOLUME_MAX;
			s->format = 0;          // No special format
			s->mono = 0;            // Always stereo output
			s->divider = OSL_FMT_44K; // Default to 44K format
			s->isStreamed = 0;      // MOD files are never streamed
			s->numSamples = 0;      // Default value

			// Assign callback functions
			s->audioCallback = oslAudioCallback_AudioCallback_MOD;
			s->playSound = oslAudioCallback_PlaySound_MOD;
			s->stopSound = oslAudioCallback_StopSound_MOD;
			s->standBySound = oslAudioCallback_StandBy_MOD;
			s->reactiveSound = oslAudioCallback_ReactiveSound_MOD;
			s->deleteSound = oslAudioCallback_DeleteSound_MOD;
        } else {
            free(s);
            s = NULL;
        }
    }

    if (!s) {
        oslHandleLoadNoFailError(filename);
    }

    return s;
}
