// With the VRAM manager off (VRAM handed out as a stack): VRAM images can be
// allocated at all, and deleting a VRAM palette gives back exactly its own
// bytes, or the next block lands on live VRAM.
#include <pspkernel.h>
#include <stdio.h>
#include <oslib/oslib.h>
#include <oslib/vram_mgr.h>

PSP_MODULE_INFO("palvram", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

int main() {
	osl_useVramManager = 0; // before oslInitGfx, which sets up VRAM
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	FILE *f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0);

	OSL_IMAGE *img = oslCreateImage(32, 32, OSL_IN_VRAM, OSL_PF_8888);
	fprintf(f, "first VRAM image %s\n", img && img->data ? "allocated PASS" : "NULL FAIL");
	if (!img || !img->data) goto done;
	u32 before = osl_currentVramPtr;
	OSL_PALETTE *p = oslCreatePaletteEx(256, OSL_IN_VRAM, OSL_PF_8888); // 1024 bytes
	u32 during = osl_currentVramPtr;
	oslDeletePalette(p);
	u32 after = osl_currentVramPtr;
	fprintf(f, "palette took %lu bytes, delete gave back %lu %s\n", during - before, during - after,
	        after == before ? "PASS" : "FAIL");

	OSL_IMAGE *next = oslCreateImage(32, 32, OSL_IN_VRAM, OSL_PF_8888);
	int overlap = next && (u8 *)next->data < (u8 *)img->data + img->totalSize;
	fprintf(f, "next VRAM image %s the first one %s\n", overlap ? "overlaps" : "is after", overlap ? "FAIL" : "PASS");
done:
	fprintf(f, "DONE\n");
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
