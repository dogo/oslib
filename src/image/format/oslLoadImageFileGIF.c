#include "oslib.h"
#include "gif_lib.h"

/*
    Note: The temporary palette here is 32-bit, but it would be easy to adapt to other formats,
    see Palette[i] = RGBA(...).
 */
u32 *osl_gifTempPalette; // Used to store the GIF palette
const short InterlacedOffset[] = { 0, 4, 2, 1 }; /* The way Interlaced image should be read - offsets */
const short InterlacedJumps[] = { 8, 8, 4, 2 };  /* The way Interlaced image should be read - jumps */
GifPixelType *osl_gifLineBuf = NULL; // Temporary buffer, grown to fit the image width
static int osl_gifLineBufSize = 0;

// Grow the temporary line buffer to hold a full line of the given width
static int oslGifEnsureLineBuf(int width) {
	if (width <= 0)
		return 0;

	if (width > osl_gifLineBufSize) {
		GifPixelType *buf = (GifPixelType *)realloc(osl_gifLineBuf, width * sizeof(GifPixelType));
		if (!buf)
			return 0;
		osl_gifLineBuf = buf;
		osl_gifLineBufSize = width;
	}

	return 1;
}

static void oslGifFreeLineBuf(void) {
	free(osl_gifLineBuf);
	osl_gifLineBuf = NULL;
	osl_gifLineBufSize = 0;
}

static int fnGifReadFunc(GifFileType *GifFile, GifByteType *buf, int count) {
	// Read data from the virtual file associated with the GIF
	VirtualFileRead(buf, 1, count, (VIRTUAL_FILE *)GifFile->UserData);

	return count;
}

static void fnCopyLine(void *dst, void *src, int count, int pixelFormat) {
	int x;
	u8 *p_dest1 = (u8 *)dst;
	u16 *p_dest2 = (u16 *)dst;
	u32 *p_dest4 = (u32 *)dst;
	u8 *p_src = (u8 *)src;
	u32 pixel_value;

	for (x = 0; x < count; x++) {
		// Get the next pixel palette entry
		pixel_value = p_src[x];
		// True color mode => convert the temporary color to the destination format
		if (osl_pixelWidth[pixelFormat] > 8)
			pixel_value = oslConvertColor(pixelFormat, OSL_PF_8888, osl_gifTempPalette[pixel_value]);

		// Copy pixel based on destination format
		if (osl_pixelWidth[pixelFormat] == 32) {
			p_dest4[x] = pixel_value;
		} else if (osl_pixelWidth[pixelFormat] == 16)
			p_dest2[x] = pixel_value;
		else if (osl_pixelWidth[pixelFormat] == 8)
			p_dest1[x] = pixel_value;
		else if (osl_pixelWidth[pixelFormat] == 4) {
			p_dest1[x >> 1] &= ~(15 << ((x & 1) << 2));
			p_dest1[x >> 1] |= (pixel_value & 15) << ((x & 1) << 2);
		}
	}
}

static int DGifGetLineByte(GifFileType *GifFile, GifPixelType *Line, int LineLen, int pixelFormat) {
	// Never decode more pixels than the temporary buffer holds
	if (LineLen <= 0 || LineLen > osl_gifLineBufSize)
		return GIF_ERROR;

	// Get the next line of pixels from the GIF and copy it to the destination buffer
	int result = DGifGetLine(GifFile, osl_gifLineBuf, LineLen);
	fnCopyLine(Line, osl_gifLineBuf, LineLen, pixelFormat);
	return result;
}

OSL_IMAGE *oslLoadImageFileGIF(char *filename, int location, int pixelFormat) {
	OSL_IMAGE *img = NULL;
	int i, j, alpha, Row = 0, Col = 0, Width, Height, ExtCode, ErrorCode;
	u32 *Palette = NULL;
	GifRecordType RecordType;
	GifByteType *Extension;
	GifFileType *GifFile;
	ColorMapObject *ColorMap;
	int transparentColor = -1;
	int imgLocation = location & OSL_LOCATION_MASK;
	VIRTUAL_FILE *f;

	// Temporary palette for frames decoded in true color (also a paletted format's frame
	// whose colour table doesn't fit: decoded in 32-bit, converted at the end)
	osl_gifTempPalette = (u32 *)malloc(256 * sizeof(u32));
	if (!osl_gifTempPalette)
		return NULL;

	// Open the GIF file using the virtual file system
	f = VirtualFileOpen((void *)filename, 0, VF_AUTO, VF_O_READ);
	if (f) {
		GifFile = DGifOpen(f, fnGifReadFunc, &ErrorCode);

		// Scan the content of the GIF file and load the image(s)
		while (GifFile) {
			if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR)
				break;

			switch (RecordType) {
			case IMAGE_DESC_RECORD_TYPE:
				if (DGifGetImageDesc(GifFile) == GIF_ERROR)
					break;

				// Ignore image position, but consider this for animated GIFs
				Row = Col = 0;
				Width = GifFile->Image.Width;
				Height = GifFile->Image.Height;

				// The temporary line buffer must hold a full line of this image
				if (!oslGifEnsureLineBuf(Width))
					break;

				// Update the color map - the file may carry neither a local nor a global one
				ColorMap = (GifFile->Image.ColorMap ? GifFile->Image.ColorMap : GifFile->SColorMap);
				if (!ColorMap)
					break;

				// Only the last frame is returned: drop the previous one
				if (img) {
					oslDeleteImage(img);
					img = NULL;
				}

				// Create the image to store the data
				int format = pixelFormat;
				if (osl_pixelWidth[format] <= 8 && ColorMap->ColorCount > 1 << osl_paletteSizes[format])
					format = OSL_PF_8888;
				img = oslCreateImage(Width, Height, format == pixelFormat ? imgLocation : OSL_IN_RAM, format);
				if (!img)
					break;

				Palette = osl_gifTempPalette;
				if (osl_pixelWidth[format] <= 8) {
					img->palette = oslCreatePalette(ColorMap->ColorCount, OSL_PF_8888);
					if (!img->palette) {
						oslDeleteImage(img);
						img = NULL;
						break;
					}
					Palette = (u32 *)img->palette->data;
				}

				// Handle the transparent color
				i = ColorMap->ColorCount;
				while (--i >= 0) {
					GifColorType *pColor = &ColorMap->Colors[i];
					if (i == transparentColor || (osl_colorKeyEnabled && RGBA(pColor->Red, pColor->Green, pColor->Blue, 0) == (osl_colorKeyValue & 0x00ffffff)))
						alpha = 0;
					else
						alpha = 0xff;
					Palette[i] = RGBA(pColor->Red, pColor->Green, pColor->Blue, alpha);
				}

				// Handle interlaced images
				if (GifFile->Image.Interlace) {
					// Perform 4 passes for interlaced images
					for (i = 0; i < 4; i++) {
						for (j = Row + InterlacedOffset[i]; j < Row + Height; j += InterlacedJumps[i]) {
							DGifGetLineByte(GifFile, (GifPixelType *)oslGetImagePixelAdr(img, Col, j), Width, img->pixelFormat);
						}
					}
				} else {
					// For non-interlaced images, read the lines sequentially
					for (i = 0; i < Height; i++) {
						DGifGetLineByte(GifFile, (GifPixelType *)oslGetImagePixelAdr(img, Col, Row), Width, img->pixelFormat);
						Row++;
					}
				}
				break;

			case EXTENSION_RECORD_TYPE:
				// Skip any extension blocks in the file
				DGifGetExtension(GifFile, &ExtCode, &Extension);

				while (Extension != NULL) {
					// Handle transparent color extension
					if (ExtCode == 249) {
						if (Extension[1] & 1) {
							transparentColor = Extension[4];
						}
					}
					DGifGetExtensionNext(GifFile, &Extension);
				}
				break;

			case TERMINATE_RECORD_TYPE:
				break;

			default:
				break;
			}

			if (RecordType == TERMINATE_RECORD_TYPE)
				break;
		}

		// Close the file when done
		if (GifFile)
			DGifCloseFile(GifFile, &ErrorCode);
		VirtualFileClose(f);

		// Free the temporary line buffer
		oslGifFreeLineBuf();

		// A frame decoded in 32-bit goes to the wanted format now; on failure it's still ours
		if (img && img->pixelFormat != pixelFormat) {
			OSL_IMAGE *converted = oslConvertImageTo(img, imgLocation, pixelFormat);
			if (!converted) oslDeleteImage(img);
			img = converted;
		}

		if (img) {
			// Swizzle the image if necessary
			if (oslImageLocationIsSwizzled(location))
				oslSwizzleImage(img);

			// Uncache the image so that it's ready to use
			oslUncacheImage(img);
		}
	}

	free(osl_gifTempPalette);
	osl_gifTempPalette = NULL;

	if (!img)
		oslHandleLoadNoFailError(filename);

	return img;
}
