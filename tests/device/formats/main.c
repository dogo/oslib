// Device test: an image converted to each pixel format and drawn 1:1 must show
// the source colours as the GE expands that format (n-bit channels widened by
// bit replication), and drawing it swizzled must give the same pixels.
#include <pspkernel.h>
#include <stdio.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("formats", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4 * 1024);

#define W 64 // two swizzle blocks wide (16 bytes x 8 rows each)
#define H 16
#define BG RGBA(255, 0, 255, 255)

static FILE *f;

// 16 grey-ish levels x 4 hues, opaque: at most 64 colours, so 8-bit palettes hold them exactly
static u32 source(int x, int y) {
	int v = (x & 15) * 17;
	switch (y & 3) {
	case 0: return RGBA(v, 0, 0, 255);
	case 1: return RGBA(0, v, 0, 255);
	case 2: return RGBA(0, 0, v, 255);
	default: return RGBA(v, v, 255 - v, 255);
	}
}

static int widen(int v, int bits) { // keep the top `bits` bits, widen them back by bit replication
	v >>= 8 - bits;
	return (v << (8 - bits)) | (v >> (2 * bits - 8));
}

static u32 expected(u32 c, int format) {
	int r = c & 0xff, g = (c >> 8) & 0xff, b = (c >> 16) & 0xff;
	switch (format) {
	case OSL_PF_5650: return RGBA(widen(r, 5), widen(g, 6), widen(b, 5), 255) & 0xffffff;
	case OSL_PF_5551: return RGBA(widen(r, 5), widen(g, 5), widen(b, 5), 255) & 0xffffff;
	case OSL_PF_4444: return RGBA(widen(r, 4), widen(g, 4), widen(b, 4), 255) & 0xffffff;
	default: return c & 0xffffff; // 8888 and palettes keep the colour
	}
}

static void check(const char *name, int format, int swizzle) {
	OSL_IMAGE *src = oslCreateImage(W, H, OSL_IN_RAM, OSL_PF_8888);
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++)
			oslSetImagePixel(src, x, y, source(x, y));
	oslUncacheImage(src);
	OSL_IMAGE *img = format == OSL_PF_8888 ? src : oslConvertImageTo(src, OSL_IN_RAM, format);
	if (swizzle) oslSwizzleImage(img);

	oslClearScreen(BG);
	oslDrawImageXY(img, 16, 16);
	oslSyncDrawing();
	sceKernelDcacheWritebackInvalidateAll();
	int bad = 0, fx = -1, fy = -1;
	u32 fgot = 0, fwant = 0;
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++) {
			u32 got = oslGetImagePixel(OSL_DEFAULT_BUFFER, 16 + x, 16 + y) & 0xffffff;
			u32 want = expected(source(x, y), format);
			if (got != want) {
				if (!bad++) {
					fx = x; fy = y; fgot = got; fwant = want;
				}
			}
		}
	oslDeleteImage(img);
	fprintf(f, "%-6s %-10s %4d/%d wrong", name, swizzle ? "swizzled" : "linear", bad, W * H);
	if (bad) fprintf(f, " (first %d,%d got %06lx want %06lx)", fx, fy, fgot, fwant);
	fprintf(f, " %s\n", bad ? "FAIL" : "PASS");
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	oslSetBilinearFilter(0);
	oslSetAlpha(OSL_FX_NONE, 0);
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0);
	oslStartDrawing();
	static const struct { const char *name; int format; } formats[] = {
		{ "8888", OSL_PF_8888 }, { "5650", OSL_PF_5650 }, { "5551", OSL_PF_5551 },
		{ "4444", OSL_PF_4444 }, { "8bit", OSL_PF_8BIT },
	};
	for (int i = 0; i < 5; i++)
		for (int s = 0; s < 2; s++)
			check(formats[i].name, formats[i].format, s);
	oslEndDrawing();
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
