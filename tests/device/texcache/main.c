// Device test: re-drawing an image after its contents changed must not sample stale
// texels from the GE's 8 KB texture cache. Only TEXFLUSH (emitted by sceGuTexImage)
// invalidates it, so OSLib has to re-bind the texture after render-to-texture
// (oslSetDrawBuffer) and CPU writes (oslUncacheImageData). Each case is paired with
// a control that flushes manually, proving the readback itself is sound.
#include <pspkernel.h>
#include <pspgu.h>
#include <stdio.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("texcache", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4 * 1024);

#define RED   RGBA(255, 0, 0, 255)
#define GREEN RGBA(0, 255, 0, 255)
#define BLUE  RGBA(0, 0, 255, 255)

static OSL_IMAGE *A;

static void gpuFill(int color) {
	oslSetDrawBuffer(A);
	oslClearScreen(color);
	oslSetDrawBuffer(OSL_DEFAULT_BUFFER);
}

static u32 screenPixel(int x, int y) {
	oslSyncDrawing();
	sceKernelDcacheWritebackInvalidateAll();
	return oslGetImagePixel(OSL_DEFAULT_BUFFER, x, y) & 0xffffff;
}

// Draw A (cache now holds red), change A's contents, draw A again at x, sample it.
static u32 run(int x, int cpuWrite, int manualFlush) {
	gpuFill(RED);
	oslDrawImageXY(A, 0, 200);
	oslSyncDrawing();
	if (cpuWrite)
		oslClearImage(A, BLUE);
	else
		gpuFill(GREEN);
	if (manualFlush)
		sceGuTexFlush();
	oslDrawImageXY(A, x, 0);
	return screenPixel(x + 10, 10);
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	A = oslCreateImage(32, 32, OSL_IN_VRAM, OSL_PF_8888);
	oslSetAlpha(OSL_FX_NONE, 0);

	oslStartDrawing();
	oslClearScreen(0);
	u32 gpu = run(0, 0, 0), gpuCtl = run(50, 0, 1);
	u32 cpu = run(100, 1, 0), cpuCtl = run(150, 1, 1);
	oslEndDrawing();

	u32 g = GREEN & 0xffffff, b = BLUE & 0xffffff;
	FILE *f = fopen("ms0:/PSP/GAME/texcache/result.txt", "w");
	fprintf(f, "render-to-texture: %06lx (control %06lx) %s\n", gpu, gpuCtl, gpu == g ? "PASS" : "FAIL");
	fprintf(f, "cpu write:         %06lx (control %06lx) %s\n", cpu, cpuCtl, cpu == b ? "PASS" : "FAIL");
	fprintf(f, "controls valid:    %s\n", gpuCtl == g && cpuCtl == b ? "yes" : "NO");
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
