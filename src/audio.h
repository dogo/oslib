/**
 * @file audio.h
 * @brief Header file for audio management in OSLib, providing structures and functions for playing various sound formats.
 */

#ifndef AUDIO_H
#define AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup audio Audio
 *  @brief Audio management functionalities in OSLib.
 *  @{
 */

/**
 * @brief Represents a sound object in OSLib.
 * 
 * This structure can handle different sound formats and supports both streamed and normal playback.
 * Users can implement custom drivers for other sound types. However, compatibility with future versions is not guaranteed.
 * Please release your source when creating custom implementations.
 */
typedef struct OSL_SOUND {
    char filename[64];  //!< Filename for reopening after standby, used if streamed.
    void *data;         //!< Pointer to sound data, format-specific.
    void *dataplus;     //!< Pointer to extended sound data.
    int baseoffset;     //!< Offset in the file to begin playback, excludes header.
    int format;         //!< Sound format identifier.
    int divider;        //!< Divider for playback rate adjustment.
    int size;           //!< Size of the sound data in bytes.
    int mono;           //!< Mono (0x10) or Stereo (0) output.
    int isStreamed;     //!< Indicates if the sound is streamed.
    u16 volumeLeft;     //!< Volume for the left channel.
    u16 volumeRight;    //!< Volume for the right channel.
    int suspendNumber;  //!< Suspend management counter.
    int (*endCallback)(struct OSL_SOUND*, int); //!< Callback when playback finishes.
    u8 userdata[32];    //!< Custom user data.
    int numSamples;     //!< Number of samples per read.
    void (*playSound)(struct OSL_SOUND*);       //!< Function to start playing the sound.
    void (*stopSound)(struct OSL_SOUND*);       //!< Function to stop playing the sound.
    int (*audioCallback)(unsigned int, void*, unsigned int); //!< Buffer fill callback.
    VIRTUAL_FILE* (*standBySound)(struct OSL_SOUND*);        //!< Handle entering standby.
    VIRTUAL_FILE** (*reactiveSound)(struct OSL_SOUND*, VIRTUAL_FILE*); //!< Handle exiting standby.
    void (*deleteSound)(struct OSL_SOUND*);     //!< Function to delete the sound object.
} OSL_SOUND;

/** @brief Channel information for internal system use only.
 */
typedef struct {
    int active;         //!< Active state of the channel.
    int threadhandle;   //!< Handle for the associated thread.
    int handle;         //!< Generic handle, context-specific.
    void (*callback)(unsigned int channel, void *buf, unsigned int reqn); //!< Callback for this channel.
    int inProgress;     //!< Flag to indicate if processing is ongoing.
} osl_audio_channelinfo;

/** @brief User-facing channel representation, primarily for sound drivers.
 */
typedef struct {
    void *data;         //!< Sound data pointer.
    void *dataplus;     //!< Extended sound data pointer.
    int format;         //!< Format of the sound.
    int divider;        //!< Divider for rate adjustment.
    int size;           //!< Size of sound data.
    int mono;           //!< Mono or stereo output.
    int isStreamed;     //!< Streaming state.
    int numSamples;     //!< Samples per buffer fill.
    OSL_SOUND *sound;   //!< Pointer to associated OSL_SOUND object.
} OSL_AUDIO_VOICE;


/** @defgroup audio_general General Audio Tasks
 *  @brief Functions for general audio management tasks.
 *  @{
 */

/**
 * @brief Initializes the audio system.
 * 
 * This function must be called before any other audio-related functions to ensure proper operation.
 */
extern int oslInitAudio();

/**
 * @brief Deinitializes the audio system.
 * 
 * Call this function to stop all audio output and free resources. Make sure to delete all sound objects before calling this function.
 */
extern void oslDeinitAudio();

/**
 * @brief Initializes Media Engine audio support.
 * 
 * This function is required for playing AT3 and MP3 files and may need additional privileges depending on the firmware version.
 * @param formats Bitmask of formats to initialize, see oslInitAudioME_formats.
 */
extern void oslInitAudioME(int formats);

/** Number of audio channels. No more than 8 sounds can be played at once! */
#define OSL_NUM_AUDIO_CHANNELS 8
/** This is the default volume for audio channels. Though the real maximum value is 0xffff, this value is the maximum value before distorsion may happen. */
#define OSL_VOLUME_MAX 0x8000

/**
 * @brief Sets the default number of samples per audio buffer read.
 * 
 * Increasing this number reduces CPU usage but can cause longer blocks of audio processing.
 * @param num New default number of samples per buffer.
 */
#define oslAudioSetDefaultSampleNumber(num) (osl_audioDefaultNumSamples = num)

extern int osl_audioDefaultNumSamples; //!< Default number of samples per buffer, initialized to 512.

/** @} */ // end of audio_general

/** @defgroup audio_load Sound Loading
 *  @brief Functions for loading and managing sounds.
 *  @{
 */

/**
 * @brief Loads a sound file and determines its format based on the file extension.
 * 
 * Supported formats include WAV, MP3, AT3, and BGM. The function selects streaming or full memory loading based on the 'stream' parameter.
 * @param filename Path to the sound file.
 * @param stream Streaming mode; OSL_FMT_STREAM for streaming, OSL_FMT_NONE for full loading.
 * @return Pointer to the loaded sound object.
 */
extern OSL_SOUND *oslLoadSoundFile(const char *filename, int stream);

/**
 * @brief Loads a WAV sound file.
 *
 * This function specifically handles the loading of WAV sound files. It determines whether to load the file entirely into memory or to stream it from disk based on the 'stream' parameter. For more details on the parameters and the behavior regarding streamed versus fully loaded sounds, refer to oslLoadSoundFile().
 * 
 * WAV files are a common uncompressed audio format that offers high fidelity, making them suitable for short sound effects where quality is a priority over file size.
 * 
 * @param filename Path to the WAV sound file. It should be a valid file stored on the memory stick. Alternate file sources have not been tested and may not work properly.
 * @param stream Determines the mode of operation:
 *               - OSL_FMT_STREAM: Sound is streamed from the file, minimizing memory usage but requiring more CPU resources.
 *               - OSL_FMT_NONE: Sound is completely loaded into memory, providing faster access at the cost of increased memory usage.
 * @return Pointer to the loaded OSL_SOUND structure, or NULL if loading fails.
 *
 * @see oslLoadSoundFile
 */
extern OSL_SOUND *oslLoadSoundFileWAV(const char *filename, int stream);

/**
 * @brief Loads a BGM sound file.
 *
 * This function is designed to load BGM (Background Music) files, a custom audio format specific to OSLib. The BGM format is optimized for space efficiency, storing sound data in a mono format which takes up less room compared to uncompressed formats like WAV.
 * 
 * The function determines whether to stream the sound or load it entirely into memory based on the 'stream' parameter. Streaming uses less memory but requires more CPU resources to handle real-time decoding and playback.
 * 
 * Encoders for creating BGM files and additional tools for handling them can be found in the OSLib distribution. Additional sound formats and utilities are provided in the OSTools extension library.
 * 
 * @param filename Path to the BGM sound file. Ensure that this is a valid path on the memory stick as alternate storage locations have not been extensively tested.
 * @param stream Determines the loading method:
 *               - OSL_FMT_STREAM: Stream the sound from the file, which uses less memory.
 *               - OSL_FMT_NONE: Load the sound entirely into memory, which is faster but uses more memory.
 * @return Pointer to the loaded OSL_SOUND structure, or NULL if the loading fails.
 *
 * @see oslLoadSoundFile
 */
extern OSL_SOUND *oslLoadSoundFileBGM(const char *filename, int stream);

/**
 * @brief Loads a MOD sound file.
 *
 * This function loads MOD format sound files, including variations like .mod, .it, .s3m, and .xm. MOD files are known for their tracker formats, widely used for creating music with pattern sequences.
 * It requires linking with the MikMod library (-lmikmod). Note that OSLib currently supports only one MOD sound being played at a time; attempting to play multiple MOD sounds concurrently may result in
 * unexpected behavior such as increased playback speed and volume.
 * 
 * @warning Streaming is not supported for MOD files. Always set the `stream` parameter to OSL_FMT_NONE to ensure the sound is loaded entirely into RAM. Using OSL_FMT_STREAM can lead to incompatibilities in future OSLib versions.
 * 
 * @warning MOD playback is resource-intensive, significantly impacting memory usage and CPU load. It can increase CPU usage by up to 50% with certain tracks. If your application is CPU-intensive, consider using a different sound format or adjusting the MOD sample rate with #oslSetModSampleRate to reduce the load.
 * 
 * It's less of an issue in applications that are GPU-intensive, where CPU cycles are available during GPU waits. Adjustments to the MOD playback settings should be carefully managed to balance performance and audio quality.
 *
 * @param filename Path to the MOD file. The file should be stored locally on the device as streaming is not supported.
 * @param stream Must be OSL_FMT_NONE. Streaming is not supported for MOD files due to their resource-intensive nature.
 * 
 * @return Pointer to the loaded OSL_SOUND structure, or NULL if the file fails to load. Ensure proper error handling in your application.
 *
 * @note Linking with -lmikmod is required to use this functionality.
 */
OSL_SOUND *oslLoadSoundFileMOD(const char *filename, int stream);

/**
 * @brief Loads an MP3 sound file.
 *
 * This function loads an MP3 sound file into the OSLib sound system. MP3 is a widely-used compressed audio format that allows for efficient storage of high-quality audio. Before calling this function,
 * it is mandatory to initialize the Media Engine audio subsystem by calling #oslInitAudioME in kernel mode. Failing to do so will result in a program crash, as MP3 playback relies on features available only
 * in kernel mode.
 *
 * @param filename Path to the MP3 file. The file should be accessible on the device, and the path must be correctly specified.
 * @param stream Determines the loading behavior:
 *               - OSL_FMT_STREAM: Stream the sound from the file, using less memory but requiring more CPU for real-time decoding.
 *               - OSL_FMT_NONE: Load the entire sound file into memory, which uses more memory but less CPU during playback.
 *
 * @return Pointer to the loaded OSL_SOUND structure, or NULL if the file fails to load or initialization requirements are not met.
 *
 * @warning Ensure that #oslInitAudioME has been called in kernel mode prior to using this function.
 */
OSL_SOUND *oslLoadSoundFileMP3(const char *filename, int stream);

/**
 * @brief Loads an AT3 (ATRAC3) sound file.
 *
 * This function handles the loading of ATRAC3 format sound files, which are proprietary audio formats developed by Sony that offer good compression rates and sound quality. Similar to MP3, initializing the
 * Media Engine audio subsystem with #oslInitAudioME in kernel mode is required before using this function to prevent the program from crashing.
 *
 * @param filename Path to the AT3 file. Ensure the file is stored locally and the path is correctly specified for successful loading.
 * @param stream Determines the loading method:
 *               - OSL_FMT_STREAM: Stream the sound from the file. This method minimizes memory usage but might increase CPU load due to ongoing decoding.
 *               - OSL_FMT_NONE: Load the sound completely into memory for quicker access at the cost of increased memory usage.
 *
 * @return Pointer to the loaded OSL_SOUND structure, or NULL if the file fails to load or if pre-conditions are not met.
 *
 * @warning The Media Engine audio subsystem must be initialized in kernel mode prior to calling this function to ensure proper functionality and to avoid crashes.
 */
OSL_SOUND *oslLoadSoundFileAT3(const char *filename, int stream);

/**
 * @brief Sets the sample rate for MOD file playback in OSLib.
 *
 * This function configures the sample rate for MOD file audio playback. Adjusting the sample rate can significantly affect both the CPU load required for decoding the audio and the playback quality. The function also requires setting a shift value to adjust the playback speed relative to the sample rate to match the internal PSP audio capabilities.
 *
 * @param freq Sample rate in samples per second. The higher the sample rate, the higher the CPU load:
 *             - 44100 Hz for high quality (default)
 *             - 22050 Hz for medium quality
 *             - 11025 Hz for low quality
 *             Decreasing the sample rate reduces CPU load but also audio quality.
 * @param stereo Currently, the only supported value is 1, indicating stereo output.
 * @param shift Sets the playback speed adjustment. It compensates for sample rate changes to maintain proper playback speed:
 *              - 0: Normal speed (use with 44100 Hz)
 *              - 1: Half speed (use with 22050 Hz to normalize speed when played at 44100 Hz)
 *              - 2: Quarter speed (use with 11025 Hz to normalize speed when played at 44100 Hz)
 *              Values are powers of two; the actual playback speed is divided by 2^shift.
 *
 * @note Setting sample rates other than 44100, 22050, or 11025 Hz is not recommended as it can lead to untested and unpredictable behavior. Future versions of OSLib might not support such customizations.
 * 
 * @code
 * // Examples of typical usage:
 * oslSetModSampleRate(11025, 1, 2); // Very low CPU usage, reduced sound quality
 * oslSetModSampleRate(22050, 1, 1); // Balanced CPU usage and sound quality
 * oslSetModSampleRate(44100, 1, 0); // High CPU usage, best sound quality
 *
 * // Examples of advanced, non-recommended usage:
 * oslSetModSampleRate(22050, 1, 0); // Plays at double speed
 * oslSetModSampleRate(33075, 1, 0); // Plays at 1.5 times normal speed
 * oslSetModSampleRate(11025, 1, 1); // Plays at double speed, low quality
 * @endcode
 */
void oslSetModSampleRate(int freq, int stereo, int shift);

/** @} */ // end of audio_load

/**
 * @defgroup audio_play Audio Player
 * @brief Provides functionalities for audio playback within OSLib.
 * @{
 */

/**
 * @brief Plays a sound on a specified channel.
 *
 * This function plays a sound on one of the available audio channels, numbered from 0 to 7. If a sound is already playing on the specified channel, it will be stopped and replaced by the new sound. This feature allows up to 8 sounds to be played simultaneously, with each channel capable of handling one sound at a time.
 *
 * The use of multiple channels enables complex audio scenarios where various sounds do not interfere with each other unless explicitly programmed to do so. For example, background music can be played on one channel, while sound effects like jumps or coin pickups can be managed on others.
 *
 * @param s Pointer to an OSL_SOUND structure representing the sound to be played.
 * @param voice Channel number on which to play the sound. Valid values are from 0 to 7.
 *
 * @code
 * // Example of loading and playing different sounds on separate channels:
 * OSL_SOUND *music, *coin, *jump, *stomp;
 * // Assume sounds are already loaded into these pointers.
 *
 * // Play background music on channel 0
 * oslPlaySound(music, 0);
 *
 * // Play a coin pickup sound effect on channel 1
 * oslPlaySound(coin, 1);
 *
 * // Attempt to play another coin sound on channel 1, which stops the first coin sound
 * oslPlaySound(coin, 1);
 *
 * // Play a jump sound effect on channel 2, allowing it to play alongside the music and coin sound
 * oslPlaySound(jump, 2);
 *
 * // Replace the coin sound with a stomp sound effect on channel 1
 * oslPlaySound(stomp, 1);
 * @endcode
 *
 * The example demonstrates managing different sound effects and music tracks, showing how replacing sounds on the same channel can control audio playback dynamically.
 */
extern void oslPlaySound(OSL_SOUND *s, int voice);

/**
 * @brief Stops a currently playing sound.
 *
 * This function immediately stops the playback of a specified sound. It is useful for managing audio playback dynamically, allowing you to halt sound effects or music based on game events or user actions.
 * After stopping, the sound can be restarted using oslPlaySound() from the beginning or manipulated further if needed.
 *
 * @param s Pointer to an OSL_SOUND structure representing the sound to stop. The sound must be currently playing; otherwise, the function has no effect.
 *
 * @note Stopping a sound does not free the memory associated with the sound; it only halts its playback. To completely release a sound resource, use oslDeleteSound().
 *
 * @code
 * OSL_SOUND *music;
 * // Assume music is loaded and playing.
 *
 * // Stop the music based on a user action or an in-game event
 * oslStopSound(music);
 *
 * // Music can be played again or manipulated further if needed
 * @endcode
 *
 * This function is part of the @ref audio_play group, which includes other audio control functions.
 */
extern void oslStopSound(OSL_SOUND *s);

/**
 * @brief Pauses, resumes, or toggles a sound.
 *
 * This function controls the pause state of a specified sound. It can pause, resume, or toggle the sound's playback based on the 'pause' parameter. This allows for dynamic control over sound playback,
 * useful in various application scenarios such as pausing the game or responding to user interactions.
 *
 * @param s Pointer to an OSL_SOUND structure representing the sound to control.
 * @param pause Control the sound's playback state:
 *              - 1: Pause the sound. If the sound is already paused, it remains paused.
 *              - 0: Resume the sound from where it was paused.
 *              - -1: Toggle between pause and play. If the sound is playing, it will be paused; if paused, it will be resumed.
 *
 * @note If the sound is not currently playing, calling this function with a resume or toggle command will have no effect.
 *
 * @code
 * OSL_SOUND *backgroundMusic;
 * // Assume backgroundMusic is loaded and playing.
 *
 * // Pause the music
 * oslPauseSound(backgroundMusic, 1);
 *
 * // Later, resume the music
 * oslPauseSound(backgroundMusic, 0);
 *
 * // Toggle music playback
 * oslPauseSound(backgroundMusic, -1);
 * @endcode
 *
 * This function is part of the @ref audio_play group.
 */
extern void oslPauseSound(OSL_SOUND *s, int pause);

/**
 * @brief Deletes a sound, freeing associated memory.
 *
 * This function stops any currently playing sound and frees all resources associated with it. It is essential to call this function for every loaded sound when it is no longer needed, to prevent memory leaks
 * and to clean up the audio system resources.
 *
 * @param s Pointer to an OSL_SOUND structure that is to be deleted. If the sound is currently playing, it will be stopped first.
 *
 * @note After calling this function, the pointer 's' should no longer be used as it refers to freed memory.
 *
 * @code
 * OSL_SOUND *effect;
 * // Assume effect is loaded and may be playing.
 *
 * // Delete the sound and free resources
 * oslDeleteSound(effect);
 *
 * // The pointer 'effect' is now invalid and should not be used.
 * @endcode
 *
 * This function is also part of the @ref audio_play group.
 */
extern void oslDeleteSound(OSL_SOUND *s);

/**
 * @brief Synchronizes audio streaming with the PSP's power management.
 *
 * This function ensures that streamed audio files continue to play seamlessly after the PSP resumes from stand-by mode. It should be called regularly within the game loop to maintain audio playback synchronization, especially when handling streamed audio files.
 *
 * @note It's crucial to call this function periodically if your application uses streamed audio and the PSP may enter stand-by mode. Failure to call this function frequently enough can result in audio not resuming correctly after the PSP wakes up from stand-by.
 *
 * @warning This function is included in the call to #oslEndFrame, which handles frame-ending procedures including audio synchronization. If you are already calling #oslEndFrame in your loop, you do not need to call oslAudioVSync separately. For newer projects, it is recommended to use #oslEndFrame for a cleaner and more comprehensive approach to frame management and audio synchronization.
 *
 * @code
 * while (gameRunning) {
 *     // Game logic and rendering code here
 *
 *     // Synchronize audio playback with frame management
 *     oslEndFrame();
 *
 *     // Alternatively, if not using oslEndFrame:
 *     oslAudioVSync();
 * }
 * @endcode
 *
 * Including this function in your game loop ensures that audio playback issues do not occur after the PSP resumes from a standby state.
 */
extern void oslAudioVSync();

/**
 * @brief Sets a callback function to be invoked when a sound finishes playing.
 *
 * This macro assigns a user-defined callback function to a sound object, which is called when the sound reaches the end of its playback. The callback can determine whether to stop the sound, continue playback, or start a different sound on the same channel.
 *
 * @param s Pointer to an OSL_SOUND structure for which the callback is to be set.
 * @param fct Pointer to the callback function that will be called when the sound finishes. The function signature should be:
 *            int callback(OSL_SOUND* sound, int voice);
 *            - sound: a pointer to the OSL_SOUND structure.
 *            - voice: the channel number on which the sound was playing.
 *            The callback function must return:
 *            - 0: to indicate the sound should stop and the channel should be released.
 *            - 1: to continue playback, which could involve repeating the same sound or starting a different track.
 *
 * @code
 * // Define a callback function that replays the same sound
 * int repeatSound(OSL_SOUND *sound, int voice) {
 *     oslPlaySound(sound, voice);  // Replay the sound on the same channel
 *     return 1;  // Indicate that playback should continue
 * }
 *
 * OSL_SOUND *backgroundMusic;
 * // Assume backgroundMusic is already loaded and initialized
 *
 * // Set the repeat function as the end callback
 * oslSetSoundEndCallback(backgroundMusic, repeatSound);
 *
 * // Start playing the background music
 * oslPlaySound(backgroundMusic, 0);
 * @endcode
 *
 * This macro facilitates dynamic sound management, allowing for complex audio behavior based on application-specific logic.
 */
#define oslSetSoundEndCallback(s, fct)  (s->endCallback = (fct))

/**
 * @brief Sets the looping state of a sound.
 *
 * This macro configures whether a sound should loop continuously or stop after playing once. It uses the oslSetSoundEndCallback function to assign a loop control callback. If looping is enabled, the
 * oslSoundLoopFunc function is set as the callback, which automatically restarts the sound when it finishes playing. If looping is disabled, no callback is set, allowing the sound to stop after reaching the end.
 *
 * @param s Pointer to an OSL_SOUND structure representing the sound to configure.
 * @param loop Boolean value determining the looping behavior:
 *             - true (non-zero): Sound loops continuously.
 *             - false (zero): Sound does not loop and stops after playing once.
 *
 * @code
 * OSL_SOUND *music;
 * // Assume music is loaded and needs to be looped
 *
 * // Enable looping
 * oslSetSoundLoop(music, 1);
 *
 * // Start playing the music, it will now loop indefinitely
 * oslPlaySound(music, 0);
 *
 * // To stop looping, you can set it like this:
 * oslSetSoundLoop(music, 0);
 * // After the current playback, music will not loop but stop.
 * @endcode
 *
 * Utilizing this macro simplifies the process of managing sound playback, especially for background music or long-standing audio effects in games and multimedia applications.
 */
#define oslSetSoundLoop(s, loop)        oslSetSoundEndCallback(s, (loop) ? oslSoundLoopFunc : NULL)

/**
 * General audio format flags for basic settings and operations within OSLib.
 */
enum {
    OSL_FMT_NONE = 0  //!< Indicates no special format settings are applied. Used as a default or placeholder value.
};

/**
 * Mask flags used to isolate or filter out specific bits from format flags, typically used in bitwise operations.
 */
enum {
    OSL_FMT_MASK = 0xff  //!< Mask used to isolate or filter out specific bits from format flags, typically used in bitwise operations.
};

/**
 * Defines the audio output configuration and streaming capabilities for playback in OSLib.
 * These flags are used to set the properties of audio channels and to control how audio data is processed and delivered.
 */
enum {
    OSL_FMT_MONO = 0,     //!< Mono audio output. Indicates that audio is output through a single audio channel.
    OSL_FMT_STEREO = 0x200, //!< Stereo audio output. Indicates that audio is output through two separate audio channels, typically left and right channels.
    OSL_FMT_STREAM = 0x400 //!< Stream audio output. Indicates that audio data is streamed from a source rather than being fully preloaded, suitable for large audio files or network streams.
};

/**
 * Sample rate options for audio playback.
 */
enum {
    OSL_FMT_44K = 0,          //!< 44,100 Hz sample rate.
    OSL_FMT_22K = 1,          //!< 22,050 Hz sample rate.
    OSL_FMT_11K = 2           //!< 11,025 Hz sample rate.
};

/**
 * @enum oslInitAudioME_formats
 * Defines the formats that need to be initialized for Media Engine (ME) audio.
 * These are used with oslInitAudioME to prepare specific audio codecs.
 */
enum oslInitAudioME_formats {
    OSL_FMT_AT3 = 1,          //!< Atrac3 and Atrac3+ formats.
    OSL_FMT_MP3 = 2,          //!< MPEG Audio-Layer 3 format.
    OSL_FMT_ALL = 3           //!< All supported formats.
};

/** @} */ // end of audio_play

/**
 * @defgroup audio_adv Advanced Audio Management
 * @brief Functions and definitions for advanced audio management in OSLib.
 * @{
 */

/**
 * @brief Retrieves the channel number on which a specific sound is currently being played.
 *
 * This function checks the active audio channels and returns the channel number where the specified sound is playing. If the sound is not active on any channel, it returns -1, indicating that the sound is not currently being played.
 *
 * This is useful for managing audio dynamically within an application, allowing for checks on sound states before making decisions on audio control (such as stopping or altering playback).
 *
 * @param s Pointer to an OSL_SOUND structure representing the sound to check. The sound object should have been initialized and possibly started playing before calling this function.
 *
 * @return The channel number (ranging from 0 to the maximum number of channels minus one) on which the sound is playing, or -1 if the sound is not currently active.
 *
 * This function can be part of the @ref audio_adv group, as it provides essential information for advanced audio management tasks.
 */
extern int oslGetSoundChannel(OSL_SOUND *s);

/**
 * @brief Creates an audio channel.
 * @param i Channel index to be created.
 * @param format Audio format for the channel (e.g., OSL_FMT_MONO, OSL_FMT_STEREO).
 * @param numSamples Number of samples to handle per audio frame.
 * @param s Pointer to the OSL_SOUND structure associated with the channel.
 * @return Returns 0 on success or a negative error code on failure.
 */
extern int oslAudioCreateChannel(int i, int format, int numSamples, OSL_SOUND *s);

/**
 * @brief Recreates an existing audio channel with new parameters.
 * @param i Channel index to be recreated.
 * @param format New audio format for the channel.
 * @param numSamples New number of samples to handle per audio frame.
 * @param s Pointer to the OSL_SOUND structure associated with the channel.
 * @return Returns 0 on success or a negative error code on failure.
 */
extern int oslAudioRecreateChannel(int i, int format, int numSamples, OSL_SOUND *s);

/**
 * @brief Deletes an audio channel.
 * @param i Channel index to be deleted.
 */
extern void oslAudioDeleteChannel(int i);

/**
 * @brief Outputs audio to a channel, blocking until completion.
 * @param channel Channel index where audio should be output.
 * @param vol1 Volume level for the left audio channel.
 * @param vol2 Volume level for the right audio channel.
 * @param buf Pointer to the buffer containing audio data to be played.
 * @return Returns the number of samples played.
 */
extern int oslAudioOutBlocking(unsigned int channel, unsigned int vol1, unsigned int vol2, void *buf);

/**
 * @typedef oslAudioThreadfunc_t
 * @brief Defines a function type for audio threading.
 * @param args Number of arguments passed to the thread function.
 * @param argp Pointer to the argument list.
 * @return Should return 0 on success or a negative error code on failure.
 */
typedef int (* oslAudioThreadfunc_t)(int args, void *argp);

/**
 * @brief Arrays indicating active and busy status of each audio channel.
 */
extern volatile int osl_audioActive[OSL_NUM_AUDIO_CHANNELS], osl_audioBusy[OSL_NUM_AUDIO_CHANNELS];

/**
 * @brief Counter used to manage suspensions in audio playback.
 */
extern int osl_suspendNumber;

/**
 * @var osl_audioVoices
 * @brief Array of OSL_AUDIO_VOICE structures, representing each audio channel's current properties.
 *
 * This array holds information about all active audio voices within the system. Each entry in the array represents a channel and includes properties such as the currently playing sound.
 */
extern OSL_AUDIO_VOICE osl_audioVoices[OSL_NUM_AUDIO_CHANNELS];

/**
 * @fn int oslSoundLoopFunc(OSL_SOUND *s, int voice)
 * @brief Callback function that loops a sound indefinitely.
 *
 * This standard callback function is designed to loop a sound. It can be set as the sound end callback for a sound object using oslSetSoundLoop. When the sound reaches the end, this function is invoked to immediately restart it, creating a continuous loop effect.
 *
 * @param s Pointer to the OSL_SOUND structure of the sound that finished playing.
 * @param voice The channel number on which the sound was playing.
 * @return Always returns 1 to indicate that the sound should continue playing.
 */
extern int oslSoundLoopFunc(OSL_SOUND *s, int voice);

/** @} */ // end of audio_adv

/** @} */ // end of audio

#ifdef __cplusplus
}
#endif

#endif	// AUDIO_H
