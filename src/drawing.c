#include "oslib.h"

// Standard memory allocation
unsigned int *osl_list = NULL;

// Memory pointers for various resources
void *osl_curTexture = NULL;
void *osl_curPalette = NULL;
void *osl_curDrawBuf = (void*)0;
void *osl_curDispBuf = (void*)0;

// Configuration flags
int osl_bilinearFilterEnabled = 0;
int osl_ditheringEnabled = 0;
int osl_isDrawingStarted = 0;
int osl_currentAlphaEffect = 0;
int osl_colorKeyEnabled = 0;
int osl_colorKeyValue = 0;
int osl_alignBuffer = 3;
int osl_doubleBuffer = 1;
int osl_textureEnabled = 1;
int osl_alphaTestEnabled = 0;
int osl_currentTexWrapU = 0;
int osl_currentTexWrapV = 0;

// Alpha blending coefficients
OSL_COLOR osl_currentAlphaCoeff = 0xFFFFFFFF;
OSL_COLOR osl_currentAlphaCoeff2 = 0xFFFFFFFF;

// Image buffers
OSL_IMAGE osl_defaultBufferImage;
OSL_IMAGE osl_secondaryBufferImage;
OSL_IMAGE *osl_curBuf = NULL;

// Pixel format configuration
const int osl_pixelWidth[] = {16, 16, 16, 32, 4, 8};

// Palette sizes (2^n)
const u8 osl_paletteSizes[] = {
	0, // 16 bits
	0, // 16 bits
	0, // 16 bits
	0, // 32 bits
	4, // 4 bits
	8 // 8 bits
};

void oslSetAlpha2(u32 effect, u32 coeff1, u32 coeff2) {
	int effet;
	osl_currentAlphaEffect = effect | OSL_FX_COLOR;

	if (effect > OSL_FX_NONE) {
		effet = effect & ~OSL_FX_COLOR;

		if (effet == OSL_FX_RGBA) {
			sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
			sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
			sceGuAmbientColor(0xFFFFFFFF);
			osl_currentAlphaCoeff = 0xFFFFFFFF;
			sceGuEnable(GU_BLEND);
			return;
		}

		if (!(effect & OSL_FX_COLOR)) {
			coeff1 = (coeff1 << 24) | 0xFFFFFF;
		}

		if (effet == OSL_FX_ALPHA) {
			// Regular alpha blending
			sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
			sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
			sceGuAmbientColor(coeff1);
			osl_currentAlphaCoeff = coeff1;
		} else if (effet == OSL_FX_ADD) {
			// Additive blending
			sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, coeff2);
			sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
			sceGuAmbientColor(coeff1);
			osl_currentAlphaCoeff = coeff1;
			osl_currentAlphaCoeff2 = coeff2;
		} else if (effet == OSL_FX_SUB) {
			// Subtractive blending
			sceGuBlendFunc(GU_REVERSE_SUBTRACT, GU_SRC_ALPHA, GU_FIX, 0, coeff2);
			sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
			sceGuAmbientColor(coeff1);
			osl_currentAlphaCoeff = coeff1;
			osl_currentAlphaCoeff2 = coeff2;
		}

		sceGuEnable(GU_BLEND);
	} else {
		// Disable blending
		osl_currentAlphaCoeff = 0xFFFFFFFF;
		sceGuDisable(GU_BLEND);
	}
}

void oslSetAlphaWrite(int action, int value1, int value2) {
#ifdef PSP
	if (action == OSL_FXAW_SET) {
		// Set the stencil function to always pass and replace the stencil buffer value with value1
		sceGuStencilFunc(GU_ALWAYS, value1, 0xFF);
		sceGuStencilOp(GU_KEEP, GU_REPLACE, GU_REPLACE);
		sceGuEnable(GU_STENCIL_TEST);
	} else if (action == OSL_FXAW_NONE) {
		// Disable stencil testing, leaving the alpha channel unchanged
		sceGuDisable(GU_STENCIL_TEST);
	}
#endif
}

void oslDisableTransparentColor() {
	osl_colorKeyEnabled = 0;
	sceGuDisable(GU_COLOR_TEST);
}

void oslSetTransparentColor(OSL_COLOR color) {
	osl_colorKeyEnabled = 1;
	osl_colorKeyValue = color;

	// Set the color test function to mask out the specified color
	sceGuColorFunc(GU_NOTEQUAL, color, 0xFFFFFFFF);
	sceGuEnable(GU_COLOR_TEST);
}

int oslConvertColor(int pfDst, int pfSrc, int color) {
	int r = 0, g = 0, b = 0, a = 0;

	// If the source and destination formats are the same, return the original color.
	if (pfSrc == pfDst) {
		return color;
	}

	// Extract color components based on the source pixel format.
	switch (pfSrc) {
	case OSL_PF_8888:
		oslRgbaGet8888(color, r, g, b, a);
		break;
	case OSL_PF_5650:
		oslRgbGet5650f(color, r, g, b);
		a = 0xff; // Set alpha to fully opaque.
		break;
	case OSL_PF_5551:
		oslRgbaGet5551f(color, r, g, b, a);
		break;
	case OSL_PF_4444:
		oslRgbaGet4444f(color, r, g, b, a);
		break;
	default:
		// Handle unknown source formats implicitly by falling through
		break;
	}

	// Convert color components to the destination pixel format.
	switch (pfDst) {
	case OSL_PF_8888:
		color = RGBA(r, g, b, a);
		break;
	case OSL_PF_5650:
		color = RGB16(r, g, b);
		break;
	case OSL_PF_5551:
		color = RGBA15(r, g, b, a);
		break;
	case OSL_PF_4444:
		color = RGBA12(r, g, b, a);
		break;
	default:
		// Handle unknown destination formats implicitly by falling through
		break;
	}

	return color;
}

int oslConvertColorEx(OSL_PALETTE *p, int pfDst, int pfSrc, int color) {
	int r = 0, g = 0, b = 0, a = 0;

	// If the source and destination formats are the same, return the original color.
	if (pfSrc == pfDst) {
		return color;
	}

	// If no palette is provided, fall back to standard color conversion.
	if (!p) {
		return oslConvertColor(pfDst, pfSrc, color);
	}

	// Extract color components based on the source pixel format.
	switch (pfSrc) {
	case OSL_PF_8888:
		oslRgbaGet8888(color, r, g, b, a);
		break;
	case OSL_PF_5650:
		oslRgbGet5650f(color, r, g, b);
		a = 0xff; // Set alpha to fully opaque.
		break;
	case OSL_PF_5551:
		oslRgbaGet5551f(color, r, g, b, a);
		break;
	case OSL_PF_4444:
		oslRgbaGet4444f(color, r, g, b, a);
		break;
	case OSL_PF_8BIT:
	case OSL_PF_4BIT:
		// Convert indexed color to true color using the palette.
		color = oslConvertColor(OSL_PF_8888, p->pixelFormat, oslGetPaletteColor(p, color));
		break;
	}

	// Convert color components to the destination pixel format.
	switch (pfDst) {
	case OSL_PF_8888:
		color = RGBA(r, g, b, a);
		break;
	case OSL_PF_5650:
		color = RGB16(r, g, b);
		break;
	case OSL_PF_5551:
		color = RGBA15(r, g, b, a);
		break;
	case OSL_PF_4444:
		color = RGBA12(r, g, b, a);
		break;
	case OSL_PF_8BIT:
	case OSL_PF_4BIT: {
		// Search for a matching color in the palette.
		for (u32 i = 0; i < p->nElements; i++) {
			if (oslGetPaletteColor(p, i) == color) {
				return i;
			}
		}
		break;
	}
	}

	return color;
}

void oslDrawTile(int u, int v, int x, int y, int tX, int tY) {
	// Allocate memory for two vertices (bottom-left and top-right of the tile)
	OSL_FAST_VERTEX *vertices = (OSL_FAST_VERTEX*)sceGuGetMemory(2 * sizeof(OSL_FAST_VERTEX));

	// Set the properties for the first vertex (top-left corner of the tile)
	vertices[0].u = u;
	vertices[0].v = v;
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0;

	// Set the properties for the second vertex (bottom-right corner of the tile)
	vertices[1].u = u + tX;
	vertices[1].v = v + tY;
	vertices[1].x = x + tX;
	vertices[1].y = y + tY;
	vertices[1].z = 0;

	// Draw the tile using the GU_SPRITES primitive, with texture and vertex data in 16-bit mode
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
}

void oslStartDrawing() {
	// Check if drawing has already started
	if (osl_isDrawingStarted) {
		return;
	}

	osl_isDrawingStarted = 1;
	osl_curTexture = NULL;
	osl_curPalette = NULL;
	sceGuStart(GU_DIRECT, osl_list);

	// Set the default alpha blending mode
	oslSetAlpha(OSL_FX_RGBA, 0xff);
}

void oslEndDrawing() {
	if (!osl_isDrawingStarted) {
		return;
	}
	sceGuFinish();
	sceGuSync(0, 0);
	osl_isDrawingStarted = 0;
}

void oslSyncDrawing() {
	if (osl_isDrawingStarted) {
		sceGuFinish();
		sceGuSync(0, 0);
		sceGuStart(GU_DIRECT, osl_list);
	}
}

void oslSetSysDisplayListSize(int newSize) {
	// Ensure that the GPU is not processing commands
	oslEndDrawing();

	if (osl_list) {
		free(osl_list);
	}

	// Allocate memory for the new display list size (aligned to 16 bytes)
	osl_list = (unsigned int*)memalign(16, newSize);
}

void oslInitGfx(int pixelFormat, int bDoubleBuffer) {
	u8 *baseAdr;
	bool alreadyInited = (osl_list != NULL);

	osl_doubleBuffer = bDoubleBuffer;

	if (!alreadyInited) {
		oslSetupFTrigo();
		oslSetSysDisplayListSize(1 << 20); // Allocate 1 MB for the display list
		sceGuInit();
	}

	sceGuStart(GU_DIRECT, osl_list);
	sceGuDisplay(0);
	sceGuDrawBuffer(pixelFormat, (void*)0, 512);

	if (bDoubleBuffer) {
		sceGuDispBuffer(480, 272, (void*)((0x22000 * osl_pixelWidth[pixelFormat]) >> 3), 512);
		baseAdr = (u8*)(OSL_UVRAM_BASE + ((0x22000 * 2 * osl_pixelWidth[pixelFormat]) >> 3));
		osl_curDrawBuf = (void*)OSL_UVRAM_BASE;
		osl_curDispBuf = (void*)(OSL_UVRAM_BASE + ((0x22000 * osl_pixelWidth[pixelFormat]) >> 3));
	} else {
		sceGuDispBuffer(480, 272, (void*)0, 512);
		baseAdr = (u8*)(OSL_UVRAM_BASE + ((0x22000 * osl_pixelWidth[pixelFormat]) >> 3));
		osl_curDrawBuf = (void*)OSL_UVRAM_BASE;
		osl_curDispBuf = osl_curDrawBuf;
	}

	// Set up depth buffer and other graphical settings
	sceGuDepthBuffer((void*)((u32)baseAdr - (u32)OSL_UVRAM_BASE), 512);
	baseAdr = (u8*)((u32)baseAdr + 0x22000 * 2);
	sceGuOffset(2048 - 480 / 2, 2048 - 272 / 2);
	sceGuViewport(2048, 2048, 480, 272);
	sceGuDepthRange(65535, 0);
	sceGuScissor(0, 0, 480, 272);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuDisable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

	// Set texture and alpha settings
	sceGuEnable(GU_TEXTURE_2D);
	osl_textureEnabled = 1;
	sceGuDisable(GU_ALPHA_TEST);
	osl_alphaTestEnabled = 0;
	sceGuDisable(GU_DITHER);
	osl_ditheringEnabled = 0;
	osl_bilinearFilterEnabled = 0;
	oslSetTextureWrap(OSL_TW_REPEAT, OSL_TW_REPEAT);

	// Initialize VRAM manager
	oslVramMgrInit();
	oslVramMgrSetParameters((void*)baseAdr, (u32)OSL_UVRAM_END - (u32)baseAdr);

	osl_isDrawingStarted = 0;

	// Finish setup and enable display
	sceGuFinish();
	sceGuSync(0, 0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(1);

	// Initialize the default buffer image
	memset(&osl_defaultBufferImage, 0, sizeof(osl_defaultBufferImage));
	osl_defaultBufferImage.sizeX = 480;
	osl_defaultBufferImage.sizeY = 272;
	osl_defaultBufferImage.realSizeX = 512;
	osl_defaultBufferImage.realSizeY = 272;
	osl_defaultBufferImage.pixelFormat = pixelFormat;
	osl_defaultBufferImage.location = OSL_IN_VRAM;
	osl_defaultBufferImage.data = osl_curDrawBuf;
	osl_defaultBufferImage.sysSizeX = 512;
	osl_defaultBufferImage.sysSizeY = 512;
	osl_defaultBufferImage.totalSize = (0x22000 * osl_pixelWidth[pixelFormat]) >> 3;
	oslSetImageTile(&osl_defaultBufferImage, 0, 0, 480, 272);
	osl_curBuf = &osl_defaultBufferImage;

	// Initialize the secondary buffer image
	memcpy(&osl_secondaryBufferImage, &osl_defaultBufferImage, sizeof(osl_secondaryBufferImage));
	osl_secondaryBufferImage.data = osl_curDispBuf;

	oslStartDrawing();
	oslSetAlpha(OSL_FX_RGBA, 0);

	if (!alreadyInited)
		oslInitConsole();
}

void oslSwapBuffers()
{
	// Reset the user's draw buffer to OSL_DEFAULT_BUFFER if it's not already set
	if (osl_curBuf != OSL_DEFAULT_BUFFER) {
		oslSetDrawBuffer(OSL_DEFAULT_BUFFER);
	}

	// The current draw buffer becomes the display buffer
	osl_curDispBuf = osl_curDrawBuf;

	// If double buffering is enabled, obtain the new draw buffer pointer
	if (osl_doubleBuffer) {
#ifdef PSP
		osl_curDrawBuf = oslAddVramPrefixPtr(sceGuSwapBuffers());
#else
		osl_curDrawBuf = oslGetUncachedPtr(sceGuSwapBuffers());
#endif
	}

	// Update buffer image pointers
	osl_defaultBufferImage.data = osl_curDrawBuf;
	osl_secondaryBufferImage.data = osl_curDispBuf;
}

void oslSetBilinearFilter(int enabled)
{
	osl_bilinearFilterEnabled = enabled;
	int filterMode = enabled ? GU_LINEAR : GU_NEAREST;
	sceGuTexFilter(filterMode, filterMode);
}

void oslSetDithering(int enabled)
{
	osl_ditheringEnabled = enabled;
	if (enabled) {
		sceGuEnable(GU_DITHER);
	} else {
		sceGuDisable(GU_DITHER);
	}
}

void oslSetAlphaTest(int condition, int value)
{
	if (!osl_alphaTestEnabled) {
		sceGuAlphaFunc(condition, value, 0xFF);
		sceGuEnable(GU_ALPHA_TEST);
		osl_alphaTestEnabled = 1;
	}
}

void oslDisableAlphaTest()
{
	if (osl_alphaTestEnabled) {
		sceGuDisable(GU_ALPHA_TEST);
		osl_alphaTestEnabled = 0;
	}
}

void oslClearScreen(int backColor)
{
	sceGuClearColor(backColor);
	sceGuClear(GU_COLOR_BUFFER_BIT);
}

void oslSetScreenClipping(int x0, int y0, int x1, int y1)
{
	sceGuScissor(x0, y0, x1, y1);
	sceGuEnable(GU_SCISSOR_TEST);
}

void oslEndGfx()
{
	sceGuTerm();

	if (osl_list) {
		free(osl_list);
		osl_list = NULL;
	}
}

void oslSetDepthTest(int enabled)
{
	if (enabled) {
		sceGuEnable(GU_DEPTH_TEST);
	} else {
		sceGuDisable(GU_DEPTH_TEST);
	}
	osl_alphaTestEnabled = enabled;
}
