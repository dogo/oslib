#include "oslib.h"

void oslResetImageProperties(OSL_IMAGE *img)
{
    if (!img) return;

    // Reset position
    img->x = 0;
    img->y = 0;

    // Reset offset
    img->offsetX0 = 0;
    img->offsetY0 = 0;
    img->offsetX1 = img->sizeX;
    img->offsetY1 = img->sizeY;

    // Reset rotation
    img->centerX = 0;
    img->centerY = 0;
    img->angle = 0;

    // Reset scaling
    img->stretchX = img->sizeX;
    img->stretchY = img->sizeY;
}
