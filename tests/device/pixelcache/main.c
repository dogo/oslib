// Device test: pixels written with oslSetImagePixel (uncached) must survive
// oslUncacheImage when the heap hands the image memory its previous owner left
// as dirty cache lines, and when the next heap block is written through the
// cache (a cache line shared with it would be written back over the pixels).
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("pixelcache", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static FILE *f;

static int lost(OSL_IMAGE *img) {
	int bad = 0;
	for (int y = 0; y < img->sizeY; y++)
		for (int x = 0; x < img->sizeX; x++)
			bad += oslGetImagePixel(img, x, y) != (int)0xff00ff00;
	return bad;
}

static void run(const char *name, int dirtyFirst) {
	void *prev = NULL;
	if (dirtyFirst) {
		// The block's previous owner: an image written through the cache, then deleted
		OSL_IMAGE *old = oslCreateImage(32, 24, OSL_IN_RAM | OSL_UNSWIZZLED, OSL_PF_8888);
		memset(old->data, 0, old->totalSize);
		prev = old->data;
		oslDeleteImage(old);
	}
	OSL_IMAGE *img = oslCreateImage(32, 24, OSL_IN_RAM | OSL_UNSWIZZLED, OSL_PF_8888);
	if (dirtyFirst && img->data != prev) {
		fprintf(f, "%-28s heap did not reuse the block (freed %p, data %p) FAIL\n", name, prev, img->data);
		oslDeleteImage(img);
		return;
	}
	// 32x24x4 = 3 KB: whole cache lines only if the data starts on one
	u8 *next = malloc(96);
	for (int y = 0; y < 24; y++)
		for (int x = 0; x < 32; x++) {
			oslSetImagePixel(img, x, y, 0xff00ff00);
			memset(next, y, 96); // what the JPEG loader's scanline buffer does
		}
	oslUncacheImage(img);
	free(next);
	int bad = lost(img);
	int aligned = ((u32)img->data & 63) == 0;
	fprintf(f, "%-28s %3d/768 pixels lost, data %s %s\n", name, bad,
		aligned ? "on a cache line" : "shares a cache line", bad || !aligned ? "FAIL" : "PASS");
	oslDeleteImage(img);
}

int main() {
	oslInit(0);
	f = fopen("result.txt", "w");
	run("fresh image", 0);
	run("image reusing a dirty block", 1);
	// Keep small allocations alive between images so later ones land at other alignments
	for (int i = 0; i < 4; i++) {
		void *pad = malloc(16 + 16 * i); // leaked on purpose: shifts the next allocations
		(void)pad;
		run("image after a small block", 0);
	}
	fclose(f);
	sceKernelExitGame();
	return 0;
}
