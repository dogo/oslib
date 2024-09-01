#include "oslib.h"

// Retrieves the value of a pixel on an image - optimized
int oslGetImagePixel(OSL_IMAGE *img, unsigned int x, unsigned int y) {
    if (x >= img->sizeX || y >= img->sizeY) {
        return -1; // Return error for out-of-bounds coordinates
    }

    void *pPixel = oslGetUncachedPtr(oslGetSwizzledPixelAddr(img, x, y));

    switch (img->pixelFormat) {
        case OSL_PF_8888:
            return *(u32*)pPixel;

        case OSL_PF_5650:
        case OSL_PF_5551:
        case OSL_PF_4444:
            return *(u16*)pPixel;

        case OSL_PF_8BIT:
            return *(u8*)pPixel;

        case OSL_PF_4BIT:
            return (*(u8*)pPixel >> ((x & 1) * 4)) & 0xF; // Extract the correct 4-bit value

        default:
            return -1; // Return error for unsupported formats
    }
}
