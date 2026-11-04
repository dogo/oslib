// Drawing results that came out wrong: tile maps whose tileset has a tile count per line
// that isn't a power of 2, oslScaleImageCreate's alpha and its width-only path, and
// converting an indexed image to true colour.
#include <pspkernel.h>
#include <stdio.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("drawfix", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static FILE *f;

static u32 screen(int x, int y) {
	return oslGetImagePixel(OSL_DEFAULT_BUFFER, x, y) & 0xffffff;
}

// A 48x32 tileset: 3 tiles of 16x16 per line, 2 lines, each tile its own colour
static const u32 tileColour[6] = { 0x0000ff, 0x00ff00, 0xff0000, 0xffff00, 0xff00ff, 0x00ffff };

static void map(const char *name, int format) {
	OSL_IMAGE *tiles = oslCreateImage(48, 32, OSL_IN_VRAM, OSL_PF_8888);
	for (int y = 0; y < 32; y++)
		for (int x = 0; x < 48; x++)
			oslSetImagePixel(tiles, x, y, tileColour[(y / 16) * 3 + x / 16] | 0xff000000);
	oslUncacheImage(tiles);
	static u16 data[6] = { 0, 1, 2, 3, 4, 5 };
	OSL_MAP *m = oslCreateMap(tiles, data, 16, 16, 6, 1, format);
	m->flags = 0; // tile 0 is opaque too
	m->drawSizeX = 6;
	m->drawSizeY = 1;
	oslClearScreen(RGBA(0, 0, 0, 255));
	oslDrawMap(m);
	oslSyncDrawing();
	sceKernelDcacheWritebackInvalidateAll();
	int bad = 0;
	for (int y = 0; y < 16; y++)
		for (int x = 0; x < 96; x++)
			bad += screen(x, y) != tileColour[x / 16];
	oslDeleteMap(m);
	oslDeleteImage(tiles);
	fprintf(f, "%-26s %4d/1536 wrong colour %s\n", name, bad, bad ? "FAIL" : "PASS");
}

static OSL_IMAGE *solid(int w, int h, u32 colour) {
	OSL_IMAGE *img = oslCreateImage(w, h, OSL_IN_RAM, OSL_PF_8888);
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
			oslSetImagePixel(img, x, y, colour);
	return img;
}

// A uniform image stays uniform, alpha included
static void scale(const char *name, int w, int h, int nw, int nh, u32 colour) {
	OSL_IMAGE *img = solid(w, h, colour);
	OSL_IMAGE *big = oslScaleImageCreate(img, OSL_IN_RAM, nw, nh, OSL_PF_8888);
	int bad = 0;
	u32 first = 0;
	for (int y = 0; big && y < nh; y++)
		for (int x = 0; x < nw; x++) {
			u32 p = oslGetImagePixel(big, x, y);
			if (p != colour && !bad++) first = p;
		}
	fprintf(f, "%-26s %2d/%d wrong", name, big ? bad : nw * nh, nw * nh);
	if (bad) fprintf(f, " (first %08lx want %08lx)", first, colour);
	fprintf(f, " %s\n", big && !bad ? "PASS" : "FAIL");
	if (big) oslDeleteImage(big);
	oslDeleteImage(img);
}

// An 8-bit image converted to 8888 takes its colours from the palette
static void convertIndexed(void) {
	OSL_IMAGE *img = oslCreateImage(4, 4, OSL_IN_RAM, OSL_PF_8BIT);
	img->palette = oslCreatePalette(2, OSL_PF_8888);
	((u32 *)img->palette->data)[0] = RGBA(0, 255, 0, 255);
	((u32 *)img->palette->data)[1] = RGBA(255, 0, 0, 255);
	for (int y = 0; y < 4; y++)
		for (int x = 0; x < 4; x++)
			oslSetImagePixel(img, x, y, (x + y) & 1);
	img = oslConvertImageTo(img, OSL_IN_RAM, OSL_PF_8888);
	int bad = 0;
	for (int y = 0; img && y < 4; y++)
		for (int x = 0; x < 4; x++)
			bad += oslGetImagePixel(img, x, y) != (u32)((x + y) & 1 ? RGBA(255, 0, 0, 255) : RGBA(0, 255, 0, 255));
	fprintf(f, "%-26s %2d/16 wrong %s\n", "8bit to 8888 conversion", img ? bad : 16, img && !bad ? "PASS" : "FAIL");
	if (img) oslDeleteImage(img);
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0);

	oslStartDrawing();
	oslSetAlpha(OSL_FX_NONE, 0);
	map("map 3 tiles per line", OSL_MF_U16);
	map("map 3 tiles per line, GBA", OSL_MF_U16_GBA);
	oslEndDrawing();

	scale("scale 2x2 to 4x4, alpha 64", 2, 2, 4, 4, RGBA(10, 20, 30, 64));
	scale("scale 2x2 to 4x2", 2, 2, 4, 2, RGBA(10, 20, 30, 255));
	convertIndexed();

	fprintf(f, "DONE\n");
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
