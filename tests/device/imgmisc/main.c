// oslImageIsMirroredH/V before and after mirroring an image (and a tile of it),
// then oslDeleteImage(NULL), which is documented to do nothing, and that a new
// image's buffer (padding included) starts zeroed even over dirty RAM/VRAM.
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("imgmisc", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static FILE *f;

static void mirrored(const char *name, OSL_IMAGE *img, int wantH, int wantV) {
	int h = !!oslImageIsMirroredH(img), v = !!oslImageIsMirroredV(img);
	fprintf(f, "%-22s IsMirroredH %d IsMirroredV %d %s\n", name, h, v, h == wantH && v == wantV ? "PASS" : "FAIL");
}

// Dirty the memory a 37x33 image will reuse, then check its whole buffer is zero:
// the GPU samples the padding past sizeX/sizeY at the image's edges.
static void zeroed(const char *name, int location) {
	OSL_IMAGE *img = oslCreateImage(37, 33, location, OSL_PF_8888);
	int size = img->totalSize;
	memset(img->data, 0xab, size);
	oslUncacheData(img->data, size);
	oslDeleteImage(img);
	img = oslCreateImage(37, 33, location, OSL_PF_8888);
	const u8 *p = (const u8 *)oslGetUncachedPtr(img->data);
	int dirty = 0;
	for (int i = 0; i < img->totalSize; i++)
		dirty += p[i] != 0;
	fprintf(f, "%-22s %d/%d bytes not zero %s\n", name, dirty, img->totalSize, dirty ? "FAIL" : "PASS");
	oslDeleteImage(img);
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

	zeroed("new image in RAM", OSL_IN_RAM);
	zeroed("new image in VRAM", OSL_IN_VRAM);

	fprintf(f, "oslDeleteImage(NULL) ");
	oslDeleteImage(NULL);
	fprintf(f, "returned PASS\nDONE\n");
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
