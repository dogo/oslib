#include "../oslib.h"

// Generic, includes support for all image types!
int oslWriteImageFile(OSL_IMAGE *img, const char* filename, int flags)
{
	// Check if the image is swizzled; writing swizzled images is not supported
	if (oslImageIsSwizzled(img))
		return 0;

	const char *ext = strrchr(filename, '.');
	if (!ext || ext == filename)
		return 0;

	char extension[10] = {0};
	size_t i = 0;
	while (i < sizeof(extension) - 1 && ext[i]) // Leave space for the null terminator
	{
		extension[i] = tolower((unsigned char) ext[i]);
		i++;
	}
#ifdef OSL_IMAGE_WRITER_PNG
	if (strcmp(extension, ".png") == 0)
		return oslWriteImageFilePNG(img, filename, flags);
#endif
	return 0;
}