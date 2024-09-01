#include "oslib.h"
#include "vfpu.h"

void oslDrawImage(OSL_IMAGE *img) {
    // Use the simple routine if no rotation is needed
    if (img->angle == 0 && img->centerX == 0 && img->centerY == 0) {
        oslDrawImageSimple(img);
        return;
    }

    // Avoid divisions by zero
    if (img->stretchX == 0 || img->stretchY == 0)
        return;

    oslSetTexture(img);

    float angleRadians = img->angle * (3.141592653f / 180.f);
    float cX = (-img->centerX * img->stretchX) / (float)(img->offsetX1 - img->offsetX0);
    float cY = (-img->centerY * img->stretchY) / (float)(img->offsetY1 - img->offsetY0);
    float tmpY = cY + img->stretchY;

    // Determine the U coefficient based on whether the image is mirrored
    float uCoeff = (img->offsetX1 >= img->offsetX0) ? 64.0f : -64.0f;
    
    // Calculate the X scaling coefficient (pixels per stripe)
    float xCoeff = uCoeff / ((float)oslAbs(img->offsetX1 - img->offsetX0) / img->stretchX);
    
    float uVal = img->offsetX0;
    float xVal = cX;

    while (uVal != img->offsetX1) {
        OSL_PRECISE_VERTEX *vertices = (OSL_PRECISE_VERTEX*)sceGuGetMemory(4 * sizeof(OSL_PRECISE_VERTEX));

        // Calculate vertex 0
        vertices[0].u = uVal;
        vertices[0].v = img->offsetY0;
        vertices[0].x = oslVfpu_cosf(angleRadians, xVal) - oslVfpu_sinf(angleRadians, cY) + img->x;
        vertices[0].y = oslVfpu_sinf(angleRadians, xVal) + oslVfpu_cosf(angleRadians, cY) + img->y;
        vertices[0].z = 0;

        // Calculate vertex 2
        vertices[2].u = uVal;
        vertices[2].v = img->offsetY1;
        vertices[2].x = oslVfpu_cosf(angleRadians, xVal) - oslVfpu_sinf(angleRadians, tmpY) + img->x;
        vertices[2].y = oslVfpu_sinf(angleRadians, xVal) + oslVfpu_cosf(angleRadians, tmpY) + img->y;
        vertices[2].z = 0;

        uVal += uCoeff;
        xVal += xCoeff;

        if (uVal > img->offsetX1) {
            xVal = cX + img->stretchX;
            uVal = img->offsetX1;
        }

        // Calculate vertex 1
        vertices[1].u = uVal;
        vertices[1].v = img->offsetY0;
        vertices[1].x = oslVfpu_cosf(angleRadians, xVal) - oslVfpu_sinf(angleRadians, cY) + img->x;
        vertices[1].y = oslVfpu_sinf(angleRadians, xVal) + oslVfpu_cosf(angleRadians, cY) + img->y;
        vertices[1].z = 0;

        // Calculate vertex 3
        vertices[3].u = uVal;
        vertices[3].v = img->offsetY1;
        vertices[3].x = oslVfpu_cosf(angleRadians, xVal) - oslVfpu_sinf(angleRadians, tmpY) + img->x;
        vertices[3].y = oslVfpu_sinf(angleRadians, xVal) + oslVfpu_cosf(angleRadians, tmpY) + img->y;
        vertices[3].z = 0;

        sceGuDrawArray(GU_TRIANGLE_STRIP, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 4, 0, vertices);
    }
}
