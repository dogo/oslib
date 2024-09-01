#include "oslib.h"

// Swizzle a texture to optimize memory access patterns on the PSP
void oslSwizzleTexture(u8* out, const u8* in, unsigned int width, unsigned int height) {
    unsigned int blockx, blocky, j;
    unsigned int width_blocks = width / 16;
    unsigned int height_blocks = height / 8;
    unsigned int src_pitch = (width - 16) / 4;
    unsigned int src_row = width * 8;
    
    const u8* ysrc = in;
    u32* dst = (u32*)out;

#ifndef PSP
    // On non-PSP platforms, simply copy the input to output
    memcpy(out, in, width * height);
    return;
#endif

    // Swizzle the texture
    for (blocky = 0; blocky < height_blocks; ++blocky) {
        const u8* xsrc = ysrc;
        for (blockx = 0; blockx < width_blocks; ++blockx) {
            const u32* src = (const u32*)xsrc;
            for (j = 0; j < 8; ++j) {
                for (int i = 0; i < 4; ++i) {
                    *(dst++) = *(src++);
                }
                src += src_pitch;
            }
            xsrc += 16;
        }
        ysrc += src_row;
    }
}

// Get the address of a swizzled pixel
void* oslGetSwizzledPixelAddr(OSL_IMAGE* img, unsigned int x, unsigned int y) {
    if (!oslImageIsSwizzled(img)) {
        return oslGetImagePixelAdr(img, x, y);
    }

    // Calculate pixel address for swizzled images
    unsigned int pixelWidth = osl_pixelWidth[img->pixelFormat];
    x = (x * pixelWidth) >> 3;
    unsigned int width = (img->realSizeX * pixelWidth) >> 3;
    unsigned int rowblocks = width / 16;

    unsigned int blockx = x / 16;
    unsigned int blocky = y / 8;

    unsigned int block_index = blockx + (blocky * rowblocks);
    unsigned int block_address = block_index * 16 * 8;

    x = x - (blockx * 16);
    y = y - (blocky * 8);

    return (void*)((u8*)img->data + block_address + x + (y * 16));
}

// Swizzle an entire image
void oslSwizzleImage(OSL_IMAGE* img) {
    // Check if the image is already swizzled
    if (oslImageIsSwizzled(img)) {
        return;
    }

    // Allocate a temporary block of memory for swizzling
    void* block = malloc(img->totalSize);
    if (block) {
        memcpy(block, img->data, img->totalSize);
        oslSwizzleTexture((u8*)img->data, (u8*)block, (img->realSizeX * osl_pixelWidth[img->pixelFormat]) >> 3, img->realSizeY);
        free(block);

        oslUncacheImageData(img);
        oslImageIsSwizzledSet(img, 1);
    } else {
        oslFatalError("oslSwizzleImage: Memory allocation failed.");
    }
}

// Swizzle an image to another image buffer
void oslSwizzleImageTo(OSL_IMAGE* imgDst, OSL_IMAGE* imgSrc) {
    if (imgDst == imgSrc) {
        oslSwizzleImage(imgDst);
        return;
    }

    // Validate that both images have the same pixel format and size
    if (imgSrc->pixelFormat != imgDst->pixelFormat || imgSrc->totalSize != imgDst->totalSize) {
        oslFatalError("oslSwizzleImageTo: Both images must have the same size and pixel format!");
        return;
    }

    oslSwizzleTexture((u8*)imgDst->data, (u8*)imgSrc->data, (imgSrc->realSizeX * osl_pixelWidth[imgSrc->pixelFormat]) >> 3, imgSrc->realSizeY);
    oslUncacheImageData(imgDst);
    oslImageIsSwizzledSet(imgDst, 1);
}
