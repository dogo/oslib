#include "oslib.h"

// Draws a pixel on an image - slow
void oslSetImagePixel(OSL_IMAGE *img, unsigned int x, unsigned int y, int pixelValue) {
    // Early exit if the pixel is out of bounds (unsigned check covers non-negative range)
    if (x >= img->sizeX || y >= img->sizeY) {
        return;
    }

    // Get the pointer to the pixel's address in the image, avoiding cache issues
    void *pPixel = oslGetUncachedPtr(oslGetSwizzledPixelAddr(img, x, y));

    // Set the pixel value based on the image's pixel format
    switch (img->pixelFormat) {
        case OSL_PF_8888:
            *(u32*)pPixel = (u32)pixelValue;
            break;

        case OSL_PF_5650:
        case OSL_PF_5551:
        case OSL_PF_4444:
            *(u16*)pPixel = (u16)pixelValue;
            break;

        case OSL_PF_8BIT:
            *(u8*)pPixel = (u8)pixelValue;
            break;

        case OSL_PF_4BIT: {
			*(u8*)pPixel &= ~(15 << ((x & 1) << 2));
			*(u8*)pPixel |= (pixelValue & 15) << ((x & 1) << 2);
            break;
        }
    }
}
