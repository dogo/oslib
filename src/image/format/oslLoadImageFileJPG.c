#include "../../oslib.h"
#include <jpeglib.h>
#include <stdio.h>

OSL_IMAGE *oslLoadImageFileJPG(char *filename, int location, int pixelFormat) {
    if (!filename) {
        return NULL;
    }

    OSL_IMAGE *img = NULL;
    const unsigned char *input = NULL, *input_free = NULL;
    int input_size = 0;

    // True color is mandatory for JPG!
    if (osl_pixelWidth[pixelFormat] <= 8) {
        return NULL;
    }

    // We only keep the location bits
    int imgLocation = location & OSL_LOCATION_MASK;

    VIRTUAL_FILE *f = VirtualFileOpen((void*)filename, 0, VF_AUTO, VF_O_READ);
    if (!f) {
        return NULL;
    }

    input = (const unsigned char *)oslReadEntireFileToMemory(f, &input_size);
    input_free = input;
    VirtualFileClose(f);

    if (!input) {
        return NULL;
    }

    FILE *infile = fmemopen((void *)input, input_size, "rb");
    if (!infile) {
        free((void*)input_free);
        return NULL;
    }

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);

    // Create image structure
    int width = cinfo.image_width;
    int height = cinfo.image_height;
    img = oslCreateImage(width, height, imgLocation, pixelFormat);

    if (img) {
        jpeg_start_decompress(&cinfo);
        JSAMPROW row_pointer[1];
        row_pointer[0] = (unsigned char *)malloc(cinfo.output_width * cinfo.num_components);

        if (row_pointer[0]) {
            int x;
            while (cinfo.output_scanline < cinfo.image_height) {
                jpeg_read_scanlines(&cinfo, row_pointer, 1);
                for (x = 0; x < cinfo.image_width; x++) {
                    unsigned int pixelValue = row_pointer[0][x * cinfo.num_components] |
                                              (row_pointer[0][x * cinfo.num_components + 1] << 8) |
                                              (row_pointer[0][x * cinfo.num_components + 2] << 16) |
                                              (0xFF << 24); // Alpha channel
                    oslSetImagePixel(img, x, cinfo.output_scanline - 1,
                                     oslConvertColor(pixelFormat, OSL_PF_8888, pixelValue));
                }
            }
            free(row_pointer[0]);
            jpeg_finish_decompress(&cinfo);
        } else {
            oslDeleteImage(img);
            img = NULL;
        }
    }

    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    free((void*)input_free);

    // Post-processing steps
    if (img != NULL && oslImageLocationIsSwizzled(location)) {
        oslSwizzleImage(img);
    }
    oslUncacheImage(img);

    if (!img) {
        oslHandleLoadNoFailError(filename);
    }

    return img;
}
