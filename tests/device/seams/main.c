// Device test: draws that oslib splits into pieces must not show seams where the pieces meet.
//  - oslDrawImageSimple strips images wider than 64 px (OSL_SLICE_SIZE) into separate quads
//  - oslDrawImageBig binds 512 px texture windows and draws 64-texel quads
//  - oslDrawMap draws a row of tiles from one tileset as adjacent sprites
#include <pspkernel.h>
#include <stdio.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("seams", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4 * 1024);

#define BG 0xff00ff // clear colour, never produced by the solid images
#define OX 16       // where images are drawn
#define OY 16

static FILE *f;

static u32 pattern(int x, int y) {
	return RGBA((x * 37) & 255, (y * 53) & 255, ((x ^ y) * 29) & 255, 255) & 0xffffff;
}

static u32 screen(int x, int y) {
	return oslGetImagePixel(OSL_DEFAULT_BUFFER, x, y) & 0xffffff;
}

static OSL_IMAGE *makeImage(int w, int h, int solid) {
	OSL_IMAGE *img = oslCreateImage(w, h, OSL_IN_VRAM, OSL_PF_8888);
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
			oslSetImagePixel(img, x, y, (solid ? 0xffffff : pattern(x, y)) | 0xff000000);
	oslUncacheImage(img);
	return img;
}

static void begin(int bilinear) {
	oslSetBilinearFilter(bilinear);
	oslClearScreen(RGBA(255, 0, 255, 255));
}

static void end(void) {
	oslSyncDrawing();
	sceKernelDcacheWritebackInvalidateAll();
}

static void report(const char *name, int bad, int total, const char *what) {
	fprintf(f, "%-34s %5d/%-5d %s %s\n", name, bad, total, what, bad ? "FAIL" : "PASS");
}

// Strip blit at 1:1 must reproduce the source exactly.
static void stripExact(void) {
	OSL_IMAGE *img = makeImage(200, 20, 0);
	begin(0);
	oslDrawImageXY(img, OX, OY);
	end();
	int bad = 0;
	for (int y = 0; y < 20; y++)
		for (int x = 0; x < 200; x++)
			bad += screen(OX + x, OY + y) != pattern(x, y);
	oslDeleteImage(img);
	report("strip 1:1 nearest vs source", bad, 4000, "differ");
}

// Scaled strip blit vs the same image drawn as one sprite (auto-strip off). Differences are
// counted, and separately those within 1 px of a strip boundary (where a seam would be).
// maxBad: on fractional zooms the GE computes each sprite's texel step with limited precision,
// so pieces can't match one big sprite exactly; a few % stray to the neighbouring texel.
static void stripVsSprite(const char *name, int bilinear, int w, int h, int maxBad) {
	OSL_IMAGE *img = makeImage(200, 20, 0);
	static u32 ref[480 * 64];
	img->stretchX = w;
	img->stretchY = h;
	oslImageSetAutoStrip(img, 0);
	begin(bilinear);
	oslDrawImageXY(img, OX, OY);
	end();
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
			ref[y * w + x] = screen(OX + x, OY + y);
	oslImageSetAutoStrip(img, 1);
	begin(bilinear);
	oslDrawImageXY(img, OX, OY);
	end();
	float zoom = (float)w / 200;
	int bad = 0, atSeam = 0;
	for (int y = 0; y < h; y++)
		for (int x = 0; x < w; x++)
			if (screen(OX + x, OY + y) != ref[y * w + x]) {
				bad++;
				for (int u = 64; u < 200; u += 64) {
					float bx = u * zoom;
					if (x >= bx - 1.5f && x <= bx + 1.5f) {
						atSeam++;
						break;
					}
				}
			}
	oslDeleteImage(img);
	char what[48];
	snprintf(what, sizeof(what), "differ (%d at seams)", atSeam);
	fprintf(f, "%-34s %5d/%-5d %s %s\n", name, bad, w * h, what, bad > maxBad ? "FAIL" : "PASS");
}

// A solid white image must cover its whole destination; count interior pixels that are not white.
static int gaps(int x0, int y0, int w, int h) {
	int bad = 0;
	for (int y = 1; y < h - 1; y++)
		for (int x = 1; x < w - 1; x++)
			bad += screen(x0 + x, y0 + y) != 0xffffff;
	return bad;
}

static void stripGaps(const char *name, int bilinear, int w, int h) {
	OSL_IMAGE *img = makeImage(200, 20, 1);
	img->stretchX = w;
	img->stretchY = h;
	begin(bilinear);
	oslDrawImageXY(img, OX, OY);
	end();
	report(name, gaps(OX, OY, w, h), (w - 2) * (h - 2), "not white");
	oslDeleteImage(img);
}

// >512 px: oslDrawImageBig at 1:1, shifted left so the 512 boundary (texel 512 -> screen 412) is visible.
static void bigExact(void) {
	OSL_IMAGE *img = makeImage(600, 20, 0);
	img->x = -100;
	img->y = OY;
	begin(0);
	oslDrawImageBig(img);
	end();
	int bad = 0;
	for (int y = 0; y < 20; y++)
		for (int x = 0; x < 480; x++)
			bad += screen(x, OY + y) != pattern(x + 100, y);
	oslDeleteImage(img);
	report("big 1:1 nearest vs source", bad, 9600, "differ");
}

static void bigGaps(const char *name, int bilinear, int w) {
	OSL_IMAGE *img = makeImage(600, 20, 1);
	img->x = OX;
	img->y = OY;
	img->stretchX = w;
	img->stretchY = 20;
	begin(bilinear);
	oslDrawImageBig(img);
	end();
	report(name, gaps(OX, OY, w, 20), (w - 2) * 18, "not white");
	oslDeleteImage(img);
}

// Map: tileset of solid 16x16 tiles; every drawn pixel must be its own tile's colour.
static void mapBleed(const char *name, int bilinear) {
	static const u32 colors[4] = { 0x0000ff, 0x00ff00, 0xff0000, 0xffffff };
	OSL_IMAGE *tiles = oslCreateImage(64, 16, OSL_IN_VRAM, OSL_PF_8888);
	for (int y = 0; y < 16; y++)
		for (int x = 0; x < 64; x++)
			oslSetImagePixel(tiles, x, y, colors[x / 16] | 0xff000000);
	oslUncacheImage(tiles);
	static u16 data[10 * 4];
	for (int i = 0; i < 40; i++)
		data[i] = (i + i / 10) % 4;
	OSL_MAP *m = oslCreateMap(tiles, data, 16, 16, 10, 4, OSL_MF_U16);
	m->flags = 0; // tile 0 is opaque too
	m->drawSizeX = 10;
	m->drawSizeY = 4;
	begin(bilinear);
	oslDrawMap(m);
	end();
	int bad = 0;
	for (int y = 0; y < 64; y++)
		for (int x = 0; x < 160; x++)
			bad += screen(x, y) != colors[data[(y / 16) * 10 + x / 16]];
	oslDeleteMap(m);
	oslDeleteImage(tiles);
	report(name, bad, 160 * 64, "wrong colour");
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	f = fopen("ms0:/PSP/GAME/seams/result.txt", "w");
	oslStartDrawing();
	oslSetAlpha(OSL_FX_NONE, 0);

	stripExact();
	stripVsSprite("strip 1.5x nearest vs sprite", 0, 300, 30, 0);
	stripVsSprite("strip 1.5x bilinear vs sprite", 1, 300, 30, 0);
	stripVsSprite("strip 0.75x nearest vs sprite", 0, 150, 15, 0);
	stripVsSprite("strip 1.37x nearest vs sprite", 0, 274, 27, 7398 / 10);
	stripVsSprite("strip 1.37x bilinear vs sprite", 1, 274, 27, 7398 / 10);
	stripGaps("strip gaps 1.37x nearest", 0, 274, 27);
	stripGaps("strip gaps 1.37x bilinear", 1, 274, 27);
	bigExact();
	bigGaps("big gaps 0.67x nearest", 0, 400);
	bigGaps("big gaps 0.67x bilinear", 1, 400);
	mapBleed("map nearest", 0);
	mapBleed("map bilinear", 1);

	oslEndDrawing();
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
