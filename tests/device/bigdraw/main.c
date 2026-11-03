// Device test: what each draw call does with an image wider than 512 pixels
// (oslDrawImage samples at most 512 texels per axis). The image is drawn 100 px
// to the left, so texture columns 100..579 land on screen, 512..579 included.
// Then 1024 and 1100 px wide images check the 1024 width limit of oslDrawImageBig.
#include <pspkernel.h>
#include <stdio.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("bigdraw", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4 * 1024);

#define W 600
#define H 24

static FILE *f;

static u32 pattern(int x, int y) {
	return RGBA((x * 7) & 255, (x >> 5) * 13 & 255, (y * 10) & 255, 255) & 0xffffff;
}

static void check(const char *name, OSL_IMAGE *img, int big) {
	oslClearScreen(RGBA(255, 0, 255, 255));
	img->x = -100;
	img->y = 16;
	if (big) oslDrawImageBig(img);
	else oslDrawImage(img);
	oslSyncDrawing();
	sceKernelDcacheWritebackInvalidateAll();
	int below = 0, above = 0, wrapped = 0; // wrong pixels for texture columns < 512 and >= 512
	for (int y = 0; y < H; y++)
		for (int sx = 0; sx < 480; sx++) {
			int x = sx + 100;
			int bad = (oslGetImagePixel(OSL_DEFAULT_BUFFER, sx, 16 + y) & 0xffffff) != pattern(x, y);
			if (x < 512) below += bad;
			else {
				above += bad;
				wrapped += (oslGetImagePixel(OSL_DEFAULT_BUFFER, sx, 16 + y) & 0xffffff) == pattern(x - 512, y);
			}
		}
	fprintf(f, "%-15s columns 100-511: %5d/%d wrong, 512-579: %4d/%d wrong (%d show column x-512) %s\n", name, below,
	        412 * H, above, 68 * H, wrapped, below || (big && above) ? "FAIL" : "PASS");
}

// oslDrawImageBig of a w wide image, drawn so its last 480 columns land on screen.
// The docs give 1024 as its width limit: past it the draw should come out wrong.
static void checkWide(int w) {
	OSL_IMAGE *img = oslCreateImage(w, H, OSL_IN_RAM, OSL_PF_8888);
	if (!img) {
		fprintf(f, "oslCreateImage(%d, %d) -> NULL FAIL\n", w, H);
		return;
	}
	for (int y = 0; y < H; y++)
		for (int x = 0; x < w; x++)
			oslSetImagePixel(img, x, y, pattern(x, y) | 0xff000000);
	oslUncacheImage(img);
	int x0 = w - 480, bad = 0;
	oslStartDrawing();
	oslClearScreen(RGBA(255, 0, 255, 255));
	img->x = -x0;
	img->y = 16;
	oslDrawImageBig(img);
	oslSyncDrawing();
	sceKernelDcacheWritebackInvalidateAll();
	for (int y = 0; y < H; y++)
		for (int sx = 0; sx < 480; sx++)
			bad += (oslGetImagePixel(OSL_DEFAULT_BUFFER, sx, 16 + y) & 0xffffff) != pattern(sx + x0, y);
	oslEndDrawing();
	fprintf(f, "oslDrawImageBig %4d wide, columns %d-%d: %5d/%d wrong %s\n", w, x0, w - 1, bad, 480 * H,
	        (w <= 1024) == !bad ? "PASS" : "FAIL");
	oslDeleteImage(img);
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	oslSetBilinearFilter(0);
	oslSetAlpha(OSL_FX_NONE, 0);
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0);
	OSL_IMAGE *img = oslCreateImage(W, H, OSL_IN_RAM, OSL_PF_8888);
	fprintf(f, "oslCreateImage(%d, %d) -> %s, size %dx%d\n", W, H, img ? "image" : "NULL", img ? img->sizeX : 0, img ? img->sizeY : 0);
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++)
			oslSetImagePixel(img, x, y, pattern(x, y) | 0xff000000);
	oslUncacheImage(img);
	oslStartDrawing();
	check("oslDrawImage", img, 0);
	check("oslDrawImageBig", img, 1);
	oslEndDrawing();
	oslWriteImageFilePNG(OSL_DEFAULT_BUFFER, "screen.png", 0);
	checkWide(1024);
	checkWide(1100);
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
