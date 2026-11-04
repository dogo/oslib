#include "../../oslib.h"
#include <png.h>
#include <pngconf.h>
#include <zlib.h>
#include <zconf.h>

// Read / Write PNG
void oslPngReadFn(png_structp png_ptr, png_bytep data, png_size_t length) {
	VIRTUAL_FILE *f = (VIRTUAL_FILE *)png_get_io_ptr(png_ptr);
	VirtualFileRead(data, length, 1, f);
}

void oslPngFlushFn(png_structp png_ptr) {
	(void)png_ptr; // Suppress unused parameter warning
	// No operation
}

OSL_IMAGE *oslLoadImageFilePNG(char *filename, int location, int volatile pixelFormat) {
	// The PNG signature, then the IHDR chunk: length (4), "IHDR" (4), width (4), height (4),
	// bit depth (1), colour type (1)
	enum { nSigSize = 8, nIhdrTag = nSigSize + 4, nColourType = nIhdrTag + 4 + 4 + 4 + 1 };
	u8 header[nColourType + 1];
	OSL_IMAGE * volatile img = NULL;
	VIRTUAL_FILE *f;
	// We only keep the location bits
	int imgLocation = location & OSL_LOCATION_MASK;
	int i;

	f = VirtualFileOpen((void *)filename, 0, VF_AUTO, VF_O_READ);
	if (!f) goto error;

	if (VirtualFileRead(header, sizeof(u8), sizeof(header), f) != sizeof(header)) goto error;

	if (!png_check_sig(header, nSigSize) || memcmp(header + nIhdrTag, "IHDR", 4)) goto error;

	// Only a PNG with its own palette can go straight into a paletted format; expand anything
	// else (low-bit grey, tRNS) and convert it afterwards
	int keepPalette = osl_pixelWidth[pixelFormat] <= 8 && header[nColourType] == PNG_COLOR_TYPE_PALETTE;
	VirtualFileSeek(f, nSigSize, PSP_SEEK_SET);

	png_structp pPngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pPngStruct) goto error;

	png_infop pPngInfo = png_create_info_struct(pPngStruct);
	if (!pPngInfo) {
		png_destroy_read_struct(&pPngStruct, NULL, NULL);
		goto error;
	}

	if (setjmp(png_jmpbuf(pPngStruct))) {
		png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);
		goto error;
	}

	png_set_read_fn(pPngStruct, f, oslPngReadFn);
	png_set_sig_bytes(pPngStruct, nSigSize);

	if (keepPalette) {
		png_read_png(pPngStruct, pPngInfo, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_BGR, NULL);
	} else {
		png_read_png(pPngStruct, pPngInfo, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR, NULL);
	}

	png_uint_32 width = png_get_image_width(pPngStruct, pPngInfo);
	png_uint_32 height = png_get_image_height(pPngStruct, pPngInfo);
	png_uint_32 depth = png_get_bit_depth(pPngStruct, pPngInfo);
	int color_type = png_get_color_type(pPngStruct, pPngInfo);

	png_byte **pRowTable = png_get_rows(pPngStruct, pPngInfo);
	// Both stay untouched when the PNG has no palette
	png_colorp palette = NULL;
	int num_palette = 0;
	if (!png_get_PLTE(pPngStruct, pPngInfo, &palette, &num_palette))
		num_palette = 0;
	png_bytep trans_alpha = NULL;
	int num_trans = 0;
	if (!png_get_tRNS(pPngStruct, pPngInfo, &trans_alpha, &num_trans, NULL))
		num_trans = 0;

	int wantedPixelFormat = pixelFormat;

	// If we don't have a palette in the PNG, or it has more entries than the pixel format holds,
	// we load the image in 32-bit mode and convert it to paletted later with oslConvertImageTo.
	if (osl_pixelWidth[pixelFormat] <= 8 && (!num_palette || num_palette > 1 << osl_paletteSizes[pixelFormat])) {
		pixelFormat = OSL_PF_8888;
		img = oslCreateImage(width, height, OSL_IN_RAM, pixelFormat);
	} else {
		// Otherwise, we create our image normally directly
		img = oslCreateImage(width, height, imgLocation, pixelFormat);
	}

	if (img) {
		// If there is need for a palette ...
		if (osl_pixelWidth[pixelFormat] <= 8) {
			img->palette = oslCreatePalette(oslMin(num_palette, 1 << osl_paletteSizes[pixelFormat]), OSL_PF_8888);
			if (img->palette) {
				// The palette can hold fewer entries than the PNG provides (e.g. a 256 colour
				// PNG loaded as OSL_PF_4BIT); never write past what was allocated
				int paletteEntries = oslMin(num_palette, img->palette->nElements);
				for (i = 0; i < paletteEntries; i++) {
					unsigned char r = palette[i].red;
					unsigned char g = palette[i].green;
					unsigned char b = palette[i].blue;
					// tRNS gives the first num_trans entries an alpha
					unsigned char a = i < num_trans ? trans_alpha[i] : 0xff;
					// Color key?
					if (osl_colorKeyEnabled && RGBA(r, g, b, 0) == (osl_colorKeyValue & 0x00ffffff)) a = 0;
					((u32 *)img->palette->data)[i] = RGBA(r, g, b, a);
				}
				oslUncachePalette(img->palette);
			}
		}

		u32 *p_dest4 = (u32 *)img->data;
		u16 *p_dest2 = (u16 *)img->data;
		u8 *p_dest1 = (u8 *)img->data;
		size_t color_per_entry = 8 / depth;
		int mask = (1 << depth) - 1;

		for (u32 y = 0; y < height; ++y) {
			const png_byte *pRow = pRowTable[y];
			for (u32 x = 0; x < width; ++x) {
				unsigned char r = 0, g = 0, b = 0, a = 0xff;
				int pixel_value = 0;

				switch (color_type) {
				case PNG_COLOR_TYPE_GRAY:
					r = g = b = *pRow++;
					// Same colour key as RGB images; black is just black
					if (osl_colorKeyEnabled && r == (osl_colorKeyValue & 0xff) && g == ((osl_colorKeyValue >> 8) & 0xff) && b == ((osl_colorKeyValue >> 16) & 0xff)) {
						a = r = g = b = 0;
					}
					break;
				case PNG_COLOR_TYPE_GRAY_ALPHA:
					// Also what PNG_TRANSFORM_EXPAND makes of a grey PNG with a tRNS chunk
					r = g = b = *pRow++;
					a = *pRow++;
					break;
				case PNG_COLOR_TYPE_RGB:
					b = *pRow++;
					g = *pRow++;
					r = *pRow++;
					if (osl_colorKeyEnabled && r == (osl_colorKeyValue & 0xff) && g == ((osl_colorKeyValue >> 8) & 0xff) && b == ((osl_colorKeyValue >> 16) & 0xff)) {
						a = r = g = b = 0;
					}
					break;
				case PNG_COLOR_TYPE_RGB_ALPHA:
					b = *pRow++;
					g = *pRow++;
					r = *pRow++;
					a = *pRow++;
					break;
				case PNG_COLOR_TYPE_PALETTE:
					pixel_value = (*pRow >> (8 - depth * ((x % color_per_entry) + 1))) & mask;
					// Last pixel for this byte
					if (x % color_per_entry == color_per_entry - 1) pRow++;
					if (img->palette) {
						r = ((u32 *)img->palette->data)[pixel_value] & 0xff;
						g = (((u32 *)img->palette->data)[pixel_value] >> 8) & 0xff;
						b = (((u32 *)img->palette->data)[pixel_value] >> 16) & 0xff;
						a = (((u32 *)img->palette->data)[pixel_value] >> 24) & 0xff;
					} else {
						// No OSL palette (32-bit, converted later): the same tRNS and colour key
						b = palette[pixel_value].blue;
						g = palette[pixel_value].green;
						r = palette[pixel_value].red;
						a = pixel_value < num_trans ? trans_alpha[pixel_value] : 0xff;
						if (osl_colorKeyEnabled && RGBA(r, g, b, 0) == (osl_colorKeyValue & 0x00ffffff)) a = 0;
					}
					break;
				}

				if (pixelFormat == OSL_PF_8888) {
					p_dest4[x] = RGBA(r, g, b, a);
				} else if (pixelFormat == OSL_PF_5650) {
					p_dest2[x] = RGB16(r, g, b);
				} else if (pixelFormat == OSL_PF_5551) {
					p_dest2[x] = RGBA15(r, g, b, a);
				} else if (pixelFormat == OSL_PF_4444) {
					p_dest2[x] = RGBA12(r, g, b, a);
				} else if (pixelFormat == OSL_PF_8BIT) {
					p_dest1[x] = pixel_value;
				} else if (pixelFormat == OSL_PF_4BIT) {
					p_dest1[x >> 1] &= ~(15 << ((x & 1) << 2));
					p_dest1[x >> 1] |= (pixel_value & 15) << ((x & 1) << 2);
				}
			}

			p_dest1 += (img->realSizeX * osl_pixelWidth[pixelFormat]) >> 3;
			p_dest2 += img->realSizeX;
			p_dest4 += img->realSizeX;
		}

		// Finally convert to the paletted format
		if (wantedPixelFormat != pixelFormat) {
			img = oslConvertImageTo(img, imgLocation, wantedPixelFormat);
		}

		png_destroy_read_struct(&pPngStruct, &pPngInfo, NULL);

		if (oslImageLocationIsSwizzled(location)) {
			oslSwizzleImage(img);
		}

		oslUncacheImage(img);
	}

error:
	if (f) VirtualFileClose(f);
	if (!img) oslHandleLoadNoFailError(filename);
	return img;
}
