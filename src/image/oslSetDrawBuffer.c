#include "oslib.h"

/*
	This function is particularly slow on Windows. Without auxiliary buffers, it performs the following operations:
	1. Saves the content of the current draw buffer to the associated image.
	2. Handles special cases for OSL_DEFAULT/OSL_SECONDARY_BUFFER.
	3. Writes the content of the new draw buffer to the GPU.
*/
void oslSetDrawBuffer(OSL_IMAGE *img) {
#ifdef PSP
	// Set the current draw buffer on the PSP
	osl_curBuf = img;
	sceGuDrawBuffer(img->pixelFormat, oslRemoveVramPrefixPtr(img->data), img->sysSizeX);
#else
	// Check if the new buffer is the same as the current one; if so, no need to proceed.
	if (img == osl_curBuf)
		return;

	// Temporarily disable 2D texturing for the transfer operation
	emuConfigure2DTransfer(1);

	// If the current or new buffer is not a special buffer, read the backbuffer data into the current image.
	if ((osl_curBuf != OSL_SECONDARY_BUFFER && osl_curBuf != OSL_DEFAULT_BUFFER) ||
	    (img != OSL_DEFAULT_BUFFER && img != OSL_SECONDARY_BUFFER)) {
		emuGlReadPixels(0, 272 - osl_curBuf->sizeY, osl_curBuf->sysSizeX, osl_curBuf->sizeY, GL_RGBA, 
		                osl_curBuf->pixelFormat, osl_curBuf->data);
	}

	// Handle OpenGL specifics for buffer switching
	if (img == OSL_SECONDARY_BUFFER || OSL_DEFAULT_BUFFER->data == OSL_SECONDARY_BUFFER->data) {
		glReadBuffer(GL_FRONT);
		glDrawBuffer(GL_FRONT);
	} else {
		glReadBuffer(GL_BACK);
		glDrawBuffer(GL_BACK);
	}

	// Draw the new image onto the backbuffer if it's not a special buffer
	if ((osl_curBuf != OSL_SECONDARY_BUFFER && osl_curBuf != OSL_DEFAULT_BUFFER) ||
	    (img != OSL_DEFAULT_BUFFER && img != OSL_SECONDARY_BUFFER)) {
		glRasterPos2i(0, 0);
		glPixelZoom(1, -1);
		glDrawPixels(img->sysSizeX, img->sizeY, GL_RGBA, emu_pixelPhysFormats[img->pixelFormat], img->data);
	}

	// Update the current draw buffer to the new one
	osl_curBuf = img;

	// Re-enable 2D texturing after the transfer
	emuConfigure2DTransfer(0);
#endif

	// Set the screen clipping to the new buffer's size
	oslSetScreenClipping(0, 0, img->sizeX, img->sizeY);
}
