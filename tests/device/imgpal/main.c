// Device test: PNGs loaded into OSLib's paletted formats (OSL_PF_8BIT/4BIT)
// must resolve, through their palette, to the pixels ImageMagick decodes from
// the same file (the .rgba files, from tests/device/imgload/gen.sh).
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("imgpal", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4 * 1024);

#define W 32
#define H 24

static FILE *f;

static void check(const char *name, int format, int tol) {
	char file[32], ref[32];
	unsigned char want[W * H * 4];
	snprintf(file, sizeof(file), "%s.png", name);
	snprintf(ref, sizeof(ref), "%s.rgba", name);
	FILE *r = fopen(ref, "rb");
	int got = r ? fread(want, 1, sizeof(want), r) : 0;
	if (r) fclose(r);
	const char *fmt = format == OSL_PF_8BIT ? "8bit" : "4bit";

	OSL_IMAGE *img = oslLoadImageFilePNG(file, OSL_IN_RAM | OSL_UNSWIZZLED, format);
	if (!img || !img->palette || got != sizeof(want) || img->sizeX != W || img->sizeY != H) {
		fprintf(f, "%-10s %s load failed FAIL\n", file, fmt);
		if (img) oslDeleteImage(img);
		return;
	}
	int badColour = 0, badAlpha = 0, fx = -1, fy = -1;
	u32 fgot = 0, fwant = 0;
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++) {
			int index = oslGetImagePixel(img, x, y);
			u32 p = index < img->palette->nElements ? ((u32 *)img->palette->data)[index] : 0xdeadbeef;
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
	fprintf(f, "%-10s %s colour %3d/%d alpha %3d/%d wrong", file, fmt, badColour, W * H, badAlpha, W * H);
	if (fx >= 0) fprintf(f, " (first %d,%d got %08lx want %08lx)", fx, fy, fgot, fwant);
	fprintf(f, " %s\n", badColour || badAlpha ? "FAIL" : "PASS");
}

int main(int argc, char *argv[]) {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0); // keep what was written if a load crashes
	// argv[1] = case number, to run one at a time
	int only = argc > 1 ? atoi(argv[1]) : -1, n = 0;
#define CASE(...) if (only < 0 || only == n) check(__VA_ARGS__); n++;
	CASE("pal8", OSL_PF_8BIT, 0)
	CASE("pal8t", OSL_PF_8BIT, 0)  // palette + tRNS
	CASE("pal4", OSL_PF_8BIT, 0)
	CASE("pal4", OSL_PF_4BIT, 0)
	CASE("gray4", OSL_PF_8BIT, 0)  // no palette: loaded as 8888, then converted
	CASE("gray4", OSL_PF_4BIT, 0)
	CASE("gray8", OSL_PF_8BIT, 0)
	CASE("grayt", OSL_PF_8BIT, 0)  // tRNS on a mid grey
	CASE("gray16", OSL_PF_8BIT, 1) // libpng truncates 16-bit, ImageMagick rounds
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
