#ifndef TEXT_H
#define TEXT_H

#include "intraFont.h"

/** @file text.h
 *  @brief Text handling functions in OSLib.
 *
 *  This file provides all text-related functions, structures, and macros
 *  necessary for rendering and managing fonts in OSLib, including
 *  loading custom fonts, handling intraFont, and drawing text on the screen.
 *
 *  Don't forget to check the 'Debug Console' section (subsection of the 'Main' section)
 *  for further debugging-related functions.
 */

/** @defgroup text Text functions
 *  @brief Text functions in OSLib.
 *
 *  This module provides various text-related functionalities, including font management,
 *  text drawing, and intraFont handling.
 *  @{
 */

/** @brief Loaded font structure.
 *
 *  This struct represents a loaded font in OSLib, holding its image, character widths,
 *  positions, and other properties like font type and intraFont data.
 */
typedef struct {
	OSL_IMAGE *img;                        //!< Image containing character sprites.
	unsigned char *charWidths;             //!< Table containing the width of each character (256 entries).
	unsigned short *charPositions;         //!< Position of characters in the image (16-bits: y:7, x:9).
	int isCharWidthConstant;               //!< Internal (to determine if charWidth needs to be freed).
	int charWidth;                         //!< Width of characters.
	int charHeight;                        //!< Height of characters (constant).
	int recentrage;                        //!< Added to text positions when drawing.
	unsigned char addedSpace;              //!< Space added between characters on the texture (allows making characters bigger than indicated by charWidths).
	int fontType;                          //!< Font type (OSL_FONT_OFT or OSL_FONT_INTRA).
	intraFont *intra;                      //!< IntraFont data.
} OSL_FONT;

/** @brief Font information type.
 *
 *  This structure holds information about a font, including character image data,
 *  palette data, and information about character sizes and widths.
 */
typedef struct {
	void *fontdata;                        //!< Character image data.
	short pixelFormat;                     //!< 1 = 1 bit (default).
	unsigned char *charWidths;             //!< Width of characters.
	int charWidth;                         //!< Default character width (if charWidths is NULL).
	int charHeight;                        //!< Height of characters (constant).
	int lineWidth;                         //!< Number of bytes of data per line.
	int recentrage;                        //!< Added to text positions for drawing text (recentering).
	unsigned char addedSpace;              //!< Space added between characters on the texture.
	unsigned short paletteCount;           //!< Palette count.
	unsigned long *paletteData;            //!< Palette data.
} OSL_FONTINFO;

/** @brief Header of a .oft file (Oslib FonT).
 *
 *  This structure represents the header for a font file used in OSLib.
 */
typedef struct {
	char strVersion[12];                   //!< "OSLFont v01".
	unsigned char pixelFormat;             //!< Number of bits per pixel (1 = 1 bit, default).
	unsigned char variableWidth;           //!< True if the first 256 bytes specify the character widths.
	int charWidth, charHeight;             //!< Mean character sizes (used for the console).
	int lineWidth;                         //!< Number of bytes of data per line.
	unsigned char addedSpace;              //!< Space added between characters on the texture.
	unsigned short paletteCount;           //!< Palette count.
	unsigned char reserved[29];            //!< Must be null (reserved).
} OSL_FONT_FORMAT_HEADER;

/** Current font.
 *
 *  This pointer holds the currently selected font for text rendering.
 *  You can read from it, but use #oslSetFont to modify it.
 */
extern OSL_FONT *osl_curFont;

/** @brief Sets the current font.
 *
 *  Sets the current font to the specified font. Use this function instead of
 *  directly modifying #osl_curFont for forward compatibility.
 *
 *  @param f Pointer to the new font to be set as the current font.
 *
 *  \code
 *  // Save the current font
 *  OSL_FONT *oldFont = osl_curFont;
 *  // Temporarily set another font
 *  oslSetFont(newFont);
 *  oslPrintf("Using the new font.\n");
 *  // Restore the old font
 *  oslSetFont(oldFont);
 *  oslPrintf("Using the normal font.\n");
 *  \endcode
 */
#define oslSetFont(f) (osl_curFont = f)

// System defines
#define OSL_TEXT_TEXWIDTH 512
#define OSL_TEXT_TEXDECAL 9

// Font type defines
/** @brief Font type for OFT. */
#define OSL_FONT_OFT 1
/** @brief Font type for INTRA. */
#define OSL_FONT_INTRA 2

// Character position macro
/** @brief Returns the position of a character 'i' in the font 'f' texture as a pair (x, y).
 *  @param f The font structure.
 *  @param i The character index.
 */
#define OSL_TEXT_CHARPOSXY(f,i) (f)->charPositions[i]&(OSL_TEXT_TEXWIDTH-1), ((f)->charPositions[i]>>OSL_TEXT_TEXDECAL)*(f)->charHeight

/** @brief Initializes the console.
 *
 *  This function is automatically called by #oslInitGfx, so manual initialization
 *  of the console is not required.
 */
extern void oslInitConsole();

/** @brief Loads a font from a file.
 *
 *  Loads a font file (.oft format) from the specified file path.
 *
 *  @param filename The path to the .oft font file.
 *  @return A pointer to the loaded font.
 *
 *  \code
 *  OSL_FONT *f = oslLoadFontFile("verdana.oft");
 *  oslSetFont(f);
 *  oslDrawString(0, 0, "Hello world using verdana!");
 *  \endcode
 */
extern OSL_FONT *oslLoadFontFile(const char *filename);

/** @brief Loads a font from a #OSL_FONTINFO file (located in RAM).
 *
 *  Use this ONLY with OFT fonts (doesn't work with intraFont). It is recommended
 *  to use oslLoadFontFile, which is more user-friendly.
 *
 *  @param fi A pointer to the OSL_FONTINFO structure in RAM.
 *  @return A pointer to the loaded font.
 */
extern OSL_FONT *oslLoadFont(OSL_FONTINFO *fi);

/** @brief Draws a character at the specified position.
 *
 *  This function draws a single character at the given coordinates (x, y). For drawing multiple characters,
 *  use oslDrawString for better performance.
 *
 *  @param x X position on the screen.
 *  @param y Y position on the screen.
 *  @param c The character to draw.
 */
extern void oslDrawChar(int x, int y, unsigned char c);

/** @brief Draws a string literal at the specified position.
 *
 *  This function draws a string at the given coordinates (x, y).
 *
 *  @param x X position on the screen.
 *  @param y Y position on the screen.
 *  @param str The string to draw.
 *
 *  \code
 *  oslDrawString(0, 0, "Test string");
 *  \endcode
 */
extern void oslDrawString(int x, int y, const char *str);

/** @brief Draws a string literal at the specified position, limited to a given width.
 *
 *  This function draws a string at the given coordinates (x, y) but limits the drawing
 *  to a specified width.
 *
 *  @param x X position on the screen.
 *  @param y Y position on the screen.
 *  @param width The maximum width for the string.
 *  @param str The string to draw.
 *
 *  \code
 *  oslDrawStringLimited(0, 0, 200, "Test string");
 *  \endcode
 */
extern void oslDrawStringLimited(int x, int y, int width, const char *str);

/** @brief Draws a formatted string literal at the specified position.
 *
 *  This function allows you to format a string and draw it at the specified coordinates (x, y).
 *
 *  @param x X position on the screen.
 *  @param y Y position on the screen.
 *  @param ... The formatted string to draw.
 *
 *  \code
 *  oslDrawStringf(0, 0, "Test string %i", 1);
 *  \endcode
 */
#define oslDrawStringf(x, y, ...) { char __str[1000]; sprintf(__str, __VA_ARGS__); oslDrawString(x, y, __str); }

/** @brief Outputs a text to the console at the current cursor position.
 *
 *  This function prints a string to the console at the current cursor position and moves
 *  the cursor. It is intended for debugging purposes.
 *
 *  @param str The string to print.
 */
extern void oslConsolePrint(const char *str);

/** @brief Sets the current text color.
 *
 *  Sets the color of the text to be drawn. This function does not work with intraFont
 *  (use oslIntraFontSetStyle instead).
 *
 *  @param color The color to set for the text.
 */
extern void oslSetTextColor(OSL_COLOR color);

/** @brief Sets the text background color.
 *
 *  Sets the background color for the text. Setting a transparent color (e.g., RGBA(0, 0, 0, 0))
 *  will disable the background, resulting in faster text rendering. This function does not work
 *  with intraFont (use oslIntraFontSetStyle instead).
 *
 *  @param color The background color to set.
 */
extern void oslSetBkColor(OSL_COLOR color);

/** @brief Draws a text box with automatic line wrapping.
 *
 *  Draws text within a rectangle defined by (x0, y0) and (x1, y1). The text will automatically wrap
 *  at the end of a line and move to the next line.
 *
 *  @param x0 X position of the top-left corner.
 *  @param y0 Y position of the top-left corner.
 *  @param x1 X position of the bottom-right corner.
 *  @param y1 Y position of the bottom-right corner.
 *  @param text The text to be drawn. Can contain \n characters for line breaks.
 *  @param format Reserved, set to 0.
 */
extern void oslDrawTextBox(int x0, int y0, int x1, int y1, const char *text, int format);

/** @brief Draws a text box with automatic word wrapping.
 *
 *  Similar to oslDrawTextBox, but wraps the text by words instead of characters.
 *
 *  @param x0 X position of the top-left corner.
 *  @param y0 Y position of the top-left corner.
 *  @param x1 X position of the bottom-right corner.
 *  @param y1 Y position of the bottom-right corner.
 *  @param text The text to be drawn. Can contain \n characters for line breaks.
 *  @param format Reserved, set to 0.
 */
extern void oslDrawTextBoxByWords(int x0, int y0, int x1, int y1, const char *text, int format);

/** @brief Deletes a font.
 *
 *  Deletes the specified font. Ensure that the font to be deleted is not currently selected
 *  (i.e., f != #osl_curFont), or the program will crash the next time you attempt to draw a character.
 *
 *  @param f The font to be deleted.
 */
extern void oslDeleteFont(OSL_FONT *f);

/** @brief Returns the width of a string in pixels.
 *
 *  Calculates the width of a string using the currently selected font. To center text horizontally
 *  or align it at the bottom of the screen, use this function to get the width.
 *
 *  @param str The string whose width is to be calculated.
 *  @return The width of the string in pixels.
 *
 *  \code
 *  const char *text = "© 2007 Brunni";
 *  int width = oslGetStringWidth(text);
 *  oslDrawString((SCREEN_WIDTH - width) / 2, SCREEN_HEIGHT - osl_curFont->charHeight, text);
 *  \endcode
 */
extern int oslGetStringWidth(const char *str);

/** @brief Returns the height of a text box.
 *
 *  Calculates the height (in pixels) required to draw a text box with the specified width,
 *  maxHeight, and format.
 *
 *  @param width The width of the text box.
 *  @param maxHeight The maximum height of the text box.
 *  @param text The text to be drawn.
 *  @param format Reserved, set to 0.
 *  @return The height of the text box in pixels.
 */
extern int oslGetTextBoxHeight(int width, int maxHeight, const char *text, int format);

/** Console horizontal position (in pixels).
 *  Use #oslMoveTo to move the cursor.
 */
extern int osl_consolePosX;

/** Console vertical position (in pixels). */
extern int osl_consolePosY;

/** @brief System OSLib font.
 *
 *  A pointer to the system font used by OSLib.
 */
extern OSL_FONT *osl_sceFont;

/** @brief Font to be loaded.
 *
 *  Holds information about the system font to be loaded.
 */
extern OSL_FONTINFO osl_sceFontInfo;

// IntraFont functions:
/** @brief Initializes intraFont.
 *
 *  Must be called before loading any pgf font. The same options will be applied
 *  to all intraFonts.
 *
 *  @param options INTRAFONT_XXX flags as defined in the code above.
 *  @return 0 on success, non-zero on failure.
 */
extern int oslIntraFontInit(unsigned int options);

/** @brief Loads a font from a pgf file (intraFont).
 *
 *  Use this function to load a pgf font with options different from those passed
 *  to oslIntraFontInit.
 *
 *  @param filename The path to the pgf font file.
 *  @param options The options for loading the font.
 *  @return A pointer to the loaded font.
 *
 *  \code
 *  OSL_FONT *f = oslLoadIntraFontFile("verdana.pgf", INTRAFONT_CACHE_ALL | INTRAFONT_STRING_UTF8);
 *  oslSetFont(f);
 *  oslDrawString(0, 0, "Hello world using verdana!");
 *  \endcode
 */
extern OSL_FONT *oslLoadIntraFontFile(const char *filename, unsigned int options);

/** @brief Loads an alternative intraFont file.
 *
 *  Use this function to load an alternative font file for an existing font.
 *
 *  @param font The font structure to update.
 *  @param filename The path to the alternative font file.
 */
extern void oslLoadAltIntraFontFile(OSL_FONT *font, const char *filename);

/** @brief Sets the style for a pgf font.
 *
 *  Applies the specified style to the pgf font. Works only with pgf fonts.
 *
 *  @param f The font structure.
 *  @param size The size of the font.
 *  @param color The color of the text.
 *  @param shadowColor The color of the text shadow.
 *  @param angle The rotation angle of the text.
 *  @param options Additional style options.
 */
extern void oslIntraFontSetStyle(OSL_FONT *f, float size, unsigned int color, unsigned int shadowColor, float angle, unsigned int options);

/** @brief Draws text along the baseline starting at x, y.
 *
 *  Draws text along the baseline starting at the specified (x, y) coordinates.
 *
 *  @param f A valid ::OSL_FONT with type ::OSL_FONT_INTRA.
 *  @param x X position on the screen.
 *  @param y Y position on the screen.
 *  @param width Column width for automatic line breaking (intraFontPrintColumn versions only).
 *  @param text The text to draw (ASCII & extended ASCII, S-JIS, or UTF-8 encoded).
 *  @return The x position after the last character.
 */
extern float oslIntraFontPrintColumn(OSL_FONT *f, float x, float y, float width, const char *text);

/** @brief Shuts down intraFont.
 *
 *  Cleans up and shuts down intraFont, freeing any allocated resources.
 */
extern void oslIntraFontShutdown();

/** @} */ // end of text group

#endif // TEXT_H
