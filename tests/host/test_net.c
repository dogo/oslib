#include "oslib.h"
#include "test.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <psphttp.h>
#include <pspnet.h>
#include <pspnet_apctl.h>
#include <pspnet_inet.h>
#include <pspnet_resolver.h>
#include <pspssl.h>
#include <psputility.h>
#include <psputility_netmodules.h>
#include <pspwlan.h>
#include <unistd.h>

#include "net.h"

int osl_quit;

void oslHandleLoadNoFailError(const char *filename) {
	(void)filename;
}

// Network bring-up: never reached by the HTTP calls under test
int sceNetInit(int poolsize, int calloutprio, int calloutstack, int netintrprio, int netintrstack) {
	(void)poolsize; (void)calloutprio; (void)calloutstack; (void)netintrprio; (void)netintrstack;
	return 0;
}

int sceNetTerm(void) {
	return 0;
}

int sceNetInetInit(void) {
	return 0;
}

int sceNetInetTerm(void) {
	return 0;
}

int sceNetResolverInit(void) {
	return 0;
}

int sceNetResolverTerm(void) {
	return 0;
}

int sceSslInit(int unknown1) {
	(void)unknown1; return 0;
}

int sceSslEnd(void) {
	return 0;
}

int sceNetApctlInit(int stackSize, int initPriority) {
	(void)stackSize; (void)initPriority; return 0;
}

int sceNetApctlTerm(void) {
	return 0;
}

int sceNetApctlConnect(int connIndex) {
	(void)connIndex; return 0;
}

int sceNetApctlDisconnect(void) {
	return 0;
}

int sceNetApctlGetState(int *pState) {
	*pState = PSP_NET_APCTL_STATE_GOT_IP; return 0;
}

int sceNetApctlGetInfo(int code, union SceNetApctlInfo *pInfo) {
	(void)code; pInfo->ip[0] = 0; return 0;
}

int sceUtilityCheckNetParam(int id) {
	(void)id; return -1;
}

int sceUtilityGetNetParam(int conf, int param, netData *data) {
	(void)conf; (void)param; data->asUint = 0; return -1;
}

int sceUtilityLoadNetModule(int module) {
	(void)module; return 0;
}

int sceUtilityUnloadNetModule(int module) {
	(void)module; return 0;
}

int sceWlanDevIsPowerOn(void) {
	return 1;
}

int sceWlanGetEtherAddr(uint8_t *etherAddr) {
	(void)etherAddr; return 0;
}

int sceKernelDelayThread(unsigned int delay) {
	return usleep(delay);
}

// Fake HTTP: one scripted response; counts objects so leaks show; sceHttpEnd ends HTTP for good
static struct {
	int status;
	int length;  // Content-Length, -1 if not sent
	int size;    // bytes the "server" delivers
	int failAt;  // read error once this many bytes were read, -1 for none
} response;
static int delivered, live, ended;

int sceHttpInit(unsigned int unknown1) {
	(void)unknown1; return 0;
}

int sceHttpEnd(void) {
	ended = 1; return 0;
}

int sceHttpsInit(int a, int b, int c, int d) {
	(void)a; (void)b; (void)c; (void)d; return 0;
}

int sceHttpsEnd(void) {
	return 0;
}

int sceHttpsLoadDefaultCert(int a, int b) {
	(void)a; (void)b; return 0;
}

int sceHttpLoadSystemCookie(void) {
	return 0;
}

int sceHttpSaveSystemCookie(void) {
	return 0;
}

int sceHttpCreateTemplate(char *agent, int a, int b) {
	(void)agent; (void)a; (void)b; return ended ? -1 : (live++, 1);
}

int sceHttpDeleteTemplate(int id) {
	(void)id; live--; return 0;
}

int sceHttpSetResolveTimeOut(int id, unsigned int t) {
	(void)id; (void)t; return 0;
}

int sceHttpSetRecvTimeOut(int id, unsigned int t) {
	(void)id; (void)t; return 0;
}

int sceHttpSetSendTimeOut(int id, unsigned int t) {
	(void)id; (void)t; return 0;
}

int sceHttpCreateConnectionWithURL(int id, const char *url, int a) {
	(void)id; (void)url; (void)a; live++; return 2;
}

int sceHttpDeleteConnection(int id) {
	(void)id; live--; return 0;
}

int sceHttpEnableKeepAlive(int id) {
	(void)id; return 0;
}

int sceHttpCreateRequestWithURL(int id, int method, char *url, SceULong64 length) {
	(void)id; (void)method; (void)url; (void)length;
	delivered = 0;
	live++;
	return 3;
}

int sceHttpDeleteRequest(int id) {
	(void)id; live--; return 0;
}

int sceHttpAddExtraHeader(int id, char *name, char *value, int a) {
	(void)id; (void)name; (void)value; (void)a; return 0;
}

int sceHttpSendRequest(int id, void *data, unsigned int size) {
	(void)id; (void)data; (void)size; return 0;
}

int sceHttpGetStatusCode(int id, int *status) {
	(void)id; *status = response.status; return 0;
}

int sceHttpGetContentLength(int id, SceULong64 *length) {
	(void)id;
	if (response.length < 0) return (int)0x80431085; // no Content-Length header
	*length = response.length;
	return 0;
}

int sceHttpReadData(int id, void *data, unsigned int size) {
	(void)id;
	if (response.failAt >= 0 && delivered >= response.failAt) return (int)0x80431081;
	int n = response.size - delivered;
	if ((unsigned int)n > size) n = size;
	if (response.failAt >= 0 && delivered + n > response.failAt) n = response.failAt - delivered;
	memset(data, 'n', n);
	delivered += n;
	return n;
}

#define FILE_ "test_net.bin"

static void writeOld(void) {
	FILE *f = fopen(FILE_, "wb");
	fputs("old", f);
	fclose(f);
}

static long fileSize(const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f) return -1;
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fclose(f);
	return size;
}

// The lowest free descriptor: grows if a call leaves a file open
static int nextFd(void) {
	int fd = open("/dev/null", O_RDONLY);
	close(fd);
	return fd;
}

// Runs one download over an existing "old" file and checks what is left around it
static void get(int status, int length, int size, int failAt, int wantOk) {
	response.status = status;
	response.length = length;
	response.size = size;
	response.failAt = failAt;
	writeOld();
	int fd = nextFd();
	int r = oslNetGetFile("http://host/x", FILE_);
	CHECK_EQ(r == 0, wantOk);
	CHECK_EQ(fileSize(FILE_), wantOk ? size : 3);
	CHECK_EQ(fileSize(FILE_ ".oslpart"), -1);
	CHECK_EQ(fileSize(FILE_ ".oslbak"), -1);
	CHECK_EQ(nextFd(), fd);
	CHECK_EQ(live, 0);
	remove(FILE_);
}

// A failure before anything was written leaves files of that name alone
static void notOursIsKept(void) {
	FILE *f = fopen(FILE_ ".oslpart", "wb");
	fputs("mine", f);
	fclose(f);
	response.status = 404;
	CHECK(oslNetGetFile("http://host/x", FILE_) < 0);
	CHECK_EQ(fileSize(FILE_ ".oslpart"), 4);
	remove(FILE_ ".oslpart");
}

static void put(const char *path, const char *text) {
	FILE *f = fopen(path, "wb");
	fputs(text, f);
	fclose(f);
}

static void respond(int status, int size) {
	response.status = status;
	response.length = size;
	response.size = size;
	response.failAt = -1;
}

// Power-off after the old file was moved aside: the next call, even a failing one, puts it back
static void backupWithoutFileIsRestored(void) {
	remove(FILE_);
	put(FILE_ ".oslbak", "old");
	respond(404, 10);
	CHECK(oslNetGetFile("http://host/x", FILE_) < 0);
	CHECK_EQ(fileSize(FILE_), 3);
	CHECK_EQ(fileSize(FILE_ ".oslbak"), -1);
	remove(FILE_);
}

// Power-off after the new file went in (or a backup that couldn't be removed): downloads go on
static void leftoverBackupIsRemoved(void) {
	put(FILE_, "new");
	put(FILE_ ".oslbak", "old");
	for (int i = 0; i < 2; i++) {
		respond(200, 100 + i);
		CHECK_EQ(oslNetGetFile("http://host/x", FILE_), 0);
		CHECK_EQ(fileSize(FILE_), 100 + i);
		CHECK_EQ(fileSize(FILE_ ".oslbak"), -1);
	}
	remove(FILE_);
}

// A directory is not replaced, nor moved aside
static void directoryIsRejected(void) {
	mkdir(FILE_, 0777);
	put(FILE_ "/inner", "keep");
	respond(200, 100);
	CHECK(oslNetGetFile("http://host/x", FILE_) < 0);
	CHECK_EQ(fileSize(FILE_ "/inner"), 4);
	CHECK_EQ(fileSize(FILE_ ".oslbak"), -1);
	CHECK_EQ(fileSize(FILE_ ".oslpart"), -1);
	remove(FILE_ "/inner");
	rmdir(FILE_);
}

int main(void) {
	get(200, 5000, 5000, -1, 1);   // complete, replaces the old file
	get(200, -1, 3000, -1, 1);     // no Content-Length (chunked): the end of data ends it
	get(404, 10, 10, -1, 0);       // not the file
	get(200, 5000, 1000, -1, 0);   // the connection closed early
	get(200, 5000, 5000, 1000, 0); // read error: its file must be closed too
	for (int i = 0; i < 3; i++) get(200, 5000, 5000, 1000, 0);

	// A failed post must not end HTTP for the next call
	char reply[16];
	response.status = 404;
	CHECK(oslNetPostForm("http://host/form", "a=1", reply, sizeof(reply)) < 0);
	CHECK_EQ(live, 0);
	get(200, 100, 100, -1, 1);
	notOursIsKept();
	backupWithoutFileIsRestored();
	leftoverBackupIsRemoved();
	directoryIsRejected();
	return TEST_RESULT();
}
