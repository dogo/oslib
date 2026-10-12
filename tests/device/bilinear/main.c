// Device test: an unscaled (1:1) image must look identical to its source even with the
// bilinear filter on. The GE's bilinear sampling of a 1:1 sprite is not exact (it depends on
// the size and even the texture address), so these came out blurred. Scaled draws must still filter.
#include <pspkernel.h>
#include <stdio.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("bilinear", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4 * 1024);

static FILE *f;

static u32 pattern(int x, int y) {
	return RGBA((x * 37) & 255, (y * 53) & 255, ((x ^ y) * 29) & 255, 255) & 0xffffff;
}

static u32 screen(int x, int y) {
	return oslGetImagePixel(OSL_DEFAULT_BUFFER, 16 + x, 16 + y) & 0xffffff;
}

static OSL_IMAGE *makeImage(int w, int h) {
	OSL_IMAGE *img = oslCreateImage(w, h, OSL_IN_VRAM, OSL_PF_8888);
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
			oslSetImagePixel(img, x, y, pattern(x, y) | 0xff000000);
	oslUncacheImage(img);
	return img;
}

static void draw(OSL_IMAGE *img, int bilinear) {
	oslSetBilinearFilter(bilinear);
	oslClearScreen(0);
	oslDrawImageXY(img, 16, 16);
	oslSyncDrawing();
	sceKernelDcacheWritebackInvalidateAll();
}

// Pixels of a w x h 1:1 draw that differ from the (optionally mirrored) source.
static int unscaledMismatches(int w, int h, int bilinear, int mirror) {
	OSL_IMAGE *img = makeImage(w, h);
	if (mirror)
		oslMirrorImageH(img);
	draw(img, bilinear);
	int bad = 0;
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
			bad += screen(x, y) != pattern(mirror ? w - 1 - x : x, y);
	oslDeleteImage(img);
	return bad;
}

static void unscaled(const char *name, int w, int h, int bilinear, int mirror) {
	int bad = unscaledMismatches(w, h, bilinear, mirror);
	fprintf(f, "%-28s %3dx%-3d %4d/%-4d differ %s\n", name, w, h, bad, w * h, bad ? "FAIL" : "PASS");
}

// Draws that genuinely resample must still be filtered: some pixels are blends, not copies of a texel.
static void stillFilters(const char *name, float u0, float v0, float u1, float v1, int w, int h) {
	OSL_IMAGE *img = makeImage(32, 32);
	img->offsetX0 = u0;
	img->offsetY0 = v0;
	img->offsetX1 = u1;
	img->offsetY1 = v1;
	img->stretchX = w;
	img->stretchY = h;
	draw(img, 1);
	int blended = 0;
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++) {
			u32 p = screen(x, y);
			int copy = 0;
			for (int ty = 0; ty < 32 && !copy; ty++)
				for (int tx = 0; tx < 32 && !copy; tx++)
					copy = p == pattern(tx, ty);
			blended += !copy;
		}
	oslDeleteImage(img);
	fprintf(f, "%-28s %3dx%-3d %4d/%-4d blended %s\n", name, w, h, blended, w * h, blended ? "PASS" : "FAIL");
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	f = fopen("ms0:/PSP/GAME/bilinear/result.txt", "w");
	oslStartDrawing();
	oslSetAlpha(OSL_FX_NONE, 0);

	unscaled("nearest (control)", 30, 20, 0, 0);
	unscaled("bilinear 1:1", 30, 20, 1, 0);
	unscaled("bilinear 1:1", 64, 48, 1, 0);
	unscaled("bilinear 1:1", 100, 50, 1, 0);
	unscaled("bilinear 1:1 mirrored", 30, 20, 1, 1);
	stillFilters("bilinear 2x stretch", 0, 0, 30, 20, 60, 40);
	stillFilters("bilinear half-texel shift", 0.5f, 0.5f, 30.5f, 20.5f, 30, 20);

	oslEndDrawing();
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
