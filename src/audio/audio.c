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

#include <limits.h>
#include <strings.h>

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
static OSL_SOUND *audio_pendingDelete[OSL_NUM_AUDIO_CHANNELS];
static OSL_SOUND *audio_pendingPlay[OSL_NUM_AUDIO_CHANNELS]; // handed over by an audio thread
// A stream whose file a standby closed: the thread that asks reopens it (an app thread: relative
// paths resolve as when it was loaded), the channel's thread installs it between two callbacks
static OSL_SOUND *audio_reopen[OSL_NUM_AUDIO_CHANNELS];
static VIRTUAL_FILE *audio_reopenFile[OSL_NUM_AUDIO_CHANNELS];
static int audio_reopenRestart[OSL_NUM_AUDIO_CHANNELS]; // from the start (oslPlaySound), or not (oslAudioVSync)
static int audio_reopenNumber[OSL_NUM_AUDIO_CHANNELS];  // the standbys the new file follows
long osl_filesave[OSL_NUM_AUDIO_CHANNELS];                  // Stores file positions for streamed audio

// PSP power management
int (*osl_audioOldPowerCallback)(int, int, void *) = NULL;   // Internal power callback for PSP audio

void oslAudioChannelThreadCallback(int channel, void *buf, unsigned int reqn) {
	// Ensure the channel index is valid and the callback is set
	if (channel < 0 || channel >= OSL_NUM_AUDIO_CHANNELS || AudioStatus[channel].callback == NULL) {
		return; // Invalid channel or no callback set, exit early
	}

	// Call the callback function
	void (*callback)(unsigned int, void *, unsigned int) = AudioStatus[channel].callback;
	callback(channel, buf, reqn);
}

void oslAudioSetChannelCallback(int channel, void *callback) {
	if (channel < 0 || channel >= OSL_NUM_AUDIO_CHANNELS) {
		return; // Ensure valid channel
	}
	AudioStatus[channel].callback = (void (*)(unsigned int, void *, unsigned int)) callback;
}

int oslAudioOutBlocking(unsigned int channel, unsigned int vol1, unsigned int vol2, void *buf) {
	if (!audio_ready) return -1;
	if (vol1 > OSL_VOLUME_MAX) vol1 = OSL_VOLUME_MAX;
	if (vol2 > OSL_VOLUME_MAX) vol2 = OSL_VOLUME_MAX;

	return sceAudioOutputPannedBlocking(AudioStatus[channel].handle, vol1, vol2, buf);
}

// A stream's file a standby closed: closing it may fail, the VIRTUAL_FILE is freed anyway
static void oslAudioDropFile(VIRTUAL_FILE *f) {
	VirtualFileGetSource(f)->fClose(f);
	free(f);
}

static void oslAudioFreeSound(OSL_SOUND *s) {
	if (s->deleteSound)
		s->deleteSound(s);
	free(s);
}

static void setChannelSound(int voice, OSL_SOUND *s);
static void oslAudioStartSound(OSL_SOUND *s, int voice);
static void oslAudioInstall(OSL_SOUND *s, int channel, VIRTUAL_FILE *f, int number, int restart);
void oslAudioCallback(unsigned int i, void *buf, unsigned int length);

#define OSL_AUDIO_ERROR_OUTPUT_BUSY 0x80260002

// The hardware refuses a new configuration while the channel's last buffer still plays: wait
// for it, a buffer at most
static int oslAudioConfigureChannel(int handle, int format, int value) {
	for (int tries = 0;; tries++) {
		int ret = format ? sceAudioChangeChannelConfig(handle, value) : sceAudioSetChannelDataLen(handle, value);
		if (ret != (int)OSL_AUDIO_ERROR_OUTPUT_BUSY || tries == 100)
			return ret;
		sceKernelDelayThread(1000);
	}
}

static int oslAudioChannelThread(int args, void *argp) {
	int channel = *(int *)argp;
	OSL_SOUND *next;

	// One pass per sound: a play handed over by an audio thread continues on this thread
	do {
		int bufferIndex = 0;
		int is_mono = (osl_audioVoices[channel].mono == 0x10);
		// Bytes per sample: mono = 2, stereo = 4
		int bytes_per_sample = is_mono ? 2 : 4;
		// Total buffer size: numSamples * bytes_per_sample * 2 (double buffer)
		int buffer_size = osl_audioVoices[channel].numSamples * bytes_per_sample * 2;

		// Allocate double-buffer for audio processing
		audio_sndbuf[channel] = (u32 *)calloc(1, buffer_size);

		// Without a buffer the loop is skipped, but the cleanup below still ends the channel
		while (audio_sndbuf[channel] && osl_audioActive[channel] > 0) {
			// The only place a playing stream's file changes: no callback reads it meanwhile
			unsigned int flags = sceKernelCpuSuspendIntr();
			OSL_SOUND *reopen = audio_reopen[channel];
			VIRTUAL_FILE *file = audio_reopenFile[channel];
			int restart = audio_reopenRestart[channel], number = audio_reopenNumber[channel];
			audio_reopen[channel] = NULL;
			audio_reopenFile[channel] = NULL;
			sceKernelCpuResumeIntr(flags);
			if (reopen && reopen == osl_audioVoices[channel].sound)
				oslAudioInstall(reopen, channel, file, number, restart);
			else if (file)
				VirtualFileClose(file); // for a sound no longer on this channel

			// Get a pointer to our actual buffer (we do double buffering)
			// For mono: advance by numSamples * 2 bytes
			// For stereo: advance by numSamples * 4 bytes
			void *bufptr = (u8 *)audio_sndbuf[channel] + bufferIndex * osl_audioVoices[channel].numSamples * bytes_per_sample;
			// Our callback function
			void (*callback)(unsigned int channel, void *buf, unsigned int reqn) = AudioStatus[channel].callback;
			// Read before the callback: its endCallback may delete the sound
			OSL_SOUND *sound = osl_audioVoices[channel].sound;
			unsigned int volumeLeft = sound ? sound->volumeLeft : 0, volumeRight = sound ? sound->volumeRight : 0;

			AudioStatus[channel].inProgress = 1;
			if (callback && osl_audioActive[channel] == 1) {
				callback(channel, bufptr, osl_audioVoices[channel].numSamples);
			} else {
				memset(bufptr, 0, osl_audioVoices[channel].numSamples * bytes_per_sample);
			}
			AudioStatus[channel].inProgress = 0;

			oslAudioOutBlocking(channel, volumeLeft, volumeRight, bufptr);
			bufferIndex = (bufferIndex ? 0 : 1);
		}

		// Clean up after the sound is done
		osl_audioActive[channel] = -2;
		free(audio_sndbuf[channel]);
		audio_sndbuf[channel] = NULL;

		// Claim a deferred deletion and a handed-over play, and switch the published sound, together:
		// another thread can then neither free the old sound early nor miss the new one.
		unsigned int flags = sceKernelCpuSuspendIntr();
		OSL_SOUND *sound = audio_pendingDelete[channel];
		audio_pendingDelete[channel] = NULL;
		next = audio_pendingPlay[channel];
		audio_pendingPlay[channel] = NULL;
		// A reopen for the ended sound is moot
		audio_reopen[channel] = NULL;
		VIRTUAL_FILE *unused = audio_reopenFile[channel];
		audio_reopenFile[channel] = NULL;
		int numSamples = osl_audioVoices[channel].numSamples, mono = osl_audioVoices[channel].mono;
		if (next) {
			setChannelSound(channel, next);
			osl_audioActive[channel] = 1;
		} else {
			osl_audioVoices[channel].sound = NULL;
		}
		sceKernelCpuResumeIntr(flags);
		if (unused)
			VirtualFileClose(unused);
		if (sound)
			oslAudioFreeSound(sound);
		if (next) {
			// The new sound may use another buffer size or channel count; a refusal ends it
			int handle = AudioStatus[channel].handle;
			if ((osl_audioVoices[channel].numSamples != numSamples &&
			     oslAudioConfigureChannel(handle, 0, osl_audioVoices[channel].numSamples) < 0) ||
			    (osl_audioVoices[channel].mono != mono && oslAudioConfigureChannel(handle, 1, osl_audioVoices[channel].mono) < 0))
				oslAudioDeleteChannel(channel);
			oslAudioStartSound(next, channel);
			AudioStatus[channel].callback = oslAudioCallback;
		}
	} while (next);

	AudioStatus[channel].callback = NULL;
	if (AudioStatus[channel].handle != -1) {
		sceAudioChRelease(AudioStatus[channel].handle);
		AudioStatus[channel].handle = -1;
	}
	AudioStatus[channel].threadhandle = -1;
	osl_audioActive[channel] = 0;

	sceKernelExitDeleteThread(0);
	return 0;
}

void oslAudioDeleteChannel(int i) {
	// Only a running thread (1 playing, 2 paused, 3 suspended) can end the channel: marking an
	// idle or ending one would leave it at -1 forever
	if (osl_audioActive[i] > 0)
		osl_audioActive[i] = -1;
}

// Whether the caller is one of the channel threads (e.g. in an endCallback)
static int oslAudioOnChannelThread(void) {
	int self = sceKernelGetThreadId();
	for (int i = 0; i < OSL_NUM_AUDIO_CHANNELS; i++)
		if (AudioStatus[i].threadhandle == self) return 1;
	return 0;
}

// Ends the channel and returns once its thread has exited. Plays handed to it meanwhile are
// dropped: the caller's own play comes later, so it wins.
static void oslAudioEndChannel(int i) {
	for (;;) {
		unsigned int flags = sceKernelCpuSuspendIntr();
		audio_pendingPlay[i] = NULL;
		oslAudioDeleteChannel(i);
		int active = osl_audioActive[i];
		sceKernelCpuResumeIntr(flags);
		if (active == 0)
			return;
		sceKernelDelayThread(1000); // a busy loop would starve a lower priority audio thread
	}
}

// Returns once the channel's thread no longer uses s: it has exited, or moved on to a sound
// handed over to it
static void oslAudioWaitSoundOff(int i, OSL_SOUND *s) {
	for (;;) {
		unsigned int flags = sceKernelCpuSuspendIntr();
		int active = osl_audioActive[i];
		int done = active == 0 || (active != -2 && osl_audioVoices[i].sound != s);
		sceKernelCpuResumeIntr(flags);
		if (done)
			return;
		sceKernelDelayThread(1000);
	}
}

// From an audio thread, a play on a busy voice can't wait for it to end (it may be the caller's
// own, or one whose end callback waits for the caller): that voice's thread starts s instead
// when its current sound ends. Returns whether the play was handed over.
static int oslAudioHandOverPlay(OSL_SOUND *s, int voice) {
	unsigned int flags = sceKernelCpuSuspendIntr();
	int active = osl_audioActive[voice];
	int handOver = active != 0 && active != -2 && osl_audioVoices[voice].sound != s;
	if (handOver) {
		audio_pendingPlay[voice] = s;
		oslAudioDeleteChannel(voice);
	}
	sceKernelCpuResumeIntr(flags);
	return handOver;
}

int oslAudioSetSoundFilename(OSL_SOUND *s, const char *filename) {
	// Threads OSLib creates have no current directory on the PSP, and one may reopen the file:
	// resolve a relative path now, unless the name is a virtual file's or its path doesn't fit
	char path[PATH_MAX];
	const char *name = filename;
	if (osl_defaultVirtualFileSource == VF_FILE && !oslFindFileInVirtualFilenameList(filename, VF_AUTO) &&
	    realpath(filename, path) && strlen(path) < sizeof(s->filename))
		name = path;
	s->suspendNumber = osl_suspendNumber;
	if (strlen(name) >= sizeof(s->filename)) {
		s->filename[0] = '\0';
		return 0;
	}
	strcpy(s->filename, name);
	return 1;
}

static int oslAudioNeedsReopen(OSL_SOUND *s) {
	return s->isStreamed && s->suspendNumber < osl_suspendNumber; // a standby closed its file
}

// Reopens a stream's file; for a resume, at the position the standby saved for channel
static VIRTUAL_FILE *oslAudioOpenAgain(const char *filename, int channel, int restart) {
	VIRTUAL_FILE *f = VirtualFileOpen((void *)filename, 0, VF_AUTO, VF_O_READ);
	if (f && !restart)
		VirtualFileSeek(f, osl_filesave[channel], SEEK_SET);
	return f;
}

// Hands channel's thread a reopened file for s (NULL if it couldn't be). Like the other requests
// to a channel's thread, only while the channel still plays s: its thread then installs or
// closes the file, but it may have ended or moved on while the file opened. A file handed
// before and not installed yet is closed: nothing ever read it.
static void oslAudioRequestReopen(OSL_SOUND *s, int channel, VIRTUAL_FILE *f, int number, int restart) {
	VIRTUAL_FILE *unused = f;
	unsigned int flags = sceKernelCpuSuspendIntr();
	if (osl_audioVoices[channel].sound == s) {
		unused = audio_reopenFile[channel];
		if (audio_reopen[channel] == s)
			restart |= audio_reopenRestart[channel];
		audio_reopen[channel] = s;
		audio_reopenFile[channel] = f;
		audio_reopenRestart[channel] = restart;
		audio_reopenNumber[channel] = number;
	}
	sceKernelCpuResumeIntr(flags);
	if (unused)
		VirtualFileClose(unused);
}

// Starts s from the beginning on voice. If a standby closed its file, the file is reopened here
// and installed by the channel's thread, before its next callback reads it.
static void oslAudioStartSound(OSL_SOUND *s, int voice) {
	if (!oslAudioNeedsReopen(s)) {
		s->playSound(s);
		return;
	}
	int number = osl_suspendNumber; // a standby during the reopen leaves it needing another
	VIRTUAL_FILE *f = oslAudioOpenAgain(s->filename, voice, 1);
	if (AudioStatus[voice].threadhandle == sceKernelGetThreadId())
		oslAudioInstall(s, voice, f, number, 1);
	else
		oslAudioRequestReopen(s, voice, f, number, 1);
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
		osl_audioVoices[i].sound = NULL;
		return -1; // Failed to reserve audio channel.
	}

	// Mark audio as ready and active.
	audio_ready = 1;
	osl_audioActive[i] = 1;

	// Create a thread name like "audiot0", "audiot1", etc.
	snprintf(threadName, sizeof(threadName), "audiot%d", i);

	// Create a kernel thread for the audio channel.
	AudioStatus[i].threadhandle = sceKernelCreateThread(threadName, (SceKernelThreadEntry) & oslAudioChannelThread, 0x10, 0x10000, 0, NULL);
	if (AudioStatus[i].threadhandle < 0) {
		// Thread creation failed, clean up the audio channel: no thread will ever end it.
		sceAudioChRelease(AudioStatus[i].handle);
		AudioStatus[i].handle = -1;
		osl_audioVoices[i].sound = NULL;
		osl_audioActive[i] = 0;
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
		osl_audioVoices[i].sound = NULL;
		return -1; // Failed to start thread.
	}

	return 0; // Successfully created and started the audio channel.
}

int oslAudioRecreateChannel(int i, int format, int numSamples, OSL_SOUND *s) {
	// If the channel already has the same number of samples and the same sound, just reconfigure it.
	if (osl_audioVoices[i].numSamples == numSamples && osl_audioVoices[i].sound == s) {
		// Replayed by the channel's own thread (an endCallback, e.g. the loop's) while a stop
		// is pending: the stop wins, or oslStopSound would wait for a channel kept alive
		if (osl_audioActive[i] < 0 && AudioStatus[i].threadhandle == sceKernelGetThreadId())
			return 0;
		audio_pendingPlay[i] = NULL; // this later play replaces one handed over
		// Update the channel configuration.
		sceAudioChangeChannelConfig(i, format);
		setChannelSound(i, s); // Update the sound properties for this channel.
		osl_audioActive[i] = 1; // Mark the channel as active.
		return 0; // Return early since the reconfiguration is sufficient.
	}

	// If the number of samples or the sound has changed, recreate the channel.
	oslAudioEndChannel(i);

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

void oslDecodeWav(unsigned int i, void *buf, unsigned int length) {
	unsigned int j, k, samples = 1 << osl_audioVoices[i].divider;
	unsigned short *data = (unsigned short *)buf, cur1, cur2;
	WAVE_SRC *wav = (WAVE_SRC *)osl_audioVoices[i].dataplus;
	unsigned char *streambuffer = NULL;
	int len;

	// Handle streamed audio
	if (wav->stream) {
		// Calculate bytes needed from file:
		// length = number of output samples
		// For mono: length samples * (bits/8) bytes
		// For stereo: length samples * 2 channels * (bits/8) bytes
		// For resampling (samples > 1): divide by samples since we duplicate

		int bytes_per_sample = wav->fmt.bits_sample >> 3;  // 2 for 16-bit
		int channels = (osl_audioVoices[i].mono == 0) ? 2 : 1;

		// How many source samples do we need?
		int source_samples = length / samples;  // Account for resampling

		// Total bytes = source_samples * channels * bytes_per_sample
		len = source_samples * channels * bytes_per_sample;

		// Allocate buffer dynamically instead of using alloca
		streambuffer = (unsigned char *)malloc(len);
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

	// Done with the stream buffer (NULL when not streamed). Free it before the end callback,
	// which may delete the sound: nothing below touches wav
	free(streambuffer);

	// If the chunk is finished, trigger the end callback, unless a stop is pending: the
	// stopping thread then owns the sound, which the callback could replay or delete
	if (wav->chunk_left <= 0) {
		if (osl_audioVoices[i].sound->endCallback && osl_audioActive[i] == 1) {
			if (osl_audioVoices[i].sound->endCallback(osl_audioVoices[i].sound, i)) {
				return;
			}
		}
		oslAudioDeleteChannel(i);
	}
}

/*
 * Function used to fill the audio buffer (44,100 Hz, 16 bits, Mono).
 * This function acts as a callback for audio processing.
 */
void oslAudioCallback(unsigned int i, void *buf, unsigned int length) {
	// Check if the sound object or audio callback is NULL to avoid null pointer dereference
	if (!osl_audioVoices[i].sound || !osl_audioVoices[i].sound->audioCallback) {
		oslAudioDeleteChannel(i); // Delete the channel if there's no valid sound or callback
		return;
	}

	// Call the audio callback for this channel
	if (!osl_audioVoices[i].sound->audioCallback(i, buf, length)) {
		// If the callback returns 0, the sound is finished. Check for the end callback, unless a
		// stop is pending: the stopping thread then owns the sound
		if (osl_audioVoices[i].sound->endCallback && osl_audioActive[i] == 1) {
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
			// Only a running stream (playing, paused or already suspended) is suspended: a stopping
			// or ending channel must end. A playing one waits for oslAudioVSync to reopen its file
			// (3); a paused one stays paused, and oslAudioVSync reopens its file too.
			unsigned int flags = sceKernelCpuSuspendIntr();
			s = osl_audioVoices[i].sound;
			int state = osl_audioActive[i];
			int suspend = s && s->isStreamed && state >= 1 && state <= 3;
			if (suspend && state == 1)
				osl_audioActive[i] = 3;
			sceKernelCpuResumeIntr(flags);
			if (suspend) {
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
		osl_audioActive[i] = 0;              // Mark audio channel as inactive
		osl_audioVoices[i].sound = NULL;     // Clear sound reference
		audio_pendingDelete[i] = NULL;
		audio_pendingPlay[i] = NULL;
		audio_reopen[i] = NULL;
		audio_reopenFile[i] = NULL;

		// Initialize audio status for each channel
		AudioStatus[i].handle = -1;          // No handle assigned yet
		AudioStatus[i].threadhandle = -1;    // No thread handle assigned yet
		AudioStatus[i].callback = NULL;      // No callback assigned
	}

	// Reset the audio state
	audio_ready = 0;
	memset(osl_audioVoices, 0, sizeof(osl_audioVoices)); // Clear all audio voices
	osl_suspendNumber = 0;                          // Reset suspend counter

#ifdef PSP
	osl_audioOldPowerCallback = osl_powerCallback;
	osl_powerCallback = oslAudioPowerCallback;
#endif

	osl_audioStandBy = 0;                          // Set standby to false
	return 0;
}

// Deinitialize the audio system. All sounds should be stopped before calling this.
void oslDeinitAudio() {
	// Stop all active audio channels
	for (int i = 0; i < OSL_NUM_AUDIO_CHANNELS; i++) {
		audio_pendingPlay[i] = NULL; // don't start a handed-over play after this
		oslAudioDeleteChannel(i); // Ensure each channel is properly deleted
	}

	// Restore the previous power callback, if applicable
	osl_powerCallback = osl_audioOldPowerCallback;

	// Mark the audio system as no longer ready
	audio_ready = 0;
}

// Weak, so oslLoadSoundFile doesn't link media.c (and -lpspaudiocodec) into every app: they are
// set once the app calls oslInitAudioME, which MP3 and AT3 need anyway and which lives in media.c
extern OSL_SOUND *oslLoadSoundFileMP3(const char *filename, int stream) __attribute__((weak));
extern OSL_SOUND *oslLoadSoundFileAT3(const char *filename, int stream) __attribute__((weak));

OSL_SOUND *oslLoadSoundFile(const char *filename, int stream) {
	if (filename == NULL || strlen(filename) < 4) {
		return NULL; // Invalid filename or file too short
	}

	// Determine if streaming is requested
	int isStream = (stream & OSL_FMT_STREAM) ? 1 : 0;

	// Memory stick names often come in upper case, so compare the extension without case
	const char *ext = filename + strlen(filename) - 4;
	if (!strcasecmp(ext, ".bgm")) {
		return oslLoadSoundFileBGM(filename, isStream);
	} else if (!strcasecmp(ext, ".wav")) {
		return oslLoadSoundFileWAV(filename, isStream);
	} else if (!strcasecmp(ext, ".mp3")) {
		return oslLoadSoundFileMP3 ? oslLoadSoundFileMP3(filename, stream) : NULL; // the ME loaders test the OSL_FMT_STREAM bit
	} else if (!strcasecmp(ext, ".at3")) {
		return oslLoadSoundFileAT3 ? oslLoadSoundFileAT3(filename, stream) : NULL;
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

	if (oslAudioOnChannelThread()) {
		// An audio thread cannot wait for another, but its target may still be decoding.
		unsigned int flags = sceKernelCpuSuspendIntr();
		int voice = oslGetSoundChannel(s);
		if (voice >= 0)
			audio_pendingDelete[voice] = s;
		sceKernelCpuResumeIntr(flags);
		if (voice >= 0)
			return;
	}

	oslAudioFreeSound(s);
}

// On channel's thread, between two callbacks (or before the first): makes f, reopened after a
// standby, s's file and frees the one the standby closed; then restarts s if asked.
static void oslAudioInstall(OSL_SOUND *s, int channel, VIRTUAL_FILE *f, int number, int restart) {
	// Requests match sounds by address: one freed during the reopen and replaced by a new sound at
	// the same address must not get its file. A new sound doesn't need a reopen.
	if (f && !oslAudioNeedsReopen(s)) {
		VirtualFileClose(f);
		f = NULL;
	}
	if (f) {
		VIRTUAL_FILE **w = s->reactiveSound(s, f);
		if (w) {
			VIRTUAL_FILE *old = *w;
			*w = f;
			s->suspendNumber = number;
			if (old)
				oslAudioDropFile(old);
		} else {
			VirtualFileClose(f); // a format that can't reopen its stream
		}
	}
	if (restart)
		s->playSound(s);
	// Play again once reopened, unless it was stopped or paused meanwhile
	unsigned int flags = sceKernelCpuSuspendIntr();
	if (osl_audioActive[channel] == 3 && !oslAudioNeedsReopen(s))
		osl_audioActive[channel] = 1;
	sceKernelCpuResumeIntr(flags);
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

	// Reopen the files of streams suspended (3) or paused (2) by a standby, where the standby left
	// them; their channel's thread installs them
	for (int i = 0; i < OSL_NUM_AUDIO_CHANNELS; i++) {
		char filename[sizeof(osl_audioVoices[i].sound->filename)];
		int number = osl_suspendNumber;
		// Copied with the interrupts off: an end callback may delete the sound meanwhile
		unsigned int flags = sceKernelCpuSuspendIntr();
		int state = osl_audioActive[i];
		OSL_SOUND *s = osl_audioVoices[i].sound;
		int reopen = 0;
		if ((state == 2 || state == 3) && s) {
			if (!oslAudioNeedsReopen(s)) {
				if (state == 3) osl_audioActive[i] = 1;
			} else if (audio_reopen[i] != s) {
				memcpy(filename, s->filename, sizeof(filename));
				reopen = 1;
			}
		}
		sceKernelCpuResumeIntr(flags);
		if (reopen) {
			VIRTUAL_FILE *f = oslAudioOpenAgain(filename, i, 0);
			if (f)
				oslAudioRequestReopen(s, i, f, number, 0);
		}
	}
}

void oslPlaySound(OSL_SOUND *s, int voice) {
	// Ensure the voice (channel) is within a valid range (0 to 7)
	if (voice < 0 || voice >= OSL_NUM_AUDIO_CHANNELS || s == NULL) {
		return;
	}

	// Wait if the audio channel is in a transition state (-2: its thread is ending, or switching
	// to a handed-over sound)
	for (;;) {
		if (oslAudioOnChannelThread() && oslAudioHandOverPlay(s, voice))
			return;
		if (osl_audioActive[voice] != -2)
			break;
		sceKernelDelayThread(1000); // a busy loop would starve a lower priority audio thread
	}

	// Create the audio channel if it's not active, otherwise recreate or reuse it
	if (!osl_audioActive[voice]) {
		oslAudioCreateChannel(voice, s->mono, osl_audioVoices[voice].numSamples, s);
	} else {
		oslAudioRecreateChannel(voice, s->mono, osl_audioVoices[voice].numSamples, s);
	}

	oslAudioStartSound(s, voice);

	// Set the callback for the audio channel
	oslAudioSetChannelCallback(voice, oslAudioCallback);
}

void oslStopSound(OSL_SOUND *s) {
	// Cancel plays of s not started yet, and find the channel playing it, together: a channel
	// thread may be switching to s right now
	unsigned int flags = sceKernelCpuSuspendIntr();
	for (int i = 0; i < OSL_NUM_AUDIO_CHANNELS; i++)
		if (audio_pendingPlay[i] == s) audio_pendingPlay[i] = NULL;
	int voice = oslGetSoundChannel(s);
	sceKernelCpuResumeIntr(flags);

	if (voice >= 0)
		oslAudioDeleteChannel(voice);

	// A custom driver may need this hook to cancel a read before its callback can finish.
	if (s && s->stopSound) {
		s->stopSound(s);
	}

	// Audio threads must not wait for themselves or for each other.
	if (voice >= 0 && !oslAudioOnChannelThread())
		oslAudioWaitSoundOff(voice, s);
}

void oslPauseSound(OSL_SOUND *s, int pause) {
	// Atomic with the audio threads, which may stop the channel at any time: only a playing (1),
	// paused (2) or suspended (3) channel changes, a stopping or ending one must end
	unsigned int flags = sceKernelCpuSuspendIntr();
	int voice = oslGetSoundChannel(s);
	if (voice >= 0) {
		int state = osl_audioActive[voice];
		if (state >= 1 && state <= 3) {
			int paused = pause == -1 ? state != 2 : pause != 0;
			// A stream whose file a standby closed waits for its file to be reopened (3)
			int suspended = oslAudioNeedsReopen(s);
			osl_audioActive[voice] = paused ? 2 : suspended ? 3 : 1;
		}
	}
	sceKernelCpuResumeIntr(flags);
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

	WAVE_SRC *wav = (WAVE_SRC *)s->dataplus;

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

int oslAudioCallback_AudioCallback_WAV(unsigned int i, void *buf, unsigned int length) {
	// Decode WAV audio data into the provided buffer
	oslDecodeWav(i, buf, length);
	return 1;
}

VIRTUAL_FILE **oslAudioCallback_ReactiveSound_WAV(OSL_SOUND *s, VIRTUAL_FILE *f) {
	// Ensure the sound and its associated data (dataplus) are valid
	if (s == NULL || s->dataplus == NULL) {
		return NULL; // Invalid sound or data, return NULL
	}

	// Return a pointer to the file pointer for the reactive sound
	VIRTUAL_FILE **w = (VIRTUAL_FILE **)&((WAVE_SRC *)s->dataplus)->fp;
	return w;
}

VIRTUAL_FILE *oslAudioCallback_StandBy_WAV(OSL_SOUND *s) {
	// Ensure the sound and its associated data (dataplus) are valid
	if (s == NULL || s->dataplus == NULL) {
		return NULL; // Invalid sound or data, return NULL
	}

	// Return the file pointer for the sound's virtual file
	VIRTUAL_FILE *f = (VIRTUAL_FILE *)((WAVE_SRC *)s->dataplus)->fp;
	return f;
}

void oslAudioCallback_DeleteSound_WAV(OSL_SOUND *s) {
	// Ensure the sound object and its associated data (dataplus) are valid
	if (s == NULL || s->dataplus == NULL) {
		return; // Invalid sound or data, exit early
	}

	WAVE_SRC *wav = (WAVE_SRC *)s->dataplus;

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
	s = (OSL_SOUND *)malloc(sizeof(OSL_SOUND));
	if (!s) {
		goto error; // Memory allocation failure
	}

	// Zero-initialize the OSL_SOUND structure
	memset(s, 0, sizeof(OSL_SOUND));

	// Allocate memory for the WAVE_SRC structure
	wav = (WAVE_SRC *)malloc(sizeof(WAVE_SRC));
	if (!wav) {
		free(s);
		goto error; // Memory allocation failure
	}

	// Open the WAV file
	if (open_wave_src(wav, filename) < 0) {
		free(s);
		free(wav);
		goto error; // Failed to open the WAV file
	}

	// Set audio format (mono or stereo) based on the WAV file's channel count
	if (wav->fmt.channels == 1) {
		s->mono = 0x10;                                 // OSL_AUDIO_FORMAT_MONO
	} else {
		s->mono = 0x00;                                 // OSL_AUDIO_FORMAT_STEREO
	}
	wav->basefp = VirtualFileTell(wav->fp);
	wav->chunk_base = wav->chunk_left;
	s->size = (int)wav->chunk_left; // Set the sound size
	s->endCallback = NULL;
	s->volumeLeft = s->volumeRight = OSL_VOLUME_MAX; // Default volume
	s->format = 0; // Default format
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

	// If the sound is streamed, remember its file to reopen it after a standby
	if (s->isStreamed) {
		if (!oslAudioSetSoundFilename(s, filename)) {
			oslFatalError("Sound file name too long!");
		}
	} else {
		// Allocate memory for the in-memory WAV data
		wav->database = (unsigned char *)malloc(s->size);
		if (!wav->database) {
			free(s);
			close_wave_src(wav);
			free(wav);
			goto error; // Memory allocation failure for in-memory data
		}

		// Read the WAV file data into memory
		VirtualFileRead(wav->database, s->size, 1, wav->fp);
		wav->data = wav->database;
		close_wave_src(wav); // Close the file as it is no longer needed
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
