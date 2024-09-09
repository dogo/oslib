#include "oslib.h"

// Helper function to enable/disable texturing
void toggleTexturing(int enable) {
    if (enable)
        oslEnableTexturing();
    else
        oslDisableTexturing();
}

OSL_COLOR oslBlendColors(OSL_COLOR color1, OSL_COLOR color2) {
    if (color2 == 0xffffffff) {  // No blending needed
        return color1;
    }

    if ((color2 & 0x00ffffff) == 0x00ffffff) {
        // No tinting, simple alpha blending
        return (color1 & 0xffffff) | (((color1 >> 24) * (color2 >> 24) >> 8) << 24);
    } else {
        return
            ((((u64)(color1 & 0xff000000) * (color2 & 0xff000000)) >> 32) & 0xff000000) |
            ((((u64)(color1 & 0xff0000) * (color2 & 0xff0000)) >> 24) & 0xff0000) |
            ((((color1 & 0xff00) * (color2 & 0xff00)) >> 16) & 0xff00) |
            (((color1 & 0xff) * (color2 & 0xff)) >> 8);
    }
}

void oslDrawLine(int x0, int y0, int x1, int y1, OSL_COLOR color) {
    OSL_LINE_VERTEX* vertices = (OSL_LINE_VERTEX*)sceGuGetMemory(2 * sizeof(OSL_LINE_VERTEX));
    color = oslBlendColors(color, osl_currentAlphaCoeff);

    vertices[0].color = color;
    vertices[0].x = x0;
    vertices[0].y = y0;
    vertices[0].z = 0;

    vertices[1].color = color;
    vertices[1].x = x1;
    vertices[1].y = y1;
    vertices[1].z = 0;

    int wasEnabled = osl_textureEnabled;
    toggleTexturing(0); // Disable texturing

    sceGuDrawArray(GU_LINES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
    sceKernelDcacheWritebackRange(vertices, 2 * sizeof(OSL_LINE_VERTEX));
    toggleTexturing(wasEnabled); // Restore previous state
}

void oslDrawRect(int x0, int y0, int x1, int y1, OSL_COLOR color) {
    OSL_LINE_VERTEX* vertices = (OSL_LINE_VERTEX*)sceGuGetMemory(8 * sizeof(OSL_LINE_VERTEX));
    color = oslBlendColors(color, osl_currentAlphaCoeff);

    // Ensure correct rectangle coordinates
    if (x1 < x0) {
        int temp = x0;
        x0 = x1;
        x1 = temp;
    }
    if (y1 < y0) {
        int temp = y0;
        y0 = y1;
        y1 = temp;
    }

    vertices[0].color = color;
    vertices[0].x = x0;
    vertices[0].y = y0;
    vertices[0].z = 0;

    vertices[1].color = color;
    vertices[1].x = x0;
    vertices[1].y = y1;
    vertices[1].z = 0;

    vertices[2].color = color;
    vertices[2].x = x1;
    vertices[2].y = y1;
    vertices[2].z = 0;

    vertices[3].color = color;
    vertices[3].x = x1;
    vertices[3].y = y0;
    vertices[3].z = 0;

    int wasEnabled = osl_textureEnabled;
    toggleTexturing(0); // Disable texturing

    sceGuDrawArray(GU_LINES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 8, 0, vertices);
    sceKernelDcacheWritebackRange(vertices, 8 * sizeof(OSL_LINE_VERTEX));
    toggleTexturing(wasEnabled); // Restore previous state
}

void oslDrawFillRect(int x0, int y0, int x1, int y1, OSL_COLOR color) {
    OSL_LINE_VERTEX* vertices = (OSL_LINE_VERTEX*)sceGuGetMemory(2 * sizeof(OSL_LINE_VERTEX));
    color = oslBlendColors(color, osl_currentAlphaCoeff);

    vertices[0].color = color;
    vertices[0].x = x0;
    vertices[0].y = y0;
    vertices[0].z = 0;

    vertices[1].color = color;
    vertices[1].x = x1;
    vertices[1].y = y1;
    vertices[1].z = 0;

    int wasEnabled = osl_textureEnabled;
    toggleTexturing(0); // Disable texturing

    sceGuDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
    sceKernelDcacheWritebackRange(vertices, 2 * sizeof(OSL_LINE_VERTEX));
    toggleTexturing(wasEnabled); // Restore previous state
}

void oslDrawGradientRect(int x0, int y0, int x1, int y1, OSL_COLOR colorTopLeft, OSL_COLOR colorTopRight, OSL_COLOR colorBottomLeft, OSL_COLOR colorBottomRight) {
    OSL_LINE_VERTEX* vertices = (OSL_LINE_VERTEX*)sceGuGetMemory(4 * sizeof(OSL_LINE_VERTEX));

    // Pre-blend colors
    colorTopLeft = oslBlendColors(colorTopLeft, osl_currentAlphaCoeff);
    colorTopRight = oslBlendColors(colorTopRight, osl_currentAlphaCoeff);
    colorBottomLeft = oslBlendColors(colorBottomLeft, osl_currentAlphaCoeff);
    colorBottomRight = oslBlendColors(colorBottomRight, osl_currentAlphaCoeff);

    vertices[0].color = colorTopLeft;
    vertices[0].x = x0;
    vertices[0].y = y0;
    vertices[0].z = 0;

    vertices[1].color = colorTopRight;
    vertices[1].x = x1;
    vertices[1].y = y0;
    vertices[1].z = 0;

    vertices[2].color = colorBottomLeft;
    vertices[2].x = x0;
    vertices[2].y = y1;
    vertices[2].z = 0;

    vertices[3].color = colorBottomRight;
    vertices[3].x = x1;
    vertices[3].y = y1;
    vertices[3].z = 0;

    int wasEnabled = osl_textureEnabled;
    toggleTexturing(0); // Disable texturing

    sceGuDrawArray(GU_TRIANGLE_STRIP, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 4, 0, vertices);
    sceKernelDcacheWritebackRange(vertices, 4 * sizeof(OSL_LINE_VERTEX));
    toggleTexturing(wasEnabled); // Restore previous state
}
