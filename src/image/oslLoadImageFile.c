#include "../oslib.h"

// Generic, includes support for all image types!
OSL_IMAGE *oslLoadImageFile(char *filename, int location, int pixelFormat) {
	if (!filename) {
		return NULL;
	}

	const char *ext = strrchr(filename, '.');
	if (!ext || ext == filename) {
		return NULL;
	}

	char extension[10] = {0};
	int i = 0;
	while (i < sizeof(extension) - 1 && ext[i]) // Leave space for the null terminator
	{
		extension[i] = tolower((unsigned char) ext[i]);
		i++;
	}

	// Load the appropriate image type based on the extension
#ifdef OSL_IMAGE_LOADER_PNG
    if (strcmp(extension, ".png") == 0)
		return oslLoadImageFilePNG(filename, location, pixelFormat);
#endif
#ifdef OSL_IMAGE_LOADER_JPEG
    if (strcmp(extension, ".jpg") == 0)
		return oslLoadImageFileJPG(filename, location, pixelFormat);
#endif
#ifdef OSL_IMAGE_LOADER_GIF
    if (strcmp(extension, ".gif") == 0)
		return oslLoadImageFileGIF(filename, location, pixelFormat);
#endif

	return NULL;
}