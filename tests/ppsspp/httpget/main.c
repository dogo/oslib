// oslNetGetFile against tests/ppsspp/httpget/server.py (run.sh starts it and PPSSPP): a
// download lands complete, a failed one returns an error and leaves an existing file as it
// was, and a failed oslNetPostForm doesn't shut HTTP down for the calls after it.
// oslNetPostForm's reply is a string, read in full, cut to the buffer, empty on failure.
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <oslib/oslib.h>

PSP_MODULE_INFO("httpget", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define URL "http://127.0.0.1:8123"

static FILE *f;
static char dir[256];

// oslNetGetFile hands the path to sceIoOpen, which doesn't know newlib's working directory
static const char *at(const char *name) {
	static char path[2][300];
	static int i;
	i = !i;
	snprintf(path[i], sizeof(path[i]), "%s/%s", dir, name);
	return path[i];
}

// Size and FNV-1a of a file; size -1 if it can't be opened
static int fileInfo(const char *path, u32 *sum) {
	FILE *in = fopen(path, "rb");
	if (!in) return -1;
	int size = 0, c;
	*sum = 2166136261u;
	while ((c = fgetc(in)) != EOF) {
		*sum = (*sum ^ c) * 16777619u;
		size++;
	}
	fclose(in);
	return size;
}

static int writeOld(const char *path) {
	FILE *out = fopen(path, "wb");
	if (!out) return -1;
	int failed = fputs("old", out) == EOF;
	if (fclose(out)) failed = 1;
	return failed ? -1 : 0;
}

static void download(void) {
	u32 want = 2166136261u, sum = 0;
	for (int i = 0; i < 64 * 1024; i++) want = (want ^ ((i * 7) & 255)) * 16777619u;
	int r = oslNetGetFile(URL "/ok", at("ok.bin"));
	int size = fileInfo(at("ok.bin"), &sum);
	fprintf(f, "download             -> %d, %d bytes %s\n", r, size, !r && size == 64 * 1024 && sum == want ? "PASS" : "FAIL");
}

// The request fails: an error comes back and the file there before is untouched
static int keeps(const char *what, const char *path) {
	if (writeOld(at("keep.bin")) < 0) {
		fprintf(f, "%-20s -> could not create old file FAIL\n", what);
		return 1;
	}
	u32 sum, old;
	fileInfo(at("keep.bin"), &old);
	int r = oslNetGetFile(path, at("keep.bin"));
	int size = fileInfo(at("keep.bin"), &sum);
	u32 tmpSum;
	int tmp = fileInfo(at("keep.bin.oslpart"), &tmpSum) >= 0;
	int ok = r < 0 && size == 3 && sum == old && !tmp;
	fprintf(f, "%-20s -> %s, file %s%s %s\n", what, r < 0 ? "error" : "success", size == 3 && sum == old ? "kept" : "changed",
	        tmp ? ", .oslpart left" : "", ok ? "PASS" : "FAIL");
	return !ok;
}

static void noDirectory(void) {
	int r = oslNetGetFile(URL "/ok", at("nodir/ok.bin"));
	fprintf(f, "missing directory    -> %s %s\n", r < 0 ? "error" : "success", r < 0 ? "PASS" : "FAIL");
}

static void afterPostError(void) {
	char response[64];
	int p = oslNetPostForm(URL "/missing", "a=1", response, sizeof(response));
	sceIoRemove(at("ok.bin"));
	int r = oslNetGetFile(URL "/ok", at("ok.bin"));
	u32 sum;
	int size = fileInfo(at("ok.bin"), &sum);
	fprintf(f, "get after post error -> post %s, get %d, %d bytes %s\n", p < 0 ? "error" : "success", r, size,
	        p < 0 && !r && size == 64 * 1024 ? "PASS" : "FAIL");
}

// The reply to /form is 20000 letters a-z: several packets
static void postReply(const char *what, const char *url, unsigned int bufferSize, int wantResult, int wantLength) {
	static char reply[32768];
	memset(reply, 'x', sizeof(reply));
	int r = oslNetPostForm(url, "a=1", reply, bufferSize);
	int terminated = memchr(reply, 0, bufferSize) != NULL, length = terminated ? (int)strlen(reply) : -1, wrong = 0;
	for (int i = 0; i < length; i++)
		wrong += reply[i] != 'a' + i % 26;
	int ok = (wantResult > 0 ? r == wantResult : r < 0) && length == wantLength && !wrong;
	fprintf(f, "%-20s -> %s, %d characters%s %s\n", what, r < 0 ? "error" : "success", length, wrong ? ", WRONG" : "",
	        ok ? "PASS" : "FAIL");
}

int main() {
	oslInit(0);
	f = fopen("result.txt", "w");
	if (!f) {
		fputs("result.txt open FAIL\n", stderr);
		sceKernelExitGame();
		return 1;
	}
	setvbuf(f, NULL, _IONBF, 0);
	if (!getcwd(dir, sizeof(dir))) {
		fprintf(f, "working directory FAIL\n");
		fclose(f);
		sceKernelExitGame();
		return 1;
	}
	int r = oslNetInit();
	if (!r) r = oslConnectToAP(1, 30, NULL);
	if (r) {
		fprintf(f, "network %d FAIL\n", r);
	} else {
		download();
		r |= keeps("404", URL "/missing");
		r |= keeps("connection cut", URL "/cut");
		noDirectory();
		afterPostError();
		postReply("post reply", URL "/form", sizeof(char[32768]), 1, 20000);
		postReply("post reply cut", URL "/form", 100, 1, 99);
		postReply("post error reply", URL "/missing", 100, -1, 0);
		sceIoRemove(at("keep.bin"));
		sceIoRemove(at("ok.bin"));
		oslDisconnectFromAP();
	}
	oslNetTerm();
	fprintf(f, "DONE\n");
	fclose(f);
	sceKernelExitGame();
	return r != 0;
}
