#include "../oslib.h"
#include "jpeg/gba-jpeg.h"
#include "jpeg/gba-jpeg-decode.h"

OSL_IMAGE *oslLoadImageFileJPG(char *filename, int location, int pixelFormat)
{
	OSL_IMAGE *img = NULL;
	const unsigned char *input, *input_free;
	//We only keep the location bits
	int imgLocation = location & OSL_LOCATION_MASK;
	VIRTUAL_FILE *f;
	
	//True color is mandatory for JPG!
	if (osl_pixelWidth[pixelFormat] > 8)		{

		f = VirtualFileOpen((void*)filename, 0, VF_AUTO, VF_O_READ);

		if (f)			{
			input = (const unsigned char*)oslReadEntireFileToMemory(f, NULL);
			input_free = input;
			VirtualFileClose(f);

			if (input)			{
				int width, height;
				JPEG_Decoder decoder;

				JPEG_Decoder_ReadHeaders(&decoder, &input);
				width = decoder.frame.width;
				height = decoder.frame.height;

				img = oslCreateImage(width, height, imgLocation, pixelFormat);

				if (img)			{
					if (!JPEG_Decoder_ReadImage (&decoder, &input, img, img->realSizeX, img->realSizeY))			{
						oslDeleteImage(img);
						img = NULL;
					}
				}

				free((void*)input_free);

				if (oslImageLocationIsSwizzled(location))
					oslSwizzleImage(img);

				oslUncacheImage(img);
			}
		}
	}

	if (!img)
		oslHandleLoadNoFailError(filename);
	return img;
}

