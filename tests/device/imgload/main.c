// Device test: OSLib's PNG/JPEG/GIF loaders, loading into OSL_PF_8888, must
// produce the pixels ImageMagick decodes from the same file (<name>.rgba, see
// gen.sh). JPEG decoders may round differently, so JPEG allows +-4 per channel;
// libpng strips 16-bit channels by truncating where ImageMagick rounds: +-1.
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("imgload", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4 * 1024);

#define W 32
#define H 24

static FILE *f;

static void check(const char *name, const char *ext, int tol) {
	char file[32], ref[32];
	unsigned char want[W * H * 4];
	snprintf(file, sizeof(file), "%s.%s", name, ext);
	snprintf(ref, sizeof(ref), "%s.rgba", name);
	FILE *r = fopen(ref, "rb");
	int got = r ? fread(want, 1, sizeof(want), r) : 0;
	if (r) fclose(r);

	OSL_IMAGE *img = oslLoadImageFile(file, OSL_IN_RAM | OSL_UNSWIZZLED, OSL_PF_8888);
	if (!img || got != sizeof(want) || img->sizeX != W || img->sizeY != H) {
		fprintf(f, "%-16s load failed FAIL\n", file);
		if (img) oslDeleteImage(img);
		return;
	}
	int badColour = 0, badAlpha = 0, fx = -1, fy = -1;
	u32 fgot = 0, fwant = 0;
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++) {
			u32 p = oslGetImagePixel(img, x, y);
			const unsigned char *w = want + (y * W + x) * 4;
			int colour = 0;
			for (int c = 0; c < 3; c++)
				colour |= abs((int)((p >> (c * 8)) & 0xff) - w[c]) > tol;
			int alpha = ((p >> 24) & 0xff) != w[3];
			badColour += colour;
			badAlpha += alpha;
			if ((colour || alpha) && fx < 0) {
				fx = x; fy = y; fgot = p; fwant = w[0] | w[1] << 8 | w[2] << 16 | (u32)w[3] << 24;
			}
		}
	oslDeleteImage(img);
	fprintf(f, "%-16s colour %3d/%d alpha %3d/%d wrong", file, badColour, W * H, badAlpha, W * H);
	if (fx >= 0) fprintf(f, " (first %d,%d got %08lx want %08lx)", fx, fy, fgot, fwant);
	fprintf(f, " %s\n", badColour || badAlpha ? "FAIL" : "PASS");
}

// With a colour key set, grey PNGs must key out the same pixels as RGB ones: the white column.
static void colourKey(const char *file) {
	oslSetTransparentColor(RGB(255, 255, 255));
	OSL_IMAGE *img = oslLoadImageFile((char *)file, OSL_IN_RAM | OSL_UNSWIZZLED, OSL_PF_8888);
	oslDisableTransparentColor();
	int keyed = 0, others = 0;
	for (int y = 0; img && y < H; y++)
		for (int x = 0; x < W; x++) {
			int clear = !(oslGetImagePixel(img, x, y) >> 24);
			if (x == W - 1) keyed += clear;
			else others += clear;
		}
	if (img) oslDeleteImage(img);
	fprintf(f, "%-16s colour key: %d/%d white keyed, %d others %s\n", file, keyed, H, others,
	        img && keyed == H && !others ? "PASS" : "FAIL");
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	f = fopen("result.txt", "w");
	check("rgba8", "png", 0);
	check("rgb8", "png", 0);
	check("rgba16", "png", 1);
	check("interlaced", "png", 0);
	check("pal8", "png", 0);
	check("pal8t", "png", 0);
	check("pal4", "png", 0);
	check("gray8", "png", 0);
	check("graya8", "png", 0);
	check("gray4", "png", 0);
	check("gray16", "png", 1);
	check("graya16", "png", 1);
	check("grayt", "png", 0); // tRNS on a mid grey
	check("jpeg", "jpg", 4);
	check("progressive", "jpg", 4);
	check("gif", "gif", 0);
	colourKey("rgb8.png");
	colourKey("gray8.png");
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
