#include "oslib.h"

void oslUnswizzleTexture(u8* out, const u8* in, unsigned int width, unsigned int height) {
    unsigned int blockX, blockY;
    unsigned int rowOffset;

    unsigned int widthBlocks = width / 16;
    unsigned int heightBlocks = height / 8;

    unsigned int dstPitch = (width - 16) / 4;
    unsigned int dstRowSize = width * 8;

    const u32* src = (const u32*)in;
    u8* destRow = out;

#ifndef PSP
    // For non-PSP platforms, simply copy the data as is.
    memcpy(out, in, width * height);
    return;
#endif

    for (blockY = 0; blockY < heightBlocks; ++blockY) {
        u8* destBlock = destRow;
        for (blockX = 0; blockX < widthBlocks; ++blockX) {
            u32* dest = (u32*)destBlock;
            for (rowOffset = 0; rowOffset < 8; ++rowOffset) {
                *(dest++) = *(src++);
                *(dest++) = *(src++);
                *(dest++) = *(src++);
                *(dest++) = *(src++);
                dest += dstPitch;
            }
            destBlock += 16;
        }
        destRow += dstRowSize;
    }
}

void oslUnswizzleImage(OSL_IMAGE *img) {
    // Check if the image is already unswizzled
    if (!oslImageIsSwizzled(img))
        return;

    // Allocate memory for the temporary buffer
    void *tempBuffer = malloc(img->totalSize);
    if (!tempBuffer) {
        // Handle memory allocation failure
        return;
    }

    // Copy image data to the temporary buffer
    memcpy(tempBuffer, img->data, img->totalSize);

    // Unswizzle the texture
    oslUnswizzleTexture((u8*)img->data, (u8*)tempBuffer, 
        (img->realSizeX * osl_pixelWidth[img->pixelFormat]) >> 3, 
        img->realSizeY);

    // Free the temporary buffer
    free(tempBuffer);

    // Update the image metadata
    oslUncacheImageData(img);
    oslImageIsSwizzledSet(img, 0);
}
