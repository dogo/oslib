// Text paths that wrote past fixed buffers: an SFont with more letters than OSL_SFONT
// holds, and the formatting macros' 1000-byte buffers. Built with -fstack-protector-all,
// so a macro that overflows its buffer ends the test before its line is finished.
#include <pspkernel.h>
#include <oslib/oslib.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("textbounds", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static FILE *f;
static char text[1501];

static void sfont(const char *file, int want) {
	fprintf(f, "%-12s ", file);
	OSL_SFONT *s = oslLoadSFontFile((char *)file, OSL_PF_8888);
	int got = s ? s->lettersCount : -1;
	if (s) oslDeleteSFont(s);
	fprintf(f, "-> %d letters (want %d) %s\n", got, want, got == want ? "PASS" : "FAIL");
}

// Each in its own frame: the canary is checked when it returns
__attribute__((noinline)) static void drawStringf(void) {
	oslDrawStringf(0, 300, "%s", text); // y off screen
}

__attribute__((noinline)) static void printf1500(void) {
	oslPrintf("%s", text);
}

int main() {
	oslInit(0);
	oslInitGfx(OSL_PF_8888, 1);
	f = fopen("result.txt", "w");
	setvbuf(f, NULL, _IONBF, 0); // keep what was written if a case crashes

	sfont("font10.png", 10);
	sfont("font300.png", -1); // rejected

	memset(text, 'x', sizeof(text) - 1);
	oslStartDrawing();
	fprintf(f, "oslDrawStringf 1500 chars ");
	drawStringf();
	fprintf(f, "-> returned PASS\n");
	fprintf(f, "oslPrintf 1500 chars      ");
	printf1500();
	fprintf(f, "-> returned PASS\n");
	oslEndDrawing();

	fprintf(f, "DONE\n");
	fclose(f);
	oslEndGfx();
	sceKernelExitGame();
	return 0;
}
