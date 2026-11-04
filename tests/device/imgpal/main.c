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

// Linker wrappers fail just the requested allocation, leaving decoding and cleanup real.
static int failFormat = -1, failPalette, faults, liveImages;

OSL_IMAGE *__real_oslCreateImage(int width, int height, short location, short format);
void __real_oslDeleteImage(OSL_IMAGE *img);
OSL_PALETTE *__real_oslCreatePaletteEx(int size, int location, short format);

OSL_IMAGE *__wrap_oslCreateImage(int width, int height, short location, short format) {
	if (format == failFormat) {
		faults++;
		return NULL;
	}
	OSL_IMAGE *img = __real_oslCreateImage(width, height, location, format);
	if (img) liveImages++;
	return img;
}

void __wrap_oslDeleteImage(OSL_IMAGE *img) {
	if (img) liveImages--;
	__real_oslDeleteImage(img);
}

OSL_PALETTE *__wrap_oslCreatePaletteEx(int size, int location, short format) {
	if (failPalette) {
		faults++;
		return NULL;
	}
	return __real_oslCreatePaletteEx(size, location, format);
}

static void allocationFailure(const char *name, int location, int format, int palette) {
	fprintf(f, "%s ", name);
	int before = liveImages;
	faults = 0;
	failFormat = palette ? -1 : format;
	failPalette = palette;
	OSL_IMAGE *img = oslLoadImageFilePNG("pal256.png", location, format);
	failFormat = -1;
	failPalette = 0;
	int rejected = img == NULL;
	if (img) oslDeleteImage(img);
	int leaked = liveImages - before;
	fprintf(f, "-> NULL %d, faults %d, images %d %s\n", rejected, faults, leaked,
	        rejected && faults == 1 && !leaked ? "PASS" : "FAIL");
}

static void conversionFailure(int palette) {
	fprintf(f, "convert %s failure ", palette ? "palette" : "image");
	OSL_IMAGE *src = oslCreateImage(W, H, OSL_IN_RAM, OSL_PF_8888);
	if (!src) {
		fprintf(f, "source allocation failed FAIL\n");
		return;
	}
	oslSetImagePixel(src, 0, 0, RGBA(1, 2, 3, 255));
	int before = liveImages;
	faults = 0;
	failFormat = palette ? -1 : OSL_PF_4BIT;
	failPalette = palette;
	OSL_IMAGE *img = oslConvertImageTo(src, OSL_IN_RAM, OSL_PF_4BIT);
	failFormat = -1;
	failPalette = 0;
	int kept = !img && liveImages == before && oslGetImagePixel(src, 0, 0) == RGBA(1, 2, 3, 255);
	fprintf(f, "-> source preserved %d, faults %d %s\n", kept, faults,
	        kept && faults == 1 ? "PASS" : "FAIL");
	if (img) oslDeleteImage(img);
	else if (kept) oslDeleteImage(src);
}

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

// With a colour key on palette entry 250's colour, exactly its pixels (x % 8 == 2 in the
// bands that use 248-255: 4 per row, 12 rows) must come out transparent
static void colourKey256(int format) {
	oslSetTransparentColor(RGB(250, 5, (250 * 7) & 255));
	OSL_IMAGE *img = oslLoadImageFilePNG("pal256.png", OSL_IN_RAM | OSL_UNSWIZZLED, format);
	oslDisableTransparentColor();
	int keyed = 0, wrong = 0;
	for (int y = 0; img && img->palette && y < H; y++)
		for (int x = 0; x < W; x++) {
			int index = oslGetImagePixel(img, x, y);
			int clear = index < img->palette->nElements && !(((u32 *)img->palette->data)[index] >> 24);
			int want = x % 8 == 2 && (y / 3) % 2;
			keyed += clear && want;
			wrong += clear != want;
		}
	if (img) oslDeleteImage(img);
	fprintf(f, "pal256.png %s colour key: %d/48 keyed, %d wrong %s\n", format == OSL_PF_8BIT ? "8bit" : "4bit",
	        keyed, wrong, keyed == 48 && !wrong ? "PASS" : "FAIL");
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
	CASE("pal256", OSL_PF_8BIT, 0)
	CASE("pal256", OSL_PF_4BIT, 0) // 256-entry palette, 16 colours used, some past index 15
	CASE("pal256t", OSL_PF_8BIT, 0)
	CASE("pal256t", OSL_PF_4BIT, 0) // the same with tRNS
	if (only < 0) {
		colourKey256(OSL_PF_8BIT);
		colourKey256(OSL_PF_4BIT);
		allocationFailure("PNG target failure", OSL_IN_VRAM, OSL_PF_4BIT, 0);
		allocationFailure("PNG unswizzled target failure", OSL_IN_VRAM | OSL_UNSWIZZLED, OSL_PF_4BIT, 0);
		allocationFailure("PNG converted palette failure", OSL_IN_RAM, OSL_PF_4BIT, 1);
		allocationFailure("PNG direct palette failure", OSL_IN_RAM, OSL_PF_8BIT, 1);
		conversionFailure(0);
		conversionFailure(1);
	}
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
