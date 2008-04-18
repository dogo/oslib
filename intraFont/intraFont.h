/*
 * intraFont.h
 * This file is used to display the PSP's internal font (pgf firmware files)
 * intraFont Version 0.22 by BenHur - http://www.psp-programming.com/benhur
 *
 * Uses parts of pgeFont by InsertWittyName - http://insomniac.0x89.org
 *
 * This work is licensed under the Creative Commons Attribution-Share Alike 3.0 License.
 * See LICENSE for more details.
 *
 */

#ifndef __INTRAFONT_H__
#define __INTRAFONT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup intraFont Font Library
 *  @{
 */

#define INTRAFONT_ADVANCE_H     0x0000 //default: advance horizontaly from one char to the next
#define INTRAFONT_ADVANCE_V     0x0100
#define INTRAFONT_ALIGN_LEFT    0x0000 //default: left-align the text
#define INTRAFONT_ALIGN_CENTER  0x0200
#define INTRAFONT_ALIGN_RIGHT   0x0400
#define INTRAFONT_WIDTH_VAR     0x0000 //default: variable-width
#define INTRAFONT_WIDTH_FIX     0x0800 //set your custom fixed witdh to 24 pixels: INTRAFONT_WIDTH_FIX | 24 
                                       //(max is 255, set to 0 to use default fixed width, this width will be scaled by size)
#define INTRAFONT_ACTIVE        0x1000 //assumes the font-texture resides inside sceGuTex already, prevents unecessary reloading -> very small speed-gain									   
#define INTRAFONT_STRING_ASCII  0x0000 //default: interpret strings as ascii text
#define INTRAFONT_STRING_SJIS   0x2000 //interpret strings as shifted-jis (japanese)
#define INTRAFONT_CACHE_MED     0x0000 //default: 256x256 texture (enough to cache about 100 chars)
#define INTRAFONT_CACHE_LARGE   0x4000 //512x512 texture(enough to cache all chars of ltn0.pgf or ... or ltn15.pgf or kr0.pgf)
#define INTRAFONT_CACHE_ASCII   0x8000 //try to cache all ASCII chars during fontload (uses less memory and is faster to draw text, but slower to load font)
                                       //if it fails: (because the cache is too small) it will automatically switch to chache on-the-fly with a medium texture
									   //if it succeeds: (all chars and shadows fit into chache) it will free some now unneeded memory
#define INTRAFONT_CACHE_ALL     0xC000 //try to cache all chars during fontload (uses less memory and is faster to draw text, but slower to load font)
                                       //if it fails: (because the cache is too small) it will automatically switch to chache on-the-fly with a large texture
									   //if it succeeds: (all chars and shadows fit into chache) it will free some now unneeded memory

/** @note The following definitions are used internally by ::intraFont and have no other relevance.*/
#define PGF_BMP_H_ROWS    0x01
#define PGF_BMP_V_ROWS    0x02
#define PGF_BMP_OVERLAY   0x03
#define PGF_NO_EXTRA1     0x04
#define PGF_NO_EXTRA2     0x08
#define PGF_NO_EXTRA3     0x10
#define PGF_CHARGLYPH     0x20
#define PGF_SHADOWGLYPH   0x40 //warning: this flag is not contained in the metric header flags and is only provided for simpler call to intraFontGetGlyph - ONLY check with (flags & PGF_CHARGLYPH)
#define PGF_CACHED        0x80
#define PGF_WIDTH_MASK    0x00FF
#define PGF_OPTIONS_MASK  0x1FFF
#define PGF_STRING_MASK   0x2000
#define PGF_CACHE_MASK    0xC000


/**
 * A Glyph struct
 *
 * @note This is used internally by ::intraFont and has no other relevance.
 */
typedef struct {
	unsigned short x;         //in pixels
	unsigned short y;         //in pixels
	unsigned char width;      //in pixels
	unsigned char height;     //in pixels
	char left;                //in pixels
	char top;                 //in pixels
	unsigned char flags;
	unsigned short shadowID;  //to look up in shadowmap
	char advance;             //in quarterpixels
	unsigned long ptr;        //offset 
} Glyph;

/**
 * A PGF_Header struct
 *
 * @note This is used internally by ::intraFont and has no other relevance.
 */
typedef struct {
	unsigned short header_start;
	unsigned short header_len;
	char pgf_id[4];
	unsigned long revision;
	unsigned long version;
	unsigned long charmap_len;
	unsigned long charptr_len;
	unsigned long charmap_bpe;
	unsigned long charptr_bpe;
	unsigned char junk00[21];
	unsigned char family[64];
	unsigned char style[64];
	unsigned char junk01[1];
	unsigned short charmap_min;
	unsigned short charmap_max;
	unsigned char junk02[50];
	unsigned long fixedsize[2];
	unsigned char junk03[14];
	unsigned char table1_len;
	unsigned char table2_len;
	unsigned char table3_len;
	unsigned char advance_len;
	unsigned char junk04[102];
	unsigned long shadowmap_len;
	unsigned long shadowmap_bpe;
	unsigned char junk05[4];
	unsigned long shadowscale[2];
	//currently no need ;
} PGF_Header;

/**
 * A Font struct
 */
typedef struct {
	char *filename;
	unsigned char *fontdata;
	
	unsigned char *texture; /**<  The bitmap data  */
	unsigned int texWidth; /**<  Texture size (power2) */
	unsigned int texHeight; /**<  Texture height (power2) */	
	unsigned short texX;
	unsigned short texY;
	unsigned short texYSize;
	
	unsigned short n_chars;
	char advancex;            //in quarterpixels
	char advancey;            //in quarterpixels
	unsigned char charmap_compr_len; /**<length of compression info*/
	unsigned short *charmap_compr; /**< Compression info on compressed charmap*/	
	unsigned short *charmap; /**<  Character map */	
	Glyph* glyph; /**<  Character glyphs */
		
	unsigned short n_shadows;
	unsigned char shadowscale; /**<  shadows in pgf file (width, height, left and top properties as well) are scaled by factor of (shadowscale>>6) */	
	Glyph* shadowGlyph; /**<  Shadow glyphs */	
	
	float size;
	unsigned int color;
	unsigned int shadowColor;
	unsigned short options;
} intraFont;


/**
 * Initialise the Font library
 *
 * @returns 1 on success.
 */
int intraFontInit(void);

/**
 * Shutdown the Font library
 */
void intraFontShutdown(void);

/**
 * Load a pgf font.
 *
 * @param filename - Path to the font
 *
 * @param  options - INTRAFONT_XXX flags as defined above including flags related to CACHE (ored together)
 *
 * @returns A ::intraFont struct
 */
intraFont* intraFontLoad(const char *filename,unsigned short options);

/**
 * Free the specified font.
 *
 * @param font - A valid ::intraFont
 */
void intraFontUnload(intraFont *font);

/**
 * Activate the specified font.
 *
 * @param font - A valid ::intraFont
 */
void intraFontActivate(intraFont *font);

/**
 * Set font style
 *
 * @param font - A valid ::intraFont
 *
 * @param size - Text size
 *
 * @param color - Text color
 *
 * @param shadowColor - Shadow color (use 0 for no shadow)
 *
 * @param options - INTRAFONT_XXX flags as defined above except flags related to CACHE (ored together)
 */
void intraFontSetStyle(intraFont *font, float size, unsigned int color, unsigned int shadowColor, unsigned short options);

/**
 * Draw UCS-2 encoded text along the baseline starting at x, y.
 *
 * @param font - A valid ::intraFont
 *
 * @param x - X position on screen
 *
 * @param y - Y position on screen
 *
 * @param color - Text color
 *
 * @param shadowColor - Shadow color (use 0 for no shadow)
 *
 * @param text - UCS-2 encoded text to draw
 *
 * @returns The x position after the last char
 */
float intraFontPrintUCS2(intraFont *font, float x, float y, const unsigned short *text);

/**
 * Draw text along the baseline starting at x, y.
 *
 * @param font - A valid ::intraFont
 *
 * @param x - X position on screen
 *
 * @param y - Y position on screen
 *
 * @param text - (ASCII or S-JIS) Text to draw
 *
 * @returns The x position after the last char
 */
float intraFontPrint(intraFont *font, float x, float y, const char *text);

/**
 * Draw text along the baseline starting at x, y (with formatting).
 *
 * @param font - A valid ::intraFont
 *
 * @param x - X position on screen
 *
 * @param y - Y position on screen
 *
 * @param text - (ASCII or S-JIS) Text to draw
 *
 * @returns The x position after the last char
 */
float intraFontPrintf(intraFont *font, float x, float y, const char *text, ...);

/**
 * Measure a length of text if it were to be drawn
 *
 * @param font - A valid ::intraFont
 *
 * @param text - (ASCII or S-JIS) Text to measure
 *
 * @returns The total width of the text (until the first newline char)
 */
float intraFontMeasureText(intraFont *font, const char *text);

/**
 * Measure a length of UCS-2 encoded text if it were to be drawn
 *
 * @param font - A valid ::intraFont
 *
 * @param text - UCS-2 encoded text to measure
 *
 * @returns The total width of the text (until the first newline char)
 */
float intraFontMeasureTextUCS2(intraFont *font, const unsigned short *text); 

/** @} */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __INTRAFONT_H__
