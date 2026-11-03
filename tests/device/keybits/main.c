// Each OSL_KEYLIST / OSL_REMOTEKEYLIST bit field must read the SDK's button
// bit (what sceCtrl / sceHprm put in value), and no other field may.
#include <pspkernel.h>
#include <pspctrl.h>
#include <psphprm.h>
#include <stdio.h>
#include <string.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("keybits", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static FILE *f;

#define PAD(field, bit) { #field, bit }
static const struct {
	const char *name;
	u32 bit;
} pad[] = { PAD(select, PSP_CTRL_SELECT), PAD(start, PSP_CTRL_START), PAD(up, PSP_CTRL_UP), PAD(right, PSP_CTRL_RIGHT),
	    PAD(down, PSP_CTRL_DOWN), PAD(left, PSP_CTRL_LEFT), PAD(L, PSP_CTRL_LTRIGGER), PAD(R, PSP_CTRL_RTRIGGER),
	    PAD(triangle, PSP_CTRL_TRIANGLE), PAD(circle, PSP_CTRL_CIRCLE), PAD(cross, PSP_CTRL_CROSS),
	    PAD(square, PSP_CTRL_SQUARE), PAD(home, PSP_CTRL_HOME), PAD(hold, PSP_CTRL_HOLD), PAD(note, PSP_CTRL_NOTE) },
  remote[] = { PAD(rmplaypause, PSP_HPRM_PLAYPAUSE), PAD(rmforward, PSP_HPRM_FORWARD), PAD(rmback, PSP_HPRM_BACK),
	       PAD(rmvolup, PSP_HPRM_VOL_UP), PAD(rmvoldown, PSP_HPRM_VOL_DOWN), PAD(rmhold, PSP_HPRM_HOLD) };

// The fields that read 1 for a value, by name
static void padFields(u32 v, char *out) {
	OSL_KEYLIST k;
	k.value = v;
	*out = 0;
#define F(x) if (k.x) strcat(out, #x " ");
	F(select) F(start) F(up) F(right) F(down) F(left) F(L) F(R) F(triangle) F(circle) F(cross) F(square) F(home) F(hold) F(note)
#undef F
}

static void remoteFields(u32 v, char *out) {
	OSL_REMOTEKEYLIST k;
	k.value = v;
	*out = 0;
#define F(x) if (k.x) strcat(out, #x " ");
	F(rmplaypause) F(rmforward) F(rmback) F(rmvolup) F(rmvoldown) F(rmhold)
#undef F
}

static void check(const char *name, u32 bit, void (*fields)(u32, char *)) {
	char got[128], want[64];
	fields(bit, got);
	snprintf(want, sizeof(want), "%s ", name);
	fprintf(f, "%-11s 0x%06lx -> %-12s %s\n", name, bit, got[0] ? got : "(none)", strcmp(got, want) ? "FAIL" : "PASS");
}

int main() {
	f = fopen("result.txt", "w");
	for (unsigned i = 0; i < sizeof(pad) / sizeof(pad[0]); i++) check(pad[i].name, pad[i].bit, padFields);
	for (unsigned i = 0; i < sizeof(remote) / sizeof(remote[0]); i++) check(remote[i].name, remote[i].bit, remoteFields);
	fprintf(f, "DONE\n");
	fclose(f);
	sceKernelExitGame();
	return 0;
}
