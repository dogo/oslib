#include "oslib.h"
#include "sfont.h"
#include <png.h>

///////////////////////////////////////////////////////////////////////////////
// Private functions
///////////////////////////////////////////////////////////////////////////////

// Get offset for a character in the font
static int _getOffset(OSL_SFONT *sfont, unsigned char ch)
{
	int offset;

	// Check for invalid character ranges
	if (ch <= 32 || (ch >= 127 && ch <= 159))
		return -1;

	// Calculate the offset for valid characters
	if (ch >= 160)
		offset = ch - 160 + 126 - 33;
	else
		offset = ch - 33;

	// Ensure the offset is within bounds
	return (offset >= sfont->lettersCount) ? -1 : offset;
}

// Get the next power of 2 for a given width
static u16 _getNextPower2(u16 width)
{
	u16 power = 1;

	// Find the next power of 2 greater than or equal to the width
	while (power < width)
	{
		power *= 2;
	}

	// Ensure the minimum power of 2 is 16
	return (power < 16) ? 16 : power;
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
}

// Struct to hold PNG image data
typedef struct
{
	u32 *rawdata;     // Pointer to the raw pixel data
	u16 sizeX;        // Actual width of the image
	u16 sizeY;        // Actual height of the image
	u16 textureSizeX; // Width adjusted to the next power of 2
	u16 textureSizeY; // Height adjusted to the next power of 2
} PNG_DATA;

// Load a PNG image (OSLib cannot handle images bigger than 512x512)
static PNG_DATA* _loadFromPNG(const char *filename) {
	// Allocate memory for PNG_DATA structure
	PNG_DATA *pngData = (PNG_DATA*)malloc(sizeof(PNG_DATA));
	if (!pngData)
		return NULL;

	// Declare necessary variables
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	FILE *fp = NULL;
	VIRTUAL_FILE *f = NULL;
	u8 **rows = NULL;
	unsigned int sig_read = 0;
	int bit_depth, color_type, interlace_type;
	png_uint_32 pw, ph;
	int passes;

	// Open virtual file
	f = VirtualFileOpen((void*)filename, 0, VF_AUTO, VF_O_READ);
	if (!f) {
		free(pngData);
		return NULL;
	}

	// Read file into memory
	int input_size = 0;
	const unsigned char *input = (const unsigned char*)oslReadEntireFileToMemory(f, &input_size);
	const unsigned char *input_free = input;
	VirtualFileClose(f);

	// Open file in memory
	fp = fmemopen((void *)input, input_size, "rb");
	if (!fp) {
		free((void*)input_free);
		free(pngData);
		return NULL;
	}

	// Initialize PNG read struct
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, user_warning_fn);
	if (!png_ptr) {
		fclose(fp);
		free((void*)input_free);
		free(pngData);
		return NULL;
	}

	// Initialize PNG info struct
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(fp);
		free((void*)input_free);
		free(pngData);
		return NULL;
	}

	// Setup error handling for PNG reading
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		free((void*)input_free);
		free(pngData);
		return NULL;
	}

	// Read the PNG header information
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &pw, &ph, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	// Set size and texture dimensions
	pngData->sizeX = (u16)pw;
	pngData->sizeY = (u16)ph;
	pngData->textureSizeX = _getNextPower2(pngData->sizeX);
	pngData->textureSizeY = _getNextPower2(pngData->sizeY);

	// Set various PNG transformations
	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	if (interlace_type == PNG_INTERLACE_ADAM7) {
		passes = png_set_interlace_handling(png_ptr);
		png_read_update_info(png_ptr, info_ptr);
	} else {
		passes = 1;
	}

	// Allocate memory for raw image data
	pngData->rawdata = (u32*)memalign(16, pngData->textureSizeX * pngData->textureSizeY * sizeof(u32));
	if (!pngData->rawdata) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		free((void*)input_free);
		free(pngData);
		return NULL;
	}

	// Allocate memory for row pointers
	rows = (u8**)malloc(pngData->sizeY * sizeof(u8*));
	if (!rows) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		free((void*)input_free);
		free(pngData->rawdata);
		free(pngData);
		return NULL;
	}

	// Set row pointers for reading PNG data
	for (int y = 0; y < pngData->sizeY; ++y) {
		rows[y] = (u8*)(pngData->rawdata + y * pngData->textureSizeX);
	}

	// Read the PNG image data row by row
	for (int pass = 0; pass < passes; ++pass) {
		png_read_rows(png_ptr, rows, NULL, pngData->sizeY);
	}

	// Cleanup
	free(rows);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	free((void*)input_free);

	return pngData;
}

// Get a pixel from a PNG image at (x, y)
static u32 _getPixel(PNG_DATA *pngData, u16 x, u16 y)
{
	// Check if coordinates are within bounds
	if (!pngData || x >= pngData->textureSizeX || y >= pngData->textureSizeY)
		return 0; // Return a default value (black) if out of bounds or null data

	return pngData->rawdata[y * pngData->textureSizeX + x];
}

// Delete (free memory) of a PNG image
static void _deletePngImage(PNG_DATA *pngData)
{
	if (!pngData)
		return;

	// Free the raw data if it's not null
	if (pngData->rawdata) {
		free(pngData->rawdata);
		pngData->rawdata = NULL;
	}

	// Free the structure itself
	free(pngData);
}

///////////////////////////////////////////////////////////////////////////////
// Public functions
///////////////////////////////////////////////////////////////////////////////
OSL_SFONT *oslLoadSFontFile(char *filename, int pixelFormat) {
	// Allocate memory for the OSL_SFONT structure
	OSL_SFONT *sfont = (OSL_SFONT*)malloc(sizeof(OSL_SFONT));
	if (!sfont) {
		oslHandleLoadNoFailError(filename);
		return NULL;
	}

	// Load the PNG image
	PNG_DATA *img = _loadFromPNG(filename);
	if (!img) {
		free(sfont);
		oslHandleLoadNoFailError(filename);
		return NULL;
	}

	unsigned int refcolor = _getPixel(img, 0, 0) & 0x00FFFFFFU;
	int letterCount = 0;
	int x = 0;

	sfont->lettersCount = 0;

	// Process each column in the PNG image to extract font letters
	while (x < img->sizeX) {
		int color = _getPixel(img, x, 0);

		if ((color & 0x00FFFFFFU) != refcolor) {
			int pos = x;

			// Find the width of the current letter by scanning until the reference color is hit
			while (x < img->sizeX && ((_getPixel(img, x, 0) & 0x00FFFFFFU) != refcolor)) {
				++x;
			}

			// Allocate memory for a new letter
			OSL_SFLETTER *lt = (OSL_SFLETTER*)malloc(sizeof(OSL_SFLETTER));
			if (!lt) {
				_deletePngImage(img);
				oslDeleteSFont(sfont); // Frees all previously allocated letters and sfont
				oslHandleLoadNoFailError(filename);
				return NULL;
			}

			lt->width = x - pos;
			lt->letter = oslCreateImage(lt->width, img->sizeY - 1, OSL_IN_RAM, pixelFormat);
			if (!lt->letter) {
				free(lt);
				_deletePngImage(img);
				oslDeleteSFont(sfont);
				oslHandleLoadNoFailError(filename);
				return NULL;
			}

			oslClearImage(lt->letter, RGBA(0, 0, 0, 0));
			oslLockImage(lt->letter);

			// Copy the image data into the letter
			for (int dx = pos; dx < pos + lt->width; ++dx) {
				for (int dy = 1; dy < img->sizeY; ++dy) {
					int pixel = _getPixel(img, dx, dy);
					pixel = oslConvertColorEx(lt->letter->palette, lt->letter->pixelFormat, OSL_PF_8888, pixel);
					oslSetImagePixel(lt->letter, dx - pos, dy - 1, pixel);
				}
			}

			oslUnlockImage(lt->letter);
			oslSwizzleImage(lt->letter);

			// Assign the letter to the font and increment the letter count
			sfont->letters[letterCount++] = lt;
		} else {
			++x;
		}
	}

	sfont->height = img->sizeY - 1;
	sfont->lettersCount = letterCount;

	// Clean up the image data after processing
	_deletePngImage(img);

	return sfont;
}

void oslDeleteSFont(OSL_SFONT *sfont) {
	if (!sfont) return;

	// Free each letter's image
	for (int i = 0; i < sfont->lettersCount; ++i) {
		if (sfont->letters[i]) {
			oslDeleteImage(sfont->letters[i]->letter);
			free(sfont->letters[i]);
		}
	}

	// Free the sfont struct itself
	free(sfont);
	sfont = NULL;
}

int oslGetSFontTextWidth(OSL_SFONT *sfont, char *text) {
	if (!sfont || !text) return 0;

	int width = 0;

	// Iterate through each character of the text
	for (int k = 0; text[k] != '\0'; ++k) {
		int offset = _getOffset(sfont, (unsigned char)text[k]);
		if (offset < 0)
			width += sfont->letters[0]->width; // Use the width of the first letter as a fallback
		else
			width += sfont->letters[offset]->width;
	}

	return width;
}

int oslSFontDrawText(OSL_SFONT *sfont, int x, int y, char *text) {
	if (!sfont || !text) return x;

	int currentX = x;

	// Iterate through each character of the text
	for (int k = 0; text[k] != '\0'; ++k) {
		int offset = _getOffset(sfont, (unsigned char)text[k]);
		if (offset < 0)
			currentX += sfont->letters[0]->width; // Move by the width of the first letter as fallback
		else {
			oslDrawImageXY(sfont->letters[offset]->letter, currentX, y);
			currentX += sfont->letters[offset]->width;
		}
	}

	return currentX;
}
