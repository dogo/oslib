// Device test: oslDrawImage's rotated path (any angle or hotspot) must draw a
// mirrored image in the same place as the plain one, with its halves swapped,
// and must finish when the frame width is not a multiple of 64.
#include <pspkernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("rotate", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4 * 1024);

#define RED 0x0000ff  // ABGR as read back
#define BLUE 0xff0000
#define CX 240
#define CY 136

static FILE *f;

typedef struct {
	int x0, y0, x1, y1;   // bounding box of drawn pixels
	int red, blue;        // pixel counts
	float rx, ry, bx, by; // centroids
} Shape;

// 100x40, left half red, right half blue: 100 is not a multiple of 64.
static OSL_IMAGE *makeImage(void) {
	OSL_IMAGE *img = oslCreateImage(100, 40, OSL_IN_VRAM, OSL_PF_8888);
	for (int y = 0; y < 40; y++)
		for (int x = 0; x < 100; x++)
			oslSetImagePixel(img, x, y, (x < 50 ? RGBA(255, 0, 0, 255) : RGBA(0, 0, 255, 255)));
	oslUncacheImage(img);
	return img;
}

static Shape draw(OSL_IMAGE *img, int angle, int mirrored) {
	OSL_IMAGE copy = *img;
	if (mirrored) oslMirrorImageH(&copy);
	copy.x = CX;
	copy.y = CY;
	copy.centerX = 50;
	copy.centerY = 20;
	copy.angle = angle;
	oslClearScreen(RGBA(0, 0, 0, 255));
	oslDrawImage(&copy);
	oslSyncDrawing();
	sceKernelDcacheWritebackInvalidateAll();

	Shape s = { 480, 272, -1, -1, 0, 0, 0, 0, 0, 0 };
	for (int y = 0; y < 272; y++)
		for (int x = 0; x < 480; x++) {
			u32 c = oslGetImagePixel(OSL_DEFAULT_BUFFER, x, y) & 0xffffff;
			if (!c) continue;
			if (x < s.x0) s.x0 = x;
			if (y < s.y0) s.y0 = y;
			if (x > s.x1) s.x1 = x;
			if (y > s.y1) s.y1 = y;
			if (c == RED) {
				s.red++; s.rx += x; s.ry += y;
			}
			if (c == BLUE) {
				s.blue++; s.bx += x; s.by += y;
			}
		}
	if (s.red) {
		s.rx /= s.red; s.ry /= s.red;
	}
	if (s.blue) {
		s.bx /= s.blue; s.by /= s.blue;
	}
	return s;
}

static int near(float a, float b, float tol) {
	return a - b <= tol && b - a <= tol;
}

// The mirrored shape must cover the same area with red and blue swapped.
static void compare(OSL_IMAGE *img, int angle) {
	Shape n = draw(img, angle, 0), m = draw(img, angle, 1);
	int box = near(n.x0, m.x0, 1) && near(n.y0, m.y0, 1) && near(n.x1, m.x1, 1) && near(n.y1, m.y1, 1);
	int swap = near(m.red, n.blue, n.blue / 50 + 2) && near(m.blue, n.red, n.red / 50 + 2) &&
	           near(m.rx, n.bx, 1) && near(m.ry, n.by, 1) && near(m.bx, n.rx, 1) && near(m.by, n.ry, 1);
	fprintf(f, "angle %3d plain    box %3d,%3d-%3d,%3d red %4d blue %4d\n", angle, n.x0, n.y0, n.x1, n.y1, n.red, n.blue);
	fprintf(f, "angle %3d mirrored box %3d,%3d-%3d,%3d red %4d blue %4d same place %s, halves swapped %s\n",
	        angle, m.x0, m.y0, m.x1, m.y1, m.red, m.blue, box ? "PASS" : "FAIL", swap ? "PASS" : "FAIL");
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	oslSetBilinearFilter(0);
	oslSetAlpha(OSL_FX_NONE, 0);
	OSL_IMAGE *img = makeImage();

	oslStartDrawing();
	f = fopen("result.txt", "w");
	// Angle 0 with a hotspot still takes the rotated path: both must sit at CX-50..CX+49
	for (int mirrored = 0; mirrored < 2; mirrored++) {
		Shape s = draw(img, 0, mirrored);
		int ok = s.x0 == CX - 50 && s.x1 == CX + 49 && s.y0 == CY - 20 && s.y1 == CY + 19;
		fprintf(f, "angle   0 %-8s box %3d,%3d-%3d,%3d (want %d,%d-%d,%d) %s\n", mirrored ? "mirrored" : "plain",
		        s.x0, s.y0, s.x1, s.y1, CX - 50, CY - 20, CX + 49, CY + 19, ok ? "PASS" : "FAIL");
	}
	compare(img, 90);
	compare(img, 37); // looped forever before 4638a26 when mirrored
	oslEndDrawing();
	oslWriteImageFilePNG(OSL_DEFAULT_BUFFER, "screen.png", 0);
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
