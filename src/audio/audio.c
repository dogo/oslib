/*
 * Audio system initialization and management.
 * Part of the pspaudiolib integration.
 *
 * Copyright (c) 2005 Adresd
 * Copyright (c) 2005 Marcus R. Brown <mrbrown@ocgnet.org>
 *
 * Licensed under the BSD license.
 */

#ifdef PSP
    #include <pspthreadman.h>
    #include <pspaudio.h>
#endif

#include "oslib.h"
#include "audio.h"

// Audio configuration constants
int osl_audioDefaultNumSamples = 512; // Default number of samples per audio channel

// Audio system state
static int audio_ready = 0;           // Indicates if audio is ready
int osl_suspendNumber = 0;            // Tracks the number of suspend events
int osl_audioStandBy = 0;             // Indicates if the audio is in standby mode

// Audio channel management
OSL_AUDIO_VOICE osl_audioVoices[OSL_NUM_AUDIO_CHANNELS];    // Array of audio voices (1 per channel)
static osl_audio_channelinfo AudioStatus[OSL_NUM_AUDIO_CHANNELS]; // Audio channel status info
volatile int osl_audioActive[OSL_NUM_AUDIO_CHANNELS];       // Tracks the active state of each audio channel
static u32 *audio_sndbuf[OSL_NUM_AUDIO_CHANNELS];           // Sound buffers for each audio channel
long osl_filesave[OSL_NUM_AUDIO_CHANNELS];                  // Stores file positions for streamed audio

// PSP power management
int (*osl_audioOldPowerCallback)(int, int, void*) = NULL;   // Internal power callback for PSP audio

void oslAudioChannelThreadCallback(int channel, void *buf, unsigned int reqn) {
    // Ensure the channel index is valid and the callback is set
    if (channel < 0 || channel >= OSL_NUM_AUDIO_CHANNELS || AudioStatus[channel].callback == NULL) {
        return; // Invalid channel or no callback set, exit early
    }

    // Call the callback function
    void (*callback)(unsigned int, void*, unsigned int) = AudioStatus[channel].callback;
    callback(channel, buf, reqn);
}

void oslAudioSetChannelCallback(int channel, void* callback) {
    if (channel < 0 || channel >= OSL_NUM_AUDIO_CHANNELS) {
        return; // Ensure valid channel
    }
    AudioStatus[channel].callback = (void(*)(unsigned int, void*, unsigned int))callback;
}

int oslAudioOutBlocking(unsigned int channel, unsigned int vol1, unsigned int vol2, void *buf) {
    if (!audio_ready) return -1;
    if (vol1 > OSL_VOLUME_MAX) vol1 = OSL_VOLUME_MAX;
    if (vol2 > OSL_VOLUME_MAX) vol2 = OSL_VOLUME_MAX;

    return sceAudioOutputPannedBlocking(AudioStatus[channel].handle, vol1, vol2, buf);
}

static int oslAudioChannelThread(int args, void *argp) {
    int channel = *(int*)argp;
    int bufferIndex = 0;

    // Allocate double-buffer for audio processing
    audio_sndbuf[channel] = (u32*)calloc(osl_audioVoices[channel].numSamples, 8);
    if (!audio_sndbuf[channel]) {
        return -1; // Memory allocation failure
    }

    memset(audio_sndbuf[channel], 0, osl_audioVoices[channel].numSamples << 3);

    while (osl_audioActive[channel] > 0) {
		// Get a pointer to our actual buffer (we do double buffering)
        void* bufptr = audio_sndbuf[channel] + bufferIndex * osl_audioVoices[channel].numSamples;
		// Our callback function
        void (*callback)(unsigned int channel, void *buf, unsigned int reqn) = AudioStatus[channel].callback;

        AudioStatus[channel].inProgress = 1;
        if (callback && osl_audioActive[channel] == 1) {
            callback(channel, bufptr, osl_audioVoices[channel].numSamples);
        } else {
            memset(bufptr, 0, osl_audioVoices[channel].numSamples << 2);
        }
        AudioStatus[channel].inProgress = 0;

        oslAudioOutBlocking(channel, osl_audioVoices[channel].sound->volumeLeft, osl_audioVoices[channel].sound->volumeRight, bufptr);
        bufferIndex = (bufferIndex ? 0 : 1);
    }

    // Clean up after the channel is done
    osl_audioActive[channel] = -2;
    free(audio_sndbuf[channel]);
    audio_sndbuf[channel] = NULL;

    AudioStatus[channel].threadhandle = -1;
    AudioStatus[channel].callback = NULL;
    if (AudioStatus[channel].handle != -1) {
        sceAudioChRelease(AudioStatus[channel].handle);
        AudioStatus[channel].handle = -1;
    }

    osl_audioVoices[channel].sound = NULL;
    osl_audioActive[channel] = 0;

    sceKernelExitDeleteThread(0);
    return 0;
}

void oslAudioDeleteChannel(int i) {
	osl_audioActive[i]=-1;
}

static void setChannelSound(int voice, OSL_SOUND *s) {
    if (s == NULL) {
        return; // Avoid null pointer dereference.
    }

    // Set the number of samples, using a default if not provided.
    osl_audioVoices[voice].numSamples = (s->numSamples == 0) ? osl_audioDefaultNumSamples : s->numSamples;

    // Set the remaining properties from the sound object to the voice.
    osl_audioVoices[voice].data = s->data;
    osl_audioVoices[voice].format = s->format;
    osl_audioVoices[voice].size = s->size;
    osl_audioVoices[voice].divider = s->divider;
    osl_audioVoices[voice].mono = s->mono;
    osl_audioVoices[voice].dataplus = s->dataplus;
    osl_audioVoices[voice].isStreamed = s->isStreamed;

    // Link the sound to the voice.
    osl_audioVoices[voice].sound = s;
}

int oslAudioCreateChannel(int i, int format, int numSamples, OSL_SOUND *s) {
    char threadName[32];
    int ret;

    // Initialize the audio status for the channel.
    AudioStatus[i].handle = -1;
    AudioStatus[i].threadhandle = -1;
    AudioStatus[i].callback = NULL;

    // Set the sound properties for the channel.
    setChannelSound(i, s);

    // Reserve the audio channel in the PSP audio system.
    AudioStatus[i].handle = sceAudioChReserve(i, osl_audioVoices[i].numSamples, format);
    if (AudioStatus[i].handle < 0) {
        return -1; // Failed to reserve audio channel.
    }

    // Mark audio as ready and active.
    audio_ready = 1;
    osl_audioActive[i] = 1;

    // Create a thread name like "audiot0", "audiot1", etc.
    snprintf(threadName, sizeof(threadName), "audiot%d", i);

    // Create a kernel thread for the audio channel.
    AudioStatus[i].threadhandle = sceKernelCreateThread(threadName, (SceKernelThreadEntry)&oslAudioChannelThread, 0x10, 0x10000, 0, NULL);
    if (AudioStatus[i].threadhandle < 0) {
        // Thread creation failed, clean up the audio channel.
        sceAudioChRelease(AudioStatus[i].handle);
        AudioStatus[i].handle = -1;
        return -1; // Failed to create thread.
    }

    // Start the created thread.
    ret = sceKernelStartThread(AudioStatus[i].threadhandle, sizeof(i), &i);
    if (ret != 0) {
        // Thread starting failed, clean up both the thread and the audio channel.
        osl_audioActive[i] = 0;
        sceKernelWaitThreadEnd(AudioStatus[i].threadhandle, NULL);
        sceKernelDeleteThread(AudioStatus[i].threadhandle);
        AudioStatus[i].threadhandle = -1;
        sceAudioChRelease(AudioStatus[i].handle);
        AudioStatus[i].handle = -1;
        return -1; // Failed to start thread.
    }

    return 0; // Successfully created and started the audio channel.
}

int oslAudioRecreateChannel(int i, int format, int numSamples, OSL_SOUND *s) {
    // If the channel already has the same number of samples and the same sound, just reconfigure it.
    if (osl_audioVoices[i].numSamples == numSamples && osl_audioVoices[i].sound == s) {
        // Update the channel configuration.
        sceAudioChangeChannelConfig(i, format);
        setChannelSound(i, s);  // Update the sound properties for this channel.
        osl_audioActive[i] = 1; // Mark the channel as active.
        return 0;  // Return early since the reconfiguration is sufficient.
    }

    // If the number of samples or the sound has changed, recreate the channel.
    oslAudioDeleteChannel(i);  // First, delete the current channel.

    // Wait until the channel is fully deactivated.
    while (osl_audioActive[i] != 0) {
        // Busy-wait until the channel becomes inactive.
    }

    // Ensure the thread has terminated properly.
    sceKernelWaitThreadEnd(AudioStatus[i].threadhandle, NULL);

    // Recreate the channel with the new configuration.
    if (oslAudioCreateChannel(i, format, numSamples, s) < 0) {
        return -1;
    }

    return 0;
}

int oslGetSoundChannel(OSL_SOUND *s) {
    // Iterate through all audio channels to find the one that matches the sound pointer.
    for (int i = 0; i < OSL_NUM_AUDIO_CHANNELS; i++) {
        if (osl_audioVoices[i].sound == s) {
            return i;
        }
    }
    return -1;
}

// ------------------------------------------

#include "readwav.h"

void oslDecodeWav(unsigned int i, void* buf, unsigned int length) {
    unsigned int j, k, samples = 1 << osl_audioVoices[i].divider;
    unsigned short* data = (unsigned short*)buf, cur1, cur2;
    WAVE_SRC* wav = (WAVE_SRC*)osl_audioVoices[i].dataplus;
    unsigned char* streambuffer = NULL;
    int len;

    // Handle streamed audio
    if (wav->stream) {
        len = (length * wav->fmt.bits_sample) >> 3;
        if (samples == 1) {
            len <<= 1; // Double the length if necessary
        } else {
            len >>= osl_audioVoices[i].divider;
        }

        // Adjust length for stereo sound
        if (osl_audioVoices[i].mono == 0) {
            len <<= 1;
        }

        // Allocate buffer dynamically instead of using alloca
        streambuffer = (unsigned char*)malloc(len);
        if (streambuffer == NULL) {
            return; // Handle allocation failure
        }

        VirtualFileRead(streambuffer, len, 1, wav->fp);
        wav->streambuffer = streambuffer;
    }

    // Process audio samples
    if (samples == 1) {
        if (osl_audioVoices[i].mono == 0) {
            length <<= 1; // Double length for stereo
        }
        for (j = 0; j < length; j++) {
            *data++ = get_next_wav_sample(wav);
        }
    } else {
        length >>= osl_audioVoices[i].divider;
        if (osl_audioVoices[i].mono == 0) {
            // Stereo case
            for (j = 0; j < length; j++) {
                cur1 = get_next_wav_sample(wav);
                cur2 = get_next_wav_sample(wav);
                for (k = 0; k < samples; k++) {
                    *data++ = cur1;
                    *data++ = cur2;
                }
            }
        } else {
            // Mono case
            for (j = 0; j < length; j++) {
                cur1 = get_next_wav_sample(wav);
                for (k = 0; k < samples; k++) {
                    *data++ = cur1;
                }
            }
        }
    }

    // If the chunk is finished, trigger the end callback
    if (wav->chunk_left <= 0) {
        if (osl_audioVoices[i].sound->endCallback) {
            if (osl_audioVoices[i].sound->endCallback(osl_audioVoices[i].sound, i)) {
                free(streambuffer); // Free dynamically allocated buffer
                return;
            }
        }
        oslAudioDeleteChannel(i);
    }

    // Free the dynamically allocated buffer if streaming
    if (wav->stream) {
        free(streambuffer);
    }
}

/*
 * Function used to fill the audio buffer (44,100 Hz, 16 bits, Mono).
 * This function acts as a callback for audio processing.
 */
void oslAudioCallback(unsigned int i, void* buf, unsigned int length) {
    // Check if the sound object or audio callback is NULL to avoid null pointer dereference
    if (!osl_audioVoices[i].sound || !osl_audioVoices[i].sound->audioCallback) {
        oslAudioDeleteChannel(i); // Delete the channel if there's no valid sound or callback
        return;
    }

    // Call the audio callback for this channel
    if (!osl_audioVoices[i].sound->audioCallback(i, buf, length)) {
        // If the callback returns 0, the sound is finished. Check for the end callback.
        if (osl_audioVoices[i].sound->endCallback) {
            // Call the end callback. If it returns non-zero, the sound continues, so we return.
            if (osl_audioVoices[i].sound->endCallback(osl_audioVoices[i].sound, i)) {
                return;
            }
        }
        // If no end callback is set or the sound is fully finished, delete the audio channel.
        oslAudioDeleteChannel(i);
    }
}

#ifdef PSP
int oslAudioPowerCallback(int unknown, int pwrflags, void *common) {
    OSL_SOUND *s;
    VIRTUAL_FILE *f = NULL;
    int i;

    // Handle PSP entering standby (power switch is toggled)
    if (pwrflags & PSP_POWER_CB_POWER_SWITCH) {
        osl_audioStandBy = 1;
        for (i = 0; i < OSL_NUM_AUDIO_CHANNELS; i++) {
            s = osl_audioVoices[i].sound;
            if (s && s->isStreamed) {
                // Mark the audio channel as suspended (3 = suspended/invalid)
                osl_audioActive[i] = 3;
                // Save the current file position in case we need to resume the stream
                f = s->standBySound(s);
                if (f) {
                    osl_filesave[i] = VirtualFileTell(f);
                }
            }
        }
        osl_suspendNumber++;
    }
    // Handle PSP resume event
    else if (pwrflags & PSP_POWER_CB_RESUME_COMPLETE) {
        osl_audioStandBy = 0;
    }

    // Call the previous power callback, if one was set
    if (osl_audioOldPowerCallback) {
        return osl_audioOldPowerCallback(unknown, pwrflags, common);
    }

    return 0;
}
#endif

int oslInitAudio() {
    int i;

    // Initialize audio channels
    for (i = 0; i < OSL_NUM_AUDIO_CHANNELS; i++) {
        osl_audioActive[i] = 0;                      // Mark audio channel as inactive
        osl_audioVoices[i].sound = NULL;             // Clear sound reference

        // Initialize audio status for each channel
        AudioStatus[i].handle = -1;                  // No handle assigned yet
        AudioStatus[i].threadhandle = -1;            // No thread handle assigned yet
        AudioStatus[i].callback = NULL;              // No callback assigned
    }

    // Reset the audio state
    audio_ready = 0;
    memset(osl_audioVoices, 0, sizeof(osl_audioVoices)); // Clear all audio voices
    osl_suspendNumber = 0;                              // Reset suspend counter

#ifdef PSP
    osl_audioOldPowerCallback = osl_powerCallback;
    osl_powerCallback = oslAudioPowerCallback;
#endif

    osl_audioStandBy = 0;                              // Set standby to false
    return 0;
}

// Deinitialize the audio system. All sounds should be stopped before calling this.
void oslDeinitAudio() {
    // Stop all active audio channels
    for (int i = 0; i < OSL_NUM_AUDIO_CHANNELS; i++) {
        oslAudioDeleteChannel(i);  // Ensure each channel is properly deleted
    }

    // Restore the previous power callback, if applicable
    osl_powerCallback = osl_audioOldPowerCallback;

    // Mark the audio system as no longer ready
    audio_ready = 0;
}

OSL_SOUND *oslLoadSoundFile(const char *filename, int stream) {
    if (filename == NULL || strlen(filename) < 4) {
        return NULL;  // Invalid filename or file too short
    }

    // Check if the file is a BGM file
    if (!strcmp(filename + strlen(filename) - 4, ".bgm")) {
        return oslLoadSoundFileBGM(filename, stream);
    }
    // Check if the file is a WAV file
    else if (!strcmp(filename + strlen(filename) - 4, ".wav")) {
        return oslLoadSoundFileWAV(filename, stream);
    }

    // Unsupported file type
    return NULL;
}

void oslDeleteSound(OSL_SOUND *s) {
    if (s == NULL) {
        return;
    }

    // Ensure the sound is not being played
    oslStopSound(s);

    // Call the custom delete function, if provided
    if (s->deleteSound != NULL) {
        s->deleteSound(s);
    }

    free(s);
}

/*
 * Reactivate streamed audio after the PSP comes out of suspend mode.
 * Return values:
 *  0 = not reactivated,
 *  1 = reactivated,
 *  2 = reactivated, but the sound needs to restart from the beginning.
 */
int oslAudioReactiveSound(OSL_SOUND *s) {
    // If the sound is streamed and was suspended (i.e., after PSP was in standby)
    if (s->isStreamed && s->suspendNumber < osl_suspendNumber) {
        VIRTUAL_FILE *f;
        VIRTUAL_FILE **w = NULL;
        int channel;

        // Reopen the virtual file. This may fail if the virtual file system parameters have changed.
        f = VirtualFileOpen(s->filename, 0, VF_AUTO, VF_O_READ);
        if (!f) {
            return 0;  // Failed to reopen the file, return without reactivation.
        }

        // Reactivate the sound with the reopened file.
        w = s->reactiveSound(s, f);
        channel = oslGetSoundChannel(s);

        // If the sound was previously associated with a channel, restore the file position.
        if (channel >= 0) {
            VirtualFileSeek(f, osl_filesave[channel], SEEK_SET);
        }

        // Update the pointer to the new file in the sound object.
        *w = f;
        s->suspendNumber = osl_suspendNumber;

        // Return 1 if reactivated on an existing channel, otherwise return 2 to restart sound.
        return (channel >= 0) ? 1 : 2;
    }

    // Return 0 if the sound was not reactivated (e.g., if it was not streamed).
    return 0;
}

int oslSoundLoopFunc(OSL_SOUND *s, int voice) {
    oslPlaySound(s, voice);

    return 1;
}

/*
 * Synchronize audio during VSync, especially handling cases where the PSP
 * may enter standby mode and we need to resume streamed audio.
 * This function is called repeatedly in a loop.
 */
void oslAudioVSync() {
    // If the system is in standby mode, we don't need to process audio
    if (osl_audioStandBy) {
        return;
    }

    // Check each audio channel
    for (int i = 0; i < OSL_NUM_AUDIO_CHANNELS; i++) {
        // Only check channels that were suspended (marked with a 3)
        if (osl_audioActive[i] != 3) {
            continue;
        }

        // If a sound is associated with the channel, attempt to reactivate it
        if (osl_audioVoices[i].sound) {
            if (oslAudioReactiveSound(osl_audioVoices[i].sound)) {
                // Mark the channel as active again if the sound is reactivated
                osl_audioActive[i] = 1;
            }
        }
    }
}

void oslPlaySound(OSL_SOUND *s, int voice) {
    // Ensure the voice (channel) is within a valid range (0 to 7)
    if (voice < 0 || voice >= OSL_NUM_AUDIO_CHANNELS || s == NULL) {
        return;
    }

    // Wait if the audio channel is in a transition state (-2 indicates it is in use)
    while (osl_audioActive[voice] == -2) {
		// Busy-wait until the channel transition.
    }

    // Create the audio channel if it's not active, otherwise recreate or reuse it
    if (!osl_audioActive[voice]) {
        oslAudioCreateChannel(voice, s->mono, osl_audioVoices[voice].numSamples, s);
    } else {
        oslAudioRecreateChannel(voice, s->mono, osl_audioVoices[voice].numSamples, s);
    }

    // Try to reactivate the sound (if suspended after standby)
    if (oslAudioReactiveSound(s) != 1) {
        // If not reactivated, restart the sound from the beginning
        s->playSound(s);
    }

    // Set the callback for the audio channel
    oslAudioSetChannelCallback(voice, oslAudioCallback);
}

void oslStopSound(OSL_SOUND *s) {
    // Find the audio channel (voice) associated with the sound
    int voice = oslGetSoundChannel(s);

    // Call the sound's custom stop function
    if (s && s->stopSound) {
        s->stopSound(s);
    }

    // If the sound was playing, delete the associated audio channel
    if (voice >= 0) {
        oslAudioDeleteChannel(voice);
    }
}

void oslPauseSound(OSL_SOUND *s, int pause) {
    // Get the audio channel (voice) associated with the sound
    int voice = oslGetSoundChannel(s);

    // Ensure the sound is valid and currently being played
    if (voice >= 0) {
        // Toggle the pause state if pause == -1
        if (pause == -1) {
            osl_audioActive[voice] = 3 - osl_audioActive[voice];  // Toggles between 1 (playing) and 2 (paused)
        }
        // Set the pause state explicitly: 2 = paused, 1 = playing
        else {
            osl_audioActive[voice] = pause ? 2 : 1;
        }
    }
}

/*
 * Standard callback to reset the playback position for WAV files.
 * This is used to either restart a streamed WAV or reset the in-memory WAV data.
 */
void oslAudioCallback_PlaySound_WAV(OSL_SOUND *s) {
    // Ensure the sound and its associated data (dataplus) are valid
    if (s == NULL || s->dataplus == NULL) {
        return; // Invalid sound or data, exit early
    }

    WAVE_SRC *wav = (WAVE_SRC*)s->dataplus;

    // If the sound is streamed, seek to the base position in the virtual file
    if (s->isStreamed) {
        VirtualFileSeek(wav->fp, wav->basefp, SEEK_SET);
    }
    // Otherwise, reset the data pointer for in-memory WAV
    else {
        wav->data = wav->database;
    }

    // Reset the remaining chunk size to the base value
    wav->chunk_left = wav->chunk_base;
}

void oslAudioCallback_StopSound_WAV(OSL_SOUND *s) {
	// Do nothing
}

int oslAudioCallback_AudioCallback_WAV(unsigned int i, void* buf, unsigned int length) {
    // Decode WAV audio data into the provided buffer
    oslDecodeWav(i, buf, length);
    return 1;
}

VIRTUAL_FILE** oslAudioCallback_ReactiveSound_WAV(OSL_SOUND *s, VIRTUAL_FILE *f) {
    // Ensure the sound and its associated data (dataplus) are valid
    if (s == NULL || s->dataplus == NULL) {
        return NULL;  // Invalid sound or data, return NULL
    }

    // Return a pointer to the file pointer for the reactive sound
    VIRTUAL_FILE **w = (VIRTUAL_FILE**)&((WAVE_SRC*)s->dataplus)->fp;
    return w;
}

VIRTUAL_FILE* oslAudioCallback_StandBy_WAV(OSL_SOUND *s) {
    // Ensure the sound and its associated data (dataplus) are valid
    if (s == NULL || s->dataplus == NULL) {
        return NULL;  // Invalid sound or data, return NULL
    }

    // Return the file pointer for the sound's virtual file
    VIRTUAL_FILE *f = (VIRTUAL_FILE*)((WAVE_SRC*)s->dataplus)->fp;
    return f;
}

void oslAudioCallback_DeleteSound_WAV(OSL_SOUND *s) {
    // Ensure the sound object and its associated data (dataplus) are valid
    if (s == NULL || s->dataplus == NULL) {
        return;  // Invalid sound or data, exit early
    }

    WAVE_SRC *wav = (WAVE_SRC*)s->dataplus;

    // If the sound is streamed, close the associated file
    if (s->isStreamed) {
        close_wave_src(wav);
    }
    // If the sound is not streamed, free the in-memory data
    else {
        free(wav->database);
    }

    // Free the memory associated with the WAV structure (dataplus)
    free(s->dataplus);
}

/*
 * Loads a WAV sound file, returning a pointer to an OSL_SOUND structure.
 * Supports both streamed and non-streamed sounds.
 */
OSL_SOUND *oslLoadSoundFileWAV(const char *filename, int stream) {
    OSL_SOUND *s = NULL;
    WAVE_SRC *wav = NULL;

    // Allocate memory for the OSL_SOUND structure
    s = (OSL_SOUND*)malloc(sizeof(OSL_SOUND));
    if (!s) {
        goto error;  // Memory allocation failure
    }

    // Zero-initialize the OSL_SOUND structure
    memset(s, 0, sizeof(OSL_SOUND));

    // Allocate memory for the WAVE_SRC structure
    wav = (WAVE_SRC*)malloc(sizeof(WAVE_SRC));
    if (!wav) {
        free(s);
        goto error;  // Memory allocation failure
    }

    // Open the WAV file
    if (open_wave_src(wav, filename) < 0) {
        free(s);
        free(wav);
        goto error;  // Failed to open the WAV file
    }

    // Set audio format (mono or stereo) based on the WAV file's channel count
	if (wav->fmt.channels == 1) {
		s->mono = 0x10;					// OSL_AUDIO_FORMAT_MONO
	} else {
		s->mono = 0x00;					// OSL_AUDIO_FORMAT_STEREO
	}
    wav->basefp = VirtualFileTell(wav->fp);
    wav->chunk_base = wav->chunk_left;
    s->size = (int)wav->chunk_left;  // Set the sound size
	s->endCallback = NULL;
    s->volumeLeft = s->volumeRight = OSL_VOLUME_MAX;  // Default volume
    s->format = 0;  // Default format
	s->numSamples = 0; // Default number of samples

    // Set the sample rate divider based on the WAV's sample rate
    if (wav->fmt.sample_rate >= 44100) {
        s->divider = OSL_FMT_44K;
    } else if (wav->fmt.sample_rate >= 22050) {
        s->divider = OSL_FMT_22K;
    } else {
        s->divider = OSL_FMT_11K;
    }

    // Set whether the sound is streamed or loaded into memory
    s->isStreamed = stream;
    s->dataplus = wav;
    wav->stream = stream;

    // If the sound is streamed, copy the filename and set the suspend number
    if (s->isStreamed) {
        if (strlen(filename) >= sizeof(s->filename)) {
            oslFatalError("Sound file name too long!");
        }
        s->suspendNumber = osl_suspendNumber;
        strcpy(s->filename, filename);
    } else {
        // Allocate memory for the in-memory WAV data
        wav->database = (unsigned char*)malloc(s->size);
        if (!wav->database) {
            free(s);
            close_wave_src(wav);
            free(wav);
            goto error;  // Memory allocation failure for in-memory data
        }

        // Read the WAV file data into memory
        VirtualFileRead(wav->database, s->size, 1, wav->fp);
        wav->data = wav->database;
        close_wave_src(wav);  // Close the file as it is no longer needed
    }

    // Set the callback functions for the sound
    s->audioCallback = oslAudioCallback_AudioCallback_WAV;
    s->playSound = oslAudioCallback_PlaySound_WAV;
    s->stopSound = oslAudioCallback_StopSound_WAV;
    s->standBySound = oslAudioCallback_StandBy_WAV;
    s->reactiveSound = oslAudioCallback_ReactiveSound_WAV;
    s->deleteSound = oslAudioCallback_DeleteSound_WAV;

    return s;

error:
    oslHandleLoadNoFailError(filename);
    return NULL;
}
