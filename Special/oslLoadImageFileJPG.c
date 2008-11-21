#include "../oslib.h"
#include "jpeg/gba-jpeg.h"
#include "jpeg/gba-jpeg-decode.h"
//#include <jpeglib.h>

OSL_IMAGE *oslLoadImageFileJPG(char *filename, int location, int pixelFormat)
{
	OSL_IMAGE *img = NULL;
	const unsigned char *input, *input_free;
	//We only keep the location bits
	int imgLocation = location & OSL_LOCATION_MASK;
	VIRTUAL_FILE *f;
	
	//True color is mandatory for JPG!
	if (osl_pixelWidth[pixelFormat] > 8)		{
		//Using libjpeg:
		/*int width = 1600;
		int height = 1200;
		unsigned char *raw_image = NULL;
		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;
		JSAMPROW row_pointer[1];

		FILE *infile = fopen( filename, "rb" );
		unsigned long location = 0;
		int i = 0;
		
		if ( !infile )
			return img;

		cinfo.err = jpeg_std_error( &jerr );
		jpeg_create_decompress( &cinfo );
		jpeg_stdio_src( &cinfo, infile );
		jpeg_read_header( &cinfo, TRUE );
		width = cinfo.image_width;
		height = cinfo.image_height;
		jpeg_start_decompress( &cinfo );
		raw_image = (unsigned char*)malloc( cinfo.output_width*cinfo.output_height*cinfo.num_components );
		row_pointer[0] = (unsigned char *)malloc( cinfo.output_width*cinfo.num_components );
		while( cinfo.output_scanline < cinfo.image_height )
		{
			jpeg_read_scanlines( &cinfo, row_pointer, 1 );
			for( i=0; i<cinfo.image_width*cinfo.num_components;i++) 
				raw_image[location++] = row_pointer[0][i];
		}
		jpeg_finish_decompress( &cinfo );
		jpeg_destroy_decompress( &cinfo );
		free( row_pointer[0] );
		fclose( infile );

		img = oslCreateImage(width, height, imgLocation, pixelFormat);
		img->data = raw_image;*/

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

