// Image memory: a failed move keeps the image where it was, loaders don't leak on any
// path (measured with mallinfo over repeated loads), and a GIF whose colour table is
// bigger than OSL_PF_4BIT holds still resolves to the right colours.
#include <pspkernel.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("imgmem", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4 * 1024);

#define W 32
#define H 24

static FILE *f;

static int heapUsed(void) {
	return mallinfo().uordblks;
}

// Fills the heap with 1 KB blocks chained through their first word; returns the chain
static void *hogHeap(void) {
	void *hog = NULL, *p;
	while ((p = malloc(1024))) {
		*(void **)p = hog;
		hog = p;
	}
	return hog;
}

static void freeHog(void *hog) {
	while (hog) {
		void *next = *(void **)hog;
		free(hog);
		hog = next;
	}
}

static void moveFails(void) {
	fprintf(f, "move to RAM without memory ");
	OSL_IMAGE *img = oslCreateImage(64, 64, OSL_IN_VRAM, OSL_PF_8888);
	if (!img) {
		fprintf(f, "-> no VRAM image FAIL\n");
		return;
	}
	oslSetImagePixel(img, 3, 3, 0x12345678);
	void *hog = hogHeap();
	int moved = oslMoveImageTo(img, OSL_IN_RAM);
	freeHog(hog);
	int stayed = !moved && img->location == OSL_IN_VRAM && oslGetImagePixel(img, 3, 3) == 0x12345678;
	fprintf(f, "-> moved %d, location %s %s\n", moved, img->location == OSL_IN_VRAM ? "VRAM" : "RAM", stayed ? "PASS" : "FAIL");
	// With a wrong location this freed a VRAM pointer from the heap
	oslDeleteImage(img);
}

// Loads 20 times and reports what the heap kept, after one load to warm up
static void leak(const char *what, const char *file, int location, int format) {
	fprintf(f, "%-27s ", what);
	OSL_IMAGE *img = oslLoadImageFile((char *)file, location, format);
	if (img) oslDeleteImage(img);
	int before = heapUsed();
	for (int i = 0; i < 20; i++) {
		img = oslLoadImageFile((char *)file, location, format);
		if (img) oslDeleteImage(img);
	}
	int kept = heapUsed() - before;
	fprintf(f, "-> %d bytes kept over 20 loads %s\n", kept, kept ? "FAIL" : "PASS");
}

// The PNG decodes, but its image can't be created: VRAM is full
static void leakPngNoVram(void) {
	void *blocks[1024];
	int n = 0;
	while (n < 1024 && (blocks[n] = oslVramMgrAllocBlock(4096))) n++;
	leak("PNG into full VRAM", "rgb.png", OSL_IN_VRAM, OSL_PF_8888);
	while (n > 0) oslVramMgrFreeBlock(blocks[--n], 4096);
}

static void gif4bit(void) {
	unsigned char want[W * H * 4];
	FILE *r = fopen("pal256.rgba", "rb");
	int got = r ? fread(want, 1, sizeof(want), r) : 0;
	if (r) fclose(r);
	OSL_IMAGE *img = oslLoadImageFileGIF("pal256.gif", OSL_IN_RAM | OSL_UNSWIZZLED, OSL_PF_4BIT);
	if (!img || !img->palette || got != sizeof(want)) {
		fprintf(f, "pal256.gif 4bit load failed FAIL\n");
		if (img) oslDeleteImage(img);
		return;
	}
	int bad = 0;
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++) {
			int index = oslGetImagePixel(img, x, y);
			u32 p = index < img->palette->nElements ? ((u32 *)img->palette->data)[index] : 0xdeadbeef;
			const unsigned char *w = want + (y * W + x) * 4;
			bad += (p & 0xffffff) != (u32)(w[0] | w[1] << 8 | w[2] << 16);
		}
	oslDeleteImage(img);
	fprintf(f, "pal256.gif 4bit colour %d/%d wrong %s\n", bad, W * H, bad ? "FAIL" : "PASS");
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0); // keep what was written if a case crashes

	moveFails();
	leak("GIF with two frames", "anim.gif", OSL_IN_RAM, OSL_PF_8888);
	leak("missing GIF, true colour", "missing.gif", OSL_IN_RAM, OSL_PF_8888);
	leakPngNoVram();
	gif4bit();

	fprintf(f, "DONE\n");
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
