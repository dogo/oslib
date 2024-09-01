#include "oslib.h"

/*
    Special PC Functions
    Allows access to an image selected as a draw buffer or texture
*/
void oslLockImage(OSL_IMAGE *img)
{
    // Check if the image is the current draw buffer
    if (img == osl_curBuf) {
        // Synchronize drawing before accessing the draw buffer
        oslSyncDrawing();
        
        #ifndef PSP
            // On non-PSP platforms, copy the contents of the draw buffer to the image
            int textureEnabled = glIsEnabled(GL_TEXTURE_2D);
            int blendingEnabled = glIsEnabled(GL_BLEND);

            // Temporarily disable 2D texturing and blending
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);

            // Read the pixels from the back buffer into the image data
            emuGlReadPixels(
                0, 
                272 - osl_curBuf->sizeY, 
                osl_curBuf->sysSizeX, 
                osl_curBuf->sizeY, 
                GL_RGBA, 
                osl_curBuf->pixelFormat, 
                osl_curBuf->data
            );

            // Re-enable texturing and blending if they were previously enabled
            if (textureEnabled) {
                glEnable(GL_TEXTURE_2D);
            }
            if (blendingEnabled) {
                glEnable(GL_BLEND);
            }
        #endif
    }
}

// Call this function once the image operations are complete
void oslUnlockImage(OSL_IMAGE *img)
{
    // Uncache the image if it is stored in RAM
    oslUncacheImage(img);

    // Check if the image is the current texture
    if (img->data == osl_curTexture) {
        osl_curTexture = NULL;
    }

    #ifndef PSP
        // On non-PSP platforms, copy the modified image data back to the framebuffer
        if (img == osl_curBuf) {
            int textureEnabled = glIsEnabled(GL_TEXTURE_2D);
            int blendingEnabled = glIsEnabled(GL_BLEND);

            // Temporarily disable 2D texturing and blending
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);

            // Write the image data back to the framebuffer
            glRasterPos2i(0, 0);
            glPixelZoom(1, -1);
            glDrawPixels(
                img->sysSizeX, 
                img->sizeY, 
                GL_RGBA, 
                emu_pixelPhysFormats[img->pixelFormat], 
                img->data
            );

            // Re-enable texturing and blending if they were previously enabled
            if (textureEnabled) {
                glEnable(GL_TEXTURE_2D);
            }
            if (blendingEnabled) {
                glEnable(GL_BLEND);
            }
        }
    #endif
}
