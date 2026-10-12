#include "oslib.h"

/**
 * Draws an image at its specified coordinates using a simple blit.
 * This function does not account for any rotation or scaling beyond the basic stretch.
 * It is optimized for simplicity and minimal overhead.
 *
 * @param img The image to draw.
 */
void oslDrawImageSimple(OSL_IMAGE *img) {
	// Pointer to the vertex array for rendering the image
	OSL_UVFLOAT_VERTEX *vertices;

	// Get the image's position
	int x = img->x;
	int y = img->y;

	// Set the texture to be used for rendering
	oslSetTexture(img);

	// The GE's bilinear filter doesn't land on exact texel centres for 1:1 sprites (it depends on the
	// size and even the texture address), blurring them. Unscaled draws need no filtering anyway,
	// unless the UVs are fractional (e.g. a half-texel shift), which does need interpolation.
	int unscaled = osl_bilinearFilterEnabled && img->offsetX0 == (int)img->offsetX0 && img->offsetY0 == (int)img->offsetY0 &&
	               img->stretchX == oslAbs(img->offsetX1 - img->offsetX0) && img->stretchY == oslAbs(img->offsetY1 - img->offsetY0);
	if (unscaled)
		sceGuTexFilter(GU_NEAREST, GU_NEAREST);

	// If auto-strip is enabled, verify and handle it
	if (oslImageGetAutoStrip(img) && oslVerifyStripBlit(img)) {
		if (unscaled)
			sceGuTexFilter(GU_LINEAR, GU_LINEAR);
		return; // Strip blit done
	}

	// Allocate memory for two vertices
	vertices = (OSL_UVFLOAT_VERTEX *)sceGuGetMemory(2 * sizeof(OSL_UVFLOAT_VERTEX));

	// Define the top-left vertex of the image
	vertices[0].u = img->offsetX0;
	vertices[0].v = img->offsetY0;
	vertices[0].x = x;
	vertices[0].y = y;
	vertices[0].z = 0;

	// Define the bottom-right vertex of the image
	vertices[1].u = img->offsetX1;
	vertices[1].v = img->offsetY1;
	vertices[1].x = x + img->stretchX;
	vertices[1].y = y + img->stretchY;
	vertices[1].z = 0;

	// Draw the image as a 2D sprite
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
	if (unscaled)
		sceGuTexFilter(GU_LINEAR, GU_LINEAR);
}
