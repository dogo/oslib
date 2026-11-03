// Device test: each oslSetAlpha effect blends a solid image over the screen as
// its formula says (S = image colour, a = its alpha times the coefficient,
// B = background). The GE's rounding isn't specified, so +-2 per channel.
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("blend", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static const int S[3] = { 200, 100, 50 }, SA = 128; // image
static const int B[3] = { 40, 80, 160 };            // background

static FILE *f;
static OSL_IMAGE *img;

enum { MIX, ADD, SUB };

// tint: colour (and alpha) the coefficient multiplies the image by
static void check(const char *name, int effect, u32 coeff, int op, const int tint[4]) {
	oslSetAlpha(OSL_FX_NONE, 0);
	oslClearScreen(RGBA(B[0], B[1], B[2], 255));
	oslSetAlpha(effect, coeff);
	oslDrawImageXY(img, 16, 16);
	oslSetAlpha(OSL_FX_NONE, 0);
	oslSyncDrawing();
	sceKernelDcacheWritebackInvalidateAll();

	float a = SA / 255.f * tint[3] / 255.f;
	int want[3], got[3], bad = 0;
	u32 p = oslGetImagePixel(OSL_DEFAULT_BUFFER, 16 + 16, 16 + 4);
	for (int c = 0; c < 3; c++) {
		float s = S[c] * tint[c] / 255.f;
		float v = op == MIX ? s * a + B[c] * (1 - a) : op == ADD ? s * a + B[c] : B[c] - s * a;
		want[c] = v < 0 ? 0 : v > 255 ? 255 : (int)(v + 0.5f);
		got[c] = (p >> (c * 8)) & 0xff;
		bad |= abs(got[c] - want[c]) > 2;
	}
	if (effect == OSL_FX_NONE) { // no blending at all: the image's own colour
		for (int c = 0; c < 3; c++) want[c] = S[c];
		bad = got[0] != S[0] || got[1] != S[1] || got[2] != S[2];
	}
	fprintf(f, "%-14s got %3d,%3d,%3d want %3d,%3d,%3d %s\n", name, got[0], got[1], got[2],
	        want[0], want[1], want[2], bad ? "FAIL" : "PASS");
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	oslSetBilinearFilter(0);
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0);
	img = oslCreateImage(32, 8, OSL_IN_RAM, OSL_PF_8888);
	for (int y = 0; y < 8; y++)
		for (int x = 0; x < 32; x++)
			oslSetImagePixel(img, x, y, RGBA(S[0], S[1], S[2], SA));
	oslUncacheImage(img);

	static const int white[4] = { 255, 255, 255, 255 }, half[4] = { 255, 255, 255, 128 };
	static const int redTint[4] = { 255, 0, 0, 255 }, cyanHalf[4] = { 0, 255, 255, 128 };
	oslStartDrawing();
	check("none", OSL_FX_NONE, 0, MIX, white);
	check("rgba", OSL_FX_RGBA, 0, MIX, white);
	check("alpha 255", OSL_FX_ALPHA, 255, MIX, white);
	check("alpha 128", OSL_FX_ALPHA, 128, MIX, half);
	check("add 255", OSL_FX_ADD, 255, ADD, white);
	check("add 128", OSL_FX_ADD, 128, ADD, half);
	check("sub 255", OSL_FX_SUB, 255, SUB, white);
	check("sub 128", OSL_FX_SUB, 128, SUB, half);
	check("tint red", OSL_FX_TINT, RGBA(255, 0, 0, 255), MIX, redTint);
	check("tint cyan/2", OSL_FX_TINT, RGBA(0, 255, 255, 128), MIX, cyanHalf);
	oslEndDrawing();
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
