#include "oslib.h"

bool oslMoveImageTo(OSL_IMAGE *img, int newLocation) {
    // On PC, this function doesn't manage image relocation, so we return success.
#ifndef PSP
    return 1;
#endif

    // If the image is already in the desired location, no need to move it.
    if (img->location == newLocation) {
        return 1;
    }

    // Keep the old image data to copy to the new location.
    OSL_IMAGE oldImage = *img;

    // Prevent automatic freeing of memory during reallocation by setting data to NULL.
    img->data = NULL;

    // Allocate new data for the image at the new location.
    if (!oslAllocImageData(img, newLocation)) {
        // If allocation fails, restore the original data pointer and return failure.
        img->data = oldImage.data;
        return 0;
    }

    // Copy the old image data to the new memory location.
    memcpy(img->data, oldImage.data, img->totalSize);

    // Free the old image data since it has been copied successfully.
    oslFreeImageData(&oldImage);

    // Update image caching after moving.
    oslUncacheImageData(img);

    return 1;
}
