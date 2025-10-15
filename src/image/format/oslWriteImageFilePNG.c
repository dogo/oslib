#include "../../oslib.h"
#include "../../png.h"
#include "../../pngconf.h"
#include "../../zlibInterface.h"
#include "../../zlib.h"
#include "../../zconf.h"

void oslPngFlushFn(png_structp png_ptr);

void oslPngWriteFn(png_structp png_ptr, png_bytep data, png_size_t length) {
    VIRTUAL_FILE *f = (VIRTUAL_FILE*)png_get_io_ptr(png_ptr);
    VirtualFileWrite(data, length, 1, f);
}

// Take a screenshot and save it as PNG
int oslWriteImageFilePNG(OSL_IMAGE *img, const char* filename, int flags) {
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    VIRTUAL_FILE *f = NULL;
    u8* line = NULL;
    int width = img->offsetX1 - img->offsetX0;
    int height = img->offsetY1 - img->offsetY0;
    int r, g, b, a;
    int lbSuccess = 0;
    const int saveAlpha = flags & OSL_WRI_ALPHA;

    // Open the file for writing
    f = VirtualFileOpen((void*)filename, 0, VF_AUTO, VF_O_WRITE);
    if (!f) {
        printf("Error: Could not open file %s for writing.\n", filename);
        return 0;
    }

    // Initialize PNG write structure
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        printf("Error: Could not create PNG write struct.\n");
        goto cleanup;
    }

    // Initialize PNG info structure
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        printf("Error: Could not create PNG info struct.\n");
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        goto cleanup;
    }

    // Set error handling for PNG writing
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("Error: PNG writing encountered an error.\n");
        goto cleanup;
    }

    // Set up custom IO functions
    png_set_write_fn(png_ptr, f, oslPngWriteFn, oslPngFlushFn);

    // Set PNG header info
    png_set_IHDR(png_ptr, info_ptr, width, height, 8,
                 saveAlpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);

    // Allocate memory for one row of image data
    line = (u8*) malloc(width * (saveAlpha ? 4 : 3));
    if (!line) {
        printf("Error: Could not allocate memory for PNG row.\n");
        goto cleanup;
    }

    // Write image data row by row
    for (int y = 0; y < height; y++) {
        int i = 0;
        for (int x = 0; x < width; x++) {
            void* ptr = oslGetImagePixelAdr(img, (int)(x + img->offsetX0), (int)(y + img->offsetY0));
            int color = 0;

            switch (osl_pixelWidth[img->pixelFormat]) {
                case 32:
                    color = *(unsigned long*)ptr;
                    break;
                case 16:
                    color = *(unsigned short*)ptr;
                    break;
                case 8:
                    color = *(unsigned char*)ptr;
                    break;
                default:
					printf("Error: Unsupported pixel format.\n");
					break;
            }

            // Convert color to RGBA8888
            color = oslConvertColor(OSL_PF_8888, img->pixelFormat, color);
            r = color & 0xff;
            g = (color >> 8) & 0xff;
            b = (color >> 16) & 0xff;
            a = (color >> 24) & 0xff;

            // Ensure alpha is fully opaque for formats without alpha
            if (img->pixelFormat == OSL_PF_5650)
                a = 0xff;

            // Store the pixel values in the row buffer
            line[i++] = r;
            line[i++] = g;
            line[i++] = b;
            if (saveAlpha)
                line[i++] = a;
        }

        // Write the row to the PNG file
        png_write_row(png_ptr, line);
    }

    // Finalize PNG writing
    png_write_end(png_ptr, info_ptr);
    lbSuccess = 1;

cleanup:
    // Free resources and close the file
    if (line) free(line);
    if (png_ptr) png_destroy_write_struct(&png_ptr, &info_ptr);
    if (f) VirtualFileClose(f);

    return lbSuccess;
}
