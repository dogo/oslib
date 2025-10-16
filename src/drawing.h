#ifndef _OSL_DRAWING_H_
#define _OSL_DRAWING_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef PSP
#undef RGB
#endif

/// Boolean type
#ifndef __cplusplus
#define bool char
#endif

/** @defgroup drawing Drawing
 *
 * Drawing operations in OSLib.
 * @{
 */

/** \ingroup drawing_color
 * Represents a true color value in OSLib.
 */
typedef unsigned long OSL_COLOR;

/** @defgroup drawing_main Main
 *
 * Core functions and configuration routines for drawing in OSLib.
 * @{
 */

/**
 * Initializes the graphical system for OSLib.
 *
 * This function sets up the rendering environment, including screen resolution, double buffering,
 * and various graphical settings. It can be called multiple times, but note that all images stored
 * in video memory (OSL_IN_VRAM) will be invalidated after calling this function, so ensure you
 * delete all such images beforehand to avoid unexpected behavior.
 *
 * @param pixelFormat The pixel format for the screen. Options include:
 *   - OSL_PF_8888: 32-bit color mode, providing high precision and smooth gradients. However, it
 *     requires twice the memory of the 16-bit mode (1088 kB in double buffer mode, half in single
 *     buffer mode) and is slower due to higher bandwidth usage.
 *   - OSL_PF_5650: 16-bit color mode, limited to 65,000 colors instead of 16 million. It requires
 *     544 kB in double buffer mode and half in single buffer mode. This is the recommended mode
 *     for most applications. You can enable dithering with `oslSetDithering` to simulate more colors.
 *
 * @param bDoubleBuffer Set to TRUE (1) to enable double buffering, which is recommended for games
 * to prevent flickering. Refer to `oslSwapBuffers` for more information on double buffering.
 */
extern void oslInitGfx(int pixelFormat, int bDoubleBuffer);

/**
 * @brief Initializes the drawing process with OSLib. Call this function before performing any drawing operations.
 *
 * This function sets up the necessary GPU state and ensures that drawing operations are properly initialized.
 * It also resets the current texture and palette to null and starts a new command list for rendering.
 *
 * @note If drawing has already started, this function will do nothing.
 *
 * @note The commented section related to intraFont initialization is left out because it could interfere
 *       with user applications. If you need specific matrix configurations, consider setting them in your application.
 */
extern void oslStartDrawing();

/**
 * Ends drawing. Call this function once you've finished rendering.
 * This command waits for the GPU to finish rendering, which can be slow.
 * OSLib may use this remaining time to process and output sound.
 */
extern void oslEndDrawing();

/**
 * Waits for the GPU (Graphics Processing Unit) to complete its current rendering tasks. Since rendering occurs in parallel with CPU operations,
 * there is a risk of modifying an image that the GPU is still processing, leading to incorrect render output.
 *
 * Calling `oslSyncDrawing` ensures that the GPU has finished its operations, allowing you to safely modify graphical data afterward.
 * However, be aware that synchronizing with the GPU can be very slow, as it waits for the GPU to complete all pending tasks.
 * Use this function only when absolutely necessary and when you fully understand its performance implications.
 *
 * **Note:** The number of drawing commands you can issue between `oslStartDrawing` and `oslEndDrawing` is limited by the size of the display list,
 * where these commands are stored before being sent to the GPU. If the display list overflows, your program may crash or exhibit erratic behavior.
 *
 * `oslSyncDrawing` resets the display list after waiting for the GPU to complete rendering, similar to what `oslStartDrawing` and `oslEndDrawing` do.
 * It is a faster alternative for syncing without initiating the end and start of a frame. If you encounter crashes due to extensive drawing commands,
 * consider inserting a call to `oslSyncDrawing` to mitigate the issue.
 */
extern void oslSyncDrawing();

/**
 * Swaps the buffers.
 *
 * In double buffering mode, this function should be called once per frame. Double buffering prevents flickering by maintaining two buffers: one that is displayed on the screen and one that is used for drawing. Once drawing is complete, the buffers are swapped, displaying the newly rendered frame and hiding the previous one for the next drawing operation.
 */
extern void oslSwapBuffers();

/**
 * Ends the graphical context, releasing memory used for the display list.
 *
 * After calling this function, all images placed in video memory (OSL_IN_VRAM) become invalid.
 * Ensure that you release ALL such images before calling this function, as they will need to be reloaded after initializing graphics again with oslInitGfx.
 */
extern void oslEndGfx();

/**
 * Sets the display list size.
 *
 * @param newSize The new size of the display list in bytes.
 *
 * @warning This is an advanced command. The default size is 1 MB. Do not use this function unless you
 * fully understand the implications. After calling this function, you must call oslStartDrawing before
 * beginning to draw again.
 */
extern void oslSetSysDisplayListSize(int newSize);

/** @} */         // end of drawing_main

/** @defgroup drawing_general General
 *
 * General drawing tasks in OSLib.
 * @{
 */

/**
 * Clears the entire screen with the specified color.
 *
 * This function fills the screen with the given color.
 *
 * @param backColor
 *        The color to fill the screen with.
 */
extern void oslClearScreen(int backColor);

/**
 * Sets the clipping region.
 *
 * Defines a rectangular area on the screen within which rendering is allowed. Any drawing outside this rectangle will not appear on the screen.
 * Initially, the clipping region is set to cover the entire screen.
 *
 * @param x0
 *        The x-coordinate of the top-left corner of the clipping region.
 * @param y0
 *        The y-coordinate of the top-left corner of the clipping region.
 * @param x1
 *        The x-coordinate of the bottom-right corner of the clipping region.
 * @param y1
 *        The y-coordinate of the bottom-right corner of the clipping region.
 *
 * @note
 *       The clipping region is automatically adjusted to cover the entire draw buffer image when oslSetDrawBuffer is called.
 */
extern void oslSetScreenClipping(int x0, int y0, int x1, int y1);

/**
 * Sets the depth test.
 *
 * Enables or disables depth testing, which determines whether a pixel should be drawn based on its depth value compared to what is currently in the depth buffer.
 *
 * @param enabled
 *        Set to 1 to enable depth testing, or 0 to disable it.
 */
extern void oslSetDepthTest(int enabled);

/**
 * @brief Resets the screen clipping region to the entire screen.
 *
 * This macro resets the clipping region, ensuring that drawing operations affect the whole screen.
 */
	#define oslResetScreenClipping() oslSetScreenClipping(0, 0, osl_curBuf->sizeX, osl_curBuf->sizeY)

/**
 * @brief Configures the current alpha effect parameters for subsequent drawing operations.
 *
 * This function sets the blending mode and other related parameters that affect how images and shapes are rendered with alpha blending.
 * The effect of this function persists for all subsequent draw calls until it is changed again.
 *
 * @param effect Specifies the blending mode to use:
 *               - `OSL_FX_NONE`: Disables blending entirely.
 *               - `OSL_FX_DEFAULT`: Resets to the default blending mode (equivalent to `OSL_FX_RGBA`).
 *               - `OSL_FX_RGBA`: Standard RGBA blending, where the alpha channel is used to blend the source and destination colors.
 *               - `OSL_FX_ALPHA`: Alpha blending using `coeff1` as the alpha value. The resulting color is `dstColor = srcColor * coeff1 + dstColor * (1 - coeff1)`.
 *               - `OSL_FX_ADD`: Additive blending, where the resulting color is `dstColor = dstColor * coeff2 + srcColor * coeff1`.
 *               - `OSL_FX_SUB`: Subtractive blending, where the resulting color is `dstColor = dstColor * coeff2 - srcColor * coeff1`.
 *               - `OSL_FX_COLOR`: Optional flag that treats `coeff1` and `coeff2` as 32-bit colors instead of alpha values. This allows for color tinting effects.
 * @param coeff1 A coefficient used in the blending operation, either as an alpha value (0 to 255) or a 32-bit color (0x00000000 to 0xFFFFFFFF).
 * @param coeff2 An extended coefficient used in the additive or subtractive blending modes, either as an alpha value or a 32-bit color.
 *
 * **Examples:**
 * \code
 * // Default alpha blending
 * oslSetAlpha(OSL_FX_DEFAULT, 0);
 *
 * // Tint an image with red
 * oslSetAlpha(OSL_FX_COLOR, RGBA(255, 0, 0, 255));
 *
 * // Render the image semi-transparent
 * oslSetAlpha(OSL_FX_ALPHA, 128);
 *
 * // Apply both tinting and semi-transparency
 * oslSetAlpha(OSL_FX_ALPHA | OSL_FX_COLOR, RGBA(255, 0, 0, 128));
 *
 * // Additive blending, similar to effects seen on older consoles
 * oslSetAlpha2(OSL_FX_ADD, 255, 128);
 * \endcode
 *
 * Understanding how color multiplication works can help you achieve various effects. For example, multiplying colors generally darkens them, with black always resulting in black,
 * while multiplying by white leaves the color unchanged.
 */
extern void oslSetAlpha2(u32 effect, u32 coeff1, u32 coeff2);

/**
 * @brief A simplified version of oslSetAlpha2, where coeff2 is set to the maximum value.
 *
 * @param effect Blending mode to use (as described in oslSetAlpha2).
 * @param coeff1 A coefficient used in the blending operation, either as an alpha value or a 32-bit color.
 */
static inline void oslSetAlpha(u32 effect, u32 coeff1)
{
	oslSetAlpha2(effect, coeff1, 0xFFFFFFFF);
}

/**
 * @brief Structure containing data for a special alpha effect.
 *
 * This structure holds parameters that define a specific alpha blending effect,
 * including the type of effect and two coefficients that may be used for blending
 * or other related calculations.
 */
typedef struct
{
	int effect;          ///< Current alpha special effect identifier.
	int coeff1;          ///< First coefficient for the alpha effect.
	int coeff2;          ///< Second coefficient for the alpha effect.
} OSL_ALPHA_PARAMS;

/**
 * @brief Current global alpha effect.
 *
 * This variable holds the identifier of the alpha effect currently in use.
 */
extern int osl_currentAlphaEffect;

/**
 * @brief Current first coefficient for the global alpha effect.
 *
 * This variable holds the first coefficient used by the current global alpha effect.
 */
extern OSL_COLOR osl_currentAlphaCoeff;

/**
 * @brief Current second coefficient for the global alpha effect.
 *
 * This variable holds the second coefficient used by the current global alpha effect.
 */
extern OSL_COLOR osl_currentAlphaCoeff2;

/**
 * @brief Stores the current alpha parameters into an OSL_ALPHA_PARAMS structure.
 *
 * This function copies the current global alpha effect parameters, including the effect type
 * and coefficients, into the provided `OSL_ALPHA_PARAMS` structure.
 *
 * @param alpha Pointer to an `OSL_ALPHA_PARAMS` structure where the current alpha parameters will be stored.
 */
static inline void oslGetAlphaEx(OSL_ALPHA_PARAMS *alpha)
{
	alpha->effect = osl_currentAlphaEffect;
	alpha->coeff1 = osl_currentAlphaCoeff;
	alpha->coeff2 = osl_currentAlphaCoeff2;
}

/**
 * @brief Sets the current alpha parameters using an OSL_ALPHA_PARAMS structure.
 *
 * This function updates the global alpha effect parameters based on the values
 * stored in the provided `OSL_ALPHA_PARAMS` structure. It uses the `oslSetAlpha2` function
 * to apply these parameters.
 *
 * @param alpha Pointer to an `OSL_ALPHA_PARAMS` structure containing the alpha parameters to be set.
 */
static inline void oslSetAlphaEx(OSL_ALPHA_PARAMS *alpha)
{
	oslSetAlpha2(alpha->effect, alpha->coeff1, alpha->coeff2);
}

/**
 * Enables or disables bilinear filtering.
 *
 * Bilinear filtering smoothes images, reducing edge aliasing when they are stretched or rotated. However, this can cause blurring in text or images that are not stretched, so it is generally not recommended to enable bilinear filtering permanently.
 *
 * @param enabled
 *        Set to 1 to enable bilinear filtering, or 0 to disable it.
 */
extern void oslSetBilinearFilter(int enabled);

/**
 * Enables or disables dithering.
 *
 * Dithering is useful in 16-bit color modes or when drawing to a 16-bit image. If enabled, it blends colors that cannot be rendered exactly by using the nearest available colors and applying a pattern of small dots. This gives the illusion of intermediate colors, such as creating the appearance of orange by dithering red and yellow.
 *
 * @param enabled
 *        Set to 1 to enable dithering, or 0 to disable it.
 */
extern void oslSetDithering(int enabled);

/**
 * @brief Enables color keying, making a specific color transparent.
 *
 * This function enables color keying, which makes any pixel matching the specified color (`color`) fully transparent.
 * This is particularly useful when loading images with a specific background color that should be treated as transparent,
 * such as bright pink backgrounds in sprite sheets.
 *
 * The transparency effect persists even after disabling color keying, as it modifies the alpha channel of the image.
 *
 * **Example 1: Loading an image with a color key**
 * \code
 * // Set bright pink as the transparent color
 * oslSetTransparentColor(RGB(255, 0, 255));
 * // Load an image with transparency, ensuring to use a pixel format that supports alpha (e.g., OSL_PF_5551)
 * OSL_IMAGE *img = oslLoadImageFilePNG("test.png", OSL_IN_RAM, OSL_PF_5551);
 * // Disable color keying to avoid affecting further drawing operations
 * oslDisableTransparentColor();
 * \endcode
 *
 * **Example 2: Drawing with color keying enabled**
 * \code
 * oslSetTransparentColor(RGB(255, 0, 255));
 * // Drawing a pink rectangle won't display anything, as pink is currently masked out
 * oslDrawFillRect(0, 0, 100, 100, RGB(255, 0, 255));
 * \endcode
 *
 * @param color The color to be treated as transparent.
 */
extern void oslSetTransparentColor(OSL_COLOR color);

/**
 * @brief Disables color keying, preventing any color-based transparency.
 *
 * When color keying is disabled, all pixels will be drawn to the screen, including those that match the previously set transparent color.
 * This function should be called after loading images with transparency or after drawing operations where color keying was used.
 */
extern void oslDisableTransparentColor();

/**
 * @brief Indicates whether bilinear filtering is currently enabled.
 *
 * This variable holds the state of bilinear filtering, which affects the smoothing
 * of textures when they are scaled or rotated. A non-zero value means bilinear
 * filtering is enabled, while a value of zero means it is disabled.
 */
extern int osl_bilinearFilterEnabled;

/**
 * @brief Indicates whether dithering is currently enabled.
 *
 * This variable holds the state of dithering, which can be used to reduce
 * the appearance of color banding by blending colors. A non-zero value means
 * dithering is enabled, while a value of zero means it is disabled.
 */
extern int osl_ditheringEnabled;

/**
 * @brief Indicates whether color keying is currently enabled.
 *
 * This variable holds the state of color keying, a technique used to make
 * certain colors transparent. A non-zero value means color keying is enabled,
 * while a value of zero means it is disabled.
 */
extern int osl_colorKeyEnabled;

enum
{
	OSL_FX_NONE = 0,              ///< No special effect.
	OSL_FX_FLAT,                  ///< Flat shading effect.
	OSL_FX_ALPHA,                 ///< Alpha blending effect.
	OSL_FX_ADD,                   ///< Additive blending effect.
	OSL_FX_SUB                    ///< Subtractive blending effect.
};

/**
 * @brief Default effect that combines RGBA channels.
 *
 * This macro is used as the default alpha effect, combining red, green, blue, and alpha channels.
 */
	#define OSL_FX_DEFAULT OSL_FX_RGBA

/**
 * @brief Opaque effect with no special processing.
 *
 * This macro sets the alpha effect to none, resulting in full opacity with no special processing.
 */
	#define OSL_FX_OPAQUE OSL_FX_NONE

/**
 * @brief Tint effect that combines alpha blending and color.
 *
 * This macro applies an alpha blending effect combined with a color effect, useful for tinting.
 */
	#define OSL_FX_TINT (OSL_FX_ALPHA | OSL_FX_COLOR)

// Cumulable avec les autres pour définir l'utilisation du canal alpha.
// Can be combined with other effects to define the use of the alpha channel.
	#define OSL_FX_RGBA 0x100

// Coefficients alpha à trois canaux
// Alpha coefficients for three channels (Red, Green, Blue).
	#define OSL_FX_COLOR 0x1000

// Internal use only - Do not access these variables directly!
// extern int osl_currentResolutionBPP;

// Couleur transparente utilisée au chargement des images.
// Transparent color used when loading images.
extern int osl_colorKeyValue;

/** @} */         // end of drawing_general

/** @defgroup drawing_color Colors
 *  @brief Color manipulation utilities.
 *
 *  This group contains macros for creating and manipulating colors in various bit formats.
 *  @{
 */

/**
 * @brief Creates a 32-bit opaque color.
 *
 * This macro generates a 32-bit color value with full opacity (alpha = 255).
 *
 * @param r Red component (0-255)
 * @param v Green component (0-255)
 * @param b Blue component (0-255)
 * @return 32-bit color value with full opacity.
 */
	#define RGB(r, v, b) ((r) | ((v) << 8) | ((b) << 16) | (0xff << 24))

/**
 * @brief Creates a 32-bit color with alpha (transparency).
 *
 * This macro generates a 32-bit color value, allowing for semi-transparency.
 *
 * @param r Red component (0-255)
 * @param v Green component (0-255)
 * @param b Blue component (0-255)
 * @param a Alpha component (0-255)
 * @return 32-bit color value with specified transparency.
 */
	#define RGBA(r, v, b, a) ((r) | ((v) << 8) | ((b) << 16) | ((a) << 24))

/**
 * @brief Creates a 12-bit opaque color.
 *
 * This macro generates a 12-bit color value with full opacity. The red, green, and blue components
 * are scaled down to fit the 12-bit format (4 bits per channel).
 *
 * @param r Red component (0-255)
 * @param v Green component (0-255)
 * @param b Blue component (0-255)
 * @return 12-bit color value with full opacity.
 */
	#define RGB12(r, v, b) ((((b) >> 4) << 8) | (((v) >> 4) << 4) | ((r) >> 4) | (0xf << 12))

/**
 * @brief Creates a 12-bit color with alpha.
 *
 * This macro generates a 12-bit color value, including an alpha component for transparency.
 *
 * @param r Red component (0-255)
 * @param v Green component (0-255)
 * @param b Blue component (0-255)
 * @param a Alpha component (0-255)
 * @return 12-bit color value with specified transparency.
 */
	#define RGBA12(r, v, b, a) ((((a) >> 4) << 12) | (((b) >> 4) << 8) | (((v) >> 4) << 4) | ((r) >> 4))

/**
 * @brief Creates a 15-bit opaque color.
 *
 * This macro generates a 15-bit color value with full opacity. The red, green, and blue components
 * are scaled down to fit the 15-bit format (5 bits for red and blue, 5 or 6 bits for green).
 *
 * @param r Red component (0-255)
 * @param v Green component (0-255)
 * @param b Blue component (0-255)
 * @return 15-bit color value with full opacity.
 */
	#define RGB15(r, v, b) ((((b) >> 3) << 10) | (((v) >> 3) << 5) | ((r) >> 3) | (1 << 15))

/**
 * @brief Creates a 15-bit color with alpha.
 *
 * This macro generates a 15-bit color value, including an alpha component for transparency.
 *
 * @param r Red component (0-255)
 * @param v Green component (0-255)
 * @param b Blue component (0-255)
 * @param a Alpha component (0-255)
 * @return 15-bit color value with specified transparency.
 */
	#define RGBA15(r, v, b, a) ((((a) >> 7) << 15) | (((b) >> 3) << 10) | (((v) >> 3) << 5) | ((r) >> 3))

/**
 * @brief Creates a 16-bit color.
 *
 * This macro generates a 16-bit color value with no alpha channel. The red, green, and blue components
 * are scaled down to fit the 16-bit format (5 bits for red, 6 bits for green, 5 bits for blue).
 *
 * @param r Red component (0-255)
 * @param v Green component (0-255)
 * @param b Blue component (0-255)
 * @return 16-bit color value.
 */
	#define RGB16(r, v, b) ((((b) >> 3) << 11) | (((v) >> 2) << 5) | ((r) >> 3))

/** @} */         // end of drawing_color

/**
 * @brief Extracts the R, G, B, A (Red, Green, Blue, Alpha) component values from a 32-bit color.
 *
 * This macro takes a 32-bit color value and extracts the individual 8-bit R, G, B, and A components.
 * The extracted values are stored in the variables provided as the second to fifth arguments.
 *
 * @param data The 32-bit color value from which to extract the components.
 * @param r The variable to store the red component (0-255).
 * @param g The variable to store the green component (0-255).
 * @param b The variable to store the blue component (0-255).
 * @param a The variable to store the alpha component (0-255).
 *
 * @code
 * u32 color = RGBA(1, 2, 3, 4);
 * u8 red, green, blue, alpha;
 *
 * oslRgbaGet8888(color, red, green, blue, alpha);
 *
 * oslPrintf("%i %i %i %i", red, green, blue, alpha);
 * @endcode
 *
 * This example will print: `1 2 3 4`.
 */
	#define oslRgbaGet8888(data, r, g, b, a) ((r) = ((data) & 0xff), (g) = (((data) >> 8) & 0xff), (b) = (((data) >> 16) & 0xff), (a) = (((data) >> 24) & 0xff))

/**
 * @brief Extracts R, G, B, A values from a 4444 color.
 *
 * This macro takes a 16-bit color value in 4444 format and extracts the individual 8-bit R, G, B, and A components. The 4-bit components are expanded to 8-bit by multiplying with 16.
 *
 * @param data The 16-bit color value in 4444 format.
 * @param r The variable to store the red component (0-255).
 * @param g The variable to store the green component (0-255).
 * @param b The variable to store the blue component (0-255).
 * @param a The variable to store the alpha component (0-255).
 *
 * @code
 * u32 color = RGBA12(255, 128, 0, 1);
 * u8 red, green, blue, alpha;
 *
 * oslRgbGet4444(color, red, green, blue, alpha);
 *
 * oslPrintf("%i %i %i %i", red, green, blue, alpha);
 * @endcode
 *
 * This example will print: `240 128 0 0`. The alpha value was 1 but was lost due to precision limits in 12-bit mode.
 * The red color value was 255 but lost precision due to 12-bit conversion, resulting in 240 instead of 255.
 * For more precise results, consider using the 'f' alternate routines like `oslRgbaGet4444f`.
 */
	#define oslRgbaGet4444(data, r, g, b, a) ((r) = ((data) & 0xf) << 4, (g) = (((data) >> 4) & 0xf) << 4, (b) = (((data) >> 8) & 0xf) << 4, (a) = (((data) >> 12) & 0xf) << 4)

/**
 * @brief Extracts R, G, B, A values from a 5551 color.
 *
 * This macro takes a 16-bit color value in 5551 format and extracts the individual 8-bit R, G, B, and A components. The 5-bit components are expanded to 8-bit by multiplying with 8, and the 1-bit alpha is expanded to 8-bit by multiplying with 128.
 *
 * @param data The 16-bit color value in 5551 format.
 * @param r The variable to store the red component (0-255).
 * @param g The variable to store the green component (0-255).
 * @param b The variable to store the blue component (0-255).
 * @param a The variable to store the alpha component (0-255).
 *
 * @code
 * u32 color = RGBA15(31, 31, 31, 1);
 * u8 red, green, blue, alpha;
 *
 * oslRgbaGet5551(color, red, green, blue, alpha);
 *
 * oslPrintf("%i %i %i %i", red, green, blue, alpha);
 * @endcode
 *
 * This example will print the expanded values of the 5551 format color.
 */
	#define oslRgbaGet5551(data, r, g, b, a) ((r) = ((data) & 0x1f) << 3, (g) = (((data) >> 5) & 0x1f) << 3, (b) = (((data) >> 10) & 0x1f) << 3, (a) = (((data) >> 15) & 0x1) << 7)

/**
 * @brief Extracts R, G, B values from a 5650 color.
 *
 * This macro takes a 16-bit color value in 5650 format and extracts the individual 8-bit R, G, and B components. The 5-bit R and B components are expanded to 8-bit by multiplying with 8, and the 6-bit G component is expanded to 8-bit by multiplying with 4.
 *
 * @param data The 16-bit color value in 5650 format.
 * @param r The variable to store the red component (0-255).
 * @param g The variable to store the green component (0-255).
 * @param b The variable to store the blue component (0-255).
 *
 * @code
 * u32 color = RGB16(31, 63, 31);
 * u8 red, green, blue;
 *
 * oslRgbGet5650(color, red, green, blue);
 *
 * oslPrintf("%i %i %i", red, green, blue);
 * @endcode
 *
 * This example will print the expanded values of the 5650 format color.
 */
	#define oslRgbGet5650(data, r, g, b) ((r) = ((data) & 0x1f) << 3, (g) = (((data) >> 5) & 0x3f) << 2, (b) = (((data) >> 11) & 0x1f) << 3)

/**
 * @brief Extracts R, G, B, A values from a 4444 color with finer precision.
 *
 * This macro extracts the R, G, B, and A components from a 16-bit color value in 4444 format. It expands the 4-bit components to 8-bit values by using both bit-shifting and masking techniques to provide better color accuracy.
 *
 * @param data The 16-bit color value in 4444 format.
 * @param r The variable to store the red component (0-255).
 * @param g The variable to store the green component (0-255).
 * @param b The variable to store the blue component (0-255).
 * @param a The variable to store the alpha component (0-255).
 *
 * @code
 * u32 color = RGBA12(255, 128, 0, 1);
 * u8 red, green, blue, alpha;
 *
 * oslRgbaGet4444f(color, red, green, blue, alpha);
 *
 * oslPrintf("%i %i %i %i", red, green, blue, alpha);
 * @endcode
 *
 * This example will provide more accurate color values compared to `oslRgbaGet4444`.
 */
	#define oslRgbaGet4444f(data, r, g, b, a) ((r) = ((data) & 0xf) << 4 | ((data) & 0xf), (g) = (((data) >> 4) & 0xf) << 4 | (((data) >> 4) & 0xf), (b) = (((data) >> 8) & 0xf) << 4 | (((data) >> 8) & 0xf), (a) = (((data) >> 12) & 0xf) << 4 | (((data) >> 12) & 0xf))

/**
 * @brief Extracts R, G, B, A values from a 5551 color with finer precision.
 *
 * This macro extracts the R, G, B, and A components from a 16-bit color value in 5551 format. It provides more accurate color values by expanding the 5-bit components and the 1-bit alpha using bit-shifting and masking.
 *
 * @param data The 16-bit color value in 5551 format.
 * @param r The variable to store the red component (0-255).
 * @param g The variable to store the green component (0-255).
 * @param b The variable to store the blue component (0-255).
 * @param a The variable to store the alpha component (0-255).
 *
 * @code
 * u32 color = RGBA15(31, 31, 31, 1);
 * u8 red, green, blue, alpha;
 *
 * oslRgbaGet5551f(color, red, green, blue, alpha);
 *
 * oslPrintf("%i %i %i %i", red, green, blue, alpha);
 * @endcode
 *
 * This example will provide more accurate color values compared to `oslRgbaGet5551`.
 */
	#define oslRgbaGet5551f(data, r, g, b, a) ((r) = ((data) & 0x1f) << 3 | ((data) & 0x1f) >> 2, (g) = (((data) >> 5) & 0x1f) << 3 | (((data) >> 5) & 0x1f) >> 2, (b) = (((data) >> 10) & 0x1f) << 3 | (((data) >> 10) & 0x1f) >> 2, (a) = (((data) >> 15) & 0x1) * 255)

/**
 * @brief Extracts R, G, B values from a 5650 color with finer precision.
 *
 * This macro extracts the R, G, and B components from a 16-bit color value in 5650 format. It expands the 5-bit R and B components and the 6-bit G component to 8-bit values for more accurate color representation.
 *
 * @param data The 16-bit color value in 5650 format.
 * @param r The variable to store the red component (0-255).
 * @param g The variable to store the green component (0-255).
 * @param b The variable to store the blue component (0-255).
 *
 * @code
 * u32 color = RGB16(31, 63, 31);
 * u8 red, green, blue;
 *
 * oslRgbGet5650f(color, red, green, blue);
 *
 * oslPrintf("%i %i %i", red, green, blue);
 * @endcode
 *
 * This example will provide more accurate color values compared to `oslRgbGet5650`.
 */
	#define oslRgbGet5650f(data, r, g, b) ((r) = ((data) & 0x1f) << 3 | ((data) & 0x1f) >> 2, (g) = (((data) >> 5) & 0x3f) << 2 | (((data) >> 5) & 0x3f) >> 4, (b) = (((data) >> 11) & 0x1f) << 3 | (((data) >> 10) & 0x1f) >> 2)

/** @} */         // end of drawing_color

/** @defgroup drawing_shapes Shapes
 *
 *  Formerly untextured geometry.
 *  @{
 */

/** @brief Draws a line from (x0, y0) to (x1, y1).
 *
 * This function draws a line on the screen from the starting point (x0, y0) to the ending point (x1, y1) using the specified color.
 *
 * @param x0 The x-coordinate of the start point of the line.
 * @param y0 The y-coordinate of the start point of the line.
 * @param x1 The x-coordinate of the end point of the line.
 * @param y1 The y-coordinate of the end point of the line.
 * @param color The color of the line.
 */
extern void oslDrawLine(int x0, int y0, int x1, int y1, OSL_COLOR color);

/** @brief Draws an empty rectangle from (x0, y0) to (x1, y1).
 *
 * This function draws the outline of a rectangle on the screen with the top-left corner at (x0, y0) and the bottom-right corner at (x1, y1). The rectangle is drawn with the specified color.
 *
 * @param x0 The x-coordinate of the top-left corner of the rectangle.
 * @param y0 The y-coordinate of the top-left corner of the rectangle.
 * @param x1 The x-coordinate of the bottom-right corner of the rectangle.
 * @param y1 The y-coordinate of the bottom-right corner of the rectangle.
 * @param color The color of the rectangle outline.
 */
extern void oslDrawRect(int x0, int y0, int x1, int y1, OSL_COLOR color);

/** @brief Draws a filled rectangle from (x0, y0) to (x1, y1).
 *
 * This function draws a filled rectangle on the screen with the top-left corner at (x0, y0) and the bottom-right corner at (x1, y1). The rectangle is filled with the specified color.
 *
 * @param x0 The x-coordinate of the top-left corner of the rectangle.
 * @param y0 The y-coordinate of the top-left corner of the rectangle.
 * @param x1 The x-coordinate of the bottom-right corner of the rectangle.
 * @param y1 The y-coordinate of the bottom-right corner of the rectangle.
 * @param color The color used to fill the rectangle.
 */
extern void oslDrawFillRect(int x0, int y0, int x1, int y1, OSL_COLOR color);

/** @brief Draws a gradient rectangle from (x0, y0) to (x1, y1).
 *
 * This function draws a rectangle on the screen with a gradient color effect. The gradient transitions from one color to another across the rectangle. The colors are applied as follows:
 * - `c1` is used for the top-left corner.
 * - `c2` is used for the top-right corner.
 * - `c3` is used for the bottom-left corner.
 * - `c4` is used for the bottom-right corner.
 *
 * @param x0 The x-coordinate of the top-left corner of the rectangle.
 * @param y0 The y-coordinate of the top-left corner of the rectangle.
 * @param x1 The x-coordinate of the bottom-right corner of the rectangle.
 * @param y1 The y-coordinate of the bottom-right corner of the rectangle.
 * @param c1 The color of the top-left corner.
 * @param c2 The color of the top-right corner.
 * @param c3 The color of the bottom-left corner.
 * @param c4 The color of the bottom-right corner.
 */
extern void oslDrawGradientRect(int x0, int y0, int x1, int y1, OSL_COLOR c1, OSL_COLOR c2, OSL_COLOR c3, OSL_COLOR c4);

/** @} */         // end of drawing_shapes

/** @defgroup drawing_adv Advanced
 *
 * Advanced drawing tasks.
 * @{
 */

/** @brief Swizzles texture data.

        This function performs a swizzling operation on texture data. Swizzling is a process that reorders texture data to improve memory access patterns, often used for better performance on specific hardware. The function takes an input texture and writes the swizzled output to the provided buffer.

        @param out Pointer to the buffer where the swizzled texture data will be stored.
        @param in Pointer to the input texture data.
        @param width The width of the texture in pixels.
        @param height The height of the texture in pixels.
 */
extern void oslSwizzleTexture(u8 *out, const u8 *in, unsigned int width, unsigned int height);

/** @brief Unswizzles texture data.

        This function performs the inverse of the swizzling operation. It converts texture data that has been swizzled back into its original order. The function takes a swizzled texture and writes the unswizzled output to the provided buffer.

        @param out Pointer to the buffer where the unswizzled texture data will be stored.
        @param in Pointer to the swizzled texture data.
        @param width The width of the texture in pixels.
        @param height The height of the texture in pixels.
 */
extern void oslUnswizzleTexture(u8 *out, const u8 *in, unsigned int width, unsigned int height);

/**
 * @brief Draws a tile from the currently selected texture at the specified position.
 *
 * This function draws a rectangular section (tile) from the currently bound texture to a specified position on the screen.
 * The tile is defined by its top-left corner (`u`, `v`) and its size (`tX`, `tY`). The destination position on the screen is specified
 * by (`x`, `y`).
 *
 * @note This function is deprecated and should not be used in new code. It may be removed in future versions.
 *
 * @param u The x-coordinate of the top-left corner of the tile in the texture.
 * @param v The y-coordinate of the top-left corner of the tile in the texture.
 * @param x The x-coordinate of the position on the screen where the tile will be drawn.
 * @param y The y-coordinate of the position on the screen where the tile will be drawn.
 * @param tX The width of the tile.
 * @param tY The height of the tile.
 */
extern void oslDrawTile(int u, int v, int x, int y, int tX, int tY);

/** @defgroup image Images
 *
 * Image support in OSLib.
 * @{
 */

/** @brief Type definition for 16-bit palette data.

        This type defines a 16-bit palette data structure with 16-byte alignment. It is used for handling image palettes in memory with proper alignment for performance optimization.

        This type is typically used when working with image data that requires a palette of 16-bit entries.

        @note The alignment ensures that the data structure is properly aligned in memory, which can be important for certain hardware optimizations.

 */
	#define OSL_PALETTEDATA16 unsigned short __attribute__((aligned(16)))

/** @brief Type definition for 32-bit palette data.

        This type defines a 32-bit palette data structure with 16-byte alignment. It is used for handling image palettes in memory with proper alignment for performance optimization.

        This type is typically used when working with image data that requires a palette of 32-bit entries.

        @note The alignment ensures that the data structure is properly aligned in memory, which can be important for certain hardware optimizations.

 */
	#define OSL_PALETTEDATA32 unsigned long __attribute__((aligned(16)))

/** @brief Structure representing a palette.

        This structure defines a palette used for image processing in OSLib. It holds information about the pixel format, the number of palette entries, the location of the palette, and a pointer to the raw palette data.

        @struct OSL_PALETTE
        @param pixelFormat
                Pixel format of the palette entries. This defines how colors are stored in the palette.
        @param nElements
                Number of entries in the palette. This indicates how many colors are available in the palette.
        @param location
                Location of the palette. This should always be set to `OSL_IN_RAM` to indicate that the palette is stored in RAM.
        @param data
                Pointer to the actual raw data representing the color entries in their pixel format.

 */
typedef struct
{
	short pixelFormat;         ///< Pixel format of palette entries
	short nElements;           ///< Number of entries
	short location;            ///< Location of the palette (always use OSL_IN_RAM)
	void *data;                ///< Actual (raw) data, representing color entries in their pixel format
} OSL_PALETTE;

/** @brief Constants for the flags member of OSL_IMAGE.

        These constants define the flags that can be used to specify various attributes of an image in OSLib.

        @enum OSL_IMAGE_FLAGS
        @param OSL_IMAGE_SWIZZLED
                Indicates that the image is swizzled. Swizzling refers to a technique used to optimize texture access patterns.
        @param OSL_IMAGE_COPY
                Indicates that the image is a copy of another image. This can be used to identify and manage image copies.
        @param OSL_IMAGE_AUTOSTRIP
                Indicates that the image can be automatically stripped. This flag is used to manage automatic stripping operations on images.
 */
enum OSL_IMAGE_FLAGS
{
	OSL_IMAGE_SWIZZLED = 1,         ///< Image is swizzled
	OSL_IMAGE_COPY = 2,                     ///< Image is a copy
	OSL_IMAGE_AUTOSTRIP = 4         ///< Image can be automatically stripped (let it one)
};

/** @brief Structure representing an image loaded in memory.

        This structure defines an image in OSLib, including various properties like dimensions, raw data, pixel format, and additional attributes used for drawing and transforming the image.

        @struct OSL_IMAGE
        @param sizeX
                Displayable width of the image.
        @param sizeY
                Displayable height of the image.
        @param sysSizeX
                Width aligned to the next power of two, used for system-level optimizations.
        @param sysSizeY
                Height aligned to the next power of two, used for system-level optimizations.
        @param realSizeX
                Actual buffer width of the image. Never write outside of these dimensions!
        @param realSizeY
                Actual buffer height of the image. Never write outside of these dimensions!
        @param data
                Pointer to the raw image data in memory.
        @param flags
                Special flags for the image, defined by `OSL_IMAGE_FLAGS`.
        @param totalSize
                Total size of the image in bytes.
        @param location
                Location of the image, either in RAM (`OSL_IN_RAM`) or VRAM (`OSL_IN_VRAM`).
        @param pixelFormat
                Pixel format of the image data, determining how the colors are stored.
        @param palette
                Pointer to the palette used in 4-bit and 8-bit color modes.
        @param frameSizeX
                Width of a frame in the image, useful for animations.
        @param frameSizeY
                Height of a frame in the image, useful for animations.
        @param x
                X-coordinate of the image position on the screen.
        @param y
                Y-coordinate of the image position on the screen.
        @param stretchX
                Final width of the image when drawn, allowing for stretching.
        @param stretchY
                Final height of the image when drawn, allowing for stretching.
        @param offsetX0
                Horizontal offset in the texture for the first point.
        @param offsetY0
                Vertical offset in the texture for the first point.
        @param offsetX1
                Horizontal offset in the texture for the second point.
        @param offsetY1
                Vertical offset in the texture for the second point.
        @param centerX
                X-coordinate of the rotation center.
        @param centerY
                Y-coordinate of the rotation center.
        @param angle
                Rotation angle in degrees, used for rotating the image.
 */
typedef struct
{
	// Protected
	u16 sizeX, sizeY;                  ///< Displayable size
	u16 sysSizeX, sysSizeY;            ///< Size aligned to the next power of two
	u16 realSizeX, realSizeY;          ///< Actual buffer size (never write outside of these dimensions!)
	void *data;                        ///< Raw image data in memory
	u8 flags;                          ///< Special image flags
	int totalSize;                     ///< Total image size in bytes
	short location;                    ///< Image location (OSL_IN_RAM or OSL_IN_VRAM)
	short pixelFormat;                 ///< Image pixel format

	// Public
	OSL_PALETTE *palette;              ///< Palette for 4 and 8-bit modes
	u16 frameSizeX, frameSizeY;         ///< Size of a frame in the image

	// Special
	int x, y;                          ///< Image positions
	int stretchX, stretchY;            ///< Final image size when drawn (stretched)
	// Morceau
	float offsetX0, offsetY0, offsetX1, offsetY1;         ///< Offset in the texture
	// Rotation
	int centerX, centerY;              ///< Rotation center
	int angle;                         ///< Angle (rotation) in degrees

} OSL_IMAGE;

/** @brief Flags indicating the memory location of an image and optional swizzling.

        This enumeration defines various flags used to specify where an image should be placed in memory (RAM or VRAM) and whether it should be swizzled or unswizzled. These flags are typically used in functions that load or manage images within OSLib.

        @enum OSL_LOCATION_FLAGS
        @param OSL_IN_NONE
                Indicates that the image does not exist or the location is not specified.
        @param OSL_IN_VRAM
                Places the image in VRAM (Video RAM), typically used for faster rendering.
        @param OSL_IN_RAM
                Places the image in RAM (main system memory), offering more flexibility but potentially slower access.
        @param OSL_LOCATION_MASK
                A mask value that covers all potential locations, ensuring compatibility with future location types.
        @param OSL_SWIZZLED
                Directly swizzles the image upon loading, optimizing it for certain types of texture mapping. Only works with specific `oslLoadImage[...]` functions.
        @param OSL_UNSWIZZLED
                Forces the image to be loaded without swizzling, overriding default behavior in the `oslLoadImage[...]` functions.

 */
typedef enum OSL_LOCATION_FLAGS
{
	OSL_IN_NONE = 0,                ///< Doesn't exist
	OSL_IN_VRAM = 1,                ///< In VRAM
	OSL_IN_RAM = 2,                 ///< In RAM
	OSL_LOCATION_MASK = 7,          ///< There will probably never be more than 8 locations...
	OSL_SWIZZLED = 8,               ///< Directly swizzle image (only works for oslLoadImage[...] functions!)
	OSL_UNSWIZZLED = 16             ///< Force no swizzling (oslLoadImage[...])
} OSL_LOCATION_FLAGS;

/** @brief Pixel formats used for defining color modes in OSLib.

        This enumeration defines various pixel formats that are used to describe the color depth and layout of image data within OSLib. These formats are aligned with the PlayStation Portable (PSP) graphics engine's formats and are used for texture and image processing.

        @enum OSL_PIXELFORMATS
        @param OSL_PF_5650
                16-bit format with 5 bits for the red, 6 bits for the green, and 5 bits for the blue components, with no alpha (transparency) channel.
                This format corresponds to `GU_PSM_5650` in the PSP's graphics engine.
        @param OSL_PF_5551
                16-bit format with 5 bits for the red, 5 bits for the green, 5 bits for the blue components, and 1 bit for alpha (transparency).
                This format corresponds to `GU_PSM_5551` in the PSP's graphics engine.
        @param OSL_PF_4444
                16-bit format with 4 bits each for the red, green, blue, and alpha components.
                This format corresponds to `GU_PSM_4444` in the PSP's graphics engine.
        @param OSL_PF_8888
                32-bit format with 8 bits for each of the red, green, blue, and alpha components, allowing for a full range of colors and transparency.
                This format corresponds to `GU_PSM_8888` in the PSP's graphics engine.
        @param OSL_PF_4BIT
                Paletted format with 4 bits per pixel, allowing for 16 different colors. This format is typically used with a palette and cannot be used as a drawbuffer.
                It corresponds to `GU_PSM_T4` in the PSP's graphics engine.
        @param OSL_PF_8BIT
                Paletted format with 8 bits per pixel, allowing for 256 different colors. Like the 4-bit format, it is used with a palette and cannot be set as a drawbuffer.
                It corresponds to `GU_PSM_T8` in the PSP's graphics engine.

 */
enum OSL_PIXELFORMATS
{
	OSL_PF_5650 = GU_PSM_5650,         ///< 16 bits, 5 bits per component, except green which has 6, no alpha
	OSL_PF_5551 = GU_PSM_5551,         ///< 15 bits, 5 bits per component, 1 alpha bit
	OSL_PF_4444 = GU_PSM_4444,         ///< 12 bits, 4 bits per component, 4 alpha bits
	OSL_PF_8888 = GU_PSM_8888,         ///< 32 bits, 8 bits per component, 8 alpha bits
	OSL_PF_4BIT = GU_PSM_T4,           ///< Paletted format, 4 bits (16 colors), cannot be set as drawbuffer
	OSL_PF_8BIT = GU_PSM_T8            ///< Paletted format, 8 bits (256 colors), cannot be set as drawbuffer
};

/**
 * @brief Creates an empty image in OSLib.
 *
 * This function creates an empty image with specified dimensions, location, and pixel format. The image is initially filled with random values, so it's important to clear it using `oslClearImage()` before use.
 *
 * @param larg
 *        Width of the image. The actual width will be aligned to the next power of two to ensure compatibility with the PSP's graphics system, and each image line will be aligned to at least 16 bytes.
 *
 * @param haut
 *        Height of the image. The actual height will be aligned to the next multiple of 8 to allow for swizzling operations.
 *
 * @param location
 *        Specifies the memory location for the image. It should be either `OSL_IN_VRAM` (Video RAM) or `OSL_IN_RAM` (system RAM).
 *
 * @param pixelFormat
 *        Defines the pixel format of the image, determining its color type, precision, and memory usage. While higher formats like `OSL_PF_8888` provide better image quality, they also consume more memory and may reduce rendering performance. It is advised to use lower formats where possible.
 *
 * @return
 *        Returns a pointer to the created `OSL_IMAGE`. If the image creation fails, the function returns `NULL`. It's essential to check for a `NULL` return value to avoid crashes, as trying to use a `NULL` image will cause the PSP to shut down after a brief period.
 *
 * @note
 * - The maximum allowable size for an image is 512x512 pixels. Both the width and height must not exceed 512 pixels.
 * - Empty images created using this function are not automatically swizzled, even if the `oslSetImageAutoSwizzle` parameter is enabled.
 * - Do NOT pass `OSL_SWIZZLED` or `OSL_UNSWIZZLED` flags to `oslCreateImage()`; these flags are not applicable during image creation.
 */
extern OSL_IMAGE *oslCreateImage(int larg, int haut, short location, short pixelFormat);

/**
 * @brief Deletes an image previously created or loaded.
 *
 * This function frees the memory associated with an image that was created or loaded using OSLib. Once deleted, the image pointer should no longer be used, as it will point to invalid memory.
 *
 * @param img
 *        A pointer to the `OSL_IMAGE` structure representing the image to be deleted. If `img` is `NULL`, the function does nothing.
 *
 * @note
 * Ensure that you set the pointer to `NULL` after deletion to avoid accidental usage of the deleted image pointer.
 */
extern void oslDeleteImage(OSL_IMAGE *img);

/**
 * @brief Loads an image from a file, allowing selection of its location and pixel format.
 *
 * This function loads an image from a specified file, automatically handling the details of format detection and memory management based on the provided location and pixel format.
 *
 * @param filename
 *        The path to the image file to be loaded. The function will automatically detect the image format based on the file extension.
 *
 * @param location
 *        The location where the image will be stored (e.g., OSL_IN_RAM or OSL_IN_VRAM). The location affects performance and memory usage.
 *
 * @param pixelFormat
 *        The pixel format for the image. This determines the color depth and memory usage of the loaded image.
 *
 * @return
 *        Returns a pointer to an `OSL_IMAGE` structure representing the loaded image. If the image fails to load (e.g., due to an invalid file path or unsupported format), the function returns `NULL`.
 *
 * @note
 * Always check if the returned image pointer is `NULL` before using it to avoid crashes. Display a user-friendly message if the image fails to load, advising the user to check their files.
 *
 * @warning
 * Avoid using this general-purpose function whenever possible, as it includes support for all image formats, potentially increasing your application's size and memory usage. Instead, use specific loading
 * routines like `oslLoadImageFilePNG`, `oslLoadImageFileJPG`, etc., to conserve RAM.
 *
 * @warning
 * When loading any image, adhere to the considerations in #oslCreateImage, such as the 512x512 size limit. Images exceeding these dimensions are not supported.
 */
extern OSL_IMAGE *oslLoadImageFile(char *filename, int location, int pixelFormat);

#ifdef OSL_IMAGE_LOADER_PNG
/**
 * @brief Loads an image from a PNG file.
 *
 * This function loads an image from a specified PNG file, allowing selection of the memory location and pixel format. It is optimized for PNG files and helps conserve memory by avoiding the inclusion of
 * unnecessary image format support.
 *
 * @param filename
 *        The path to the PNG file to be loaded.
 *
 * @param location
 *        The location where the image will be stored (e.g., OSL_IN_RAM or OSL_IN_VRAM). The location affects performance and memory usage.
 *
 * @param pixelFormat
 *        The pixel format for the image. This determines the color depth and memory usage of the loaded image.
 *
 * @return
 *        Returns a pointer to an `OSL_IMAGE` structure representing the loaded PNG image. If the image fails to load (e.g., due to an invalid file path or unsupported format), the function returns `NULL`.
 *
 * @note
 * Always check if the returned image pointer is `NULL` before using it to avoid crashes. Display a user-friendly message if the image fails to load, advising the user to check their files.
 *
 * @warning
 * The same considerations that apply to `oslLoadImageFile` also apply here, including the 512x512 size limit and the importance of handling `NULL` pointers.
 */
extern OSL_IMAGE *oslLoadImageFilePNG(char *filename, int location, int pixelFormat);
#endif

#ifdef OSL_IMAGE_LOADER_JPEG
/**
 * @brief Loads an image from a JPG file.
 *
 * This function loads an image from a specified JPG file, with options to select the memory location and pixel format. It is optimized for JPG files, but the decompression is limited and may not support
 * high-quality JPGs.
 *
 * @param filename
 *        The path to the JPG file to be loaded.
 *
 * @param location
 *        The location where the image will be stored (e.g., OSL_IN_RAM or OSL_IN_VRAM). The location affects performance and memory usage.
 *
 * @param pixelFormat
 *        The pixel format for the image. This determines the color depth and memory usage of the loaded image.
 *
 * @return
 *        Returns a pointer to an `OSL_IMAGE` structure representing the loaded JPG image. If the image fails to load (e.g., due to an invalid file path, unsupported format, or high-quality JPG),
 *        the function returns `NULL`.
 *
 * @note
 * Always check if the returned image pointer is `NULL` before using it to avoid crashes. Display a user-friendly message if the image fails to load, advising the user to check their files.
 *
 * @warning
 * The JPG decompressor in this function is simple and does not support JPG files with a quality factor higher than 90%. Loading such files may cause the function to crash. Ensure that users are aware of
 * this limitation in your instructions.
 *
 * @see oslLoadImageFile
 */
extern OSL_IMAGE *oslLoadImageFileJPG(char *filename, int location, int pixelFormat);
#endif

#ifdef OSL_IMAGE_LOADER_GIF
/**
 * @brief Loads an image from a GIF file.
 *
 * This function loads an image from a specified GIF file, with options to select the memory location and pixel format. Similar to other image loading functions, it automatically handles the necessary
 * steps for loading the image, but with some limitations regarding the image size and format.
 *
 * @param filename
 *        The path to the GIF file to be loaded.
 *
 * @param location
 *        The location where the image will be stored (e.g., OSL_IN_RAM or OSL_IN_VRAM). This affects the performance and memory usage of the loaded image.
 *
 * @param pixelFormat
 *        The pixel format for the image. This determines the color depth and memory usage of the loaded image.
 *
 * @return
 *        Returns a pointer to an `OSL_IMAGE` structure representing the loaded GIF image. If the image fails to load, the function returns `NULL`.
 *
 * @note
 * Always check if the returned image pointer is `NULL` before using it to avoid crashes. Display a user-friendly message if the image fails to load, advising the user to check their files.
 *
 * @see oslLoadImageFile
 */
extern OSL_IMAGE *oslLoadImageFileGIF(char *filename, int location, int pixelFormat);
#endif

/**
 * @brief Controls automatic swizzling of images.
 *
 * This variable determines whether images are automatically swizzled upon loading. Swizzling optimizes memory access patterns for images in VRAM, enhancing performance during rendering.
 *
 * - If set to a non-zero value, images loaded by functions like `oslLoadImageFile` and its variants will be automatically swizzled if the location is in VRAM.
 * - If set to zero, images will not be swizzled automatically.
 *
 * @note
 * This variable is typically set internally by `oslSetImageAutoSwizzle`, but it can also be modified directly for custom control.
 */
extern int osl_autoSwizzleImages;

/**
 * @brief Controls whether images should be automatically swizzled upon loading.
 *
 * This function allows you to enable or disable automatic swizzling of images when they are loaded into memory. Swizzling is a process that optimizes memory access patterns for images stored in VRAM,
 * improving rendering performance.
 *
 * - **Enabled by default:** Automatic swizzling is enabled by default, which was not the case in the earlier versions of OSLib.
 * - **Swizzling control:** If you need a raw image without swizzling, you can specify `OSL_UNSWIZZLED` when loading the image. Conversely, you can force swizzling by using `OSL_SWIZZLED`.
 *
 * @param enabled
 *        Set this parameter to `1` to enable automatic swizzling or `0` to disable it. When disabled, images will be loaded in their raw format, allowing direct access through the `data` member of the
 *        `OSL_IMAGE` structure.
 *
 * @code
 * // Example usage:
 * // Force loading image as unswizzled
 * oslLoadImageFilePNG("test.png", OSL_IN_RAM | OSL_UNSWIZZLED, OSL_PF_5551);
 * // Force swizzled
 * oslLoadImageFilePNG("test.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_5551);
 * // Will swizzle if osl_autoSwizzleImages is set to true.
 * oslLoadImageFilePNG("test.png", OSL_IN_RAM, OSL_PF_5551);
 * @endcode
 */
static inline void oslSetImageAutoSwizzle(int enabled)
{
	osl_autoSwizzleImages = enabled;
}

/**
 * @brief Controls advanced parameters for image creation.
 *
 * This parameter allows for advanced control over how image sizes are aligned when creating images. It is intended for internal or advanced use and might be subject to breaking changes in future versions.
 *
 * You can set `osl_alignBuffer` to control the alignment of image sizes:
 * - **1**: Align the image width (horizontal size) to the nearest power of two.
 * - **2**: Align the image height (vertical size) to the nearest multiple of 8, which is required for proper swizzling.
 *
 * The default value is **3**, which applies both of the above alignments.
 *
 * \b Warning: This parameter is intended for advanced use cases. Modifying it without understanding its implications may lead to unexpected behavior or compatibility issues.
 *
 * @see oslCreateImage for details on how image sizes are handled.
 */
extern int osl_alignBuffer;

/**
 * @brief Allocates memory for image data based on the properties of the provided OSL_IMAGE structure.
 *
 * This function allocates the necessary memory for an image based on its size and pixel format as defined in the `OSL_IMAGE` structure. It should be called before accessing or manipulating the image data.
 *
 * @param img
 *    Pointer to an `OSL_IMAGE` structure for which memory needs to be allocated.
 * @param location
 *    Specifies the location where the image data should be allocated. Valid values are typically `OSL_IN_RAM` or `OSL_IN_VRAM`.
 *
 * @return
 *    Returns a pointer to the allocated memory for the image data. If memory allocation fails, it may return `NULL`. Always check for `NULL` to avoid accessing invalid memory.
 *
 * \b Important: Ensure that the image's `data` member is properly initialized before using it. This function only allocates memory and does not initialize the data.
 *
 * @see oslCreateImage for image creation and `oslFreeImageData` for memory deallocation.
 */
extern void *oslAllocImageData(OSL_IMAGE *img, int location);

/**
 * @brief Frees the memory allocated for image data.
 *
 * This function deallocates the memory previously allocated for an image's data. It should be called to release memory and prevent memory leaks when the image is no longer needed.
 *
 * @param img
 *    Pointer to an `OSL_IMAGE` structure for which memory should be freed.
 *
 * \b Important: After calling this function, the `data` member of the `OSL_IMAGE` structure becomes invalid. Ensure that the pointer is not used after memory has been freed.
 *
 * @see oslAllocImageData for memory allocation and `oslCreateImage` for image creation.
 */
extern void oslFreeImageData(OSL_IMAGE *img);

/**
 * @brief Adds a VRAM prefix to an address.
 *
 * This macro modifies an address by adding a VRAM prefix. It is used to convert a given address to its VRAM-mapped address by setting the appropriate prefix bit.
 *
 * @param adr
 *    The original address to which the VRAM prefix will be added.
 *
 * @return
 *    The address with the VRAM prefix added. The prefix is added by setting the highest bit of the address.
 *
 * \b Example:
 *    \code
 *    void *vramAddress = oslAddVramPrefixPtr(0x12345678);
 *    \endcode
 *    This will result in `vramAddress` being `0x12345678 | 0x04000000`.
 */
	#define oslAddVramPrefixPtr(adr) ((void *)((int)(adr) | 0x04000000))

/**
 * @brief Removes the VRAM prefix from an address.
 *
 * This macro removes the VRAM prefix from an address. It is used to convert a VRAM-mapped address back to its original address by clearing the VRAM prefix bit.
 *
 * @param adr
 *    The VRAM-mapped address from which the VRAM prefix will be removed.
 *
 * @return
 *    The address with the VRAM prefix removed. The prefix is removed by clearing the highest bit of the address.
 *
 * \b Example:
 *    \code
 *    void *originalAddress = oslRemoveVramPrefixPtr(0x12345678 | 0x04000000);
 *    \endcode
 *    This will result in `originalAddress` being `0x12345678`.
 */
	#define oslRemoveVramPrefixPtr(adr) ((void *)((int)(adr) & (~0x04000000)))

/**
 * @brief Checks if an image location is swizzled.
 *
 * This function determines if the image located at a specific location is swizzled. Swizzling is a technique used to improve performance by rearranging image data in memory.
 *
 * @param location
 *    The location identifier of the image to check. This can be an address or flag indicating where the image is stored.
 *
 * @return
 *    Returns a non-zero value if the image at the specified location is swizzled, and 0 if it is not.
 *
 * \b Note:
 *    This function is intended for internal use and may be subject to change in future versions of OSLib. It is recommended to avoid using this function directly.
 */
extern int oslImageLocationIsSwizzled(int location);

/** @defgroup image_drawing Drawing images
 *
 * Image support in OSLib.
 * @{
 */

	#ifdef PSP
/**
 * @brief Draws an image at the specified position.
 *
 * This macro sets the position of the image `img` to the coordinates `(px, py)` and then draws it using the `oslDrawImage` function.
 *
 * @param img
 *    Pointer to the image to be drawn.
 * @param px
 *    The x-coordinate where the image should be drawn.
 * @param py
 *    The y-coordinate where the image should be drawn.
 *
 * \note
 *    This macro is specifically for PSP and uses `oslDrawImage` to render the image.
 */
	#define oslDrawImageXY(img, px, py) ({ (img)->x=(px), (img)->y=(py); oslDrawImage(img); })

/**
 * @brief Draws an image at the specified position (backward compatibility).
 *
 * This macro is similar to `oslDrawImageXY`, but uses the `oslDrawImageSimple` function for drawing. It is kept for backward compatibility and is not recommended for new code.
 *
 * @param img
 *    Pointer to the image to be drawn.
 * @param px
 *    The x-coordinate where the image should be drawn.
 * @param py
 *    The y-coordinate where the image should be drawn.
 *
 * \note
 *    This macro is specifically for PSP and uses `oslDrawImageSimple`, which is kept for compatibility but should be avoided in new code.
 */
	#define oslDrawImageSimpleXY(img, px, py) ({ (img)->x=(px), (img)->y=(py); oslDrawImageSimple(img); })
	#else
/**
 * @brief Draws an image at the specified position.
 *
 * This macro sets the position of the image `img` to the coordinates `(px, py)` and then draws it using the `oslDrawImage` function.
 *
 * @param img
 *    Pointer to the image to be drawn.
 * @param px
 *    The x-coordinate where the image should be drawn.
 * @param py
 *    The y-coordinate where the image should be drawn.
 *
 * \note
 *    This macro is used in environments other than PSP. It sets the image's position and then calls `oslDrawImage`.
 */
	#define oslDrawImageXY(img, px, py)       \
		{                                     \
			(img)->x = (px), (img)->y = (py); \
			oslDrawImage(img);                \
		}

/**
 * @brief Draws an image at the specified position (backward compatibility).
 *
 * This macro is similar to `oslDrawImageXY`, but uses the `oslDrawImageSimple` function for drawing. It is kept for backward compatibility and is not recommended for new code.
 *
 * @param img
 *    Pointer to the image to be drawn.
 * @param px
 *    The x-coordinate where the image should be drawn.
 * @param py
 *    The y-coordinate where the image should be drawn.
 *
 * \note
 *    This macro is used in environments other than PSP and uses `oslDrawImageSimple`, which is kept for compatibility but should be avoided in new code.
 */
	#define oslDrawImageSimpleXY(img, px, py) \
		{                                     \
			(img)->x = (px), (img)->y = (py); \
			oslDrawImageSimple(img);          \
		}
	#endif

/**
 * @brief Draws an image at its current position.
 *
 * This function renders the image specified by the `img` parameter at the position stored in the image's `x` and `y` attributes. The image must be properly initialized and loaded before calling this function.
 *
 * @param img
 *    Pointer to the `OSL_IMAGE` structure representing the image to be drawn.
 *
 * \note
 *    Ensure that the image is correctly loaded and initialized before calling this function. The image's position is determined by its `x` and `y` attributes, which should be set before drawing.
 */
extern void oslDrawImage(OSL_IMAGE *img);

/**
 * @brief Draws an image at its current position (deprecated).
 *
 * This function renders the image specified by the `img` parameter at the position stored in the image's `x` and `y` attributes. It is provided for backward compatibility with older code but should not be used in new code.
 *
 * @param img
 *    Pointer to the `OSL_IMAGE` structure representing the image to be drawn.
 *
 * \note
 *    This function is maintained for compatibility with older code. For new code, use `oslDrawImage` instead. Ensure the image is properly initialized and its position is correctly set before calling this function.
 */
extern void oslDrawImageSimple(OSL_IMAGE *img);

/**
 * @brief Draws a very large image.
 *
 * This function is designed to draw images larger than the standard 512x512 pixel limit. It provides a way to render images up to 1024 pixels in width, but comes with additional complexity and potentially significant performance drawbacks.
 *
 * \note
 *    The PSP hardware is not optimized for handling images larger than 512x512 pixels. While this function allows you to draw larger images, it can be extremely slow and is not recommended for typical use. The GPU must process all the pixels of the image, even if you scale it down, which can be very inefficient.
 *
 *    Texture wrapping (`oslSetTextureWrap`) is not supported. Make sure that offset coordinates are within the bounds of the image to avoid unpredictable results.
 *
 * \warning
 *    Using this function may severely impact performance. Always try to rescale your images to an acceptable size (less than 512x512) during loading. This function is available if you need it, but it's essential to understand its limitations and performance implications.
 *
 * @param img
 *    Pointer to the `OSL_IMAGE` structure representing the image to be drawn.
 *
 * \deprecated
 *    This function is deprecated and may change or be removed in future releases of OSLib. It is recommended to use `oslDrawImage` for standard image drawing needs.
 */
extern void oslDrawImageBig(OSL_IMAGE *img);

/** @} */         // end of image_drawing

/** @defgroup image_placement Image placement
 *
 * Image placement in OSLib.
 * @{
 */

#ifdef PSP
/**
 * @brief Sets the source rectangle of the image to be displayed.
 *
 * This macro allows you to define a sub-region of a larger image to be rendered. This is useful when you have an image that contains multiple sprites or tiles arranged in a grid.
 *
 * For example, if you have a 128x128 image containing 16 sprites of 32x32 each, you can use this macro to select and display a specific sprite:
 *
 * \code
 * oslSetImageTile(image, 0, 0, 32, 32);
 * oslDrawImage(image);
 * \endcode
 *
 * This will display the portion of the image starting at (0,0) and ending at (32,32), rendering a 32x32 sprite. To display the next sprite to the right, you would use:
 *
 * \code
 * oslSetImageTile(image, 32, 0, 64, 32);
 * \endcode
 *
 * @param img
 *    The image from which a tile is to be extracted.
 * @param x0
 *    The x-coordinate of the top-left corner of the tile.
 * @param y0
 *    The y-coordinate of the top-left corner of the tile.
 * @param x1
 *    The x-coordinate of the bottom-right corner of the tile (exclusive).
 * @param y1
 *    The y-coordinate of the bottom-right corner of the tile (exclusive).
 *
 * @note
 *    The specified coordinates are used to define the source rectangle within the image. The image will be drawn with the specified tile dimensions.
 *
 * @see
 *    Refer to `oslDrawImage` for rendering the image with the selected tile.
 */
	#define oslSetImageTile(img, x0, y0, x1, y1) ({ (img)->offsetX0=(float)x0, (img)->offsetY0=(float)y0, (img)->offsetX1=(float)x1, (img)->offsetY1=(float)y1; (img)->stretchX = oslAbs((int)((float)((img)->offsetX1) - (float)((img)->offsetX0))), (img)->stretchY = oslAbs((int)((float)((img)->offsetY1) - (float)((img)->offsetY0))); })

/**
 * @brief Sets the source rectangle of the image to be displayed using tile size.
 *
 * This macro allows you to define a sub-region of a larger image to be rendered by specifying the size of the tile directly. This simplifies the process of defining a tile or sprite within an image.
 *
 * For example, if you have a 128x128 image containing 32x32 sprites, you can use this macro to select and display a specific sprite by setting the size of the tile:
 *
 * \code
 * oslSetImageTileSize(image, 0, 0, 32, 32);
 * oslDrawImage(image);
 * \endcode
 *
 * This will display a 32x32 tile starting from (0,0) and ending at (32,32). To display a sprite starting at (32,0) with the same size, you would use:
 *
 * \code
 * oslSetImageTileSize(image, 32, 0, 32, 32);
 * \endcode
 *
 * @param img
 *    The image from which a tile is to be extracted.
 * @param x0
 *    The x-coordinate of the top-left corner of the tile.
 * @param y0
 *    The y-coordinate of the top-left corner of the tile.
 * @param x1
 *    The width of the tile.
 * @param y1
 *    The height of the tile.
 *
 * @note
 *    The specified coordinates define the starting position and size of the tile within the image. The image will be drawn with the specified tile dimensions.
 *
 * @see
 *    Refer to `oslDrawImage` for rendering the image with the selected tile.
 */
	#define oslSetImageTileSize(img, x0, y0, x1, y1) ({ (img)->offsetX0 = (float)(x0), (img)->offsetY0 = (float)(y0), (img)->offsetX1 = (float)(x0) + (float)(x1), (img)->offsetY1 = (float)(y0) + (float)(y1), (img)->stretchX = (int)(x1), (img)->stretchY = (int)(y1); })

/**
 * @brief Resets the image rectangle to its full (original) size.
 *
 * This macro resets the portion of the image to be drawn to cover the entire image. It effectively cancels any previous tile or sub-region settings, allowing you to draw the full image again.
 *
 * For example, if you previously set a specific tile or sub-region of the image using `oslSetImageTile` or `oslSetImageTileSize`, you can use this macro to revert to displaying the entire image.
 *
 * \code
 * oslSetImageTile(image, 0, 0, 32, 32);
 * oslDrawImage(image);
 * // Reset to full image
 * oslResetImageTile(image);
 * oslDrawImage(image);
 * \endcode
 *
 * @param img
 *    The image whose rectangle is to be reset.
 *
 * @note
 *    After calling this macro, the image will be rendered using its full dimensions (`sizeX` and `sizeY`), effectively ignoring any previously set tile or sub-region settings.
 *
 * @see
 *    Refer to `oslSetImageTile` and `oslSetImageTileSize` for setting specific sub-regions of an image.
 */
	#define oslResetImageTile(img) ({ (img)->offsetX0=0, (img)->offsetY0=0, (img)->offsetX1=(img)->sizeX, (img)->offsetY1=(img)->sizeY; (img)->stretchX = (img)->sizeX, (img)->stretchY = (img)->sizeY; })

/**
 * @brief Mirrors an image (or an image tile) horizontally.
 *
 * This macro flips the image horizontally by swapping the `offsetX0` and `offsetX1` coordinates of the image. It effectively mirrors the image along the vertical axis.
 * If this macro is used in a loop or multiple times, make sure to reset the mirroring after drawing the image to avoid undesired effects, as each application of this macro will mirror the image again.
 *
 * For example, if you want to mirror an image horizontally and then reset it to its original orientation, you might use:
 *
 * \code
 * oslMirrorImageH(image);
 * oslDrawImage(image);
 * oslResetImageTile(image); // Reset after drawing
 * \endcode
 *
 * @param img
 *    The image to be mirrored.
 *
 * @note
 *    Mirroring an image twice will revert it to its original orientation. If you are applying mirroring in a loop, ensure to reset the mirroring state after drawing to maintain correct orientation.
 *
 * @see
 *    Use `oslMirrorImageV` to mirror an image vertically.
 */
	#define oslMirrorImageH(img) ({ float ___tmp;  ___tmp = (img)->offsetX0; (img)->offsetX0 = (img)->offsetX1; (img)->offsetX1 = ___tmp; })

/**
 * @brief Mirrors an image (or an image tile) vertically.
 *
 * This macro flips the image vertically by swapping the `offsetY0` and `offsetY1` coordinates of the image. It effectively mirrors the image along the horizontal axis.
 * If this macro is used in a loop or multiple times, make sure to reset the mirroring after drawing the image to avoid undesired effects, as each application of this macro will mirror the image again.
 *
 * For example, if you want to mirror an image vertically and then reset it to its original orientation, you might use:
 *
 * \code
 * oslMirrorImageV(image);
 * oslDrawImage(image);
 * oslResetImageTile(image); // Reset after drawing
 * \endcode
 *
 * @param img
 *    The image to be mirrored.
 *
 * @note
 *    Mirroring an image twice will revert it to its original orientation. If you are applying mirroring in a loop, ensure to reset the mirroring state after drawing to maintain correct orientation.
 *
 * @see
 *    Use `oslMirrorImageH` to mirror an image horizontally.
 */
	#define oslMirrorImageV(img) ({ float ___tmp;  ___tmp = (img)->offsetY0; (img)->offsetY0 = (img)->offsetY1; (img)->offsetY1 = ___tmp; })
#else
/**
 * @brief Sets the source rectangle of the image to be displayed.
 *
 * This macro allows you to define a sub-region of a larger image to be rendered. This is useful when you have an image that contains multiple sprites or tiles arranged in a grid.
 *
 * For example, if you have a 128x128 image containing 16 sprites of 32x32 each, you can use this macro to select and display a specific sprite:
 *
 * \code
 * oslSetImageTile(image, 0, 0, 32, 32);
 * oslDrawImage(image);
 * \endcode
 *
 * This will display the portion of the image starting at (0,0) and ending at (32,32), rendering a 32x32 sprite. To display the next sprite to the right, you would use:
 *
 * \code
 * oslSetImageTile(image, 32, 0, 64, 32);
 * \endcode
 *
 * @param img
 *    The image from which a tile is to be extracted.
 * @param x0
 *    The x-coordinate of the top-left corner of the tile.
 * @param y0
 *    The y-coordinate of the top-left corner of the tile.
 * @param x1
 *    The x-coordinate of the bottom-right corner of the tile (exclusive).
 * @param y1
 *    The y-coordinate of the bottom-right corner of the tile (exclusive).
 *
 * @note
 *    The specified coordinates are used to define the source rectangle within the image. The image will be drawn with the specified tile dimensions.
 *
 * @see
 *    Refer to `oslDrawImage` for rendering the image with the selected tile.
 */
	#define oslSetImageTile(img, x0, y0, x1, y1)                                                                                                                                        \
		{                                                                                                                                                                               \
			(img)->offsetX0 = (float)x0, (img)->offsetY0 = (float)y0, (img)->offsetX1 = (float)x1, (img)->offsetY1 = (float)y1;                                                         \
			(img)->stretchX = oslAbs((int)((float)((img)->offsetX1) - (float)((img)->offsetX0))), (img)->stretchY = oslAbs((int)((float)((img)->offsetY1) - (float)((img)->offsetY0))); \
		}

/**
 * @brief Sets the source rectangle of the image to be displayed using tile size.
 *
 * This macro allows you to define a sub-region of a larger image to be rendered by specifying the size of the tile directly. This simplifies the process of defining a tile or sprite within an image.
 *
 * For example, if you have a 128x128 image containing 32x32 sprites, you can use this macro to select and display a specific sprite by setting the size of the tile:
 *
 * \code
 * oslSetImageTileSize(image, 0, 0, 32, 32);
 * oslDrawImage(image);
 * \endcode
 *
 * This will display a 32x32 tile starting from (0,0) and ending at (32,32). To display a sprite starting at (32,0) with the same size, you would use:
 *
 * \code
 * oslSetImageTileSize(image, 32, 0, 32, 32);
 * \endcode
 *
 * @param img
 *    The image from which a tile is to be extracted.
 * @param x0
 *    The x-coordinate of the top-left corner of the tile.
 * @param y0
 *    The y-coordinate of the top-left corner of the tile.
 * @param x1
 *    The width of the tile.
 * @param y1
 *    The height of the tile.
 *
 * @note
 *    The specified coordinates define the starting position and size of the tile within the image. The image will be drawn with the specified tile dimensions.
 *
 * @see
 *    Refer to `oslDrawImage` for rendering the image with the selected tile.
 */
	#define oslSetImageTileSize(img, x0, y0, x1, y1)                                                                                                                                                                          \
		{                                                                                                                                                                                                                     \
			(img)->offsetX0 = (float)(x0), (img)->offsetY0 = (float)(y0), (img)->offsetX1 = (float)(x0) + (float)(x1), (img)->offsetY1 = (float)(y0) + (float)(y1), (img)->stretchX = (int)(x1), (img)->stretchY = (int)(y1); \
		}

/**
 * @brief Resets the image rectangle to its full (original) size.
 *
 * This macro resets the portion of the image to be drawn to cover the entire image. It effectively cancels any previous tile or sub-region settings, allowing you to draw the full image again.
 *
 * For example, if you previously set a specific tile or sub-region of the image using `oslSetImageTile` or `oslSetImageTileSize`, you can use this macro to revert to displaying the entire image.
 *
 * \code
 * oslSetImageTile(image, 0, 0, 32, 32);
 * oslDrawImage(image);
 * // Reset to full image
 * oslResetImageTile(image);
 * oslDrawImage(image);
 * \endcode
 *
 * @param img
 *    The image whose rectangle is to be reset.
 *
 * @note
 *    After calling this macro, the image will be rendered using its full dimensions (`sizeX` and `sizeY`), effectively ignoring any previously set tile or sub-region settings.
 *
 * @see
 *    Refer to `oslSetImageTile` and `oslSetImageTileSize` for setting specific sub-regions of an image.
 */
	#define oslResetImageTile(img)                                                                                    \
		{                                                                                                             \
			(img)->offsetX0 = 0, (img)->offsetY0 = 0, (img)->offsetX1 = (img)->sizeX, (img)->offsetY1 = (img)->sizeY; \
			(img)->stretchX = (img)->sizeX, (img)->stretchY = (img)->sizeY;                                           \
		}

/**
 * @brief Mirrors an image (or an image tile) horizontally.
 *
 * This macro flips the image horizontally by swapping the `offsetX0` and `offsetX1` coordinates of the image. It effectively mirrors the image along the vertical axis.
 * If this macro is used in a loop or multiple times, make sure to reset the mirroring after drawing the image to avoid undesired effects, as each application of this macro will mirror the image again.
 *
 * For example, if you want to mirror an image horizontally and then reset it to its original orientation, you might use:
 *
 * \code
 * oslMirrorImageH(image);
 * oslDrawImage(image);
 * oslResetImageTile(image); // Reset after drawing
 * \endcode
 *
 * @param img
 *    The image to be mirrored.
 *
 * @note
 *    Mirroring an image twice will revert it to its original orientation. If you are applying mirroring in a loop, ensure to reset the mirroring state after drawing to maintain correct orientation.
 *
 * @see
 *    Use `oslMirrorImageV` to mirror an image vertically.
 */
	#define oslMirrorImageH(img)               \
		{                                      \
			float ___tmp;                      \
			___tmp = (img)->offsetX0;          \
			(img)->offsetX0 = (img)->offsetX1; \
			(img)->offsetX1 = ___tmp;          \
		}

/**
 * @brief Mirrors an image (or an image tile) vertically.
 *
 * This macro flips the image vertically by swapping the `offsetY0` and `offsetY1` coordinates of the image. It effectively mirrors the image along the horizontal axis.
 * If this macro is used in a loop or multiple times, make sure to reset the mirroring after drawing the image to avoid undesired effects, as each application of this macro will mirror the image again.
 *
 * For example, if you want to mirror an image vertically and then reset it to its original orientation, you might use:
 *
 * \code
 * oslMirrorImageV(image);
 * oslDrawImage(image);
 * oslResetImageTile(image); // Reset after drawing
 * \endcode
 *
 * @param img
 *    The image to be mirrored.
 *
 * @note
 *    Mirroring an image twice will revert it to its original orientation. If you are applying mirroring in a loop, ensure to reset the mirroring state after drawing to maintain correct orientation.
 *
 * @see
 *    Use `oslMirrorImageH` to mirror an image horizontally.
 */
	#define oslMirrorImageV(img)               \
		{                                      \
			float ___tmp;                      \
			___tmp = (img)->offsetY0;          \
			(img)->offsetY0 = (img)->offsetY1; \
			(img)->offsetY1 = ___tmp;          \
		}

#endif

/**
 * @brief Returns whether the image is mirrored horizontally.
 *
 * This macro checks if the image has been mirrored horizontally. An image is considered to be mirrored horizontally if the x-coordinate of the bottom-right corner
 * (`offsetX1`) is greater than the x-coordinate of the top-left corner (`offsetX0`).
 *
 * @param img
 *    The image to check for horizontal mirroring.
 *
 * @return
 *    Returns `true` if the image was mirrored horizontally, `false` otherwise.
 *
 * @see
 *    Use `oslMirrorImageH` to mirror the image horizontally.
 */
	#define oslImageIsMirroredH(img) ((img)->offsetX1 > (img)->offsetX0)

/**
 * @brief Returns whether the image is mirrored vertically.
 *
 * This macro checks if the image has been mirrored vertically. An image is considered to be mirrored vertically if the y-coordinate of the bottom-right corner
 * (`offsetY1`) is greater than the y-coordinate of the top-left corner (`offsetY0`).
 *
 * @param img
 *    The image to check for vertical mirroring.
 *
 * @return
 *    Returns `true` if the image was mirrored vertically, `false` otherwise.
 *
 * @see
 *    Use `oslMirrorImageV` to mirror the image vertically.
 */
	#define oslImageIsMirroredV(img) ((img)->offsetY1 > (img)->offsetY0)

/**
 * @brief Calculates the x-coordinate for moving an image so that its top-left corner is at the specified position when no rotation is applied.
 *
 * This macro computes the x-coordinate for positioning an image's top-left corner at `(x, y)` considering the image's rotation parameters. It takes into account the rotation center and the size of the image.
 *
 * @param img
 *    The image to position.
 * @param x
 *    The desired x-coordinate for the top-left corner of the image.
 *
 * @return
 *    The x-coordinate adjusted for the rotation center.
 *
 * @see
 *    Use `oslImageRotMoveY` for the y-coordinate.
 */
	#define oslImageRotMoveX(img, x) ((((x) + (img)->centerX * (img)->stretchX) / (img)->sizeX))

/**
 * @brief Calculates the y-coordinate for moving an image so that its top-left corner is at the specified position when no rotation is applied.
 *
 * This macro computes the y-coordinate for positioning an image's top-left corner at `(x, y)` considering the image's rotation parameters. It takes into account the rotation center and the size of the image.
 *
 * @param img
 *    The image to position.
 * @param y
 *    The desired y-coordinate for the top-left corner of the image.
 *
 * @return
 *    The y-coordinate adjusted for the rotation center.
 *
 * @see
 *    Use `oslImageRotMoveX` for the x-coordinate.
 */
	#define oslImageRotMoveY(img, y) ((((y) + (img)->centerY * (img)->stretchY) / (img)->sizeY))

/**
 * @brief Sets the rotation center of an image to its center.
 *
 * This macro sets the rotation center (`centerX`, `centerY`) to the middle of the image. This is useful for rotation operations where the center of rotation is typically the middle of the image.
 *
 * @param img
 *    The image to modify.
 *
 * @see
 *    Use `oslImageSetRotCenter` as an alias for this macro.
 */
	#define oslSetImageRotCenter(img) ((img)->centerX = (int)oslAbs((img)->offsetX1 - (img)->offsetX0) >> 1, (img)->centerY = (int)oslAbs((img)->offsetY1 - (img)->offsetY0) >> 1)

/**
 * @brief Alias for `oslSetImageRotCenter`.
 *
 * This macro is an alias for `oslSetImageRotCenter`, setting the rotation center to the middle of the image.
 *
 * @param img
 *    The image to modify.
 *
 * @see
 *    Use `oslSetImageRotCenter` for detailed documentation.
 */
	#define oslImageSetRotCenter oslSetImageRotCenter

/**
 * @brief Crops 0.5 pixel from each corner of the image.
 *
 * This macro adjusts the image's borders to eliminate visual artifacts caused by bilinear filtering when the image is stretched. By cropping 0.5 pixel from each corner, it helps prevent unwanted borders that might appear around the image.
 *
 * @param img
 *    The image to correct.
 *
 * @note
 *    This is a hacky routine and should be used carefully. It's intended for use with bilinear filtering and stretched images.
 */
	#define oslCorrectImageHalfBorder(img) ((img)->offsetX0 = (img)->offsetX0 + 0.5f, (img)->offsetY0 = (img)->offsetY0 + 0.5f, (img)->offsetX1 = (img)->offsetX1 - 0.5f, (img)->offsetY1 = (img)->offsetY1 - 0.5f, img->stretchX--, img->stretchY--)

/**
 * @brief Undoes the effect of `oslCorrectImageHalfBorder`.
 *
 * This macro reverts the adjustments made by `oslCorrectImageHalfBorder`. It should be used with caution, as calling it without a preceding `oslCorrectImageHalfBorder` or calling it multiple times may lead to incorrect image adjustments.
 *
 * @param img
 *    The image to reset.
 *
 * @note
 *    Prefer using `oslResetImageTile` for more consistent behavior.
 */
	#define oslResetImageHalfBorder(img) ((img)->offsetX0 = (img)->offsetX0 - 0.5f, (img)->offsetY0 = (img)->offsetY0 - 0.5f, (img)->offsetX1 = (img)->offsetX1 + 0.5f, (img)->offsetY1 = (img)->offsetY1 + 0.5f, img->stretchX++, img->stretchY++)

/**
 * @brief Returns the width of the image, considering the current frame or tile.
 *
 * This macro computes the width of the image based on the current frame or tile settings. To retrieve the full image width, use `image->sizeX`.
 *
 * @param img
 *    The image to measure.
 *
 * @return
 *    The width of the image frame or tile.
 */
	#define oslGetImageWidth(img) ((int)oslAbs((img)->offsetX1 - (img)->offsetX0))

/**
 * @brief Returns the height of the image, considering the current frame or tile.
 *
 * This macro computes the height of the image based on the current frame or tile settings. To retrieve the full image height, use `image->sizeY`.
 *
 * @param img
 *    The image to measure.
 *
 * @return
 *    The height of the image frame or tile.
 */
	#define oslGetImageHeight(img) ((int)oslAbs((img)->offsetY1 - (img)->offsetY0))

/**
 * @brief Returns the width of the image, considering the current frame or tile.
 *
 * This macro is an alias for `oslGetImageWidth`, providing the width of the image based on the current frame or tile settings. To get the full image width, use `image->sizeX`.
 *
 * @param img
 *    The image to measure.
 *
 * @return
 *    The width of the image frame or tile.
 */
	#define oslGetImageSizeX oslGetImageWidth

/**
 * @brief Returns the height of the image, considering the current frame or tile.
 *
 * This macro is an alias for `oslGetImageHeight`, providing the height of the image based on the current frame or tile settings. To get the full image height, use `image->sizeY`.
 *
 * @param img
 *    The image to measure.
 *
 * @return
 *    The height of the image frame or tile.
 */
	#define oslGetImageSizeY oslGetImageHeight

/** @} */         // end of image_placement

/** @defgroup image_manip Image manipulation
 *
 * Image manipulation in OSLib.
 * @{
 */

/** Creates an alternate image referencing a part (tile) of another one.

        @param img
                The original image from which a tile will be created.
        @param offsetX0
                The starting x position of the image part to reference.
        @param offsetY0
                The starting y position of the image part to reference.
        @param offsetX1
                The ending x position of the image part to reference.
        @param offsetY1
                The ending y position of the image part to reference.

        This will return a pointer to a new image, but the data will not be copied to the new image. Instead, it will reference the original
        image data. When you delete the new image, the original image will be left unchanged; only the size of an `OSL_IMAGE` will be freed from memory
        (about 80 bytes). However, if you delete the original image, the new one will be unusable. Attempting to draw it may result in corrupt data,
        and writing to it may cause a crash.

        Example usage:
        \code
        OSL_IMAGE *imageTile = oslCreateImageTile(originalImage, 0, 0, 32, 32);
        oslDrawImage(imageTile);                                  // Will draw a 32x32 image inside of originalImage
        oslDeleteImage(imageTile);                                // Only frees the copy, the original remains untouched!
        \endcode

        This routine was meant to create individual "sprite" images from a big one and manipulate them easily. However, as it's not very clear,
        it's advised to avoid using it. An alternative approach would be:
        \code
        oslSetImageTile(originalImage, 0, 0, 32, 32);
        oslDrawImage(originalImage);                              // Will draw a 32x32 image inside of originalImage
        \endcode
 */
extern OSL_IMAGE *oslCreateImageTile(OSL_IMAGE *img, int offsetX0, int offsetY0, int offsetX1, int offsetY1);

/** Creates an alternate image referencing a part (tile) of another one using width and height.

        @param img
                The original image from which a tile will be created.
        @param offsetX0
                The starting x position of the image part to reference.
        @param offsetY0
                The starting y position of the image part to reference.
        @param width
                The width of the image part to reference.
        @param height
                The height of the image part to reference.

        This function returns a pointer to a new image, but the data will not be copied to the new image. Instead, it will reference the original
        image data. When you delete the new image, the original image will be left unchanged; only the size of an `OSL_IMAGE` will be freed from memory
        (about 80 bytes). However, if you delete the original image, the new one will be unusable. Attempting to draw it may result in corrupt data,
        and writing to it may cause a crash.

        Example usage:
        \code
        OSL_IMAGE *imageTile = oslCreateImageTileSize(originalImage, 0, 0, 32, 32);
        oslDrawImage(imageTile);                                  // Will draw a 32x32 image inside of originalImage
        oslDeleteImage(imageTile);                                // Only frees the copy, the original remains untouched!
        \endcode

        This routine was designed to create individual "sprite" images from a large image using width and height specifications. An alternative approach
        for creating image tiles is using coordinates with the `oslSetImageTile` function.
 */
extern OSL_IMAGE *oslCreateImageTileSize(OSL_IMAGE *img, int offsetX0, int offsetY0, int width, int height);

/** Converts an image to another pixel format and places it in another location.

        @param imgOriginal
                The original image to be converted.
        @param newLocation
                The location where the converted image should be placed. Common locations include `OSL_IN_RAM` and `OSL_IN_VRAM`.
        @param newFormat
                The pixel format to convert the image to, such as `OSL_PF_5551`, `OSL_PF_8888`, etc.

        This function converts an image to a different pixel format and places it in a specified location. The original image may have been moved or modified
        during the conversion process, so you should use the returned image instead of the original one.

        Example usage:
        \code
        image = oslConvertImageTo(image, OSL_IN_RAM, OSL_PF_5551);
        \endcode

        After calling this function, the `imgOriginal` image should no longer be used directly. Instead, the function returns a new image with the specified
        pixel format and location. Ensure to properly manage and delete the converted image as needed.
 */
extern OSL_IMAGE *oslConvertImageTo(OSL_IMAGE *imgOriginal, int newLocation, int newFormat);

/** Creates a copy of an image.

        @param src
                The source image to be copied.
        @param newLocation
                The location where the copied image should be placed. Common locations include `OSL_IN_RAM` and `OSL_IN_VRAM`.

        This function creates a complete copy of the source image, including its image data. The returned image is a new instance, independent of the original. Changes to the copied image will not affect the source image and vice versa.

        Example usage:
        \code
        OSL_IMAGE *originalImage = oslLoadImageFile("example.png", OSL_IN_RAM, OSL_PF_8888);
        OSL_IMAGE *copiedImage = oslCreateImageCopy(originalImage, OSL_IN_VRAM);
        // Now you can use copiedImage independently of originalImage
        \endcode

        Both the original and copied images are not linked together. Ensure to manage and free the memory for both images as needed.
 */
extern OSL_IMAGE *oslCreateImageCopy(OSL_IMAGE *src, int newLocation);

/** Creates a copy of an image and swizzles the new one.

        @param src
                The source image to be copied.
        @param newLocation
                The location where the swizzled image should be placed. Common locations include `OSL_IN_RAM` and `OSL_IN_VRAM`.

        This function creates a complete copy of the source image and applies swizzling to the new image. Swizzling is a technique used to improve texture performance by rearranging the image data. The returned image is a new instance, independent of the original.

        Example usage:
        \code
        OSL_IMAGE *originalImage = oslLoadImageFile("example.png", OSL_IN_RAM, OSL_PF_8888);
        OSL_IMAGE *swizzledImage = oslCreateSwizzledImage(originalImage, OSL_IN_VRAM);
        // Now you can use swizzledImage independently of originalImage
        \endcode

        Both the original and swizzled images are not linked together. Ensure to manage and free the memory for both images as needed.
 */
extern OSL_IMAGE *oslCreateSwizzledImage(OSL_IMAGE *src, int newLocation);

/** Copies the content of one image to another.

        @param imgDst
                The destination image where the content will be copied to.
        @param imgSrc
                The source image from which the content will be copied.

        This function copies the image data from `imgSrc` to `imgDst`. Both images must have the same dimensions (width and height) and pixel format. If the formats do not match, the copy operation will fail, and the destination image may not be updated correctly.

        Note: This function only copies the image data and does not change the properties of `imgDst`. Ensure that `imgDst` has been properly initialized and allocated with the same format and size as `imgSrc`.

        Example usage:
        \code
        OSL_IMAGE *sourceImage = oslLoadImageFile("source.png", OSL_IN_RAM, OSL_PF_8888);
        OSL_IMAGE *destinationImage = oslCreateImage(sourceImage->sizeX, sourceImage->sizeY, OSL_IN_RAM, OSL_PF_8888);
        oslCopyImageTo(destinationImage, sourceImage);
        // Now destinationImage contains a copy of the sourceImage data
        \endcode

        Ensure both images have the same properties before calling this function to avoid unexpected behavior.
 */
extern void oslCopyImageTo(OSL_IMAGE *imgDst, OSL_IMAGE *imgSrc);

/** Swizzles an image to improve drawing performance.

        @param img
                The image to be swizzled.

        Swizzling is a process that optimizes image drawing performance by reorganizing the image data into a format that is more efficient for the GPU to handle. After swizzling, the image can be drawn much faster. However, swizzled images cannot be modified directly due to their special format.

        If you need to modify the image after swizzling, you will need to unswizzle it, which can be slow. Therefore, it is recommended to swizzle images that are stored in RAM and are not intended to be modified frequently.

        Hint: When loading images, use the `OSL_SWIZZLED` bit to automatically swizzle images for convenience.

        Example usage:
        \code
        OSL_IMAGE *image = oslLoadImageFile("example.png", OSL_IN_RAM, OSL_PF_8888);
        oslSwizzleImage(image);
        // The image is now swizzled and can be drawn faster
        \endcode

        Be cautious with unswizzling images frequently, as it can impact performance. Use swizzling for images that are primarily used for rendering and do not require modifications.
 */
extern void oslSwizzleImage(OSL_IMAGE *img);

/** Restores an image from its swizzled format to its original format.

        @param img
                The swizzled image to be unswizzled.

        This function performs the opposite operation of `oslSwizzleImage`, converting the swizzled image back to its original format. After unswizzling, the image will be accessible for raw reading and writing.

        Note that unswizzling an image can be a slow process, so it should be used judiciously. It's generally best to swizzle images for performance improvements and avoid frequent unswizzling unless necessary.

        Example usage:
        \code
        OSL_IMAGE *swizzledImage = oslLoadImageFile("example.png", OSL_IN_RAM, OSL_PF_8888);
        oslSwizzleImage(swizzledImage);
        // Use the image for fast drawing
        oslUnswizzleImage(swizzledImage);
        // Now you can read or write to the image directly
        \endcode

        Make sure to only unswizzle images that were previously swizzled to avoid potential issues with image data integrity.
 */
extern void oslUnswizzleImage(OSL_IMAGE *img);

/** Swizzles an image and writes the result to another image.

        @param imgDst
                The destination image where the swizzled data will be written. This image must have enough memory allocated to hold the swizzled data.
        @param imgSrc
                The source image that will be swizzled.

        This function swizzles the `imgSrc` image and writes the swizzled result to `imgDst`. This routine is considered outdated. For modern usage, it is recommended to use `oslSwizzleImage` which operates directly on a single image.

        Example usage:
        \code
        OSL_IMAGE *sourceImage = oslLoadImageFile("example.png", OSL_IN_RAM, OSL_PF_8888);
        OSL_IMAGE *destinationImage = oslCreateImage(sourceImage->sizeX, sourceImage->sizeY, OSL_PF_8888);
        oslSwizzleImageTo(destinationImage, sourceImage);
        // Use destinationImage for fast drawing
        \endcode

        Note: Ensure that `imgDst` has been properly allocated and has sufficient size to accommodate the swizzled image data from `imgSrc`.
 */
extern void oslSwizzleImageTo(OSL_IMAGE *imgDst, OSL_IMAGE *imgSrc);

/** Moves an image to a new location in memory.

        @param img
                The image to be moved.
        @param newLocation
                The new location where the image should be moved. This can be either `OSL_IN_RAM` or `OSL_IN_VRAM`.

        This function relocates an image to a new memory location. The new location can be either RAM or VRAM. After moving, the image can be used in its new location, but note that moving images between different memory types might have implications for performance and accessibility.

        Example usage:
        \code
        OSL_IMAGE *image = oslLoadImageFile("example.png", OSL_IN_RAM, OSL_PF_8888);
        oslMoveImageTo(image, OSL_IN_VRAM);
        // Now the image is in VRAM and can be used for rendering
        \endcode

        Returns `true` if the move was successful, and `false` otherwise.
 */
extern bool oslMoveImageTo(OSL_IMAGE *img, int newLocation);

/** Clears an image with a specific value.

        @param img
                The image to be cleared. This image must be properly allocated and must have a format compatible with the provided color value.
        @param color
                The color value used to clear the image. The format of this color value depends on the pixel format of the image. For example:
                - If the image format is `OSL_PF_4BIT`, the color value is a palette entry number.
                - If the image format is `OSL_PF_5551`, the color value is a 15-bit color with alpha.

        This function fills the entire image with the specified color value. The color is applied according to the image's pixel format. For instance, in a `OSL_PF_4444` format, you might use a 16-bit color value where each channel has 4 bits.

        Example usage:
        \code
        OSL_IMAGE *img = oslCreateImage(32, 32, OSL_IN_RAM, OSL_PF_4444);
        // Clear the image to black with full opacity (alpha=255)
        oslClearImage(img, RGBA12(0, 0, 0, 255));
        \endcode

        Note: The `color` parameter must be appropriate for the image's pixel format. The provided value should match the format's bit depth and channel order.
 */
extern void oslClearImage(OSL_IMAGE *img, int color);         // A tester!!!

/** Writes an image to a file.

        @param img
                The image to be written to the file. This image must not be swizzled; if it is, the function will not perform any operation.
        @param filename
                The path to the file where the image will be saved. The file type is determined based on the file extension.
        @param flags
                Options for the image writing operation. These flags control how the image is processed during writing. The specific flags available depend on the implementation and version of OSLib.

        @return
                Returns a status code indicating the success or failure of the operation. The exact values and meaning of the return codes depend on the implementation.

        This function saves the provided image to the specified file. The file type is automatically detected based on the file extension, which includes support for various formats. However, using this function can increase the size of your project and use unnecessary RAM because it supports every possible format.

        \b Note: If the image is swizzled, the function will not perform any action. To write a swizzled image, you must first unswizzle it using the #oslUnswizzleImage function.

        Example usage:
        \code
        OSL_IMAGE *img = oslCreateImage(64, 64, OSL_IN_RAM, OSL_PF_5551);
        // Assuming img is properly initialized and contains image data
        int result = oslWriteImageFile(img, "output_image.png", 0);
        if (result == 0) {
                // Image was written successfully
        } else {
                // Error occurred
        }
        \endcode
 */
extern int oslWriteImageFile(OSL_IMAGE *img, const char *filename, int flags);

#ifdef OSL_IMAGE_WRITER_PNG
/** Writes an image to a PNG file. Same remarks as oslWriteImageFile apply.
        @param img
                The image you want to write.
        @param filename
                The name of the file that you want to write to.
        @param flags
                Either 0 or OSL_WRI_ALPHA. If OSL_WRI_ALPHA is specified, the alpha will be written to the PNG image file, making it semi-transparent. Else, alpha is ignored and always set to opaque.

        You can pass for example OSL_SECONDARY_BUFFER (the buffer which is currently displayed on the screen) to take a screenshot.
   \code
   oslWriteImageFilePNG(OSL_SECONDARY_BUFFER, "screenshot.png", 0);
   \endcode

   \b Note: The same considerations as #oslWriteImageFile apply. */
extern int oslWriteImageFilePNG(OSL_IMAGE *img, const char *filename, int flags);
#endif

/** Resets the properties of an image, including its position, tile settings, angle, rotation center, and stretching.

        @param img
                The image whose properties will be reset.

        This function resets the following properties of the image:
        - Position: The image's current position will be reset to its default.
        - Image Tile: Any modifications to the image tile (e.g., using `oslSetImageTile`) will be cleared.
        - Angle: Any rotation applied to the image will be reset.
        - Rotation Center: The center used for rotation will be reset.
        - Stretching: Any stretching applied to the image will be reset.

        Example usage:
        \code
        OSL_IMAGE *img = oslCreateImage(64, 64, OSL_IN_RAM, OSL_PF_5551);
        // Modify image properties
        oslResetImageProperties(img);
        \endcode
 */
extern void oslResetImageProperties(OSL_IMAGE *img);

/** Draws srcImg to dstImg with scaling applied.

        @param dstImg
                The destination image where the scaled source image will be drawn.
        @param srcImg
                The source image to be scaled and drawn.
        @param newX
                The x-coordinate for the position of the scaled source image in the destination image.
        @param newY
                The y-coordinate for the position of the scaled source image in the destination image.
        @param newWidth
                The width of the scaled source image.
        @param newHeight
                The height of the scaled source image.

        This function scales the source image (srcImg) and draws it onto the destination image (dstImg) at the specified position and with the specified dimensions.

        Example usage:
        \code
        OSL_IMAGE *srcImg = oslCreateImage(64, 64, OSL_IN_RAM, OSL_PF_5551);
        OSL_IMAGE *dstImg = oslCreateImage(128, 128, OSL_IN_RAM, OSL_PF_5551);
        // Scale and draw srcImg onto dstImg
        oslScaleImage(dstImg, srcImg, 10, 10, 100, 100);
        \endcode
 */
extern void oslScaleImage(OSL_IMAGE *dstImg, OSL_IMAGE *srcImg, int newX, int newY, int newWidth, int newHeight);

/** Creates a scaled copy of an image.

        @param img
                The image to be scaled and copied.
        @param newLocation
                The location where the new scaled image will be stored (e.g., in RAM or VRAM).
        @param newWidth
                The width of the scaled image.
        @param newHeight
                The height of the scaled image.
        @param newPixelFormat
                The pixel format of the new scaled image.

        @return
                A pointer to the new scaled image. If the scaling operation fails, the function may return NULL.

        This function creates a new image that is a scaled version of the original image (img). The new image will have the specified dimensions and pixel format.

        Example usage:
        \code
        OSL_IMAGE *originalImg = oslCreateImage(64, 64, OSL_IN_RAM, OSL_PF_5551);
        OSL_IMAGE *scaledImg = oslScaleImageCreate(originalImg, OSL_IN_RAM, 128, 128, OSL_PF_5551);
        \endcode
 */
extern OSL_IMAGE *oslScaleImageCreate(OSL_IMAGE *img, short newLocation, int newWidth, int newHeight, short newPixelFormat);

/** Enum for specifying write flags when saving an image to a file.

        The flags determine how the image data is processed when written to a file.
        For example, flags can control whether the alpha channel is included in the output file.

        @see oslWriteImageFilePNG for details on specific write flags.

        @note Use these flags to customize the behavior of image writing functions.

        \code
        int flags = OSL_WRI_ALPHA; // Include alpha channel when writing the image
        oslWriteImageFile(myImage, "output.png", flags);
        \endcode
 */
enum OSL_WRITE_FLAGS
{
	OSL_WRI_ALPHA = 1         ///< Writes the alpha channel of the image. If set, the alpha channel data will be included in the output file.
};

/** @} */         // end of image_manip

/** @defgroup image_sprites Simili sprite system
 *
 * A basic system allowing you to cut your images just as if they were sprites. Remember that the maximum size of an image is 512x512!
 * @{
 */

/** Defines the size of a frame in an image, preparing it for use as a sprite sheet.

        A sprite sheet is an image where multiple sprites (small images) are arranged in a grid. Each sprite in the sheet has the same dimensions, and the grid is typically filled from left to right and top to bottom.

        @param img
                Pointer to the image to be used as a sprite sheet.
        @param width
                The width of each sprite frame in pixels.
        @param height
                The height of each sprite frame in pixels.

        For example, if you have a 64x64 image containing 32x32 sprites, the image will be divided into four sprites as follows:

        \code
        sprite 1 | sprite 2
        ---------|---------
        sprite 3 | sprite 4

        Coordinates:
        0, 0     | 32, 0
        ---------|---------
        0, 32    | 32, 32
        \endcode

        \b Important: The maximum size of an image is 512x512! Ensure that your sprite sheet dimensions do not exceed this limit.

        \see oslCreateImage for considerations when creating images.
 */
static inline void oslSetImageFrameSize(OSL_IMAGE *img, u16 width, u16 height)
{
	img->frameSizeX = width;
	img->frameSizeY = height;
}

/** Sets the current frame of an image for sprite-based rendering.

        This function prepares the image to display a specific frame from a sprite sheet. A sprite sheet is an image where multiple sprites (small images) are arranged in a grid. This function is used to select the sprite frame that you want to draw.

        @param img
                Pointer to the image to set the frame for.
        @param frame
                The index of the frame to be displayed. Frames are indexed starting from 0.

        \b Example:
        \code
        // Draws a specific frame of an image
        void DrawImageFrame(OSL_IMAGE *img, int frame) {
                oslSetImageFrame(img, frame);
                oslDrawImage(img);
        }

        // Alternative method using a macro
 #define DrawImageFrame(img, frame) ({ oslSetImageFrame(img, frame); oslDrawImage(img); })

        // Inline function for specific frame drawing
        extern inline void DrawImageFrame(OSL_IMAGE *img, int frame) {
                oslSetImageFrame(img, frame);
                oslDrawImage(img);
        }

        // Draws a specific frame of an image at a specified location
        void DrawImageFrameXY(OSL_IMAGE *img, int x, int y, int frame) {
                oslSetImageFrame(img, frame);
                oslDrawImageXY(img, x, y);
        }

        // Load an image with specified frame size
        OSL_IMAGE *LoadSpriteFilePNG(char *filename, int location, int pixelFormat, int frameWidth, int frameHeight) {
                OSL_IMAGE *img = oslLoadImageFile(filename, location, pixelFormat);
                if (img)
                        oslSetImageFrameSize(img, frameWidth, frameHeight);
                return img;
        }

        void main() {
                // Initialization
                [...]

                // 30x40 pixels per sprite
                OSL_IMAGE *myImage;
                myImage = LoadSpriteFilePNG("test.png", OSL_IN_RAM, OSL_PF_5551, 30, 40);
                [...]

                // Draw the frame n°2 (that is, the third sprite) at location (0, 0)
                DrawImageFrameXY(myImage, 0, 0, 2);
                [...]
        }
        \endcode
 */
extern void oslSetImageFrame(OSL_IMAGE *img, int frame);

/** @} */         // end of image_sprites

/** @defgroup image_palette Image palettes
 *
 * Palette for 4 and 8-bit images.
 * @{
 */

/** Creates a new (empty) palette with the specified pixel format.

        This function creates a new palette that can be used with 4-bit or 8-bit images. The palette is allocated in the specified location and uses the provided pixel format. For most cases, the pixel format should be `OSL_PF_8888` as palettes are generally small in size.

        @param size
                The number of colors in the palette. For example, a 4-bit image palette will have 16 colors (2^4), and an 8-bit image palette will have 256 colors (2^8).
        @param location
                The memory location where the palette will be stored. Use `OSL_IN_RAM` to allocate the palette in RAM.
        @param pixelFormat
                The pixel format of each palette entry. Typically, this should be `OSL_PF_8888` to match the common palette size.

        \b Note:
        - After creating and filling the palette, it is essential to uncache it to ensure that changes are applied correctly.
        - If you need to uncache both the image and its palette, you can use `oslUncacheImage(img)` which will also uncache the palette.

        \b Example:
        \code
        OSL_IMAGE *img = oslCreateImage(32, 32, OSL_IN_RAM, OSL_PF_4BIT);
        // Determine the palette size based on the image's pixel format.
        int palSize = 1 << osl_paletteSizes[img->pixelFormat];
        // Create the palette with the required size.
        img->palette = oslCreatePaletteEx(palSize, OSL_IN_RAM, OSL_PF_8888);
        // Get a pointer to the palette data.
        u32 *paletteData = (u32*)img->palette->data;
        // Set all entries to bright opaque red.
        for (int i = 0; i < img->palette->nElements; i++)
                paletteData[i] = RGBA(255, 0, 0, 255);
        // Uncache the palette after finishing.
        oslUncachePalette(img->palette);
        // Alternatively, uncache the image which also uncaches the palette.
        // oslUncacheImage(img);
        \endcode
 */
extern OSL_PALETTE *oslCreatePaletteEx(int size, int location, short pixelFormat);

/** Creates a new palette with the specified pixel format.

        This is a simpler function for creating a new palette. It allocates the palette in RAM and uses the provided pixel format. This function is a wrapper around `oslCreatePaletteEx` with the location parameter set to `OSL_IN_RAM`.

        @param size
                The number of colors in the palette (e.g., 16 for 4-bit images, 256 for 8-bit images).
        @param pixelFormat
                The pixel format of each palette entry, typically `OSL_PF_8888`.

        @return
                A pointer to the newly created palette.

        \b Example:
        \code
        OSL_PALETTE *palette = oslCreatePalette(16, OSL_PF_8888);
        \endcode
 */
static inline OSL_PALETTE *oslCreatePalette(int size, short pixelFormat)
{
	return oslCreatePaletteEx(size, OSL_IN_RAM, pixelFormat);
}

/** Creates a palette from existing data.

        This function creates a palette using data that already exists. The data is not copied; it is used directly. You need to specify the size of the palette and the pixel format of the data.

        @param data
                Pointer to the existing palette data.
        @param size
                The number of colors in the palette.
        @param pixelFormat
                The pixel format of each palette entry.

        @return
                A pointer to the newly created palette.

        \b Note:
        - The data is not copied; it is used as is. Ensure that the data remains valid for the lifetime of the palette.
        - Only the `OSL_PALETTE` structure is freed when the palette is deleted; the data is not affected.

        \b Example:
        \code
        void *existingData = existing palette data;
        OSL_PALETTE *palette = oslCreatePaletteFrom(existingData, 256, OSL_PF_8888);
        \endcode
 */
extern OSL_PALETTE *oslCreatePaletteFrom(void *data, int size, short pixelFormat);

/** Deletes an existing palette.

        This function frees the memory used by an `OSL_PALETTE` structure. If the palette was created using `oslCreatePaletteFrom`, the data used to create the palette is not freed.

        @param p
                Pointer to the palette to be deleted.

        \b Example:
        \code
        oslDeletePalette(palette);
        \endcode
 */
extern void oslDeletePalette(OSL_PALETTE *p);

/** Returns a color entry from a palette.

        This function retrieves a color from the palette at a specified index. The color value is in the same pixel format as the palette.

        @param p
                Pointer to the palette.
        @param index
                The index of the color entry to retrieve.

        @return
                The color value at the specified index.

        \b Example:
        \code
        int color = oslGetPaletteColor(palette, 0);
        \endcode
 */
extern int oslGetPaletteColor(OSL_PALETTE *p, int index);

/** Uncaches a palette.

        After accessing palette data in a cached way (e.g., through `pal->data`), you must uncache the palette to ensure changes are applied correctly.

        @param pal
                Pointer to the palette to uncache.

        \b Example:
        \code
        oslUncachePalette(palette);
        \endcode
 */
extern void oslUncachePalette(OSL_PALETTE *pal);

/** @} */         // end of image_palette

/** @defgroup image_pixel Pixel access
 *
 * Low-level access to images.
 * @{
 */

/** Flushes the image data from the cache.

        This function is necessary to call after modifying an image's data directly (e.g., manipulating `img->data` manually). It ensures that the image data is properly written back and invalidated in the cache.

        @param img
                Pointer to the image whose data needs to be flushed from the cache.

        \b Note:
        - This routine only flushes the image data and does not affect the associated palette data. To flush the palette data as well, use #oslUncacheImage.

        \code
        OSL_IMAGE *img = your image;
        // Modify the image data manually
        oslUncacheImageData(img); // Ensure changes are flushed
        \endcode
 */
static inline void oslUncacheImageData(OSL_IMAGE *img)
{
	if (img != NULL)
		sceKernelDcacheWritebackInvalidateRange(img->data, img->totalSize);
}

/** Uncaches an entire image, including its associated palette.

        This function not only flushes the image data from the cache but also handles the associated palette data, ensuring that both are properly uncached.

        @param img
                Pointer to the image to be uncached.

        \b Note:
        - This routine is a comprehensive solution for ensuring that all parts of the image (data and palette) are properly managed in memory.

        \code
        OSL_IMAGE *img = your image;
        oslUncacheImage(img); // Flushes image data and associated palette
        \endcode
 */
extern void oslUncacheImage(OSL_IMAGE *img);

/** High-level routine to read a single pixel from an image.

        This function retrieves the value of a pixel at a specified position from an image. Note that caching is enabled, so after reading and/or writing pixels, you should uncache the image. The recommended approach is to use #oslLockImage and #oslUnlockImage for proper management. For an example of usage, see #oslGetImagePixel.

        @param img
                Pointer to the image from which the pixel is read.
        @param x
                The x-coordinate of the pixel to be read.
        @param y
                The y-coordinate of the pixel to be read.

        @return
                The value of the pixel at the specified position. The returned value depends on the pixel format of the image. It could be a palette entry number for paletted images or a color value in the image's format. You might need to convert this value depending on your needs.

        \b Example:
        \code
        // Get a 32-bit (OSL_PF_8888) color for the pixel at x, y, regardless of the image's pixel format.
        int pixel = oslConvertColorEx(yourImage->palette, OSL_PF_8888, yourImage->pixelFormat, oslGetImagePixel(yourImage, x, y));
        \endcode

        \b Note:
        - This function works for both regular and swizzled images.
        - It may be slower compared to raw access methods; consider using raw access if performance is critical.
 */
extern int oslGetImagePixel(OSL_IMAGE *img, unsigned int x, unsigned int y);

/** Sets a pixel value on the image.

        This function sets the value of a pixel at a specified position in the image. The value should be in the image's destination format, which could be a palette entry or a color value. If needed, perform conversions to match the image's format.

        @param img
                Pointer to the image where the pixel will be set.
        @param x
                The x-coordinate of the pixel to be set.
        @param y
                The y-coordinate of the pixel to be set.
        @param pixelValue
                The value to set at the specified pixel position. This value should be in the format used by the image (e.g., a color value or a palette entry).

        \b Example:
        \code
        // Before performing low-level operations on an image, lock it to handle cache operations.
        oslLockImage(image);
        {
                // Iterate over all pixels in the image
                for (j = 0; j < image->sizeY; j++) {
                        for (i = 0; i < image->sizeX; i++) {
                                // Get the current pixel value (assuming a 8888 format here)
                                u32 pixel = oslGetImagePixel(image, i, j);
                                // Mask out the green and blue components
                                oslSetImagePixel(image, i, j, pixel & 0xff0000ff);
                        }
                }
        }
        oslUnlockImage(image);
        \endcode

        \b Note:
        - Similar caching considerations apply as with #oslGetImagePixel. Make sure to manage the image cache properly by locking and unlocking the image.
 */
extern void oslSetImagePixel(OSL_IMAGE *img, unsigned int x, unsigned int y, int pixelValue);

/** Returns the address of the beginning (left border) of a specific line in an image.

        This macro provides access to the raw data of a specific line in the image. When using this macro, make sure to respect the pixel format of the image. For images with 16-bit pixel formats (e.g., 4444, 5551, or 5650), use a `u16` pointer and the appropriate RGBxx macros for accessing pixel values.

        @param img
                Pointer to the image from which the line address is obtained.
        @param y
                The line number (0 is the first line, 1 is the second, and so on).

        @return
                A pointer to the beginning of the specified line in the image's raw data.

        \b Example:
        \code
        // Create a 15-bit image (16-bit pixel width, with 1 alpha bit included)
        OSL_IMAGE *img = oslCreateImage(32, 32, OSL_IN_RAM, OSL_PF_5551);

        // Get a pointer to the 4th line (0-based index)
        u16 *data = (u16*)oslGetImageLine(img, 3);

        // Loop through each pixel in the line
        for (int i = 0; i < img->sizeX; i++) {
                // Set this pixel to bright red
 * data = RGBA15(255, 0, 0, 255);
                // Move to the next pixel
                data++;
        }
        \endcode

        \b Note:
        - Ensure you use the correct pixel format and data type when accessing raw image data.
 */
	#define oslGetImageLine(img, y) ((char *)((img)->data) + (y) * (((img)->realSizeX * osl_pixelWidth[(img)->pixelFormat]) >> 3))

/** Gets the address of a single pixel in an image.

        This macro provides a pointer to the address of a specific pixel in the image. It is designed to work with pixel formats ranging from 8 to 32 bits per pixel. Note that this macro does not handle 4-bit images correctly, as a byte in 4-bit images represents two pixels. For 4-bit images, this macro returns a pointer to the corresponding byte, and additional bit manipulation is required to access individual pixels.

        @param img
                Pointer to the image from which to retrieve the pixel address.
        @param x
                The x-coordinate of the pixel.
        @param y
                The y-coordinate of the pixel.

        @return
                A pointer to the address of the specified pixel in the image's raw data.

        \b Example:
        \code
        void setPixel(OSL_IMAGE *img, int x, int y, int value) {
                u8 *data = (u8 *)oslGetImagePixelAddr(img, x, y);

                // Ensure the pixel value does not exceed the bit depth
                value &= 0xf;

                // Handle 4-bit images by checking if x is even or odd
                if (x & 1) {
                        // Mask out the last 4 bits
 * data &= 0x0f;
                        // Write to the last 4 bits
 * data |= value << 4;
                } else {
                        // Mask out the first 4 bits
 * data &= 0xf0;
                        // Write to the first 4 bits
 * data |= value;
                }

                // Alternative method with parity checking
 * data &= ~(15 << ((x & 1) << 2));
 * data |= value << ((x & 1) << 2);
        }
        \endcode

        \b Note:
        - Ensure that you handle the pixel data according to its format and bit depth. For 4-bit images, additional bit manipulation is required.
 */
	#define oslGetImagePixelAddr(img, x, y) ((char *)((img)->data) + ((((y) * (img)->realSizeX + (x)) * osl_pixelWidth[(img)->pixelFormat]) >> 3))

/** For backward compatibility. */
	#define oslGetImagePixelAdr oslGetImagePixelAddr

/**
 * Returns the address of a pixel, even if the image is swizzled.
 * This implementation is slower but works in every case, including swizzled images.
 *
 * @param img
 *     Pointer to the OSL_IMAGE structure representing the image.
 * @param x
 *     The x-coordinate of the pixel.
 * @param y
 *     The y-coordinate of the pixel.
 *
 * @return
 *     A pointer to the address of the pixel in the image's raw data. The exact type of this pointer
 *     depends on the image's pixel format and needs to be cast appropriately.
 *
 * \note
 *     This function is designed to handle swizzled images. It might be slower than direct access methods,
 *     but it ensures correct behavior for swizzled images by accounting for the swizzling pattern.
 *
 * \code
 * // Example usage:
 * OSL_IMAGE *img = Load or create your image;
 * void *pixelAddr = oslGetSwizzledPixelAddr(img, x, y);
 *
 * // Depending on the pixel format, cast the pointer to the appropriate type
 * u32 *pixelData = (u32*)pixelAddr;
 *
 * // Now you can read or modify the pixel data
 * *pixelData = New pixel value;
 * \endcode
 */
void *oslGetSwizzledPixelAddr(OSL_IMAGE *img, unsigned int x, unsigned int y);

/**
 * @brief Converts a color from one pixel format to another.
 *
 * This function converts a color value from the source pixel format (`pfSrc`) to the destination pixel format (`pfDst`).
 * It supports various pixel formats, including `OSL_PF_8888`, `OSL_PF_5650`, `OSL_PF_5551`, and `OSL_PF_4444`.
 *
 * @param pfDst The destination pixel format. This can be one of the `OSL_PF_*` values (e.g., `OSL_PF_8888`, `OSL_PF_5650`).
 * @param pfSrc The source pixel format. This can be one of the `OSL_PF_*` values (e.g., `OSL_PF_8888`, `OSL_PF_5650`).
 * @param color The color value to be converted, expressed in the source pixel format.
 *
 * @return The color converted to the destination pixel format.
 */
extern int oslConvertColor(int pfDst, int pfSrc, int color);

/**
 * @brief Converts a color between different pixel formats, optionally using a palette for indexed formats.
 *
 * This function converts a color from one pixel format (`pfSrc`) to another (`pfDst`). If either the source or destination
 * format is a paletted format (`OSL_PF_8BIT` or `OSL_PF_4BIT`), the provided palette (`p`) will be used for the conversion.
 *
 * @note If the source and destination formats are the same, the original color is returned without modification.
 *
 * @param p     Pointer to an `OSL_PALETTE` structure. This is used when the source or destination format is paletted.
 *              If `p` is `NULL`, the conversion will be done without using a palette.
 * @param pfDst The destination pixel format. This can be any of the `OSL_PF_*` values (e.g., `OSL_PF_8888`, `OSL_PF_5650`).
 * @param pfSrc The source pixel format. This can be any of the `OSL_PF_*` values (e.g., `OSL_PF_8888`, `OSL_PF_5650`).
 * @param color The color value to be converted.
 *
 * @return The color converted to the destination pixel format.
 */
extern int oslConvertColorEx(OSL_PALETTE *p, int pfDst, int pfSrc, int color);

/**
 * Locks an image to ensure safe access and modifications.
 * This function automatically handles drawing to the drawbuffer, uncaching the image when finished,
 * and flushing the texture cache to avoid rendering issues when the image is modified during drawing.
 *
 * @param img
 *     Pointer to the OSL_IMAGE structure representing the image to lock.
 *
 * \note
 *     While locking an image is not strictly necessary, it is highly recommended as it ensures
 *     proper management of image access and rendering, preventing potential errors caused by
 *     concurrent modifications and drawing operations.
 *
 * \code
 * // Lock the image to safely modify it
 * oslLockImage(myImage);
 * {
 *     // Perform software access and modification to the image
 *     u16 *ptr = (u16*)myImage->data;
 *     *ptr = 0; // Example modification
 * }
 * // Unlock the image after modifications are complete
 * oslUnlockImage(myImage);
 *
 * // Now you can safely draw the modified image
 * oslDrawImage(myImage);
 * \endcode
 */
extern void oslLockImage(OSL_IMAGE *img);

/**
 * Unlocks an image after modifications have been completed.
 * This function should be called after using `oslLockImage` to release the lock and finalize any
 * pending operations.
 *
 * @param img
 *     Pointer to the OSL_IMAGE structure representing the image to unlock.
 *
 * \note
 *     Always ensure that `oslUnlockImage` is called after `oslLockImage` to properly finalize
 *     modifications and ensure the image is ready for rendering or further operations.
 */
extern void oslUnlockImage(OSL_IMAGE *img);

/**
 * List of pixel widths (in bits) for each pixel format.
 *
 * This array provides the number of bits used for each pixel in various pixel formats.
 * The index of the array corresponds to the pixel format identifier.
 *
 * \note
 *     For example, if `osl_pixelWidth[OSL_PF_8888]` is 32, it indicates that the `OSL_PF_8888` pixel
 *     format uses 32 bits per pixel.
 */
extern const int osl_pixelWidth[];

/**
 * List of palette sizes (in powers of two) for each pixel format.
 *
 * This array provides the size of the palette for each pixel format that uses palette-based
 * color representation. For non-paletted formats, the value is zero. For paletted formats,
 * the size of the palette can be calculated as `1 << osl_paletteSizes[pixelFormat]`, where
 * `pixelFormat` is the index into the array.
 *
 * \note
 *     For example, if `osl_paletteSizes[OSL_PF_4BIT]` is 4, it indicates that the `OSL_PF_4BIT`
 *     pixel format uses a palette size of `1 << 4`, or 16 colors.
 */
extern const u8 osl_paletteSizes[];

/** @} */         // end of image_pixel

/** @defgroup image_drawbuf Drawbuffers
 *
 * Drawbuffers is an advanced and powerful capability of OSLib images. It allows you to draw directly on an image exactly as if you were drawing on the screen!
 * See #oslSetDrawBuffer for a basic code sample.
 * @{
 */

/** Returns the current drawbuffer as an OSL_IMAGE. You can save it to restore it later.
 *
 * @return
 *     A pointer to the current drawbuffer image.
 */
	#define oslGetDrawBuffer() osl_curBuf

/** An image representing the primary buffer image, which is the buffer to which you are currently writing.
 *
 * \note
 *     This buffer is managed by `oslSwapBuffers`. In single buffering mode, `OSL_DEFAULT_BUFFER` is identical to `OSL_SECONDARY_BUFFER`.
 */
	#define OSL_DEFAULT_BUFFER (&osl_defaultBufferImage)

/** An image representing the secondary buffer image, which is the buffer currently displayed to the user.
 *
 * \note
 *     In single buffering mode, `OSL_DEFAULT_BUFFER` is identical to `OSL_SECONDARY_BUFFER`. This buffer is updated and managed by `oslSwapBuffers`.
 */
	#define OSL_SECONDARY_BUFFER (&osl_secondaryBufferImage)

/** Returns the width of the current drawbuffer.
 *
 * @return
 *     The width of the current drawbuffer in pixels. For the default drawbuffer (i.e., the screen), it will be 480 pixels, which is the resolution of the PSP LCD.
 */
	#define OSL_SCREEN_WIDTH (osl_curBuf->sizeX)

/** Returns the height of the current drawbuffer.
 *
 * @return
 *     The height of the current drawbuffer in pixels. For the default drawbuffer (i.e., the screen), it will be 272 pixels, which is the resolution of the PSP LCD.
 */
	#define OSL_SCREEN_HEIGHT (osl_curBuf->sizeY)

/** Define an image as the current drawbuffer. Use this function to specify the image to which drawing operations will be directed. Remember to restore the original drawbuffer after you have finished drawing.

        \code
        OSL_IMAGE *drawBuf = oslCreateImage(320, 182, OSL_IN_VRAM, OSL_PF_5650);
        // Clear the image to black
        oslClearImage(drawBuf, RGB16(0, 0, 0));
        // Set that image as the drawbuffer
        oslSetDrawBuffer(drawBuf);
        // Draw a red filled rectangle on the image
        oslDrawFillRect(0, 0, 100, 100, RGB(255, 0, 0));
        // Restore the default drawbuffer
        oslSetDrawBuffer(OSL_DEFAULT_BUFFER);
        // Draw that image somewhere on the screen
        oslDrawImageXY(drawBuf, 80, 45);
        \endcode

        \b Important:
        - The image pixel format must be non-paletted. Supported formats are 4444, 5551, 5650, and 8888. Note that 8888 mode (32-bit) is slower and results in a larger image size.
        - The image must be in video memory (OSL_IN_VRAM). The GPU can only write to video memory and can only read from regular RAM.
        - A common issue is that the image may not be displayed if its alpha is null. To avoid this, either clear the image after creation or temporarily disable alpha blending by calling `oslSetAlpha(OSL_FX_NONE, 0)`.

        @param img
                The image to set as the current drawbuffer.

        @return
                None.
 */
extern void oslSetDrawBuffer(OSL_IMAGE *img);

/**
 * @brief Controls writing to the alpha channel of the draw buffer.
 *
 * This function is useful when rendering to an image rather than the standard draw buffer. By default, alpha values are not modified in the draw buffer.
 * This function allows you to specify how alpha values should be written based on the given action. It also provides the ability to enable or disable
 * stencil testing, which can control the regions of the buffer where alpha values are written.
 *
 * @param action Specifies the action to take:
 *               - `OSL_FXAW_SET`: Sets the alpha channel to `value1`. Every pixel written to the draw buffer will set its alpha value to `value1`.
 *                                  Even fully transparent pixels will have their alpha values set unless additional alpha testing is enabled.
 *               - `OSL_FXAW_NONE`: Disables stencil testing, leaving the alpha channel unchanged.
 * @param value1 If `action` is `OSL_FXAW_SET`, this specifies the alpha value to be written, ranging from 0 (transparent) to 255 (opaque).
 * @param value2 Currently not used; should be set to 0.
 *
 * **Example:**
 * \code
 * // Clear the image to fully transparent (alpha = 0)
 * oslClearImage(buffer, RGBA(0, 0, 0, 0));
 * // Set the cleared image as the draw buffer
 * oslSetDrawBuffer(buffer);
 * // Enable alpha writing with full opacity (alpha = 255)
 * oslSetAlphaWrite(OSL_FXAW_SET, 255, 0);
 * // Only draw pixels with an alpha value greater than 0, preventing fully transparent pixels from being written
 * oslSetAlphaTest(OSL_FXAT_GREATER, 0);
 * // Continue rendering...
 * \endcode
 */
extern void oslSetAlphaWrite(int action, int value1, int value2);

/**
 * Sets alpha testing parameters.
 *
 * Configures the alpha test, which determines whether a pixel should be drawn based on its alpha value. If the test passes, the pixel is written to the screen; otherwise, it is ignored.
 *
 * @param condition
 *        The condition for the test to pass. Can be one of the following:
 *        - OSL_FXAT_NEVER: Never passes.
 *        - OSL_FXAT_ALWAYS: Always passes.
 *        - OSL_FXAT_EQUAL: Alpha is equal to the value.
 *        - OSL_FXAT_NOTEQUAL: Alpha is not equal to the value.
 *        - OSL_FXAT_LESS: Alpha is less than the value.
 *        - OSL_FXAT_LEQUAL: Alpha is less than or equal to the value.
 *        - OSL_FXAT_GREATER: Alpha is greater than the value.
 *        - OSL_FXAT_GEQUAL: Alpha is greater than or equal to the value.
 *
 * @param value
 *        The reference value for comparison.
 *
 * @note
 *       Pixels from an OSL_PF_5650 image do not initially contain alpha. The alpha value is automatically set to opaque (alpha = 255).
 */
extern void oslSetAlphaTest(int condition, int value);

/**
 * Disables alpha testing.
 *
 * Alpha testing is a method of controlling the visibility of pixels based on their alpha values.
 */
extern void oslDisableAlphaTest();

/** Holds whether alpha testing is currently enabled. */
extern int osl_alphaTestEnabled;

/** Available effects for #oslSetAlphaWrite. */
enum OSL_FX_ALPHAWRITE
{
	/** Does not write alpha. */
	OSL_FXAW_NONE,

	/** Sets alpha to value1 if the alpha channel of the pixel is not equal to value2. */
	OSL_FXAW_SET,
};

/** Alpha test comparison operators. Used with #oslSetAlphaTest to define the conditions for alpha testing. */
enum OSL_FX_ALPHATEST
{
	/** Never passes the alpha test. */
	OSL_FXAT_NEVER = GU_NEVER,

	/** Always passes the alpha test. */
	OSL_FXAT_ALWAYS = GU_ALWAYS,

	/** Passes the alpha test if the alpha value is equal to the reference value. */
	OSL_FXAT_EQUAL = GU_EQUAL,

	/** Passes the alpha test if the alpha value is not equal to the reference value. */
	OSL_FXAT_NOTEQUAL = GU_NOTEQUAL,

	/** Passes the alpha test if the alpha value is less than the reference value. */
	OSL_FXAT_LESS = GU_LESS,

	/** Passes the alpha test if the alpha value is less than or equal to the reference value. */
	OSL_FXAT_LEQUAL = GU_LEQUAL,

	/** Passes the alpha test if the alpha value is greater than the reference value. */
	OSL_FXAT_GREATER = GU_GREATER,

	/** Passes the alpha test if the alpha value is greater than or equal to the reference value. */
	OSL_FXAT_GEQUAL = GU_GEQUAL
};

/** @} */         // end of image_drawbuf

/** @defgroup image_adv Advanced
 *
 * Advanced routines for working with images.
 * @{
 */

/** Returns a nonzero value if the image is a copy of another image, or 0 if it is an original.
        A copy is an image that was created as a clone of another image, rather than being directly created.
 */
	#define oslImageIsCopy(img) (img->flags & OSL_IMAGE_COPY)

/** Returns a nonzero value if the image is swizzled, or 0 if it is not.
        Swizzling refers to a method of reorganizing image data to improve memory access patterns or GPU performance.
 */
	#define oslImageIsSwizzled(img) (img->flags & OSL_IMAGE_SWIZZLED)

/** Returns a nonzero value if automatic striping is enabled for the image, or 0 if it is not.
        Automatic striping can be used to optimize the handling of large images by breaking them into smaller, more manageable strips.
 */
	#define oslImageGetAutoStrip(img) (img->flags & OSL_IMAGE_AUTOSTRIP)

/** Sets or clears the copy flag for an image. This function is for internal use only and controls whether the image is marked as a copy of another image.
        @param img
                Pointer to the `OSL_IMAGE` structure whose copy flag is to be set or cleared.
        @param enabled
                `true` to set the image as a copy, `false` to clear the copy flag.
 */
static inline void oslImageIsCopySet(OSL_IMAGE *img, bool enabled)
{
	if (enabled)
		img->flags |= OSL_IMAGE_COPY;
	else
		img->flags &= ~OSL_IMAGE_COPY;
}

/** Sets or clears the swizzled flag for an image. This function is for internal use only and determines whether the image is marked as swizzled.
        @param img
                Pointer to the `OSL_IMAGE` structure whose swizzled flag is to be set or cleared.
        @param enabled
                `true` to set the image as swizzled, `false` to clear the swizzled flag.
 */
static inline void oslImageIsSwizzledSet(OSL_IMAGE *img, bool enabled)
{
	if (enabled)
		img->flags |= OSL_IMAGE_SWIZZLED;
	else
		img->flags &= ~OSL_IMAGE_SWIZZLED;
}

/** Sets or clears the automatic striping flag for an image. This function is for internal use only and manages whether the image should be automatically divided into stripes for faster blitting.
        @param img
                Pointer to the `OSL_IMAGE` structure whose automatic striping flag is to be set or cleared.
        @param enabled
                `true` to enable automatic striping, `false` to disable it.
 */
static inline void oslImageSetAutoStrip(OSL_IMAGE *img, bool enabled)
{
	if (enabled)
		img->flags |= OSL_IMAGE_AUTOSTRIP;
	else
		img->flags &= ~OSL_IMAGE_AUTOSTRIP;
}

/** @} */         // end of image_adv

/** @} */         // end of image

/** \ingroup drawing
        @defgroup drawing_lowlev Low level drawing

        Low-level drawing routines, closer to the hardware and thus faster.
        @{
 */

/** @defgroup drawing_lowlev_general Basic documentation

        This section provides basic guidance on drawing textured and untextured objects, assuming some familiarity with GU (Graphics Utility).

        Here's a basic example of drawing a textured object and an untextured object:

        \code
        void drawSomethingTextured(OSL_IMAGE *img) {
                // Define the vertices for your shape. Sprites use 2 vertices, triangles use 3, and quads use 4.
                VERTEX_TYPE *vertices;

                // Set the provided image as the current texture
                oslSetTexture(img);

                // Allocate memory for the vertices
                vertices = (VERTEX_TYPE*)sceGuGetMemory(NUMBER_OF_VERTICES * sizeof(vertices[0]));

                // Initialize the vertex data
                vertices[0].something = ...;
                // [Additional vertex data setup]

                // Draw the array of vertices using GU (Graphics Utility)
                sceGuDrawArray(PRIMITIVETYPE, VERTEX_DESCRIPTION | GU_TRANSFORM_2D, NUMBER_OF_VERTICES, 0, vertices);
        }

        void drawSomethingUntextured(OSL_IMAGE *img) {
                // Define the vertices for your shape. Sprites use 2 vertices, triangles use 3, and quads use 4.
                VERTEX_TYPE *vertices;

                // Disable texturing
                oslDisableTexturing();

                // Allocate memory for the vertices
                vertices = (VERTEX_TYPE*)sceGuGetMemory(NUMBER_OF_VERTICES * sizeof(vertices[0]));

                // Initialize the vertex data
                vertices[0].something = ...;
                // [Additional vertex data setup]

                // Draw the array of vertices using GU
                sceGuDrawArray(PRIMITIVETYPE, VERTEX_DESCRIPTION | GU_TRANSFORM_2D, NUMBER_OF_VERTICES, 0, vertices);
        }
        \endcode

        As you can see, drawing with GU is straightforward if you are familiar with it. For those new to GU, it is recommended to review tutorials and examples available on ps2dev.org for a deeper understanding.
 */

	#ifdef PSP
/** Disables texturing, making the image opaque and drawn using vertex colors. */
	#define oslEnableTexturing() ({ if (!osl_textureEnabled)  sceGuEnable(GU_TEXTURE_2D), osl_textureEnabled = 1; })

/** Enables texturing. This function should not be called directly; it is managed by oslSetTexture. */
	#define oslDisableTexturing() ({ if (osl_textureEnabled)  sceGuDisable(GU_TEXTURE_2D), osl_textureEnabled = 0; })

	#else
/** Disables texturing, making the image opaque and drawn using vertex colors. */
	#define oslEnableTexturing()                                    \
		{                                                           \
			if (!osl_textureEnabled)                                \
			sceGuEnable(GU_TEXTURE_2D), osl_textureEnabled = 1; \
		}

/** Enables texturing. This function should not be called directly; it is managed by oslSetTexture. */
	#define oslDisableTexturing()                                    \
		{                                                            \
			if (osl_textureEnabled)                                  \
			sceGuDisable(GU_TEXTURE_2D), osl_textureEnabled = 0; \
		}
	#endif

/**
 * Defines the maximum width of an image stripe for efficient drawing.
 * Images wider than this value should be drawn in multiple stripes.
 * For example, if the image width is 256 pixels, it should be drawn
 * in 4 stripes, each 64 pixels wide. This prevents performance issues
 * related to handling large images.
 */
	#define OSL_SLICE_SIZE 64

/**
 * This function is intended for internal use only by `oslDrawImageSimple`.
 * It is used to verify or handle strip blitting operations for images.
 * Generally, this function manages how images are processed when they
 * are too large to be handled in one go and need to be split into smaller
 * strips for efficient rendering.
 *
 * Do not use or modify this function directly. For details on its usage,
 * refer to the source code of `oslDrawImageSimple`.
 */
extern int oslVerifyStripBlit(OSL_IMAGE *img);

/** Holds the current texture wrap state for the U coordinate. */
extern int osl_currentTexWrapU;

/** Holds the current texture wrap state for the V coordinate. */
extern int osl_currentTexWrapV;

/**
 * Sets the behavior of textures when their bounds are exceeded.
 * This function determines how textures are wrapped or clamped when
 * they are drawn beyond their original size.
 *
 * For example, if you have a 128x128 image and you draw a region
 * larger than this (e.g., 200x200), the area outside the image's
 * bounds can be handled in different ways:
 * - **Clamp**: The same pixel will be repeated indefinitely.
 * - **Repeat**: The image texture will be tiled.
 *
 * @param u The wrapping mode for the U coordinate.
 * @param v The wrapping mode for the V coordinate.
 *
 * @param u
 *   - `OSL_TW_CLAMP`: Clamps the texture (repeats the edge pixel).
 *   - `OSL_TW_REPEAT`: Tiles the texture (repeats the image).
 *
 * @param v
 *   - `OSL_TW_CLAMP`: Clamps the texture in the V coordinate direction.
 *   - `OSL_TW_REPEAT`: Tiles the texture in the V coordinate direction.
 */
static inline void oslSetTextureWrap(int u, int v)
{
	sceGuTexWrap(u, v);                   // Set texture wrapping modes
	osl_currentTexWrapU = u;              // Update current U wrap mode
	osl_currentTexWrapV = v;              // Update current V wrap mode
}

/** Available values for #oslSetTextureWrap. */
enum OSL_TEXWRAP_MODES
{
	OSL_TW_REPEAT = GU_REPEAT,         ///< Repeats the texture. The texture tiles indefinitely.
	OSL_TW_CLAMP = GU_CLAMP,           ///< Clamps the texture. The last pixel of the texture is repeated indefinitely beyond its bounds.
};

/**
 * @brief Defines the current texture for low-level image drawing.
 *
 * This function sets the texture to be used for rendering operations. It specifies which image should be applied as a texture when drawing objects. The texture image must be in video memory (VRAM) and should be in a non-paletted format (e.g., 4444, 5551, 5650).
 *
 * @param img A pointer to the `OSL_IMAGE` structure that represents the image to be used as the texture. The image should be loaded into video memory for proper texture mapping.
 *
 * @note
 * - The image must be in video memory (OSL_IN_VRAM) for the texture to be applied correctly.
 * - Ensure that the image format is compatible with the texturing requirements of the GPU.
 * - Calling this function changes the current texture state, so you should call it before each drawing operation if you are using different textures.
 */
extern void oslSetTexture(OSL_IMAGE *img);

/**
 * @brief Defines the current texture and its starting coordinates for rendering.
 *
 * This function sets a texture and specifies the beginning coordinates within the texture. It is useful for handling large textures by breaking them into smaller 512x512 sections.
 *
 * @param img A pointer to the `OSL_IMAGE` structure representing the texture image. The image should be in video memory (VRAM) and should be in a non-paletted format.
 * @param x The x-coordinate (horizontal) of the starting point within the texture, which should be a multiple of 512.
 * @param y The y-coordinate (vertical) of the starting point within the texture, which should be a multiple of 512.
 *
 * The coordinates (x, y) will be mapped to (u, v) = (0, 0) when drawing the texture, allowing access to a 512x512 portion of the texture.
 */
extern void oslSetTexturePart(OSL_IMAGE *img, int x, int y);

/**
 * @brief Blends two colors together.
 *
 * This function combines two colors using a blending algorithm. It is used internally and is not recommended for direct use in application code.
 *
 * @param c1 The first color to blend.
 * @param c2 The second color to blend.
 *
 * @return The blended color result.
 */
OSL_COLOR oslBlendColors(OSL_COLOR c1, OSL_COLOR c2);

/**
 * @brief Applies alpha parameters to a color, tinting it.
 *
 * This function blends a given color with the current alpha coefficient. It is used to apply alpha blending effects to vertex colors.
 *
 * @param c The color to tint with the current alpha coefficient.
 *
 * @return The tinted color with alpha applied.
 */
static inline OSL_COLOR oslBlendColor(OSL_COLOR c)
{
	return oslBlendColors(c, osl_currentAlphaCoeff);
}

/**
 * @brief System display list for low-level drawing operations.
 *
 * This pointer holds the address of the system's display list used for rendering graphics. It is utilized internally by OSLib for managing drawing commands and operations.
 */
extern unsigned int *osl_list;

/**
 * @brief Indicates if texturing is currently enabled.
 *
 * This variable holds the state of the texturing feature. It is set to a non-zero value when texturing is enabled, and to zero when it is disabled. This state controls whether textures are applied to drawn objects.
 */
extern int osl_textureEnabled;

/**
 * @brief Current texture being used for drawing.
 *
 * This pointer holds the current texture that has been set for drawing operations. It is used internally to avoid redundant texture setting operations if the texture hasn't changed.
 */
extern void *osl_curTexture;

/**
 * @brief Current palette being used.
 *
 * This pointer holds the current palette that has been set for drawing operations. It is used internally to avoid redundant palette uploads if the palette hasn't changed.
 */
extern void *osl_curPalette;

/**
 * @brief Current draw buffer being used.
 *
 * This pointer holds the current draw buffer where drawing operations are being directed. It helps to manage the active drawing target and avoid unnecessary buffer switches.
 */
extern void *osl_curDrawBuf;

/**
 * @brief Current display buffer being used.
 *
 * This pointer holds the current display buffer that is being used for displaying the final output. It is part of the internal mechanism for managing what is shown on screen.
 */
extern void *osl_curDispBuf;

/**
 * @brief Default and secondary buffers.
 *
 * These represent the default and secondary buffers used in OSLib. The `osl_defaultBufferImage` is the primary buffer where the image is drawn, while `osl_secondaryBufferImage` is used for double buffering. `osl_curBuf` points to the currently active buffer.
 */
extern OSL_IMAGE osl_defaultBufferImage, osl_secondaryBufferImage, *osl_curBuf;

/**
 * @brief Flag indicating if drawing has started.
 *
 * This flag indicates whether drawing operations have been initiated. It helps in managing and coordinating drawing processes within OSLib.
 */
extern int osl_isDrawingStarted;

// Don't access this

/** @defgroup drawing_lowlev_vertex Vertex types
 *
 * Low level drawing routines, nearer to the hardware and thus faster.
 * @{
 */

/**
 * @brief Precise vertex with texture.
 *
 * This vertex type uses floating-point coordinates for both texture mapping and spatial positioning, offering high precision.
 */
typedef struct
{
	float u, v;          ///< Texture coordinates (floating point)
	float x, y, z;          ///< Spatial coordinates (floating point)
} OSL_PRECISE_VERTEX;

/**
 * @brief Fast vertex with texture.
 *
 * This vertex type uses 16-bit coordinates for both texture mapping and spatial positioning, providing faster rendering at the cost of reduced precision.
 */
typedef struct
{
	unsigned short u, v;          ///< Texture coordinates (16-bit)
	short x, y, z;          ///< Spatial coordinates (16-bit)
} OSL_FAST_VERTEX;

/**
 * @brief Fast vertex with texture and 16-bit color.
 *
 * This vertex type includes 16-bit coordinates for both texture mapping and spatial positioning, along with 32-bit color information. It is optimized for faster rendering with moderate precision.
 */
typedef struct
{
	unsigned short u, v;          ///< Texture coordinates (16-bit)
	unsigned long color;          ///< Color value (32-bit)
	short x, y, z;          ///< Spatial coordinates (16-bit)
} OSL_FAST_VERTEX_COLOR32;

/**
 * @brief Untextured vertex with 32-bit color.
 *
 * This vertex type is used for untextured drawing, with 16-bit spatial coordinates and 32-bit color information.
 */
typedef struct
{
	unsigned long color;          ///< Color value (32-bit)
	short x, y, z;          ///< Spatial coordinates (16-bit)
} OSL_LINE_VERTEX;

/**
 * @brief Untextured vertex with 16-bit color.
 *
 * This vertex type is used for untextured drawing, with 16-bit spatial coordinates and 16-bit color information.
 */
typedef struct
{
	unsigned long color;          ///< Color value (32-bit)
	short x, y, z;          ///< Spatial coordinates (16-bit)
} OSL_LINE_VERTEX_COLOR32;

/**
 * @brief Precise texture vertex.
 *
 * This vertex type combines 16-bit spatial coordinates with floating-point texture coordinates, providing a balance between precision and performance.
 */
typedef struct
{
	float u, v;          ///< Texture coordinates (floating point)
	short x, y, z;          ///< Spatial coordinates (16-bit)
} OSL_UVFLOAT_VERTEX;

/** @} */         // end of drawing_lowlev_vertex

/** @} */         // end of drawing_lowlev

/** @} */         // end of drawing

// *** Image ***

// *** General ***

// *** Internal ***

// Graphics::image

// Graphics::palette

// Graphics::drawing

// Graphics::screen

// Graphics::alpha

#ifdef __cplusplus
}
#endif

#endif
