/**
 * @file bgm.h
 * @brief Defines the BGM_FORMAT_HEADER structure for BGM file format in OSLib.
 */

#ifndef _OSL_BGM_H_
#define _OSL_BGM_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct BGM_FORMAT_HEADER
 * @brief Header structure for BGM (Background Music) files in the OSLib library.
 * 
 * This structure provides metadata about the BGM file, including version information,
 * format specifications, and audio properties.
 */
typedef struct {
    /**
     * @brief Version string of the BGM format.
     * 
     * This is always set to "OSLBGM v01" to indicate the version of the BGM format.
     * 
     * @note The string is null-terminated and occupies 11 bytes.
     */
    char strVersion[11];

    /**
     * @brief Format version.
     * 
     * Indicates the version of the BGM format. This value is always set to 1.
     * 
     * @note The value should always be 1.
     */
    int format;

    /**
     * @brief Audio sampling rate.
     * 
     * Specifies the number of samples per second (in Hz) used in the audio data.
     */
    int sampleRate;

    /**
     * @brief Number of audio channels.
     * 
     * Specifies the number of audio channels:
     * - 1 for mono
     * - 2 for stereo
     */
    unsigned char nbChannels;

    /**
     * @brief Reserved space.
     * 
     * This field is reserved for future use and should be set to zero. It is not used in
     * the current format version.
     * 
     * @note The reserved field occupies 32 bytes.
     */
    unsigned char reserved[32];
} BGM_FORMAT_HEADER;

#ifdef __cplusplus
}
#endif

#endif /* _OSL_BGM_H_ */
