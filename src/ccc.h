/**
 * @file ccc.h
 * @brief Header file for character code conversion functions in OSLib.
 *
 * This file provides macros that map to functions in BenHur's intraFont library for character code conversion.
 * These functions enable converting characters from various encodings to UCS2, setting error characters,
 * and managing the shutdown of the Character Code Conversion Library.
 */

/** @defgroup CharacterCodeConversion Character Code Conversion (ccc)
 *
 * Functions for character code conversion using BenHur's intraFont library.
 * @{
 */

#ifndef __OSL_CCC_H__
#define __OSL_CCC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "libccc.h"

/**
 * @brief Convert characters from Shift JIS encoding to UCS2 encoding.
 *
 * @param dst Output buffer for the converted string.
 * @param count Size of the output buffer.
 * @param str Input string in Shift JIS encoding.
 * @param cp Code page to use for conversion.
 * @return Number of converted character codes.
 */
#define oslCccSJIStoUCS2    cccSJIStoUCS2

/**
 * @brief Convert characters from GBK encoding to UCS2 encoding.
 *
 * @param dst Output buffer for the converted string.
 * @param count Size of the output buffer.
 * @param str Input string in GBK encoding.
 * @param cp Code page to use for conversion.
 * @return Number of converted character codes.
 */
#define oslCccGBKtoUCS2     cccGBKtoUCS2

/**
 * @brief Convert characters from Korean encoding to UCS2 encoding.
 *
 * @param dst Output buffer for the converted string.
 * @param count Size of the output buffer.
 * @param str Input string in Korean encoding.
 * @param cp Code page to use for conversion.
 * @return Number of converted character codes.
 */
#define oslCccKORtoUCS2     cccKORtoUCS2

/**
 * @brief Convert characters from BIG5 encoding to UCS2 encoding.
 *
 * @param dst Output buffer for the converted string.
 * @param count Size of the output buffer.
 * @param str Input string in BIG5 encoding.
 * @param cp Code page to use for conversion.
 * @return Number of converted character codes.
 */
#define oslCccBIG5toUCS2    cccBIG5toUCS2

/**
 * @brief Convert characters from UTF-8 encoding to UCS2 encoding.
 *
 * @param dst Output buffer for the converted string.
 * @param count Size of the output buffer.
 * @param str Input string in UTF-8 encoding.
 * @param cp Code page to use for conversion.
 * @return Number of converted character codes.
 */
#define oslCccUTF8toUCS2    cccUTF8toUCS2

/**
 * @brief Convert characters from a specified code page to UCS2 encoding.
 *
 * @param dst Output buffer for the converted string.
 * @param count Size of the output buffer.
 * @param str Input string in the specified code page.
 * @param cp Code page to use for conversion.
 * @return Number of converted character codes.
 */
#define oslCccCodetoUCS2    cccCodetoUCS2

/**
 * @brief Set the error character for failed code conversions.
 *
 * The error character is used when a code point cannot be converted to UCS2.
 *
 * @param code New error character (default: 0).
 * @return Previous error character.
 */
#define oslCccSetErrorCharUCS2  cccSetErrorCharUCS2

/**
 * @brief Shutdown the Character Code Conversion Library.
 *
 * This function cleans up any resources used by the Character Code Conversion Library.
 */
#define oslCccShutDown  cccShutDown

/** @} */ // end of Character Code Conversion (ccc)

#ifdef __cplusplus
}
#endif

#endif // __OSL_CCC_H__
