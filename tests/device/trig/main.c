// Device test: the integer trig tables must hold sin/cos * 16384 rounded to nearest, so that
// oslSini/oslCosi give exact results on exact angles (truncation made oslSini(30, 100) == 49).
#include <pspkernel.h>
#include <stdio.h>
#include <math.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("trig", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

extern int osl_isinus[], osl_icosinus[];

static FILE *f;

static void exact(const char *name, int got, int want) {
	fprintf(f, "%-18s %5d (want %5d) %s\n", name, got, want, got == want ? "PASS" : "FAIL");
}

int main() {
	oslInit(0);
	oslSetupFTrigo();
	f = fopen("ms0:/PSP/GAME/trig/result.txt", "w");

	exact("oslSini(30, 100)", oslSini(30, 100), 50);
	exact("oslSini(150, 100)", oslSini(150, 100), 50);
	exact("oslSini(90, 100)", oslSini(90, 100), 100);
	exact("oslSini(270, 100)", oslSini(270, 100), -100);
	exact("oslCosi(60, 100)", oslCosi(60, 100), 50);
	exact("oslCosi(0, 100)", oslCosi(0, 100), 100);
	exact("oslCosi(180, 100)", oslCosi(180, 100), -100);

	// Whole tables against libm, rounded. A few entries sit within 0.001 of .5 (e.g. cos(43) * 16384
	// = 11982.49905), beyond float precision, so allow a handful; truncation put 389 entries off.
	int bad = 0;
	for (int i = 0; i <= 360; i++) {
		double r = i * M_PI / 180;
		bad += osl_isinus[i] != (int)lround(sin(r) * 16384);
		bad += osl_icosinus[i] != (int)lround(cos(r) * 16384);
	}
	fprintf(f, "%-18s %5d/722 entries off %s\n", "tables vs libm", bad, bad > 4 ? "FAIL" : "PASS");

	fclose(f);
	sceKernelExitGame();
	return 0;
}
