/**
 * @file sfont.h
 * @brief Functions and structures to manage SFonts in the OSLib.
 *
 * This file contains the definitions and function declarations for working with SFonts,
 * which are bitmap-based fonts used in the OSLib.
 */

#ifndef _SFONT_H
#define _SFONT_H

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup SFont SFont
 *  @brief Functions to manage SFonts
 *  @{
 */

/**
 * @brief Struct describing a single SFont letter.
 */
typedef struct
{
    OSL_IMAGE *letter; /**< The image representing the letter. */
    unsigned int width; /**< The letter's width in pixels. */
} OSL_SFLETTER;

/**
 * @brief Struct describing a SFont.
 */
typedef struct
{
    OSL_SFLETTER *letters[256]; /**< Array of pointers to single letters. */
    int height; /**< The height of the font in pixels. */
    int lettersCount; /**< The total number of letters in the font. */
} OSL_SFONT;

/**
 * @brief Load an SFont from a file.
 * 
 * @param filename The name of the file to load the SFont from.
 * @param pixelFormat The pixel format to use for the SFont.
 * @return A pointer to the loaded OSL_SFONT, or NULL on failure.
 */
OSL_SFONT *oslLoadSFontFile(char *filename, int pixelFormat);

/**
 * @brief Delete a loaded SFont.
 * 
 * @param sfont The SFont to delete.
 */
void oslDeleteSFont(OSL_SFONT *sfont);

/**
 * @brief Get the text width using an SFont.
 * 
 * @param sfont The SFont to use for measurement.
 * @param text The text string to measure.
 * @return The width in pixels of the string rendered with the given SFont.
 */
int oslGetSFontTextWidth(OSL_SFONT *sfont, char *text);

/**
 * @brief Print a string using an SFont.
 * 
 * @param sfont The SFont to use for printing.
 * @param x The X position on the screen to start printing.
 * @param y The Y position on the screen to start printing.
 * @param text The text string to print.
 * @return The X position of the end of the rendered string.
 */
int oslSFontDrawText(OSL_SFONT *sfont, int x, int y, char *text);

/** @} */ // end of SFont

#ifdef __cplusplus
}
#endif

#endif /* _SFONT_H */
