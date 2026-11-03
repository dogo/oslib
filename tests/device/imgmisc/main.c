// oslImageIsMirroredH/V before and after mirroring an image (and a tile of it),
// then oslDeleteImage(NULL), which is documented to do nothing.
#include <pspkernel.h>
#include <stdio.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("imgmisc", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static FILE *f;

static void mirrored(const char *name, OSL_IMAGE *img, int wantH, int wantV) {
	int h = !!oslImageIsMirroredH(img), v = !!oslImageIsMirroredV(img);
	fprintf(f, "%-22s IsMirroredH %d IsMirroredV %d %s\n", name, h, v, h == wantH && v == wantV ? "PASS" : "FAIL");
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0); // keep what was written if the NULL delete crashes

	OSL_IMAGE *img = oslCreateImage(32, 16, OSL_IN_RAM, OSL_PF_8888);
	mirrored("new image", img, 0, 0);
	oslMirrorImageH(img);
	mirrored("mirrored H", img, 1, 0);
	oslMirrorImageV(img);
	mirrored("mirrored H and V", img, 1, 1);
	oslMirrorImageH(img);
	mirrored("mirrored V", img, 0, 1);
	oslSetImageTile(img, 8, 4, 24, 12);
	mirrored("tile", img, 0, 0);
	oslMirrorImageH(img);
	mirrored("tile mirrored H", img, 1, 0);
	oslDeleteImage(img);

	fprintf(f, "oslDeleteImage(NULL) ");
	oslDeleteImage(NULL);
	fprintf(f, "returned PASS\nDONE\n");
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
