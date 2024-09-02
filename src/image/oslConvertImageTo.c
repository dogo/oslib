#include "oslib.h"

extern int oslFindColorInPalette(OSL_PALETTE *palette, int paletteSize, OSL_COLOR targetColor) {
    for (int i = 0; i < paletteSize; i++) {
        // Convert the current palette entry to a 32-bit value
        u32 currentColor = oslConvertColor(OSL_PF_8888, palette->pixelFormat, oslGetPaletteColor(palette, i));
        if (currentColor == targetColor) {
            return i;
        }
    }
    return -1;
}

OSL_IMAGE *oslConvertImageTo(OSL_IMAGE *originalImage, int newLocation, int newFormat) {
    int paletteSize = 1 << osl_paletteSizes[newFormat];
    int width = originalImage->sizeX, height = originalImage->sizeY;
    OSL_IMAGE *newImage;
    int paletteCount = 0;

    // Create a new image with the specified location and format
    newImage = oslCreateImage(width, height, newLocation, newFormat);
    if (!newImage) {
        return NULL; // Return NULL if image creation failed
    }

    // If the new format uses a palette, create one
    if (osl_pixelWidth[newFormat] <= 8) {
        newImage->palette = oslCreatePalette(paletteSize, OSL_PF_8888);
    }

    // Initialize the new image data to zero
    memset(newImage->data, 0, newImage->totalSize);

    // Iterate over each pixel in the original image
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            u32 pixel;

            // Paletted mode: need to handle palette indexing
            if (osl_pixelWidth[newFormat] <= 8) {
                u32 *paletteData = (u32*)newImage->palette->data;

                // Convert the pixel color to 32-bit format
                pixel = oslConvertColorEx(originalImage->palette, OSL_PF_8888, originalImage->pixelFormat, oslGetImagePixel(originalImage, x, y));

                // Check if the color already exists in the palette
                int colorIndex = oslFindColorInPalette(newImage->palette, paletteCount, pixel);

                if (colorIndex < 0) {
                    // If the color is not found, add it to the palette if there is space
                    if (paletteCount < paletteSize) {
                        paletteData[paletteCount] = pixel;
                        colorIndex = paletteCount++;
                    } else {
                        // Not enough colors in the palette; fallback to the first color
                        colorIndex = 0;
                    }
                }

                // Set the pixel color in the new image using the palette index
                oslSetImagePixel(newImage, x, y, colorIndex);
            } else {
                // True color mode: directly convert and set the pixel color
                pixel = oslConvertColorEx(originalImage->palette, newFormat, originalImage->pixelFormat, oslGetImagePixel(originalImage, x, y));
                oslSetImagePixel(newImage, x, y, pixel);
            }
        }
    }

    // Clean up the original image
    oslDeleteImage(originalImage);
    oslUncacheImage(newImage);

    return newImage;
}
