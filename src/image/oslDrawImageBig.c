#include "oslib.h"
#include "vfpu.h"

#define TEXSIZEX_LIMIT 512
#define TEXSIZEX_LIMITF 512.0f
#define TEXSIZEY_LIMIT 512
#define TEXSIZEY_LIMITF 512.0f

void oslSetTexturePart(OSL_IMAGE *img, int x, int y) {
	u8 *data = (u8*)img->data;

	// Adjust the texture offset when swizzling is enabled
#ifdef PSP
	int swizzleScaleFactor = oslImageIsSwizzled(img) ? 8 : 1;
#else
	int swizzleScaleFactor = 1;
#endif

	oslEnableTexturing();

	if (img->palette && osl_curPalette != img->palette) {
		osl_curPalette = img->palette;
		// Update the palette
		sceGuClutMode(img->palette->pixelFormat, 0, 0xff, 0);
		sceGuClutLoad((img->palette->nElements >> 3), img->palette->data);
	}

	data = (u8*)oslGetImagePixelAdr(img, x * swizzleScaleFactor, y);

	if (osl_curTexture != data) {
		osl_curTexture = data;
		// Update the texture
		sceGuTexMode(img->pixelFormat, 0, 0, oslImageIsSwizzled(img));
		sceGuTexImage(0, TEXSIZEX_LIMIT, TEXSIZEY_LIMIT, img->realSizeX, data);
	}
}

void oslDrawImageBig(OSL_IMAGE *img) {
	OSL_PRECISE_VERTEX *vertices = NULL;
	float cX, cY, tmpY;
	float uVal, xVal;
	const float uCoeff = 64.0f, vCoeff = TEXSIZEY_LIMITF;
	float xCoeff, yCoeff;
	float angleRadians;
	float offsetX0 = img->offsetX0, offsetX1 = img->offsetX1, offsetY0 = img->offsetY0, offsetY1 = img->offsetY1;
	float uLimit, vLimit, vVal;
	int uStart, vStart;
	bool flipH = 0, flipV = 0;
	bool lastTexWrapU = osl_currentTexWrapU, lastTexWrapV = osl_currentTexWrapV;
	int nDiff;

	// Early return for small images
	if (img->sizeX <= TEXSIZEX_LIMIT && img->sizeY <= TEXSIZEY_LIMIT) {
		oslDrawImage(img);
		return;
	}

	// Handle horizontal mirroring
	if (offsetX0 > offsetX1) {
		float tmp = offsetX0;
		offsetX0 = offsetX1;
		offsetX1 = tmp;
		flipH = 1;
	}

	// Handle vertical mirroring
	if (offsetY0 > offsetY1) {
		float tmp = offsetY0;
		offsetY0 = offsetY1;
		offsetY1 = tmp;
		flipV = 1;
	}

	xVal = 0.0f;
	angleRadians = (img->angle * 3.141592653f) / 180.0f;

	// X and Y scaling coefficients
	xCoeff = uCoeff / ((float)(offsetX1 - offsetX0) / (float)img->stretchX);
	yCoeff = vCoeff / ((float)(offsetY1 - offsetY0) / (float)img->stretchY);

	cX = (-img->centerX * img->stretchX) / (int)(offsetX1 - offsetX0);
	cY = (-img->centerY * img->stretchY) / (int)(offsetY1 - offsetY0);

	// Calculate initial vertical offset
	nDiff = (int)offsetY0 >= 0 ? (int)offsetY0 % TEXSIZEY_LIMIT : TEXSIZEY_LIMIT + (int)offsetY0 % TEXSIZEY_LIMIT;
	vStart = offsetY0 - nDiff;
	vVal = offsetY0;

	vLimit = (offsetY1 - vStart >= TEXSIZEY_LIMITF) ? vStart + TEXSIZEY_LIMITF : offsetY1;

	// Adjust coefficients for flipping
	if (flipV) {
		cY += img->stretchY;
		tmpY = cY - ((vLimit - vVal) * img->stretchY) / (int)(offsetY1 - offsetY0);
	} else {
		tmpY = cY + ((vLimit - vVal) * img->stretchY) / (int)(offsetY1 - offsetY0);
	}

	if (flipH) xCoeff = -xCoeff;
	if (flipV) yCoeff = -yCoeff;

	// Clamp the texture to its end
	oslSetTextureWrap(OSL_TW_CLAMP, OSL_TW_CLAMP);

	do {
		// For each line
		uVal = offsetX0;
		xVal = cX;

		if (flipH) {
			xVal += img->stretchX;
		}

		nDiff = (int)offsetX0 >= 0 ? (int)offsetX0 % TEXSIZEX_LIMIT : TEXSIZEX_LIMIT + (int)offsetX0 % TEXSIZEX_LIMIT;
		uStart = offsetX0 - nDiff;
		uLimit = (offsetX1 - uStart >= TEXSIZEX_LIMITF) ? uStart + TEXSIZEX_LIMITF : offsetX1;

		do {
			bool bDrawn = (uStart >= 0 && uStart < img->sizeX && vStart >= 0 && vStart < img->sizeY);

			if (bDrawn) {
				oslSetTexturePart(img, uStart, vStart);
			}

			while (uVal < uLimit) {
				if (bDrawn) {
					vertices = (OSL_PRECISE_VERTEX*)sceGuGetMemory(4 * sizeof(OSL_PRECISE_VERTEX));

					vertices[0].u = uVal - uStart;
					vertices[0].v = vVal - vStart;
					vertices[0].x = oslVfpu_cosf(angleRadians, xVal) - oslVfpu_sinf(angleRadians, cY) + img->x;
					vertices[0].y = oslVfpu_sinf(angleRadians, xVal) + oslVfpu_cosf(angleRadians, cY) + img->y;
					vertices[0].z = 0;

					vertices[2].u = uVal - uStart;
					vertices[2].v = vLimit - vStart;
					vertices[2].x = oslVfpu_cosf(angleRadians, xVal) - oslVfpu_sinf(angleRadians, tmpY) + img->x;
					vertices[2].y = oslVfpu_sinf(angleRadians, xVal) + oslVfpu_cosf(angleRadians, tmpY) + img->y;
					vertices[2].z = 0;
				}

				if (uVal + uCoeff >= uLimit) {
					xVal += ((uLimit - uVal) * xCoeff) / uCoeff;
					uVal += uLimit - uVal;
				} else {
					uVal += uCoeff;
					xVal += xCoeff;
				}

				if (bDrawn) {
					vertices[1].u = uVal - uStart;
					vertices[1].v = vVal - vStart;
					vertices[1].x = oslVfpu_cosf(angleRadians, xVal) - oslVfpu_sinf(angleRadians, cY) + img->x;
					vertices[1].y = oslVfpu_sinf(angleRadians, xVal) + oslVfpu_cosf(angleRadians, cY) + img->y;
					vertices[1].z = 0;

					vertices[3].u = uVal - uStart;
					vertices[3].v = vLimit - vStart;
					vertices[3].x = oslVfpu_cosf(angleRadians, xVal) - oslVfpu_sinf(angleRadians, tmpY) + img->x;
					vertices[3].y = oslVfpu_sinf(angleRadians, xVal) + oslVfpu_cosf(angleRadians, tmpY) + img->y;
					vertices[3].z = 0;

					sceGuDrawArray(GU_TRIANGLE_STRIP, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 4, 0, vertices);
				}
			}

			// Check if there are any pixels left to draw
			if (uLimit < offsetX1) {
				uLimit += TEXSIZEX_LIMITF;
				uStart += TEXSIZEX_LIMIT;
			} else {
				break;
			}

			if (uLimit >= offsetX1) {
				uLimit = offsetX1;
			}
		} while (1);

		float oldVl = vLimit;

		// Check if there are any lines left
		if (vLimit < offsetY1) {
			vLimit += TEXSIZEY_LIMITF;
			vStart += TEXSIZEY_LIMIT;
		} else {
			break;
		}

		if (vLimit >= offsetY1) {
			vLimit = offsetY1;
		}

		// Advance the coefficients
		vVal += (tmpY - cY) * TEXSIZEY_LIMITF / yCoeff;
		cY += (tmpY - cY);
		tmpY += (vLimit - oldVl) * yCoeff / TEXSIZEY_LIMITF;

	} while (1);

	// Restore the texture wrapping
	oslSetTextureWrap(lastTexWrapU, lastTexWrapV);
}
